/**
 * @file xy_hal_ir.h
 * @brief IR (Infrared Remote Control) Hardware Abstraction Layer
 * @version 2.0
 * @date 2026-02-27
 */

#ifndef XY_HAL_IR_H
#define XY_HAL_IR_H
#include "xy_hal_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_hal.h"
#include <stdint.h>

/**
 * @brief IR 协议类型
 */
typedef enum {
    XY_HAL_IR_PROTOCOL_NEC = 0,     /**< NEC 协议 */
    XY_HAL_IR_PROTOCOL_RC5,         /**< RC5 协议 */
    XY_HAL_IR_PROTOCOL_RC6,         /**< RC6 协议 */
    XY_HAL_IR_PROTOCOL_SONY,        /**< Sony 协议 */
    XY_HAL_IR_PROTOCOL_SAMSUNG,     /**< Samsung 协议 */
    XY_HAL_IR_PROTOCOL_UNKNOWN,     /**< 未知协议 */
} xy_hal_ir_protocol_t;

/**
 * @brief IR 工作模式
 */
typedef enum {
    XY_HAL_IR_MODE_RECEIVER = 0,    /**< 接收模式 */
    XY_HAL_IR_MODE_TRANSMITTER,     /**< 发射模式 */
} xy_hal_ir_mode_t;

/**
 * @brief IR 接收配置
 */
typedef struct {
    xy_hal_ir_protocol_t protocol;  /**< 协议类型 */
    uint32_t carrier_freq;          /**< 载波频率 (Hz)，通常 38kHz */
    uint8_t enable_auto_repeat;     /**< 使能自动重复 */
} xy_hal_ir_rx_config_t;

/**
 * @brief IR 发射配置
 */
typedef struct {
    xy_hal_ir_protocol_t protocol;  /**< 协议类型 */
    uint32_t carrier_freq;          /**< 载波频率 (Hz) */
    uint32_t duty_cycle;            /**< 占空比 (0-10000) */
} xy_hal_ir_tx_config_t;

/**
 * @brief IR 接收数据结构
 */
typedef struct {
    uint32_t address;           /**< 地址码 */
    uint32_t command;           /**< 命令码 */
    uint32_t raw_data;          /**< 原始数据 */
    uint8_t repeat;             /**< 重复标志 */
    uint8_t valid;              /**< 有效标志 */
    xy_hal_ir_protocol_t protocol; /**< 协议类型 */
} xy_hal_ir_rx_data_t;

/**
 * @brief IR 事件类型
 */
typedef enum {
    XY_HAL_IR_EVENT_RX_COMPLETE = 0,    /**< 接收完成 */
    XY_HAL_IR_EVENT_RX_TIMEOUT,         /**< 接收超时 */
    XY_HAL_IR_EVENT_TX_COMPLETE,        /**< 发射完成 */
    XY_HAL_IR_EVENT_ERROR,              /**< 错误 */
} xy_hal_ir_event_t;

/**
 * @brief IR 回调类型
 */
typedef void (*xy_hal_ir_callback_t)(xy_hal_ir_event_t event, void *arg);

/**
 * @brief IR 句柄
 */
typedef struct {
    void *instance;                 /**< IR 实例 */
    xy_hal_ir_mode_t mode;          /**< 工作模式 */
    uint8_t initialized;            /**< 初始化标志 */
} xy_hal_ir_handle_t;

/**
 * @brief 初始化 IR 接收
 * @param handle IR 句柄
 * @param config IR 接收配置
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_ir_rx_init(xy_hal_ir_handle_t *handle,
                                 const xy_hal_ir_rx_config_t *config);

/**
 * @brief 初始化 IR 发射
 * @param handle IR 句柄
 * @param config IR 发射配置
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_ir_tx_init(xy_hal_ir_handle_t *handle,
                                 const xy_hal_ir_tx_config_t *config);

/**
 * @brief 反初始化 IR
 * @param handle IR 句柄
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_ir_deinit(xy_hal_ir_handle_t *handle);

/**
 * @brief 使能 IR 接收
 * @param handle IR 句柄
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_ir_rx_enable(xy_hal_ir_handle_t *handle);

/**
 * @brief 禁用 IR 接收
 * @param handle IR 句柄
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_ir_rx_disable(xy_hal_ir_handle_t *handle);

/**
 * @brief 接收 IR 数据 (阻塞)
 * @param handle IR 句柄
 * @param data 接收数据输出
 * @param timeout 超时时间 (ms)
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_ir_receive(xy_hal_ir_handle_t *handle,
                                 xy_hal_ir_rx_data_t *data,
                                 uint32_t timeout);

/**
 * @brief 接收 IR 数据 (非阻塞)
 * @param handle IR 句柄
 * @param data 接收数据输出
 * @return XY_HAL_OK 成功，XY_HAL_ERROR_BUSY 无数据，其他值失败
 */
