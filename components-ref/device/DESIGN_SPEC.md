# XinYi 设备组件架构设计

## 1. 架构概述

XinYi 设备组件架构结合了 RT-Thread 的统一设备模型和 Zephyr 的结构化 API 设计，为嵌入式系统提供灵活、高效的设备管理框架。

### 1.1 设计原则

- **统一接口**: 所有设备使用统一的注册、查找、操作接口
- **模块化**: 按功能分类组织设备驱动
- **可扩展**: 支持新设备类型和驱动的动态添加
- **可配置**: 通过 Kconfig 进行编译时裁剪
- **兼容性**: 与现有 HAL 框架无缝集成

### 1.2 架构层次

```
┌─────────────────────────────────────────┐
│           应用层 (Applications)          │
├─────────────────────────────────────────┤
│         HAL 层 (xy_hal_*)               │
│      (统一接口，平台无关)                │
├─────────────────────────────────────────┤
│        Device 层 (xy_dev_*)             │
│   (设备驱动，平台相关)                   │
├─────────────────────────────────────────┤
│     MCU HAL 层 (STM32 HAL, etc.)        │
│    (底层硬件操作)                       │
└─────────────────────────────────────────┘
```

## 2. 设备框架设计

### 2.1 核心头文件 (xy_device.h)

