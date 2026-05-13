/**
 * @file xy_mpu6050.c
 * @brief MPU6050 Accelerometer/Gyroscope Device Driver Implementation
 * @version 1.0.0
 * @date 2026-02-28
 */

#include "xy_mpu6050.h"
#include <string.h>

/* MPU6050 Registers */
#define MPU6050_REG_PWR_MGMT_1      0x6B
#define MPU6050_REG_PWR_MGMT_2      0x6C
#define MPU6050_REG_ACCEL_XOUT_H    0x3B
#define MPU6050_REG_GYRO_XOUT_H     0x43
#define MPU6050_REG_TEMP_OUT_H      0x41
#define MPU6050_REG_WHO_AM_I        0x75
#define MPU6050_WHO_AM_I_VALUE      0x68

int xy_mpu6050_init(xy_mpu6050_t *mpu, void *i2c_handle)
{
    if (!mpu || !i2c_handle) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    memset(mpu, 0, sizeof(*mpu));
    xy_i2c_device_init(&mpu->i2c_dev, i2c_handle, 0x68, 1000);
    
    /* Check WHO_AM_I */
    uint8_t who_am_i;
    xy_i2c_device_read_reg(&mpu->i2c_dev, MPU6050_REG_WHO_AM_I, &who_am_i, 1);
    
    if (who_am_i != MPU6050_WHO_AM_I_VALUE) {
        return XY_DEVICE_NOT_FOUND;
    }
    
    return xy_mpu6050_wake(mpu);
}

int xy_mpu6050_wake(xy_mpu6050_t *mpu)
{
    if (!mpu) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    /* Wake up from sleep */
    uint8_t cfg = 0x00;
    return xy_i2c_device_write_reg(&mpu->i2c_dev, MPU6050_REG_PWR_MGMT_1, &cfg, 1);
}

int xy_mpu6050_sleep(xy_mpu6050_t *mpu)
{
    if (!mpu) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    /* Enter sleep mode */
    uint8_t cfg = 0x40;
    return xy_i2c_device_write_reg(&mpu->i2c_dev, MPU6050_REG_PWR_MGMT_1, &cfg, 1);
}

int xy_mpu6050_read(xy_mpu6050_t *mpu)
{
    if (!mpu) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    uint8_t buffer[14];
    int result = xy_i2c_device_read_reg(&mpu->i2c_dev, MPU6050_REG_ACCEL_XOUT_H, buffer, 14);
    
    if (result < 0) {
        return result;
    }
    
    /* Parse accelerometer data */
    mpu->accel_x = (buffer[0] << 8) | buffer[1];
    mpu->accel_y = (buffer[2] << 8) | buffer[3];
    mpu->accel_z = (buffer[4] << 8) | buffer[5];
    
    /* Parse temperature data */
    mpu->temperature = (buffer[6] << 8) | buffer[7];
    
    /* Parse gyroscope data */
    mpu->gyro_x = (buffer[8] << 8) | buffer[9];
    mpu->gyro_y = (buffer[10] << 8) | buffer[11];
    mpu->gyro_z = (buffer[12] << 8) | buffer[13];
    
    return XY_DEVICE_OK;
}
