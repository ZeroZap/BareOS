#ifndef __SENSOR_SC7A20_H__
#define __SENSOR_SC7A20_H__

#include "sensor_core.h"

/* SC7A20 I2C地址 */
#define SC7A20_ADDR_DEFAULT 0x18
#define SC7A20_ADDR_ALT     0x19

/* 寄存器定义 */
#define SC7A20_REG_WHOAMI    0x0F
#define SC7A20_REG_CTRL_REG1 0x20
#define SC7A20_REG_CTRL_REG2 0x21
#define SC7A20_REG_CTRL_REG3 0x22
#define SC7A20_REG_CTRL_REG4 0x23
#define SC7A20_REG_OUT_X_L   0x28
#define SC7A20_REG_OUT_X_H   0x29
#define SC7A20_REG_OUT_Y_L   0x2A
#define SC7A20_REG_OUT_Y_H   0x2B
#define SC7A20_REG_OUT_Z_L   0x2C
#define SC7A20_REG_OUT_Z_H   0x2D

/* WHO_AM_I值 */
#define SC7A20_WHOAMI_VALUE 0x11

/* 数据速率配置 */
#define SC7A20_ODR_POWER_DOWN 0x00
#define SC7A20_ODR_1HZ        0x10
#define SC7A20_ODR_10HZ       0x20
#define SC7A20_ODR_25HZ       0x30
#define SC7A20_ODR_50HZ       0x40
#define SC7A20_ODR_100HZ      0x50
#define SC7A20_ODR_200HZ      0x60
#define SC7A20_ODR_400HZ      0x70

/* 量程配置 */
#define SC7A20_RANGE_2G  0x00
#define SC7A20_RANGE_4G  0x10
#define SC7A20_RANGE_8G  0x20
#define SC7A20_RANGE_16G 0x30

/* 私有数据 */
typedef struct {
    uint8_t i2c_addr;
    uint8_t range;     /* 量程 (2/4/8/16 g) */
    uint8_t odr_reg;   /* ODR寄存器值 */
    int16_t offset[3]; /* 偏移校准 */
} sc7a20_priv_t;

/* 创建传感器设备 */
sensor_device_t *sc7a20_create(const char *name, void *i2c_bus);

#endif /* __SENSOR_SC7A20_H__ */