```c
/**
 * @file xy_device.h
 * @brief XinYi Device Framework
 * @version 2.0
 * @date 2026-02-28
 */

#ifndef XY_DEVICE_H
#define XY_DEVICE_H

#include "xy_hal.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 设备类型定义 */
typedef enum {
    XY_DEV_TYPE_ADC = 0,        /**< ADC 设备 */
    XY_DEV_TYPE_DAC,            /**< DAC 设备 */
    XY_DEV_TYPE_UART,           /**< UART 设备 */
    XY_DEV_TYPE_SPI,            /**< SPI 设备 */
    XY_DEV_TYPE_I2C,            /**< I2C 设备 */
    XY_DEV_TYPE_GPIO,           /**< GPIO 设备 */
    XY_DEV_TYPE_PWM,            /**< PWM 设备 */
    XY_DEV_TYPE_TIMER,          /**< 定时器设备 */
    XY_DEV_TYPE_RTC,            /**< RTC 设备 */
    XY_DEV_TYPE_WDG,            /**< 看门狗设备 */
    XY_DEV_TYPE_FLASH,          /**< Flash 设备 */
    XY_DEV_TYPE_SENSOR,         /**< 传感器设备 */
    XY_DEV_TYPE_STORAGE,        /**< 存储设备 */
    XY_DEV_TYPE_BUS,            /**< 总线设备 */
    XY_DEV_TYPE_MISC,           /**< 杂项设备 */
    XY_DEV_TYPE_MAX
} xy_dev_type_t;

/* 设备标志 */
typedef enum {
    XY_DEV_FLAG_RDWR      = 0x0001, /**< 可读写 */
    XY_DEV_FLAG_RDONLY    = 0x0002, /**< 只读 */
    XY_DEV_FLAG_WRONLY    = 0x0004, /**< 只写 */
    XY_DEV_FLAG_STREAM    = 0x0008, /**< 流设备 */
    XY_DEV_FLAG_BLOCK     = 0x0010, /**< 块设备 */
    XY_DEV_FLAG_INT       = 0x0020, /**< 支持中断 */
    XY_DEV_FLAG_DMA       = 0x0040, /**< 支持 DMA */
    XY_DEV_FLAG_ASYNC     = 0x0080, /**< 支持异步操作 */
    XY_DEV_FLAG_POLL      = 0x0100, /**< 支持轮询 */
    XY_DEV_FLAG_EVENT     = 0x0200, /**< 支持事件 */
} xy_dev_flag_t;

/* 设备状态 */
typedef enum {
    XY_DEV_STATE_CLOSED = 0,    /**< 设备关闭 */
    XY_DEV_STATE_OPENED,        /**< 设备打开 */
    XY_DEV_STATE_BUSY,          /**< 设备忙 */
    XY_DEV_STATE_ERROR,         /**< 设备错误 */
    XY_DEV_STATE_SUSPENDED,     /**< 设备挂起 */
} xy_dev_state_t;

/* 设备控制命令 */
typedef enum {
    XY_DEV_CMD_CONFIG = 0,      /**< 配置设备 */
    XY_DEV_CMD_ENABLE,          /**< 使能设备 */
    XY_DEV_CMD_DISABLE,         /**< 禁用设备 */
    XY_DEV_CMD_RESET,           /**< 复位设备 */
    XY_DEV_CMD_GET_INFO,        /**< 获取设备信息 */
    XY_DEV_CMD_SET_CALLBACK,    /**< 设置回调 */
    XY_DEV_CMD_GET_STATE,       /**< 获取设备状态 */
    XY_DEV_CMD_SET_POWER,       /**< 设置电源模式 */
    XY_DEV_CMD_GET_POWER,       /**< 获取电源模式 */
} xy_dev_cmd_t;

/* 异步操作回调 */
typedef void (*xy_async_callback_t)(void *dev, int event, void *arg);

/* 设备操作集 */
typedef struct {
    xy_error_t (*init)(void *dev);
    xy_error_t (*deinit)(void *dev);
    xy_error_t (*open)(void *dev, uint32_t flags);
    xy_error_t (*close)(void *dev);
    int32_t (*read)(void *dev, uint32_t pos, void *buf, size_t size);
    int32_t (*write)(void *dev, uint32_t pos, const void *buf, size_t size);
    xy_error_t (*control)(void *dev, uint32_t cmd, void *args);
    xy_error_t (*async_read)(void *dev, uint32_t pos, void *buf, size_t size,
                            xy_async_callback_t cb, void *arg);
    xy_error_t (*async_write)(void *dev, uint32_t pos, const void *buf, size_t size,
                             xy_async_callback_t cb, void *arg);
    xy_error_t (*ioctl)(void *dev, uint32_t cmd, void *args);
} xy_dev_ops_t;

/* 设备结构 */
typedef struct xy_device {
    const char *name;                 /**< 设备名称 */
    xy_dev_type_t type;               /**< 设备类型 */
    uint32_t flags;                   /**< 设备标志 */
    xy_dev_state_t state;             /**< 设备状态 */
    const xy_dev_ops_t *ops;          /**< 设备操作集 */
    void *priv_data;                  /**< 私有数据 */
    uint8_t ref_count;                /**< 引用计数 */
    struct xy_device *next;           /**< 链表指针 */
} xy_device_t;

/* 设备信息结构 */
typedef struct {
    const char *name;                 /**< 设备名称 */
    xy_dev_type_t type;               /**< 设备类型 */
    uint32_t flags;                   /**< 设备标志 */
    xy_dev_state_t state;             /**< 设备状态 */
    uint32_t max_data_size;           /**< 最大数据尺寸 */
    uint32_t buffer_size;             /**< 缓冲区大小 */
} xy_dev_info_t;

/* ==================== 设备框架 API ==================== */

/**
 * @brief 初始化设备框架
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_device_init(void);

/**
 * @brief 注册设备
 * @param dev 设备结构指针
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_device_register(xy_device_t *dev);

/**
 * @brief 反注册设备
 * @param dev 设备结构指针
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_device_unregister(xy_device_t *dev);

/**
 * @brief 查找设备
 * @param name 设备名称
 * @return 设备指针，NULL 表示未找到
 */
xy_device_t *xy_device_find(const char *name);

/**
 * @brief 打开设备
 * @param name 设备名称
 * @param flags 设备标志
 * @return 设备指针，NULL 表示失败
 */
xy_device_t *xy_device_open(const char *name, uint32_t flags);

/**
 * @brief 关闭设备
 * @param dev 设备指针
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_device_close(xy_device_t *dev);

/**
 * @brief 读取设备数据
 * @param dev 设备指针
 * @param pos 读取位置
 * @param buf 数据缓冲区
 * @param size 缓冲区大小
 * @return 实际读取字节数，负值表示错误
 */
int32_t xy_device_read(xy_device_t *dev, uint32_t pos, void *buf, size_t size);

/**
 * @brief 写入设备数据
 * @param dev 设备指针
 * @param pos 写入位置
 * @param buf 数据缓冲区
 * @param size 数据大小
 * @return 实际写入字节数，负值表示错误
 */
int32_t xy_device_write(xy_device_t *dev, uint32_t pos, const void *buf, size_t size);

/**
 * @brief 控制设备
 * @param dev 设备指针
 * @param cmd 控制命令
 * @param args 命令参数
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_device_control(xy_device_t *dev, uint32_t cmd, void *args);

/**
 * @brief 获取设备信息
 * @param dev 设备指针
 * @param info 设备信息输出
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_device_get_info(xy_device_t *dev, xy_dev_info_t *info);

/**
 * @brief 获取设备状态
 * @param dev 设备指针
 * @return 设备状态
 */
xy_dev_state_t xy_device_get_state(xy_device_t *dev);

/**
 * @brief 枚举设备
 * @param type 设备类型，XY_DEV_TYPE_MAX 表示所有类型
 * @param names 设备名数组输出
 * @param max_count 最大设备数量
 * @return 实际设备数量
 */
uint32_t xy_device_enumerate(xy_dev_type_t type, const char **names, uint32_t max_count);

/**
 * @brief 获取设备计数
 * @param type 设备类型，XY_DEV_TYPE_MAX 表示所有类型
 * @return 设备数量
 */
uint32_t xy_device_get_count(xy_dev_type_t type);

/**
 * @brief 设备电源管理
 * @param dev 设备指针
 * @param power_mode 电源模式
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_device_set_power_mode(xy_device_t *dev, uint8_t power_mode);

/**
 * @brief 获取设备电源模式
 * @param dev 设备指针
 * @param power_mode 电源模式输出
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_device_get_power_mode(xy_device_t *dev, uint8_t *power_mode);

/**
 * @brief 异步读取设备数据
 * @param dev 设备指针
 * @param pos 读取位置
 * @param buf 数据缓冲区
 * @param size 缓冲区大小
 * @param cb 回调函数
 * @param arg 回调参数
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_device_async_read(xy_device_t *dev, uint32_t pos, void *buf, 
                                size_t size, xy_async_callback_t cb, void *arg);

/**
 * @brief 异步写入设备数据
 * @param dev 设备指针
 * @param pos 写入位置
 * @param buf 数据缓冲区
 * @param size 数据大小
 * @param cb 回调函数
 * @param arg 回调参数
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_device_async_write(xy_device_t *dev, uint32_t pos, const void *buf,
                                 size_t size, xy_async_callback_t cb, void *arg);

/* ==================== 便利宏定义 ==================== */

/**
 * @brief 设备注册宏
 */
#define XY_DEVICE_REGISTER(name, type, init_func, ops_ptr, priv_data_ptr) \
    static xy_device_t name##_device = { \
        .name = #name, \
        .type = type, \
        .flags = 0, \
        .state = XY_DEV_STATE_CLOSED, \
        .ops = ops_ptr, \
        .priv_data = priv_data_ptr, \
        .ref_count = 0, \
        .next = NULL, \
    }; \
    XY_INITIALIZER(xy_register_##name##_device, \
                   XY_INIT_LEVEL_DRIVER, \
                   xy_device_register, &name##_device)

/**
 * @brief 检查设备是否准备好
 */
#define XY_DEVICE_READY(dev) \
    ((dev) && (dev)->state == XY_DEV_STATE_OPENED)

/**
 * @brief 设备读取宏 (简化版)
 */
#define XY_DEVICE_READ(dev, buf, size) \
    xy_device_read(dev, 0, buf, size)

/**
 * @brief 设备写入宏 (简化版)
 */
#define XY_DEVICE_WRITE(dev, buf, size) \
    xy_device_write(dev, 0, buf, size)

/**
 * @brief 设备控制宏 (简化版)
 */
#define XY_DEVICE_CONTROL(dev, cmd, args) \
    xy_device_control(dev, cmd, args)

#ifdef __cplusplus
}
#endif

#endif /* XY_DEVICE_H */
```

### 2.2 设备框架实现 (xy_device.c)

