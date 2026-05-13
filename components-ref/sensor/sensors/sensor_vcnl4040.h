/**
 * @file sensor_vcnl4040.h
 * @brief VCNL4040 接近+光感传感器驱动
 */
#ifndef __SENSOR_VCNL4040_H__
#define __SENSOR_VCNL4040_H__

#include "sensor_core.h"

#define VCNL4040_ADDR 0x60

typedef struct {
    uint8_t i2c_addr;
} vcnl4040_priv_t;

/**
 * @brief Create VCNL4040 sensor instance
 * @param name Sensor name
 * @param i2c_bus I2C bus handle
 * @return sensor_device_t* sensor handle
 */
sensor_device_t *vcnl4040_create(const char *name, void *i2c_bus);

#endif
