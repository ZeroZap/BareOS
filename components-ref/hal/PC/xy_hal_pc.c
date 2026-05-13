#include "xy_hal_sys.h"
/**
 * @file xy_hal_pc.c
 * @brief PC/Linux HAL simulation - common functions
 */

#include "xy_hal.h"
#include <stdint.h>
#include <unistd.h>

/**
 * @brief Delay in milliseconds (PC simulation)
 */
void xy_hal_delay_ms(uint32_t ms)
{
    usleep(ms * 1000);
}

/**
 * @brief Get system tick count (PC simulation)
 */
uint32_t xy_hal_sys_get_tick_count(void)
{
    static uint32_t tick = 0;
    tick += 10; /* Simulate 10ms ticks */
    return tick;
}

/**
 * @brief Get system tick frequency
 */
uint32_t xy_hal_sys_get_tick_freq(void)
{
    return XY_HAL_SYS_TICK_FREQ;
}

/**
 * @brief Get clock info (PC simulation - dummy values)
 */
xy_hal_error_t xy_hal_sys_get_clock_info(xy_hal_sys_clock_info_t *info)
{
    if (!info) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    /* Dummy values for PC simulation */
    info->sysclk = 100000000;  /* 100 MHz */
    info->hclk = 100000000;
    info->pclk1 = 50000000;
    info->pclk2 = 50000000;
    info->pclk3 = 50000000;
    info->hsi_ready = 1;
    info->hse_ready = 0;
    info->pll_ready = 1;
    
    return XY_HAL_OK;
}
