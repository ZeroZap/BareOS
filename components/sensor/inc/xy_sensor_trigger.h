/**
 * @file xy_sensor_trigger.h
 * @brief Sensor Trigger Mechanism
 * @version 1.0.0
 * @date 2026-03-05
 */

#ifndef XY_SENSOR_TRIGGER_H
#define XY_SENSOR_TRIGGER_H

#include <stdint.h>
#include "xy_sensor_channel.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 触发器类型 ==================== */

/**
 * @brief 触发器类型
 */
typedef enum {
    XY_SENSOR_TRIG_NONE = 0,
    XY_SENSOR_TRIG_DATA_READY,      /**< 数据就绪触发 */
    XY_SENSOR_TRIG_THRESHOLD,       /**< 阈值触发 */
    XY_SENSOR_TRIG_TAP,             /**< 敲击检测 */
    XY_SENSOR_TRIG_MOTION,          /**< 运动检测 */
    XY_SENSOR_TRIG_FREEFALL,        /**< 自由落体检测 */
    XY_SENSOR_TRIG_FIFO_FULL,       /**< FIFO 满 */
    XY_SENSOR_TRIG_FIFO_WATERMARK,  /**< FIFO 水位 */
    XY_SENSOR_TRIG_EXTERNAL,        /**< 外部中断 */
} xy_sensor_trigger_type_t;

/* ==================== 触发器配置 ==================== */

/**
 * @brief 触发器结构
 */
typedef struct xy_sensor_trigger {
    xy_sensor_trigger_type_t type;  /**< 触发类型 */
    xy_sensor_channel_t channel;    /**< 关联通道 */
    
    /**
     * @brief 触发回调函数
     * @param dev 传感器设备
     * @param trigger 触发器
     */
    void (*trigger_handler)(struct xy_sensor_device *dev,
                            const struct xy_sensor_trigger *trigger);
    
    void *user_data;                /**< 用户数据 */
} xy_sensor_trigger_t;

/* ==================== 阈值配置 ==================== */

/**
 * @brief 阈值配置
 */
typedef struct {
    xy_sensor_value_t lower;        /**< 下阈值 */
    xy_sensor_value_t upper;        /**< 上阈值 */
    xy_sensor_value_t slope;        /**< 斜率阈值 */
    uint16_t duration;              /**< 持续时间 (ms) */
    uint8_t hysteresis;             /**< 迟滞 (%) */
} xy_sensor_thresh_config_t;

/* ==================== FIFO 配置 ==================== */

/**
 * @brief FIFO 配置
 */
typedef struct {
    uint16_t watermark;             /**< 水位标记 */
    uint16_t size;                  /**< FIFO 大小 */
    uint8_t mode;                   /**< FIFO 模式 */
} xy_sensor_fifo_config_t;

/* ==================== API ==================== */

/**
 * @brief 使能触发器
 */
int xy_sensor_trigger_enable(xy_sensor_device_t *dev,
                             xy_sensor_trigger_type_t type);

/**
 * @brief 禁用触发器
 */
int xy_sensor_trigger_disable(xy_sensor_device_t *dev,
                              xy_sensor_trigger_type_t type);

/**
 * @brief 设置阈值配置
 */
int xy_sensor_trigger_set_thresh(xy_sensor_device_t *dev,
                                 xy_sensor_channel_t channel,
                                 const xy_sensor_thresh_config_t *config);

/**
 * @brief 设置 FIFO 配置
 */
int xy_sensor_trigger_set_fifo(xy_sensor_device_t *dev,
                               const xy_sensor_fifo_config_t *config);

/**
 * @brief 触发器中断处理 (内部使用)
 */
void xy_sensor_trigger_fire(xy_sensor_device_t *dev,
                            xy_sensor_trigger_type_t type);

#ifdef __cplusplus
}
#endif

#endif /* XY_SENSOR_TRIGGER_H */
