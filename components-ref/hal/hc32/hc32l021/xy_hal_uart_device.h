/**
 * @file xy_hal_uart_device.h
 * @brief HC32L021 UART Device Driver Header
 * @version 1.0.0
 * @date 2026-03-22
 */

#ifndef XY_HAL_UART_DEVICE_H
#define XY_HAL_UART_DEVICE_H

#include "xy_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

// UART device configuration structure
typedef struct {
    uint32_t baud_rate;
    uint8_t data_bits;
    uint8_t stop_bits;
    uint8_t parity;
    uint8_t flow_control;
} xy_uart_config_t;

// UART device structure
typedef struct xy_uart_device {
    uint8_t device_id;              // UART0, UART1, etc.
    xy_uart_config_t config;        // UART configuration
    xy_uart_callback_t callback;    // Optional callback function
    void *user_data;               // User data for callback
} xy_uart_device_t;

// Function prototypes
int xy_hal_uart_init(xy_uart_device_t *device);
int xy_hal_uart_deinit(xy_uart_device_t *device);
int xy_hal_uart_send(xy_uart_device_t *device, const void *data, size_t length);
int xy_hal_uart_receive(xy_uart_device_t *device, void *buffer, size_t length, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif // XY_HAL_UART_DEVICE_H