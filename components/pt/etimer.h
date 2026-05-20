/**
 * @file etimer.h
 * @brief Event timer — integrates with the process model and protothread scheduler.
 *
 * When an etimer expires, etimer_run() posts PROCESS_EVENT_TIMER to the
 * process that called etimer_set(), so PROCESS_WAIT_EVENT_UNTIL works:
 *
 *   static struct etimer t;
 *
 *   PROCESS_THREAD(my_proc, ev, data)
 *   {
 *       PROCESS_BEGIN();
 *       etimer_set(&t, 500);
 *       PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER && data == &t);
 *       // 500 ms elapsed
 *       PROCESS_END();
 *   }
 *
 * For use outside a process (pure polling, no event posting), call
 * etimer_set() from main() or BSP code — owner will be NULL and no event
 * is posted; use etimer_expired() to poll.
 *
 * etimer_run() is called automatically by process_run().  Do not call it
 * manually unless you are not using the process scheduler.
 */

#ifndef XY_ETIMER_H
#define XY_ETIMER_H

#include <stdint.h>
#include "xy_typedef.h"

/* Forward-declare process to avoid a circular include. */
struct process;

#ifdef __cplusplus
extern "C" {
#endif

/* ── Handle ─────────────────────────────────────────────────────────── */

struct etimer {
    struct etimer  *next;        /* intrusive active-list link           */
    struct process *owner;       /* process to notify on expiry (or NULL)*/
    uint32_t        start_tick;
    uint32_t        interval_ms;
    bool            armed;
};

/* ── API ─────────────────────────────────────────────────────────────── */

/**
 * Arm the timer for @interval_ms ms.
 * Captures the currently executing process as the owner so that
 * PROCESS_EVENT_TIMER is posted to it when the timer expires.
 * Calling on an already-armed timer restarts it.
 */
void etimer_set(struct etimer *et, uint32_t interval_ms);

/** Disarm the timer and remove it from the active list. */
void etimer_stop(struct etimer *et);

/** Restart measured from now (same interval). */
void etimer_reset(struct etimer *et);

/** Restart from the previous expiry point — no drift for periodic timers. */
void etimer_restart(struct etimer *et);

/** True if armed and the interval has elapsed. */
bool etimer_expired(const struct etimer *et);

/** Milliseconds remaining until expiry (0 if expired or disarmed). */
uint32_t etimer_remaining_ms(const struct etimer *et);

/** Configured interval in ms. */
uint32_t etimer_interval_ms(const struct etimer *et);

/**
 * Walk all active etimers; for each expired one post PROCESS_EVENT_TIMER
 * to its owner process (if set) and remove it from the active list.
 * Called automatically by process_run() — call manually only when not
 * using the process scheduler.
 */
void etimer_run(void);

/* ── Tick source ─────────────────────────────────────────────────────── */

uint32_t etimer_now_ms(void);

#ifdef __cplusplus
}
#endif

#endif /* XY_ETIMER_H */
