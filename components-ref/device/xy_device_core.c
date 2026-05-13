/**
 * @file xy_device_core.c
 * @brief Device Framework Core - 设备注册表与电源管理
 * @version 1.0.0
 * @date 2026-03-12
 * 
 * 实现功能:
 * - 静态设备注册表 (编译时设备定义)
 * - 设备名称/类型查找
 * - 设备电源管理 (休眠/唤醒)
 * - 设备引用计数
 */

#include "xy_device_core.h"
#include <string.h>
#include <stdio.h>

/* ==================== 全局设备注册表 ==================== */

/**
 * @brief 全局设备注册表 (静态数组)
 * 
 * 支持最多 XY_DEVICE_REGISTRY_MAX 个设备
 * 设备在编译时静态定义，运行时自动初始化
 */
static xy_device_registry_entry_t g_device_registry[XY_DEVICE_REGISTRY_MAX];
static size_t g_device_count = 0;
static bool g_registry_initialized = false;

/**
 * @brief 设备电源状态
 */
typedef struct {
    xy_device_pm_state_t state;
    uint32_t last_active_time;
    uint32_t sleep_timeout_ms;
    xy_device_pm_callback_t callback;
    void *user_data;
} xy_device_pm_info_t;

static xy_device_pm_info_t g_pm_info[XY_DEVICE_REGISTRY_MAX];

/* ==================== 注册表初始化 ==================== */

/**
 * @brief 初始化设备注册表
 */
int xy_device_registry_init(void)
{
    if (g_registry_initialized) {
        return XY_DEVICE_OK;
    }

    memset(g_device_registry, 0, sizeof(g_device_registry));
    memset(g_pm_info, 0, sizeof(g_pm_info));
    g_device_count = 0;
    g_registry_initialized = true;

    return XY_DEVICE_OK;
}

/**
 * @brief 注册设备到全局注册表
 */
int xy_device_registry_register(xy_device_t *dev)
{
    if (!g_registry_initialized) {
        xy_device_registry_init();
    }

    if (!dev || !dev->name) {
        return XY_DEVICE_INVALID_PARAM;
    }

    if (g_device_count >= XY_DEVICE_REGISTRY_MAX) {
        return XY_DEVICE_NO_MEM;
    }

    /* 检查是否已注册 */
    for (size_t i = 0; i < g_device_count; i++) {
        if (strcmp(g_device_registry[i].device->name, dev->name) == 0) {
            return XY_DEVICE_ERROR; /* 设备名冲突 */
        }
    }

    /* 注册新设备 */
    size_t idx = g_device_count++;
    g_device_registry[idx].device = dev;
    g_device_registry[idx].ref_count = 0;
    g_device_registry[idx].flags = 0;

    /* 初始化电源管理信息 */
    g_pm_info[idx].state = XY_DEVICE_PM_ACTIVE;
    g_pm_info[idx].last_active_time = 0;
    g_pm_info[idx].sleep_timeout_ms = XY_DEVICE_DEFAULT_SLEEP_TIMEOUT;
    g_pm_info[idx].callback = NULL;
    g_pm_info[idx].user_data = NULL;

    return XY_DEVICE_OK;
}

/**
 * @brief 从全局注册表注销设备
 */
int xy_device_registry_unregister(xy_device_t *dev)
{
    if (!g_registry_initialized || !dev) {
        return XY_DEVICE_INVALID_PARAM;
    }

    for (size_t i = 0; i < g_device_count; i++) {
        if (g_device_registry[i].device == dev) {
            /* 检查引用计数 */
            if (g_device_registry[i].ref_count > 0) {
                return XY_DEVICE_BUSY;
            }

            /* 移除设备，后续设备前移 */
            for (size_t j = i; j < g_device_count - 1; j++) {
                g_device_registry[j] = g_device_registry[j + 1];
                g_pm_info[j] = g_pm_info[j + 1];
            }

            memset(&g_device_registry[g_device_count - 1], 0, 
                   sizeof(xy_device_registry_entry_t));
            memset(&g_pm_info[g_device_count - 1], 0, sizeof(xy_device_pm_info_t));
            g_device_count--;

            return XY_DEVICE_OK;
        }
    }

    return XY_DEVICE_NOT_FOUND;
}

