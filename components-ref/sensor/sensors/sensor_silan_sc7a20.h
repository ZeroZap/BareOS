/**
 * @file sensor_silan_sc7a20.h
 * @brief 士兰微 SC7A20 3轴加速度计驱动
 * @description 国产高性能3轴MEMS加速度计，兼容LIS2DH12
 */
#ifndef __SENSOR_SILAN_SC7A20_H__
#define __SENSOR_SILAN_SC7A20_H__

#include "sensor_core.h"

/* I2C地址 */
#define SILAN_SC7A20_ADDR_DEFAULT 0x18
#define SILAN_SC7A20_ADDR_ALT     0x19

/* 寄存器定义 */
#define SILAN_SC7A20_REG_WHOAMI   0x0F
#define SILAN_SC7A20_REG_CTRL1    0x20
#define SILAN_SC7A20_REG_CTRL2    0x21
#define SILAN_SC7A20_REG_CTRL3    0x22
#define SILAN_SC7A20_REG_CTRL4    0x23
#define SILAN_SC7A20_REG_CTRL5    0x24
#define SILAN_SC7A20_REG_CTRL6    0x25
#define SILAN_SC7A20_REG_STATUS   0x27
#define SILAN_SC7A20_REG_OUT_X_L  0x28
#define SILAN_SC7A20_REG_OUT_X_H  0x29
#define SILAN_SC7A20_REG_OUT_Y_L  0x2A
#define SILAN_SC7A20_REG_OUT_Y_H  0x2B
#define SILAN_SC7A20_REG_OUT_Z_L  0x2C
#define SILAN_SC7A20_REG_OUT_Z_H  0x2D
#define SILAN_SC7A20_REG_FIFO_CTRL 0x2E
#define SILAN_SC7A20_REG_FIFO_SRC 0x2F
#define SILAN_SC7A20_REG_INT1_SRC 0x31
#define SILAN_SC7A20_REG_INT2_SRC 0x35
#define SILAN_SC7A20_REG_CLICK    0x39

/* WHO_AM_I值 */
#define SILAN_SC7A20_WHOAMI_VALUE 0x11

/* 量程 */
#define SILAN_SC7A20_RANGE_2G   0x00
#define SILAN_SC7A20_RANGE_4G   0x08
#define SILAN_SC7A20_RANGE_8G   0x10
#define SILAN_SC7A20_RANGE_16G  0x18

/* 采样率 */
#define SILAN_SC7A20_RATE_POWER_DOWN  0x00
#define SILAN_SC7A20_RATE_1HZ    0x10
#define SILAN_SC7A20_RATE_10HZ   0x20
#define SILAN_SC7A20_RATE_25HZ   0x30
#define SILAN_SC7A20_RATE_50HZ   0x40
#define SILAN_SC7A20_RATE_100HZ  0x50
#define SILAN_SC7A20_RATE_200HZ   0x60
#define SILAN_SC7A20_RATE_400HZ   0x70

/* 高通滤波器截止频率 */
#define SILAN_SC7A20_HPF_OFF     0x00
#define SILAN_SC7A20_HPF_0_1HZ   0x01
#define SILAN_SC7A20_HPF_0_2HZ   0x02
#define SILAN_SC7A20_HPF_0_4HZ   0x03
#define SILAN_SC7A20_HPF_1HZ     0x04
#define SILAN_SC7A20_HPF_2HZ     0x05
#define SILAN_SC7A20_HPF_4HZ     0x06
#define SILAN_SC7A20_HPF_8HZ     0x07

/* 私有数据 */
typedef struct {
    uint8_t i2c_addr;
    uint8_t range;
    uint8_t rate;
    int16_t offset[3];
} silan_sc7a20_priv_t;

/* 创建传感器设备 */
sensor_device_t *silan_sc7a20_create(const char *name, void *i2c_bus, uint8_t addr);

/* 配置函数 */
int silan_sc7a20_set_range(sensor_device_t *dev, uint8_t range);
int silan_sc7a20_set_rate(sensor_device_t *dev, uint8_t rate);
int silan_sc7a20_enable_high_pass(sensor_device_t *dev, uint8_t enable);

#endif /* __SENSOR_SILAN_SC7A20_H__ */