```c
/**
 * @file xy_device.c
 * @brief XinYi Device Framework Implementation
 * @version 2.0
 * @date 2026-02-28
 */

#include "xy_device.h"
#include <string.h>

/* 最大设备数量 */
#ifndef XY_DEVICE_MAX_COUNT
#define XY_DEVICE_MAX_COUNT 32
#endif

/* 设备链表 */
static xy_device_t *g_device_list = NULL;
static uint32_t g_device_count = 0;

/* 设备框架互斥锁 */
static xy_mutex_t g_device_mutex = XY_MUTEX_INITIALIZER;

xy_error_t xy_device_init(void)
{
    g_device_list = NULL;
    g_device_count = 0;
    return XY_OK;
}

xy_error_t xy_device_register(xy_device_t *dev)
{
    if (!dev || !dev->name) {
        return XY_ERROR_INVALID_PARAM;
    }

    /* 检查设备名是否已存在 */
    xy_device_t *existing = xy_device_find(dev->name);
    if (existing) {
        return XY_ERROR_ALREADY_EXISTS;
    }

    /* 检查设备数量限制 */
    if (g_device_count >= XY_DEVICE_MAX_COUNT) {
        return XY_ERROR_NO_RESOURCE;
    }

    /* 添加到链表头部 */
    xy_mutex_lock(&g_device_mutex);
    dev->next = g_device_list;
    g_device_list = dev;
    g_device_count++;
    xy_mutex_unlock(&g_device_mutex);

    return XY_OK;
}

xy_error_t xy_device_unregister(xy_device_t *dev)
{
    if (!dev) {
        return XY_ERROR_INVALID_PARAM;
    }

    xy_mutex_lock(&g_device_mutex);
    
    /* 如果是头节点 */
    if (g_device_list == dev) {
        g_device_list = dev->next;
        g_device_count--;
        xy_mutex_unlock(&g_device_mutex);
        return XY_OK;
    }

    /* 查找前一个节点 */
    xy_device_t *prev = g_device_list;
    while (prev && prev->next != dev) {
        prev = prev->next;
    }

    if (!prev) {
        xy_mutex_unlock(&g_device_mutex);
        return XY_ERROR_NOT_FOUND;
    }

    /* 从链表移除 */
    prev->next = dev->next;
    g_device_count--;
    xy_mutex_unlock(&g_device_mutex);

    return XY_OK;
}

xy_device_t *xy_device_find(const char *name)
{
    if (!name) {
        return NULL;
    }

    xy_mutex_lock(&g_device_mutex);
    xy_device_t *dev = g_device_list;
    while (dev) {
        if (strcmp(dev->name, name) == 0) {
            xy_mutex_unlock(&g_device_mutex);
            return dev;
        }
        dev = dev->next;
    }
    xy_mutex_unlock(&g_device_mutex);

    return NULL;
}

xy_device_t *xy_device_open(const char *name, uint32_t flags)
{
    xy_device_t *dev = xy_device_find(name);
    if (!dev) {
        return NULL;
    }

    if (dev->ops && dev->ops->open) {
        xy_error_t ret = dev->ops->open(dev, flags);
        if (ret != XY_OK) {
            return NULL;
        }
    }

    dev->state = XY_DEV_STATE_OPENED;
    dev->ref_count++;
    
    return dev;
}

xy_error_t xy_device_close(xy_device_t *dev)
{
    if (!dev) {
        return XY_ERROR_INVALID_PARAM;
    }

    if (dev->ref_count > 0) {
        dev->ref_count--;
    }

    if (dev->ops && dev->ops->close) {
        xy_error_t ret = dev->ops->close(dev);
        if (ret != XY_OK) {
            return ret;
        }
    }

    dev->state = XY_DEV_STATE_CLOSED;
    return XY_OK;
}

int32_t xy_device_read(xy_device_t *dev, uint32_t pos, void *buf, size_t size)
{
    if (!dev || !buf || size == 0) {
        return XY_ERROR_INVALID_PARAM;
    }

    if (dev->state != XY_DEV_STATE_OPENED) {
        return XY_ERROR_NOT_READY;
    }

    if (!(dev->flags & XY_DEV_FLAG_RDWR) && !(dev->flags & XY_DEV_FLAG_RDONLY)) {
        return XY_ERROR_ACCESS_DENIED;
    }

    if (dev->ops && dev->ops->read) {
        return dev->ops->read(dev, pos, buf, size);
    }

    return XY_ERROR_NOT_SUPPORTED;
}

int32_t xy_device_write(xy_device_t *dev, uint32_t pos, const void *buf, size_t size)
{
    if (!dev || !buf || size == 0) {
        return XY_ERROR_INVALID_PARAM;
    }

    if (dev->state != XY_DEV_STATE_OPENED) {
        return XY_ERROR_NOT_READY;
    }

    if (!(dev->flags & XY_DEV_FLAG_RDWR) && !(dev->flags & XY_DEV_FLAG_WRONLY)) {
        return XY_ERROR_ACCESS_DENIED;
    }

    if (dev->ops && dev->ops->write) {
        return dev->ops->write(dev, pos, buf, size);
    }

    return XY_ERROR_NOT_SUPPORTED;
}

xy_error_t xy_device_control(xy_device_t *dev, uint32_t cmd, void *args)
{
    if (!dev) {
        return XY_ERROR_INVALID_PARAM;
    }

    if (dev->ops && dev->ops->control) {
        return dev->ops->control(dev, cmd, args);
    }

    return XY_ERROR_NOT_SUPPORTED;
}

xy_error_t xy_device_get_info(xy_device_t *dev, xy_dev_info_t *info)
{
    if (!dev || !info) {
        return XY_ERROR_INVALID_PARAM;
    }

    info->name = dev->name;
    info->type = dev->type;
    info->flags = dev->flags;
    info->state = dev->state;
    info->max_data_size = 0;  /* 需要驱动实现提供 */
    info->buffer_size = 0;    /* 需要驱动实现提供 */

    return XY_OK;
}

xy_dev_state_t xy_device_get_state(xy_device_t *dev)
{
    if (!dev) {
        return XY_DEV_STATE_ERROR;
    }
    return dev->state;
}

uint32_t xy_device_enumerate(xy_dev_type_t type, const char **names, uint32_t max_count)
{
    if (!names || max_count == 0) {
        return 0;
    }

    xy_mutex_lock(&g_device_mutex);
    xy_device_t *dev = g_device_list;
    uint32_t count = 0;

    while (dev && count < max_count) {
        if (type == XY_DEV_TYPE_MAX || dev->type == type) {
            names[count] = dev->name;
            count++;
        }
        dev = dev->next;
    }
    xy_mutex_unlock(&g_device_mutex);

    return count;
}

uint32_t xy_device_get_count(xy_dev_type_t type)
{
    xy_mutex_lock(&g_device_mutex);
    xy_device_t *dev = g_device_list;
    uint32_t count = 0;

    while (dev) {
        if (type == XY_DEV_TYPE_MAX || dev->type == type) {
            count++;
        }
        dev = dev->next;
    }
    xy_mutex_unlock(&g_device_mutex);

    return count;
}

/* ==================== 总线设备框架 ==================== */

/* 总线操作集 */
typedef struct {
    xy_error_t (*take_bus)(void *bus);
    xy_error_t (*release_bus)(void *bus);
    xy_error_t (*transfer)(void *bus, void *node, const void *send_buf, 
                          void *recv_buf, size_t length);
    xy_error_t (*configure)(void *bus, void *node, const void *config);
} xy_bus_ops_t;

/* 总线设备结构 */
typedef struct {
    xy_device_t parent;              /**< 父设备 */
    const xy_bus_ops_t *bus_ops;     /**< 总线操作 */
    uint32_t speed;                  /**< 总线速度 */
    void *bus_data;                  /**< 总线私有数据 */
    uint8_t node_count;              /**< 节点数量 */
} xy_bus_device_t;

/* 总线节点结构 */
typedef struct {
    xy_device_t parent;              /**< 设备节点 */
    xy_bus_device_t *bus;            /**< 所属总线 */
    uint32_t addr;                   /**< 设备地址 */
    void *node_data;                 /**< 节点私有数据 */
} xy_bus_node_t;

/* 总线设备操作 */
xy_error_t xy_bus_take(xy_bus_device_t *bus)
{
    if (!bus || !bus->bus_ops || !bus->bus_ops->take_bus) {
        return XY_ERROR_INVALID_PARAM;
    }
    return bus->bus_ops->take_bus(bus);
}

xy_error_t xy_bus_release(xy_bus_device_t *bus)
{
    if (!bus || !bus->bus_ops || !bus->bus_ops->release_bus) {
        return XY_ERROR_INVALID_PARAM;
    }
    return bus->bus_ops->release_bus(bus);
}

xy_error_t xy_bus_transfer(xy_bus_device_t *bus, xy_bus_node_t *node,
                          const void *send_buf, void *recv_buf, size_t length)
{
    if (!bus || !node || !bus->bus_ops || !bus->bus_ops->transfer) {
        return XY_ERROR_INVALID_PARAM;
    }
    return bus->bus_ops->transfer(bus, node, send_buf, recv_buf, length);
}

/* ==================== 传感器设备框架 ==================== */

/* 传感器类型定义 */
typedef enum {
    XY_SENSOR_TYPE_TEMP = 0,         /**< 温度传感器 */
    XY_SENSOR_TYPE_HUMIDITY,         /**< 湿度传感器 */
    XY_SENSOR_TYPE_PRESSURE,         /**< 压力传感器 */
    XY_SENSOR_TYPE_ACCELEROMETER,    /**< 加速度计 */
    XY_SENSOR_TYPE_GYROSCOPE,        /**< 陀螺仪 */
    XY_SENSOR_TYPE_MAGNETOMETER,     /**< 磁力计 */
    XY_SENSOR_TYPE_LIGHT,            /**< 光传感器 */
    XY_SENSOR_TYPE_PROXIMITY,        /**< 接近传感器 */
    XY_SENSOR_TYPE_GAS,              /**< 气体传感器 */
    XY_SENSOR_TYPE_COLOR,            /**< 颜色传感器 */
    XY_SENSOR_TYPE_HALL,             /**< 霍尔传感器 */
    XY_SENSOR_TYPE_IR,               /**< 红外传感器 */
    XY_SENSOR_TYPE_UV,               /**< 紫外传感器 */
    XY_SENSOR_TYPE_PH,               /**< pH 传感器 */
    XY_SENSOR_TYPE_EC,               /**< 电导率传感器 */
    XY_SENSOR_TYPE_MAX
} xy_sensor_type_t;

/* 传感器数据值 */
typedef struct {
    int32_t val1;                    /**< 整数部分 */
    int32_t val2;                    /**< 小数部分 (百万分之一) */
} xy_sensor_value_t;

/* 传感器事件 */
typedef enum {
    XY_SENSOR_EVT_DATA_READY = 0,    /**< 数据就绪 */
    XY_SENSOR_EVT_THRESHOLD,         /**< 阈值事件 */
    XY_SENSOR_EVT_ERROR,             /**< 错误事件 */
} xy_sensor_evt_t;

/* 传感器配置 */
typedef struct {
    xy_sensor_type_t sensor_type;    /**< 传感器类型 */
    uint32_t sample_rate;            /**< 采样率 (Hz) */
    uint8_t power_mode;              /**< 电源模式 */
    uint32_t scale;                  /**< 测量范围 */
    uint8_t resolution;              /**< 分辨率 (位) */
} xy_sensor_config_t;

/* 传感器操作集 */
typedef struct {
    xy_error_t (*sample_fetch)(void *sensor, xy_sensor_type_t channel);
    xy_error_t (*channel_get)(void *sensor, xy_sensor_type_t channel, 
                             xy_sensor_value_t *val);
    xy_error_t (*configure)(void *sensor, const xy_sensor_config_t *config);
    xy_error_t (*set_threshold)(void *sensor, xy_sensor_type_t channel,
                                const xy_sensor_value_t *threshold);
} xy_sensor_ops_t;

/* 传感器设备结构 */
typedef struct {
    xy_device_t parent;              /**< 父设备 */
    const xy_sensor_ops_t *sensor_ops; /**< 传感器操作 */
    xy_sensor_config_t config;       /**< 传感器配置 */
    void *sensor_data;               /**< 传感器私有数据 */
} xy_sensor_device_t;

/* 传感器设备操作 */
xy_error_t xy_sensor_sample_fetch(xy_sensor_device_t *sensor, xy_sensor_type_t channel)
{
    if (!sensor || !sensor->sensor_ops || !sensor->sensor_ops->sample_fetch) {
        return XY_ERROR_INVALID_PARAM;
    }
    return sensor->sensor_ops->sample_fetch(sensor, channel);
}

xy_error_t xy_sensor_channel_get(xy_sensor_device_t *sensor, xy_sensor_type_t channel,
                                xy_sensor_value_t *val)
{
    if (!sensor || !val || !sensor->sensor_ops || !sensor->sensor_ops->channel_get) {
        return XY_ERROR_INVALID_PARAM;
    }
    return sensor->sensor_ops->channel_get(sensor, channel, val);
}

#endif /* XY_DEVICE_IMPLEMENTATION */
```

