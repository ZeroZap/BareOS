/**
 * @file xy_hal_pc_impl.c
 * @brief HAL PC Simulation Implementation
 * @version 1.0.0
 * @date 2026-03-13
 */

#include "xy_hal_pc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

/* ==================== GPIO Simulation ==================== */

static xy_hal_gpio_port_t gpio_ports[16];

int xy_hal_gpio_init_pc(uint8_t port, uint16_t pin, uint8_t mode)
{
    if (port >= 16) return -1;
    
    gpio_ports[port].port_id = port;
    gpio_ports[port].pin_mask |= (1 << pin);
    gpio_ports[port].direction = (mode == 1) ? 1 : 0;  /* 1=Output */
    
    printf("[HAL] GPIO P%d.%d initialized (mode=%d)\n", port, pin, mode);
    return 0;
}

int xy_hal_gpio_write_pc(uint8_t port, uint16_t pin, uint8_t value)
{
    if (port >= 16) return -1;
    printf("[HAL] GPIO P%d.%d = %d\n", port, pin, value);
    return 0;
}

int xy_hal_gpio_read_pc(uint8_t port, uint16_t pin)
{
    if (port >= 16) return 0;
    /* 仿真：随机返回 0 或 1 */
    return rand() % 2;
}

void xy_hal_gpio_toggle_pc(uint8_t port, uint16_t pin)
{
    if (port >= 16) return;
    printf("[HAL] GPIO P%d.%d toggled\n", port, pin);
}

/* ==================== UART Simulation ==================== */

int xy_hal_uart_init_pc(uint8_t instance, uint32_t baudrate)
{
    printf("[HAL] UART%d initialized @ %lu bps\n", instance, baudrate);
    return 0;
}

int xy_hal_uart_send_pc(uint8_t instance, const uint8_t *data, uint16_t len, uint32_t timeout)
{
    (void)instance; (void)timeout;
    printf("[HAL] UART TX: ");
    for (int i = 0; i < len; i++) {
        if (data[i] >= 32 && data[i] < 127) {
            printf("%c", data[i]);
        } else {
            printf("\\x%02X", data[i]);
        }
    }
    printf("\n");
    return len;
}

int xy_hal_uart_recv_pc(uint8_t instance, uint8_t *data, uint16_t len, uint32_t timeout)
{
    (void)instance; (void)timeout;
    /* 仿真：返回测试数据 */
    for (int i = 0; i < len; i++) {
        data[i] = 'A' + (i % 26);
    }
    return len;
}

/* ==================== I2C Simulation ==================== */

int xy_hal_i2c_init_pc(uint8_t instance, uint32_t speed)
{
    printf("[HAL] I2C%d initialized @ %lu Hz\n", instance, speed);
    return 0;
}

int xy_hal_i2c_start_pc(uint8_t instance)
{
    (void)instance;
    printf("[HAL] I2C START\n");
    return 0;
}

int xy_hal_i2c_stop_pc(uint8_t instance)
{
    (void)instance;
    printf("[HAL] I2C STOP\n");
    return 0;
}

int xy_hal_i2c_write_pc(uint8_t instance, uint8_t addr, const uint8_t *data, uint16_t len)
{
    (void)instance;
    printf("[HAL] I2C Write: addr=0x%02X, len=%d\n", addr, len);
    return len;
}

int xy_hal_i2c_read_pc(uint8_t instance, uint8_t addr, uint8_t *data, uint16_t len)
{
    (void)instance;
    printf("[HAL] I2C Read: addr=0x%02X, len=%d\n", addr, len);
    /* 仿真：返回测试数据 */
    for (int i = 0; i < len; i++) {
        data[i] = i;
    }
    return len;
}

/* ==================== SPI Simulation ==================== */

int xy_hal_spi_init_pc(uint8_t instance, uint32_t speed, uint8_t mode)
{
    printf("[HAL] SPI%d initialized @ %lu Hz (mode=%d)\n", instance, speed, mode);
    return 0;
}

int xy_hal_spi_transfer_pc(uint8_t instance, const uint8_t *tx, uint8_t *rx, uint16_t len)
{
    (void)instance;
    printf("[HAL] SPI Transfer: len=%d\n", len);
    if (tx && rx) {
        memcpy(rx, tx, len);
    }
    return len;
}

/* ==================== ADC Simulation ==================== */

int xy_hal_adc_init_pc(uint8_t instance, uint8_t resolution, float vref)
{
    printf("[HAL] ADC%d initialized (%d-bit, Vref=%.2fV)\n", instance, resolution, vref);
    return 0;
}

int xy_hal_adc_read_pc(uint8_t instance, uint8_t channel, uint32_t *value)
{
    (void)instance; (void)channel;
    /* 仿真：返回中间值 */
    if (value) {
        *value = (1 << 16) / 2;  /* 半量程 */
    }
    return 0;
}

/* ==================== Delay Simulation ==================== */

void xy_hal_delay_ms_pc(uint32_t ms)
{
    usleep(ms * 1000);
}

void xy_hal_delay_us_pc(uint32_t us)
{
    usleep(us);
}

/* ==================== Timestamp ==================== */

uint32_t xy_hal_get_tick_pc(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint32_t)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}
