/**
 * @file sensor_ens160.h
 * @brief ENS160 气体传感器驱动
 */
#ifndef __SENSOR_ENS160_H__
#define __SENSOR_ENS160_H__

#include "sensor_core.h"

#define ENS160_ADDR 0x52

typedef struct {
    uint8_t i2c_addr;
} ens160_priv_t;

/**
 * @brief Create ENS160 sensor instance
 * @param name Sensor name
 * @param i2c_bus I2C bus handle
 * @return sensor_device_t* sensor handle
 */
sensor_device_t *ens160_create(const char *name, void *i2c_bus);

#endif
