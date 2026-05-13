/**
 * @file xy_mux_spi.h
 * @brief MUX SPI Interface
 * @version 1.0.0
 * @date 2026-03-02
 */

#ifndef XY_MUX_SPI_H
#define XY_MUX_SPI_H

#include "xy_mux.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SPI 模式
 */
typedef enum {
    XY_MUX_SPI_MODE0 = 0,   /**< CPOL=0, CPHA=0 */
    XY_MUX_SPI_MODE1,       /**< CPOL=0, CPHA=1 */
    XY_MUX_SPI_MODE2,       /**< CPOL=1, CPHA=0 */
    XY_MUX_SPI_MODE3,       /**< CPOL=1, CPHA=1 */
} xy_mux_spi_mode_t;

/**
 * @brief SPI 配置
 */
typedef struct {
    uint32_t speed;         /**< 速度 (Hz) */
    xy_mux_spi_mode_t mode; /**< SPI 模式 */
    uint8_t bits;           /**< 数据位 (8/16) */
    uint8_t cs_pin;         /**< 片选引脚 */
} xy_mux_spi_config_t;

/**
 * @brief SPI 命令
 */
typedef enum {
    XY_MUX_SPI_CMD_SET_SPEED = 0,   /**< 设置速度 */
    XY_MUX_SPI_CMD_GET_SPEED,       /**< 获取速度 */
    XY_MUX_SPI_CMD_SET_MODE,        /**< 设置模式 */
    XY_MUX_SPI_CMD_GET_MODE,        /**< 获取模式 */
    XY_MUX_SPI_CMD_SET_CONFIG,      /**< 设置配置 */
} xy_mux_spi_cmd_t;

/**
 * @brief 注册 MUX SPI
 */
int32_t xy_mux_spi_register(xy_mux_manager_t *mgr, uint8_t channel,
                            const xy_mux_ops_t *ops, void *user_data);

/**
 * @brief 配置 SPI
 */
int32_t xy_mux_spi_config(xy_mux_manager_t *mgr, uint8_t channel,
                          const xy_mux_spi_config_t *config);

/**
 * @brief SPI 传输
 */
int32_t xy_mux_spi_transfer(xy_mux_manager_t *mgr, uint8_t channel,
                            const void *tx_data, void *rx_data, size_t len);

/**
 * @brief SPI 读取
 */
int32_t xy_mux_spi_read(xy_mux_manager_t *mgr, uint8_t channel,
                        void *data, size_t len);

/**
 * @brief SPI 写入
 */
int32_t xy_mux_spi_write(xy_mux_manager_t *mgr, uint8_t channel,
                         const void *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif
