/**
 * @file xy_hal_spi_dev.h
 * @brief XinYi HAL SPI Unified Device API
 * @version 1.0.0
 * @date 2026-03-15
 * 
 * @note 统一 SPI 设备 API，适用于所有平台 (STM32/WCH/HC32)
 * @note 支持主/从模式，轮询/中断/DMA 传输
 */

#ifndef XY_HAL_SPI_DEV_H
#define XY_HAL_SPI_DEV_H
#include "xy_hal_error.h"

#include "xy_hal_spi_types.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== SPI Device Handle ==================== */

/**
 * @brief SPI 设备句柄 (不透明指针)
 */
typedef void *xy_hal_spi_t;

/* ==================== Device Management API ==================== */

/**
 * @brief 绑定 SPI 设备
 * @param name 设备名称 (如 "SPI1", "SPI2")
 * @return SPI 设备句柄，NULL 表示失败
 */
xy_hal_spi_t xy_hal_spi_bind(const char *name);

/**
 * @brief 解绑 SPI 设备
 * @param spi SPI 设备句柄
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_spi_unbind(xy_hal_spi_t spi);

/* ==================== Configuration API ==================== */

/**
 * @brief 配置 SPI 参数
 * @param spi SPI 设备句柄
 * @param config SPI 配置结构
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_spi_configure(xy_hal_spi_t spi,
                                    const xy_hal_spi_config_t *config);

/**
 * @brief 获取当前 SPI 配置
 * @param spi SPI 设备句柄
 * @param config 配置结构 (输出)
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_spi_get_config(xy_hal_spi_t spi,
                                     xy_hal_spi_config_t *config);

/**
 * @brief 设置波特率预分频
 * @param spi SPI 设备句柄
 * @param prescaler 预分频值
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_spi_set_baudrate(xy_hal_spi_t spi, uint32_t prescaler);

/* ==================== Blocking Transfer API ==================== */

/**
 * @brief 发送数据 (阻塞模式)
 * @param spi SPI 设备句柄
 * @param tx_data 发送数据缓冲区
 * @param length 数据长度
 * @param timeout 超时时间 (毫秒)
 * @return 实际发送字节数，负值表示错误
 */
int32_t xy_hal_spi_send(xy_hal_spi_t spi, const uint8_t *tx_data,
                        size_t length, uint32_t timeout);

/**
 * @brief 接收数据 (阻塞模式)
 * @param spi SPI 设备句柄
 * @param rx_data 接收数据缓冲区
 * @param length 数据长度
 * @param timeout 超时时间 (毫秒)
 * @return 实际接收字节数，负值表示错误
 */
int32_t xy_hal_spi_receive(xy_hal_spi_t spi, uint8_t *rx_data,
                           size_t length, uint32_t timeout);

/**
 * @brief 全双工收发 (阻塞模式)
 * @param spi SPI 设备句柄
 * @param tx_data 发送数据缓冲区 (NULL 表示填充 0x00)
 * @param rx_data 接收数据缓冲区
 * @param length 数据长度
 * @param timeout 超时时间 (毫秒)
 * @return 实际传输字节数，负值表示错误
 * 
 * @par 使用示例:
 * @code
 * uint8_t tx_buf[10] = {0x01, 0x02, 0x03};
 * uint8_t rx_buf[10];
 * xy_hal_spi_transfer(spi, tx_buf, rx_buf, 10, 100);
 * @endcode
 */
int32_t xy_hal_spi_transfer(xy_hal_spi_t spi, const uint8_t *tx_data,
                            uint8_t *rx_data, size_t length, uint32_t timeout);

/**
 * @brief 发送单个字节 (阻塞模式)
 * @param spi SPI 设备句柄
 * @param data 发送数据
 * @param timeout 超时时间 (毫秒)
 * @return 接收到的字节，负值表示错误
 */
int32_t xy_hal_spi_send_byte(xy_hal_spi_t spi, uint8_t data, uint32_t timeout);

/**
 * @brief 接收单个字节 (阻塞模式)
 * @param spi SPI 设备句柄
 * @param timeout 超时时间 (毫秒)
 * @return 接收到的字节，负值表示错误
 */
int32_t xy_hal_spi_receive_byte(xy_hal_spi_t spi, uint32_t timeout);

/* ==================== Non-blocking Transfer API ==================== */

/**
 * @brief 发送数据 (非阻塞模式)
 * @param spi SPI 设备句柄
 * @param tx_data 发送数据缓冲区
 * @param length 数据长度
 * @return 实际发送字节数，0 表示忙，负值表示错误
 */
int32_t xy_hal_spi_send_nb(xy_hal_spi_t spi, const uint8_t *tx_data,
                           size_t length);

/**
 * @brief 接收数据 (非阻塞模式)
 * @param spi SPI 设备句柄
 * @param rx_data 接收数据缓冲区
 * @param length 数据长度
 * @return 实际接收字节数，0 表示无数据，负值表示错误
 */
