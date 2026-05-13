/**
 * @file xy_device_async.c
 * @brief Device Asynchronous Operations Implementation
 * @version 1.0.0
 * @date 2026-03-15
 * 
 * @note 设备异步操作实现 - 支持非阻塞 I/O 和回调
 */

#include "inc/xy_device_async.h"
#include "inc/xy_device.h"
#include "xy_os.h"
#include <string.h>

/* ==================== Private Types ==================== */

/**
 * @brief 设备异步操作私有数据
 */
typedef struct {
    xy_device_async_request_t request;   /**< 当前请求 */
    bool initialized;                    /**< 是否已初始化 */
    bool is_busy;                        /**< 是否忙碌 */
} xy_device_async_data_t;

/* ==================== Private Variables ==================== */

/* 简单实现：使用静态数组存储异步数据 */
static xy_device_async_data_t async_data[16];

/* ==================== Private Functions ==================== */

/**
 * @brief 查找设备的异步数据
 */
static xy_device_async_data_t *async_find_data(xy_device_t *dev)
{
    /* 简单实现：使用 user_data 存储指针 */
    return (xy_device_async_data_t *)dev->user_data;
}

/**
 * @brief 获取当前时间戳 (毫秒)
 * @return uint32_t 当前时间戳 (毫秒)
 */
static uint32_t async_get_tick_ms(void)
{
    return xy_os_time_get_ms();
}

__attribute__((weak)) uint32_t async_get_tick_ms(void)
{
    return xy_os_time_get_ms();
}

/**
 * @brief 检查请求是否超时
 */
static bool async_check_timeout(xy_device_async_request_t *req)
{
    if (req->timeout_ms == 0) {
        return false; /* 无超时 */
    }
    
    uint32_t now = async_get_tick_ms();
    uint32_t elapsed = now - req->start_time;
    
    return (elapsed >= req->timeout_ms);
}

/**
 * @brief 完成异步请求
 */
static void async_complete_request(xy_device_async_data_t *data, int result)
{
    if (!data || !data->initialized) {
        return;
    }
    
    xy_device_async_request_t *req = &data->request;
    
    /* 更新状态 */
    if (result >= 0) {
        req->state = XY_DEVICE_ASYNC_STATE_COMPLETED;
        req->transferred = (size_t)result;
        req->error_code = 0;
    } else {
        req->state = XY_DEVICE_ASYNC_STATE_ERROR;
        req->error_code = result;
    }
    
    data->is_busy = false;
    
    /* 调用回调 */
    if (req->callback) {
        req->callback(NULL, req->op, result, req->user_data);
    }
}

/* ==================== Public Implementation ==================== */

int xy_device_async_init(xy_device_t *dev)
{
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    /* 查找或分配异步数据 */
    xy_device_async_data_t *data = async_find_data(dev);
    
    if (!data) {
        /* 分配第一个可用槽位 */
        for (int i = 0; i < 16; i++) {
            if (!async_data[i].initialized) {
                data = &async_data[i];
                memset(data, 0, sizeof(*data));
                dev->user_data = data;
                break;
            }
        }
        
        if (!data) {
            return XY_DEVICE_NO_MEM;
        }
    }
    
    /* 初始化异步数据 */
    memset(data, 0, sizeof(*data));
    data->initialized = true;
    data->is_busy = false;
    
    return XY_DEVICE_OK;
}

int xy_device_async_read(xy_device_t *dev, void *buffer, size_t length,
                         xy_device_async_callback_t callback, void *user_data,
                         uint32_t timeout_ms)
{
    if (!dev || !buffer || length == 0) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    xy_device_async_data_t *data = async_find_data(dev);
    
    if (!data || !data->initialized) {
        return XY_DEVICE_NOT_INIT;
    }
    
    if (data->is_busy) {
        return XY_DEVICE_BUSY;
    }
    
    /* 设置请求 */
    xy_device_async_request_t *req = &data->request;
    req->op = XY_DEVICE_ASYNC_OP_READ;
    req->state = XY_DEVICE_ASYNC_STATE_PENDING;
    req->buffer = buffer;
    req->length = length;
    req->transferred = 0;
    req->error_code = 0;
    req->callback = callback;
    req->user_data = user_data;
    req->timeout_ms = timeout_ms;
    req->start_time = async_get_tick_ms();
    
    data->is_busy = true;
    
    /* 启动实际的异步读取操作 */
    /* 调用设备驱动的异步读取回调 */
    if (dev->ops && dev->ops->read_async) {
        int ret = dev->ops->read_async(dev, buffer, length, callback, user_data, timeout_ms);
        if (ret != XY_DEVICE_OK) {
            data->is_busy = false;
            return ret;
        }
    }
    /* 如果设备不支持异步读取，标记为立即完成 */
    else {
        req->state = XY_DEVICE_ASYNC_STATE_COMPLETED;
        req->transferred = 0; /* 实际读取由轮询完成 */
    }
    
    return XY_DEVICE_OK;
}

