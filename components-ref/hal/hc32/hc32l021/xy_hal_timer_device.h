/**
 * @file xy_hal_timer_device.h
 * @brief HC32L021 Timer Device Driver Header
 * @version 1.0.0
 * @date 2026-03-22
 */

#ifndef XY_HAL_TIMER_DEVICE_H
#define XY_HAL_TIMER_DEVICE_H

#include "xy_hal.h"
#include "../inc/xy_hal_timer.h"

#ifdef __cplusplus
extern "C" {
#endif

// Timer device structure
typedef struct xy_timer_device {
    uint8_t device_id;          // TMR0, TMR1, etc.
    xy_hal_timer_config_t config; // Timer configuration
    xy_hal_timer_callback_t callback; // Optional callback function
    void *user_data;           // User data for callback
} xy_timer_device_t;

// Function prototypes (these map to the standard HAL interface)
int xy_hal_timer_init(void *device, const xy_hal_timer_config_t *config);
int xy_hal_timer_start(void *device);
int xy_hal_timer_stop(void *device);
uint32_t xy_hal_timer_get_counter(void *device);
int xy_hal_timer_set_counter(void *device, uint32_t value);

#ifdef __cplusplus
}
#endif

#endif // XY_HAL_TIMER_DEVICE_H