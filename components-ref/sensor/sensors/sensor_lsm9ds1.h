/**
 * @file sensor_lsm9ds1.h
 * @brief ST LSM9DS1 9轴惯性传感器(加速度+陀螺仪+磁力计)
 */
#ifndef __SENSOR_LSM9DS1_H__
#define __SENSOR_LSM9DS1_H__

#include "sensor_core.h"

/* IMU I2C地址 */
#define LSM9DS1_IMU_ADDR_DEFAULT 0x6A
#define LSM9DS1_IMU_ADDR_ALT     0x6B

/* Magnetometer I2C地址 */
#define LSM9DS1_MAG_ADDR_DEFAULT 0x1C
#define LSM9DS1_MAG_ADDR_ALT     0x1E

#define LSM9DS1_SPI_CS_NONE  0xFF

/* IMU寄存器 */
#define LSM9DS1_REG_WHOAMI_IMU   0x0F
#define LSM9DS1_REG_CTRL1_XL     0x10
#define LSM9DS1_REG_CTRL2_G     0x11
#define LSM9DS1_REG_CTRL3_C     0x12
#define LSM9DS1_REG_OUTX_L_G    0x22
#define LSM9DS1_REG_OUTX_L_XL   0x28

/* Magnetometer寄存器 */
#define LSM9DS1_REG_WHOAMI_MAG   0x0F
#define LSM9DS1_REG_CTRL_REG1_M  0x20
#define LSM9DS1_REG_OUTX_L_M    0x28

#define LSM9DS1_IMU_WHOAMI_VALUE  0x68
#define LSM9DS1_MAG_WHOAMI_VALUE  0x3D

#define LSM9DS1_ACCEL_RANGE_2G   0x0
#define LSM9DS1_ACCEL_RANGE_4G   0x2
#define LSM9DS1_ACCEL_RANGE_8G   0x3
#define LSM9DS1_ACCEL_RANGE_16G  0x1

#define LSM9DS1_GYRO_RANGE_250DPS  0x0
#define LSM9DS1_GYRO_RANGE_500DPS  0x2
#define LSM9DS1_GYRO_RANGE_1000DPS 0x3
#define LSM9DS1_GYRO_RANGE_2000DPS 0x1

typedef struct {
    uint8_t imu_addr;
    uint8_t mag_addr;
    uint8_t spi_cs;
    uint8_t accel_range;
    uint8_t gyro_range;
} lsm9ds1_priv_t;

sensor_device_t *lsm9ds1_create_accel(const char *name, void *i2c_bus);
sensor_device_t *lsm9ds1_create_gyro(const char *name, void *i2c_bus);
sensor_device_t *lsm9ds1_create_mag(const char *name, void *i2c_bus);

#endif
