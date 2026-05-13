/**
 * @file xy_mux_gpio.h
 * @brief MUX GPIO Interface
 * @version 1.0.0
 * @date 2026-03-02
 */

#ifndef XY_MUX_GPIO_H
#define XY_MUX_GPIO_H

#include "xy_mux.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief GPIO 方向
 */
typedef enum {
    XY_MUX_GPIO_INPUT = 0,
    XY_MUX_GPIO_OUTPUT,
} xy_mux_gpio_dir_t;

/**
 * @brief GPIO 电平
 */
typedef enum {
    XY_MUX_GPIO_LOW = 0,
    XY_MUX_GPIO_HIGH,
} xy_mux_gpio_level_t;

/**
 * @brief GPIO 配置
 */
typedef struct {
    xy_mux_gpio_dir_t dir;        /**< 方向 */
    xy_mux_gpio_level_t pull;     /**< 上下拉 */
    bool interrupt_enable;        /**< 中断使能 */
} xy_mux_gpio_config_t;

/**
 * @brief GPIO 命令
 */
typedef enum {
    XY_MUX_GPIO_CMD_SET_DIR = 0,  /**< 设置方向 */
    XY_MUX_GPIO_CMD_GET_DIR,      /**< 获取方向 */
    XY_MUX_GPIO_CMD_SET_PULL,     /**< 设置上下拉 */
    XY_MUX_GPIO_CMD_ENABLE_IRQ,   /**< 使能中断 */
    XY_MUX_GPIO_CMD_DISABLE_IRQ,  /**< 禁用中断 */
} xy_mux_gpio_cmd_t;

/**
 * @brief 注册 MUX GPIO
 * @param mgr MUX 管理器
 * @param channel GPIO 通道号
 * @param ops GPIO 操作接口
 * @param user_data 用户数据
 * @return XY_MUX_OK 成功，其他值失败
 */
int32_t xy_mux_gpio_register(xy_mux_manager_t *mgr, uint8_t channel,
                             const xy_mux_ops_t *ops, void *user_data);

/**
 * @brief 配置 GPIO
 * @param mgr MUX 管理器
 * @param channel GPIO 通道号
 * @param config GPIO 配置
 * @return XY_MUX_OK 成功，其他值失败
 */
int32_t xy_mux_gpio_config(xy_mux_manager_t *mgr, uint8_t channel,
                           const xy_mux_gpio_config_t *config);

/**
 * @brief 读取 GPIO 电平
 * @param mgr MUX 管理器
 * @param channel GPIO 通道号
 * @return GPIO 电平，负值表示错误
 */
int32_t xy_mux_gpio_read(xy_mux_manager_t *mgr, uint8_t channel);

/**
 * @brief 写入 GPIO 电平
 * @param mgr MUX 管理器
 * @param channel GPIO 通道号
 * @param level GPIO 电平
 * @return XY_MUX_OK 成功，其他值失败
 */
int32_t xy_mux_gpio_write(xy_mux_manager_t *mgr, uint8_t channel,
                          xy_mux_gpio_level_t level);

/**
 * @brief 切换 GPIO 电平
 * @param mgr MUX 管理器
 * @param channel GPIO 通道号
 * @return XY_MUX_OK 成功，其他值失败
 */
int32_t xy_mux_gpio_toggle(xy_mux_manager_t *mgr, uint8_t channel);

#ifdef __cplusplus
}
#endif

#endif
