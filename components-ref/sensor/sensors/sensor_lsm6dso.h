/**
 * @file sensor_lsm6dso.h
 * @brief ST LSM6DSO 6轴惯性传感器驱动
 * @description I2C/SPI 双接口，支持加速度计+陀螺仪
 */
#ifndef __SENSOR_LSM6DSO_H__
#define __SENSOR_LSM6DSO_H__

#include "sensor_core.h"

/* I2C地址 */
#define LSM6DSO_ADDR_DEFAULT 0x6A
#define LSM6DSO_ADDR_ALT     0x6B

/* SPI片选 */
#define LSM6DSO_SPI_CS_NONE  0xFF

/* 寄存器定义 - WHO & ID */
#define LSM6DSO_REG_WHOAMI     0x0F
#define LSM6DSO_REG_CTRL1_XL   0x10
#define LSM6DSO_REG_CTRL2_G   0x11
#define LSM6DSO_REG_CTRL3_C   0x12
#define LSM6DSO_REG_CTRL4_C   0x13
#define LSM6DSO_REG_CTRL5_XL  0x15
#define LSM6DSO_REG_CTRL6_C   0x16
#define LSM6DSO_REG_CTRL7_G   0x17
#define LSM6DSO_REG_CTRL8_XL  0x18
#define LSM6DSO_REG_OUTX_L_G  0x22
#define LSM6DSO_REG_OUTX_L_XL 0x28
#define LSM6DSO_REG_STATUS_REG 0x1E

/* WHO_AM_I值 */
#define LSM6DSO_WHOAMI_VALUE  0x6C
#define LSM6DSO_ID            LSM6DSO_WHOAMI_VALUE

/* 加速度计量程 */
#define LSM6DSO_ACCEL_RANGE_2G   0x0
#define LSM6DSO_ACCEL_RANGE_4G   0x2
#define LSM6DSO_ACCEL_RANGE_8G   0x3
#define LSM6DSO_ACCEL_RANGE_16G  0x1

/* 陀螺仪量程 */
#define LSM6DSO_GYRO_RANGE_125DPS  0x1
#define LSM6DSO_GYRO_RANGE_250DPS  0x0
#define LSM6DSO_GYRO_RANGE_500DPS  0x2
#define LSM6DSO_GYRO_RANGE_1000DPS 0x3
#define LSM6DSO_GYRO_RANGE_2000DPS 0x1

/* 加速度计采样率 */
#define LSM6DSO_ACCEL_RATE_OFF     0x0
#define LSM6DSO_ACCEL_RATE_12_5Hz  0x1
#define LSM6DSO_ACCEL_RATE_26Hz    0x2
#define LSM6DSO_ACCEL_RATE_52Hz    0x3
#define LSM6DSO_ACCEL_RATE_104Hz   0x4
#define LSM6DSO_ACCEL_RATE_208Hz   0x5
#define LSM6DSO_ACCEL_RATE_416Hz   0x6
#define LSM6DSO_ACCEL_RATE_833Hz   0x7
#define LSM6DSO_ACCEL_RATE_1_66KHz 0x8
#define LSM6DSO_ACCEL_RATE_3_33KHz 0x9
#define LSM6DSO_ACCEL_RATE_6_66KHz 0xA

/* 陀螺仪采样率 */
#define LSM6DSO_GYRO_RATE_OFF     0x0
#define LSM6DSO_GYRO_RATE_12_5Hz  0x1
#define LSM6DSO_GYRO_RATE_26Hz    0x2
#define LSM6DSO_GYRO_RATE_52Hz    0x3
#define LSM6DSO_GYRO_RATE_104Hz   0x4
#define LSM6DSO_GYRO_RATE_208Hz   0x5
#define LSM6DSO_GYRO_RATE_416Hz   0x6
#define LSM6DSO_GYRO_RATE_833Hz   0x7
#define LSM6DSO_GYRO_RATE_1_66KHz 0x8
#define LSM6DSO_GYRO_RATE_3_33KHz 0x9
#define LSM6DSO_GYRO_RATE_6_66KHz 0xA

/* 私有数据 */
typedef struct {
    uint8_t i2c_addr;
    uint8_t spi_cs;
    uint8_t accel_range;
    uint8_t gyro_range;
    uint8_t accel_rate;
    uint8_t gyro_rate;
    int16_t accel_offset[3];
    int16_t gyro_offset[3];
} lsm6dso_priv_t;

/* 创建传感器设备 (I2C) */
sensor_device_t *lsm6dso_create_accel(const char *name, void *i2c_bus, uint8_t addr);
sensor_device_t *lsm6dso_create_gyro(const char *name, void *i2c_bus, uint8_t addr);
sensor_device_t *lsm6dso_create_imu(const char *name, void *i2c_bus, uint8_t addr);

/* 创建传感器设备 (SPI) */
sensor_device_t *lsm6dso_create_spi_accel(const char *name, void *spi_bus, uint8_t cs);
sensor_device_t *lsm6dso_create_spi_gyro(const char *name, void *spi_bus, uint8_t cs);
sensor_device_t *lsm6dso_create_spi_imu(const char *name, void *spi_bus, uint8_t cs);

/* 配置函数 */
int lsm6dso_set_accel_range(sensor_device_t *dev, uint8_t range);
int lsm6dso_set_gyro_range(sensor_device_t *dev, uint8_t range);
int lsm6dso_set_accel_rate(sensor_device_t *dev, uint8_t rate);
int lsm6dso_set_gyro_rate(sensor_device_t *dev, uint8_t rate);

#endif /* __SENSOR_LSM6DSO_H__ */
