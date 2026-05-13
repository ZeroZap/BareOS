/**
 * @file xy_mpu6050.h
 * @brief MPU6050 Accelerometer/Gyroscope Device Driver
 * @version 1.0.0
 * @date 2026-02-28
 */

#ifndef XY_MPU6050_H
#define XY_MPU6050_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_device.h"
#include <stdint.h>

/**
 * @brief MPU6050 设备结构
 */
typedef struct {
    xy_i2c_device_t i2c_dev;
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
    int16_t temperature;
} xy_mpu6050_t;

/**
 * @brief 初始化 MPU6050
 * @param mpu MPU6050 设备
 * @param i2c_handle I2C 句柄
 * @return XY_DEVICE_OK 成功
 */
int xy_mpu6050_init(xy_mpu6050_t *mpu, void *i2c_handle);

/**
 * @brief 读取传感器数据
 * @param mpu MPU6050 设备
 * @return XY_DEVICE_OK 成功
 */
int xy_mpu6050_read(xy_mpu6050_t *mpu);

/**
 * @brief 唤醒 MPU6050
 * @param mpu MPU6050 设备
 * @return XY_DEVICE_OK 成功
 */
int xy_mpu6050_wake(xy_mpu6050_t *mpu);

/**
 * @brief 进入睡眠模式
 * @param mpu MPU6050 设备
 * @return XY_DEVICE_OK 成功
 */
int xy_mpu6050_sleep(xy_mpu6050_t *mpu);

#ifdef __cplusplus
}
#endif

#endif /* XY_MPU6050_H */
