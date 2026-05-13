/**
 * @file sensor_im69d.h
 * @brief IM69D 声音传感器驱动
 */
#ifndef __SENSOR_IM69D_H__
#define __SENSOR_IM69D_H__

#include "sensor_core.h"

#define IM69D_ADDR 0x30

typedef struct {
    uint8_t i2c_addr;
} im69d_priv_t;

sensor_device_t *im69d_create(const char *name, void *i2c_bus);

#endif
