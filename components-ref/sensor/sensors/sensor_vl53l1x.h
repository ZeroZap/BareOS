/**
 * @file sensor_vl53l1x.h
 * @brief ST VL53L1X ToF 激光测距传感器驱动
 */
#ifndef __SENSOR_VL53L1X_H__
#define __SENSOR_VL53L1X_H__

#include "sensor_core.h"

#define VL53L1X_ADDR_DEFAULT 0x29

typedef struct {
    uint8_t i2c_addr;
} vl53l1x_priv_t;

sensor_device_t *vl53l1x_create(const char *name, void *i2c_bus, uint8_t addr);

#endif
