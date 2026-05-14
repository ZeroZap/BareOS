/**
 * @file process.h
 * @brief Contiki-NG-compatible process model for bare-metal.
 *
 * A process is a protothread that receives events.  The scheduler
 * (process_run, called each main-loop iteration) dispatches pending events
 * to their target processes and advances them.
 *
 * ── Declaring a process ──────────────────────────────────────────────
 *
 *   // In a .c file:
 *   PROCESS(gnss_process, "GNSS Task");
 *
 *   PROCESS_THREAD(gnss_process, ev, data)
 *   {
 *       PROCESS_BEGIN();
 *
 *       while (1) {
 *           PROCESS_WAIT_EVENT_UNTIL(ev == APP_EVT_GNSS_FIX);
 *           handle_fix((xy_gnss_pos_t *)data);
 *       }
 *
 *       PROCESS_END();
 *   }
 *
 * ── Starting / posting ───────────────────────────────────────────────
 *
 *   process_start(&gnss_process, NULL);       // in app_init()
 *   process_post(&gnss_process, APP_EVT_GNSS_FIX, &pos);   // from ISR or another process
 *
 * ── Main loop ────────────────────────────────────────────────────────
 *
 *   while (1) {
 *       at_obj_process(g_at_4g);
 *       ctimer_run();
 *       process_run();         // dispatches all pending events
 *       xy_event_dispatch();   // fires xy_event subscribers
 *   }
 *
 * ── Event IDs ────────────────────────────────────────────────────────
 *
 * The system reserves 0x00–0x0F.  Application event IDs start at
 * PROCESS_EVENT_USER (0x10).  They can be shared with xy_event IDs.
 */

#ifndef PROCESS_H
#define PROCESS_H

#include "pt.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Event type ─────────────────────────────────────────────────────── */

typedef uint8_t  process_event_t;
typedef void    *process_data_t;

#define PROCESS_EVENT_NONE      0x00u  /* no event (placeholder)         */
#define PROCESS_EVENT_INIT      0x01u  /* posted when process starts     */
#define PROCESS_EVENT_POLL      0x02u  /* process_poll() was called      */
#define PROCESS_EVENT_EXIT      0x03u  /* process is about to exit       */
#define PROCESS_EVENT_CONTINUE  0x04u  /* resume after PROCESS_PAUSE()   */
#define PROCESS_EVENT_TIMER     0x05u  /* etimer or ctimer expired       */
#define PROCESS_EVENT_MSG       0x06u  /* generic message                */
#define PROCESS_EVENT_USER      0x10u  /* application-defined IDs start  */

/* ── Process queue depth ─────────────────────────────────────────────── */

#ifndef PROCESS_QUEUE_SIZE
#define PROCESS_QUEUE_SIZE  16u       /* max pending events (power of 2) */
#endif

#ifndef PROCESS_MAX
#define PROCESS_MAX         16u       /* max registered processes        */
#endif

/* ── Process states ─────────────────────────────────────────────────── */

#define PROCESS_STATE_NONE      0u
#define PROCESS_STATE_RUNNING   1u
#define PROCESS_STATE_CALLED    2u

/* ── Process structure ──────────────────────────────────────────────── */

struct process {
    struct process   *next;     /* scheduler linked list      */
    const char       *name;
    /* Thread function: returns PT_ENDED (0) when finished.   */
    int (*thread)(struct pt *, process_event_t, process_data_t);
    struct pt         pt;
    uint8_t           state;
    bool              needspoll;
};

/* ── Declaration / definition macros ────────────────────────────────── */

/**
 * Declare an externally defined process.
 */
#define PROCESS_NAME(name) extern struct process name

/**
 * Define a process variable and its thread prototype.
 * @param name      C variable name for the process.
 * @param strname   Human-readable name string.
 */
#define PROCESS(name, strname)                                              \
    int process_thread_##name(struct pt *, process_event_t, process_data_t);\
    struct process name = { NULL, strname, process_thread_##name,          \
                            { 0 }, PROCESS_STATE_NONE, false }

/**
 * Open the thread function body.
 * @param name  Must match the name used in PROCESS().
 * @param ev    Receives the current event.
 * @param data  Receives event data pointer.
 */
#define PROCESS_THREAD(name, ev, data)                                      \
    int process_thread_##name(struct pt *process_pt,                        \
                              process_event_t ev,                           \
                              process_data_t  data)

/* ── Thread body macros ─────────────────────────────────────────────── */

#define PROCESS_BEGIN()         PT_BEGIN(process_pt)
#define PROCESS_END()           PT_END(process_pt)

/**
 * Yield until the next event is delivered (any event).
 */
#define PROCESS_WAIT_EVENT()    PT_YIELD(process_pt)

/**
 * Yield until @cond is true (checked at each event delivery).
 */
#define PROCESS_WAIT_EVENT_UNTIL(cond)  PT_WAIT_UNTIL(process_pt, cond)

/**
 * Yield once; resume at the next scheduler iteration.
 */
#define PROCESS_PAUSE()         PT_YIELD(process_pt)
#define PROCESS_YIELD()         PT_YIELD(process_pt)
#define PROCESS_YIELD_UNTIL(c)  PT_WAIT_UNTIL(process_pt, c)

/**
 * Exit the process immediately.
 */
#define PROCESS_EXIT()          PT_EXIT(process_pt)

/* ── Scheduler API ──────────────────────────────────────────────────── */

/**
 * Initialise the process scheduler.  Call once at system startup.
 */
void process_init(void);

/**
 * Start a process and post PROCESS_EVENT_INIT to it.
 * @param p     Process to start.
 * @param data  Initial data delivered with PROCESS_EVENT_INIT.
 */
void process_start(struct process *p, process_data_t data);

/**
 * Stop and remove a process.
 */
void process_exit(struct process *p);

/**
 * Post an event to a specific process.
 * @param p   Target process (NULL broadcasts to all running processes).
 * @param ev  Event type.
 * @param data Event payload pointer.
 * @return 0 on success, -1 if the event queue is full.
 */
int process_post(struct process *p, process_event_t ev, process_data_t data);

/**
 * Request an immediate poll of @p (posted before normal events).
 * ISR-safe.
 */
void process_poll(struct process *p);

/**
 * Run one iteration of the scheduler — dispatch pending events.
 * Call each main-loop iteration.
 * @return number of events dispatched.
 */
int process_run(void);

/**
 * Returns true if the process is currently running.
 */
bool process_is_running(const struct process *p);

/**
 * Returns the number of pending events in the queue.
 */
int process_nevents(void);

/**
 * Returns the process currently being dispatched (NULL if called outside
 * a process thread — e.g. from BSP init or ISR).
 * Used internally by etimer_set() to capture the owner process.
 */
struct process *process_current(void);

#ifdef __cplusplus
}
#endif

#endif /* PROCESS_H */
