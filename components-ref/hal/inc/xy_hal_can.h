/**
 * @file xy_hal_can.h
 * @brief XinYi CAN Hardware Abstraction Layer
 * @version 2.0
 * @date 2026-02-28
 */

#ifndef XY_HAL_CAN_H
#define XY_HAL_CAN_H
#include "xy_hal_error.h"

#include "xy_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief CAN 标准 ID 最大值
 */
#define XY_HAL_CAN_STD_ID_MAX       (0x7FF)
#include "xy_hal_error.h"

/**
 * @brief CAN 扩展 ID 最大值
 */
#define XY_HAL_CAN_EXT_ID_MAX       (0x1FFFFFFF)
#include "xy_hal_error.h"

/**
 * @brief CAN 数据长度最大值
 */
#define XY_HAL_CAN_DATA_LEN_MAX     (8)
#include "xy_hal_error.h"

/**
 * @brief CAN 波特率定义
 */
typedef enum {
    XY_HAL_CAN_BAUD_125K = 125000,    /**< 125 Kbps */
    XY_HAL_CAN_BAUD_250K = 250000,    /**< 250 Kbps */
    XY_HAL_CAN_BAUD_500K = 500000,    /**< 500 Kbps */
    XY_HAL_CAN_BAUD_1M   = 1000000,   /**< 1 Mbps */
    XY_HAL_CAN_BAUD_2M   = 2000000,   /**< 2 Mbps */
    XY_HAL_CAN_BAUD_4M   = 4000000,   /**< 4 Mbps */
    XY_HAL_CAN_BAUD_8M   = 8000000,   /**< 8 Mbps */
} xy_hal_can_baud_t;

/**
 * @brief CAN 帧类型
 */
typedef enum {
    XY_HAL_CAN_FRAME_STD = 0,        /**< 标准帧 */
    XY_HAL_CAN_FRAME_EXT,            /**< 扩展帧 */
} xy_hal_can_frame_type_t;

/**
 * @brief CAN 数据/远程帧类型
 */
typedef enum {
    XY_HAL_CAN_DATA_FRAME = 0,       /**< 数据帧 */
    XY_HAL_CAN_REMOTE_FRAME,         /**< 远程帧 */
} xy_hal_can_data_type_t;

/**
 * @brief CAN 模式
 */
typedef enum {
    XY_HAL_CAN_MODE_NORMAL = 0,      /**< 正常模式 */
    XY_HAL_CAN_MODE_LOOPBACK,        /**< 回环模式 */
    XY_HAL_CAN_MODE_SILENT,          /**< 静默模式 */
    XY_HAL_CAN_MODE_SILENT_LOOPBACK, /**< 静默回环模式 */
} xy_hal_can_mode_t;

/**
 * @brief CAN 接收 FIFO
 */
typedef enum {
    XY_HAL_CAN_FIFO_0 = 0,           /**< FIFO 0 */
    XY_HAL_CAN_FIFO_1,               /**< FIFO 1 */
} xy_hal_can_fifo_t;

/**
 * @brief CAN 中断类型
 */
typedef enum {
    XY_HAL_CAN_IT_TX_MAILBOX_EMPTY = 0,   /**< 发送邮箱空 */
    XY_HAL_CAN_IT_RX_FIFO_MSG_PENDING,    /**< 接收 FIFO 消息挂起 */
    XY_HAL_CAN_IT_RX_FIFO_OVERFLOW,       /**< 接收 FIFO 溢出 */
    XY_HAL_CAN_IT_WAKEUP,                 /**< 唤醒中断 */
    XY_HAL_CAN_IT_ERROR_WARNING,          /**< 错误警告 */
    XY_HAL_CAN_IT_ERROR_PASSIVE,          /**< 被动错误 */
    XY_HAL_CAN_IT_BUS_OFF,                /**< 总线关闭 */
    XY_HAL_CAN_IT_LAST_ERROR_CODE,        /**< 最后错误码 */
    XY_HAL_CAN_IT_ERROR,                  /**< 通用错误 */
} xy_hal_can_it_t;

/**
 * @brief CAN 滤波器模式
 */
typedef enum {
    XY_HAL_CAN_FILTERMODE_IDMASK = 0,     /**< ID 掩码模式 */
    XY_HAL_CAN_FILTERMODE_IDLIST,         /**< ID 列表模式 */
} xy_hal_can_filtermode_t;

/**
 * @brief CAN 滤波器尺度
 */
typedef enum {
    XY_HAL_CAN_FILTERSCALE_16BIT = 0,     /**< 16 位滤波器 */
    XY_HAL_CAN_FILTERSCALE_32BIT,         /**< 32 位滤波器 */
} xy_hal_can_filterscale_t;

