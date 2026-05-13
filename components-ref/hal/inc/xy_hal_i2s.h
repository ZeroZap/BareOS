/**
 * @file xy_hal_i2s.h
 * @brief XinYi I2S Hardware Abstraction Layer
 * @version 2.0
 * @date 2026-02-28
 */

#ifndef XY_HAL_I2S_H
#define XY_HAL_I2S_H
#include "xy_hal_error.h"

#include "xy_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief I2S 模式
 */
typedef enum {
    XY_HAL_I2S_MODE_SLAVE_TX = 0,      /**< 从机发送 */
    XY_HAL_I2S_MODE_SLAVE_RX,          /**< 从机接收 */
    XY_HAL_I2S_MODE_MASTER_TX,         /**< 主机发送 */
    XY_HAL_I2S_MODE_MASTER_RX,         /**< 主机接收 */
    XY_HAL_I2S_MODE_MASTER_FULLDUPLEX, /**< 主机全双工 */
    XY_HAL_I2S_MODE_SLAVE_FULLDUPLEX,  /**< 从机全双工 */
} xy_hal_i2s_mode_t;

/**
 * @brief I2S 标准
 */
typedef enum {
    XY_HAL_I2S_STANDARD_PHILIPS = 0,   /**< Philips 标准 */
    XY_HAL_I2S_STANDARD_MSB,           /**< MSB 对齐标准 */
    XY_HAL_I2S_STANDARD_LSB,           /**< LSB 对齐标准 */
    XY_HAL_I2S_STANDARD_PCM_SHORT,     /**< PCM 短帧 */
    XY_HAL_I2S_STANDARD_PCM_LONG,      /**< PCM 长帧 */
} xy_hal_i2s_standard_t;

/**
 * @brief I2S 数据格式
 */
typedef enum {
    XY_HAL_I2S_DATAFORMAT_16B = 0,     /**< 16 位数据 */
    XY_HAL_I2S_DATAFORMAT_16B_EXTENDED,/**< 16 位扩展 (24 位传输) */
    XY_HAL_I2S_DATAFORMAT_24B,         /**< 24 位数据 */
    XY_HAL_I2S_DATAFORMAT_32B,         /**< 32 位数据 */
} xy_hal_i2s_dataformat_t;

/**
 * @brief I2S 时钟极性
 */
typedef enum {
    XY_HAL_I2S_CPOL_LOW = 0,           /**< 时钟低电平空闲 */
    XY_HAL_I2S_CPOL_HIGH,              /**< 时钟高电平空闲 */
} xy_hal_i2s_cpol_t;

/**
 * @brief I2S 主时钟输出
 */
typedef enum {
    XY_HAL_I2S_MCLK_DISABLE = 0,       /**< 禁用主时钟输出 */
    XY_HAL_I2S_MCLK_ENABLE,            /**< 使能主时钟输出 */
} xy_hal_i2s_mckoe_t;

/**
 * @brief I2S 通信模式
 */
typedef enum {
    XY_HAL_I2S_COMMUNICATION_FULLDUPLEX = 0, /**< 全双工 */
    XY_HAL_I2S_COMMUNICATION_RXONLY,        /**< 仅接收 */
    XY_HAL_I2S_COMMUNICATION_TXONLY,        /**< 仅发送 */
    XY_HAL_I2S_COMMUNICATION_PSEUDODUPLEX,   /**< 伪双工 */
} xy_hal_i2s_communication_t;

/**
 * @brief I2S 事件类型
 */
typedef enum {
    XY_HAL_I2S_EVENT_TX_COMPLETE = 0,    /**< 发送完成 */
    XY_HAL_I2S_EVENT_RX_COMPLETE,        /**< 接收完成 */
    XY_HAL_I2S_EVENT_TX_HALF_COMPLETE,   /**< 发送半完成 */
    XY_HAL_I2S_EVENT_RX_HALF_COMPLETE,   /**< 接收半完成 */
    XY_HAL_I2S_EVENT_ERROR,              /**< 错误事件 */
    XY_HAL_I2S_EVENT_OVR,                /**< 溢出事件 */
    XY_HAL_I2S_EVENT_UDR,                /**< 欠载事件 */
    XY_HAL_I2S_EVENT_FRE,                /**< 帧错误事件 */
} xy_hal_i2s_evt_t;