/**
 * @brief 按名称查找设备
 */
xy_device_t *xy_device_find_by_name(const char *name)
{
    if (!g_registry_initialized || !name) {
        return NULL;
    }

    for (size_t i = 0; i < g_device_count; i++) {
        xy_device_t *dev = g_device_registry[i].device;
        if (dev && dev->name && strcmp(dev->name, name) == 0) {
            return dev;
        }
    }

    return NULL;
}

/**
 * @brief 按类型查找设备
 * 
 * @param type 设备类型
 * @param index 索引 (同类型第几个设备，从 0 开始)
 * @return xy_device_t* 设备指针，未找到返回 NULL
 */
xy_device_t *xy_device_find_by_type(xy_device_type_t type, size_t index)
{
    if (!g_registry_initialized) {
        return NULL;
    }

    size_t count = 0;
    for (size_t i = 0; i < g_device_count; i++) {
        xy_device_t *dev = g_device_registry[i].device;
        if (dev && dev->type == type) {
            if (count == index) {
                return dev;
            }
            count++;
        }
    }

    return NULL;
}

/**
 * @brief 遍历所有设备
 */
int xy_device_foreach(xy_device_callback_t callback, void *arg)
{
    if (!g_registry_initialized || !callback) {
        return XY_DEVICE_INVALID_PARAM;
    }

    int result = XY_DEVICE_OK;
    for (size_t i = 0; i < g_device_count; i++) {
        xy_device_t *dev = g_device_registry[i].device;
        if (dev) {
            result = callback(dev, arg);
            if (result < 0) {
                break;
            }
        }
    }

    return result;
}

/**
 * @brief 获取设备数量
 */
size_t xy_device_get_count(void)
{
    return g_device_count;
}

/**
 * @brief 获取注册表信息
 */
const xy_device_registry_entry_t *xy_device_get_registry(size_t *count)
{
    if (count) {
        *count = g_device_count;
    }
    return g_device_registry;
}

/* ==================== 设备引用计数 ==================== */

/**
 * @brief 增加设备引用计数
 */
int xy_device_acquire(xy_device_t *dev)
{
    if (!g_registry_initialized || !dev) {
        return XY_DEVICE_INVALID_PARAM;
    }

    for (size_t i = 0; i < g_device_count; i++) {
        if (g_device_registry[i].device == dev) {
            g_device_registry[i].ref_count++;
            /* 唤醒设备 */
            if (g_pm_info[i].state == XY_DEVICE_PM_SLEEP_STATE) {
                xy_device_wake(dev);
            }
            g_pm_info[i].last_active_time = xy_device_get_tick();
            return XY_DEVICE_OK;
        }
    }

    return XY_DEVICE_NOT_FOUND;
}

/**
 * @brief 减少设备引用计数
 */
int xy_device_release(xy_device_t *dev)
{
    if (!g_registry_initialized || !dev) {
        return XY_DEVICE_INVALID_PARAM;
    }

    for (size_t i = 0; i < g_device_count; i++) {
        if (g_device_registry[i].device == dev) {
            if (g_device_registry[i].ref_count > 0) {
                g_device_registry[i].ref_count--;
            }
            g_pm_info[i].last_active_time = xy_device_get_tick();
            return XY_DEVICE_OK;
        }
    }

    return XY_DEVICE_NOT_FOUND;
}

/**
 * @brief 获取设备引用计数
 */
int xy_device_get_ref_count(xy_device_t *dev)
{
    if (!g_registry_initialized || !dev) {
        return -1;
    }

    for (size_t i = 0; i < g_device_count; i++) {
        if (g_device_registry[i].device == dev) {
            return (int)g_device_registry[i].ref_count;
        }
    }

    return -1;
}

/* ==================== 设备电源管理 ==================== */

/**
 * @brief 设置设备电源管理回调
 */
