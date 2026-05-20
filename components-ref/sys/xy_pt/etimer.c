/**
 * @file etimer.c
 */

#include "etimer.h"
#include "kernel/osal/inc/xy_os_tick.h"  /* xy_os_get_tick_ms() */

/* ── tick provider ─────────────────────────────────────────────────── */

uint32_t etimer_now_ms(void)
{
    return xy_os_get_tick_ms();
}

/* ── API ────────────────────────────────────────────────────────────── */

void etimer_set(struct etimer *et, uint32_t interval_ms)
{
    et->start_tick   = etimer_now_ms();
    et->interval_ms  = interval_ms;
    et->armed        = true;
}

void etimer_stop(struct etimer *et)
{
    et->armed = false;
}

void etimer_reset(struct etimer *et)
{
    et->start_tick  = etimer_now_ms();
    et->armed       = true;
}

void etimer_restart(struct etimer *et)
{
    /* Advance start by exactly one interval to avoid drift. */
    et->start_tick += et->interval_ms;
    et->armed       = true;
}

bool etimer_expired(const struct etimer *et)
{
    if (!et->armed) {
        return false;
    }
    uint32_t elapsed = etimer_now_ms() - et->start_tick;
    return elapsed >= et->interval_ms;
}

uint32_t etimer_remaining_ms(const struct etimer *et)
{
    if (!et->armed) {
        return 0;
    }
    uint32_t elapsed = etimer_now_ms() - et->start_tick;
    if (elapsed >= et->interval_ms) {
        return 0;
    }
    return et->interval_ms - elapsed;
}

uint32_t etimer_interval_ms(const struct etimer *et)
{
    return et->interval_ms;
}
