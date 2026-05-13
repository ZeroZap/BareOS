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

/* ==================== 设备框架 API 实现 ==================== */

xy_error_t xy_device_init(void)
{
    g_device_list = NULL;
    g_device_count = 0;
    
    /* 初始化互斥锁 */
    xy_mutex_init(&g_device_mutex);
    
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

    /* 加锁保护链表操作 */
    xy_mutex_lock(&g_device_mutex);
    
    /* 添加到链表头部 */
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

    if (dev->api && dev->api->open) {
        xy_error_t ret = dev->api->open(dev, flags);
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

    if (dev->api && dev->api->close) {
        xy_error_t ret = dev->api->close(dev);
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

    if (dev->api && dev->api->read) {
        return dev->api->read(dev, pos, buf, size);
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

    if (dev->api && dev->api->write) {
        return dev->api->write(dev, pos, buf, size);
    }

    return XY_ERROR_NOT_SUPPORTED;
}

xy_error_t xy_device_control(xy_device_t *dev, uint32_t cmd, void *args)
{
    if (!dev) {
        return XY_ERROR_INVALID_PARAM;
    }

    if (dev->api && dev->api->control) {
        return dev->api->control(dev, cmd, args);
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
    info->version = 0x020000; /* 版本 2.0.0 */

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

xy_error_t xy_device_set_power_mode(xy_device_t *dev, uint8_t power_mode)
{
    if (!dev) {
        return XY_ERROR_INVALID_PARAM;
    }
    
    if (dev->api && dev->api->power_control) {
        return dev->api->power_control(dev, power_mode);
    }
    
    return XY_ERROR_NOT_SUPPORTED;
}

xy_error_t xy_device_get_power_mode(xy_device_t *dev, uint8_t *power_mode)
{
    if (!dev || !power_mode) {
        return XY_ERROR_INVALID_PARAM;
    }
    
    XY_UNUSED(power_mode);
    
    /* 如果设备支持控制命令，使用控制接口 */
    if (dev->api && dev->api->control) {
        return dev->api->control(dev, XY_DEV_CMD_GET_POWER, power_mode);
    }
    
    return XY_ERROR_NOT_SUPPORTED;
}

xy_error_t xy_device_async_read(xy_device_t *dev, uint32_t pos, void *buf, 
                                size_t size, xy_async_callback_t cb, void *arg)
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

    if (dev->api && dev->api->async_read) {
        return dev->api->async_read(dev, pos, buf, size, cb, arg);
    }

    return XY_ERROR_NOT_SUPPORTED;
}

xy_error_t xy_device_async_write(xy_device_t *dev, uint32_t pos, const void *buf,
                                 size_t size, xy_async_callback_t cb, void *arg)
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

    if (dev->api && dev->api->async_write) {
        return dev->api->async_write(dev, pos, buf, size, cb, arg);
    }

    return XY_ERROR_NOT_SUPPORTED;
}

/* ==================== 总线设备框架实现 ==================== */

xy_error_t xy_bus_take(xy_bus_device_t *bus)
{
    if (!bus || !bus->bus_api || !bus->bus_api->take_bus) {
        return XY_ERROR_INVALID_PARAM;
    }
    return bus->bus_api->take_bus(&bus->parent);
}

xy_error_t xy_bus_release(xy_bus_device_t *bus)
{
    if (!bus || !bus->bus_api || !bus->bus_api->release_bus) {
        return XY_ERROR_INVALID_PARAM;
    }
    return bus->bus_api->release_bus(&bus->parent);
}

xy_error_t xy_bus_transfer(xy_bus_device_t *bus, xy_bus_node_t *node,
                          const void *send_buf, void *recv_buf, size_t length)
{
    if (!bus || !node || !bus->bus_api || !bus->bus_api->transfer) {
        return XY_ERROR_INVALID_PARAM;
    }
    return bus->bus_api->transfer(&bus->parent, &node->parent, 
                                 send_buf, recv_buf, length);
}

xy_error_t xy_bus_configure(xy_bus_device_t *bus, xy_bus_node_t *node,
                            const void *config)
{
    if (!bus || !node || !bus->bus_api || !bus->bus_api->configure) {
        return XY_ERROR_INVALID_PARAM;
    }
    return bus->bus_api->configure(&bus->parent, &node->parent, config);
}

/* ==================== 传感器设备框架实现 ==================== */

xy_error_t xy_sensor_sample_fetch(void *sensor, xy_sensor_type_t channel)
{
    if (!sensor) {
        return XY_ERROR_INVALID_PARAM;
    }
    
    xy_sensor_device_t *sdev = (xy_sensor_device_t *)sensor;
    if (!sdev->sensor_api || !sdev->sensor_api->sample_fetch) {
        return XY_ERROR_NOT_SUPPORTED;
    }
    
    return sdev->sensor_api->sample_fetch(sensor, channel);
}

xy_error_t xy_sensor_channel_get(void *sensor, xy_sensor_type_t channel,
                                xy_sensor_value_t *val)
{
    if (!sensor || !val) {
        return XY_ERROR_INVALID_PARAM;
    }
    
    xy_sensor_device_t *sdev = (xy_sensor_device_t *)sensor;
    if (!sdev->sensor_api || !sdev->sensor_api->channel_get) {
        return XY_ERROR_NOT_SUPPORTED;
    }
    
    return sdev->sensor_api->channel_get(sensor, channel, val);
}

/* ==================== 便利函数 ==================== */

int xy_device_exists(const char *name)
{
    return xy_device_find(name) != NULL ? 1 : 0;
}

xy_dev_type_t xy_device_get_type(const char *name)
{
    xy_device_t *dev = xy_device_find(name);
    return dev ? dev->type : XY_DEV_TYPE_MAX;
}

uint32_t xy_device_get_flags(const char *name)
{
    xy_device_t *dev = xy_device_find(name);
    return dev ? dev->flags : 0;
}

int xy_device_is_opened(const char *name)
{
    xy_device_t *dev = xy_device_find(name);
    return dev && (dev->state == XY_DEV_STATE_OPENED) ? 1 : 0;
}

int xy_device_get_ref_count(const char *name)
{
    xy_device_t *dev = xy_device_find(name);
    return dev ? dev->ref_count : 0;
}

/* ==================== 设备管理器 ==================== */

typedef struct {
    xy_device_t *devices[XY_DEVICE_MAX_COUNT];
    uint32_t count;
    uint32_t max_count;
} xy_device_manager_t;

static xy_device_manager_t g_dev_mgr = { 0 };

xy_error_t xy_device_mgr_init(void)
{
    memset(&g_dev_mgr, 0, sizeof(g_dev_mgr));
    g_dev_mgr.max_count = XY_DEVICE_MAX_COUNT;
    return xy_device_init();
}

xy_error_t xy_device_mgr_register(const char *group, xy_device_t *dev)
{
    XY_UNUSED(group);
    return xy_device_register(dev);
}

xy_device_t *xy_device_mgr_find_by_group(const char *group, int index)
{
    XY_UNUSED(group);
    XY_UNUSED(index);
    
    /* 简单实现：返回第一个匹配的设备 */
    return g_device_list;
}

uint32_t xy_device_mgr_get_group_count(const char *group)
{
    XY_UNUSED(group);
    
    /* 简单实现：返回所有设备数量 */
    return g_device_count;
}

#endif /* XY_DEVICE_IMPLEMENTATION */