int xy_device_set_pm_callback(xy_device_t *dev, 
                              xy_device_pm_callback_t callback,
                              void *user_data)
{
    if (!g_registry_initialized || !dev) {
        return XY_DEVICE_INVALID_PARAM;
    }

    for (size_t i = 0; i < g_device_count; i++) {
        if (g_device_registry[i].device == dev) {
            g_pm_info[i].callback = callback;
            g_pm_info[i].user_data = user_data;
            return XY_DEVICE_OK;
        }
    }

    return XY_DEVICE_NOT_FOUND;
}

/**
 * @brief 设置设备休眠超时
 */
int xy_device_set_sleep_timeout(xy_device_t *dev, uint32_t timeout_ms)
{
    if (!g_registry_initialized || !dev) {
        return XY_DEVICE_INVALID_PARAM;
    }

    for (size_t i = 0; i < g_device_count; i++) {
        if (g_device_registry[i].device == dev) {
            g_pm_info[i].sleep_timeout_ms = timeout_ms;
            return XY_DEVICE_OK;
        }
    }

    return XY_DEVICE_NOT_FOUND;
}

/**
 * @brief 获取设备电源状态
 */
xy_device_pm_state_t xy_device_get_pm_state(xy_device_t *dev)
{
    if (!g_registry_initialized || !dev) {
        return XY_DEVICE_PM_UNKNOWN;
    }

    for (size_t i = 0; i < g_device_count; i++) {
        if (g_device_registry[i].device == dev) {
            return g_pm_info[i].state;
        }
    }

    return XY_DEVICE_PM_UNKNOWN;
}

/**
 * @brief 设备进入休眠
 */
int xy_device_sleep(xy_device_t *dev)
{
    if (!g_registry_initialized || !dev) {
        return XY_DEVICE_INVALID_PARAM;
    }

    for (size_t i = 0; i < g_device_count; i++) {
        if (g_device_registry[i].device == dev) {
            if (g_pm_info[i].state == XY_DEVICE_PM_SLEEP_STATE) {
                return XY_DEVICE_OK; /* 已在休眠 */
            }

            /* 检查引用计数 */
            if (g_device_registry[i].ref_count > 0) {
                return XY_DEVICE_BUSY; /* 有引用，不能休眠 */
            }

            /* 调用回调 */
            if (g_pm_info[i].callback) {
                int ret = g_pm_info[i].callback(dev, XY_DEVICE_PM_SLEEP_EVENT, 
                                                g_pm_info[i].user_data);
                if (ret < 0) {
                    return ret;
                }
            }

            g_pm_info[i].state = XY_DEVICE_PM_SLEEP_STATE;
            return XY_DEVICE_OK;
        }
    }

    return XY_DEVICE_NOT_FOUND;
}

/**
 * @brief 设备唤醒
 */
int xy_device_wake(xy_device_t *dev)
{
    if (!g_registry_initialized || !dev) {
        return XY_DEVICE_INVALID_PARAM;
    }

    for (size_t i = 0; i < g_device_count; i++) {
        if (g_device_registry[i].device == dev) {
            if (g_pm_info[i].state == XY_DEVICE_PM_ACTIVE) {
                return XY_DEVICE_OK; /* 已在活动状态 */
            }

            /* 调用回调 */
            if (g_pm_info[i].callback) {
                int ret = g_pm_info[i].callback(dev, XY_DEVICE_PM_WAKE, 
                                                g_pm_info[i].user_data);
                if (ret < 0) {
                    return ret;
                }
            }

            g_pm_info[i].state = XY_DEVICE_PM_ACTIVE;
            g_pm_info[i].last_active_time = xy_device_get_tick();
            return XY_DEVICE_OK;
        }
    }

    return XY_DEVICE_NOT_FOUND;
}

/**
 * @brief 自动电源管理检查
 * 
 * 应在系统心跳或空闲任务中定期调用
 * 检查超时设备并自动进入休眠
 */
