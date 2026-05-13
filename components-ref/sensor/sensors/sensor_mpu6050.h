#ifndef __SENSOR_MPU6050_H__
#define __SENSOR_MPU6050_H__

#include "sensor_core.h"

/* MPU6050 I2C地址 */
#define MPU6050_ADDR_DEFAULT 0x68
#define MPU6050_ADDR_ALT     0x69

/* 寄存器定义 */
#define MPU6050_REG_WHOAMI       0x75
#define MPU6050_REG_PWR_MGMT_1   0x6B
#define MPU6050_REG_PWR_MGMT_2   0x6C
#define MPU6050_REG_ACCEL_XOUT_H 0x3B
#define MPU6050_REG_GYRO_XOUT_H  0x43
#define MPU6050_REG_CONFIG       0x1A
#define MPU6050_REG_GYRO_CONFIG  0x1B
#define MPU6050_REG_ACCEL_CONFIG 0x1C
#define MPU6050_REG_INT_ENABLE   0x38
#define MPU6050_REG_INT_STATUS   0x3A

/* WHO_AM_I值 */
#define MPU6050_WHOAMI_VALUE 0x68

/* 私有数据 */
typedef struct {
    uint8_t i2c_addr;
    uint8_t accel_range;
    uint8_t gyro_range;
    int16_t accel_offset[3];
    int16_t gyro_offset[3];
} mpu6050_priv_t;

/* 创建传感器设备 */
sensor_device_t *mpu6050_create_accel(const char *name, void *i2c_bus);
sensor_device_t *mpu6050_create_gyro(const char *name, void *i2c_bus);

#endif /* __SENSOR_MPU6050_H__ */