/**
 * @file xy_sensor_attr.h
 * @brief Sensor Attributes
 * @version 1.0.0
 * @date 2026-03-05
 */

#ifndef XY_SENSOR_ATTR_H
#define XY_SENSOR_ATTR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 传感器属性 ==================== */

/**
 * @brief 传感器属性类型
 */
typedef enum {
    XY_SENSOR_ATTR_NONE = 0,
    
    /* 采样配置 */
    XY_SENSOR_ATTR_SAMPLING_FREQUENCY,  /**< 采样频率 (Hz) */
    XY_SENSOR_ATTR_OVERSAMPLING,        /**< 过采样率 */
    
    /* 阈值配置 */
    XY_SENSOR_ATTR_LOWER_THRESH,        /**< 下阈值 */
    XY_SENSOR_ATTR_UPPER_THRESH,        /**< 上阈值 */
    XY_SENSOR_ATTR_SLOPE_THRESH,        /**< 斜率阈值 */
    XY_SENSOR_ATTR_HYSTERESIS,          /**< 迟滞 (%) */
    XY_SENSOR_ATTR_DURATION,            /**< 持续时间 (ms) */
    
    /* 量程配置 */
    XY_SENSOR_ATTR_FULL_SCALE,          /**< 满量程 */
    XY_SENSOR_ATTR_RANGE,               /**< 测量范围 */
    XY_SENSOR_ATTR_RESOLUTION,          /**< 分辨率 */
    
    /* 校准配置 */
    XY_SENSOR_ATTR_OFFSET,              /**< 偏移量 */
    XY_SENSOR_ATTR_SCALE,               /**< 比例因子 */
    XY_SENSOR_ATTR_CALIBRATION,         /**< 校准数据 */
    
    /* 电源配置 */
    XY_SENSOR_ATTR_POWER_MODE,          /**< 电源模式 */
    XY_SENSOR_ATTR_LOW_POWER,           /**< 低功耗使能 */
    
    /* FIFO 配置 */
    XY_SENSOR_ATTR_FIFO_ENABLE,         /**< FIFO 使能 */
    XY_SENSOR_ATTR_FIFO_WATERMARK,      /**< FIFO 水位 */
    XY_SENSOR_ATTR_FIFO_MODE,           /**< FIFO 模式 */
    
    /* 中断配置 */
    XY_SENSOR_ATTR_INTERRUPT_ENABLE,    /**< 中断使能 */
    XY_SENSOR_ATTR_INTERRUPT_POLARITY,  /**< 中断极性 */
    XY_SENSOR_ATTR_INTERRUPT_PIN,       /**< 中断引脚 */
    
} xy_sensor_attr_t;

/* ==================== 电源模式 ==================== */
/* Note: xy_sensor_power_mode_t is defined in xy_sensor_device.h */
/* This file only defines the attribute enum */

/* ==================== FIFO 模式 ==================== */

typedef enum {
    XY_SENSOR_FIFO_DISABLE = 0,     /**< 禁用 FIFO */
    XY_SENSOR_FIFO_STREAM,          /**< 流模式 */
    XY_SENSOR_FIFO_STOP,            /**< 停止模式 */
    XY_SENSOR_FIFO_BYPASS,          /**< 旁路模式 */
} xy_sensor_fifo_mode_t;

/* ==================== 满量程配置 ==================== */

/* 加速度计量程 */
typedef enum {
    XY_SENSOR_ACCEL_RANGE_2G = 0,   /**< ±2g */
    XY_SENSOR_ACCEL_RANGE_4G,       /**< ±4g */
    XY_SENSOR_ACCEL_RANGE_8G,       /**< ±8g */
    XY_SENSOR_ACCEL_RANGE_16G,      /**< ±16g */
} xy_sensor_accel_range_t;

/* 陀螺仪量程 */
typedef enum {
    XY_SENSOR_GYRO_RANGE_125 = 0,   /**< ±125 dps */
    XY_SENSOR_GYRO_RANGE_250,       /**< ±250 dps */
    XY_SENSOR_GYRO_RANGE_500,       /**< ±500 dps */
    XY_SENSOR_GYRO_RANGE_1000,      /**< ±1000 dps */
    XY_SENSOR_GYRO_RANGE_2000,      /**< ±2000 dps */
} xy_sensor_gyro_range_t;

/* 压力量程 */
typedef enum {
    XY_SENSOR_PRESSURE_RANGE_LOW = 0,   /**< 低压范围 */
    XY_SENSOR_PRESSURE_RANGE_HIGH,      /**< 高压范围 */
} xy_sensor_pressure_range_t;

#ifdef __cplusplus
}
#endif

#endif /* XY_SENSOR_ATTR_H */
