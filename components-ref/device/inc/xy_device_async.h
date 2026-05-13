/**
 * @file xy_device_async.h
 * @brief Device Asynchronous Operations
 * @version 1.0.0
 * @date 2026-03-15
 * 
 * @note 设备异步操作模块 - 支持非阻塞 I/O 和回调
 */

#ifndef XY_DEVICE_ASYNC_H
#define XY_DEVICE_ASYNC_H

#include "xy_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Async Operation Types ==================== */

/**
 * @brief 异步操作类型
 */
typedef enum {
    XY_DEVICE_ASYNC_OP_NONE = 0,
    XY_DEVICE_ASYNC_OP_READ,
    XY_DEVICE_ASYNC_OP_WRITE,
    XY_DEVICE_ASYNC_OP_IOCTL,
} xy_device_async_op_t;

/**
 * @brief 异步操作状态
 */
typedef enum {
    XY_DEVICE_ASYNC_STATE_IDLE = 0,
    XY_DEVICE_ASYNC_STATE_PENDING,
    XY_DEVICE_ASYNC_STATE_COMPLETED,
    XY_DEVICE_ASYNC_STATE_ERROR,
} xy_device_async_state_t;

/* ==================== Async Callback ==================== */

/**
 * @brief 异步操作完成回调
 * @param dev 设备句柄
 * @param op 操作类型
 * @param result 操作结果
 * @param user_data 用户数据
 */
typedef void (*xy_device_async_callback_t)(xy_device_t *dev, 
                                           xy_device_async_op_t op,
                                           int result,
                                           void *user_data);

/* ==================== Async Request Structure ==================== */

/**
 * @brief 异步操作请求
 */
typedef struct {
    xy_device_async_op_t op;           /**< 操作类型 */
    xy_device_async_state_t state;     /**< 操作状态 */
    void *buffer;                      /**< 数据缓冲区 */
    size_t length;                     /**< 数据长度 */
    size_t transferred;                /**< 已传输字节数 */
    int error_code;                    /**< 错误码 */
    xy_device_async_callback_t callback; /**< 完成回调 */
    void *user_data;                   /**< 用户数据 */
    uint32_t timeout_ms;               /**< 超时时间 */
    uint32_t start_time;               /**< 开始时间 */
} xy_device_async_request_t;

/* ==================== Async Operations API ==================== */

/**
 * @brief 初始化异步操作
 * @param dev 设备句柄
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_device_async_init(xy_device_t *dev);

/**
 * @brief 异步读取
 * @param dev 设备句柄
 * @param buffer 数据缓冲区
 * @param length 数据长度
 * @param callback 完成回调
 * @param user_data 用户数据
 * @param timeout_ms 超时时间 (0=无超时)
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_device_async_read(xy_device_t *dev, void *buffer, size_t length,
                         xy_device_async_callback_t callback, void *user_data,
                         uint32_t timeout_ms);

/**
 * @brief 异步写入
 * @param dev 设备句柄
 * @param buffer 数据缓冲区
 * @param length 数据长度
 * @param callback 完成回调
 * @param user_data 用户数据
 * @param timeout_ms 超时时间 (0=无超时)
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_device_async_write(xy_device_t *dev, const void *buffer, size_t length,
                          xy_device_async_callback_t callback, void *user_data,
                          uint32_t timeout_ms);

/**
 * @brief 取消异步操作
 * @param dev 设备句柄
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_device_async_cancel(xy_device_t *dev);

/**
 * @brief 获取异步操作状态
 * @param dev 设备句柄
 * @param state 状态 (输出)
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_device_async_get_state(xy_device_t *dev, xy_device_async_state_t *state);

/**
 * @brief 获取已传输字节数
 * @param dev 设备句柄
 * @param transferred 已传输字节数 (输出)
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_device_async_get_transferred(xy_device_t *dev, size_t *transferred);

/**
 * @brief 轮询异步操作完成
 * @param dev 设备句柄
 * @return 1=完成，0=未完成，负值=错误
 */
int xy_device_async_poll(xy_device_t *dev);

/**
 * @brief 等待异步操作完成
 * @param dev 设备句柄
 * @param timeout_ms 超时时间
 * @return XY_DEVICE_OK 完成，XY_DEVICE_TIMEOUT 超时
 */
int xy_device_async_wait(xy_device_t *dev, uint32_t timeout_ms);

/**
 * @brief 检查设备是否就绪 (非阻塞)
 * @param dev 设备句柄
 * @param for_write true=检查可写，false=检查可读
 * @return 1=就绪，0=未就绪，负值=错误
 */
int xy_device_async_ready(xy_device_t *dev, bool for_write);

#ifdef __cplusplus
}
#endif

#endif /* XY_DEVICE_ASYNC_H */