/**
 * @brief I2S 配置结构
 */
typedef struct {
    xy_hal_i2s_mode_t mode;              /**< 工作模式 */
    xy_hal_i2s_standard_t standard;      /**< I2S 标准 */
    xy_hal_i2s_dataformat_t data_format; /**< 数据格式 */
    xy_hal_i2s_cpol_t cpol;              /**< 时钟极性 */
    xy_hal_i2s_mckoe_t mck_output;       /**< 主时钟输出 */
    uint32_t audio_freq;                /**< 音频频率 */
    uint32_t clock_freq;                /**< 时钟频率 */
    xy_hal_i2s_communication_t comm_mode; /**< 通信模式 */
} xy_hal_i2s_config_t;

/**
 * @brief I2S 回调类型
 */
typedef void (*xy_hal_i2s_callback_t)(void *i2s, xy_hal_i2s_evt_t event, void *arg);

/**
 * @brief I2S 初始化
 * @param i2s I2S 实例
 * @param config 配置结构
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2s_init(void *i2s, const xy_hal_i2s_config_t *config);

/**
 * @brief I2S 反初始化
 * @param i2s I2S 实例
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2s_deinit(void *i2s);

/**
 * @brief I2S 启动
 * @param i2s I2S 实例
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2s_start(void *i2s);

/**
 * @brief I2S 停止
 * @param i2s I2S 实例
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2s_stop(void *i2s);

/**
 * @brief I2S 发送数据 (阻塞)
 * @param i2s I2S 实例
 * @param data 数据缓冲区
 * @param size 数据大小 (以 16 位为单位)
 * @param timeout 超时时间 (ms)
 * @return 实际发送数量，负值表示错误
 */
int32_t xy_hal_i2s_send(void *i2s, const void *data, size_t size, uint32_t timeout);

/**
 * @brief I2S 接收数据 (阻塞)
 * @param i2s I2S 实例
 * @param data 数据缓冲区
 * @param size 缓冲区大小 (以 16 位为单位)
 * @param timeout 超时时间 (ms)
 * @return 实际接收数量，负值表示错误
 */
int32_t xy_hal_i2s_receive(void *i2s, void *data, size_t size, uint32_t timeout);

/**
 * @brief I2S 发送数据 (非阻塞，DMA)
 * @param i2s I2S 实例
 * @param data 数据缓冲区
 * @param size 数据大小 (以 16 位为单位)
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2s_send_dma(void *i2s, const void *data, size_t size);

/**
 * @brief I2S 接收数据 (非阻塞，DMA)
 * @param i2s I2S 实例
 * @param data 数据缓冲区
 * @param size 缓冲区大小 (以 16 位为单位)
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2s_receive_dma(void *i2s, void *data, size_t size);

/**
 * @brief I2S 发送数据 (非阻塞，中断)
 * @param i2s I2S 实例
 * @param data 数据缓冲区
 * @param size 数据大小 (以 16 位为单位)
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2s_send_it(void *i2s, const void *data, size_t size);

/**
 * @brief I2S 接收数据 (非阻塞，中断)
 * @param i2s I2S 实例
 * @param data 数据缓冲区
 * @param size 缓冲区大小 (以 16 位为单位)
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2s_receive_it(void *i2s, void *data, size_t size);

/**
 * @brief 设置音频频率
 * @param i2s I2S 实例
 * @param freq 音频频率 (Hz)
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2s_set_audio_freq(void *i2s, uint32_t freq);

/**
 * @brief 获取音频频率
 * @param i2s I2S 实例
 * @return 音频频率 (Hz)，负值表示错误
 */
int32_t xy_hal_i2s_get_audio_freq(void *i2s);

/**
 * @brief 设置数据格式
 * @param i2s I2S 实例
 * @param format 数据格式
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2s_set_data_format(void *i2s, xy_hal_i2s_dataformat_t format);

/**
 * @brief 获取数据格式
 * @param i2s I2S 实例
 * @return 数据格式，负值表示错误
 */
int32_t xy_hal_i2s_get_data_format(void *i2s);

/**
 * @brief 设置 I2S 模式
 * @param i2s I2S 实例
 * @param mode I2S 模式
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2s_set_mode(void *i2s, xy_hal_i2s_mode_t mode);

/**
 * @brief 获取 I2S 模式
 * @param i2s I2S 实例
 * @return I2S 模式，负值表示错误
 */
