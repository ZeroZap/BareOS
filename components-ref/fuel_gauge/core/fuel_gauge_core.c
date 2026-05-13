/**
 * @file fuel_gauge_core.c
 * @brief Fuel Gauge Core Implementation
 * @version 1.0.0
 * @date 2026-03-05
 */

#include "xy_fuel_gauge.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* 全局变量 */
static xy_fuel_gauge_t *g_fg_list = NULL;
static uint8_t g_fg_count = 0;

/**
 * @brief 注册电量计设备
 */
xy_fuel_gauge_status_t xy_fuel_gauge_device_register(xy_fuel_gauge_t *fg)
{
    if (!fg || !fg->name) {
        return XY_FG_ERROR_INVALID_PARAM;
    }
    
    /* 检查是否已注册 */
    xy_fuel_gauge_t *node = g_fg_list;
    while (node) {
        if (strcmp(node->name, fg->name) == 0) {
            xy_log_w("Fuel gauge already registered: %s\n", fg->name);
            return XY_FG_ERROR;
        }
        node = node->next;
    }
    
    /* 添加到链表 */
    fg->next = g_fg_list;
    g_fg_list = fg;
    g_fg_count++;
    
    xy_log_i("Fuel gauge registered: %s (total=%d)\n", fg->name, g_fg_count);
    return XY_FG_OK;
}

/**
 * @brief 初始化电量计
 */
xy_fuel_gauge_status_t xy_fuel_gauge_init(xy_fuel_gauge_t *fg)
{
    if (!fg || !fg->api) {
        return XY_FG_ERROR_INVALID_PARAM;
    }
    
    if (fg->initialized) {
        return XY_FG_OK;
    }
    
    if (!fg->api->init) {
        return XY_FG_ERROR_NOT_SUPPORTED;
    }
    
    int ret = fg->api->init(fg);
    if (ret == 0) {
        fg->initialized = true;
        xy_log_i("Fuel gauge initialized: %s\n", fg->name);
        return XY_FG_OK;
    }
    
    return XY_FG_ERROR;
}

/**
 * @brief 反初始化电量计
 */
xy_fuel_gauge_status_t xy_fuel_gauge_deinit(xy_fuel_gauge_t *fg)
{
    if (!fg || !fg->initialized) {
        return XY_FG_ERROR_INVALID_PARAM;
    }
    
    fg->initialized = false;
    xy_log_i("Fuel gauge deinitialized: %s\n", fg->name);
    return XY_FG_OK;
}

/**
 * @brief 获取最新数据
 */
xy_fuel_gauge_status_t xy_fuel_gauge_fetch(xy_fuel_gauge_t *fg)
{
    if (!fg || !fg->initialized) {
        return XY_FG_ERROR_NOT_INITIALIZED;
    }
    
    if (!fg->api->fetch) {
        return XY_FG_ERROR_NOT_SUPPORTED;
    }
    
    xy_fuel_gauge_status_t ret = fg->api->fetch(fg);
    
    if (ret == XY_FG_OK) {
        fg->latest.timestamp = xy_os_tick_get();
    }
    
    return ret;
}

/**
 * @brief 读取指定数据
 */