void xy_device_pm_check(void)
{
    if (!g_registry_initialized) {
        return;
    }

    uint32_t current_tick = xy_device_get_tick();

    for (size_t i = 0; i < g_device_count; i++) {
        xy_device_t *dev = g_device_registry[i].device;
        if (!dev || !dev->initialized) {
            continue;
        }

        /* 跳过已有引用的设备 */
        if (g_device_registry[i].ref_count > 0) {
            continue;
        }

        /* 检查超时 */
        uint32_t idle_time = current_tick - g_pm_info[i].last_active_time;
        if (idle_time >= g_pm_info[i].sleep_timeout_ms &&
            g_pm_info[i].state == XY_DEVICE_PM_ACTIVE) {
            xy_device_sleep(dev);
        }
    }
}

/**
 * @brief 获取系统 tick (弱实现，用户可重写)
 */
__attribute__((weak))
uint32_t xy_device_get_tick(void)
{
    /* 默认返回 0，用户应提供实际实现 */
    return 0;
}

/* ==================== 设备统计信息 ==================== */

/**
 * @brief 获取设备统计信息
 */
int xy_device_get_stats(xy_device_stats_t *stats)
{
    if (!g_registry_initialized || !stats) {
        return XY_DEVICE_INVALID_PARAM;
    }

    memset(stats, 0, sizeof(*stats));
    stats->total_devices = g_device_count;

    for (size_t i = 0; i < g_device_count; i++) {
        xy_device_t *dev = g_device_registry[i].device;
        if (!dev) continue;

        switch (dev->type) {
            case XY_DEVICE_TYPE_I2C:    stats->i2c_count++; break;
            case XY_DEVICE_TYPE_SPI:    stats->spi_count++; break;
            case XY_DEVICE_TYPE_UART:   stats->uart_count++; break;
            case XY_DEVICE_TYPE_GPIO:   stats->gpio_count++; break;
            case XY_DEVICE_TYPE_SENSOR: stats->sensor_count++; break;
            case XY_DEVICE_TYPE_DISPLAY: stats->display_count++; break;
            case XY_DEVICE_TYPE_MEMORY: stats->memory_count++; break;
            default: stats->other_count++; break;
        }

        if (g_pm_info[i].state == XY_DEVICE_PM_SLEEP_STATE) {
            stats->sleep_count++;
        }
    }

    return XY_DEVICE_OK;
}

/**
 * @brief 打印设备列表 (调试用)
 */
void xy_device_print_list(void)
{
    if (!g_registry_initialized) {
        printf("Device registry not initialized\r\n");
        return;
    }

    printf("\r\n=== Device Registry (%zu devices) ===\r\n", g_device_count);
    printf("%-20s %-12s %-8s %-8s %s\r\n", 
           "Name", "Type", "RefCnt", "PM State", "Initialized");
    printf("----------------------------------------------\r\n");

    for (size_t i = 0; i < g_device_count; i++) {
        xy_device_t *dev = g_device_registry[i].device;
        if (!dev) continue;

        const char *type_str = "UNKNOWN";
        switch (dev->type) {
            case XY_DEVICE_TYPE_I2C:    type_str = "I2C"; break;
            case XY_DEVICE_TYPE_SPI:    type_str = "SPI"; break;
            case XY_DEVICE_TYPE_UART:   type_str = "UART"; break;
            case XY_DEVICE_TYPE_GPIO:   type_str = "GPIO"; break;
            case XY_DEVICE_TYPE_SENSOR: type_str = "SENSOR"; break;
            case XY_DEVICE_TYPE_DISPLAY: type_str = "DISPLAY"; break;
            case XY_DEVICE_TYPE_MEMORY: type_str = "MEMORY"; break;
            default: break;
        }

        const char *pm_str = "UNKNOWN";
        switch (g_pm_info[i].state) {
            case XY_DEVICE_PM_ACTIVE: pm_str = "ACTIVE"; break;
            case XY_DEVICE_PM_SLEEP_STATE:  pm_str = "SLEEP"; break;
            default: break;
        }

        printf("%-20s %-12s %-8d %-8s %s\r\n",
               dev->name ? dev->name : "(null)",
               type_str,
               (int)g_device_registry[i].ref_count,
               pm_str,
               dev->initialized ? "YES" : "NO");
    }
    printf("----------------------------------------------\r\n\r\n");
}
