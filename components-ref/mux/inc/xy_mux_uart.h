/**
 * @file xy_mux_uart.h
 * @brief MUX UART Interface
 * @version 1.0.0
 * @date 2026-03-02
 */

#ifndef XY_MUX_UART_H
#define XY_MUX_UART_H

#include "xy_mux.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief UART 配置
 */
typedef struct {
    uint32_t baudrate;      /**< 波特率 */
    uint8_t data_bits;      /**< 数据位 (5-8) */
    uint8_t stop_bits;      /**< 停止位 (1-2) */
    uint8_t parity;         /**< 校验位 (0=无，1=奇，2=偶) */
    uint8_t flow_control;   /**< 流控制 (0=无，1=RTS/CTS) */
} xy_mux_uart_config_t;

/**
 * @brief UART 命令
 */
typedef enum {
    XY_MUX_UART_CMD_SET_BAUD = 0,   /**< 设置波特率 */
    XY_MUX_UART_CMD_GET_BAUD,       /**< 获取波特率 */
    XY_MUX_UART_CMD_SET_CONFIG,     /**< 设置配置 */
    XY_MUX_UART_CMD_GET_CONFIG,     /**< 获取配置 */
    XY_MUX_UART_CMD_FLUSH,          /**< 清空缓冲区 */
    XY_MUX_UART_CMD_SET_TIMEOUT,    /**< 设置超时 */
} xy_mux_uart_cmd_t;

/**
 * @brief 注册 MUX UART
 * @param mgr MUX 管理器
 * @param channel UART 通道号
 * @param ops UART 操作接口
 * @param user_data 用户数据
 * @return XY_MUX_OK 成功，其他值失败
 */
int32_t xy_mux_uart_register(xy_mux_manager_t *mgr, uint8_t channel,
                             const xy_mux_ops_t *ops, void *user_data);

/**
 * @brief 配置 UART
 * @param mgr MUX 管理器
 * @param channel UART 通道号
 * @param config UART 配置
 * @return XY_MUX_OK 成功，其他值失败
 */
int32_t xy_mux_uart_config(xy_mux_manager_t *mgr, uint8_t channel,
                           const xy_mux_uart_config_t *config);

/**
 * @brief UART 读取
 * @param mgr MUX 管理器
 * @param channel UART 通道号
 * @param data 数据缓冲区
 * @param len 数据长度
 * @param timeout 超时时间 (ms)
 * @return 读取字节数，负值表示错误
 */
int32_t xy_mux_uart_read(xy_mux_manager_t *mgr, uint8_t channel,
                         void *data, size_t len, uint32_t timeout);

/**
 * @brief UART 写入
 * @param mgr MUX 管理器
 * @param channel UART 通道号
 * @param data 数据
 * @param len 数据长度
 * @param timeout 超时时间 (ms)
 * @return 写入字节数，负值表示错误
 */
int32_t xy_mux_uart_write(xy_mux_manager_t *mgr, uint8_t channel,
                          const void *data, size_t len, uint32_t timeout);

#ifdef __cplusplus
}
#endif

#endif
