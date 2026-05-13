/**
 * @file xy_os_tick.h
 * @brief XY OSAL Tick Interface
 * @version 1.0.0
 */

#ifndef XY_OS_TICK_H
#define XY_OS_TICK_H

#include "xy_os_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get current OS tick count
 * @return Current tick count
 */
uint32_t xy_os_tick_get(void);

/**
 * @brief Get OS tick frequency in Hz
 * @return Tick frequency in Hz
 */
uint32_t xy_os_tick_freq(void);

#ifdef __cplusplus
}
#endif

#endif // XY_OS_TICK_H