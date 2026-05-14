/**
 * @file process.c
 * @brief Contiki-NG-compatible process scheduler for bare-metal.
 *
 * Event queue: power-of-2 ring, ISR-safe (single write side, main-loop read).
 * Each process_run() call:
 *   1. Runs etimer_run() — posts PROCESS_EVENT_TIMER for expired etimers.
 *   2. Services poll requests (process_poll()).
 *   3. Dispatches all queued events to their target processes.
 */

#include "process.h"
#include "etimer.h"    /* etimer_run() */
#include <string.h>

/* ── Event queue ─────────────────────────────────────────────────────── */

typedef struct {
    struct process  *p;
    process_event_t  ev;
    process_data_t   data;
} ev_slot_t;

#define QUEUE_MASK  (PROCESS_QUEUE_SIZE - 1u)
typedef char _proc_pow2_chk[((PROCESS_QUEUE_SIZE & (PROCESS_QUEUE_SIZE-1u))==0)?1:-1];

static ev_slot_t         s_queue[PROCESS_QUEUE_SIZE];
static volatile uint32_t s_head;   /* producer — ISR-safe */
static volatile uint32_t s_tail;   /* consumer — main loop only */

/* ── Process list ────────────────────────────────────────────────────── */

static struct process *s_list;
static struct process *s_current;  /* process being dispatched right now */

/* ── Public accessor ────────────────────────────────────────────────── */

struct process *process_current(void)
{
    return s_current;
}

/* ── Helpers ─────────────────────────────────────────────────────────── */

static void list_add(struct process *p)
{
    p->next = s_list;
    s_list  = p;
}

static void list_remove(struct process *p)
{
    struct process **pp = &s_list;
    while (*pp) {
        if (*pp == p) { *pp = p->next; p->next = NULL; return; }
        pp = &(*pp)->next;
    }
}

static void call_process(struct process *p, process_event_t ev, process_data_t data)
{
    if (!p || p->state == PROCESS_STATE_NONE) return;

    struct process *saved = s_current;
    s_current = p;
    p->state  = PROCESS_STATE_CALLED;

    int ret = p->thread(&p->pt, ev, data);

    if (ret == PT_ENDED || ret == PT_EXITED) {
        /* Process thread has finished — remove from scheduler. */
        list_remove(p);
        p->state = PROCESS_STATE_NONE;
        /* Broadcast EXIT so other processes can react (optional). */
        process_post(NULL, PROCESS_EVENT_EXIT, (void *)p);
    } else {
        p->state = PROCESS_STATE_RUNNING;
    }

    s_current = saved;
}

/* ── Public API ─────────────────────────────────────────────────────── */

void process_init(void)
{
    s_list    = NULL;
    s_current = NULL;
    s_head    = 0;
    s_tail    = 0;
    memset(s_queue, 0, sizeof(s_queue));
}

void process_start(struct process *p, process_data_t data)
{
    if (!p || p->state != PROCESS_STATE_NONE) return;
    PT_INIT(&p->pt);
    p->state     = PROCESS_STATE_RUNNING;
    p->needspoll = false;
    list_add(p);
    call_process(p, PROCESS_EVENT_INIT, data);
}

void process_exit(struct process *p)
{
    if (!p || p->state == PROCESS_STATE_NONE) return;
    list_remove(p);
    p->state = PROCESS_STATE_NONE;
    process_post(NULL, PROCESS_EVENT_EXIT, (void *)p);
}

int process_post(struct process *p, process_event_t ev, process_data_t data)
{
    uint32_t head = s_head;
    if ((head + 1u - s_tail) > QUEUE_MASK) return -1;  /* full */

    s_queue[head & QUEUE_MASK].p    = p;
    s_queue[head & QUEUE_MASK].ev   = ev;
    s_queue[head & QUEUE_MASK].data = data;
    s_head = head + 1u;
    return 0;
}

void process_poll(struct process *p)
{
    if (p && p->state != PROCESS_STATE_NONE)
        p->needspoll = true;
}

int process_run(void)
{
    int n = 0;

    /* 1. Advance expired etimers → posts PROCESS_EVENT_TIMER. */
    etimer_run();

    /* 2. Service poll requests. */
    for (struct process *p = s_list; p; p = p->next) {
        if (p->needspoll) {
            p->needspoll = false;
            call_process(p, PROCESS_EVENT_POLL, NULL);
            n++;
        }
    }

    /* 3. Drain the event queue. */
    while (s_tail != s_head) {
        ev_slot_t slot = s_queue[s_tail & QUEUE_MASK];
        s_tail++;
        n++;

        if (slot.p == NULL) {
            /* Broadcast — deliver to every running process. */
            for (struct process *p = s_list; p; p = p->next)
                call_process(p, slot.ev, slot.data);
        } else {
            call_process(slot.p, slot.ev, slot.data);
        }
    }

    return n;
}

bool process_is_running(const struct process *p)
{
    return p && (p->state != PROCESS_STATE_NONE);
}

int process_nevents(void)
{
    return (int)(s_head - s_tail);
}
