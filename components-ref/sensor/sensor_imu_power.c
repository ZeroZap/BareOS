/**
 * @file sensor_imu_power.c
 * @brief IMU 低功耗模式管理实现
 */
#include "sensor_imu_power.h"
#include <string.h>

/* ODR 查找表 */
static const uint16_t imu_odr_table[] = IMU_ODR_TABLE;

/**
 * @brief ODR 转寄存器值
 */
uint8_t sensor_imu_odr_to_reg(uint16_t odr)
{
    for (int i = 0; i < sizeof(imu_odr_table); i++) {
        if (imu_odr_table[i] >= odr) {
            return i;
        }
    }
    return 4; /* 默认 104Hz */
}

/**
 * @brief 设置 IMU 功耗模式
 */
sensor_err_t sensor_imu_set_power_mode(sensor_device_t *dev, imu_power_mode_t mode)
{
    if (dev == NULL) {
        return SENSOR_ERROR;
    }

    SENSOR_LOG("Set IMU %s to %s mode", dev->info.name, imu_power_mode_str(mode));

    /* 获取目标 ODR */
    uint16_t accel_odr, gyro_odr;
    imu_power_get_odr(mode, &accel_odr, &gyro_odr);

    /* 如果驱动实现了 set_odr 函数则调用 */
    if (dev->ops && dev->ops->config) {
        dev->ops->config(dev, SENSOR_CONFIG_TYPE_ACCEL_ODR, &accel_odr);
        dev->ops->config(dev, SENSOR_CONFIG_TYPE_GYRO_ODR, &gyro_odr);
    }

    /* 更新设备模式 */
    dev->odr = accel_odr;
    
    return SENSOR_EOK;
}

/**
 * @brief 设置加速度计 ODR
 */
sensor_err_t sensor_imu_set_accel_odr(sensor_device_t *dev, uint16_t odr)
{
    if (dev == NULL || dev->ops == NULL || dev->ops->config == NULL) {
        return SENSOR_ERROR;
    }
    return dev->ops->config(dev, SENSOR_CONFIG_TYPE_ACCEL_ODR, &odr);
}

/**
 * @brief 设置陀螺仪 ODR
 */
sensor_err_t sensor_imu_set_gyro_odr(sensor_device_t *dev, uint16_t odr)
{
    if (dev == NULL || dev->ops == NULL || dev->ops->config == NULL) {
        return SENSOR_ERROR;
    }
    return dev->ops->config(dev, SENSOR_CONFIG_TYPE_GYRO_ODR, &odr);
}

/**
 * @brief 设置加速度计量程
 */
sensor_err_t sensor_imu_set_accel_range(sensor_device_t *dev, uint8_t range)
{
    if (dev == NULL || dev->ops == NULL || dev->ops->config == NULL) {
        return SENSOR_ERROR;
    }
    return dev->ops->config(dev, SENSOR_CONFIG_TYPE_ACCEL_RANGE, &range);
}

/**
 * @brief 设置陀螺仪量程
 */
sensor_err_t sensor_imu_set_gyro_range(sensor_device_t *dev, uint8_t range)
{
    if (dev == NULL || dev->ops == NULL || dev->ops->config == NULL) {
        return SENSOR_ERROR;
    }
    return dev->ops->config(dev, SENSOR_CONFIG_TYPE_GYRO_RANGE, &range);
}

/**
 * @brief 获取当前功耗模式
 */
sensor_err_t sensor_imu_get_power_mode(sensor_device_t *dev, imu_power_mode_t *mode)
{
    if (dev == NULL || mode == NULL) {
        return SENSOR_ERROR;
    }

    /* 根据 ODR 判断当前模式 */
    uint16_t odr = dev->odr;
    if (odr >= 3330) {
        *mode = IMU_POWER_MODE_HIGH_PERFORMANCE;
    } else if (odr >= 208) {
        *mode = IMU_POWER_MODE_NORMAL;
    } else if (odr >= 52) {
        *mode = IMU_POWER_MODE_LOW_POWER;
    } else if (odr >= 12) {
        *mode = IMU_POWER_MODE_ULTRA_LOW_POWER;
    } else if (odr > 0) {
        *mode = IMU_POWER_MODE_SLEEP;
    } else {
        *mode = IMU_POWER_MODE_OFF;
    }

    return SENSOR_EOK;
}
