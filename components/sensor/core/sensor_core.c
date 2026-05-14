/**
 * @file sensor_core.c
 * @brief Sensor Core Implementation
 * @version 1.0.0
 * @date 2026-03-05
 */

#include "xy_sensor.h"
#include "xy_log.h"
#include <string.h>
#include <stdlib.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* ==================== 全局变量 ==================== */

static xy_sensor_device_t *g_sensor_list = NULL;    // 传感器链表
static uint8_t g_sensor_count = 0;                  // 传感器数量

/* ==================== 设备管理 ==================== */

/**
 * @brief 注册传感器设备
 */
int xy_sensor_device_register(xy_sensor_device_t *dev)
{
    if (!dev || !dev->name) {
        return -1;
    }
    
    /* 检查是否已注册 */
    xy_sensor_device_t *node = g_sensor_list;
    while (node) {
        if (strcmp(node->name, dev->name) == 0) {
            xy_log_w("Sensor already registered: %s\n", dev->name);
            return -1;
        }
        node = node->next;
    }
    
    /* 添加到链表头部 */
    dev->next = g_sensor_list;
    g_sensor_list = dev;
    g_sensor_count++;
    
    xy_log_i("Sensor registered: %s (type=%d, total=%d)\n", 
             dev->name, dev->type, g_sensor_count);
    
    return 0;
}

/**
 * @brief 注销传感器设备
 */
int xy_sensor_device_unregister(xy_sensor_device_t *dev)
{
    if (!dev) {
        return -1;
    }
    
    /* 查找设备 */
    xy_sensor_device_t *prev = NULL;
    xy_sensor_device_t *node = g_sensor_list;
    
    while (node) {
        if (node == dev) {
            if (prev) {
                prev->next = node->next;
            } else {
                g_sensor_list = node->next;
            }
            
            g_sensor_count--;
            xy_log_i("Sensor unregistered: %s\n", dev->name);
            return 0;
        }
        prev = node;
        node = node->next;
    }
    
    return -1;
}

/**
 * @brief 根据名称获取传感器设备
 */
xy_sensor_device_t* xy_sensor_device_get(const char *name)
{
    if (!name) {
        return NULL;
    }
    
    xy_sensor_device_t *node = g_sensor_list;
    while (node) {
        if (strcmp(node->name, name) == 0) {
            return node;
        }
        node = node->next;
    }
    
    return NULL;
}

/**
 * @brief 根据类型获取传感器设备
 */
xy_sensor_device_t* xy_sensor_device_get_by_type(xy_sensor_type_t type, uint8_t index)
{
    xy_sensor_device_t *node = g_sensor_list;
    uint8_t count = 0;
    
    while (node) {
        if (node->type == type) {
            if (count == index) {
                return node;
            }
            count++;
        }
        node = node->next;
    }
    
    return NULL;
}

/**
 * @brief 遍历所有传感器设备
 */
void xy_sensor_device_foreach(void (*callback)(xy_sensor_device_t *, void *),
                              void *user_data)
{
    if (!callback) {
        return;
    }
    
    xy_sensor_device_t *node = g_sensor_list;
    while (node) {
        callback(node, user_data);
        node = node->next;
    }
}

/**
 * @brief 获取传感器数量
 */
uint8_t xy_sensor_device_count(void)
{
    return g_sensor_count;
}

/* ==================== 核心 API ==================== */

/**
 * @brief 初始化传感器
 */
xy_sensor_status_t xy_sensor_init(xy_sensor_device_t *dev)
{
    if (!dev || !dev->api) {
        return XY_SENSOR_ERROR_INVALID_PARAM;
    }
    
    if (dev->initialized) {
        return XY_SENSOR_OK;
    }
    
    if (!dev->api->init) {
        return XY_SENSOR_ERROR_NOT_SUPPORTED;
    }
    
    int ret = dev->api->init(dev);
    if (ret == 0) {
        dev->initialized = true;
        dev->power_mode = XY_SENSOR_POWER_MODE_NORMAL;
        xy_log_i("Sensor initialized: %s\n", dev->name);
        return XY_SENSOR_OK;
    }
    
    return XY_SENSOR_ERROR;
}

/**
 * @brief 反初始化传感器
 */
xy_sensor_status_t xy_sensor_deinit(xy_sensor_device_t *dev)
{
    if (!dev || !dev->initialized) {
        return XY_SENSOR_ERROR_INVALID_PARAM;
    }
    
    dev->initialized = false;
    dev->power_mode = XY_SENSOR_POWER_MODE_OFF;
    
    xy_log_i("Sensor deinitialized: %s\n", dev->name);
    return XY_SENSOR_OK;
}

