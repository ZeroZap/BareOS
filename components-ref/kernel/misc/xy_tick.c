/**
 * @file xy_tick.c
 * @brief XinYi System Tick Module Implementation
 * @version 1.0.0
 * @date 2026-02-28
 */

#include "xy_tick.h"

static volatile uint32_t g_tick_count = 0;
static uint32_t g_tick_freq = 1000;  /* Default 1kHz (1ms) */

void xy_tick_init(uint32_t tick_freq_hz)
{
    g_tick_count = 0;
    g_tick_freq = tick_freq_hz;
}

uint32_t xy_tick_get(void)
{
    return g_tick_count;
}

uint32_t xy_tick_get_freq(void)
{
    return g_tick_freq;
}

void xy_tick_increment(void)
{
    g_tick_count++;
}

void xy_tick_set(uint32_t tick)
{
    g_tick_count = tick;
}

void xy_tick_delay(uint32_t ticks)
{
    uint32_t start = g_tick_count;
    while ((g_tick_count - start) < ticks) {
        /* Busy wait */
    }
}

void xy_tick_delay_until(uint32_t tick)
{
    uint32_t now = g_tick_count;
    if (tick > now) {
        xy_tick_delay(tick - now);
    }
}

void xy_tick_get_safe(uint32_t *tick)
{
    if (tick) {
        *tick = g_tick_count;
    }
}
