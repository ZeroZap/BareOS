/**
 * @file sensor_hs_ads1100.h
 * @brief 航顺 HS-ADS1100 3轴加速度计驱动
 */
#ifndef __SENSOR_HS_ADS1100_H__
#define __SENSOR_HS_ADS1100_H__

#include "sensor_core.h"

#define HS_ADS1100_ADDR_DEFAULT 0x18
#define HS_ADS1100_REG_WHOAMI   0x0F
#define HS_ADS1100_REG_CTRL1    0x20
#define HS_ADS1100_REG_OUT_X_L  0x28
#define HS_ADS1100_WHOAMI_VALUE 0x20

typedef struct {
    uint8_t i2c_addr;
} hs_ads1100_priv_t;

sensor_device_t *hs_ads1100_create(const char *name, void *i2c_bus, uint8_t addr);

#endif
