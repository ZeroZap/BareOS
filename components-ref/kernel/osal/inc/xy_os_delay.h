/**
 * @file xy_os_delay.h
 * @brief XY OSAL Delay Interface
 * @version 1.0.0
 */

#ifndef XY_OS_DELAY_H
#define XY_OS_DELAY_H

#include "xy_os_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Delay for specified number of ticks
 * @param ticks Number of ticks to delay
 * @return Status code
 */
xy_os_status_t xy_os_delay(uint32_t ticks);

/**
 * @brief Delay until specified tick count
 * @param ticks Target tick count
 * @return Status code
 */
xy_os_status_t xy_os_delay_until(uint32_t ticks);

#ifdef __cplusplus
}
#endif

#endif // XY_OS_DELAY_H