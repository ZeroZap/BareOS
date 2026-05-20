/**
 * @file xy_sensor_device.h
 * @brief Sensor Device Model
 * @version 1.0.0
 * @date 2026-03-05
 */

#ifndef XY_SENSOR_DEVICE_H
#define XY_SENSOR_DEVICE_H

#include <stdint.h>
#include "xy_typedef.h"
#include "xy_typedef.h"
#include "xy_sensor_channel.h"
#include "xy_sensor_attr.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
struct xy_sensor_device;
struct xy_sensor_trigger;

/* ==================== 传感器类型 ==================== */

typedef enum {
    XY_SENSOR_TYPE_NONE = 0,
    XY_SENSOR_TYPE_TEMP,          /**< 温度传感器 */
    XY_SENSOR_TYPE_HUMIDITY,      /**< 湿度传感器 */
    XY_SENSOR_TYPE_PRESSURE,      /**< 压力传感器 */
    XY_SENSOR_TYPE_ACCEL,         /**< 加速度计 */
    XY_SENSOR_TYPE_GYRO,          /**< 陀螺仪 */
    XY_SENSOR_TYPE_MAGN,          /**< 磁力计 */
    XY_SENSOR_TYPE_LIGHT,         /**< 光线传感器 */
    XY_SENSOR_TYPE_PROX,          /**< 接近传感器 */
    XY_SENSOR_TYPE_GAS,           /**< 气体传感器 */
    XY_SENSOR_TYPE_POWER,         /**< 电源传感器 */
    XY_SENSOR_TYPE_GPS,           /**< GPS 模块 */
    XY_SENSOR_TYPE_COMBO,         /**< 组合传感器 */
    XY_SENSOR_TYPE_COUNT,
} xy_sensor_type_t;

/* ==================== 总线类型 ==================== */

typedef enum {
    XY_SENSOR_BUS_NONE = 0,
    XY_SENSOR_BUS_I2C,
    XY_SENSOR_BUS_SPI,
    XY_SENSOR_BUS_UART,
} xy_sensor_bus_type_t;

/**
 * @brief 总线信息
 */
typedef struct {
    xy_sensor_bus_type_t type;    /**< 总线类型 */
    void *bus_handle;             /**< 总线句柄 */
    uint8_t address;              /**< 设备地址 (I2C/SPI) */
    uint8_t chip_select;          /**< 片选引脚 (SPI) */
} xy_sensor_bus_t;

/* ==================== 驱动 API ==================== */

/**
 * @brief 传感器驱动 API
 */
typedef struct {
    int (*init)(struct xy_sensor_device *dev);
    int (*sample_fetch)(struct xy_sensor_device *dev, xy_sensor_channel_t channel);
    int (*channel_get)(struct xy_sensor_device *dev, 
                       xy_sensor_channel_t channel, 
                       xy_sensor_value_t *val);
    int (*attr_set)(struct xy_sensor_device *dev,
                    xy_sensor_channel_t channel,
                    xy_sensor_attr_t attr,
                    const xy_sensor_value_t *val);
    int (*attr_get)(struct xy_sensor_device *dev,
                    xy_sensor_channel_t channel,
                    xy_sensor_attr_t attr,
                    xy_sensor_value_t *val);
    int (*trigger_set)(struct xy_sensor_device *dev,
                       const struct xy_sensor_trigger *trigger);
} xy_sensor_driver_api_t;

/* ==================== 电源模式 ==================== */

typedef enum {
    XY_SENSOR_POWER_MODE_NORMAL = 0,    /**< 正常模式 */
    XY_SENSOR_POWER_MODE_SLEEP,         /**< 睡眠模式 */
    XY_SENSOR_POWER_MODE_LOW_POWER,     /**< 低功耗模式 */
    XY_SENSOR_POWER_MODE_OFF,           /**< 关闭 */
} xy_sensor_power_mode_t;

/* ==================== 传感器设备 ==================== */

/**
 * @brief 传感器设备结构
 */
typedef struct xy_sensor_device {
    const char *name;                   /**< 设备名称 */
    xy_sensor_type_t type;              /**< 传感器类型 */
    const xy_sensor_driver_api_t *api;  /**< 驱动接口 */
    void *data;                         /**< 私有数据 */
    xy_sensor_bus_t bus;                /**< 总线信息 */
    xy_sensor_power_mode_t power_mode;  /**< 电源模式 */
    bool initialized;                   /**< 初始化标志 */
    uint32_t sample_count;              /**< 采样计数 */
    uint32_t last_sample_time;          /**< 上次采样时间 */
    
    struct xy_sensor_device *next;      /**< 下一设备 (链表) */
} xy_sensor_device_t;

/* ==================== 设备注册 ==================== */

/**
 * @brief 注册传感器设备
 */
int xy_sensor_device_register(xy_sensor_device_t *dev);

/**
 * @brief 注销传感器设备
 */
int xy_sensor_device_unregister(xy_sensor_device_t *dev);

/* ==================== 设备信息 ==================== */

/**
 * @brief 获取设备名称
 */
static inline const char* xy_sensor_device_get_name(xy_sensor_device_t *dev)
{
    return dev ? dev->name : NULL;
}

/**
 * @brief 获取设备类型
 */
static inline xy_sensor_type_t xy_sensor_device_get_type(xy_sensor_device_t *dev)
{
    return dev ? dev->type : XY_SENSOR_TYPE_NONE;
}

/**
 * @brief 检查设备是否初始化
 */
static inline bool xy_sensor_device_is_ready(xy_sensor_device_t *dev)
{
    return dev && dev->initialized;
}

/**
 * @brief 获取采样计数
 */
static inline uint32_t xy_sensor_device_get_sample_count(xy_sensor_device_t *dev)
{
    return dev ? dev->sample_count : 0;
}

#ifdef __cplusplus
}
#endif

#endif /* XY_SENSOR_DEVICE_H */
