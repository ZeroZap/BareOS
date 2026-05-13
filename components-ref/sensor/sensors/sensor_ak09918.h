/**
 * @file sensor_ak09918.h
 * @brief AK09918 3轴磁力计驱动 (国产AKM)
 */
#ifndef __SENSOR_AK09918_H__
#define __SENSOR_AK09918_H__

#include "sensor_core.h"

#define AK09918_ADDR_DEFAULT 0x0C
#define AK09918_REG_WHOAMI   0x00
#define AK09918_REG_CTRL1    0x31
#define AK09918_REG_CTRL2    0x32
#define AK09918_REG_DATA     0x31
#define AK09918_WHOAMI_VALUE 0x09

typedef struct { uint8_t i2c_addr; } ak09918_priv_t;
sensor_device_t *ak09918_create(const char *name, void *i2c_bus);

#endif
