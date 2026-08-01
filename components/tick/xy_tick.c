#include "xy_tick.h"

volatile xy_tick_t g_xy_tick;

void xy_tick_init(void)
{
    g_xy_tick = 0;
}

xy_tick_t xy_tick_now(void)
{
    return g_xy_tick;
}

void xy_tick_set(xy_tick_t tick)
{
    g_xy_tick = tick;
}

void xy_tick_advance(xy_tick_t ticks)
{
    g_xy_tick += ticks;
}

xy_tick_t xy_tick_from_ms(uint32_t ms)
{
    uint64_t ticks = ((uint64_t)ms * (uint64_t)XY_TICK_HZ + 999ULL) / 1000ULL;
    return (xy_tick_t)ticks;
}

uint32_t xy_tick_to_ms(xy_tick_t ticks)
{
    return (uint32_t)(((uint64_t)ticks * 1000ULL) / (uint64_t)XY_TICK_HZ);
}

uint32_t xy_tick_now_ms(void)
{
    return xy_tick_to_ms(xy_tick_now());
}

uint32_t xy_tick_now_s(void)
{
    return (uint32_t)(xy_tick_now() / (xy_tick_t)XY_TICK_HZ);
}
