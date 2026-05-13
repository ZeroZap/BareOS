/**
 * @file xy_hal_uart_dev.h
 * @brief XinYi HAL UART Unified Device API
 * @version 1.0.0
 * @date 2026-03-15
 * 
 * @note 统一 UART 设备 API，适用于所有平台 (STM32/WCH/HC32)
 * @note 支持轮询/中断/DMA 三种传输模式
 */

#ifndef XY_HAL_UART_DEV_H
#define XY_HAL_UART_DEV_H
#include "xy_hal_error.h"

#include "xy_hal_uart_types.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== UART Device Handle ==================== */

/**
 * @brief UART 设备句柄 (不透明指针)
 */
typedef void *xy_hal_uart_t;

/* ==================== Device Management API ==================== */

/**
 * @brief 绑定 UART 设备
 * @param name 设备名称 (如 "USART1", "UART2")
 * @return UART 设备句柄，NULL 表示失败
 * 
 * @par 使用示例:
 * @code
 * xy_hal_uart_t uart = xy_hal_uart_bind("USART1");
 * if (!uart) {
 *     // 错误处理
 * }
 * @endcode
 */
xy_hal_uart_t xy_hal_uart_bind(const char *name);

/**
 * @brief 解绑 UART 设备
 * @param uart UART 设备句柄
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_uart_unbind(xy_hal_uart_t uart);

/* ==================== Configuration API ==================== */

/**
 * @brief 配置 UART 参数
 * @param uart UART 设备句柄
 * @param config UART 配置结构
 * @return XY_HAL_OK 成功，XY_HAL_ERROR_INVALID_PARAM 参数错误，
 *         XY_HAL_ERROR_NOT_SUPPORTED 不支持的配置
 * 
 * @par 使用示例:
 * @code
 * xy_hal_uart_config_t cfg = {
 *     .baudrate = 115200,
 *     .wordlen = XY_HAL_UART_WORDLEN_8B,
 *     .stopbits = XY_HAL_UART_STOPBITS_1,
 *     .parity = XY_HAL_UART_PARITY_NONE,
 *     .flowctrl = XY_HAL_UART_FLOWCTRL_NONE,
 *     .mode = XY_HAL_UART_MODE_TX_RX,
 *     .transfer_mode = XY_HAL_UART_TRANSFER_POLLING,
 * };
 * xy_hal_uart_configure(uart, &cfg);
 * @endcode
 */
xy_hal_error_t xy_hal_uart_configure(xy_hal_uart_t uart, 
                                     const xy_hal_uart_config_t *config);

/**
 * @brief 获取当前 UART 配置
 * @param uart UART 设备句柄
 * @param config 配置结构 (输出)
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_uart_get_config(xy_hal_uart_t uart,
                                      xy_hal_uart_config_t *config);

/**
 * @brief 设置波特率
 * @param uart UART 设备句柄
 * @param baudrate 波特率
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_uart_set_baudrate(xy_hal_uart_t uart, uint32_t baudrate);

/**
 * @brief 获取当前波特率
 * @param uart UART 设备句柄
 * @return 当前波特率，0 表示错误
 */
uint32_t xy_hal_uart_get_baudrate(xy_hal_uart_t uart);

/* ==================== Blocking Transfer API ==================== */

/**
 * @brief 发送数据 (阻塞模式)
 * @param uart UART 设备句柄
 * @param data 数据缓冲区
 * @param length 数据长度
 * @param timeout 超时时间 (毫秒)，XY_HAL_WAIT_FOREVER 表示永久等待
 * @return 实际发送字节数，负值表示错误
 * 
 * @par 使用示例:
 * @code
 * const char *msg = "Hello UART!";
 * int32_t sent = xy_hal_uart_write(uart, msg, strlen(msg), 100);
 * if (sent < 0) {
 *     // 错误处理
 * }
 * @endcode
 */
int32_t xy_hal_uart_write(xy_hal_uart_t uart, const uint8_t *data, 
                          size_t length, uint32_t timeout);

/**
 * @brief 接收数据 (阻塞模式)
 * @param uart UART 设备句柄
 * @param data 数据缓冲区
 * @param length 缓冲区长度
 * @param timeout 超时时间 (毫秒)
 * @return 实际接收字节数，0 表示超时，负值表示错误
 * 
 * @par 使用示例:
 * @code
 * char buf[64];
 * int32_t received = xy_hal_uart_read(uart, (uint8_t*)buf, sizeof(buf), 100);
 * if (received > 0) {
 *     // 处理接收到的数据
 * }
 * @endcode
 */
int32_t xy_hal_uart_read(xy_hal_uart_t uart, uint8_t *data, 
                         size_t length, uint32_t timeout);

