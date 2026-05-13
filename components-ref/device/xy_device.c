/**
 * @file xy_device.c
 * @brief Device Driver Framework Implementation
 * @version 1.0.0
 * @date 2026-02-28
 */

#include "xy_device.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

// Include HAL types for PC build
#ifdef HAL_PLATFORM_PC
#include "xy_hal.h"

// Forward declaration for PC-specific function
extern void xy_hal_delay_ms(uint32_t ms);
#endif

/* ==================== I2C Device Implementation ==================== */

xy_error_t xy_i2c_device_init(xy_i2c_device_t *dev, void *i2c_handle, 
                       uint16_t addr, uint32_t timeout)
{
    if (!dev || !i2c_handle) {
        return XY_DEVICE_INVALID_PARAM;
    }

    memset(dev, 0, sizeof(*dev));
    dev->base.name = "i2c_device";
    dev->base.type = XY_DEVICE_TYPE_I2C;
    dev->i2c_handle = i2c_handle;
    dev->dev_addr = addr;
    dev->timeout = timeout ? timeout : 1000;
    dev->base.initialized = true;

    return XY_DEVICE_OK;
}

xy_error_t xy_i2c_device_read_reg(xy_i2c_device_t *dev, uint8_t reg, 
                           uint8_t *data, size_t len)
{
    if (!dev || !data || !dev->base.initialized) {
        return XY_DEVICE_INVALID_PARAM;
    }

    /* Write register address */
    xy_hal_error_t ret;
    ret = xy_hal_i2c_master_transmit(dev->i2c_handle, dev->dev_addr, 
                                     &reg, 1, dev->timeout);
    if (ret != XY_HAL_OK) {
        return XY_DEVICE_IO_ERROR;
    }

    /* Read data */
    ret = xy_hal_i2c_master_receive(dev->i2c_handle, dev->dev_addr, 
                                    data, len, dev->timeout);
    if (ret != XY_HAL_OK) {
        return XY_DEVICE_IO_ERROR;
    }

    return (int)len;
}

xy_error_t xy_i2c_device_write_reg(xy_i2c_device_t *dev, uint8_t reg, 
                            const uint8_t *data, size_t len)
{
    if (!dev || !data || !dev->base.initialized) {
        return XY_DEVICE_INVALID_PARAM;
    }

    /* Write register address + data */
    uint8_t buffer[16];
    if (len + 1 > sizeof(buffer)) {
        return XY_DEVICE_NO_MEM;
    }

    buffer[0] = reg;
    memcpy(&buffer[1], data, len);

    xy_hal_error_t ret = xy_hal_i2c_master_transmit(dev->i2c_handle, 
                                                    dev->dev_addr, 
                                                    buffer, len + 1, 
                                                    dev->timeout);
    if (ret != XY_HAL_OK) {
        return XY_DEVICE_IO_ERROR;
    }

    return (int)len;
}

xy_error_t xy_i2c_device_read(xy_i2c_device_t *dev, uint8_t *data, size_t len)
{
    if (!dev || !data || !dev->base.initialized) {
        return XY_DEVICE_INVALID_PARAM;
    }

    xy_hal_error_t ret = xy_hal_i2c_master_receive(dev->i2c_handle, 
                                                   dev->dev_addr, 
                                                   data, len, 
                                                   dev->timeout);
    if (ret != XY_HAL_OK) {
        return XY_DEVICE_IO_ERROR;
    }

    return (int)len;
}

xy_error_t xy_i2c_device_write(xy_i2c_device_t *dev, const uint8_t *data, size_t len)
{
    if (!dev || !data || !dev->base.initialized) {
        return XY_DEVICE_INVALID_PARAM;
    }

    xy_hal_error_t ret = xy_hal_i2c_master_transmit(dev->i2c_handle, 
                                                    dev->dev_addr, 
                                                    data, len, 
                                                    dev->timeout);
    if (ret != XY_HAL_OK) {
        return XY_DEVICE_IO_ERROR;
    }

    return (int)len;
}

/* ==================== SPI Device Implementation ==================== */

xy_error_t xy_spi_device_init(xy_spi_device_t *dev, void *spi_handle, 
                       void *cs_pin, uint32_t speed, uint8_t mode)
{
    if (!dev || !spi_handle) {
        return XY_DEVICE_INVALID_PARAM;
    }

    memset(dev, 0, sizeof(*dev));
    dev->base.name = "spi_device";
    dev->base.type = XY_DEVICE_TYPE_SPI;
    dev->spi_handle = spi_handle;
    dev->cs_pin = cs_pin;
    dev->speed = speed ? speed : 1000000; /* 1MHz default */
    dev->mode = mode;
    dev->base.initialized = true;

    return XY_DEVICE_OK;
}

