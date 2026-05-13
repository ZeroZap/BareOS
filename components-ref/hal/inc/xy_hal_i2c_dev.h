/**
 * @file xy_hal_i2c_dev.h
 * @brief XinYi HAL I2C Unified Device API
 * @version 1.0.0
 * @date 2026-03-15
 * 
 * @note 统一 I2C 设备 API，适用于所有平台 (STM32/WCH/HC32)
 * @note 支持主/从模式，轮询/中断/DMA 传输
 */

#ifndef XY_HAL_I2C_DEV_H
#define XY_HAL_I2C_DEV_H
#include "xy_hal_error.h"

#include "xy_hal_i2c_types.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== I2C Device Handle ==================== */

/**
 * @brief I2C 设备句柄 (不透明指针)
 */
typedef void *xy_hal_i2c_t;

/* ==================== Device Management API ==================== */

/**
 * @brief 绑定 I2C 设备
 * @param name 设备名称 (如 "I2C1", "I2C2")
 * @return I2C 设备句柄，NULL 表示失败
 */
xy_hal_i2c_t xy_hal_i2c_bind(const char *name);

/**
 * @brief 解绑 I2C 设备
 * @param i2c I2C 设备句柄
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2c_unbind(xy_hal_i2c_t i2c);

/* ==================== Configuration API ==================== */

/**
 * @brief 配置 I2C 参数
 * @param i2c I2C 设备句柄
 * @param config I2C 配置结构
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2c_configure(xy_hal_i2c_t i2c,
                                    const xy_hal_i2c_config_t *config);

/**
 * @brief 获取当前 I2C 配置
 * @param i2c I2C 设备句柄
 * @param config 配置结构 (输出)
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2c_get_config(xy_hal_i2c_t i2c,
                                     xy_hal_i2c_config_t *config);

/**
 * @brief 设置时钟速度
 * @param i2c I2C 设备句柄
 * @param clock_speed 时钟速度 (Hz)
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2c_set_clock_speed(xy_hal_i2c_t i2c, uint32_t clock_speed);

/* ==================== Blocking Transfer API ==================== */

/**
 * @brief 主模式发送数据 (阻塞)
 * @param i2c I2C 设备句柄
 * @param dev_addr 设备地址 (7 位)
 * @param data 发送数据缓冲区
 * @param length 数据长度
 * @param timeout 超时时间 (毫秒)
 * @return 实际发送字节数，负值表示错误
 * 
 * @par 使用示例:
 * @code
 * uint8_t reg = 0x00;
 * xy_hal_i2c_master_transmit(i2c, 0x68, &reg, 1, 100);
 * @endcode
 */
int32_t xy_hal_i2c_master_transmit(xy_hal_i2c_t i2c, uint16_t dev_addr,
                                   const uint8_t *data, size_t length,
                                   uint32_t timeout);

/**
 * @brief 主模式接收数据 (阻塞)
 * @param i2c I2C 设备句柄
 * @param dev_addr 设备地址 (7 位)
 * @param data 接收数据缓冲区
 * @param length 数据长度
 * @param timeout 超时时间 (毫秒)
 * @return 实际接收字节数，负值表示错误
 */
int32_t xy_hal_i2c_master_receive(xy_hal_i2c_t i2c, uint16_t dev_addr,
                                  uint8_t *data, size_t length,
                                  uint32_t timeout);

/**
 * @brief 写入寄存器 (阻塞)
 * @param i2c I2C 设备句柄
 * @param dev_addr 设备地址
 * @param reg_addr 寄存器地址
 * @param reg_size 寄存器地址大小
 * @param data 数据缓冲区
 * @param length 数据长度
 * @param timeout 超时时间 (毫秒)
 * @return XY_HAL_OK 成功，其他值失败
 * 
 * @par 使用示例:
 * @code
 * uint8_t value = 0x55;
 * xy_hal_i2c_reg_write(i2c, 0x68, 0x00, 1, &value, 1, 100);
 * @endcode
 */
xy_hal_error_t xy_hal_i2c_reg_write(xy_hal_i2c_t i2c, uint16_t dev_addr,
                                    const uint8_t *reg_addr, size_t reg_size,
                                    const uint8_t *data, size_t length,
                                    uint32_t timeout);

/**
 * @brief 读取寄存器 (阻塞)
 * @param i2c I2C 设备句柄
 * @param dev_addr 设备地址
 * @param reg_addr 寄存器地址
 * @param reg_size 寄存器地址大小
 * @param data 数据缓冲区
 * @param length 数据长度
 * @param timeout 超时时间 (毫秒)
 * @return XY_HAL_OK 成功，其他值失败
 * 
 * @par 使用示例:
 * @code
 * uint8_t value;
 * xy_hal_i2c_reg_read(i2c, 0x68, 0x00, 1, &value, 1, 100);
 * @endcode
 */
xy_hal_error_t xy_hal_i2c_reg_read(xy_hal_i2c_t i2c, uint16_t dev_addr,
                                   const uint8_t *reg_addr, size_t reg_size,
                                   uint8_t *data, size_t length,
                                   uint32_t timeout);