/**
 * @brief 发送字符串 (阻塞模式)
 * @param uart UART 设备句柄
 * @param str 字符串 (以'\0'结尾)
 * @param timeout 超时时间 (毫秒)
 * @return 实际发送字节数 (不含'\0')，负值表示错误
 */
int32_t xy_hal_uart_puts(xy_hal_uart_t uart, const char *str, uint32_t timeout);

/**
 * @brief 接收一行数据 (阻塞模式，以'\n'结尾)
 * @param uart UART 设备句柄
 * @param buf 缓冲区
 * @param bufsize 缓冲区大小
 * @param timeout 超时时间 (毫秒)
 * @return 实际接收字节数 (不含'\n')，0 表示超时，负值表示错误
 */
int32_t xy_hal_uart_gets(xy_hal_uart_t uart, char *buf, 
                         size_t bufsize, uint32_t timeout);

/**
 * @brief 发送单个字符 (阻塞模式)
 * @param uart UART 设备句柄
 * @param ch 字符
 * @param timeout 超时时间 (毫秒)
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_uart_putchar(xy_hal_uart_t uart, uint8_t ch, 
                                   uint32_t timeout);

/**
 * @brief 接收单个字符 (阻塞模式)
 * @param uart UART 设备句柄
 * @param timeout 超时时间 (毫秒)
 * @return 接收到的字符，负值表示错误
 */
int32_t xy_hal_uart_getchar(xy_hal_uart_t uart, uint32_t timeout);

/**
 * @brief 等待发送完成
 * @param uart UART 设备句柄
 * @param timeout 超时时间 (毫秒)
 * @return XY_HAL_OK 发送完成，XY_HAL_ERROR_TIMEOUT 超时
 */
xy_hal_error_t xy_hal_uart_wait_tx_complete(xy_hal_uart_t uart, uint32_t timeout);

/* ==================== Non-blocking Transfer API ==================== */

/**
 * @brief 发送数据 (非阻塞模式)
 * @param uart UART 设备句柄
 * @param data 数据缓冲区
 * @param length 数据长度
 * @return 实际发送字节数，0 表示发送缓冲区满，负值表示错误
 */
int32_t xy_hal_uart_write_nb(xy_hal_uart_t uart, const uint8_t *data, 
                             size_t length);

/**
 * @brief 接收数据 (非阻塞模式)
 * @param uart UART 设备句柄
 * @param data 数据缓冲区
 * @param length 缓冲区长度
 * @return 实际接收字节数，0 表示无数据，负值表示错误
 */
int32_t xy_hal_uart_read_nb(xy_hal_uart_t uart, uint8_t *data, 
                            size_t length);

/**
 * @brief 检查是否有数据可接收
 * @param uart UART 设备句柄
 * @return 1 表示有数据，0 表示无数据
 */
int32_t xy_hal_uart_data_available(xy_hal_uart_t uart);

/**
 * @brief 检查发送缓冲区是否为空
 * @param uart UART 设备句柄
 * @return 1 表示空，0 表示忙
 */
int32_t xy_hal_uart_tx_empty(xy_hal_uart_t uart);

/* ==================== Asynchronous Transfer API (Interrupt/DMA) ==================== */

/**
 * @brief 发送数据 (异步模式)
 * @param uart UART 设备句柄
 * @param data 数据缓冲区
 * @param length 数据长度
 * @param callback 回调函数
 * @param arg 用户参数
 * @return XY_HAL_OK 成功，其他值失败
 * 
 * @par 使用示例:
 * @code
 * void tx_complete(void *uart, xy_hal_uart_event_t event, void *arg) {
 *     if (event == XY_HAL_UART_EVENT_TX_DONE) {
 *         // 发送完成
 *     }
 * }
 * 
 * xy_hal_uart_write_async(uart, data, len, tx_complete, NULL);
 * @endcode
 */
xy_hal_error_t xy_hal_uart_write_async(xy_hal_uart_t uart, const uint8_t *data,
                                       size_t length, xy_hal_uart_callback_t callback,
                                       void *arg);

/**
 * @brief 接收数据 (异步模式)
 * @param uart UART 设备句柄
 * @param data 数据缓冲区
 * @param length 缓冲区长度
 * @param callback 回调函数
 * @param arg 用户参数
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_uart_read_async(xy_hal_uart_t uart, uint8_t *data,
                                      size_t length, xy_hal_uart_callback_t callback,
                                      void *arg);

/**
 * @brief 停止异步传输
 * @param uart UART 设备句柄
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_uart_stop_async(xy_hal_uart_t uart);

/* ==================== Status and Error Handling ==================== */

/**
 * @brief 获取 UART 状态
 * @param uart UART 设备句柄
 * @param status 状态结构 (输出)
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_uart_get_status(xy_hal_uart_t uart,
                                      xy_hal_uart_status_t *status);

/**
 * @brief 获取错误标志
 * @param uart UART 设备句柄
 * @return 错误标志位
 */
