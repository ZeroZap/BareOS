/**
 * @file xy_hal_rng.h
 * @brief RNG (Random Number Generator) Hardware Abstraction Layer
 * @version 2.0
 * @date 2026-02-27
 */

#ifndef XY_HAL_RNG_H
#define XY_HAL_RNG_H
#include "xy_hal_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_hal.h"
#include <stdint.h>
#include <stddef.h>

/**
 * @brief RNG 配置结构
 */
typedef struct {
    uint8_t clock_enable;     /**< 时钟使能 */
    uint8_t interrupt_enable; /**< 中断使能 */
} xy_hal_rng_config_t;

/**
 * @brief RNG 事件类型
 */
typedef enum {
    XY_HAL_RNG_EVENT_READY = 0,   /**< 随机数就绪 */
    XY_HAL_RNG_EVENT_ERROR,       /**< 错误 (种子丢失等) */
} xy_hal_rng_event_t;

/**
 * @brief RNG 回调类型
 */
typedef void (*xy_hal_rng_callback_t)(xy_hal_rng_event_t event, void *arg);

/**
 * @brief 初始化 RNG
 * @param rng RNG 实例
 * @param config RNG 配置
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_rng_init(void *rng, const xy_hal_rng_config_t *config);

/**
 * @brief 反初始化 RNG
 * @param rng RNG 实例
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_rng_deinit(void *rng);

/**
 * @brief 使能 RNG
 * @param rng RNG 实例
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_rng_enable(void *rng);

/**
 * @brief 禁用 RNG
 * @param rng RNG 实例
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_rng_disable(void *rng);

/**
 * @brief 获取 32 位随机数 (阻塞)
 * @param rng RNG 实例
 * @param timeout 超时时间 (ms)
 * @return 随机数，负值表示错误
 */
int32_t xy_hal_rng_get_random(void *rng, uint32_t timeout);

/**
 * @brief 获取 32 位随机数 (非阻塞)
 * @param rng RNG 实例
 * @param value 随机数输出
 * @return XY_HAL_OK 成功，XY_HAL_ERROR_BUSY 未就绪，其他值失败
 */
xy_hal_error_t xy_hal_rng_get_random_nb(void *rng, uint32_t *value);

/**
 * @brief 生成随机数缓冲区
 * @param rng RNG 实例
 * @param buffer 输出缓冲区
 * @param count 随机数数量
 * @param timeout 超时时间 (ms)
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_rng_get_buffer(void *rng, uint32_t *buffer,
                                     size_t count, uint32_t timeout);

/**
 * @brief 生成指定范围的随机数
 * @param rng RNG 实例
 * @param min 最小值
 * @param max 最大值
 * @return 随机数，负值表示错误
 */
int32_t xy_hal_rng_get_random_range(void *rng, int32_t min, int32_t max);

/**
 * @brief 注册 RNG 回调
 * @param rng RNG 实例
 * @param event 事件类型
 * @param callback 回调函数
 * @param arg 用户参数
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_rng_register_callback(void *rng, xy_hal_rng_event_t event,
                                            xy_hal_rng_callback_t callback, void *arg);

/**
 * @brief 检查 RNG 是否就绪
 * @param rng RNG 实例
 * @return 1 就绪，0 未就绪，负值表示错误
 */
int xy_hal_rng_is_ready(void *rng);

/**
 * @brief 清除 RNG 错误状态
 * @param rng RNG 实例
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_rng_clear_error(void *rng);

/**
 * @brief 获取 RNG 错误码
 * @param rng RNG 实例
 * @return 错误码，0 表示无错误
 */
int xy_hal_rng_get_error_code(void *rng);

/**
 * @brief 软件 RNG (基于硬件种子的伪随机)
 * @param seed 种子值 (0 表示使用硬件 RNG)
 * @return 随机数
 */
uint32_t xy_hal_rng_soft_random(uint32_t seed);

/**
 * @brief 软件随机数缓冲区
 * @param buffer 输出缓冲区
 * @param count 随机数数量
 * @param seed 种子值 (0 表示使用硬件 RNG)
 */
void xy_hal_rng_soft_buffer(uint32_t *buffer, size_t count, uint32_t seed);

#ifdef __cplusplus
}
#endif

#endif /* XY_HAL_RNG_H */