## 3. 驱动实现模板

### 3.1 UART 驱动示例 (xy_dev_uart.c)

```c
/**
 * @file xy_dev_uart.c
 * @brief XinYi UART Device Driver
 * @version 2.0
 * @date 2026-02-28
 */

#include "../inc/xy_device.h"

/* UART 设备私有数据 */
typedef struct {
    void *hal_handle;                /**< HAL 句柄 */
    uint32_t baudrate;               /**< 波特率 */
    uint8_t data_bits;               /**< 数据位 */
    uint8_t stop_bits;               /**< 停止位 */
    uint8_t parity;                  /**< 校验位 */
    uint8_t flow_ctrl;               /**< 流控制 */
    uint8_t tx_enabled;              /**< TX 使能 */
    uint8_t rx_enabled;              /**< RX 使能 */
} xy_uart_dev_data_t;

/* UART 设备配置 */
typedef struct {
    uint32_t baudrate;               /**< 波特率 */
    uint8_t data_bits;               /**< 数据位 */
    uint8_t stop_bits;               /**< 停止位 */
    uint8_t parity;                  /**< 校验位 */
    uint8_t flow_ctrl;               /**< 流控制 */
    uint32_t flags;                  /**< 设备标志 */
} xy_uart_config_t;

/* UART 设备操作集 */
static xy_error_t uart_init(void *dev);
static xy_error_t uart_deinit(void *dev);
static xy_error_t uart_open(void *dev, uint32_t flags);
static xy_error_t uart_close(void *dev);
static int32_t uart_read(void *dev, uint32_t pos, void *buf, size_t size);
static int32_t uart_write(void *dev, uint32_t pos, const void *buf, size_t size);
static xy_error_t uart_control(void *dev, uint32_t cmd, void *args);

static const xy_dev_ops_t uart_ops = {
    .init = uart_init,
    .deinit = uart_deinit,
    .open = uart_open,
    .close = uart_close,
    .read = uart_read,
    .write = uart_write,
    .control = uart_control,
};

/* UART 设备实例 */
static xy_uart_dev_data_t uart1_data = { 0 };
static xy_device_t uart1_device = {
    .name = "uart1",
    .type = XY_DEV_TYPE_UART,
    .flags = XY_DEV_FLAG_RDWR | XY_DEV_FLAG_INT,
    .ops = &uart_ops,
    .priv_data = &uart1_data,
    .ref_count = 0,
    .next = NULL,
};

/* UART 初始化 */
static xy_error_t uart_init(void *dev)
{
    xy_device_t *device = (xy_device_t *)dev;
    xy_uart_dev_data_t *data = (xy_uart_dev_data_t *)device->priv_data;
    
    if (!data) {
        return XY_ERROR_INVALID_PARAM;
    }
    
    /* 使用 HAL 初始化 UART */
    xy_hal_uart_config_t hal_config = {
        .baudrate = data->baudrate ? data->baudrate : 115200,
        .wordlen = (data->data_bits == 9) ? XY_HAL_UART_WORDLEN_9B : XY_HAL_UART_WORDLEN_8B,
        .stopbits = (data->stop_bits == 2) ? XY_HAL_UART_STOPBITS_2 : XY_HAL_UART_STOPBITS_1,
        .parity = data->parity,
        .flowctrl = data->flow_ctrl,
        .mode = XY_HAL_UART_MODE_TX_RX,
    };
    
    xy_error_t ret = xy_hal_uart_init(data->hal_handle, &hal_config);
    if (ret != XY_OK) {
        return ret;
    }
    
    data->tx_enabled = 1;
    data->rx_enabled = 1;
    device->state = XY_DEV_STATE_CLOSED;
    
    return XY_OK;
}

/* UART 打开 */
static xy_error_t uart_open(void *dev, uint32_t flags)
{
    xy_device_t *device = (xy_device_t *)dev;
    xy_uart_dev_data_t *data = (xy_uart_dev_data_t *)device->priv_data;
    
    XY_UNUSED(flags);
    
    /* 检查 UART 是否已初始化 */
    if (!data->hal_handle) {
        return XY_ERROR_NOT_INIT;
    }
    
    device->state = XY_DEV_STATE_OPENED;
    return XY_OK;
}

/* UART 读取 */
static int32_t uart_read(void *dev, uint32_t pos, void *buf, size_t size)
{
    XY_UNUSED(pos);
    xy_device_t *device = (xy_device_t *)dev;
    xy_uart_dev_data_t *data = (xy_uart_dev_data_t *)device->priv_data;
    
    if (device->state != XY_DEV_STATE_OPENED || !data->rx_enabled) {
        return XY_ERROR_NOT_READY;
    }
    
    /* 使用 HAL 读取数据 */
    return xy_hal_uart_recv(data->hal_handle, (uint8_t *)buf, size, 1000);
}

/* UART 写入 */
static int32_t uart_write(void *dev, uint32_t pos, const void *buf, size_t size)
{
    XY_UNUSED(pos);
    xy_device_t *device = (xy_device_t *)dev;
    xy_uart_dev_data_t *data = (xy_uart_dev_data_t *)device->priv_data;
    
    if (device->state != XY_DEV_STATE_OPENED || !data->tx_enabled) {
        return XY_ERROR_NOT_READY;
    }
    
    /* 使用 HAL 发送数据 */
    return xy_hal_uart_send(data->hal_handle, (const uint8_t *)buf, size, 1000);
}

/* UART 控制 */
static xy_error_t uart_control(void *dev, uint32_t cmd, void *args)
{
    xy_device_t *device = (xy_device_t *)dev;
    xy_uart_dev_data_t *data = (xy_uart_dev_data_t *)device->priv_data;
    
    switch (cmd) {
    case XY_DEV_CMD_CONFIG: {
        xy_uart_config_t *config = (xy_uart_config_t *)args;
        if (!config) return XY_ERROR_INVALID_PARAM;
        
        data->baudrate = config->baudrate;
        data->data_bits = config->data_bits;
        data->stop_bits = config->stop_bits;
        data->parity = config->parity;
        data->flow_ctrl = config->flow_ctrl;
        break;
    }
    case XY_DEV_CMD_GET_INFO: {
        xy_dev_info_t *info = (xy_dev_info_t *)args;
        if (!info) return XY_ERROR_INVALID_PARAM;
        
        info->max_data_size = 256;  /* UART 最大单次传输大小 */
        info->buffer_size = 1024;   /* UART 缓冲区大小 */
        break;
    }
    default:
        return XY_ERROR_INVALID_PARAM;
    }
    
    return XY_OK;
}

/* UART 设备注册 */
static int uart_driver_init(void)
{
    return xy_device_register(&uart1_device);
}

/* 注册初始化函数 */
XY_INITIALIZER(uart_driver_init, XY_INIT_LEVEL_DRIVER, uart_driver_init, NULL);

/* ==================== 便利函数 ==================== */

xy_error_t xy_uart_set_baudrate(const char *name, uint32_t baudrate)
{
    xy_device_t *dev = xy_device_find(name);
    if (!dev || dev->type != XY_DEV_TYPE_UART) {
        return XY_ERROR_INVALID_PARAM;
    }
    
    xy_uart_dev_data_t *data = (xy_uart_dev_data_t *)dev->priv_data;
    data->baudrate = baudrate;
    
    /* 如果设备已打开，重新配置 */
    if (dev->state == XY_DEV_STATE_OPENED) {
        xy_uart_config_t config = {
            .baudrate = baudrate,
            .data_bits = data->data_bits,
            .stop_bits = data->stop_bits,
            .parity = data->parity,
            .flow_ctrl = data->flow_ctrl,
        };
        return xy_device_control(dev, XY_DEV_CMD_CONFIG, &config);
    }
    
    return XY_OK;
}

xy_error_t xy_uart_flush(const char *name)
{
    xy_device_t *dev = xy_device_find(name);
    if (!dev || dev->type != XY_DEV_TYPE_UART) {
        return XY_ERROR_INVALID_PARAM;
    }
    
    /* 使用 HAL 刷新 UART */
    xy_uart_dev_data_t *data = (xy_uart_dev_data_t *)dev->priv_data;
    return xy_hal_uart_flush(data->hal_handle);
}

#endif /* XY_UART_DEVICE_IMPLEMENTATION */
```

