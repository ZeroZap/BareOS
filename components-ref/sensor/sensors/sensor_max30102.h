/**
 * @file sensor_max30102.h
 * @brief MAX30102 心率/血氧传感器驱动
 */
#ifndef __SENSOR_MAX30102_H__
#define __SENSOR_MAX30102_H__

#include "sensor_core.h"

#define MAX30102_ADDR 0x57

typedef struct {
    uint8_t i2c_addr;
} max30102_priv_t;

sensor_device_t *max30102_create(const char *name, void *i2c_bus);

#endif
