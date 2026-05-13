/**
 * @file xy_hal_wdg.h
 * @brief Watchdog Timer Hardware Abstraction Layer
 * @version 2.0
 * @date 2026-02-27
 */

#ifndef XY_HAL_WDG_H
#define XY_HAL_WDG_H
#include "xy_hal_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_hal.h"
#include <stdint.h>

/**
 * @brief 看门狗类型
 */
typedef enum {
    XY_HAL_WDG_IWDG = 0,    /**< 独立看门狗 (IWDG) */
    XY_HAL_WDG_WWDG,        /**< 窗口看门狗 (WWDG) */
} xy_hal_wdg_type_t;

/**
 * @brief 独立看门狗配置
 */
typedef struct {
    uint32_t prescaler;     /**< 预分频系数 */
    uint32_t reload;        /**< 重装载值 */
    uint32_t timeout_ms;    /**< 超时时间 (ms) */
} xy_hal_iwdg_config_t;

/**
 * @brief 窗口看门狗配置
 */
typedef struct {
    uint32_t prescaler;     /**< 预分频系数 */
    uint32_t window;        /**< 窗口值 */
    uint32_t counter;       /**< 计数器值 */
    uint32_t timeout_ms;    /**< 超时时间 (ms) */
} xy_hal_wwdg_config_t;

/**
 * @brief 看门狗回调类型
 */
typedef void (*xy_hal_wdg_callback_t)(void *wdg, void *arg);

/**
 * @brief 初始化独立看门狗
 * @param wdg 看门狗实例
 * @param config IWDG 配置
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_iwdg_init(void *wdg, const xy_hal_iwdg_config_t *config);

/**
 * @brief 启动独立看门狗
 * @param wdg 看门狗实例
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_iwdg_start(void *wdg);

/**
 * @brief 喂独立看门狗
 * @param wdg 看门狗实例
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_iwdg_feed(void *wdg);

/**
 * @brief 刷新独立看门狗 (别名)
 * @param wdg 看门狗实例
 * @return XY_HAL_OK 成功，其他值失败
 */
static inline xy_hal_error_t xy_hal_iwdg_refresh(void *wdg) {
    return xy_hal_iwdg_feed(wdg);
}

/**
 * @brief 喂狗 (通用别名)
 * @param wdg 看门狗实例
 * @return XY_HAL_OK 成功，其他值失败
 */
static inline xy_hal_error_t xy_hal_wdg_feed(void *wdg) {
    return xy_hal_iwdg_feed(wdg);
}

/**
 * @brief 获取独立看门狗剩余时间
 * @param wdg 看门狗实例
 * @return 剩余时间 (ms)，负值表示错误
 */
int xy_hal_iwdg_get_remaining_time(void *wdg);

/**
 * @brief 配置独立看门狗超时时间
 * @param wdg 看门狗实例
 * @param timeout_ms 超时时间 (ms)
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_iwdg_set_timeout(void *wdg, uint32_t timeout_ms);

/**
 * @brief 初始化窗口看门狗
 * @param wdg 看门狗实例
 * @param config WWDG 配置
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_wwdg_init(void *wdg, const xy_hal_wwdg_config_t *config);

/**
 * @brief 启动窗口看门狗
 * @param wdg 看门狗实例
 * @param enable_early_wakeup 使能提前唤醒：1=使能，0=禁用
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_wwdg_start(void *wdg, uint8_t enable_early_wakeup);

/**
 * @brief 喂窗口看门狗
 * @param wdg 看门狗实例
 * @param counter 计数器值 (必须在窗口范围内)
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_wwdg_feed(void *wdg, uint32_t counter);

/**
 * @brief 获取窗口看门狗剩余时间
 * @param wdg 看门狗实例
 * @return 剩余时间 (ms)，负值表示错误
 */
int xy_hal_wwdg_get_remaining_time(void *wdg);

/**
 * @brief 配置窗口看门狗窗口值
 * @param wdg 看门狗实例
 * @param window 窗口值
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_wwdg_set_window(void *wdg, uint32_t window);

/**
 * @brief 注册窗口看门狗提前唤醒中断回调
 * @param wdg 看门狗实例
 * @param callback 回调函数
 * @param arg 用户参数
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_wwdg_register_ewi_callback(void *wdg,
                                                 xy_hal_wdg_callback_t callback,
                                                 void *arg);

/**
 * @brief 使能看门狗中断 (如果支持)
 * @param wdg 看门狗实例
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_wdg_enable_irq(void *wdg);

/**
 * @brief 禁用看门狗中断 (如果支持)
 * @param wdg 看门狗实例
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_wdg_disable_irq(void *wdg);

/**
 * @brief 系统复位 (通过看门狗)
 */
void xy_hal_wdg_system_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* XY_HAL_WDG_H */