void xy_spi_device_cs(xy_spi_device_t *dev, bool select)
{
    if (!dev || !dev->cs_pin) {
        return;
    }

    /* CS low to select - cs_pin is a GPIO handle */
    /* For PC build, we just track state */
#ifdef HAL_PLATFORM_PC
    extern int xy_hal_pin_write_handle(void *pin_handle, int state);
    xy_hal_pin_write_handle(dev->cs_pin, select ? 0 : 1);
#else
    /* For embedded builds, cs_pin should be a struct with port/pin */
    /* This needs platform-specific implementation */
#endif
}

xy_error_t xy_spi_device_transfer(xy_spi_device_t *dev, const uint8_t *tx, 
                           uint8_t *rx, size_t len)
{
    if (!dev || !dev->base.initialized) {
        return XY_DEVICE_INVALID_PARAM;
    }

    xy_spi_device_cs(dev, true);

    xy_hal_error_t ret;
    if (tx && rx) {
        ret = xy_hal_spi_transmit_receive(dev->spi_handle, tx, rx, len, 1000);
    } else if (tx) {
        ret = xy_hal_spi_transmit(dev->spi_handle, tx, len, 1000);
    } else if (rx) {
        ret = xy_hal_spi_receive(dev->spi_handle, rx, len, 1000);
    } else {
        xy_spi_device_cs(dev, false);
        return XY_DEVICE_INVALID_PARAM;
    }

    xy_spi_device_cs(dev, false);

    if (ret != XY_HAL_OK) {
        return XY_DEVICE_IO_ERROR;
    }

    return (int)len;
}

int xy_spi_device_send(xy_spi_device_t *dev, const uint8_t *data, size_t len)
{
    return xy_spi_device_transfer(dev, data, NULL, len);
}

int xy_spi_device_recv(xy_spi_device_t *dev, uint8_t *data, size_t len)
{
    return xy_spi_device_transfer(dev, NULL, data, len);
}

/* ==================== UART Device Implementation ==================== */

xy_error_t xy_uart_device_init(xy_uart_device_t *dev, void *uart_handle, 
                        uint32_t baudrate)
{
    if (!dev || !uart_handle) {
        return XY_DEVICE_INVALID_PARAM;
    }

    memset(dev, 0, sizeof(*dev));
    dev->base.name = "uart_device";
    dev->base.type = XY_DEVICE_TYPE_UART;
    dev->uart_handle = uart_handle;
    dev->baudrate = baudrate ? baudrate : 115200;
    dev->base.initialized = true;

    return XY_DEVICE_OK;
}

xy_error_t xy_uart_device_send(xy_uart_device_t *dev, const uint8_t *data, size_t len)
{
    if (!dev || !data || !dev->base.initialized) {
        return XY_DEVICE_INVALID_PARAM;
    }

    xy_hal_error_t ret = xy_hal_uart_send(dev->uart_handle, data, len, 1000);
    if (ret != XY_HAL_OK) {
        return XY_DEVICE_IO_ERROR;
    }

    return (int)len;
}

xy_error_t xy_uart_device_recv(xy_uart_device_t *dev, uint8_t *data, size_t len)
{
    if (!dev || !data || !dev->base.initialized) {
        return XY_DEVICE_INVALID_PARAM;
    }

    int ret = xy_hal_uart_recv(dev->uart_handle, data, len, 1000);
    if (ret != XY_HAL_OK) {
        return XY_DEVICE_IO_ERROR;
    }

    return (int)len;
}

int xy_uart_device_printf(xy_uart_device_t *dev, const char *fmt, ...)
{
    if (!dev || !fmt || !dev->base.initialized) {
        return XY_DEVICE_INVALID_PARAM;
    }

    char buffer[128];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    if (len > 0) {
        return xy_uart_device_send(dev, (uint8_t *)buffer, len);
    }

    return len;
}

/* ==================== GPIO Device Implementation ==================== */