xy_fuel_gauge_status_t xy_fuel_gauge_get(xy_fuel_gauge_t *fg,
                                         xy_fuel_gauge_data_type_t type,
                                         int32_t *val)
{
    if (!fg || !fg->initialized || !val) {
        return XY_FG_ERROR_INVALID_PARAM;
    }
    
    /* 先获取最新数据 */
    xy_fuel_gauge_status_t ret = xy_fuel_gauge_fetch(fg);
    if (ret != XY_FG_OK) {
        return ret;
    }
    
    /* 如果有 channel_get API，使用它 */
    if (fg->api->channel_get) {
        return fg->api->channel_get(fg, type, val);
    }
    
    /* 否则从最新数据中读取 */
    switch (type) {
        case XY_FG_DATA_VOLTAGE:
            *val = fg->latest.voltage_mv;
            break;
        case XY_FG_DATA_CURRENT:
            *val = fg->latest.current_ma;
            break;
        case XY_FG_DATA_SOC:
            *val = fg->latest.soc;
            break;
        case XY_FG_DATA_SOH:
            *val = fg->latest.soh;
            break;
        case XY_FG_DATA_TEMPERATURE:
            *val = fg->latest.temperature_c;
            break;
        case XY_FG_DATA_CYCLE_COUNT:
            *val = fg->latest.cycle_count;
            break;
        case XY_FG_DATA_FULL_CAPACITY:
            *val = fg->latest.full_capacity_mah;
            break;
        case XY_FG_DATA_REMAIN_CAPACITY:
            *val = fg->latest.remain_capacity_mah;
            break;
        default:
            return XY_FG_ERROR_NOT_SUPPORTED;
    }
    
    return XY_FG_OK;
}

/**
 * @brief 设置告警阈值
 */
xy_fuel_gauge_status_t xy_fuel_gauge_set_alert(xy_fuel_gauge_t *fg,
                                               const xy_fuel_gauge_alert_t *alert)
{
    if (!fg || !fg->initialized || !alert) {
        return XY_FG_ERROR_INVALID_PARAM;
    }
    
    if (!fg->api->alert_set) {
        return XY_FG_ERROR_NOT_SUPPORTED;
    }
    
    return fg->api->alert_set(fg, alert);
}

/**
 * @brief 获取告警状态
 */
xy_fuel_gauge_status_t xy_fuel_gauge_get_alert(xy_fuel_gauge_t *fg,
                                               xy_fuel_gauge_alert_t *alert)
{
    if (!fg || !fg->initialized || !alert) {
        return XY_FG_ERROR_INVALID_PARAM;
    }
    
    if (!fg->api->alert_get) {
        return XY_FG_ERROR_NOT_SUPPORTED;
    }
    
    return fg->api->alert_get(fg, alert);
}

/**
 * @brief 根据名称获取电量计设备
 */
xy_fuel_gauge_t* xy_fuel_gauge_device_get(const char *name)
{
    if (!name) {
        return NULL;
    }
    
    xy_fuel_gauge_t *node = g_fg_list;
    while (node) {
        if (strcmp(node->name, name) == 0) {
            return node;
        }
        node = node->next;
    }
    
    return NULL;
}

/**
 * @brief 遍历所有电量计设备
 */
void xy_fuel_gauge_device_foreach(void (*callback)(xy_fuel_gauge_t *, void *),
                                  void *user_data)
{
    if (!callback) {
        return;
    }
    
    xy_fuel_gauge_t *node = g_fg_list;
    while (node) {
        callback(node, user_data);
        node = node->next;
    }
}

/**
 * @brief 获取电量计设备数量
 */
uint8_t xy_fuel_gauge_device_count(void)
{
    return g_fg_count;
}

/**
 * @brief 打印电量计信息
 */
void xy_fuel_gauge_print_info(xy_fuel_gauge_t *fg)
{
    if (!fg) {
        return;
    }
    
    xy_log_i("=== Fuel Gauge Info ===\n");
    xy_log_i("Name: %s\n", fg->name);
    xy_log_i("Initialized: %s\n", fg->initialized ? "Yes" : "No");
    xy_log_i("Voltage: %d mV\n", fg->latest.voltage_mv);
    xy_log_i("Current: %d mA\n", fg->latest.current_ma);
    xy_log_i("SOC: %d %%\n", fg->latest.soc);
    xy_log_i("SOH: %d %%\n", fg->latest.soh);
    xy_log_i("Temperature: %.1f °C\n", fg->latest.temperature_c / 10.0f);
    xy_log_i("=======================\n");
}

/**
 * @brief 打印所有电量计信息
 */
void xy_fuel_gauge_print_all(void)
{
    xy_log_i("Total fuel gauges: %d\n", g_fg_count);
    xy_fuel_gauge_device_foreach(xy_fuel_gauge_print_info, NULL);
}