### 3.2 SPI 总线驱动示例 (xy_bus_spi.c)

```c
/**
 * @file xy_bus_spi.c
 * @brief XinYi SPI Bus Device Driver
 * @version 2.0
 * @date 2026-02-28
 */

#include "../inc/xy_device.h"

/* SPI 总线私有数据 */
typedef struct {
    void *hal_handle;                /**< HAL 句柄 */
    uint32_t max_speed;              /**< 最大速度 */
    uint8_t mode;                    /**< SPI 模式 */
    uint8_t bits_per_word;           /**< 每字位数 */
    uint8_t cs_active_high;          /**< CS 极性 */
} xy_spi_bus_data_t;

/* SPI 节点私有数据 */
typedef struct {
    xy_bus_node_t parent;            /**< 总线节点父类 */
    uint8_t cs_pin;                  /**< CS 引脚 */
    uint32_t speed;                  /**< 设备速度 */
    uint8_t mode;                    /**< 设备模式 */
    uint8_t bits_per_word;           /**< 设备每字位数 */
} xy_spi_node_data_t;

/* SPI 总线操作集 */
static xy_error_t spi_bus_take(void *bus);
static xy_error_t spi_bus_release(void *bus);
static xy_error_t spi_bus_transfer(void *bus, void *node, 
                                  const void *send_buf, void *recv_buf, size_t length);
static xy_error_t spi_bus_configure(void *bus, void *node, const void *config);

static const xy_bus_ops_t spi_bus_ops = {
    .take_bus = spi_bus_take,
    .release_bus = spi_bus_release,
    .transfer = spi_bus_transfer,
    .configure = spi_bus_configure,
};

/* SPI 总线操作实现 */
static xy_error_t spi_bus_take(void *bus)
{
    xy_bus_device_t *bus_dev = (xy_bus_device_t *)bus;
    xy_spi_bus_data_t *data = (xy_spi_bus_data_t *)bus_dev->bus_data;
    
    /* 使用 HAL 锁定 SPI 总线 */
    XY_UNUSED(data);
    return XY_OK;
}

static xy_error_t spi_bus_release(void *bus)
{
    xy_bus_device_t *bus_dev = (xy_bus_device_t *)bus;
    xy_spi_bus_data_t *data = (xy_spi_bus_data_t *)bus_dev->bus_data;
    
    XY_UNUSED(data);
    return XY_OK;
}

static xy_error_t spi_bus_transfer(void *bus, void *node, 
                                  const void *send_buf, void *recv_buf, size_t length)
{
    xy_bus_device_t *bus_dev = (xy_bus_device_t *)bus;
    xy_spi_bus_data_t *bus_data = (xy_spi_bus_data_t *)bus_dev->bus_data;
    xy_spi_node_data_t *node_data = (xy_spi_node_data_t *)((xy_bus_node_t *)node)->node_data;
    
    XY_UNUSED(bus_data);
    XY_UNUSED(node_data);
    
    /* 使用 HAL 进行 SPI 传输 */
    return xy_hal_spi_transmit_receive(bus_data->hal_handle, 
                                      (const uint8_t *)send_buf, 
                                      (uint8_t *)recv_buf, 
                                      length, 
                                      1000);
}

/* SPI 总线设备注册 */
static int spi_bus_driver_init(void)
{
    static xy_spi_bus_data_t spi1_bus_data = { 0 };
    static xy_bus_device_t spi1_bus = {
        .parent = {
            .name = "spi1",
            .type = XY_DEV_TYPE_BUS,
            .flags = XY_DEV_FLAG_RDWR,
            .ops = NULL,  /* 总线设备不直接使用通用操作 */
            .priv_data = &spi1_bus_data,
            .ref_count = 0,
            .next = NULL,
        },
        .bus_ops = &spi_bus_ops,
        .speed = 1000000,  /* 1MHz */
        .bus_data = &spi1_bus_data,
        .node_count = 0,
    };
    
    return xy_device_register(&spi1_bus.parent);
}

/* 注册初始化函数 */
XY_INITIALIZER(spi_bus_driver_init, XY_INIT_LEVEL_DRIVER, spi_bus_driver_init, NULL);

#endif /* XY_SPI_BUS_IMPLEMENTATION */
```