xy_hal_error_t xy_hal_ir_receive_nb(xy_hal_ir_handle_t *handle,
                                    xy_hal_ir_rx_data_t *data);

/**
 * @brief 发送 NEC 协议数据
 * @param handle IR 句柄
 * @param address 地址码
 * @param command 命令码
 * @param timeout 超时时间 (ms)
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_ir_send_nec(xy_hal_ir_handle_t *handle,
                                  uint16_t address, uint8_t command,
                                  uint32_t timeout);

/**
 * @brief 发送 RC5 协议数据
 * @param handle IR 句柄
 * @param address 地址码
 * @param command 命令码
 * @param timeout 超时时间 (ms)
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_ir_send_rc5(xy_hal_ir_handle_t *handle,
                                  uint8_t address, uint8_t command,
                                  uint32_t timeout);

/**
 * @brief 发送 Sony 协议数据
 * @param handle IR 句柄
 * @param data 数据
 * @param bits 位数 (12/15/20)
 * @param timeout 超时时间 (ms)
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_ir_send_sony(xy_hal_ir_handle_t *handle,
                                   uint32_t data, uint8_t bits,
                                   uint32_t timeout);

/**
 * @brief 发送原始 IR 数据
 * @param handle IR 句柄
 * @param timings 时间序列 (微秒)
 * @param count 时间序列数量
 * @param timeout 超时时间 (ms)
 * @return XY_HAL_OK 成功，其他值失败
 *
 * @note timings 数组格式：[高电平时间，低电平时间，高电平时间，低电平时间，...]
 */
xy_hal_error_t xy_hal_ir_send_raw(xy_hal_ir_handle_t *handle,
                                  const uint32_t *timings, size_t count,
                                  uint32_t timeout);

/**
 * @brief 注册 IR 回调
 * @param handle IR 句柄
 * @param callback 回调函数
 * @param arg 用户参数
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_ir_register_callback(xy_hal_ir_handle_t *handle,
                                           xy_hal_ir_callback_t callback,
                                           void *arg);

/**
 * @brief 获取接收到的原始数据
 * @param handle IR 句柄
 * @param buffer 缓冲区
 * @param size 缓冲区大小
 * @return 实际读取的数据数量，负值表示错误
 */
int xy_hal_ir_get_raw_data(xy_hal_ir_handle_t *handle, uint32_t *buffer, size_t size);

/**
 * @brief 清除 IR 接收缓冲区
 * @param handle IR 句柄
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_ir_clear_buffer(xy_hal_ir_handle_t *handle);

/**
 * @brief 解码 NEC 协议原始数据
 * @param raw_data 原始数据
 * @param address 地址码输出
 * @param command 命令码输出
 * @param repeat 重复标志输出
 * @return 0 成功，-1 失败
 */
int xy_hal_ir_decode_nec(uint32_t raw_data, uint16_t *address,
                         uint8_t *command, uint8_t *repeat);

/**
 * @brief 开始载波输出
 * @param handle IR 句柄
 * @param frequency 载波频率 (Hz)
 * @param duty_cycle 占空比 (0-10000)
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_ir_carrier_start(xy_hal_ir_handle_t *handle,
                                       uint32_t frequency, uint32_t duty_cycle);

/**
 * @brief 停止载波输出
 * @param handle IR 句柄
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_ir_carrier_stop(xy_hal_ir_handle_t *handle);

#ifdef __cplusplus
}
#endif

#endif /* XY_HAL_IR_H */
