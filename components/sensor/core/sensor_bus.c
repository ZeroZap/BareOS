/**
 * @file sensor_bus.c
 * @brief Sensor Bus Abstraction (I2C/SPI)
 * @version 1.0.0
 * @date 2026-03-05
 */

#include "xy_sensor.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* ==================== I2C 总线 ==================== */

/**
 * @brief I2C 读取
 */
int xy_sensor_i2c_read(xy_sensor_bus_t *bus, uint8_t reg, uint8_t *data, uint16_t len)
{
    if (!bus || !data || len == 0) {
        return -1;
    }
    
    if (bus->type != XY_SENSOR_BUS_I2C) {
        return -1;
    }
    
    /* 调用底层 I2C 驱动 */
    /* 示例代码，实际需根据平台调整 */
    #ifdef CONFIG_HAL_I2C
    return xy_hal_i2c_read(bus->bus_handle, bus->address, reg, data, len);
    #else
    return -1;
    #endif
}

/**
 * @brief I2C 写入
 */
int xy_sensor_i2c_write(xy_sensor_bus_t *bus, uint8_t reg, const uint8_t *data, uint16_t len)
{
    if (!bus || !data || len == 0) {
        return -1;
    }
    
    if (bus->type != XY_SENSOR_BUS_I2C) {
        return -1;
    }
    
    #ifdef CONFIG_HAL_I2C
    return xy_hal_i2c_write(bus->bus_handle, bus->address, reg, data, len);
    #else
    return -1;
    #endif
}

/**
 * @brief I2C 读取寄存器 (单字节)
 */
int xy_sensor_i2c_read_reg(xy_sensor_bus_t *bus, uint8_t reg, uint8_t *value)
{
    return xy_sensor_i2c_read(bus, reg, value, 1);
}

/**
 * @brief I2C 写入寄存器 (单字节)
 */
int xy_sensor_i2c_write_reg(xy_sensor_bus_t *bus, uint8_t reg, uint8_t value)
{
    return xy_sensor_i2c_write(bus, reg, &value, 1);
}

/**
 * @brief I2C 读取寄存器 (多字节)
 */
int xy_sensor_i2c_read_reg16(xy_sensor_bus_t *bus, uint8_t reg, uint16_t *value)
{
    uint8_t buf[2];
    int ret = xy_sensor_i2c_read(bus, reg, buf, 2);
    if (ret == 0) {
        *value = (buf[1] << 8) | buf[0];
    }
    return ret;
}

/**
 * @brief I2C 写入寄存器 (多字节)
 */
int xy_sensor_i2c_write_reg16(xy_sensor_bus_t *bus, uint8_t reg, uint16_t value)
{
    uint8_t buf[2];
    buf[0] = value & 0xFF;
    buf[1] = (value >> 8) & 0xFF;
    return xy_sensor_i2c_write(bus, reg, buf, 2);
}

/* ==================== SPI 总线 ==================== */

/**
 * @brief SPI 读取
 */
int xy_sensor_spi_read(xy_sensor_bus_t *bus, uint8_t reg, uint8_t *data, uint16_t len)
{
    if (!bus || !data || len == 0) {
        return -1;
    }
    
    if (bus->type != XY_SENSOR_BUS_SPI) {
        return -1;
    }
    
    #ifdef CONFIG_HAL_SPI
    /* 拉低片选 */
    xy_hal_gpio_write(bus->chip_select, 0);
    
    /* 发送寄存器地址 */
    xy_hal_spi_write(bus->bus_handle, &reg, 1);
    
    /* 读取数据 */
    int ret = xy_hal_spi_read(bus->bus_handle, data, len);
    
    /* 释放片选 */
    xy_hal_gpio_write(bus->chip_select, 1);
    
    return ret;
    #else
    return -1;
    #endif
}

/**
 * @brief SPI 写入
 */
int xy_sensor_spi_write(xy_sensor_bus_t *bus, uint8_t reg, const uint8_t *data, uint16_t len)
{
    if (!bus || !data || len == 0) {
        return -1;
    }
    
    if (bus->type != XY_SENSOR_BUS_SPI) {
        return -1;
    }
    
    #ifdef CONFIG_HAL_SPI
    /* 拉低片选 */
    xy_hal_gpio_write(bus->chip_select, 0);
    
    /* 发送寄存器地址 */
    xy_hal_spi_write(bus->bus_handle, &reg, 1);
    
    /* 发送数据 */
    int ret = xy_hal_spi_write(bus->bus_handle, data, len);
    
    /* 释放片选 */
    xy_hal_gpio_write(bus->chip_select, 1);
    
    return ret;
    #else
    return -1;
    #endif
}

/* ==================== 工具函数 ==================== */

/**
 * @brief 检查设备 ID
 */
int xy_sensor_check_device_id(xy_sensor_bus_t *bus, uint8_t id_reg, uint8_t expected_id)
{
    uint8_t id;
    int ret;
    
    if (bus->type == XY_SENSOR_BUS_I2C) {
        ret = xy_sensor_i2c_read_reg(bus, id_reg, &id);
    } else if (bus->type == XY_SENSOR_BUS_SPI) {
        ret = xy_sensor_spi_read(bus, id_reg, &id, 1);
    } else {
        return -1;
    }
    
    if (ret != 0) {
        xy_log_e("Failed to read device ID\n");
        return -1;
    }
    
    if (id != expected_id) {
        xy_log_e("Device ID mismatch: expected=0x%02X, got=0x%02X\n", expected_id, id);
        return -1;
    }
    
    xy_log_i("Device ID verified: 0x%02X\n", id);
    return 0;
}

/**
 * @brief 配置 I2C 总线
 */
void xy_sensor_bus_config_i2c(xy_sensor_bus_t *bus, void *handle, uint8_t address)
{
    if (!bus) return;
    
    bus->type = XY_SENSOR_BUS_I2C;
    bus->bus_handle = handle;
    bus->address = address;
    bus->chip_select = 0;
}

/**
 * @brief 配置 SPI 总线
 */
void xy_sensor_bus_config_spi(xy_sensor_bus_t *bus, void *handle, uint8_t cs_pin)
{
    if (!bus) return;
    
    bus->type = XY_SENSOR_BUS_SPI;
    bus->bus_handle = handle;
    bus->address = 0;
    bus->chip_select = cs_pin;
}
