#ifndef __SENSOR_KX023_H__
#define __SENSOR_KX023_H__

#include "sensor_core.h"

/* KX023 I2C地址 */
#define KX023_ADDR_DEFAULT 0x1E
#define KX023_ADDR_ALT     0x1F

/* 寄存器定义 */
#define KX023_REG_WHO_AM_I  0x0F
#define KX023_REG_XOUT_L    0x06
#define KX023_REG_CNTL1     0x18
#define KX023_REG_CNTL2     0x19
#define KX023_REG_ODCNTL    0x1B
#define KX023_REG_SOFT_REST 0x7F

/* WHO_AM_I */
#define KX023_WHO_AM_I_VALUE 0x15

/* 功耗模式 */
typedef enum {
    KX023_MODE_STANDBY   = 0x00,
    KX023_MODE_LOW_POWER = 0x01, /* 0.9μA @ 0.781Hz */
    KX023_MODE_HIGH_RES  = 0x81,
} kx023_mode_t;

/* 数据速率 */
typedef enum {
    KX023_ODR_0_781HZ = 0x08,
    KX023_ODR_1_563HZ = 0x09,
    KX023_ODR_3_125HZ = 0x0A,
    KX023_ODR_6_25HZ  = 0x0B,
    KX023_ODR_12_5HZ  = 0x00,
    KX023_ODR_25HZ    = 0x01,
    KX023_ODR_50HZ    = 0x02,
    KX023_ODR_100HZ   = 0x03,
} kx023_odr_t;

/* 私有数据 */
typedef struct {
    uint8_t i2c_addr;
    kx023_mode_t mode;
    kx023_odr_t odr;
} kx023_priv_t;

sensor_device_t *kx023_create(const char *name, void *i2c_bus);

#endif /* __SENSOR_KX023_H__ */