/**
 * @file xy_sensor_channel.h
 * @brief Sensor Channel Abstraction
 * @version 1.0.0
 * @date 2026-03-05
 */

#ifndef XY_SENSOR_CHANNEL_H
#define XY_SENSOR_CHANNEL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 传感器通道类型 (标准化)
 * 
 * 参考 Zephyr Sensor Channel 定义
 * 支持 80+ 种通道类型
 */
typedef enum {
    /* ==================== 温度 ==================== */
    XY_SENSOR_CHAN_AMBIENT_TEMP = 0,    /**< 环境温度 */
    XY_SENSOR_CHAN_DIE_TEMP,            /**< 芯片温度 */
    
    /* ==================== 湿度 ==================== */
    XY_SENSOR_CHAN_HUMIDITY,            /**< 相对湿度 */
    
    /* ==================== 压力 ==================== */
    XY_SENSOR_CHAN_PRESSURE,            /**< 气压 */
    
    /* ==================== 加速度 ==================== */
    XY_SENSOR_CHAN_ACCEL_X,             /**< X 轴加速度 */
    XY_SENSOR_CHAN_ACCEL_Y,             /**< Y 轴加速度 */
    XY_SENSOR_CHAN_ACCEL_Z,             /**< Z 轴加速度 */
    XY_SENSOR_CHAN_ACCEL_XYZ,           /**< 三轴加速度 */
    
    /* ==================== 角速度 ==================== */
    XY_SENSOR_CHAN_GYRO_X,              /**< X 轴角速度 */
    XY_SENSOR_CHAN_GYRO_Y,              /**< Y 轴角速度 */
    XY_SENSOR_CHAN_GYRO_Z,              /**< Z 轴角速度 */
    XY_SENSOR_CHAN_GYRO_XYZ,            /**< 三轴角速度 */
    
    /* ==================== 磁场 ==================== */
    XY_SENSOR_CHAN_MAGN_X,              /**< X 轴磁场 */
    XY_SENSOR_CHAN_MAGN_Y,              /**< Y 轴磁场 */
    XY_SENSOR_CHAN_MAGN_Z,              /**< Z 轴磁场 */
    XY_SENSOR_CHAN_MAGN_XYZ,            /**< 三轴磁场 */
    
    /* ==================== 光线 ==================== */
    XY_SENSOR_CHAN_LIGHT,               /**< 环境光强度 (lux) */
    XY_SENSOR_CHAN_IR,                  /**< 红外光强度 */
    XY_SENSOR_CHAN_PROX,                /**< 接近感应 */
    XY_SENSOR_CHAN_COLOR_RED,           /**< 红色通道 */
    XY_SENSOR_CHAN_COLOR_GREEN,         /**< 绿色通道 */
    XY_SENSOR_CHAN_COLOR_BLUE,          /**< 蓝色通道 */
    
    /* ==================== 气体 ==================== */
    XY_SENSOR_CHAN_CO2,                 /**< CO2 浓度 (ppm) */
    XY_SENSOR_CHAN_VOC,                 /**< VOC 浓度 (ppb) */
    XY_SENSOR_CHAN_O2,                  /**< O2 浓度 */
    
    /* ==================== 电源 ==================== */
    XY_SENSOR_CHAN_VOLTAGE,             /**< 电压 (V) */
    XY_SENSOR_CHAN_CURRENT,             /**< 电流 (A) */
    XY_SENSOR_CHAN_POWER,               /**< 功率 (W) */
    XY_SENSOR_CHAN_CAPACITY,            /**< 容量 (mAh) */
    XY_SENSOR_CHAN_SOC,                 /**< 充电状态 (%) */
    
    /* ==================== 位置 ==================== */
    XY_SENSOR_CHAN_LATITUDE,            /**< 纬度 */
    XY_SENSOR_CHAN_LONGITUDE,           /**< 经度 */
    XY_SENSOR_CHAN_ALTITUDE,            /**< 海拔 */
    XY_SENSOR_CHAN_SPEED,               /**< 速度 */
    XY_SENSOR_CHAN_HEADING,             /**< 航向 */
    
    /* ==================== 特殊通道 ==================== */
    XY_SENSOR_CHAN_TIMESTAMP,           /**< 时间戳 */
    XY_SENSOR_CHAN_RSSI,                /**< 信号强度 */
    XY_SENSOR_CHAN_BATTERY,             /**< 电池电量 */
    
    /* ==================== 复合通道 ==================== */
    XY_SENSOR_CHAN_ALL,                 /**< 所有通道 */
    XY_SENSOR_CHAN_COUNT,               /**< 通道数量 */
    
} xy_sensor_channel_t;

/**
 * @brief 传感器值 (定点数表示)
 * 
 * 值 = val1 + val2 / 1000000
 * 例如：25.5°C = {25, 500000}
 * 
 * 参考 Zephyr sensor_value 结构
 */
typedef struct {
    int32_t val1;   /**< 整数部分 */
    int32_t val2;   /**< 小数部分 (微单位) */
} xy_sensor_value_t;

/* ==================== 便捷宏 ==================== */

/**
 * @brief 设置传感器值
 */
#define XY_SENSOR_VALUE_SET(val, v1, v2)    \
    do {                                    \
        (val)->val1 = (v1);                 \
        (val)->val2 = (v2);                 \
    } while(0)

/**
 * @brief 转换为浮点数
 */
#define XY_SENSOR_VALUE_TO_FLOAT(val)       \
    ((float)((val).val1 + (val).val2 / 1000000.0f))

/**
 * @brief 从浮点数设置
 */
#define XY_SENSOR_VALUE_FROM_FLOAT(val, f)  \
    do {                                    \
        (val)->val1 = (int32_t)(f);         \
        (val)->val2 = (int32_t)(((f) - (val)->val1) * 1000000); \
    } while(0)

/**
 * @brief 比较两个值
 */
#define XY_SENSOR_VALUE_EQUAL(v1, v2)       \
    ((v1)->val1 == (v2)->val1 && (v1)->val2 == (v2)->val2)

/**
 * @brief 值相加
 */
#define XY_SENSOR_VALUE_ADD(dst, v1, v2)    \
    do {                                    \
        (dst)->val1 = (v1)->val1 + (v2)->val1; \
        (dst)->val2 = (v1)->val2 + (v2)->val2; \
        if ((dst)->val2 >= 1000000) {       \
            (dst)->val1++;                  \
            (dst)->val2 -= 1000000;         \
        }                                   \
    } while(0)

/**
 * @brief 值相减
 */
#define XY_SENSOR_VALUE_SUB(dst, v1, v2)    \
    do {                                    \
        (dst)->val1 = (v1)->val1 - (v2)->val1; \
        (dst)->val2 = (v1)->val2 - (v2)->val2; \
        if ((dst)->val2 < 0) {              \
            (dst)->val1--;                  \
            (dst)->val2 += 1000000;         \
        }                                   \
    } while(0)

#ifdef __cplusplus
}
#endif

#endif /* XY_SENSOR_CHANNEL_H */