xy_hal_uart_error_t xy_hal_uart_get_error(xy_hal_uart_t uart);

/**
 * @brief 清除错误标志
 * @param uart UART 设备句柄
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_uart_clear_error(xy_hal_uart_t uart);

/**
 * @brief 获取统计信息
 * @param uart UART 设备句柄
 * @param stats 统计结构 (输出)
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_uart_get_stats(xy_hal_uart_t uart,
                                     xy_hal_uart_stats_t *stats);

/**
 * @brief 重置统计信息
 * @param uart UART 设备句柄
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_uart_reset_stats(xy_hal_uart_t uart);

/* ==================== Control API ==================== */

/**
 * @brief 控制 UART 功能 (ioctl 风格)
 * @param uart UART 设备句柄
 * @param cmd 控制命令
 * @param arg 参数
 * @return XY_HAL_OK 成功，其他值失败
 * 
 * @par 支持的控制命令:
 * - XY_HAL_UART_CMD_FLUSH_TX: 清空发送缓冲区
 * - XY_HAL_UART_CMD_FLUSH_RX: 清空接收缓冲区
 * - XY_HAL_UART_CMD_BREAK: 发送 Break 信号
 * - XY_HAL_UART_CMD_ENABLE_LOOPBACK: 启用回环测试
 */
xy_hal_error_t xy_hal_uart_control(xy_hal_uart_t uart, int cmd, void *arg);

/**
 * @brief 清空发送缓冲区
 * @param uart UART 设备句柄
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_uart_flush_tx(xy_hal_uart_t uart);

/**
 * @brief 清空接收缓冲区
 * @param uart UART 设备句柄
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_uart_flush_rx(xy_hal_uart_t uart);

/**
 * @brief 发送 Break 信号
 * @param uart UART 设备句柄
 * @param duration Break 信号持续时间 (毫秒)
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_uart_send_break(xy_hal_uart_t uart, uint32_t duration);

/**
 * @brief 启用/禁用接收器
 * @param uart UART 设备句柄
 * @param enable 1=启用，0=禁用
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_uart_enable_receiver(xy_hal_uart_t uart, int enable);

/**
 * @brief 启用/禁用发送器
 * @param uart UART 设备句柄
 * @param enable 1=启用，0=禁用
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_uart_enable_transmitter(xy_hal_uart_t uart, int enable);

/* ==================== Legacy API Compatibility ==================== */

/**
 * @brief 传统 API - 初始化 UART
 * @param uart UART 实例 (平台特定)
 * @param config 配置结构
 * @return XY_HAL_OK 成功，其他值失败
 * @deprecated 请使用 xy_hal_uart_bind() + xy_hal_uart_configure()
 */
xy_hal_error_t xy_hal_uart_init(void *uart, const xy_hal_uart_config_t *config);

/**
 * @brief 传统 API - 反初始化 UART
 * @param uart UART 实例
 * @return XY_HAL_OK 成功，其他值失败
 * @deprecated 请使用 xy_hal_uart_unbind()
 */
xy_hal_error_t xy_hal_uart_deinit(void *uart);

/**
 * @brief 传统 API - 发送数据
 * @param uart UART 实例
 * @param data 数据缓冲区
 * @param len 数据长度
 * @param timeout 超时时间
 * @return 实际发送字节数，负值表示错误
 * @deprecated 请使用 xy_hal_uart_write()
 */
int32_t xy_hal_uart_transmit(void *uart, const uint8_t *data, 
                             size_t len, uint32_t timeout);

/**
 * @brief 传统 API - 接收数据
 * @param uart UART 实例
 * @param data 数据缓冲区
 * @param len 缓冲区长度
 * @param timeout 超时时间
 * @return 实际接收字节数，负值表示错误
 * @deprecated 请使用 xy_hal_uart_read()
 */
int32_t xy_hal_uart_receive(void *uart, uint8_t *data, 
                            size_t len, uint32_t timeout);

/* ==================== Control Command Definitions ==================== */

#define XY_HAL_UART_CMD_FLUSH_TX        0x01  /**< 清空发送缓冲区 */
#include "xy_hal_error.h"
#define XY_HAL_UART_CMD_FLUSH_RX        0x02  /**< 清空接收缓冲区 */
#include "xy_hal_error.h"
#define XY_HAL_UART_CMD_BREAK           0x03  /**< 发送 Break 信号 */
#include "xy_hal_error.h"
#define XY_HAL_UART_CMD_ENABLE_LOOPBACK 0x04  /**< 启用回环测试 */
#include "xy_hal_error.h"
#define XY_HAL_UART_CMD_DISABLE_LOOPBACK 0x05 /**< 禁用回环测试 */
#include "xy_hal_error.h"

#ifdef __cplusplus
}
#endif

#endif /* XY_HAL_UART_DEV_H */
