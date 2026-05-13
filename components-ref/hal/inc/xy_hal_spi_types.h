/**
 * @file xy_hal_spi_types.h
 * @brief XinYi HAL SPI Unified Type Definitions
 * @version 1.0.0
 * @date 2026-03-15
 * 
 * @note 统一 SPI 类型定义，适用于所有平台 (STM32/WCH/HC32)
 */

#ifndef XY_HAL_SPI_TYPES_H
#define XY_HAL_SPI_TYPES_H
#include "xy_hal_error.h"

#include "xy_hal.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== SPI Mode (CPOL/CPHA) ==================== */

/**
 * @brief SPI 模式 (时钟极性和相位)
 */
typedef enum {
    XY_HAL_SPI_MODE_0 = 0,         /**< CPOL=0, CPHA=0 (最常用) */
    XY_HAL_SPI_MODE_1,             /**< CPOL=0, CPHA=1 */
    XY_HAL_SPI_MODE_2,             /**< CPOL=1, CPHA=0 */
    XY_HAL_SPI_MODE_3,             /**< CPOL=1, CPHA=1 */
    XY_HAL_SPI_MODE_COUNT          /**< 用于验证 */
} xy_hal_spi_mode_t;

/* ==================== SPI Data Size ==================== */

/**
 * @brief SPI 数据大小
 */
typedef enum {
    XY_HAL_SPI_DATASIZE_8BIT = 0,  /**< 8 位数据 */
    XY_HAL_SPI_DATASIZE_16BIT,     /**< 16 位数据 */
    XY_HAL_SPI_DATASIZE_COUNT      /**< 用于验证 */
} xy_hal_spi_datasize_t;

/* ==================== SPI Bit Order ==================== */

/**
 * @brief SPI 位序
 */
typedef enum {
    XY_HAL_SPI_FIRSTBIT_MSB = 0,   /**< MSB 先行 (最常用) */
    XY_HAL_SPI_FIRSTBIT_LSB,       /**< LSB 先行 */
    XY_HAL_SPI_FIRSTBIT_COUNT      /**< 用于验证 */
} xy_hal_spi_firstbit_t;

/* ==================== SPI NSS (Chip Select) Mode ==================== */

/**
 * @brief SPI NSS (片选) 模式
 */
typedef enum {
    XY_HAL_SPI_NSS_SOFT = 0,       /**< 软件控制 NSS */
    XY_HAL_SPI_NSS_HARD_INPUT,     /**< 硬件 NSS 输入 */
    XY_HAL_SPI_NSS_HARD_OUTPUT,    /**< 硬件 NSS 输出 */
    XY_HAL_SPI_NSS_COUNT           /**< 用于验证 */
} xy_hal_spi_nss_t;

/* ==================== SPI Direction ==================== */

/**
 * @brief SPI 通信方向
 */
typedef enum {
    XY_HAL_SPI_DIR_2LINES = 0,     /**< 全双工 (2 线) */
    XY_HAL_SPI_DIR_2LINES_RXONLY,  /**< 全双工仅接收 */
    XY_HAL_SPI_DIR_1LINE,          /**< 半双工 (1 线) */
    XY_HAL_SPI_DIR_COUNT           /**< 用于验证 */
} xy_hal_spi_direction_t;

/* ==================== SPI Transfer Mode ==================== */

/**
 * @brief SPI 传输模式
 */
typedef enum {
    XY_HAL_SPI_TRANSFER_POLLING = 0, /**< 轮询模式 */
    XY_HAL_SPI_TRANSFER_INTERRUPT,   /**< 中断模式 */
    XY_HAL_SPI_TRANSFER_DMA,         /**< DMA 模式 */
} xy_hal_spi_transfer_t;

/* ==================== SPI Event Types ==================== */

/**
 * @brief SPI 事件类型
 */
typedef enum {
    XY_HAL_SPI_EVENT_TX_DONE = 0,  /**< 发送完成 */
    XY_HAL_SPI_EVENT_RX_DONE,      /**< 接收完成 */
    XY_HAL_SPI_EVENT_TX_RX_DONE,   /**< 发送接收完成 */
    XY_HAL_SPI_EVENT_ERROR,        /**< 错误发生 */
    XY_HAL_SPI_EVENT_OVR,          /**< 溢出错误 */
    XY_HAL_SPI_EVENT_MODF,         /**< 模式错误 */
    XY_HAL_SPI_EVENT_CRCERR,       /**< CRC 错误 */
    XY_HAL_SPI_EVENT_TIFRFE,       /**< TI 帧格式错误 */
} xy_hal_spi_event_t;

/* ==================== SPI Error Flags ==================== */

/**
 * @brief SPI 错误标志
 */
typedef enum {
    XY_HAL_SPI_ERROR_NONE    = 0x00, /**< 无错误 */
    XY_HAL_SPI_ERROR_OVR     = 0x01, /**< 溢出错误 */
    XY_HAL_SPI_ERROR_MODF    = 0x02, /**< 模式错误 */
    XY_HAL_SPI_ERROR_CRCERR  = 0x04, /**< CRC 错误 */
    XY_HAL_SPI_ERROR_FRE     = 0x08, /**< 帧格式错误 */
} xy_hal_spi_error_t;

/* ==================== SPI Configuration Structure ==================== */

/**
 * @brief SPI 配置结构
 */
typedef struct {
    xy_hal_spi_mode_t mode;           /**< SPI 模式 (CPOL/CPHA) */
    xy_hal_spi_direction_t direction; /**< 通信方向 */
    xy_hal_spi_datasize_t datasize;   /**< 数据大小 */
    xy_hal_spi_firstbit_t firstbit;   /**< 位序 */
    xy_hal_spi_nss_t nss;             /**< NSS 模式 */
    uint32_t baudrate_prescaler;      /**< 波特率预分频 */
    uint8_t is_master;                /**< 1=主模式，0=从模式 */
    xy_hal_spi_transfer_t transfer_mode; /**< 传输模式 */
    uint8_t crc_enable;               /**< CRC 校验使能 */
    uint8_t crc_polynomial;           /**< CRC 多项式 */
} xy_hal_spi_config_t;

/* ==================== SPI Callback Function ==================== */

/**
 * @brief SPI 回调函数类型
 * @param spi SPI 设备句柄
 * @param event 事件类型
 * @param arg 用户参数
 */
typedef void (*xy_hal_spi_callback_t)(void *spi, xy_hal_spi_event_t event, void *arg);

/* ==================== SPI Status ==================== */

/**
 * @brief SPI 状态标志
 */
typedef struct {
    uint8_t tx_busy;      /**< 发送忙 */
    uint8_t rx_busy;      /**< 接收忙 */
    uint8_t tx_complete;  /**< 发送完成 */
    uint8_t rx_available; /**< 接收数据可用 */
    uint32_t errors;      /**< 错误标志 */
} xy_hal_spi_status_t;

/* ==================== SPI Statistics ==================== */

/**
 * @brief SPI 统计信息
 */
typedef struct {
    uint32_t tx_bytes;    /**< 发送字节数 */
    uint32_t rx_bytes;    /**< 接收字节数 */
    uint32_t tx_errors;   /**< 发送错误数 */
    uint32_t rx_errors;   /**< 接收错误数 */
    uint32_t crc_errors;  /**< CRC 错误数 */
} xy_hal_spi_stats_t;

#ifdef __cplusplus
}
#endif

#endif /* XY_HAL_SPI_TYPES_H */
