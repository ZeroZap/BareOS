/**
 * @file etimer.h
 * @brief Event timer — PT-friendly one-shot timers driven by xy_timer.
 *
 * etimer integrates with the Protothread model:
 *
 *   static struct etimer t;
 *
 *   PT_THREAD(my_task(struct pt *pt)) {
 *       PT_BEGIN(pt);
 *       etimer_set(&t, 500);                   // arm 500 ms
 *       PT_WAIT_UNTIL(pt, etimer_expired(&t)); // suspend until expired
 *       etimer_reset(&t);                      // restart same interval
 *       PT_END(pt);
 *   }
 *
 * Internally uses xy_timer (sys/xy_timer) so the same SysTick ISR that
 * drives software timers also drives etimers — no additional hardware needed.
 */

#ifndef XY_ETIMER_H
#define XY_ETIMER_H

#include <stdint.h>
#include <stdbool.h>

/* ────────────────────────────────────────────────
 * etimer handle
 * ──────────────────────────────────────────────── */

struct etimer {
    uint32_t start_tick;   /* tick value when armed           */
    uint32_t interval_ms;  /* requested interval in ms        */
    bool     armed;        /* false = never set / already stopped */
};

/* ────────────────────────────────────────────────
 * API
 * ──────────────────────────────────────────────── */

/**
 * Arm the timer to expire after interval_ms milliseconds.
 * Calling on an already-armed timer restarts it.
 */
void etimer_set(struct etimer *et, uint32_t interval_ms);

/**
 * Stop (disarm) the timer.  etimer_expired() returns false after this.
 */
void etimer_stop(struct etimer *et);

/**
 * Restart the timer with its original interval (measured from now).
 * Safe to call even if the timer has already expired.
 */
void etimer_reset(struct etimer *et);

/**
 * Restart using the original expiry point as the new start (no drift).
 * Useful for fixed-period polling without accumulating jitter.
 */
void etimer_restart(struct etimer *et);

/**
 * Returns true if the timer was armed and the interval has elapsed.
 * A stopped timer always returns false.
 */
bool etimer_expired(const struct etimer *et);

/**
 * Returns the number of milliseconds remaining until expiry.
 * Returns 0 if already expired or not armed.
 */
uint32_t etimer_remaining_ms(const struct etimer *et);

/**
 * Returns the configured interval in ms (useful after reset/restart).
 */
uint32_t etimer_interval_ms(const struct etimer *et);

/* ────────────────────────────────────────────────
 * Tick provider — must be implemented by the platform.
 * Default implementation calls xy_os_get_tick_ms()
 * (defined in kernel/osal).
 * ──────────────────────────────────────────────── */

uint32_t etimer_now_ms(void);

#endif /* XY_ETIMER_H */