/**
 * @brief 扫描 I2C 总线设备
 * @param i2c I2C 设备句柄
 * @param addrs 设备地址数组 (输出)
 * @param max_count 最大设备数量
 * @param timeout 超时时间 (毫秒)
 * @return 检测到的设备数量，负值表示错误
 * 
 * @par 使用示例:
 * @code
 * uint8_t addrs[16];
 * int count = xy_hal_i2c_scan(i2c, addrs, 16, 10);
 * for (int i = 0; i < count; i++) {
 *     printf("Found device at 0x%02X\n", addrs[i]);
 * }
 * @endcode
 */
int32_t xy_hal_i2c_scan(xy_hal_i2c_t i2c, uint8_t *addrs, size_t max_count,
                        uint32_t timeout);

/* ==================== Non-blocking Transfer API ==================== */

/**
 * @brief 主模式发送数据 (非阻塞)
 * @param i2c I2C 设备句柄
 * @param dev_addr 设备地址
 * @param data 发送数据缓冲区
 * @param length 数据长度
 * @return 实际发送字节数，0 表示忙，负值表示错误
 */
int32_t xy_hal_i2c_master_transmit_nb(xy_hal_i2c_t i2c, uint16_t dev_addr,
                                      const uint8_t *data, size_t length);

/**
 * @brief 主模式接收数据 (非阻塞)
 * @param i2c I2C 设备句柄
 * @param dev_addr 设备地址
 * @param data 接收数据缓冲区
 * @param length 数据长度
 * @return 实际接收字节数，0 表示无数据，负值表示错误
 */
int32_t xy_hal_i2c_master_receive_nb(xy_hal_i2c_t i2c, uint16_t dev_addr,
                                     uint8_t *data, size_t length);

/* ==================== Asynchronous Transfer API ==================== */

/**
 * @brief 主模式发送数据 (异步)
 * @param i2c I2C 设备句柄
 * @param dev_addr 设备地址
 * @param data 发送数据缓冲区
 * @param length 数据长度
 * @param callback 回调函数
 * @param arg 用户参数
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2c_master_transmit_async(xy_hal_i2c_t i2c, uint16_t dev_addr,
                                                const uint8_t *data, size_t length,
                                                xy_hal_i2c_callback_t callback, void *arg);

/**
 * @brief 主模式接收数据 (异步)
 * @param i2c I2C 设备句柄
 * @param dev_addr 设备地址
 * @param data 接收数据缓冲区
 * @param length 数据长度
 * @param callback 回调函数
 * @param arg 用户参数
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2c_master_receive_async(xy_hal_i2c_t i2c, uint16_t dev_addr,
                                               uint8_t *data, size_t length,
                                               xy_hal_i2c_callback_t callback, void *arg);

/**
 * @brief 停止异步传输
 * @param i2c I2C 设备句柄
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2c_stop_async(xy_hal_i2c_t i2c);

/* ==================== Status and Error Handling ==================== */

/**
 * @brief 获取 I2C 状态
 * @param i2c I2C 设备句柄
 * @param status 状态结构 (输出)
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2c_get_status(xy_hal_i2c_t i2c,
                                     xy_hal_i2c_status_t *status);

/**
 * @brief 获取错误标志
 * @param i2c I2C 设备句柄
 * @return 错误标志
 */
xy_hal_i2c_error_t xy_hal_i2c_get_error(xy_hal_i2c_t i2c);

/**
 * @brief 清除错误标志
 * @param i2c I2C 设备句柄
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2c_clear_error(xy_hal_i2c_t i2c);

/**
 * @brief 获取统计信息
 * @param i2c I2C 设备句柄
 * @param stats 统计结构 (输出)
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2c_get_stats(xy_hal_i2c_t i2c,
                                    xy_hal_i2c_stats_t *stats);

/**
 * @brief 重置统计信息
 * @param i2c I2C 设备句柄
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2c_reset_stats(xy_hal_i2c_t i2c);

/* ==================== Control API ==================== */

/**
 * @brief 控制 I2C 功能
 * @param i2c I2C 设备句柄
 * @param cmd 控制命令
 * @param arg 参数
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2c_control(xy_hal_i2c_t i2c, int cmd, void *arg);

/**
 * @brief 使能/禁用 I2C
 * @param i2c I2C 设备句柄
 * @param enable 1=使能，0=禁用
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2c_enable(xy_hal_i2c_t i2c, int enable);

/**
 * @brief 软件复位 I2C
 * @param i2c I2C 设备句柄
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2c_reset(xy_hal_i2c_t i2c);

/**
 * @brief 检查设备是否存在
 * @param i2c I2C 设备句柄
 * @param dev_addr 设备地址
 * @param timeout 超时时间 (毫秒)
 * @return XY_HAL_OK 存在，其他值不存在或错误
 */
xy_hal_error_t xy_hal_i2c_probe(xy_hal_i2c_t i2c, uint16_t dev_addr,
                                uint32_t timeout);

/* ==================== Control Command Definitions ==================== */

#define XY_HAL_I2C_CMD_FLUSH_TX     0x01  /**< 清空发送缓冲区 */
#include "xy_hal_error.h"
#define XY_HAL_I2C_CMD_FLUSH_RX     0x02  /**< 清空接收缓冲区 */
#include "xy_hal_error.h"
#define XY_HAL_I2C_CMD_RESET        0x03  /**< 软件复位 */
#include "xy_hal_error.h"
#define XY_HAL_I2C_CMD_PROBE        0x04  /**< 探测设备 */
#include "xy_hal_error.h"

#ifdef __cplusplus
}
#endif

#endif /* XY_HAL_I2C_DEV_H */
