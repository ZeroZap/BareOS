/**
 * @file xy_hal_exti.h
 * @brief EXTI (External Interrupt) Hardware Abstraction Layer
 * @version 2.0
 * @date 2026-02-27
 */

#ifndef XY_HAL_EXTI_H
#define XY_HAL_EXTI_H
#include "xy_hal_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_hal.h"
#include <stdint.h>

/**
 * @brief EXTI 触发方式
 */
typedef enum {
    XY_HAL_EXTI_TRIGGER_RISING = 0,   /**< 上升沿触发 */
    XY_HAL_EXTI_TRIGGER_FALLING,      /**< 下降沿触发 */
    XY_HAL_EXTI_TRIGGER_BOTH,         /**< 双边沿触发 */
} xy_hal_exti_trigger_t;

/**
 * @brief EXTI 线路
 */
typedef enum {
    XY_HAL_EXTI_LINE_0 = 0,     /**< 线路 0 */
    XY_HAL_EXTI_LINE_1,         /**< 线路 1 */
    XY_HAL_EXTI_LINE_2,         /**< 线路 2 */
    XY_HAL_EXTI_LINE_3,         /**< 线路 3 */
    XY_HAL_EXTI_LINE_4,         /**< 线路 4 */
    XY_HAL_EXTI_LINE_5,         /**< 线路 5 */
    XY_HAL_EXTI_LINE_6,         /**< 线路 6 */
    XY_HAL_EXTI_LINE_7,         /**< 线路 7 */
    XY_HAL_EXTI_LINE_8,         /**< 线路 8 */
    XY_HAL_EXTI_LINE_9,         /**< 线路 9 */
    XY_HAL_EXTI_LINE_10,        /**< 线路 10 */
    XY_HAL_EXTI_LINE_11,        /**< 线路 11 */
    XY_HAL_EXTI_LINE_12,        /**< 线路 12 */
    XY_HAL_EXTI_LINE_13,        /**< 线路 13 */
    XY_HAL_EXTI_LINE_14,        /**< 线路 14 */
    XY_HAL_EXTI_LINE_15,        /**< 线路 15 */
    XY_HAL_EXTI_LINE_MAX,       /**< 最大线路数 */
} xy_hal_exti_line_t;

/**
 * @brief EXTI 配置结构
 */
typedef struct {
    xy_hal_exti_line_t line;        /**< EXTI 线路 */
    xy_hal_exti_trigger_t trigger;  /**< 触发方式 */
    uint8_t enable;                 /**< 使能标志 */
} xy_hal_exti_config_t;

/**
 * @brief EXTI 回调类型
 */
typedef void (*xy_hal_exti_callback_t)(xy_hal_exti_line_t line, void *arg);

/**
 * @brief EXTI 处理句柄
 */
typedef struct {
    xy_hal_exti_line_t line;            /**< EXTI 线路 */
    xy_hal_exti_callback_t callback;    /**< 回调函数 */
    void *arg;                          /**< 用户参数 */
} xy_hal_exti_handle_t;

/**
 * @brief 初始化 EXTI
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_exti_init(void);

/**
 * @brief 反初始化 EXTI
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_exti_deinit(void);

/**
 * @brief 配置 EXTI 线路
 * @param config EXTI 配置
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_exti_configure(const xy_hal_exti_config_t *config);

/**
 * @brief 使能 EXTI 线路
 * @param line EXTI 线路
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_exti_enable(xy_hal_exti_line_t line);

/**
 * @brief 禁用 EXTI 线路
 * @param line EXTI 线路
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_exti_disable(xy_hal_exti_line_t line);

/**
 * @brief 注册 EXTI 回调
 * @param line EXTI 线路
 * @param callback 回调函数
 * @param arg 用户参数
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_exti_register_callback(xy_hal_exti_line_t line,
                                             xy_hal_exti_callback_t callback,
                                             void *arg);

/**
 * @brief 注销 EXTI 回调
 * @param line EXTI 线路
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_exti_unregister_callback(xy_hal_exti_line_t line);

/**
 * @brief 获取中断挂起位
 * @param line EXTI 线路
 * @return 1 挂起，0 未挂起
 */
int xy_hal_exti_get_pending(xy_hal_exti_line_t line);

/**
 * @brief 设置中断挂起位
 * @param line EXTI 线路
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_exti_set_pending(xy_hal_exti_line_t line);

/**
 * @brief 清除中断挂起位
 * @param line EXTI 线路
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_exti_clear_pending(xy_hal_exti_line_t line);

/**
 * @brief 生成软件中断
 * @param line EXTI 线路
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_exti_generate_software_interrupt(xy_hal_exti_line_t line);

/**
 * @brief EXTI 中断处理函数 (由底层调用)
 * @param line EXTI 线路
 */
void xy_hal_exti_irq_handler(xy_hal_exti_line_t line);

/**
 * @brief 配置 GPIO 到 EXTI 线路映射
 * @param port GPIO 端口号
 * @param pin GPIO 引脚号
 * @param line EXTI 线路
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_exti_map_gpio(uint8_t port, uint8_t pin,
                                    xy_hal_exti_line_t line);

#ifdef __cplusplus
}
#endif

#endif /* XY_HAL_EXTI_H */
