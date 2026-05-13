/**
 * @file xy_mux_i2c.h
 * @brief MUX I2C Interface
 * @version 1.0.0
 * @date 2026-03-02
 */

#ifndef XY_MUX_I2C_H
#define XY_MUX_I2C_H

#include "xy_mux.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief I2C 配置
 */
typedef struct {
    uint32_t speed;         /**< 速度 (100k/400k/1M) */
    uint8_t addr_bits;      /**< 地址位数 (7/10) */
} xy_mux_i2c_config_t;

/**
 * @brief I2C 命令
 */
typedef enum {
    XY_MUX_I2C_CMD_SET_SPEED = 0,   /**< 设置速度 */
    XY_MUX_I2C_CMD_GET_SPEED,       /**< 获取速度 */
    XY_MUX_I2C_CMD_SET_CONFIG,      /**< 设置配置 */
    XY_MUX_I2C_CMD_GET_CONFIG,      /**< 获取配置 */
    XY_MUX_I2C_CMD_SCAN,            /**< 扫描设备 */
} xy_mux_i2c_cmd_t;

/**
 * @brief I2C 传输消息
 */
typedef struct {
    uint16_t addr;              /**< 设备地址 */
    uint16_t flags;             /**< 传输标志 */
    uint16_t len;               /**< 数据长度 */
    uint8_t *buf;               /**< 数据缓冲区 */
} xy_mux_i2c_msg_t;

#define XY_MUX_I2C_M_RD         0x0001  /**< 读标志 */

/**
 * @brief 注册 MUX I2C
 */
int32_t xy_mux_i2c_register(xy_mux_manager_t *mgr, uint8_t channel,
                            const xy_mux_ops_t *ops, void *user_data);

/**
 * @brief 配置 I2C
 */
int32_t xy_mux_i2c_config(xy_mux_manager_t *mgr, uint8_t channel,
                          const xy_mux_i2c_config_t *config);

/**
 * @brief I2C 传输
 */
int32_t xy_mux_i2c_transfer(xy_mux_manager_t *mgr, uint8_t channel,
                            xy_mux_i2c_msg_t *msgs, int count);

/**
 * @brief I2C 读取
 */
int32_t xy_mux_i2c_read(xy_mux_manager_t *mgr, uint8_t channel,
                        uint16_t addr, void *data, size_t len);

/**
 * @brief I2C 写入
 */
int32_t xy_mux_i2c_write(xy_mux_manager_t *mgr, uint8_t channel,
                         uint16_t addr, const void *data, size_t len);

/**
 * @brief I2C 扫描设备
 */
int32_t xy_mux_i2c_scan(xy_mux_manager_t *mgr, uint8_t channel,
                        uint16_t *addrs, size_t max_count);

#ifdef __cplusplus
}
#endif

#endif
