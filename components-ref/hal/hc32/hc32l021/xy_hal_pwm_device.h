/**
 * @file xy_hal_pwm_device.h
 * @brief HC32L021 PWM Device Driver Header
 * @version 1.0.0
 * @date 2026-03-22
 */

#ifndef XY_HAL_PWM_DEVICE_H
#define XY_HAL_PWM_DEVICE_H

#include "xy_hal.h"
#include "../inc/xy_hal_pwm.h"

#ifdef __cplusplus
extern "C" {
#endif

// PWM device structure
typedef struct xy_pwm_device {
    uint8_t device_id;          // TMR0, TMR1, etc.
    xy_hal_pwm_config_t config;  // PWM configuration
    xy_hal_pwm_callback_t callback; // Optional callback function
    void *user_data;           // User data for callback
} xy_pwm_device_t;

// Function prototypes (these map to the standard HAL interface)
int xy_hal_pwm_init(void *device, const xy_hal_pwm_config_t *config);
int xy_hal_pwm_start(void *device);
int xy_hal_pwm_stop(void *device);
int xy_hal_pwm_set_duty_cycle(void *device, uint32_t duty_cycle);
int xy_hal_pwm_set_frequency(void *device, uint32_t frequency);

#ifdef __cplusplus
}
#endif

#endif // XY_HAL_PWM_DEVICE_H