xy_error_t xy_gpio_device_init(xy_gpio_device_t *dev, void *port, uint8_t pin,
                        xy_gpio_mode_t mode, xy_gpio_pull_t pull)
{
    if (!dev || !port) {
        return XY_DEVICE_INVALID_PARAM;
    }

    memset(dev, 0, sizeof(*dev));
    dev->base.name = "gpio_device";
    dev->base.type = XY_DEVICE_TYPE_GPIO;
    dev->gpio_port = port;
    dev->gpio_pin = pin;
    dev->mode = mode;
    dev->pull = pull;
    dev->base.initialized = true;

    /* Configure GPIO */
    xy_hal_gpio_config_t config;
    config.mode = (xy_hal_gpio_mode_t)mode;
    config.pull = (xy_hal_gpio_pull_t)pull;
    config.otype = XY_HAL_GPIO_OTYPE_PP;
    config.speed = XY_HAL_GPIO_SPEED_LOW;

    xy_hal_pin_init(port, pin, &config);

    return XY_DEVICE_OK;
}

void xy_gpio_device_set(xy_gpio_device_t *dev, bool value)
{
    if (!dev || !dev->base.initialized) {
        return;
    }

    xy_hal_pin_write(dev->gpio_port, dev->gpio_pin, value ? 1 : 0);
}

bool xy_gpio_device_get(xy_gpio_device_t *dev)
{
    if (!dev || !dev->base.initialized) {
        return false;
    }

    return xy_hal_pin_read(dev->gpio_port, dev->gpio_pin) != 0;
}

xy_error_t xy_gpio_device_toggle(xy_gpio_device_t *dev)
{
    if (!dev || !dev->base.initialized) {
        return XY_DEVICE_INVALID_PARAM;
    }

    xy_hal_pin_toggle(dev->gpio_port, dev->gpio_pin);
    return XY_DEVICE_OK;
}

/* ==================== Device Manager Implementation ==================== */

xy_error_t xy_device_manager_init(xy_device_manager_t *mgr, size_t max_count)
{
    if (!mgr || max_count == 0) {
        return XY_DEVICE_INVALID_PARAM;
    }

    memset(mgr, 0, sizeof(*mgr));
    mgr->max_count = max_count;
    mgr->count = 0;

    /* Allocate device array */
    mgr->devices = (xy_device_t **)calloc(max_count, sizeof(xy_device_t *));
    if (!mgr->devices) {
        return XY_DEVICE_NO_MEM;
    }

    return XY_DEVICE_OK;
}

xy_error_t xy_device_manager_register(xy_device_manager_t *mgr, xy_device_t *dev)
{
    if (!mgr || !dev || mgr->count >= mgr->max_count) {
        return XY_DEVICE_INVALID_PARAM;
    }

    /* Check if already registered */
    for (size_t i = 0; i < mgr->count; i++) {
        if (mgr->devices[i] == dev) {
            return XY_DEVICE_ERROR;
        }
    }

    mgr->devices[mgr->count++] = dev;
    return XY_DEVICE_OK;
}

xy_error_t xy_device_manager_unregister(xy_device_manager_t *mgr, xy_device_t *dev)
{
    if (!mgr || !dev) {
        return XY_DEVICE_INVALID_PARAM;
    }

    for (size_t i = 0; i < mgr->count; i++) {
        if (mgr->devices[i] == dev) {
            /* Shift remaining devices */
            for (size_t j = i; j < mgr->count - 1; j++) {
                mgr->devices[j] = mgr->devices[j + 1];
            }
            mgr->count--;
            return XY_DEVICE_OK;
        }
    }

    return XY_DEVICE_NOT_FOUND;
}

xy_device_t *xy_device_manager_find(xy_device_manager_t *mgr, const char *name)
{
    if (!mgr || !name) {
        return NULL;
    }

    for (size_t i = 0; i < mgr->count; i++) {
        if (mgr->devices[i] && mgr->devices[i]->name &&
            strcmp(mgr->devices[i]->name, name) == 0) {
            return mgr->devices[i];
        }
    }

    return NULL;
}

int xy_device_manager_foreach(xy_device_manager_t *mgr, 
                              int (*callback)(xy_device_t *dev, void *arg),
                              void *arg)
{
    if (!mgr || !callback) {
        return XY_DEVICE_INVALID_PARAM;
    }

    int result = XY_DEVICE_OK;
    for (size_t i = 0; i < mgr->count; i++) {
        if (mgr->devices[i]) {
            result = callback(mgr->devices[i], arg);
            if (result < 0) {
                break;
            }
        }
    }

    return result;
}
