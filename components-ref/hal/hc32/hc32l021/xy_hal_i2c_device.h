/**
 * @file xy_hal_i2c_device.h
 * @brief HC32L021 I2C Device Driver Header
 * @version 1.0.0
 * @date 2026-03-22
 */

#ifndef XY_HAL_I2C_DEVICE_H
#define XY_HAL_I2C_DEVICE_H

#include "xy_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

// I2C address modes
#define XY_I2C_ADDRESS_MODE_7BIT    (0)
#define XY_I2C_ADDRESS_MODE_10BIT   (1)

// I2C baud rates
#define XY_I2C_BAUD_RATE_STANDARD   (100000)   // 100 kHz
#define XY_I2C_BAUD_RATE_FAST       (400000)   // 400 kHz
#define XY_I2C_BAUD_RATE_FAST_PLUS  (1000000)  // 1 MHz

// I2C master/slave
#define XY_I2C_MASTER               (1)
#define XY_I2C_SLAVE                (0)

// I2C events
#define XY_I2C_EVENT_TRANSFER_COMPLETE  (1)
#define XY_I2C_EVENT_ERROR              (2)
#define XY_I2C_EVENT_ADDRESS_MATCH      (3)

// I2C device configuration structure
typedef struct {
    uint8_t address_mode;       // 7-bit or 10-bit addressing
    uint32_t baud_rate;         // Baud rate in Hz
    uint8_t master_slave;       // Master (1) or Slave (0)
    uint16_t own_address;       // Own address (for slave mode)
} xy_i2c_config_t;

// I2C callback function type
typedef void (*xy_i2c_callback_t)(struct xy_i2c_device *device, uint32_t event);

// I2C device structure
typedef struct xy_i2c_device {
    uint8_t device_id;          // I2C0, I2C1, etc.
    xy_i2c_config_t config;     // I2C configuration
    xy_i2c_callback_t callback; // Optional callback function
    void *user_data;           // User data for callback
} xy_i2c_device_t;

// Function prototypes
int xy_hal_i2c_init(xy_i2c_device_t *device);
int xy_hal_i2c_deinit(xy_i2c_device_t *device);
int xy_hal_i2c_master_send(xy_i2c_device_t *device, uint16_t slave_address, 
                          const void *data, size_t length, uint32_t timeout_ms);
int xy_hal_i2c_master_receive(xy_i2c_device_t *device, uint16_t slave_address,
                             void *buffer, size_t length, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif // XY_HAL_I2C_DEVICE_H