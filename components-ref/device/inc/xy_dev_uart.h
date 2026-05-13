/**
 * @file xy_dev_uart.h
 * @brief XinYi UART Device Driver API
 * @version 2.0
 * @date 2026-02-28
 */

#ifndef XY_DEV_UART_H
#define XY_DEV_UART_H

#include "xy_device.h"
#include "xy_hal_uart.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief UART 配置结构
 */
typedef struct {
    uint32_t baudrate;              /**< 波特率 */
    uint8_t wordlen;                /**< 字长 (5-9 位) */
    uint8_t stopbits;               /**< 停止位 (1-2) */
    uint8_t parity;                 /**< 校验 (0=无, 1=奇, 2=偶) */
    uint8_t flowctrl;               /**< 流控制 (0=无, 1=RTS, 2=CTS, 3=RTS+CTS) */
    uint8_t mode;                   /**< 模式 (TX/RX/双工) */
    uint8_t enable_irq;             /**< 使能中断 */
    uint8_t enable_dma;             /**< 使能 DMA */
} xy_uart_config_t;

/**
 * @brief UART 事件类型
 */
typedef enum {
    XY_UART_EVT_TX_DONE = 0,        /**< 发送完成 */
    XY_UART_EVT_RX_DONE,            /**< 接收完成 */
    XY_UART_EVT_TX_HALF_DONE,       /**< 发送半完成 */
    XY_UART_EVT_RX_HALF_DONE,       /**< 接收半完成 */
    XY_UART_EVT_ERROR,              /**< 错误事件 */
    XY_UART_EVT_OVERRUN,            /**< 溢出事件 */
    XY_UART_EVT_PARITY_ERROR,       /**< 奇偶校验错误 */
    XY_UART_EVT_FRAMING_ERROR,      /**< 帧错误 */
} xy_uart_evt_t;

/**
 * @brief UART 回调类型
 */
typedef void (*xy_uart_callback_t)(void *dev, xy_uart_evt_t event, void *arg);

/**
 * @brief UART 驱动 API 结构
 */
typedef struct {
    xy_error_t (*init)(struct xy_device *dev, const xy_uart_config_t *config);
    xy_error_t (*deinit)(struct xy_device *dev);
    int32_t (*send)(struct xy_device *dev, const uint8_t *data, size_t len, uint32_t timeout);
    int32_t (*recv)(struct xy_device *dev, uint8_t *data, size_t len, uint32_t timeout);
    xy_error_t (*flush)(struct xy_device *dev);
    xy_error_t (*set_baudrate)(struct xy_device *dev, uint32_t baudrate);
    uint32_t (*get_baudrate)(struct xy_device *dev);
    xy_error_t (*enable_irq)(struct xy_device *dev, uint8_t irq_type);
    xy_error_t (*disable_irq)(struct xy_device *dev, uint8_t irq_type);
    xy_error_t (*async_send)(struct xy_device *dev, const uint8_t *data, size_t len,
                            xy_uart_callback_t cb, void *arg);
    xy_error_t (*async_recv)(struct xy_device *dev, uint8_t *data, size_t len,
                            xy_uart_callback_t cb, void *arg);
    xy_error_t (*register_callback)(struct xy_device *dev, 
                                   xy_uart_callback_t callback, void *arg);
    xy_error_t (*set_power_mode)(struct xy_device *dev, uint8_t power_mode);
    xy_error_t (*get_power_mode)(struct xy_device *dev, uint8_t *power_mode);
} xy_uart_api_t;

/**
 * @brief 初始化 UART 设备
 * @param dev 设备指针
 * @param config 配置结构
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_uart_dev_init(void *dev, const xy_uart_config_t *config);

/**
 * @brief 发送数据
 * @param dev 设备指针
 * @param data 数据指针
 * @param len 数据长度
 * @param timeout 超时时间 (ms)
 * @return 实际发送字节数，负值表示错误
 */
int32_t xy_uart_dev_send(void *dev, const uint8_t *data, size_t len, uint32_t timeout);

/**
 * @brief 接收数据
 * @param dev 设备指针
 * @param data 数据缓冲区
 * @param len 缓冲区长度
 * @param timeout 超时时间 (ms)
 * @return 实际接收字节数，负值表示错误
 */
int32_t xy_uart_dev_recv(void *dev, uint8_t *data, size_t len, uint32_t timeout);

/**
 * @brief 异步发送数据
 * @param dev 设备指针
 * @param data 数据指针
 * @param len 数据长度
 * @param cb 回调函数
 * @param arg 用户参数
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_uart_dev_async_send(void *dev, const uint8_t *data, size_t len,
                                  xy_uart_callback_t cb, void *arg);

/**
 * @brief 异步接收数据
 * @param dev 设备指针
 * @param data 数据缓冲区
 * @param len 缓冲区长度
 * @param cb 回调函数
 * @param arg 用户参数
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_uart_dev_async_recv(void *dev, uint8_t *data, size_t len,
                                  xy_uart_callback_t cb, void *arg);

/**
 * @brief 设置波特率
 * @param dev 设备指针
 * @param baudrate 波特率
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_uart_dev_set_baudrate(void *dev, uint32_t baudrate);

/**
 * @brief 获取波特率
 * @param dev 设备指针
 * @return 波特率值，负值表示错误
 */
int32_t xy_uart_dev_get_baudrate(void *dev);

/**
 * @brief 刷新 UART 缓冲区
 * @param dev 设备指针
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_uart_dev_flush(void *dev);

/**
 * @brief 使能 UART 中断
 * @param dev 设备指针
 * @param irq_type 中断类型
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_uart_dev_enable_irq(void *dev, uint8_t irq_type);

/**
 * @brief 禁用 UART 中断
 * @param dev 设备指针
 * @param irq_type 中断类型
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_uart_dev_disable_irq(void *dev, uint8_t irq_type);

/**
 * @brief 注册 UART 回调
 * @param dev 设备指针
 * @param callback 回调函数
 * @param arg 用户参数
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_uart_dev_register_callback(void *dev, 
                                        xy_uart_callback_t callback, 
                                        void *arg);

/**
 * @brief UART 设备控制
 * @param dev 设备指针
 * @param cmd 控制命令
 * @param args 命令参数
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_uart_dev_control(void *dev, uint32_t cmd, void *args);

/* UART 控制命令 */
#define XY_UART_CMD_SET_CONFIG        0x01
#define XY_UART_CMD_GET_CONFIG        0x02
#define XY_UART_CMD_FLUSH             0x03
#define XY_UART_CMD_SET_BAUDRATE      0x04
#define XY_UART_CMD_GET_BAUDRATE      0x05
#define XY_UART_CMD_SET_CALLBACK      0x06
#define XY_UART_CMD_GET_STATE         0x07
#define XY_UART_CMD_RESET             0x08
#define XY_UART_CMD_ENABLE_RX         0x09
#define XY_UART_CMD_DISABLE_RX        0x0A
#define XY_UART_CMD_ENABLE_TX         0x0B
#define XY_UART_CMD_DISABLE_TX        0x0C
#define XY_UART_CMD_SET_POWER_MODE    0x0D
#define XY_UART_CMD_GET_POWER_MODE    0x0E

#ifdef __cplusplus
}
#endif

#endif /* XY_DEV_UART_H */
