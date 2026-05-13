/**
 * @file sensor_ina219.h
 * @brief INA219 电流/功率传感器驱动
 */
#ifndef __SENSOR_INA219_H__
#define __SENSOR_INA219_H__

#include "sensor_core.h"

#define INA219_ADDR 0x40

typedef struct {
    uint8_t i2c_addr;
} ina219_priv_t;

/**
 * @brief Create INA219 sensor instance
 * @param name Sensor name
 * @param i2c_bus I2C bus handle
 * @return sensor_device_t* sensor handle
 */
sensor_device_t *ina219_create(const char *name, void *i2c_bus);

#endif
