#ifndef __SENSOR_BMA400_H__
#define __SENSOR_BMA400_H__

#include "sensor_core.h"

/* BMA400 I2C地址 */
#define BMA400_ADDR_DEFAULT 0x14
#define BMA400_ADDR_ALT     0x15

/* 寄存器定义 */
#define BMA400_REG_CHIPID      0x00
#define BMA400_REG_STATUS      0x03
#define BMA400_REG_ACC_X_LSB   0x04
#define BMA400_REG_ACC_CONFIG0 0x19
#define BMA400_REG_ACC_CONFIG1 0x1A
#define BMA400_REG_ACC_CONFIG2 0x1B
#define BMA400_REG_INT_CONFIG0 0x1F
#define BMA400_REG_INT_CONFIG1 0x20
#define BMA400_REG_CMD         0x7E

/* CHIP_ID */
#define BMA400_CHIP_ID 0x90

/* 功耗模式 */
typedef enum {
    BMA400_POWER_MODE_SLEEP     = 0x00, /* 0.4μA */
    BMA400_POWER_MODE_LOW_POWER = 0x01, /* 3.5μA @ 25Hz */
    BMA400_POWER_MODE_NORMAL    = 0x02, /* 14μA @ 100Hz */
} bma400_power_mode_t;

/* 数据速率 */
typedef enum {
    BMA400_ODR_12_5HZ = 0x05,
    BMA400_ODR_25HZ   = 0x06,
    BMA400_ODR_50HZ   = 0x07,
    BMA400_ODR_100HZ  = 0x08,
    BMA400_ODR_200HZ  = 0x09,
    BMA400_ODR_400HZ  = 0x0A,
    BMA400_ODR_800HZ  = 0x0B,
} bma400_odr_t;

/* 量程 */
typedef enum {
    BMA400_RANGE_2G  = 0x00,
    BMA400_RANGE_4G  = 0x01,
    BMA400_RANGE_8G  = 0x02,
    BMA400_RANGE_16G = 0x03,
} bma400_range_t;

/* 私有数据 */
typedef struct {
    uint8_t i2c_addr;
    bma400_power_mode_t power_mode;
    bma400_odr_t odr;
    bma400_range_t range;
    uint8_t range_g;
} bma400_priv_t;

sensor_device_t *bma400_create(const char *name, void *i2c_bus);

#endif /* __SENSOR_BMA400_H__ */