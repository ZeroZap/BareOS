/**
 * @file ctimer.h
 * @brief Callback timer — fires a function when the interval elapses.
 *
 * ctimer sits on top of etimer but adds a callback instead of requiring the
 * caller to poll.  Active ctimers are kept in an intrusive singly-linked list;
 * call ctimer_run() once per main-loop iteration to service expired timers.
 *
 * Usage:
 *   static struct ctimer blink_timer;
 *
 *   static void blink_cb(void *arg) {
 *       led_toggle();
 *       ctimer_reset(&blink_timer);   // restart same interval
 *   }
 *
 *   // In init:
 *   ctimer_set(&blink_timer, 500, blink_cb, NULL);
 *
 *   // In main loop:
 *   ctimer_run();
 */

#ifndef CTIMER_H
#define CTIMER_H

#include "etimer.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Handle ─────────────────────────────────────────────────────────── */

struct ctimer {
    struct ctimer  *next;       /* intrusive linked list — do not touch */
    struct etimer   et;         /* underlying event timer               */
    void          (*f)(void *); /* callback                             */
    void           *ptr;        /* callback argument                    */
};

/* ── API ─────────────────────────────────────────────────────────────── */

/**
 * Arm the callback timer.
 * If already active, restarts with the new interval and callback.
 *
 * @param ct           Timer handle (caller-allocated, must stay valid).
 * @param interval_ms  Interval in milliseconds.
 * @param f            Callback invoked on expiry.
 * @param ptr          Argument forwarded to @f.
 */
void ctimer_set(struct ctimer *ct, uint32_t interval_ms,
                void (*f)(void *), void *ptr);

/**
 * Restart the timer using its original interval measured from the
 * previous expiry point (no drift accumulation).
 */
void ctimer_reset(struct ctimer *ct);

/**
 * Restart the timer with its original interval measured from now.
 */
void ctimer_restart(struct ctimer *ct);

/**
 * Cancel the timer.  Safe to call on a stopped timer.
 */
void ctimer_stop(struct ctimer *ct);

/**
 * Returns true if the timer has expired (or was never started).
 */
bool ctimer_expired(const struct ctimer *ct);

/**
 * Walk the active list and invoke callbacks for all expired timers.
 * Call once per main-loop iteration.
 * Returns the number of callbacks fired.
 */
int ctimer_run(void);

#ifdef __cplusplus
}
#endif

#endif /* CTIMER_H */
