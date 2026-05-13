/**
 * @file sensor_dmp6100.h
 * @brief 国产 DMP6100 3轴加速度计驱动
 */
#ifndef __SENSOR_DMP6100_H__
#define __SENSOR_DMP6100_H__

#include "sensor_core.h"

#define DMP6100_ADDR_DEFAULT 0x12
#define DMP6100_REG_WHOAMI   0x00
#define DMP6100_REG_CTRL     0x0C
#define DMP6100_REG_DATA     0x18
#define DMP6100_WHOAMI_VALUE 0x60

typedef struct {
    uint8_t i2c_addr;
} dmp6100_priv_t;

sensor_device_t *dmp6100_create(const char *name, void *i2c_bus, uint8_t addr);

#endif
