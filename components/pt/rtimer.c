/**
 * @file rtimer.c
 * @brief Real-time timer.
 *
 * Only one rtimer can be pending at a time (matches Contiki-NG model).
 * The BSP provides rtimer_arch_init/now/schedule; this file only holds
 * the registered callback and calls it when rtimer_run() fires from ISR.
 */

#include "rtimer.h"
#include "xy_typedef.h"

static struct rtimer *s_pending;

void rtimer_init(void)
{
    s_pending = NULL;
    rtimer_arch_init();
}

int rtimer_set(struct rtimer *t, rtimer_clock_t time,
               rtimer_callback_t func, void *ptr)
{
    t->time = time;
    t->func = func;
    t->ptr  = ptr;
    s_pending = t;
    rtimer_arch_schedule(time);
    return 0;
}

rtimer_clock_t rtimer_now(void)
{
    return rtimer_arch_now();
}

void rtimer_run(void)
{
    struct rtimer *t = s_pending;
    if (!t) return;
    s_pending = NULL;
    if (t->func) {
        t->func(t, t->ptr);
    }
}

/* ── Weak BSP stubs ─────────────────────────────────────────────────── */

__attribute__((weak)) void rtimer_arch_init(void)
{
    /* BSP: configure hardware timer for rtimer */
}

__attribute__((weak)) rtimer_clock_t rtimer_arch_now(void)
{
    /* BSP: return hardware timer counter value */
    return 0;
}

__attribute__((weak)) void rtimer_arch_schedule(rtimer_clock_t when)
{
    /* BSP: load compare register so ISR fires at 'when' ticks */
    (void)when;
}
