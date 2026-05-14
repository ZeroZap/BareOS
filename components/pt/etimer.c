/**
 * @file etimer.c
 * @brief Event timer — active-list management + process event posting.
 *
 * Active etimers are kept in an intrusive singly-linked list.
 * etimer_run() (called by process_run) walks the list and posts
 * PROCESS_EVENT_TIMER to expired timers' owner processes.
 */

#include "etimer.h"
#include "process.h"   /* process_current(), process_post(), PROCESS_EVENT_TIMER */

/* ── Tick source ─────────────────────────────────────────────────────── */

extern volatile unsigned int g_sys_tick_ms;

uint32_t etimer_now_ms(void)
{
    return (uint32_t)g_sys_tick_ms;
}

/* ── Active list ─────────────────────────────────────────────────────── */

static struct etimer *s_active;

static void list_add(struct etimer *et)
{
    /* Remove first to avoid duplicates */
    struct etimer **pp = &s_active;
    while (*pp) {
        if (*pp == et) { *pp = et->next; break; }
        pp = &(*pp)->next;
    }
    et->next = s_active;
    s_active  = et;
}

static void list_remove(struct etimer *et)
{
    struct etimer **pp = &s_active;
    while (*pp) {
        if (*pp == et) { *pp = et->next; et->next = NULL; return; }
        pp = &(*pp)->next;
    }
}

/* ── API ─────────────────────────────────────────────────────────────── */

void etimer_set(struct etimer *et, uint32_t interval_ms)
{
    et->start_tick  = etimer_now_ms();
    et->interval_ms = interval_ms;
    et->armed       = true;
    et->owner       = process_current(); /* NULL when called outside a process */
    list_add(et);
}

void etimer_stop(struct etimer *et)
{
    et->armed = false;
    list_remove(et);
}

void etimer_reset(struct etimer *et)
{
    et->start_tick = etimer_now_ms();
    et->armed      = true;
    list_add(et);
}

void etimer_restart(struct etimer *et)
{
    et->start_tick += et->interval_ms;  /* advance by one period — no drift */
    et->armed       = true;
    list_add(et);
}

bool etimer_expired(const struct etimer *et)
{
    if (!et->armed) return false;
    return (etimer_now_ms() - et->start_tick) >= et->interval_ms;
}

uint32_t etimer_remaining_ms(const struct etimer *et)
{
    if (!et->armed) return 0;
    uint32_t elapsed = etimer_now_ms() - et->start_tick;
    return (elapsed >= et->interval_ms) ? 0u : (et->interval_ms - elapsed);
}

uint32_t etimer_interval_ms(const struct etimer *et)
{
    return et->interval_ms;
}

/* ── etimer_run — called by process_run() ──────────────────────────── */

void etimer_run(void)
{
    struct etimer *et = s_active;

    while (et) {
        struct etimer *next = et->next;

        if (et->armed && etimer_expired(et)) {
            list_remove(et);
            et->armed = false;

            if (et->owner) {
                /* Post to the owning process — PROCESS_WAIT_EVENT_UNTIL
                 * checks (ev == PROCESS_EVENT_TIMER && data == &my_et). */
                process_post(et->owner, PROCESS_EVENT_TIMER, (void *)et);
            }
        }
        et = next;
    }
}