int32_t xy_hal_spi_receive_nb(xy_hal_spi_t spi, uint8_t *rx_data,
                              size_t length);

/**
 * @brief 检查是否可发送
 * @param spi SPI 设备句柄
 * @return 1 表示可发送，0 表示忙
 */
int32_t xy_hal_spi_tx_ready(xy_hal_spi_t spi);

/**
 * @brief 检查是否有数据可接收
 * @param spi SPI 设备句柄
 * @return 1 表示有数据，0 表示无数据
 */
int32_t xy_hal_spi_rx_available(xy_hal_spi_t spi);

/* ==================== Asynchronous Transfer API ==================== */

/**
 * @brief 发送数据 (异步模式)
 * @param spi SPI 设备句柄
 * @param tx_data 发送数据缓冲区
 * @param length 数据长度
 * @param callback 回调函数
 * @param arg 用户参数
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_spi_send_async(xy_hal_spi_t spi, const uint8_t *tx_data,
                                     size_t length, xy_hal_spi_callback_t callback,
                                     void *arg);

/**
 * @brief 接收数据 (异步模式)
 * @param spi SPI 设备句柄
 * @param rx_data 接收数据缓冲区
 * @param length 数据长度
 * @param callback 回调函数
 * @param arg 用户参数
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_spi_receive_async(xy_hal_spi_t spi, uint8_t *rx_data,
                                        size_t length, xy_hal_spi_callback_t callback,
                                        void *arg);

/**
 * @brief 全双工收发 (异步模式)
 * @param spi SPI 设备句柄
 * @param tx_data 发送数据缓冲区
 * @param rx_data 接收数据缓冲区
 * @param length 数据长度
 * @param callback 回调函数
 * @param arg 用户参数
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_spi_transfer_async(xy_hal_spi_t spi, const uint8_t *tx_data,
                                         uint8_t *rx_data, size_t length,
                                         xy_hal_spi_callback_t callback, void *arg);

/**
 * @brief 停止异步传输
 * @param spi SPI 设备句柄
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_spi_stop_async(xy_hal_spi_t spi);

/* ==================== Status and Error Handling ==================== */

/**
 * @brief 获取 SPI 状态
 * @param spi SPI 设备句柄
 * @param status 状态结构 (输出)
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_spi_get_status(xy_hal_spi_t spi,
                                     xy_hal_spi_status_t *status);

/**
 * @brief 获取错误标志
 * @param spi SPI 设备句柄
 * @return 错误标志
 */
xy_hal_spi_error_t xy_hal_spi_get_error(xy_hal_spi_t spi);

/**
 * @brief 清除错误标志
 * @param spi SPI 设备句柄
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_spi_clear_error(xy_hal_spi_t spi);

/**
 * @brief 获取统计信息
 * @param spi SPI 设备句柄
 * @param stats 统计结构 (输出)
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_spi_get_stats(xy_hal_spi_t spi,
                                    xy_hal_spi_stats_t *stats);

/**
 * @brief 重置统计信息
 * @param spi SPI 设备句柄
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_spi_reset_stats(xy_hal_spi_t spi);

/* ==================== Control API ==================== */

/**
 * @brief 控制 SPI 功能
 * @param spi SPI 设备句柄
 * @param cmd 控制命令
 * @param arg 参数
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_spi_control(xy_hal_spi_t spi, int cmd, void *arg);

/**
 * @brief 使能/禁用 SPI
 * @param spi SPI 设备句柄
 * @param enable 1=使能，0=禁用
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_spi_enable(xy_hal_spi_t spi, int enable);

/**
 * @brief 设置片选引脚
 * @param spi SPI 设备句柄
 * @param cs_pin 片选引脚号
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_spi_set_cs(xy_hal_spi_t spi, uint8_t cs_pin);

/**
 * @brief 手动控制片选
 * @param spi SPI 设备句柄
 * @param assert 1=选中，0=释放
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_spi_cs_assert(xy_hal_spi_t spi, int assert);

/* ==================== Control Command Definitions ==================== */

#define XY_HAL_SPI_CMD_FLUSH_TX     0x01  /**< 清空发送缓冲区 */
#include "xy_hal_error.h"
#define XY_HAL_SPI_CMD_FLUSH_RX     0x02  /**< 清空接收缓冲区 */
#include "xy_hal_error.h"
#define XY_HAL_SPI_CMD_SET_MODE     0x03  /**< 设置 SPI 模式 */
#include "xy_hal_error.h"
#define XY_HAL_SPI_CMD_GET_MODE     0x04  /**< 获取 SPI 模式 */
#include "xy_hal_error.h"
#define XY_HAL_SPI_CMD_SET_CS       0x05  /**< 设置片选引脚 */
#include "xy_hal_error.h"

#ifdef __cplusplus
}
#endif

#endif /* XY_HAL_SPI_DEV_H */
