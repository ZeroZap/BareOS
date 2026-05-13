/**
 * @file sensor_sgp40.h
 * @brief SGP40 气体传感器驱动
 */
#ifndef __SENSOR_SGP40_H__
#define __SENSOR_SGP40_H__

#include "sensor_core.h"

#define SGP40_ADDR 0x44

typedef struct {
    uint8_t i2c_addr;
} sgp40_priv_t;

/**
 * @brief Create SGP40 sensor instance
 * @param name Sensor name
 * @param i2c_bus I2C bus handle
 * @return sensor_device_t* sensor handle
 */
sensor_device_t *sgp40_create(const char *name, void *i2c_bus);

#endif
