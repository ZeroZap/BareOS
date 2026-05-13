#ifndef __SENSOR_LIS2DH12_H__
#define __SENSOR_LIS2DH12_H__

#include "sensor_core.h"

/* LIS2DH12 I2C地址 */
#define LIS2DH12_ADDR_DEFAULT 0x18
#define LIS2DH12_ADDR_ALT     0x19

/* 寄存器定义 */
#define LIS2DH12_REG_TEMP_CFG 0x1F
#define LIS2DH12_REG_CTRL1    0x20
#define LIS2DH12_REG_CTRL2    0x21
#define LIS2DH12_REG_CTRL3    0x22
#define LIS2DH12_REG_CTRL4    0x23
#define LIS2DH12_REG_CTRL5    0x24
#define LIS2DH12_REG_CTRL6    0x25
#define LIS2DH12_REG_STATUS   0x27
#define LIS2DH12_REG_OUT_X_L  0x28
#define LIS2DH12_REG_WHOAMI   0x0F

/* WHO_AM_I值 */
#define LIS2DH12_WHOAMI_VALUE 0x33

/* 功耗模式 */
typedef enum {
    LIS2DH12_MODE_LOW_POWER,       /* 低功耗模式: 2μA @ 1Hz */
    LIS2DH12_MODE_NORMAL,          /* 正常模式: 11μA @ 100Hz */
    LIS2DH12_MODE_HIGH_RESOLUTION, /* 高分辨率模式: 11μA @ 100Hz */
} lis2dh12_mode_t;

/* 数据速率 */
typedef enum {
    LIS2DH12_ODR_POWER_DOWN = 0x00,
    LIS2DH12_ODR_1HZ        = 0x10,
    LIS2DH12_ODR_10HZ       = 0x20,
    LIS2DH12_ODR_25HZ       = 0x30,
    LIS2DH12_ODR_50HZ       = 0x40,
    LIS2DH12_ODR_100HZ      = 0x50,
    LIS2DH12_ODR_200HZ      = 0x60,
    LIS2DH12_ODR_400HZ      = 0x70,
} lis2dh12_odr_t;

/* 量程 */
typedef enum {
    LIS2DH12_RANGE_2G  = 0x00,
    LIS2DH12_RANGE_4G  = 0x10,
    LIS2DH12_RANGE_8G  = 0x20,
    LIS2DH12_RANGE_16G = 0x30,
} lis2dh12_range_t;

/* 私有数据 */
typedef struct {
    uint8_t i2c_addr;
    lis2dh12_mode_t mode;
    lis2dh12_odr_t odr;
    lis2dh12_range_t range;
    uint8_t range_g; /* 量程(g) */
    int16_t offset[3];
} lis2dh12_priv_t;

sensor_device_t *lis2dh12_create(const char *name, void *i2c_bus);

#endif /* __SENSOR_LIS2DH12_H__ */