/**
 * @brief 获取最新采样数据
 */
xy_sensor_status_t xy_sensor_sample_fetch(xy_sensor_device_t *dev,
                                          xy_sensor_channel_t channel)
{
    if (!dev || !dev->initialized) {
        return XY_SENSOR_ERROR_NOT_INITIALIZED;
    }
    
    if (!dev->api->sample_fetch) {
        return XY_SENSOR_ERROR_NOT_SUPPORTED;
    }
    
    xy_sensor_status_t ret = dev->api->sample_fetch(dev, channel);
    
    if (ret == XY_SENSOR_OK) {
        dev->sample_count++;
        dev->last_sample_time = xy_os_tick_get();
    }
    
    return ret;
}

/**
 * @brief 读取通道数据
 */
xy_sensor_status_t xy_sensor_channel_get(xy_sensor_device_t *dev,
                                         xy_sensor_channel_t channel,
                                         xy_sensor_value_t *val)
{
    if (!dev || !dev->initialized || !val) {
        return XY_SENSOR_ERROR_INVALID_PARAM;
    }
    
    if (!dev->api->channel_get) {
        return XY_SENSOR_ERROR_NOT_SUPPORTED;
    }
    
    return dev->api->channel_get(dev, channel, val);
}

/**
 * @brief 设置传感器属性
 */
xy_sensor_status_t xy_sensor_attr_set(xy_sensor_device_t *dev,
                                      xy_sensor_channel_t channel,
                                      xy_sensor_attr_t attr,
                                      const xy_sensor_value_t *val)
{
    if (!dev || !dev->initialized) {
        return XY_SENSOR_ERROR_NOT_INITIALIZED;
    }
    
    if (!dev->api->attr_set) {
        return XY_SENSOR_ERROR_NOT_SUPPORTED;
    }
    
    return dev->api->attr_set(dev, channel, attr, val);
}

/**
 * @brief 获取传感器属性
 */
xy_sensor_status_t xy_sensor_attr_get(xy_sensor_device_t *dev,
                                      xy_sensor_channel_t channel,
                                      xy_sensor_attr_t attr,
                                      xy_sensor_value_t *val)
{
    if (!dev || !dev->initialized || !val) {
        return XY_SENSOR_ERROR_INVALID_PARAM;
    }
    
    if (!dev->api->attr_get) {
        return XY_SENSOR_ERROR_NOT_SUPPORTED;
    }
    
    return dev->api->attr_get(dev, channel, attr, val);
}

/**
 * @brief 设置触发器
 */
xy_sensor_status_t xy_sensor_trigger_set(xy_sensor_device_t *dev,
                                         const xy_sensor_trigger_t *trigger)
{
    if (!dev || !dev->initialized || !trigger) {
        return XY_SENSOR_ERROR_INVALID_PARAM;
    }
    
    if (!dev->api->trigger_set) {
        return XY_SENSOR_ERROR_NOT_SUPPORTED;
    }
    
    return dev->api->trigger_set(dev, trigger);
}

/**
 * @brief 禁用触发器
 */
xy_sensor_status_t xy_sensor_trigger_unset(xy_sensor_device_t *dev,
                                           xy_sensor_trigger_type_t type)
{
    if (!dev || !dev->initialized) {
        return XY_SENSOR_ERROR_NOT_INITIALIZED;
    }
    
    /* 简化实现：设置 NULL 触发器禁用 */
    xy_sensor_trigger_t trigger = {
        .type = type,
        .trigger_handler = NULL,
    };
    
    return xy_sensor_trigger_set(dev, &trigger);
}

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

/* ==================== 工具函数 ==================== */

/**
 * @brief 打印传感器信息
 */
void xy_sensor_print_info(xy_sensor_device_t *dev)
{
    if (!dev) {
        return;
    }
    
    xy_log_i("=== Sensor Info ===\n");
    xy_log_i("Name: %s\n", dev->name);
    xy_log_i("Type: %d\n", dev->type);
    xy_log_i("Initialized: %s\n", dev->initialized ? "Yes" : "No");
    xy_log_i("Power Mode: %d\n", dev->power_mode);
    xy_log_i("Sample Count: %lu\n", dev->sample_count);
    xy_log_i("=================\n");
}

/**
 * @brief 打印所有传感器信息
 */
void xy_sensor_print_all(void)
{
    xy_log_i("Total sensors: %d\n", g_sensor_count);
    
    xy_sensor_device_foreach(xy_sensor_print_info, NULL);
}
