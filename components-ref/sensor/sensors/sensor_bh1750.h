/**
 * @file sensor_bh1750.h
 * @brief BH1750 环境光传感器驱动
 */
#ifndef __SENSOR_BH1750_H__
#define __SENSOR_BH1750_H__

#include "sensor_core.h"

#define BH1750_ADDR 0x23

typedef struct {
    uint8_t i2c_addr;
} bh1750_priv_t;

/**
 * @brief Create BH1750 sensor instance
 * @param name Sensor name
 * @param i2c_bus I2C bus handle
 * @return sensor_device_t* sensor handle
 */
sensor_device_t *bh1750_create(const char *name, void *i2c_bus);

#endif
