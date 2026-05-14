/**
 * @file rtimer.h
 * @brief Real-time timer — single-shot, ISR-fired, sub-millisecond precision.
 *
 * rtimer fires its callback directly from a hardware timer ISR (or from a
 * high-priority interrupt).  It is suitable for tight real-time tasks where
 * even a 1 ms jitter from the main loop is unacceptable.
 *
 * The BSP must implement:
 *   - rtimer_arch_init()       — configure the hardware timer
 *   - rtimer_arch_now()        — return current hardware ticks
 *   - rtimer_arch_schedule(t)  — set the hardware compare register to fire at tick t
 *
 * The hardware ISR must call rtimer_run() when the compare fires.
 *
 * Usage:
 *   static struct rtimer rt;
 *
 *   static void my_rt_cb(struct rtimer *t, void *ptr) {
 *       // execute tight real-time action
 *       // re-arm if needed:
 *       rtimer_set(t, rtimer_now() + RTIMER_SECOND / 100, my_rt_cb, ptr);
 *   }
 *
 *   rtimer_set(&rt, rtimer_now() + RTIMER_SECOND / 10, my_rt_cb, NULL);
 *
 * RTIMER_SECOND is the number of hardware ticks per second (BSP-defined).
 */

#ifndef RTIMER_H
#define RTIMER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Tick type ──────────────────────────────────────────────────────── */

typedef uint32_t rtimer_clock_t;

/**
 * Hardware ticks per second.  The BSP must define this to match the
 * hardware timer frequency.  Default: 1 000 000 (1 MHz → 1 µs per tick).
 */
#ifndef RTIMER_SECOND
#define RTIMER_SECOND  1000000UL
#endif

/* ── Handle ─────────────────────────────────────────────────────────── */

struct rtimer;
typedef void (*rtimer_callback_t)(struct rtimer *t, void *ptr);

struct rtimer {
    rtimer_clock_t    time;   /* absolute tick value when it fires */
    rtimer_callback_t func;
    void             *ptr;
};

/* ── API ─────────────────────────────────────────────────────────────── */

/**
 * Initialise the rtimer module (calls rtimer_arch_init()).
 * Call once at BSP init.
 */
void rtimer_init(void);

/**
 * Schedule @func to be called at hardware tick @time.
 * Only ONE rtimer can be active at a time.
 *
 * @return 0 on success, -1 if @time is already in the past.
 */
int rtimer_set(struct rtimer *t, rtimer_clock_t time,
               rtimer_callback_t func, void *ptr);

/**
 * Return the current hardware tick counter.
 * Thin wrapper around rtimer_arch_now().
 */
rtimer_clock_t rtimer_now(void);

/**
 * Called from the hardware timer ISR when the compare fires.
 * Invokes the registered callback.
 */
void rtimer_run(void);

/* ── BSP hooks (implement in BSP) ───────────────────────────────────── */

void           rtimer_arch_init(void);
rtimer_clock_t rtimer_arch_now(void);
void           rtimer_arch_schedule(rtimer_clock_t when);

#ifdef __cplusplus
}
#endif

#endif /* RTIMER_H */