## 4. 构建系统配置

### 4.1 CMakeLists.txt

```cmake
# components/device/CMakeLists.txt
cmake_minimum_required(VERSION 3.12)
project(xy_device C)

# Configuration options
option(XY_DEVICE_ENABLED "Enable XY Device Framework" ON)
option(XY_DEVICE_UART_ENABLED "Enable UART Device" ON)
option(XY_DEVICE_SPI_ENABLED "Enable SPI Device" ON)
option(XY_DEVICE_I2C_ENABLED "Enable I2C Device" ON)
option(XY_DEVICE_GPIO_ENABLED "Enable GPIO Device" ON)
option(XY_DEVICE_ADC_ENABLED "Enable ADC Device" ON)
option(XY_DEVICE_SENSOR_ENABLED "Enable Sensor Device" ON)

# Core device framework
set(DEVICE_CORE_SOURCES
    src/xy_device.c
)

# Core device headers
set(DEVICE_CORE_HEADERS
    inc/xy_device.h
)

# Conditional sources based on options
if(XY_DEVICE_UART_ENABLED)
    list(APPEND DEVICE_CORE_SOURCES src/xy_dev_uart.c)
    list(APPEND DEVICE_CORE_HEADERS inc/xy_dev_uart.h)
endif()

if(XY_DEVICE_SPI_ENABLED)
    list(APPEND DEVICE_CORE_SOURCES src/xy_dev_spi.c)
    list(APPEND DEVICE_CORE_HEADERS inc/xy_dev_spi.h)
endif()

if(XY_DEVICE_I2C_ENABLED)
    list(APPEND DEVICE_CORE_SOURCES src/xy_dev_i2c.c)
    list(APPEND DEVICE_CORE_HEADERS inc/xy_dev_i2c.h)
endif()

if(XY_DEVICE_GPIO_ENABLED)
    list(APPEND DEVICE_CORE_SOURCES src/xy_dev_gpio.c)
    list(APPEND DEVICE_CORE_HEADERS inc/xy_dev_gpio.h)
endif()

if(XY_DEVICE_ADC_ENABLED)
    list(APPEND DEVICE_CORE_SOURCES src/xy_dev_adc.c)
    list(APPEND DEVICE_CORE_HEADERS inc/xy_dev_adc.h)
endif()

if(XY_DEVICE_SENSOR_ENABLED)
    list(APPEND DEVICE_CORE_SOURCES src/xy_dev_sensor.c)
    list(APPEND DEVICE_CORE_HEADERS inc/xy_dev_sensor.h)
endif()

# Bus implementations
set(DEVICE_BUS_SOURCES
    bus/xy_bus_spi.c
    bus/xy_bus_i2c.c
    bus/xy_bus_can.c
)

# Create device library
add_library(xy_device STATIC
    ${DEVICE_CORE_SOURCES}
    ${DEVICE_BUS_SOURCES}
)

# Include directories
target_include_directories(xy_device PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../hal/inc
)

# Dependencies
target_link_libraries(xy_device PRIVATE
    xy_hal
)

# Compile definitions
target_compile_definitions(xy_device PUBLIC
    XY_DEVICE_ENABLED
    $<$<BOOL:${XY_DEVICE_UART_ENABLED}>:XY_DEVICE_UART_ENABLED>
    $<$<BOOL:${XY_DEVICE_SPI_ENABLED}>:XY_DEVICE_SPI_ENABLED>
    $<$<BOOL:${XY_DEVICE_I2C_ENABLED}>:XY_DEVICE_I2C_ENABLED>
    $<$<BOOL:${XY_DEVICE_GPIO_ENABLED}>:XY_DEVICE_GPIO_ENABLED>
    $<$<BOOL:${XY_DEVICE_ADC_ENABLED}>:XY_DEVICE_ADC_ENABLED>
    $<$<BOOL:${XY_DEVICE_SENSOR_ENABLED}>:XY_DEVICE_SENSOR_ENABLED>
)

# Compile options
target_compile_options(xy_device PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wno-unused-parameter
)

# Installation
install(TARGETS xy_device
    ARCHIVE DESTINATION lib
)

install(FILES ${DEVICE_CORE_HEADERS}
    DESTINATION include/xy_device
)

# Testing
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

# Print configuration
message(STATUS "XY Device Framework Configuration:")
message(STATUS "  Core: ${XY_DEVICE_ENABLED}")
message(STATUS "  UART: ${XY_DEVICE_UART_ENABLED}")
message(STATUS "  SPI: ${XY_DEVICE_SPI_ENABLED}")
message(STATUS "  I2C: ${XY_DEVICE_I2C_ENABLED}")
message(STATUS "  GPIO: ${XY_DEVICE_GPIO_ENABLED}")
message(STATUS "  ADC: ${XY_DEVICE_ADC_ENABLED}")
message(STATUS "  Sensor: ${XY_DEVICE_SENSOR_ENABLED}")
```