int32_t xy_hal_i2s_get_mode(void *i2s);

/**
 * @brief 设置 I2S 标准
 * @param i2s I2S 实例
 * @param standard I2S 标准
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2s_set_standard(void *i2s, xy_hal_i2s_standard_t standard);

/**
 * @brief 获取 I2S 标准
 * @param i2s I2S 实例
 * @return I2S 标准，负值表示错误
 */
int32_t xy_hal_i2s_get_standard(void *i2s);

/**
 * @brief 设置时钟极性
 * @param i2s I2S 实例
 * @param cpol 时钟极性
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2s_set_cpol(void *i2s, xy_hal_i2s_cpol_t cpol);

/**
 * @brief 获取时钟极性
 * @param i2s I2S 实例
 * @return 时钟极性，负值表示错误
 */
int32_t xy_hal_i2s_get_cpol(void *i2s);

/**
 * @brief 使能主时钟输出
 * @param i2s I2S 实例
 * @param enable 使能标志
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2s_enable_mck(void *i2s, uint8_t enable);

/**
 * @brief 获取主时钟输出状态
 * @param i2s I2S 实例
 * @return 1=使能，0=禁用，负值表示错误
 */
int32_t xy_hal_i2s_is_mck_enabled(void *i2s);

/**
 * @brief 注册 I2S 回调
 * @param i2s I2S 实例
 * @param callback 回调函数
 * @param arg 用户参数
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2s_register_callback(void *i2s,
                                           xy_hal_i2s_callback_t callback,
                                           void *arg);

/**
 * @brief I2S 控制
 * @param i2s I2S 实例
 * @param cmd 控制命令
 * @param args 命令参数
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2s_control(void *i2s, uint32_t cmd, void *args);

/**
 * @brief 获取 I2S 状态
 * @param i2s I2S 实例
 * @return 状态，负值表示错误
 */
int32_t xy_hal_i2s_get_state(void *i2s);

/**
 * @brief 获取 I2S 错误码
 * @param i2s I2S 实例
 * @return 错误码，负值表示错误
 */
int32_t xy_hal_i2s_get_error(void *i2s);

/**
 * @brief 清除 I2S 错误标志
 * @param i2s I2S 实例
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2s_clear_error(void *i2s);

/**
 * @brief I2S 传输 (发送和接收)
 * @param i2s I2S 实例
 * @param tx_data 发送数据
 * @param rx_data 接收数据
 * @param size 数据大小
 * @param timeout 超时时间 (ms)
 * @return 实际传输数量，负值表示错误
 */
int32_t xy_hal_i2s_transmit_receive(void *i2s, const void *tx_data, 
                                   void *rx_data, size_t size, uint32_t timeout);

/**
 * @brief I2S 全双工传输 (DMA)
 * @param i2s I2S 实例
 * @param tx_data 发送数据
 * @param rx_data 接收数据
 * @param size 数据大小
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2s_transmit_receive_dma(void *i2s, const void *tx_data,
                                              void *rx_data, size_t size);

/**
 * @brief I2S 休眠模式
 * @param i2s I2S 实例
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2s_enter_sleep(void *i2s);

/**
 * @brief I2S 唤醒
 * @param i2s I2S 实例
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2s_exit_sleep(void *i2s);

/**
 * @brief I2S 低功耗模式
 * @param i2s I2S 实例
 * @param low_power_mode 低功耗模式
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2s_set_low_power_mode(void *i2s, uint8_t low_power_mode);

/**
 * @brief 获取 I2S 低功耗模式
 * @param i2s I2S 实例
 * @return 低功耗模式，负值表示错误
 */
int32_t xy_hal_i2s_get_low_power_mode(void *i2s);

