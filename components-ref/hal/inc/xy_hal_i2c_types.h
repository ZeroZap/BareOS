/**
 * @file xy_hal_i2c_types.h
 * @brief XinYi HAL I2C Unified Type Definitions
 * @version 1.0.0
 * @date 2026-03-15
 * 
 * @note 统一 I2C 类型定义，适用于所有平台 (STM32/WCH/HC32)
 */

#ifndef XY_HAL_I2C_TYPES_H
#define XY_HAL_I2C_TYPES_H
#include "xy_hal_error.h"

#include "xy_hal.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== I2C Addressing Mode ==================== */

/**
 * @brief I2C 寻址模式
 */
typedef enum {
    XY_HAL_I2C_ADDR_7BIT = 0,      /**< 7 位地址 (最常用) */
    XY_HAL_I2C_ADDR_10BIT,         /**< 10 位地址 */
    XY_HAL_I2C_ADDR_COUNT          /**< 用于验证 */
} xy_hal_i2c_addr_mode_t;

/* ==================== I2C Duty Cycle ==================== */

/**
 * @brief I2C 占空比 (快速模式)
 */
typedef enum {
    XY_HAL_I2C_DUTY_2 = 0,         /**< Tlow/Thigh = 2 */
    XY_HAL_I2C_DUTY_16_9,          /**< Tlow/Thigh = 16/9 */
    XY_HAL_I2C_DUTY_COUNT          /**< 用于验证 */
} xy_hal_i2c_duty_t;

/* ==================== I2C Transfer Mode ==================== */

/**
 * @brief I2C 传输模式
 */
typedef enum {
    XY_HAL_I2C_TRANSFER_POLLING = 0, /**< 轮询模式 */
    XY_HAL_I2C_TRANSFER_INTERRUPT,   /**< 中断模式 */
    XY_HAL_I2C_TRANSFER_DMA,         /**< DMA 模式 */
} xy_hal_i2c_transfer_t;

/* ==================== I2C Event Types ==================== */

/**
 * @brief I2C 事件类型
 */
typedef enum {
    XY_HAL_I2C_EVENT_TX_DONE = 0,  /**< 发送完成 */
    XY_HAL_I2C_EVENT_RX_DONE,      /**< 接收完成 */
    XY_HAL_I2C_EVENT_TRANSFER_COMPLETE, /**< 传输完成 */
    XY_HAL_I2C_EVENT_ERROR,        /**< 错误发生 */
    XY_HAL_I2C_EVENT_AF,           /**< 应答失败 (NACK) */
    XY_HAL_I2C_EVENT_OVR,          /**< 溢出错误 */
    XY_HAL_I2C_EVENT_ARLO,         /**< 仲裁丢失 */
    XY_HAL_I2C_EVENT_BERR,         /**< 总线错误 */
    XY_HAL_I2C_EVENT_TIMEOUT,      /**< 超时错误 */
} xy_hal_i2c_event_t;

/* ==================== I2C Error Flags ==================== */

/**
 * @brief I2C 错误标志
 */
typedef enum {
    XY_HAL_I2C_ERROR_NONE    = 0x00, /**< 无错误 */
    XY_HAL_I2C_ERROR_AF      = 0x01, /**< 应答失败 */
    XY_HAL_I2C_ERROR_OVR     = 0x02, /**< 溢出错误 */
    XY_HAL_I2C_ERROR_ARLO    = 0x04, /**< 仲裁丢失 */
    XY_HAL_I2C_ERROR_BERR    = 0x08, /**< 总线错误 */
    XY_HAL_I2C_ERROR_TIMEOUT = 0x10, /**< 超时错误 */
} xy_hal_i2c_error_t;

/* ==================== I2C Configuration Structure ==================== */

/**
 * @brief I2C 配置结构
 */
typedef struct {
    uint32_t clock_speed;             /**< 时钟速度 (Hz)，如 100000=100kHz, 400000=400kHz */
    xy_hal_i2c_addr_mode_t addr_mode; /**< 寻址模式 */
    xy_hal_i2c_duty_t duty_cycle;     /**< 占空比 */
    uint16_t own_address;             /**< 自身地址 (从模式) */
    uint8_t general_call_mode;        /**< 通用调用使能 */
    xy_hal_i2c_transfer_t transfer_mode; /**< 传输模式 */
    uint8_t stretch_clock;            /**< 时钟拉伸使能 */
    uint8_t analog_filter;            /**< 模拟滤波器使能 */
    uint8_t digital_filter;           /**< 数字滤波器使能 */
} xy_hal_i2c_config_t;

/* ==================== I2C Callback Function ==================== */

/**
 * @brief I2C 回调函数类型
 * @param i2c I2C 设备句柄
 * @param event 事件类型
 * @param arg 用户参数
 */
typedef void (*xy_hal_i2c_callback_t)(void *i2c, xy_hal_i2c_event_t event, void *arg);

/* ==================== I2C Status ==================== */

/**
 * @brief I2C 状态标志
 */
typedef struct {
    uint8_t tx_busy;      /**< 发送忙 */
    uint8_t rx_busy;      /**< 接收忙 */
    uint8_t bus_busy;     /**< 总线忙 */
    uint8_t tx_complete;  /**< 发送完成 */
    uint8_t rx_available; /**< 接收数据可用 */
    uint32_t errors;      /**< 错误标志 */
} xy_hal_i2c_status_t;

/* ==================== I2C Statistics ==================== */

/**
 * @brief I2C 统计信息
 */
typedef struct {
    uint32_t tx_bytes;    /**< 发送字节数 */
    uint32_t rx_bytes;    /**< 接收字节数 */
    uint32_t tx_errors;   /**< 发送错误数 */
    uint32_t rx_errors;   /**< 接收错误数 */
    uint32_t nack_count;  /**< NACK 计数 */
    uint32_t retries;     /**< 重试次数 */
} xy_hal_i2c_stats_t;

/* ==================== I2C Transfer Structure ==================== */

/**
 * @brief I2C 传输描述结构 (用于复杂传输)
 */
typedef struct {
    uint16_t dev_addr;        /**< 设备地址 */
    uint8_t *buffer;          /**< 数据缓冲区 */
    size_t size;              /**< 数据大小 */
    uint8_t *reg_addr;        /**< 寄存器地址 (可选) */
    uint8_t reg_size;         /**< 寄存器地址大小 */
    uint8_t send_stop;        /**< 发送停止条件 */
} xy_hal_i2c_transfer_t;

#ifdef __cplusplus
}
#endif

#endif /* XY_HAL_I2C_TYPES_H */
