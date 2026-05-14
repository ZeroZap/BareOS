/**
 * @file stimer.h
 * @brief Seconds-granularity polling timer.
 *
 * stimer is designed for long-duration timeouts where millisecond precision
 * is not needed (minutes, hours).  It uses the same g_sys_tick_ms source
 * as etimer but expresses time in whole seconds to avoid 32-bit overflow
 * when tracking multi-hour intervals.
 *
 *   stimer_set(&t, 300);               // 5-minute timeout
 *   if (stimer_expired(&t)) { ... }    // poll anywhere
 *
 * No callbacks, no linked list — pure polling, zero overhead.
 */

#ifndef STIMER_H
#define STIMER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Handle ─────────────────────────────────────────────────────────── */

struct stimer {
    uint32_t start_s;     /* wall-clock seconds when armed      */
    uint32_t interval_s;  /* duration in seconds                */
};

/* ── API ─────────────────────────────────────────────────────────────── */

/**
 * Arm the timer to expire after @interval_s seconds.
 */
void stimer_set(struct stimer *st, uint32_t interval_s);

/**
 * Returns true if the timer has elapsed.
 */
bool stimer_expired(const struct stimer *st);

/**
 * Seconds elapsed since the timer was set (or last reset).
 */
uint32_t stimer_elapsed(const struct stimer *st);

/**
 * Seconds remaining until expiry (0 if already expired).
 */
uint32_t stimer_remaining(const struct stimer *st);

/**
 * Restart with the same interval, measured from the previous start
 * (no drift accumulation for periodic timers).
 */
void stimer_reset(struct stimer *st);

/**
 * Restart with the same interval measured from now.
 */
void stimer_restart(struct stimer *st);

/**
 * Seconds since boot — provided by the platform.
 * Default implementation derives from etimer_now_ms() / 1000.
 */
uint32_t stimer_now_s(void);

#ifdef __cplusplus
}
#endif

#endif /* STIMER_H */
