/**
 * @file xy_net_platform.h
 * @brief XinYi Network Platform Abstraction
 */

#ifndef XY_NET_PLATFORM_H
#define XY_NET_PLATFORM_H

#include "xy_net_config.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get current tick count in milliseconds
 * @return Current tick count
 */
XY_NET_TICK_TYPE xy_net_get_tick(void);

/**
 * @brief Delay for specified milliseconds
 * @param ms Milliseconds to delay
 */
void xy_net_delay_ms(uint32_t ms);

/**
 * @brief Memory allocation wrapper
 * @param size Size to allocate
 * @return Pointer to allocated memory, or NULL on failure
 */
void* xy_net_malloc(size_t size);

/**
 * @brief Memory free wrapper  
 * @param ptr Pointer to free
 */
void xy_net_free(void* ptr);

#ifdef __cplusplus
}
#endif

#endif // XY_NET_PLATFORM_H