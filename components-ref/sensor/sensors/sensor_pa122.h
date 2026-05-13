/**
 * @file sensor_pa122.h
 * @brief PA122 接近传感器驱动
 */
#ifndef __SENSOR_PA122_H__
#define __SENSOR_PA122_H__

#include "sensor_core.h"

#define PA122_ADDR 0x52

typedef struct {
    uint8_t i2c_addr;
} pa122_priv_t;

/**
 * @brief Create PA122 sensor instance
 * @param name Sensor name
 * @param i2c_bus I2C bus handle
 * @return sensor_device_t* sensor handle
 */
sensor_device_t *pa122_create(const char *name, void *i2c_bus);

#endif