int xy_device_async_write(xy_device_t *dev, const void *buffer, size_t length,
                          xy_device_async_callback_t callback, void *user_data,
                          uint32_t timeout_ms)
{
    if (!dev || !buffer || length == 0) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    xy_device_async_data_t *data = async_find_data(dev);
    
    if (!data || !data->initialized) {
        return XY_DEVICE_NOT_INIT;
    }
    
    if (data->is_busy) {
        return XY_DEVICE_BUSY;
    }
    
    /* 设置请求 */
    xy_device_async_request_t *req = &data->request;
    req->op = XY_DEVICE_ASYNC_OP_WRITE;
    req->state = XY_DEVICE_ASYNC_STATE_PENDING;
    req->buffer = (void *)buffer;
    req->length = length;
    req->transferred = 0;
    req->error_code = 0;
    req->callback = callback;
    req->user_data = user_data;
    req->timeout_ms = timeout_ms;
    req->start_time = async_get_tick_ms();
    
    data->is_busy = true;
    
    /* 启动实际的异步写入操作 */
    /* 调用设备驱动的异步写入回调 */
    if (dev->ops && dev->ops->write_async) {
        int ret = dev->ops->write_async(dev, buffer, length, callback, user_data, timeout_ms);
        if (ret != XY_DEVICE_OK) {
            data->is_busy = false;
            return ret;
        }
    }
    /* 如果设备不支持异步写入，标记为立即完成 */
    else {
        req->state = XY_DEVICE_ASYNC_STATE_COMPLETED;
        req->transferred = 0; /* 实际写入由轮询完成 */
    }
    
    return XY_DEVICE_OK;
}

int xy_device_async_cancel(xy_device_t *dev)
{
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    xy_device_async_data_t *data = async_find_data(dev);
    
    if (!data || !data->initialized) {
        return XY_DEVICE_NOT_INIT;
    }
    
    if (!data->is_busy) {
        return XY_DEVICE_OK; /* 没有进行中的操作 */
    }
    
    /* 取消操作 */
    data->is_busy = false;
    data->request.state = XY_DEVICE_ASYNC_STATE_ERROR;
    data->request.error_code = XY_DEVICE_BUSY; /* 使用 BUSY 表示取消 */
    
    /* 调用回调通知取消 */
    if (data->request.callback) {
        data->request.callback(dev, data->request.op, 
                              XY_DEVICE_BUSY, 
                              data->request.user_data);
    }
    
    return XY_DEVICE_OK;
}

int xy_device_async_get_state(xy_device_t *dev, xy_device_async_state_t *state)
{
    if (!dev || !state) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    xy_device_async_data_t *data = async_find_data(dev);
    
    if (!data || !data->initialized) {
        return XY_DEVICE_NOT_INIT;
    }
    
    *state = data->request.state;
    return XY_DEVICE_OK;
}

int xy_device_async_get_transferred(xy_device_t *dev, size_t *transferred)
{
    if (!dev || !transferred) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    xy_device_async_data_t *data = async_find_data(dev);
    
    if (!data || !data->initialized) {
        return XY_DEVICE_NOT_INIT;
    }
    
    *transferred = data->request.transferred;
    return XY_DEVICE_OK;
}

int xy_device_async_poll(xy_device_t *dev)
{
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    xy_device_async_data_t *data = async_find_data(dev);
    
    if (!data || !data->initialized) {
        return XY_DEVICE_NOT_INIT;
    }
    
    if (!data->is_busy) {
        return 0; /* 没有进行中的操作 */
    }
    
    /* 检查超时 */
    if (async_check_timeout(&data->request)) {
        async_complete_request(data, XY_DEVICE_TIMEOUT);
        return 1; /* 完成 (超时) */
    }
    
    /* 检查底层驱动是否完成操作 */
    /* 如果设备驱动支持轮询，调用它 */
    if (dev->ops && dev->ops->poll_async) {
        int ret = dev->ops->poll_async(dev);
        if (ret != 0) {
            /* 驱动报告完成 */
            async_complete_request(data, ret > 0 ? ret : XY_DEVICE_OK);
            return 1;
        }
    }
    /* 否则假设操作已完成 (简单实现) */
    else {
        data->request.state = XY_DEVICE_ASYNC_STATE_COMPLETED;
        data->request.transferred = data->request.length;
        async_complete_request(data, data->request.length);
        return 1;
    }
    
    return 0; /* 未完成 */
}

int xy_device_async_wait(xy_device_t *dev, uint32_t timeout_ms)
{
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    xy_device_async_data_t *data = async_find_data(dev);
    
    if (!data || !data->initialized) {
        return XY_DEVICE_NOT_INIT;
    }
    
    if (!data->is_busy) {
        return XY_DEVICE_OK; /* 没有进行中的操作 */
    }
    
    uint32_t start = async_get_tick_ms();
    
    while (data->is_busy) {
        /* 轮询检查 */
        int ret = xy_device_async_poll(dev);
        if (ret != 0) {
            /* 完成或错误 */
            return (data->request.state == XY_DEVICE_ASYNC_STATE_COMPLETED) ?
                   XY_DEVICE_OK : XY_DEVICE_TIMEOUT;
        }
        
        /* 检查总超时 */
        uint32_t elapsed = async_get_tick_ms() - start;
        if (elapsed >= timeout_ms) {
            /* 超时，取消操作 */
            xy_device_async_cancel(dev);
            return XY_DEVICE_TIMEOUT;
        }
        
        /* 短暂延迟，避免忙等 */
        xy_os_time_delay_us(100);  /* 100us 延迟 */
    }
    
    return XY_DEVICE_OK;
}

int xy_device_async_ready(xy_device_t *dev, bool for_write)
{
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    /* 设备就绪检查 */
    /* 如果设备驱动支持，调用它 */
    if (dev->ops && dev->ops->ready) {
        return dev->ops->ready(dev, for_write);
    }
    
    /* 默认：如果异步操作不忙，则认为就绪 */
    xy_device_async_data_t *data = async_find_data(dev);
    if (data && data->initialized) {
        return data->is_busy ? 0 : 1;
    }
    
    return 1; /* 默认就绪 */
}

/* ==================== End of File ==================== */