/**
 * @brief CAN 消息结构
 */
typedef struct {
    uint32_t id;                          /**< CAN ID */
    xy_hal_can_frame_type_t frame_type;   /**< 帧类型 (标准/扩展) */
    xy_hal_can_data_type_t data_type;     /**< 数据类型 (数据/远程) */
    uint8_t dlc;                          /**< 数据长度码 (0-8) */
    uint8_t data[XY_HAL_CAN_DATA_LEN_MAX]; /**< 数据 */
    uint8_t fifo;                         /**< 接收 FIFO */
    uint32_t timestamp;                   /**< 时间戳 */
} xy_hal_can_msg_t;

/**
 * @brief CAN 配置结构
 */
typedef struct {
    xy_hal_can_baud_t baudrate;            /**< 波特率 */
    xy_hal_can_mode_t mode;                /**< 工作模式 */
    uint8_t sjw;                          /**< 同步跳跃宽度 */
    uint8_t bs1;                          /**< 时间段 1 */
    uint8_t bs2;                          /**< 时间段 2 */
    uint8_t auto_bus_off;                 /**< 自动总线关闭管理 */
    uint8_t auto_wake_up;                 /**< 自动唤醒 */
    uint8_t auto_retrans;                 /**< 自动重传 */
    uint8_t rx_fifo_locked;               /**< 接收 FIFO 锁定 */
    uint8_t tx_fifo_priority;             /**< 发送 FIFO 优先级 */
} xy_hal_can_config_t;

/**
 * @brief CAN 滤波器配置
 */
typedef struct {
    uint32_t filter_id;                   /**< 滤波器 ID */
    uint32_t filter_mask;                 /**< 滤波器掩码 */
    xy_hal_can_filtermode_t filter_mode;  /**< 滤波器模式 */
    xy_hal_can_filterscale_t filter_scale; /**< 滤波器尺度 */
    uint8_t fifo_assignment;              /**< FIFO 分配 */
    uint8_t activation;                   /**< 激活 */
    uint32_t bank_number;                 /**< 滤波器组号 */
} xy_hal_can_filter_config_t;

/**
 * @brief CAN 状态结构
 */
typedef struct {
    uint32_t error_code;                  /**< 错误码 */
    uint8_t error_state;                  /**< 错误状态 */
    uint8_t txe;                          /**< 发送邮箱空 */
    uint8_t rx0e;                         /**< 接收 FIFO 0 溢出 */
    uint8_t rx1e;                         /**< 接收 FIFO 1 溢出 */
    uint8_t wake_up;                      /**< 唤醒标志 */
    uint32_t tx_mailboxes_requests;       /**< 发送邮箱请求 */
} xy_hal_can_state_t;

/**
 * @brief CAN 事件类型
 */
typedef enum {
    XY_HAL_CAN_EVENT_TX_COMPLETE = 0,      /**< 发送完成 */
    XY_HAL_CAN_EVENT_RX_READY,             /**< 接收就绪 */
    XY_HAL_CAN_EVENT_RX_PENDING,           /**< 接收挂起 */
    XY_HAL_CAN_EVENT_ERROR,                /**< 错误事件 */
    XY_HAL_CAN_EVENT_WAKEUP,               /**< 唤醒事件 */
    XY_HAL_CAN_EVENT_TX_MAILBOX_EMPTY,     /**< 发送邮箱空 */
    XY_HAL_CAN_EVENT_FIFO_OVERFLOW,        /**< FIFO 溢出 */
    XY_HAL_CAN_EVENT_BUS_OFF,              /**< 总线关闭 */
    XY_HAL_CAN_EVENT_PASSIVE_ERROR,        /**< 被动错误 */
    XY_HAL_CAN_EVENT_WARNING,              /**< 警告 */
} xy_hal_can_evt_t;

/**
 * @brief CAN 回调类型
 */
typedef void (*xy_hal_can_callback_t)(void *can, xy_hal_can_evt_t event, void *arg);

