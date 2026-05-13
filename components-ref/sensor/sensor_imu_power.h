/**
 * @file sensor_imu_power.h
 * @brief IMU 传感器低功耗模式管理接口
 */
#ifndef __SENSOR_IMU_POWER_H__
#define __SENSOR_IMU_POWER_H__

#include "sensor_core.h"

/* IMU ODR 查找表 */
#define IMU_ODR_TABLE {0, 1, 12, 26, 52, 104, 208, 416, 833, 1660, 3330, 6660}

/**
 * @brief IMU 功耗模式
 */
typedef enum {
    IMU_POWER_MODE_HIGH_PERFORMANCE = 0,  /**< 高性能模式 6.6kHz */
    IMU_POWER_MODE_NORMAL,                  /**< 正常模式 208Hz */
    IMU_POWER_MODE_LOW_POWER,               /**< 低功耗模式 52Hz */
    IMU_POWER_MODE_ULTRA_LOW_POWER,         /**< 超低功耗模式 12Hz */
    IMU_POWER_MODE_SLEEP,                   /**< 睡眠模式 1Hz */
    IMU_POWER_MODE_OFF,                     /**< 关闭模式 */
    IMU_POWER_MODE_MAX
} imu_power_mode_t;

/**
 * @brief IMU 功耗模式配置
 */
typedef struct {
    imu_power_mode_t mode;     /**< 功耗模式 */
    uint16_t accel_odr;        /**< 加速度计 ODR (Hz) */
    uint16_t gyro_odr;         /**< 陀螺仪 ODR (Hz) */
    uint8_t accel_range;       /**< 加速度计量程 (g) */
    uint8_t gyro_range;        /**< 陀螺仪量程 (dps) */
} imu_power_config_t;

/**
 * @brief 获取功耗模式对应 ODR
 */
static inline void imu_power_get_odr(imu_power_mode_t mode, uint16_t *accel_odr, uint16_t *gyro_odr)
{
    switch (mode) {
    case IMU_POWER_MODE_HIGH_PERFORMANCE:
        *accel_odr = 6660; *gyro_odr = 6660;
        break;
    case IMU_POWER_MODE_NORMAL:
        *accel_odr = 208; *gyro_odr = 208;
        break;
    case IMU_POWER_MODE_LOW_POWER:
        *accel_odr = 52; *gyro_odr = 52;
        break;
    case IMU_POWER_MODE_ULTRA_LOW_POWER:
        *accel_odr = 12; *gyro_odr = 12;
        break;
    case IMU_POWER_MODE_SLEEP:
        *accel_odr = 1; *gyro_odr = 1;
        break;
    case IMU_POWER_MODE_OFF:
    default:
        *accel_odr = 0; *gyro_odr = 0;
        break;
    }
}

/**
 * @brief 功耗模式转字符串
 */
static inline const char *imu_power_mode_str(imu_power_mode_t mode)
{
    static const char *names[] = {
        "HP", "Normal", "LP", "ULP", "Sleep", "Off"
    };
    if (mode < IMU_POWER_MODE_MAX) {
        return names[mode];
    }
    return "Unknown";
}

/**
 * @brief ODR 转寄存器值
 */
uint8_t sensor_imu_odr_to_reg(uint16_t odr);

/**
 * @brief 设置 IMU 功耗模式 (通用接口)
 */
sensor_err_t sensor_imu_set_power_mode(sensor_device_t *dev, imu_power_mode_t mode);

/**
 * @brief 设置加速度计 ODR
 */
sensor_err_t sensor_imu_set_accel_odr(sensor_device_t *dev, uint16_t odr);

/**
 * @brief 设置陀螺仪 ODR
 */
sensor_err_t sensor_imu_set_gyro_odr(sensor_device_t *dev, uint16_t odr);

/**
 * @brief 设置加速度计量程
 */
sensor_err_t sensor_imu_set_accel_range(sensor_device_t *dev, uint8_t range);

/**
 * @brief 设置陀螺仪量程
 */
sensor_err_t sensor_imu_set_gyro_range(sensor_device_t *dev, uint8_t range);

/**
 * @brief 获取当前功耗模式
 */
sensor_err_t sensor_imu_get_power_mode(sensor_device_t *dev, imu_power_mode_t *mode);

/* 默认配置 */
#define IMU_POWER_CONFIG_DEFAULT { \
    .mode = IMU_POWER_MODE_NORMAL, \
    .accel_odr = 100, \
    .gyro_odr = 100, \
    .accel_range = 2, \
    .gyro_range = 250, \
}

#endif
