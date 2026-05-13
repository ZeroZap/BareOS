/**
 * @file xy_os_tick.c
 * @brief XY OSAL Tick Implementation
 * @version 1.0.0
 */

#include "xy_os_tick.h"
#include "../misc/xy_tick.h"

uint32_t xy_os_tick_get(void)
{
    return xy_tick_get();
}

uint32_t xy_os_tick_freq(void)
{
    return 1000; // Default 1kHz
}