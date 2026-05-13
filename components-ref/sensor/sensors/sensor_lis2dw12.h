/**
 * @file sensor_lis2dw12.h
 * @brief ST LIS2DW12 低功耗3轴加速度计驱动
 * @description I2C/SPI 接口，超低功耗设计
 */
#ifndef __SENSOR_LIS2DW12_H__
#define __SENSOR_LIS2DW12_H__

#include "sensor_core.h"

/* I2C地址 */
#define LIS2DW12_ADDR_DEFAULT 0x18
#define LIS2DW12_ADDR_ALT     0x19

/* SPI片选 */
#define LIS2DW12_SPI_CS_NONE  0xFF

/* 寄存器定义 */
#define LIS2DW12_REG_WHOAMI     0x0F
#define LIS2DW12_REG_CTRL1      0x20
#define LIS2DW12_REG_CTRL2      0x21
#define LIS2DW12_REG_CTRL3      0x22
#define LIS2DW12_REG_CTRL4      0x23
#define LIS2DW12_REG_CTRL5      0x24
#define LIS2DW12_REG_STATUS     0x27
#define LIS2DW12_REG_OUT_X_L    0x28
#define LIS2DW12_REG_OUT_X_H    0x29
#define LIS2DW12_REG_OUT_Y_L    0x2A
#define LIS2DW12_REG_OUT_Y_H    0x2B
#define LIS2DW12_REG_OUT_Z_L    0x2C
#define LIS2DW12_REG_OUT_Z_H    0x2D

/* WHO_AM_I值 */
#define LIS2DW12_WHOAMI_VALUE  0x44

/* 工作模式 */
#define LIS2DW12_MODE_LOW_POWER      0x00
#define LIS2DW12_MODE_NORMAL         0x01
#define LIS2DW12_MODE_HIGH_PERF      0x02
#define LIS2DW12_MODE_HIGH_FREQ      0x03

/* 量程 */
#define LIS2DW12_RANGE_2G   0x00
#define LIS2DW12_RANGE_4G   0x01
#define LIS2DW12_RANGE_8G   0x02
#define LIS2DW12_RANGE_16G  0x03

/* 采样率 */
#define LIS2DW12_RATE_POWER_DOWN  0x00
#define LIS2DW12_RATE_1_6HZ       0x01
#define LIS2DW12_RATE_12_5HZ      0x02
#define LIS2DW12_RATE_25HZ        0x03
#define LIS2DW12_RATE_50HZ        0x04
#define LIS2DW12_RATE_100HZ       0x05
#define LIS2DW12_RATE_200HZ        0x06
#define LIS2DW12_RATE_400HZ       0x07
#define LIS2DW12_RATE_800HZ       0x08

/* 私有数据 */
typedef struct {
    uint8_t i2c_addr;
    uint8_t spi_cs;
    uint8_t range;
    uint8_t rate;
    uint8_t mode;
    int16_t offset[3];
} lis2dw12_priv_t;

/* 创建传感器设备 (I2C) */
sensor_device_t *lis2dw12_create(const char *name, void *i2c_bus, uint8_t addr);

/* 创建传感器设备 (SPI) */
sensor_device_t *lis2dw12_create_spi(const char *name, void *spi_bus, uint8_t cs);

/* 配置函数 */
int lis2dw12_set_range(sensor_device_t *dev, uint8_t range);
int lis2dw12_set_rate(sensor_device_t *dev, uint8_t rate);
int lis2dw12_set_mode(sensor_device_t *dev, uint8_t mode);
int lis2dw12_enable_high_pass(sensor_device_t *dev, uint8_t enable);

#endif /* __SENSOR_LIS2DW12_H__ */
