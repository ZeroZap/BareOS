/**
 * @file sensor_trigger.c
 * @brief Sensor Trigger Subsystem
 * @version 1.0.0
 * @date 2026-03-05
 */

#include "xy_sensor.h"
#include "xy_log.h"
#include "xy_string.h"

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* ==================== 触发器管理 ==================== */

/**
 * @brief 使能触发器
 */
int xy_sensor_trigger_enable(xy_sensor_device_t *dev,
                             xy_sensor_trigger_type_t type)
{
    if (!dev || !dev->initialized) {
        return -1;
    }
    
    xy_sensor_trigger_t trigger = {
        .type = type,
        .channel = XY_SENSOR_CHAN_ALL,
    };
    
    return xy_sensor_trigger_set(dev, &trigger);
}

/**
 * @brief 禁用触发器
 */
int xy_sensor_trigger_disable(xy_sensor_device_t *dev,
                              xy_sensor_trigger_type_t type)
{
    if (!dev || !dev->initialized) {
        return -1;
    }
    
    return xy_sensor_trigger_unset(dev, type);
}

/**
 * @brief 设置阈值配置
 */
int xy_sensor_trigger_set_thresh(xy_sensor_device_t *dev,
                                 xy_sensor_channel_t channel,
                                 const xy_sensor_thresh_config_t *config)
{
    if (!dev || !config) {
        return -1;
    }
    
    /* 设置下阈值 */
    xy_sensor_value_t val;
    val.val1 = config->lower.val1;
    val.val2 = config->lower.val2;
    xy_sensor_attr_set(dev, channel, XY_SENSOR_ATTR_LOWER_THRESH, &val);
    
    /* 设置上阈值 */
    val.val1 = config->upper.val1;
    val.val2 = config->upper.val2;
    xy_sensor_attr_set(dev, channel, XY_SENSOR_ATTR_UPPER_THRESH, &val);
    
    /* 设置斜率阈值 */
    val.val1 = config->slope.val1;
    val.val2 = config->slope.val2;
    xy_sensor_attr_set(dev, channel, XY_SENSOR_ATTR_SLOPE_THRESH, &val);
    
    return 0;
}

/**
 * @brief 设置 FIFO 配置
 */
int xy_sensor_trigger_set_fifo(xy_sensor_device_t *dev,
                               const xy_sensor_fifo_config_t *config)
{
    if (!dev || !config) {
        return -1;
    }
    
    xy_sensor_value_t val;
    
    /* 设置 FIFO 水位 */
    val.val1 = config->watermark;
    xy_sensor_attr_set(dev, XY_SENSOR_CHAN_ALL, XY_SENSOR_ATTR_FIFO_WATERMARK, &val);
    
    /* 设置 FIFO 模式 */
    val.val1 = config->mode;
    xy_sensor_attr_set(dev, XY_SENSOR_CHAN_ALL, XY_SENSOR_ATTR_FIFO_MODE, &val);
    
    return 0;
}

/**
 * @brief 触发器中断处理 (内部使用)
 */
void xy_sensor_trigger_fire(xy_sensor_device_t *dev,
                            xy_sensor_trigger_type_t type)
{
    if (!dev) {
        return;
    }
    
    xy_log_d("Trigger fired: type=%d, sensor=%s\n", type, dev->name);
    
    /* 触发回调由驱动层处理 */
    /* 这里仅提供框架 */
}

/* ==================== 中断处理 ==================== */

/**
 * @brief 通用中断处理函数
 */
void xy_sensor_irq_handler(void *user_data)
{
    xy_sensor_device_t *dev = (xy_sensor_device_t *)user_data;
    
    if (!dev || !dev->initialized) {
        return;
    }
    
    /* 读取中断状态 */
    /* 根据具体传感器实现 */
    
    /* 触发回调 */
    xy_sensor_trigger_fire(dev, XY_SENSOR_TRIG_DATA_READY);
}

/* ==================== 阈值检测 ==================== */

/**
 * @brief 检查阈值
 */
bool xy_sensor_check_threshold(xy_sensor_device_t *dev,
                               xy_sensor_channel_t channel,
                               const xy_sensor_value_t *val)
{
    if (!dev || !val) {
        return false;
    }
    
    /* 获取阈值配置 */
    xy_sensor_value_t lower, upper;
    
    if (xy_sensor_attr_get(dev, channel, XY_SENSOR_ATTR_LOWER_THRESH, &lower) != XY_SENSOR_OK) {
        return false;
    }
    
    if (xy_sensor_attr_get(dev, channel, XY_SENSOR_ATTR_UPPER_THRESH, &upper) != XY_SENSOR_OK) {
        return false;
    }
    
    /* 比较 */
    if (val->val1 < lower.val1 || (val->val1 == lower.val1 && val->val2 < lower.val2)) {
        xy_log_d("Value below lower threshold\n");
        return true;
    }
    
    if (val->val1 > upper.val1 || (val->val1 == upper.val1 && val->val2 > upper.val2)) {
        xy_log_d("Value above upper threshold\n");
        return true;
    }
    
    return false;
}
