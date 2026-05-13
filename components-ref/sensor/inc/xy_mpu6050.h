/**
 * @file xy_mpu6050.h
 * @brief MPU6050 6-Axis IMU Driver (Accelerometer + Gyroscope)
 * @version 1.0.0
 * @date 2026-03-01 凌晨
 */

#ifndef XY_MPU6050_H
#define XY_MPU6050_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_dev_i2c.h"
#include <stdint.h>

/**
 * @brief MPU6050 I2C 地址
 */
#define MPU6050_ADDR_AD0_LOW    0x68  /* AD0 -> GND */
#define MPU6050_ADDR_AD0_HIGH   0x69  /* AD0 -> VCC */

/**
 * @brief MPU6050 寄存器地址
 */
#define MPU6050_REG_SMPLRT_DIV      0x19
#define MPU6050_REG_CONFIG          0x1A
#define MPU6050_REG_GYRO_CONFIG     0x1B
#define MPU6050_REG_ACCEL_CONFIG    0x1C
#define MPU6050_REG_PWR_MGMT_1      0x6B
#define MPU6050_REG_PWR_MGMT_2      0x6C
#define MPU6050_REG_ACCEL_XOUT_H    0x3B
#define MPU6050_REG_GYRO_XOUT_H     0x43
#define MPU6050_REG_WHO_AM_I        0x75
#define MPU6050_WHO_AM_I_VALUE      0x68

/**
 * @brief 加速度计量程
 */
typedef enum {
    MPU6050_ACCEL_2G = 0,     /**< ±2g */
    MPU6050_ACCEL_4G = 1,     /**< ±4g */
    MPU6050_ACCEL_8G = 2,     /**< ±8g */
    MPU6050_ACCEL_16G = 3,    /**< ±16g */
} xy_mpu6050_accel_range_t;

/**
 * @brief 陀螺仪量程
 */
typedef enum {
    MPU6050_GYRO_250DPS = 0,  /**< ±250°/s */
    MPU6050_GYRO_500DPS = 1,  /**< ±500°/s */
    MPU6050_GYRO_1000DPS = 2, /**< ±1000°/s */
    MPU6050_GYRO_2000DPS = 3, /**< ±2000°/s */
} xy_mpu6050_gyro_range_t;

/**
 * @brief 数字低通滤波器配置
 */
typedef enum {
    MPU6050_DLPF_184HZ = 1,   /**< 184Hz */
    MPU6050_DLPF_94HZ = 2,    /**< 94Hz */
    MPU6050_DLPF_44HZ = 3,    /**< 44Hz */
    MPU6050_DLPF_21HZ = 4,    /**< 21Hz */
    MPU6050_DLPF_10HZ = 5,    /**< 10Hz */
    MPU6050_DLPF_5HZ = 6,     /**< 5Hz */
} xy_mpu6050_dlpf_t;

/**
 * @brief 错误码
 */
#define XY_MPU6050_OK           0
#define XY_MPU6050_ERROR        (-1)
#define XY_MPU6050_INVALID_PARAM (-2)
#define XY_MPU6050_NOT_FOUND    (-3)
#define XY_MPU6050_ID_ERROR     (-4)

/**
 * @brief MPU6050 原始数据
 */
typedef struct {
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
    int16_t temperature;
} xy_mpu6050_raw_data_t;

/**
 * @brief MPU6050 校准数据
 */
typedef struct {
    float accel_offset_x;
    float accel_offset_y;
    float accel_offset_z;
    float gyro_offset_x;
    float gyro_offset_y;
    float gyro_offset_z;
} xy_mpu6050_calib_t;

/**
 * @brief MPU6050 设备结构
 */
typedef struct {
    xy_i2c_device_t i2c_dev;      /**< I2C 设备 */
    uint8_t addr;                 /**< I2C 地址 */
    xy_mpu6050_accel_range_t accel_range;
    xy_mpu6050_gyro_range_t gyro_range;
    xy_mpu6050_dlpf_t dlpf;
    xy_mpu6050_raw_data_t raw;
    xy_mpu6050_calib_t calib;
    float accel_g[3];             /**< 加速度 (g) */
    float gyro_dps[3];            /**< 角速度 (°/s) */
    float temperature_c;          /**< 温度 (°C) */
    uint8_t initialized;
} xy_mpu6050_t;

/**
 * @brief 初始化 MPU6050
 * @param dev MPU6050 设备句柄
 * @param i2c_handle I2C 句柄
 * @param addr I2C 地址
 * @return XY_MPU6050_OK 成功，其他值失败
 */
int xy_mpu6050_init(xy_mpu6050_t *dev, void *i2c_handle, uint8_t addr);

/**
 * @brief 反初始化 MPU6050
 * @param dev MPU6050 设备句柄
 * @return XY_MPU6050_OK 成功，其他值失败
 */
int xy_mpu6050_deinit(xy_mpu6050_t *dev);

/**
 * @brief 读取原始数据
 * @param dev MPU6050 设备句柄
 * @return XY_MPU6050_OK 成功，其他值失败
 */
int xy_mpu6050_read_raw(xy_mpu6050_t *dev);

/**
 * @brief 读取加速度 (g)
 * @param dev MPU6050 设备句柄
 * @param ax X 轴加速度指针
 * @param ay Y 轴加速度指针
 * @param az Z 轴加速度指针
 * @return XY_MPU6050_OK 成功，其他值失败
 */
int xy_mpu6050_read_accel(xy_mpu6050_t *dev, float *ax, float *ay, float *az);

/**
 * @brief 读取角速度 (°/s)
 * @param dev MPU6050 设备句柄
 * @param gx X 轴角速度指针
 * @param gy Y 轴角速度指针
 * @param gz Z 轴角速度指针
 * @return XY_MPU6050_OK 成功，其他值失败
 */
int xy_mpu6050_read_gyro(xy_mpu6050_t *dev, float *gx, float *gy, float *gz);

/**
 * @brief 读取温度 (°C)
 * @param dev MPU6050 设备句柄
 * @param temp 温度指针
 * @return XY_MPU6050_OK 成功，其他值失败
 */
int xy_mpu6050_read_temperature(xy_mpu6050_t *dev, float *temp);

/**
 * @brief 设置加速度计量程
 * @param dev MPU6050 设备句柄
 * @param range 量程
 * @return XY_MPU6050_OK 成功，其他值失败
 */
int xy_mpu6050_set_accel_range(xy_mpu6050_t *dev, xy_mpu6050_accel_range_t range);

/**
 * @brief 设置陀螺仪量程
 * @param dev MPU6050 设备句柄
 * @param range 量程
 * @return XY_MPU6050_OK 成功，其他值失败
 */
int xy_mpu6050_set_gyro_range(xy_mpu6050_t *dev, xy_mpu6050_gyro_range_t range);

/**
 * @brief 校准传感器 (需要水平放置)
 * @param dev MPU6050 设备句柄
 * @param samples 采样次数
 * @return XY_MPU6050_OK 成功，其他值失败
 */
int xy_mpu6050_calibrate(xy_mpu6050_t *dev, uint16_t samples);

#ifdef __cplusplus
}
#endif

#endif /* XY_MPU6050_H */
