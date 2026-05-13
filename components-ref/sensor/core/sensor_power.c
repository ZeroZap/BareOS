/**
 * @file sensor_power.c
 * @brief Sensor Power Management
 * @version 1.0.0
 * @date 2026-03-05
 */

#include "xy_sensor.h"
#include "xy_log.h"

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* ==================== 电源管理 ==================== */

/**
 * @brief 设置电源模式
 */
xy_sensor_status_t xy_sensor_set_power_mode(xy_sensor_device_t *dev,
                                            xy_sensor_power_mode_t mode)
{
    if (!dev || !dev->initialized) {
        return XY_SENSOR_ERROR_NOT_INITIALIZED;
    }
    
    xy_sensor_value_t val = {0};
    val.val1 = mode;
    
    xy_sensor_status_t ret = xy_sensor_attr_set(dev,
                                                XY_SENSOR_CHAN_ALL,
                                                XY_SENSOR_ATTR_POWER_MODE,
                                                &val);
    
    if (ret == XY_SENSOR_OK) {
        dev->power_mode = mode;
        xy_log_d("Sensor %s power mode: %d\n", dev->name, mode);
    }
    
    return ret;
}

/**
 * @brief 获取电源模式
 */
xy_sensor_power_mode_t xy_sensor_get_power_mode(xy_sensor_device_t *dev)
{
    if (!dev) {
        return XY_SENSOR_POWER_MODE_OFF;
    }
    return dev->power_mode;
}

/**
 * @brief 进入睡眠模式
 */
xy_sensor_status_t xy_sensor_sleep(xy_sensor_device_t *dev)
{
    return xy_sensor_set_power_mode(dev, XY_SENSOR_POWER_MODE_SLEEP);
}

/**
 * @brief 唤醒传感器
 */
xy_sensor_status_t xy_sensor_wakeup(xy_sensor_device_t *dev)
{
    return xy_sensor_set_power_mode(dev, XY_SENSOR_POWER_MODE_NORMAL);
}

/**
 * @brief 进入低功耗模式
 */
xy_sensor_status_t xy_sensor_low_power(xy_sensor_device_t *dev)
{
    return xy_sensor_set_power_mode(dev, XY_SENSOR_POWER_MODE_LOW_POWER);
}

/**
 * @brief 关闭传感器
 */
xy_sensor_status_t xy_sensor_power_off(xy_sensor_device_t *dev)
{
    return xy_sensor_set_power_mode(dev, XY_SENSOR_POWER_MODE_OFF);
}

/* ==================== 自动电源管理 ==================== */

/**
 * @brief 自动睡眠 (空闲超时)
 */
void xy_sensor_auto_sleep(xy_sensor_device_t *dev, uint32_t idle_timeout_ms)
{
    if (!dev || !dev->initialized) {
        return;
    }
    
    uint32_t current_time = xy_os_tick_get();
    uint32_t idle_time = current_time - dev->last_sample_time;
    
    if (idle_time > idle_timeout_ms) {
        xy_sensor_sleep(dev);
        xy_log_i("Sensor %s auto-sleep (idle=%lu ms)\n", dev->name, idle_time);
    }
}

/**
 * @brief 唤醒_on_运动
 */
void xy_sensor_wakeup_on_motion(xy_sensor_device_t *dev)
{
    if (!dev) {
        return;
    }
    
    xy_sensor_trigger_t trigger = {
        .type = XY_SENSOR_TRIG_MOTION,
        .channel = XY_SENSOR_CHAN_ACCEL_XYZ,
    };
    
    xy_sensor_trigger_set(dev, &trigger);
}
