/**
 * @file xy_sensor.h
 * @brief Unified Sensor API - Reference: Zephyr Sensor Framework
 * @version 1.0.0
 * @date 2026-03-05
 */

#ifndef XY_SENSOR_H
#define XY_SENSOR_H

#include <stdint.h>
#include <stdbool.h>
#include "xy_sensor_channel.h"
#include "xy_sensor_attr.h"
#include "xy_sensor_device.h"
#include "xy_sensor_trigger.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 版本信息 ==================== */

#define XY_SENSOR_VERSION_MAJOR     1
#define XY_SENSOR_VERSION_MINOR     0
#define XY_SENSOR_VERSION_PATCH     0

/* ==================== 状态码 ==================== */

typedef enum {
    XY_SENSOR_OK = 0,
    XY_SENSOR_ERROR,
    XY_SENSOR_ERROR_INVALID_PARAM,
    XY_SENSOR_ERROR_NOT_SUPPORTED,
    XY_SENSOR_ERROR_NOT_INITIALIZED,
    XY_SENSOR_ERROR_TIMEOUT,
    XY_SENSOR_ERROR_NO_DATA,
    XY_SENSOR_ERROR_BUSY,
} xy_sensor_status_t;

/* ==================== 核心 API ==================== */

/**
 * @brief 初始化传感器
 * @param dev 传感器设备
 * @return 状态码
 */
xy_sensor_status_t xy_sensor_init(xy_sensor_device_t *dev);

/**
 * @brief 反初始化传感器
 * @param dev 传感器设备
 * @return 状态码
 */
xy_sensor_status_t xy_sensor_deinit(xy_sensor_device_t *dev);

/**
 * @brief 获取最新采样数据
 * @param dev 传感器设备
 * @param channel 通道类型 (XY_SENSOR_CHAN_ALL 表示所有通道)
 * @return 状态码
 */
xy_sensor_status_t xy_sensor_sample_fetch(xy_sensor_device_t *dev,
                                          xy_sensor_channel_t channel);

/**
 * @brief 读取通道数据
 * @param dev 传感器设备
 * @param channel 通道类型
 * @param val 输出值
 * @return 状态码
 */
xy_sensor_status_t xy_sensor_channel_get(xy_sensor_device_t *dev,
                                         xy_sensor_channel_t channel,
                                         xy_sensor_value_t *val);

/**
 * @brief 设置传感器属性
 * @param dev 传感器设备
 * @param channel 通道类型
 * @param attr 属性类型
 * @param val 属性值
 * @return 状态码
 */
xy_sensor_status_t xy_sensor_attr_set(xy_sensor_device_t *dev,
                                      xy_sensor_channel_t channel,
                                      xy_sensor_attr_t attr,
                                      const xy_sensor_value_t *val);

/**
 * @brief 获取传感器属性
 * @param dev 传感器设备
 * @param channel 通道类型
 * @param attr 属性类型
 * @param val 输出属性值
 * @return 状态码
 */
xy_sensor_status_t xy_sensor_attr_get(xy_sensor_device_t *dev,
                                      xy_sensor_channel_t channel,
                                      xy_sensor_attr_t attr,
                                      xy_sensor_value_t *val);

/**
 * @brief 设置触发器
 * @param dev 传感器设备
 * @param trigger 触发器配置
 * @return 状态码
 */
xy_sensor_status_t xy_sensor_trigger_set(xy_sensor_device_t *dev,
                                         const xy_sensor_trigger_t *trigger);

/**
 * @brief 禁用触发器
 * @param dev 传感器设备
 * @param type 触发器类型
 * @return 状态码
 */
xy_sensor_status_t xy_sensor_trigger_unset(xy_sensor_device_t *dev,
                                           xy_sensor_trigger_type_t type);

/* ==================== 设备管理 API ==================== */

/**
 * @brief 根据名称获取传感器设备
 * @param name 设备名称
 * @return 设备指针，NULL 表示未找到
 */
xy_sensor_device_t* xy_sensor_device_get(const char *name);

/**
 * @brief 根据类型获取传感器设备
 * @param type 传感器类型
 * @param index 索引 (同类型多个设备)
 * @return 设备指针，NULL 表示未找到
 */
xy_sensor_device_t* xy_sensor_device_get_by_type(xy_sensor_type_t type, uint8_t index);

/**
 * @brief 遍历所有传感器设备
 * @param callback 回调函数
 * @param user_data 用户数据
 */
void xy_sensor_device_foreach(void (*callback)(xy_sensor_device_t *, void *),
                              void *user_data);

