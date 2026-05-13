/**
 * @file sensor_gd30df.h
 * @brief 兆易创新 GD30DF 3轴加速度计驱动
 */
#ifndef __SENSOR_GD30DF_H__
#define __SENSOR_GD30DF_H__

#include "sensor_core.h"

#define GD30DF_ADDR_DEFAULT 0x18
#define GD30DF_REG_WHOAMI   0x0F
#define GD30DF_REG_CTRL1    0x20
#define GD30DF_REG_OUT_X_L  0x28
#define GD30DF_WHOAMI_VALUE 0x1E

typedef struct {
    uint8_t i2c_addr;
} gd30df_priv_t;

sensor_device_t *gd30df_create(const char *name, void *i2c_bus, uint8_t addr);

#endif
