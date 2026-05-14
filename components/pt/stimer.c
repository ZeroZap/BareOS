/**
 * @file stimer.c
 * @brief Seconds-granularity polling timer.
 */

#include "stimer.h"
#include "etimer.h"   /* for etimer_now_ms() */

uint32_t stimer_now_s(void)
{
    return etimer_now_ms() / 1000u;
}

void stimer_set(struct stimer *st, uint32_t interval_s)
{
    st->start_s    = stimer_now_s();
    st->interval_s = interval_s;
}

bool stimer_expired(const struct stimer *st)
{
    return (stimer_now_s() - st->start_s) >= st->interval_s;
}

uint32_t stimer_elapsed(const struct stimer *st)
{
    return stimer_now_s() - st->start_s;
}

uint32_t stimer_remaining(const struct stimer *st)
{
    uint32_t elapsed = stimer_now_s() - st->start_s;
    return (elapsed >= st->interval_s) ? 0u : (st->interval_s - elapsed);
}

void stimer_reset(struct stimer *st)
{
    /* Advance start by one interval — no drift for periodic timers */
    st->start_s += st->interval_s;
}

void stimer_restart(struct stimer *st)
{
    st->start_s = stimer_now_s();
}
