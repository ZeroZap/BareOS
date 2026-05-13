/**
 * @file sensor_bmp390.h
 * @brief Bosch BMP390 气压计驱动
 */
#ifndef __SENSOR_BMP390_H__
#define __SENSOR_BMP390_H__

#include "sensor_core.h"

#define BMP390_ADDR_DEFAULT 0x76
#define BMP390_ADDR_ALT     0x77

#define BMP390_REG_CHIP_ID     0x00
#define BMP390_REG_STATUS      0x1B
#define BMP390_REG_PRESS_XLSB  0x1C
#define BMP390_REG_PRESS_LSB   0x1D
#define BMP390_REG_PRESS_MSB   0x1E
#define BMP390_REG_TEMP_XLSB   0x1F
#define BMP390_REG_TEMP_LSB    0x20
#define BMP390_REG_TEMP_MSB    0x21
#define BMP390_REG_PWR_CTRL    0x1A
#define BMP390_REG_OSR         0x1C
#define BMP390_REG_ODR         0x1D

#define BMP390_CHIP_ID  0x50

typedef struct {
    uint8_t i2c_addr;
} bmp390_priv_t;

/**
 * @brief Create BMP390 sensor instance
 * @param name Sensor name
 * @param i2c_bus I2C bus handle
 * @param addr I2C address (0 for default)
 * @return sensor_device_t* sensor handle
 */
sensor_device_t *bmp390_create(const char *name, void *i2c_bus, uint8_t addr);

#endif
