/**
 * @file xy_hal_uart_types.h
 * @brief XinYi HAL UART Unified Type Definitions
 * @version 1.0.0
 * @date 2026-03-15
 * 
 * @note 统一 UART 类型定义，适用于所有平台 (STM32/WCH/HC32)
 */

#ifndef XY_HAL_UART_TYPES_H
#define XY_HAL_UART_TYPES_H
#include "xy_hal_error.h"

#include "xy_hal.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== UART Word Length ==================== */

/**
 * @brief UART 数据位长度
 */
typedef enum {
    XY_HAL_UART_WORDLEN_7B = 0,    /**< 7 位数据位 */
    XY_HAL_UART_WORDLEN_8B,        /**< 8 位数据位 (最常用) */
    XY_HAL_UART_WORDLEN_9B,        /**< 9 位数据位 */
    XY_HAL_UART_WORDLEN_COUNT      /**< 用于验证 */
} xy_hal_uart_wordlen_t;

/* ==================== UART Stop Bits ==================== */

/**
 * @brief UART 停止位
 */
typedef enum {
    XY_HAL_UART_STOPBITS_1 = 0,    /**< 1 位停止位 (最常用) */
    XY_HAL_UART_STOPBITS_1_5,      /**< 1.5 位停止位 */
    XY_HAL_UART_STOPBITS_2,        /**< 2 位停止位 */
    XY_HAL_UART_STOPBITS_COUNT     /**< 用于验证 */
} xy_hal_uart_stopbits_t;

/* ==================== UART Parity ==================== */

/**
 * @brief UART 校验位
 */
typedef enum {
    XY_HAL_UART_PARITY_NONE = 0,   /**< 无校验 (最常用) */
    XY_HAL_UART_PARITY_EVEN,       /**< 偶校验 */
    XY_HAL_UART_PARITY_ODD,        /**< 奇校验 */
    XY_HAL_UART_PARITY_COUNT       /**< 用于验证 */
} xy_hal_uart_parity_t;

/* ==================== UART Flow Control ==================== */

/**
 * @brief UART 流控制
 */
typedef enum {
    XY_HAL_UART_FLOWCTRL_NONE = 0, /**< 无流控制 */
    XY_HAL_UART_FLOWCTRL_RTS,      /**< RTS 流控制 */
    XY_HAL_UART_FLOWCTRL_CTS,      /**< CTS 流控制 */
    XY_HAL_UART_FLOWCTRL_RTS_CTS,  /**< RTS+CTS 流控制 */
    XY_HAL_UART_FLOWCTRL_COUNT     /**< 用于验证 */
} xy_hal_uart_flowctrl_t;

/* ==================== UART Mode ==================== */

/**
 * @brief UART 工作模式
 */
typedef enum {
    XY_HAL_UART_MODE_TX = 0x01,    /**< 仅发送 */
    XY_HAL_UART_MODE_RX = 0x02,    /**< 仅接收 */
    XY_HAL_UART_MODE_TX_RX = 0x03, /**< 发送 + 接收 (最常用) */
} xy_hal_uart_mode_t;

/* ==================== UART Event Types ==================== */

/**
 * @brief UART 事件类型
 */
typedef enum {
    XY_HAL_UART_EVENT_TX_DONE = 0, /**< 发送完成 */
    XY_HAL_UART_EVENT_RX_DONE,     /**< 接收完成 */
    XY_HAL_UART_EVENT_RX_HALF,     /**< 接收一半 (DMA 模式) */
    XY_HAL_UART_EVENT_ERROR,       /**< 错误发生 */
    XY_HAL_UART_EVENT_IDLE,        /**< 空闲线检测 */
    XY_HAL_UART_EVENT_ORE,         /**< 溢出错误 */
    XY_HAL_UART_EVENT_NE,          /**< 噪声错误 */
    XY_HAL_UART_EVENT_FE,          /**< 帧错误 */
    XY_HAL_UART_EVENT_PE,          /**< 校验错误 */
} xy_hal_uart_event_t;

/* ==================== UART Error Flags ==================== */

/**
 * @brief UART 错误标志
 */
typedef enum {
    XY_HAL_UART_ERROR_NONE    = 0x00, /**< 无错误 */
    XY_HAL_UART_ERROR_ORE     = 0x01, /**< 溢出错误 */
    XY_HAL_UART_ERROR_NE      = 0x02, /**< 噪声错误 */
    XY_HAL_UART_ERROR_FE      = 0x04, /**< 帧错误 */
    XY_HAL_UART_ERROR_PE      = 0x08, /**< 校验错误 */
    XY_HAL_UART_ERROR_PARITY  = 0x10, /**< 校验错误 (别名) */
} xy_hal_uart_error_t;

/* ==================== UART Transfer Mode ==================== */

/**
 * @brief UART 传输模式
 */
typedef enum {
    XY_HAL_UART_TRANSFER_POLLING = 0, /**< 轮询模式 */
    XY_HAL_UART_TRANSFER_INTERRUPT,   /**< 中断模式 */
    XY_HAL_UART_TRANSFER_DMA,         /**< DMA 模式 */
} xy_hal_uart_transfer_t;

/* ==================== UART Configuration Structure ==================== */

/**
 * @brief UART 配置结构
 */
typedef struct {
    uint32_t baudrate;                    /**< 波特率 (如 115200) */
    xy_hal_uart_wordlen_t wordlen;        /**< 数据位长度 */
    xy_hal_uart_stopbits_t stopbits;      /**< 停止位 */
    xy_hal_uart_parity_t parity;          /**< 校验位 */
    xy_hal_uart_flowctrl_t flowctrl;      /**< 流控制 */
    xy_hal_uart_mode_t mode;              /**< 工作模式 */
    xy_hal_uart_transfer_t transfer_mode; /**< 传输模式 */
    uint8_t oversampling;                 /**< 过采样 (8/16) */
    uint8_t swap_txrx;                    /**< 交换 TX/RX 引脚 */
    uint8_t invert_txrx;                  /**< 反相 TX/RX 信号 */
} xy_hal_uart_config_t;

/* ==================== UART Callback Function ==================== */

/**
 * @brief UART 回调函数类型
 * @param uart UART 设备句柄
 * @param event 事件类型
 * @param arg 用户参数
 */
typedef void (*xy_hal_uart_callback_t)(void *uart, xy_hal_uart_event_t event, void *arg);

/* ==================== UART Status ==================== */

/**
 * @brief UART 状态标志
 */
typedef struct {
    uint8_t tx_busy;      /**< 发送忙 */
    uint8_t rx_busy;      /**< 接收忙 */
    uint8_t tx_complete;  /**< 发送完成 */
    uint8_t rx_available; /**< 接收数据可用 */
    uint32_t errors;      /**< 错误标志 */
} xy_hal_uart_status_t;

/* ==================== UART Statistics ==================== */

/**
 * @brief UART 统计信息
 */
typedef struct {
    uint32_t tx_bytes;    /**< 发送字节数 */
    uint32_t rx_bytes;    /**< 接收字节数 */
    uint32_t tx_errors;   /**< 发送错误数 */
    uint32_t rx_errors;   /**< 接收错误数 */
    uint32_t frame_errors;/**< 帧错误数 */
    uint32_t parity_errors;/**< 校验错误数 */
} xy_hal_uart_stats_t;

#ifdef __cplusplus
}
#endif

#endif /* XY_HAL_UART_TYPES_H */
