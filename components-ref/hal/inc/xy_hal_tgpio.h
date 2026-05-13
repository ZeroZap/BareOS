/**
 * @file xy_hal_tgpio.h
 * @brief Time-aware GPIO Hardware Abstraction Layer
 * @version 2.0
 * @date 2026-02-27
 *
 * @brief 时间敏感型 GPIO 控制接口
 *
 * 时间感知 GPIO 允许开发者以微秒级甚至纳秒级的精度控制 GPIO 引脚状态变化的时间。
 * 它不仅仅是在某个时刻设置引脚高低电平，而是能够精确规划未来多个时间点的 GPIO 动作序列。
 *
 * @note 此功能依赖硬件定时器支持，并非所有 MCU 都可用
 * @note 使用前请确认目标平台是否支持此功能
 */

#ifndef XY_HAL_TGPIO_H
#define XY_HAL_TGPIO_H
#include "xy_hal_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_hal.h"
#include <stdint.h>
#include <stddef.h>

/**
 * @brief 时间戳类型 (微秒)
 */
typedef uint64_t xy_hal_timestamp_t;

/**
 * @brief GPIO 时间事件结构
 */
typedef struct {
    xy_hal_timestamp_t timestamp; /**< 事件触发时间戳 (微秒) */
    uint8_t pin;                  /**< GPIO 引脚号 */
    uint8_t state;                /**< 目标状态：0=低电平，1=高电平 */
} xy_hal_tgpio_event_t;

/**
 * @brief 时间 GPIO 配置结构
 */
typedef struct {
    uint8_t pin;                      /**< GPIO 引脚号 */
    uint8_t max_events;               /**< 最大事件数 */
    xy_hal_timestamp_t time_base;     /**< 时间基准：0=立即开始 */
} xy_hal_tgpio_config_t;

/**
 * @brief 时间 GPIO 序列配置
 */
typedef struct {
    const xy_hal_tgpio_event_t *events;   /**< 事件数组 */
    size_t event_count;                   /**< 事件数量 */
    uint8_t repeat;                       /**< 重复次数：0=无限循环 */
    uint8_t auto_start;                   /**< 自动启动：1=配置后自动启动 */
} xy_hal_tgpio_sequence_t;

/**
 * @brief 时间 GPIO 句柄
 */
typedef struct {
    void *timer_instance;         /**< 绑定的定时器实例 */
    void *port;                   /**< GPIO 端口 */
    uint8_t pin;                  /**< GPIO 引脚号 */
    uint8_t initialized;          /**< 初始化标志 */
    uint8_t running;              /**< 运行状态 */
} xy_hal_tgpio_handle_t;

/**
 * @brief 初始化时间 GPIO
 * @param handle 时间 GPIO 句柄
 * @param config 配置结构
 * @return XY_HAL_OK 成功，XY_HAL_ERROR_NOT_SUPPORT 不支持此功能，
 *         XY_HAL_ERROR_INVALID_PARAM 参数无效
 */
xy_hal_error_t xy_hal_tgpio_init(xy_hal_tgpio_handle_t *handle,
                                 const xy_hal_tgpio_config_t *config);

/**
 * @brief 反初始化时间 GPIO
 * @param handle 时间 GPIO 句柄
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_tgpio_deinit(xy_hal_tgpio_handle_t *handle);

/**
 * @brief 配置时间 GPIO 序列
 * @param handle 时间 GPIO 句柄
 * @param sequence 序列配置
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_tgpio_configure_sequence(xy_hal_tgpio_handle_t *handle,
                                               const xy_hal_tgpio_sequence_t *sequence);

/**
 * @brief 添加单个时间事件
 * @param handle 时间 GPIO 句柄
 * @param event 事件结构
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_tgpio_add_event(xy_hal_tgpio_handle_t *handle,
                                      const xy_hal_tgpio_event_t *event);

/**
 * @brief 清除所有待处理事件
 * @param handle 时间 GPIO 句柄
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_tgpio_clear_events(xy_hal_tgpio_handle_t *handle);

/**
 * @brief 启动时间 GPIO 序列
 * @param handle 时间 GPIO 句柄
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_tgpio_start(xy_hal_tgpio_handle_t *handle);

/**
 * @brief 停止时间 GPIO 序列
 * @param handle 时间 GPIO 句柄
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_tgpio_stop(xy_hal_tgpio_handle_t *handle);

/**
 * @brief 暂停时间 GPIO 序列
 * @param handle 时间 GPIO 句柄
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_tgpio_pause(xy_hal_tgpio_handle_t *handle);

/**
 * @brief 恢复时间 GPIO 序列
 * @param handle 时间 GPIO 句柄
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_tgpio_resume(xy_hal_tgpio_handle_t *handle);

/**
 * @brief 获取当前时间戳
 * @return 当前时间戳 (微秒)
 */
xy_hal_timestamp_t xy_hal_tgpio_get_timestamp(void);

/**
 * @brief 获取剩余事件数
 * @param handle 时间 GPIO 句柄
 * @return 剩余事件数，负值表示错误
 */
int xy_hal_tgpio_get_remaining_events(xy_hal_tgpio_handle_t *handle);

/**
 * @brief 检查序列是否完成
 * @param handle 时间 GPIO 句柄
 * @return 1 完成，0 未完成，负值表示错误
 */
int xy_hal_tgpio_is_complete(xy_hal_tgpio_handle_t *handle);

/**
 * @brief 设置序列完成回调
 * @param handle 时间 GPIO 句柄
 * @param callback 回调函数
 * @param arg 用户参数
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_tgpio_set_complete_callback(xy_hal_tgpio_handle_t *handle,
                                                  void (*callback)(void *arg),
                                                  void *arg);

/**
 * @brief 生成精确脉冲 (便捷函数)
 * @param handle 时间 GPIO 句柄
 * @param pulse_width_us 脉冲宽度 (微秒)
 * @param period_us 周期 (微秒)
 * @param count 脉冲数量：0=连续
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_tgpio_generate_pulse(xy_hal_tgpio_handle_t *handle,
                                           uint32_t pulse_width_us,
                                           uint32_t period_us,
                                           uint32_t count);

/**
 * @brief 生成 WS2812 时序数据 (便捷函数)
 * @param handle 时间 GPIO 句柄
 * @param data 数据字节
 * @return XY_HAL_OK 成功，其他值失败
 *
 * @note WS2812 时序要求:
 *       - 0 码：高电平 0.35µs，低电平 0.80µs
 *       - 1 码：高电平 0.70µs，低电平 0.60µs
 */
xy_hal_error_t xy_hal_tgpio_ws2812_write_byte(xy_hal_tgpio_handle_t *handle,
                                              uint8_t data);

/**
 * @brief 生成 WS2812 时序数据数组 (便捷函数)
 * @param handle 时间 GPIO 句柄
 * @param data 数据缓冲区
 * @param len 数据长度
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_tgpio_ws2812_write(xy_hal_tgpio_handle_t *handle,
                                         const uint8_t *data, size_t len);

/**
 * @brief 生成单总线设备时序 (便捷函数)
 * @param handle 时间 GPIO 句柄
 * @param data 数据字节
 * @param is_write 是否为写操作：1=写，0=读
 * @return 读取的数据位 (读操作时)，XY_HAL_ERROR_* (写操作失败时)
 *
 * @note 适用于 DHT11、DS18B20 等单总线设备
 */
int xy_hal_tgpio_1wire_transfer(xy_hal_tgpio_handle_t *handle, uint8_t data,
                                int is_write);

#ifdef __cplusplus
}
#endif

#endif /* XY_HAL_TGPIO_H */
