/**
 * @file sensor_aht10.h
 * @brief 国产 AHT10 温湿度传感器驱动
 */
#ifndef __SENSOR_AHT10_H__
#define __SENSOR_AHT10_H__

#include "sensor_core.h"

#define AHT10_ADDR_DEFAULT 0x38
#define AHT10_REG_STATUS   0x00
#define AHT10_REG_DATA     0x00

typedef struct {
    uint8_t i2c_addr;
} aht10_priv_t;

/**
 * @brief Create AHT10 sensor instance
 * @param name Sensor name
 * @param i2c_bus I2C bus handle
 * @param addr I2C address (0 for default)
 * @return sensor_device_t* sensor handle
 */
sensor_device_t *aht10_create(const char *name, void *i2c_bus, uint8_t addr);

#endif