/**
 * @brief I2S 时钟配置
 * @param i2s I2S 实例
 * @param clock_config 时钟配置
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2s_config_clock(void *i2s, const void *clock_config);

/**
 * @brief I2S 噪声抑制
 * @param i2s I2S 实例
 * @param enable 使能标志
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2s_enable_noise_suppression(void *i2s, uint8_t enable);

/**
 * @brief I2S 回声消除
 * @param i2s I2S 实例
 * @param enable 使能标志
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2s_enable_echo_cancel(void *i2s, uint8_t enable);

/**
 * @brief I2S 自动增益控制
 * @param i2s I2S 实例
 * @param enable 使能标志
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2s_enable_agc(void *i2s, uint8_t enable);

/**
 * @brief I2S 均衡器
 * @param i2s I2S 实例
 * @param config 均衡器配置
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2s_config_equalizer(void *i2s, const void *config);

/**
 * @brief I2S 音频效果
 * @param i2s I2S 实例
 * @param effect 音频效果类型
 * @param enable 使能标志
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2s_enable_audio_effect(void *i2s, uint8_t effect, uint8_t enable);

/**
 * @brief I2S 事件回调处理
 * @param i2s I2S 实例
 * @param event 事件类型
 * @param arg 用户参数
 */
void xy_hal_i2s_event_handler(void *i2s, xy_hal_i2s_evt_t event, void *arg);

/* I2S 控制命令 */
#define XY_HAL_I2S_CMD_SET_MODE              0x01
#include "xy_hal_error.h"
#define XY_HAL_I2S_CMD_GET_MODE              0x02
#include "xy_hal_error.h"
#define XY_HAL_I2S_CMD_SET_STANDARD          0x03
#include "xy_hal_error.h"
#define XY_HAL_I2S_CMD_GET_STANDARD          0x04
#include "xy_hal_error.h"
#define XY_HAL_I2S_CMD_SET_DATA_FORMAT       0x05
#include "xy_hal_error.h"
#define XY_HAL_I2S_CMD_GET_DATA_FORMAT       0x06
#include "xy_hal_error.h"
#define XY_HAL_I2S_CMD_SET_CPOL              0x07
#include "xy_hal_error.h"
#define XY_HAL_I2S_CMD_GET_CPOL              0x08
#include "xy_hal_error.h"
#define XY_HAL_I2S_CMD_SET_MCK_OUTPUT        0x09
#include "xy_hal_error.h"
#define XY_HAL_I2S_CMD_GET_MCK_OUTPUT        0x0A
#include "xy_hal_error.h"
#define XY_HAL_I2S_CMD_SET_AUDIO_FREQ        0x0B
#include "xy_hal_error.h"
#define XY_HAL_I2S_CMD_GET_AUDIO_FREQ        0x0C
#include "xy_hal_error.h"
#define XY_HAL_I2S_CMD_SET_CLOCK_FREQ        0x0D
#include "xy_hal_error.h"
#define XY_HAL_I2S_CMD_GET_CLOCK_FREQ        0x0E
#include "xy_hal_error.h"
#define XY_HAL_I2S_CMD_CONFIG_CLOCK          0x0F
#include "xy_hal_error.h"
#define XY_HAL_I2S_CMD_SET_COMM_MODE         0x10
#include "xy_hal_error.h"
#define XY_HAL_I2S_CMD_GET_COMM_MODE         0x11
#include "xy_hal_error.h"
#define XY_HAL_I2S_CMD_ENABLE_NS             0x12
#include "xy_hal_error.h"
#define XY_HAL_I2S_CMD_ENABLE_EC             0x13
#include "xy_hal_error.h"
#define XY_HAL_I2S_CMD_ENABLE_AGC            0x14
#include "xy_hal_error.h"
#define XY_HAL_I2S_CMD_CONFIG_EQ             0x15
#include "xy_hal_error.h"
#define XY_HAL_I2S_CMD_ENABLE_EFFECT         0x16
#include "xy_hal_error.h"
#define XY_HAL_I2S_CMD_GET_STATE             0x17
#include "xy_hal_error.h"
#define XY_HAL_I2S_CMD_GET_ERROR             0x18
#include "xy_hal_error.h"
#define XY_HAL_I2S_CMD_CLEAR_ERROR           0x19
#include "xy_hal_error.h"
#define XY_HAL_I2S_CMD_ENTER_SLEEP           0x1A
#include "xy_hal_error.h"
#define XY_HAL_I2S_CMD_EXIT_SLEEP            0x1B
#include "xy_hal_error.h"
#define XY_HAL_I2S_CMD_SET_LOW_POWER_MODE    0x1C
#include "xy_hal_error.h"
#define XY_HAL_I2S_CMD_GET_LOW_POWER_MODE    0x1D
#include "xy_hal_error.h"

#ifdef __cplusplus
}
#endif

#endif /* XY_HAL_I2S_H */