/**
 * @brief 获取传感器数量
 * @return 传感器数量
 */
uint8_t xy_sensor_device_count(void);

/* ==================== 便捷 API ==================== */

/**
 * @brief 读取温度 (便捷 API)
 * @param dev 温度传感器
 * @param temp_c 输出温度 (摄氏度)
 * @return 状态码
 */
static inline xy_sensor_status_t xy_sensor_read_temp(xy_sensor_device_t *dev, 
                                                     float *temp_c)
{
    xy_sensor_value_t val;
    xy_sensor_status_t ret = xy_sensor_channel_get(dev, 
                                                   XY_SENSOR_CHAN_AMBIENT_TEMP, 
                                                   &val);
    if (ret == XY_SENSOR_OK && temp_c) {
        *temp_c = val.val1 + val.val2 / 1000000.0f;
    }
    return ret;
}

/**
 * @brief 读取湿度 (便捷 API)
 */
static inline xy_sensor_status_t xy_sensor_read_humidity(xy_sensor_device_t *dev,
                                                         float *humidity_pct)
{
    xy_sensor_value_t val;
    xy_sensor_status_t ret = xy_sensor_channel_get(dev,
                                                   XY_SENSOR_CHAN_HUMIDITY,
                                                   &val);
    if (ret == XY_SENSOR_OK && humidity_pct) {
        *humidity_pct = val.val1 + val.val2 / 1000000.0f;
    }
    return ret;
}

/**
 * @brief 读取压力 (便捷 API)
 */
static inline xy_sensor_status_t xy_sensor_read_pressure(xy_sensor_device_t *dev,
                                                         float *pressure_kpa)
{
    xy_sensor_value_t val;
    xy_sensor_status_t ret = xy_sensor_channel_get(dev,
                                                   XY_SENSOR_CHAN_PRESSURE,
                                                   &val);
    if (ret == XY_SENSOR_OK && pressure_kpa) {
        *pressure_kpa = val.val1 + val.val2 / 1000000.0f;
    }
    return ret;
}

/**
 * @brief 读取加速度 (便捷 API)
 */
static inline xy_sensor_status_t xy_sensor_read_accel(xy_sensor_device_t *dev,
                                                      float *accel_x,
                                                      float *accel_y,
                                                      float *accel_z)
{
    xy_sensor_value_t val;
    xy_sensor_status_t ret;
    
    if (accel_x) {
        ret = xy_sensor_channel_get(dev, XY_SENSOR_CHAN_ACCEL_X, &val);
        if (ret == XY_SENSOR_OK) *accel_x = val.val1 + val.val2 / 1000000.0f;
    }
    
    if (accel_y) {
        ret = xy_sensor_channel_get(dev, XY_SENSOR_CHAN_ACCEL_Y, &val);
        if (ret == XY_SENSOR_OK) *accel_y = val.val1 + val.val2 / 1000000.0f;
    }
    
    if (accel_z) {
        ret = xy_sensor_channel_get(dev, XY_SENSOR_CHAN_ACCEL_Z, &val);
        if (ret == XY_SENSOR_OK) *accel_z = val.val1 + val.val2 / 1000000.0f;
    }
    
    return XY_SENSOR_OK;
}

/**
 * @brief 读取角速度 (便捷 API)
 */
static inline xy_sensor_status_t xy_sensor_read_gyro(xy_sensor_device_t *dev,
                                                     float *gyro_x,
                                                     float *gyro_y,
                                                     float *gyro_z)
{
    xy_sensor_value_t val;
    xy_sensor_status_t ret;
    
    if (gyro_x) {
        ret = xy_sensor_channel_get(dev, XY_SENSOR_CHAN_GYRO_X, &val);
        if (ret == XY_SENSOR_OK) *gyro_x = val.val1 + val.val2 / 1000000.0f;
    }
    
    if (gyro_y) {
        ret = xy_sensor_channel_get(dev, XY_SENSOR_CHAN_GYRO_Y, &val);
        if (ret == XY_SENSOR_OK) *gyro_y = val.val1 + val.val2 / 1000000.0f;
    }
    
    if (gyro_z) {
        ret = xy_sensor_channel_get(dev, XY_SENSOR_CHAN_GYRO_Z, &val);
        if (ret == XY_SENSOR_OK) *gyro_z = val.val1 + val.val2 / 1000000.0f;
    }
    
    return XY_SENSOR_OK;
}

#ifdef __cplusplus
}
#endif

#endif /* XY_SENSOR_H */