### 4.2 Kconfig

```
# components/device/Kconfig
menu "XY Device Framework Configuration"

config XY_DEVICE_ENABLED
    bool "Enable XY Device Framework"
    default y
    help
      Enable the XY Device framework for managing hardware devices.

if XY_DEVICE_ENABLED

config XY_DEVICE_MAX_COUNT
    int "Maximum number of devices"
    default 32
    range 8 256
    help
      Maximum number of devices that can be registered.

config XY_DEVICE_UART_ENABLED
    bool "Enable UART Device"
    default y
    help
      Enable UART device support.

config XY_DEVICE_SPI_ENABLED
    bool "Enable SPI Device"
    default y
    help
      Enable SPI device support.

config XY_DEVICE_I2C_ENABLED
    bool "Enable I2C Device"
    default y
    help
      Enable I2C device support.

config XY_DEVICE_GPIO_ENABLED
    bool "Enable GPIO Device"
    default y
    help
      Enable GPIO device support.

config XY_DEVICE_ADC_ENABLED
    bool "Enable ADC Device"
    default y
    help
      Enable ADC device support.

config XY_DEVICE_SENSOR_ENABLED
    bool "Enable Sensor Device"
    default y
    depends on XY_DEVICE_ADC_ENABLED
    help
      Enable sensor device support.

config XY_DEVICE_BUS_ENABLED
    bool "Enable Bus Device Support"
    default y
    help
      Enable bus device support (SPI, I2C, CAN buses).

config XY_DEVICE_POWER_MANAGEMENT
    bool "Enable Device Power Management"
    default y
    help
      Enable power management for devices.

config XY_DEVICE_ASYNC_SUPPORT
    bool "Enable Async Device Operations"
    default y
    help
      Enable asynchronous device operations with callbacks.

config XY_DEVICE_DMA_SUPPORT
    bool "Enable DMA Device Operations"
    default y
    depends on XY_DEVICE_ASYNC_SUPPORT
    help
      Enable DMA-based device operations.

endif # XY_DEVICE_ENABLED

endmenu
```

## 5. 使用示例

### 5.1 设备使用示例 (xy_device_example.c)

