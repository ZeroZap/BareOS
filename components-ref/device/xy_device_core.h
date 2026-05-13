/**
 * @file xy_device_core.h
 * @brief Device Framework Core - Enhanced Device Management
 * @version 1.0.0
 * @date 2026-03-12
 */

#ifndef XY_DEVICE_CORE_H
#define XY_DEVICE_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_device.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/**
 * @brief 设备注册表最大容量
 */
#ifndef XY_DEVICE_REGISTRY_MAX
#define XY_DEVICE_REGISTRY_MAX  32
#endif

/**
 * @brief 默认休眠超时 (ms)
 */
#ifndef XY_DEVICE_DEFAULT_SLEEP_TIMEOUT
#define XY_DEVICE_DEFAULT_SLEEP_TIMEOUT  10000
#endif

/**
 * @brief 设备电源管理状态
 */
typedef enum {
    XY_DEVICE_PM_UNKNOWN = 0,
    XY_DEVICE_PM_ACTIVE,
    XY_DEVICE_PM_SLEEP_STATE,
    XY_DEVICE_PM_OFF,
} xy_device_pm_state_t;

/**
 * @brief 设备电源管理事件
 */
typedef enum {
    XY_DEVICE_PM_SLEEP_EVENT = 0,
    XY_DEVICE_PM_WAKE,
} xy_device_pm_event_t;

/**
 * @brief 设备电源管理回调
 */
typedef int (*xy_device_pm_callback_t)(xy_device_t *dev, 
                                       xy_device_pm_event_t event,
                                       void *user_data);

/**
 * @brief 设备注册表条目
 */
typedef struct {
    xy_device_t *device;              /**< 设备指针 */
    uint32_t ref_count;               /**< 引用计数 */
    uint32_t flags;                   /**< 标志位 */
} xy_device_registry_entry_t;

/**
 * @brief 设备统计信息
 */
typedef struct {
    size_t total_devices;
    size_t i2c_count;
    size_t spi_count;
    size_t uart_count;
    size_t gpio_count;
    size_t sensor_count;
    size_t display_count;
    size_t memory_count;
    size_t other_count;
    size_t sleep_count;
} xy_device_stats_t;

/**
 * @brief 设备回调函数类型
 */
typedef int (*xy_device_callback_t)(xy_device_t *dev, void *arg);

/* ==================== 注册表管理 ==================== */

/**
 * @brief 初始化设备注册表
 */
int xy_device_registry_init(void);

/**
 * @brief 注册设备到全局注册表
 */
int xy_device_registry_register(xy_device_t *dev);

/**
 * @brief 从全局注册表注销设备
 */
int xy_device_registry_unregister(xy_device_t *dev);

/**
 * @brief 按名称查找设备
 */
xy_device_t *xy_device_find_by_name(const char *name);

/**
 * @brief 按类型查找设备
 */
xy_device_t *xy_device_find_by_type(xy_device_type_t type, size_t index);

/**
 * @brief 遍历所有设备
 */
int xy_device_foreach(xy_device_callback_t callback, void *arg);

/**
 * @brief 获取设备数量
 */
size_t xy_device_get_count(void);

/**
 * @brief 获取注册表信息
 */
const xy_device_registry_entry_t *xy_device_get_registry(size_t *count);

/* ==================== 引用计数 ==================== */

/**
 * @brief 增加设备引用计数
 */
int xy_device_acquire(xy_device_t *dev);

/**
 * @brief 减少设备引用计数
 */
int xy_device_release(xy_device_t *dev);

/**
 * @brief 获取设备引用计数
 */
int xy_device_get_ref_count(xy_device_t *dev);

/* ==================== 电源管理 ==================== */

/**
 * @brief 设置设备电源管理回调
 */
int xy_device_set_pm_callback(xy_device_t *dev, 
                              xy_device_pm_callback_t callback,
                              void *user_data);

/**
 * @brief 设置设备休眠超时
 */
int xy_device_set_sleep_timeout(xy_device_t *dev, uint32_t timeout_ms);

/**
 * @brief 获取设备电源状态
 */
xy_device_pm_state_t xy_device_get_pm_state(xy_device_t *dev);

/**
 * @brief 设备进入休眠
 */
int xy_device_sleep(xy_device_t *dev);

/**
 * @brief 设备唤醒
 */
int xy_device_wake(xy_device_t *dev);

/**
 * @brief 自动电源管理检查 (心跳调用)
 */
void xy_device_pm_check(void);

/**
 * @brief 获取系统 tick (弱实现，用户可重写)
 */
uint32_t xy_device_get_tick(void);

/* ==================== 统计与调试 ==================== */

/**
 * @brief 获取设备统计信息
 */
int xy_device_get_stats(xy_device_stats_t *stats);

/**
 * @brief 打印设备列表 (调试用)
 */
void xy_device_print_list(void);

#ifdef __cplusplus
}
#endif

#endif /* XY_DEVICE_CORE_H */
