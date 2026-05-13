/**
 * @file sensor_max44009.h
 * @brief MAX44009 环境光传感器驱动
 */
#ifndef __SENSOR_MAX44009_H__
#define __SENSOR_MAX44009_H__

#include "sensor_core.h"

#define MAX44009_ADDR 0x4A

typedef struct {
    uint8_t i2c_addr;
} max44009_priv_t;

sensor_device_t *max44009_create(const char *name, void *i2c_bus);

#endif
