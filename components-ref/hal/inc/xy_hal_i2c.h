/**
 * @file xy_hal_i2c.h
 * @brief I2C Hardware Abstraction Layer
 * @version 2.0
 * @date 2025-10-26
 */

#ifndef XY_HAL_I2C_H
#define XY_HAL_I2C_H
#include "xy_hal_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_hal.h"
#include <stddef.h>
#include <stdint.h>

/* I2C Addressing Mode */
typedef enum {
  XY_HAL_I2C_ADDR_7BIT = 0, /**< 7-bit addressing */
  XY_HAL_I2C_ADDR_10BIT,    /**< 10-bit addressing */
} xy_hal_i2c_addr_mode_t;

/* I2C Duty Cycle (for fast mode) */
typedef enum {
  XY_HAL_I2C_DUTY_2 = 0, /**< Tlow/Thigh = 2 */
  XY_HAL_I2C_DUTY_16_9,  /**< Tlow/Thigh = 16/9 */
} xy_hal_i2c_duty_t;

/* I2C Configuration Structure */
typedef struct {
  uint32_t clock_speed; /**< Clock speed in Hz (e.g., 100000 for 100kHz) */
  xy_hal_i2c_addr_mode_t addr_mode; /**< Addressing mode */
  xy_hal_i2c_duty_t duty_cycle;     /**< Duty cycle for fast mode */
  uint16_t own_address;             /**< Own address (slave mode) */
  uint8_t general_call_mode; /**< Enable general call: 1=enable, 0=disable */
} xy_hal_i2c_config_t;

/* I2C Event Types */
typedef enum {
  XY_HAL_I2C_EVENT_TX_DONE = 0, /**< TX complete */
  XY_HAL_I2C_EVENT_RX_DONE,     /**< RX complete */
  XY_HAL_I2C_EVENT_ERROR,       /**< Error occurred */
} xy_hal_i2c_event_t;

/* I2C callback */
typedef void (*xy_hal_i2c_callback_t)(void *i2c, xy_hal_i2c_event_t event,
                                      void *arg);

/**
 * @brief Initialize I2C
 * @param i2c I2C instance
 * @param config I2C configuration
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_i2c_init(void *i2c, const xy_hal_i2c_config_t *config);

/**
 * @brief Deinitialize I2C
 * @param i2c I2C instance
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_i2c_deinit(void *i2c);

/**
 * @brief Transmit data via I2C (blocking)
 * @param i2c I2C instance
 * @param dev_addr Device address
 * @param data Data buffer
 * @param len Data length
 * @param timeout Timeout in milliseconds
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_i2c_master_transmit(void *i2c, uint16_t dev_addr,
                                          const uint8_t *data, size_t len,
                                          uint32_t timeout);

/**
 * @brief Receive data via I2C (blocking)
 * @param i2c I2C instance
 * @param dev_addr Device address
 * @param data Data buffer
 * @param len Buffer length
 * @param timeout Timeout in milliseconds
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_i2c_master_receive(void *i2c, uint16_t dev_addr,
                                         uint8_t *data, size_t len,
                                         uint32_t timeout);

/**
 * @brief Write data to register via I2C (blocking)
 * @param i2c I2C instance
 * @param dev_addr Device address
 * @param reg_addr Register address
 * @param data Data buffer
 * @param len Data length
 * @param timeout Timeout in milliseconds
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_i2c_mem_write(void *i2c, uint16_t dev_addr,
                                    uint16_t reg_addr, const uint8_t *data,
                                    size_t len, uint32_t timeout);

/**
 * @brief Read data from register via I2C (blocking)
 * @param i2c I2C instance
 * @param dev_addr Device address
 * @param reg_addr Register address
 * @param data Data buffer
 * @param len Buffer length
 * @param timeout Timeout in milliseconds
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_i2c_mem_read(void *i2c, uint16_t dev_addr,
                                   uint16_t reg_addr, uint8_t *data, size_t len,
                                   uint32_t timeout);

/**
 * @brief Transmit data via I2C (non-blocking with DMA)
 * @param i2c I2C instance
 * @param dev_addr Device address
 * @param data Data buffer
 * @param len Data length
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_i2c_master_transmit_dma(void *i2c, uint16_t dev_addr,
                                              const uint8_t *data, size_t len);

/**
 * @brief Receive data via I2C (non-blocking with DMA)
 * @param i2c I2C instance
 * @param dev_addr Device address
 * @param data Data buffer
 * @param len Buffer length
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_i2c_master_receive_dma(void *i2c, uint16_t dev_addr,
                                             uint8_t *data, size_t len);

/**
 * @brief Register I2C callback
 * @param i2c I2C instance
 * @param callback Callback function
 * @param arg User argument
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_i2c_register_callback(void *i2c,
                                            xy_hal_i2c_callback_t callback,
                                            void *arg);

/**
 * @brief Check if device is ready
 * @param i2c I2C instance
 * @param dev_addr Device address
 * @param trials Number of trials
 * @param timeout Timeout in milliseconds
 * @return 0 if device is ready, negative on error
 */
xy_hal_error_t xy_hal_i2c_is_device_ready(void *i2c, uint16_t dev_addr,
                                          uint32_t trials, uint32_t timeout);

/**
 * @brief Control I2C error pin (if available)
 * @param i2c I2C instance
 * @return XY_HAL_OK on success, XY_HAL_ERROR_INVALID_PARAM if parameters
 * invalid
 */
xy_hal_error_t xy_hal_i2c_error(void *i2c);

/**
 * @brief Set I2C error callback
 * @param i2c I2C instance
 * @param callback Callback function
 * @param arg User argument
 * @return 0 on success, negative on error
 */
xy_hal_error_t
xy_hal_i2c_set_error_cb(void *i2c, xy_hal_i2c_callback_t callback, void *arg);

#ifdef __cplusplus
}
#endif

#endif /* XY_HAL_I2C_H */