/**
 * @brief 初始化 CAN
 * @param can CAN 实例
 * @param config 配置结构
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_can_init(void *can, const xy_hal_can_config_t *config);

/**
 * @brief 反初始化 CAN
 * @param can CAN 实例
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_can_deinit(void *can);

/**
 * @brief 启动 CAN
 * @param can CAN 实例
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_can_start(void *can);

/**
 * @brief 停止 CAN
 * @param can CAN 实例
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_can_stop(void *can);

/**
 * @brief 发送 CAN 消息
 * @param can CAN 实例
 * @param msg 消息结构
 * @param timeout 超时时间 (ms)
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_can_send(void *can, const xy_hal_can_msg_t *msg, uint32_t timeout);

/**
 * @brief 接收 CAN 消息
 * @param can CAN 实例
 * @param msg 消息结构输出
 * @param fifo FIFO 选择
 * @param timeout 超时时间 (ms)
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_can_receive(void *can, xy_hal_can_msg_t *msg, 
                                 xy_hal_can_fifo_t fifo, uint32_t timeout);

/**
 * @brief 配置 CAN 滤波器
 * @param can CAN 实例
 * @param filter_config 滤波器配置
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_can_config_filter(void *can, 
                                       const xy_hal_can_filter_config_t *filter_config);

/**
 * @brief 批量配置 CAN 滤波器
 * @param can CAN 实例
 * @param filters 滤波器配置数组
 * @param count 滤波器数量
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_can_config_filters(void *can,
                                        const xy_hal_can_filter_config_t *filters,
                                        size_t count);

/**
 * @brief 使能 CAN 中断
 * @param can CAN 实例
 * @param it 中断类型
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_can_enable_irq(void *can, xy_hal_can_it_t it);

/**
 * @brief 禁用 CAN 中断
 * @param can CAN 实例
 * @param it 中断类型
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_can_disable_irq(void *can, xy_hal_can_it_t it);

/**
 * @brief 获取 CAN 状态
 * @param can CAN 实例
 * @param state 状态结构输出
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_can_get_state(void *can, xy_hal_can_state_t *state);

/**
 * @brief 获取错误计数
 * @param can CAN 实例
 * @param tx_errors 发送错误计数输出
 * @param rx_errors 接收错误计数输出
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_can_get_error_count(void *can, uint8_t *tx_errors, 
                                         uint8_t *rx_errors);

/**
 * @brief 复位错误计数
 * @param can CAN 实例
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_can_reset_error_count(void *can);

/**
 * @brief 检查 CAN 是否就绪
 * @param can CAN 实例
 * @return 1=就绪，0=未就绪，负值表示错误
 */
int32_t xy_hal_can_is_ready(void *can);

/**
 * @brief 检查 CAN 是否处于错误状态
 * @param can CAN 实例
 * @return 1=错误状态，0=正常状态，负值表示错误
 */
int32_t xy_hal_can_is_error(void *can);

/**
 * @brief 检查总线是否关闭
 * @param can CAN 实例
 * @return 1=总线关闭，0=正常，负值表示错误
 */
int32_t xy_hal_can_is_bus_off(void *can);

/**
 * @brief 检查错误被动状态
 * @param can CAN 实例
 * @return 1=错误被动，0=正常，负值表示错误
 */
int32_t xy_hal_can_is_error_passive(void *can);

/**
 * @brief 检查错误警告状态
 * @param can CAN 实例
 * @return 1=错误警告，0=正常，负值表示错误
 */
int32_t xy_hal_can_is_error_warning(void *can);

/**
 * @brief 设置 CAN 模式
 * @param can CAN 实例
 * @param mode 模式
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_can_set_mode(void *can, xy_hal_can_mode_t mode);

/**
 * @brief 获取 CAN 模式
 * @param can CAN 实例
 * @return 模式，负值表示错误
 */
int32_t xy_hal_can_get_mode(void *can);

/**
 * @brief 设置 CAN 波特率
 * @param can CAN 实例
 * @param baud 波特率
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_can_set_baudrate(void *can, xy_hal_can_baud_t baud);

/**
 * @brief 获取 CAN 波特率
 * @param can CAN 实例
 * @return 波特率，负值表示错误
 */
int32_t xy_hal_can_get_baudrate(void *can);

/**
 * @brief 注册 CAN 回调
 * @param can CAN 实例
 * @param callback 回调函数
 * @param arg 用户参数
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_can_register_callback(void *can,
                                           xy_hal_can_callback_t callback,
                                           void *arg);

/**
 * @brief CAN 控制
 * @param can CAN 实例
 * @param cmd 控制命令
 * @param args 命令参数
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_can_control(void *can, uint32_t cmd, void *args);

/**
 * @brief 获取接收 FIFO 计数
 * @param can CAN 实例
 * @param fifo FIFO 选择
 * @return FIFO 计数，负值表示错误
 */
int32_t xy_hal_can_get_fifo_count(void *can, xy_hal_can_fifo_t fifo);

/**
 * @brief 释放接收 FIFO
 * @param can CAN 实例
 * @param fifo FIFO 选择
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_can_release_fifo(void *can, xy_hal_can_fifo_t fifo);

/**
 * @brief 获取发送邮箱状态
 * @param can CAN 实例
 * @param mailbox 邮箱号
 * @return 1=空闲，0=忙碌，负值表示错误
 */
int32_t xy_hal_can_get_tx_mailbox_state(void *can, uint8_t mailbox);

