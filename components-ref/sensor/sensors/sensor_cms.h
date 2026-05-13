/**
 * @file sensor_cms.h
 * @brief 华润微 CMS 系列3轴加速度计驱动
 */
#ifndef __SENSOR_CMS_H__
#define __SENSOR_CMS_H__

#include "sensor_core.h"

#define CMS_ADDR_DEFAULT 0x18
#define CMS_REG_WHOAMI   0x0F
#define CMS_REG_CTRL1    0x20
#define CMS_REG_OUT_X_L  0x28
#define CMS_WHOAMI_VALUE 0x11

typedef struct {
    uint8_t i2c_addr;
} cms_priv_t;

sensor_device_t *cms_create(const char *name, void *i2c_bus, uint8_t addr);

#endif
