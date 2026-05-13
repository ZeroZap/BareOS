/**
 * @file xy_hal_adc_device.h
 * @brief HC32L021 ADC Device Driver Header
 * @version 1.0.0
 * @date 2026-03-22
 */

#ifndef XY_HAL_ADC_DEVICE_H
#define XY_HAL_ADC_DEVICE_H

#include "xy_hal.h"
#include "../inc/xy_hal_adc.h"

#ifdef __cplusplus
extern "C" {
#endif

// ADC device structure
typedef struct xy_adc_device {
    uint8_t device_id;          // ADC0, etc.
    xy_hal_adc_config_t config;  // ADC configuration
    xy_hal_adc_callback_t callback; // Optional callback function
    void *user_data;           // User data for callback
} xy_adc_device_t;

// Function prototypes (these map to the standard HAL interface)
int xy_hal_adc_init(void *device, const xy_hal_adc_config_t *config);
int xy_hal_adc_deinit(void *device);
int32_t xy_hal_adc_read(void *device, uint8_t channel, uint32_t timeout);

#ifdef __cplusplus
}
#endif

#endif // XY_HAL_ADC_DEVICE_H