/**
 * @brief 取消发送请求
 * @param can CAN 实例
 * @param mailbox 邮箱号
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_can_abort_tx_request(void *can, uint8_t mailbox);

/**
 * @brief 发送远程帧
 * @param can CAN 实例
 * @param id CAN ID
 * @param frame_type 帧类型
 * @param fifo FIFO 选择
 * @param timeout 超时时间 (ms)
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_can_send_remote_frame(void *can, uint32_t id,
                                          xy_hal_can_frame_type_t frame_type,
                                          uint8_t fifo, uint32_t timeout);

/**
 * @brief 获取时间戳
 * @param can CAN 实例
 * @return 时间戳
 */
uint32_t xy_hal_can_get_timestamp(void *can);

/**
 * @brief 使能时间戳
 * @param can CAN 实例
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_can_enable_timestamp(void *can);

/**
 * @brief 禁用时间戳
 * @param can CAN 实例
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_can_disable_timestamp(void *can);

/**
 * @brief 获取 CAN 错误码
 * @param can CAN 实例
 * @return 错误码，负值表示错误
 */
int32_t xy_hal_can_get_error_code(void *can);

/**
 * @brief 清除 CAN 错误标志
 * @param can CAN 实例
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_can_clear_error_flags(void *can);

/**
 * @brief 使能自动重传
 * @param can CAN 实例
 * @param enable 使能标志
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_can_enable_autoretrans(void *can, uint8_t enable);

/**
 * @brief 使能自动唤醒
 * @param can CAN 实例
 * @param enable 使能标志
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_can_enable_autowakeup(void *can, uint8_t enable);

/**
 * @brief 使能总线关闭管理
 * @param can CAN 实例
 * @param enable 使能标志
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_can_enable_autobusoff(void *can, uint8_t enable);

/**
 * @brief CAN 事件回调处理
 * @param can CAN 实例
 * @param event 事件类型
 * @param arg 用户参数
 */
void xy_hal_can_event_handler(void *can, xy_hal_can_evt_t event, void *arg);

/* CAN 控制命令 */
#define XY_HAL_CAN_CMD_SET_MODE              0x01
#include "xy_hal_error.h"
#define XY_HAL_CAN_CMD_GET_MODE              0x02
#include "xy_hal_error.h"
#define XY_HAL_CAN_CMD_SET_BAUDRATE          0x03
#include "xy_hal_error.h"
#define XY_HAL_CAN_CMD_GET_BAUDRATE          0x04
#include "xy_hal_error.h"
#define XY_HAL_CAN_CMD_CONFIG_FILTER         0x05
#include "xy_hal_error.h"
#define XY_HAL_CAN_CMD_ENABLE_IRQ            0x06
#include "xy_hal_error.h"
#define XY_HAL_CAN_CMD_DISABLE_IRQ           0x07
#include "xy_hal_error.h"
#define XY_HAL_CAN_CMD_GET_STATE             0x08
#include "xy_hal_error.h"
#define XY_HAL_CAN_CMD_GET_ERROR_COUNT       0x09
#include "xy_hal_error.h"
#define XY_HAL_CAN_CMD_RESET_ERROR_COUNT     0x0A
#include "xy_hal_error.h"
#define XY_HAL_CAN_CMD_SET_CALLBACK          0x0B
#include "xy_hal_error.h"
#define XY_HAL_CAN_CMD_GET_FIFO_COUNT        0x0C
#include "xy_hal_error.h"
#define XY_HAL_CAN_CMD_RELEASE_FIFO          0x0D
#include "xy_hal_error.h"
#define XY_HAL_CAN_CMD_ABORT_TX_REQUEST      0x0E
#include "xy_hal_error.h"
#define XY_HAL_CAN_CMD_ENABLE_TIMESTAMP      0x0F
#include "xy_hal_error.h"
#define XY_HAL_CAN_CMD_DISABLE_TIMESTAMP     0x10
#include "xy_hal_error.h"
#define XY_HAL_CAN_CMD_GET_TIMESTAMP         0x11
#include "xy_hal_error.h"
#define XY_HAL_CAN_CMD_CLEAR_ERROR_FLAGS     0x12
#include "xy_hal_error.h"
#define XY_HAL_CAN_CMD_ENABLE_AUTORETRANS    0x13
#include "xy_hal_error.h"
#define XY_HAL_CAN_CMD_ENABLE_AUTOWAKEUP     0x14
#include "xy_hal_error.h"
#define XY_HAL_CAN_CMD_ENABLE_AUTOBUSOFF     0x15
#include "xy_hal_error.h"

#ifdef __cplusplus
}
#endif

#endif /* XY_HAL_CAN_H */