```c
/**
 * @file xy_device_example.c
 * @brief XY Device Framework Usage Examples
 * @version 2.0
 * @date 2026-02-28
 */

#include "xy_device.h"

void device_example_main(void)
{
    /* 1. 初始化设备框架 */
    xy_device_init();
    
    /* 2. 查找设备 */
    xy_device_t *uart_dev = xy_device_find("uart1");
    if (!uart_dev) {
        xy_log_e("UART device not found\n");
        return;
    }
    
    /* 3. 打开设备 */
    xy_device_t *opened_dev = xy_device_open("uart1", XY_DEV_FLAG_RDWR);
    if (!opened_dev) {
        xy_log_e("Failed to open UART device\n");
        return;
    }
    
    /* 4. 使用设备 */
    const char *msg = "Hello from XY Device Framework!\r\n";
    int32_t written = xy_device_write(opened_dev, 0, msg, strlen(msg));
    if (written < 0) {
        xy_log_e("Write failed: %d\n", written);
    }
    
    /* 5. 控制设备 */
    xy_uart_config_t config = {
        .baudrate = 9600,
        .data_bits = 8,
        .stop_bits = 1,
        .parity = 0,
        .flow_ctrl = 0,
    };
    xy_device_control(opened_dev, XY_DEV_CMD_CONFIG, &config);
    
    /* 6. 关闭设备 */
    xy_device_close(opened_dev);
    
    /* 7. 枚举设备 */
    const char *device_names[32];
    uint32_t count = xy_device_enumerate(XY_DEV_TYPE_UART, device_names, 32);
    xy_log_i("Found %u UART devices:\n", count);
    for (uint32_t i = 0; i < count; i++) {
        xy_log_i("  %s\n", device_names[i]);
    }
}

/* 异步操作示例 */
static void async_callback(void *dev, int event, void *arg)
{
    XY_UNUSED(dev);
    XY_UNUSED(arg);
    
    switch (event) {
    case 0: /* TX complete */
        xy_log_i("Async TX complete\n");
        break;
    case 1: /* RX complete */
        xy_log_i("Async RX complete\n");
        break;
    case 2: /* Error */
        xy_log_e("Async operation error\n");
        break;
    }
}

void async_device_example(void)
{
    xy_device_t *uart_dev = xy_device_open("uart1", XY_DEV_FLAG_RDWR | XY_DEV_FLAG_ASYNC);
    if (!uart_dev) return;
    
    const char *data = "Async data";
    xy_device_async_write(uart_dev, 0, data, strlen(data), async_callback, NULL);
    
    /* Wait for completion */
    while (xy_device_get_state(uart_dev) == XY_DEV_STATE_BUSY) {
        xy_os_delay(10);
    }
    
    xy_device_close(uart_dev);
}

/* 传感器设备示例 */
void sensor_device_example(void)
{
    xy_device_t *temp_sensor = xy_device_find("temp_sensor1");
    if (!temp_sensor) {
        xy_log_e("Temperature sensor not found\n");
        return;
    }
    
    /* 获取传感器数据 */
    xy_sensor_device_t *sensor = (xy_sensor_device_t *)temp_sensor;
    xy_sensor_sample_fetch(sensor, XY_SENSOR_TYPE_TEMP);
    
    xy_sensor_value_t temp_val;
    xy_sensor_channel_get(sensor, XY_SENSOR_TYPE_TEMP, &temp_val);
    
    float temperature = temp_val.val1 + (float)temp_val.val2 / 1000000.0f;
    xy_log_i("Temperature: %.2f°C\n", temperature);
}

/* 总线设备示例 */
void bus_device_example(void)
{
    xy_device_t *spi_bus = xy_device_find("spi1");
    if (!spi_bus) {
        xy_log_e("SPI bus not found\n");
        return;
    }
    
    xy_bus_device_t *bus = (xy_bus_device_t *)spi_bus;
    
    /* 获取总线节点 */
    xy_device_t *spi_device = xy_device_find("spi_flash");
    if (!spi_device) {
        xy_log_e("SPI device not found\n");
        return;
    }
    
    xy_bus_node_t *node = (xy_bus_node_t *)spi_device;
    
    /* 总线传输 */
    const uint8_t cmd[] = {0x9F};  /* Read JEDEC ID */
    uint8_t response[3];
    
    xy_bus_take(bus);
    xy_bus_transfer(bus, node, cmd, response, sizeof(cmd));
    xy_bus_release(bus);
    
    xy_log_i("JEDEC ID: 0x%02X%02X%02X\n", 
             response[0], response[1], response[2]);
}

#endif /* XY_DEVICE_EXAMPLE_IMPLEMENTATION */
```

## 6. 与 HAL 集成

### 6.1 集成策略

XinYi 设备组件与 HAL 组件的关系:

```
应用层
    ↓
设备层 (xy_device) - 统一设备接口
    ↓
HAL 层 (xy_hal) - 硬件抽象
    ↓
MCU HAL - 具体实现 (STM32 HAL, etc.)
```

### 6.2 集成示例

```c
/* xy_dev_uart.c 中与 HAL 集成 */
#include "xy_device.h"
#include "xy_hal_uart.h"

static xy_error_t uart_init(void *dev)
{
    xy_device_t *device = (xy_device_t *)dev;
    xy_uart_dev_data_t *data = (xy_uart_dev_data_t *)device->priv_data;
    
    /* 从 HAL 初始化 */
    xy_hal_uart_config_t hal_config = {
        .baudrate = data->baudrate,
        .wordlen = XY_HAL_UART_WORDLEN_8B,
        .stopbits = XY_HAL_UART_STOPBITS_1,
        .parity = XY_HAL_UART_PARITY_NONE,
        .flowctrl = XY_HAL_UART_FLOWCTRL_NONE,
        .mode = XY_HAL_UART_MODE_TX_RX,
    };
    
    return xy_hal_uart_init(data->hal_handle, &hal_config);
}

static int32_t uart_write(void *dev, uint32_t pos, const void *buf, size_t size)
{
    XY_UNUSED(pos);
    xy_device_t *device = (xy_device_t *)dev;
    xy_uart_dev_data_t *data = (xy_uart_dev_data_t *)device->priv_data;
    
    /* 通过 HAL 写入 */
    return xy_hal_uart_send(data->hal_handle, (const uint8_t *)buf, size, 1000);
}
```

## 7. 优势分析

### 7.1 设计优势

1. **统一接口**: 所有设备使用相同的注册、查找、操作接口
2. **模块化**: 按功能分类，易于扩展
3. **兼容性**: 与现有 HAL 框架无缝集成
4. **灵活性**: 支持多种设备类型和总线模型
5. **可配置**: 通过 Kconfig 裁剪功能
6. **可扩展**: 支持新设备类型的动态添加

### 7.2 与 RT-Thread/Zephyr 对比

| 特性 | XinYi 设备框架 | RT-Thread | Zephyr |
|------|----------------|-----------|--------|
| **设备模型** | 统一结构 | 统一结构 | 分离结构 |
| **注册方式** | 动态/静态 | 动态 | 静态 |
| **API 一致性** | 高 | 高 | 高 |
| **学习曲线** | 低 | 低 | 高 |
| **集成难度** | 低 | 低 | 中 |
| **代码大小** | 小 | 小 | 大 |

## 8. 实施计划

### 8.1 短期 (1-2 周)

- [ ] 完善核心设备框架 (xy_device.h/c)
- [ ] 实现基础设备驱动 (UART/SPI/I2C)
- [ ] 创建总线设备框架 (SPI/I2C 总线)
- [ ] 编写设备驱动模板

### 8.2 中期 (1 个月)

- [ ] 实现更多设备类型 (GPIO/ADC/PWM)
- [ ] 添加传感器设备框架
- [ ] 完善异步操作支持
- [ ] 创建设备驱动示例

### 8.3 长期 (3 个月)

- [ ] 完善设备电源管理
- [ ] 添加设备热插拔支持
- [ ] 实现设备驱动热更新
- [ ] 完善设备驱动认证机制

---

**维护者**: XinYi Team  
**联系方式**: zerozap2020@gmail.com  
**许可证**: Apache License 2.0
