/**
 * @file xy_hal_pc.h
 * @brief HAL PC Simulation Layer Header
 * @version 1.0.0
 * @date 2026-03-13
 */

#ifndef XY_HAL_PC_H
#define XY_HAL_PC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/**
 * @brief PC 仿真 GPIO 端口结构
 */
struct xy_hal_gpio_port {
    uint8_t port_id;
    uint32_t pin_mask;
    uint8_t direction;  /* 0=Input, 1=Output */
    uint8_t pull;       /* 0=None, 1=Pull-up, 2=Pull-down */
};

/**
 * @brief PC 仿真 UART 结构
 */
typedef struct {
    uint8_t instance;
    uint32_t baudrate;
    FILE *fp;
} xy_hal_uart_t;

/**
 * @brief PC 仿真 I2C 结构
 */
typedef struct {
    uint8_t instance;
    uint32_t speed;
    uint8_t current_addr;
} xy_hal_i2c_t;

/**
 * @brief PC 仿真 SPI 结构
 */
typedef struct {
    uint8_t instance;
    uint32_t speed_hz;
    uint8_t mode;
} xy_hal_spi_t;

/**
 * @brief PC 仿真 ADC 结构
 */
typedef struct {
    uint8_t instance;
    uint8_t resolution;  /* 8/12/16 bit */
    float vref;          /* Reference voltage */
} xy_hal_adc_t;

/* ==================== GPIO Simulation ==================== */

int xy_hal_gpio_init_pc(uint8_t port, uint16_t pin, uint8_t mode);
int xy_hal_gpio_write_pc(uint8_t port, uint16_t pin, uint8_t value);
int xy_hal_gpio_read_pc(uint8_t port, uint16_t pin);
void xy_hal_gpio_toggle_pc(uint8_t port, uint16_t pin);

/* ==================== UART Simulation ==================== */

int xy_hal_uart_init_pc(uint8_t instance, uint32_t baudrate);
int xy_hal_uart_send_pc(uint8_t instance, const uint8_t *data, uint16_t len, uint32_t timeout);
int xy_hal_uart_recv_pc(uint8_t instance, uint8_t *data, uint16_t len, uint32_t timeout);

/* ==================== I2C Simulation ==================== */

int xy_hal_i2c_init_pc(uint8_t instance, uint32_t speed);
int xy_hal_i2c_start_pc(uint8_t instance);
int xy_hal_i2c_stop_pc(uint8_t instance);
int xy_hal_i2c_write_pc(uint8_t instance, uint8_t addr, const uint8_t *data, uint16_t len);
int xy_hal_i2c_read_pc(uint8_t instance, uint8_t addr, uint8_t *data, uint16_t len);

/* ==================== SPI Simulation ==================== */

int xy_hal_spi_init_pc(uint8_t instance, uint32_t speed, uint8_t mode);
int xy_hal_spi_transfer_pc(uint8_t instance, const uint8_t *tx, uint8_t *rx, uint16_t len);

/* ==================== ADC Simulation ==================== */

int xy_hal_adc_init_pc(uint8_t instance, uint8_t resolution, float vref);
int xy_hal_adc_read_pc(uint8_t instance, uint8_t channel, uint32_t *value);

/* ==================== Delay Simulation ==================== */

void xy_hal_delay_ms(uint32_t ms);
void xy_hal_delay_us(uint32_t us);

#ifdef __cplusplus
}
#endif

#endif /* XY_HAL_PC_H */
