/**
 * @file sensor_sgp30.h
 * @brief SGP30 气体传感器驱动
 */
#ifndef __SENSOR_SGP30_H__
#define __SENSOR_SGP30_H__

#include "sensor_core.h"

#define SGP30_ADDR 0x58

typedef struct {
    uint8_t i2c_addr;
} sgp30_priv_t;

/**
 * @brief Create SGP30 sensor instance
 * @param name Sensor name
 * @param i2c_bus I2C bus handle
 * @return sensor_device_t* sensor handle
 */
sensor_device_t *sgp30_create(const char *name, void *i2c_bus);

#endif
