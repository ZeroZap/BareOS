/**
 * @file xy_hal_pwm.h
 * @brief XinYi PWM Hardware Abstraction Layer
 * @version 2.0
 * @date 2026-02-28
 */

#ifndef XY_HAL_PWM_H
#define XY_HAL_PWM_H
#include "xy_hal_error.h"

#include "xy_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief PWM 频率单位
 */
#define XY_HAL_PWM_FREQ_HZ 1000000  /* 1MHz = 1000000 Hz */
#include "xy_hal_error.h"

/**
 * @brief PWM 通道枚举
 */
typedef enum {
    XY_HAL_PWM_CHANNEL_1 = 0,       /**< 通道 1 */
    XY_HAL_PWM_CHANNEL_2,           /**< 通道 2 */
    XY_HAL_PWM_CHANNEL_3,           /**< 通道 3 */
    XY_HAL_PWM_CHANNEL_4,           /**< 通道 4 */
    XY_HAL_PWM_CHANNEL_MAX
} xy_hal_pwm_channel_t;

/**
 * @brief PWM 极性
 */
typedef enum {
    XY_HAL_PWM_POLARITY_HIGH = 0,   /**< 高电平有效 */
    XY_HAL_PWM_POLARITY_LOW,        /**< 低电平有效 */
} xy_hal_pwm_polarity_t;

/**
 * @brief PWM 模式
 */
typedef enum {
    XY_HAL_PWM_MODE_EDGE_ALIGN = 0, /**< 边沿对齐 */
    XY_HAL_PWM_MODE_CENTER_ALIGN,   /**< 中心对齐 */
    XY_HAL_PWM_MODE_COMPLEMENTARY,  /**< 互补输出 */
} xy_hal_pwm_mode_t;

/**
 * @brief PWM 波形形状
 */
typedef enum {
    XY_HAL_PWM_WAVE_SQUARE = 0,     /**< 方波 */
    XY_HAL_PWM_WAVE_TRIANGLE,       /**< 三角波 */
    XY_HAL_PWM_WAVE_SAWTOOTH_UP,    /**< 锯齿波上升 */
    XY_HAL_PWM_WAVE_SAWTOOTH_DOWN,  /**< 锯齿波下降 */
} xy_hal_pwm_wave_shape_t;

/**
 * @brief PWM 配置结构
 */
typedef struct {
    uint32_t frequency;             /**< 频率 (Hz) */
    uint32_t duty_cycle;            /**< 占空比 (0-10000 = 0.00%-100.00%) */
    xy_hal_pwm_polarity_t polarity; /**< 极性 */
    xy_hal_pwm_mode_t mode;         /**< 模式 */
    xy_hal_pwm_wave_shape_t wave_shape; /**< 波形形状 */
    uint8_t deadtime;               /**< 死区时间 (ns) */
    uint8_t complementary_enable;   /**< 互补输出使能 */
    uint8_t fault_mode;             /**< 故障模式 */
} xy_hal_pwm_config_t;

/**
 * @brief PWM 互补输出配置
 */
typedef struct {
    xy_hal_pwm_polarity_t polarity; /**< 互补输出极性 */
    uint32_t deadtime_ns;          /**< 死区时间 (纳秒) */
    uint8_t output_enable;         /**< 输出使能 */
    uint8_t idle_state;            /**< 空闲状态 */
} xy_hal_pwm_complementary_config_t;

/**
 * @brief PWM 事件类型
 */
typedef enum {
    XY_HAL_PWM_EVENT_UPDATE = 0,    /**< 更新事件 */
    XY_HAL_PWM_EVENT_COMPARE_CH1,   /**< 比较通道 1 */
    XY_HAL_PWM_EVENT_COMPARE_CH2,   /**< 比较通道 2 */
    XY_HAL_PWM_EVENT_COMPARE_CH3,   /**< 比较通道 3 */
    XY_HAL_PWM_EVENT_COMPARE_CH4,   /**< 比较通道 4 */
    XY_HAL_PWM_EVENT_FAULT,         /**< 故障事件 */
    XY_HAL_PWM_EVENT_ERROR,         /**< 错误事件 */
} xy_hal_pwm_evt_t;

/**
 * @brief PWM 回调类型
 */
typedef void (*xy_hal_pwm_callback_t)(void *pwm, xy_hal_pwm_evt_t event, void *arg);

/**
 * @brief 初始化 PWM
 * @param pwm PWM 实例
 * @param channel 通道
 * @param config 配置结构
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_pwm_init(void *pwm, xy_hal_pwm_channel_t channel,
                              const xy_hal_pwm_config_t *config);

/**
 * @brief 反初始化 PWM
 * @param pwm PWM 实例
 * @param channel 通道
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_pwm_deinit(void *pwm, xy_hal_pwm_channel_t channel);

/**
 * @brief 启动 PWM 输出
 * @param pwm PWM 实例
 * @param channel 通道
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_pwm_start(void *pwm, xy_hal_pwm_channel_t channel);

/**
 * @brief 停止 PWM 输出
 * @param pwm PWM 实例
 * @param channel 通道
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_pwm_stop(void *pwm, xy_hal_pwm_channel_t channel);

/**
 * @brief 设置 PWM 频率
 * @param pwm PWM 实例
 * @param channel 通道
 * @param frequency 频率 (Hz)
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_pwm_set_frequency(void *pwm, xy_hal_pwm_channel_t channel,
                                       uint32_t frequency);

/**
 * @brief 获取 PWM 频率
 * @param pwm PWM 实例
 * @param channel 通道
 * @return 频率 (Hz)，负值表示错误
 */
int32_t xy_hal_pwm_get_frequency(void *pwm, xy_hal_pwm_channel_t channel);

/**
 * @brief 设置 PWM 占空比
 * @param pwm PWM 实例
 * @param channel 通道
 * @param duty_cycle 占空比 (0-10000 = 0.00%-100.00%)
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_pwm_set_duty_cycle(void *pwm, xy_hal_pwm_channel_t channel,
                                       uint32_t duty_cycle);

/**
 * @brief 获取 PWM 占空比
 * @param pwm PWM 实例
 * @param channel 通道
 * @return 占空比 (0-10000)，负值表示错误
 */
int32_t xy_hal_pwm_get_duty_cycle(void *pwm, xy_hal_pwm_channel_t channel);

/**
 * @brief 设置 PWM 极性
 * @param pwm PWM 实例
 * @param channel 通道
 * @param polarity 极性
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_pwm_set_polarity(void *pwm, xy_hal_pwm_channel_t channel,
                                      xy_hal_pwm_polarity_t polarity);

/**
 * @brief 获取 PWM 极性
 * @param pwm PWM 实例
 * @param channel 通道
 * @return 极性，负值表示错误
 */
int32_t xy_hal_pwm_get_polarity(void *pwm, xy_hal_pwm_channel_t channel);

/**
 * @brief 设置 PWM 模式
 * @param pwm PWM 实例
 * @param channel 通道
 * @param mode 模式
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_pwm_set_mode(void *pwm, xy_hal_pwm_channel_t channel,
                                 xy_hal_pwm_mode_t mode);

/**
 * @brief 获取 PWM 模式
 * @param pwm PWM 实例
 * @param channel 通道
 * @return 模式，负值表示错误
 */
int32_t xy_hal_pwm_get_mode(void *pwm, xy_hal_pwm_channel_t channel);

/**
 * @brief 设置死区时间
 * @param pwm PWM 实例
 * @param channel 通道
 * @param deadtime_ns 死区时间 (纳秒)
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_pwm_set_deadtime(void *pwm, xy_hal_pwm_channel_t channel,
                                      uint32_t deadtime_ns);

/**
 * @brief 获取死区时间
 * @param pwm PWM 实例
 * @param channel 通道
 * @return 死区时间 (纳秒)，负值表示错误
 */
int32_t xy_hal_pwm_get_deadtime(void *pwm, xy_hal_pwm_channel_t channel);

/**
 * @brief 使能互补输出
 * @param pwm PWM 实例
 * @param channel 通道
 * @param config 互补输出配置
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_pwm_enable_complementary(void *pwm, xy_hal_pwm_channel_t channel,
                                              const xy_hal_pwm_complementary_config_t *config);

/**
 * @brief 禁用互补输出
 * @param pwm PWM 实例
 * @param channel 通道
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_pwm_disable_complementary(void *pwm, xy_hal_pwm_channel_t channel);

/**
 * @brief 设置比较值
 * @param pwm PWM 实例
 * @param channel 通道
 * @param compare 比较值
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_pwm_set_compare(void *pwm, xy_hal_pwm_channel_t channel,
                                     uint32_t compare);

/**
 * @brief 获取比较值
 * @param pwm PWM 实例
 * @param channel 通道
 * @return 比较值，负值表示错误
 */
int32_t xy_hal_pwm_get_compare(void *pwm, xy_hal_pwm_channel_t channel);

/**
 * @brief 设置自动重载值
 * @param pwm PWM 实例
 * @param channel 通道
 * @param autoreload 自动重载值
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_pwm_set_autoreload(void *pwm, xy_hal_pwm_channel_t channel,
                                       uint32_t autoreload);

/**
 * @brief 获取自动重载值
 * @param pwm PWM 实例
 * @param channel 通道
 * @return 自动重载值，负值表示错误
 */
int32_t xy_hal_pwm_get_autoreload(void *pwm, xy_hal_pwm_channel_t channel);

/**
 * @brief 设置预分频
 * @param pwm PWM 实例
 * @param channel 通道
 * @param prescaler 预分频值
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_pwm_set_prescaler(void *pwm, xy_hal_pwm_channel_t channel,
                                       uint32_t prescaler);

/**
 * @brief 获取预分频
 * @param pwm PWM 实例
 * @param channel 通道
 * @return 预分频值，负值表示错误
 */
int32_t xy_hal_pwm_get_prescaler(void *pwm, xy_hal_pwm_channel_t channel);

/**
 * @brief 设置输出状态
 * @param pwm PWM 实例
 * @param channel 通道
 * @param state 状态 (1=使能, 0=禁用)
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_pwm_set_output_state(void *pwm, xy_hal_pwm_channel_t channel,
                                          uint8_t state);

/**
 * @brief 获取输出状态
 * @param pwm PWM 实例
 * @param channel 通道
 * @return 状态 (1=使能, 0=禁用)，负值表示错误
 */
int32_t xy_hal_pwm_get_output_state(void *pwm, xy_hal_pwm_channel_t channel);

/**
 * @brief 设置故障状态
 * @param pwm PWM 实例
 * @param channel 通道
 * @param fault_state 故障状态
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_pwm_set_fault_state(void *pwm, xy_hal_pwm_channel_t channel,
                                         uint8_t fault_state);

/**
 * @brief 获取故障状态
 * @param pwm PWM 实例
 * @param channel 通道
 * @return 故障状态，负值表示错误
 */
int32_t xy_hal_pwm_get_fault_state(void *pwm, xy_hal_pwm_channel_t channel);

/**
 * @brief 注册 PWM 回调
 * @param pwm PWM 实例
 * @param channel 通道
 * @param callback 回调函数
 * @param arg 用户参数
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_pwm_register_callback(void *pwm, xy_hal_pwm_channel_t channel,
                                           xy_hal_pwm_callback_t callback,
                                           void *arg);

/**
 * @brief PWM 控制
 * @param pwm PWM 实例
 * @param channel 通道
 * @param cmd 控制命令
 * @param args 命令参数
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_pwm_control(void *pwm, xy_hal_pwm_channel_t channel,
                                 uint32_t cmd, void *args);

/**
 * @brief 占空比转比较值
 * @param pwm PWM 实例
 * @param channel 通道
 * @param duty_cycle 占空比 (0-10000)
 * @return 比较值
 */
uint32_t xy_hal_pwm_duty_to_compare(void *pwm, xy_hal_pwm_channel_t channel,
                                   uint32_t duty_cycle);

/**
 * @brief 比较值转占空比
 * @param pwm PWM 实例
 * @param channel 通道
 * @param compare 比较值
 * @return 占空比 (0-10000)
 */
uint32_t xy_hal_pwm_compare_to_duty(void *pwm, xy_hal_pwm_channel_t channel,
                                   uint32_t compare);

/**
 * @brief 获取 PWM 周期 (微秒)
 * @param pwm PWM 实例
 * @param channel 通道
 * @return 周期 (μs)
 */
uint32_t xy_hal_pwm_get_period_us(void *pwm, xy_hal_pwm_channel_t channel);

/**
 * @brief 获取 PWM 周期 (纳秒)
 * @param pwm PWM 实例
 * @param channel 通道
 * @return 周期 (ns)
 */
uint64_t xy_hal_pwm_get_period_ns(void *pwm, xy_hal_pwm_channel_t channel);

/**
 * @brief 设置 PWM 波形形状
 * @param pwm PWM 实例
 * @param channel 通道
 * @param shape 波形形状
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_pwm_set_wave_shape(void *pwm, xy_hal_pwm_channel_t channel,
                                       xy_hal_pwm_wave_shape_t shape);

/**
 * @brief 获取 PWM 波形形状
 * @param pwm PWM 实例
 * @param channel 通道
 * @return 波形形状，负值表示错误
 */
int32_t xy_hal_pwm_get_wave_shape(void *pwm, xy_hal_pwm_channel_t channel);

/**
 * @brief 配置 PWM 脉冲数模式
 * @param pwm PWM 实例
 * @param channel 通道
 * @param pulses 脉冲数
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_pwm_config_pulse_mode(void *pwm, xy_hal_pwm_channel_t channel,
                                           uint32_t pulses);

/**
 * @brief 使能 PWM 脉冲模式
 * @param pwm PWM 实例
 * @param channel 通道
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_pwm_enable_pulse_mode(void *pwm, xy_hal_pwm_channel_t channel);

/**
 * @brief 禁用 PWM 脉冲模式
 * @param pwm PWM 实例
 * @param channel 通道
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_pwm_disable_pulse_mode(void *pwm, xy_hal_pwm_channel_t channel);

/**
 * @brief 获取 PWM 状态
 * @param pwm PWM 实例
 * @param channel 通道
 * @return 状态，负值表示错误
 */
int32_t xy_hal_pwm_get_state(void *pwm, xy_hal_pwm_channel_t channel);

/**
 * @brief PWM 输出同步
 * @param pwm PWM 实例
 * @param channel_mask 通道掩码
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_pwm_sync_output(void *pwm, uint8_t channel_mask);

/**
 * @brief PWM 输入捕获
 * @param pwm PWM 实例
 * @param channel 通道
 * @param timeout 超时时间 (ms)
 * @return 捕获值，负值表示错误
 */
int32_t xy_hal_pwm_input_capture(void *pwm, xy_hal_pwm_channel_t channel,
                                 uint32_t timeout);

/**
 * @brief 设置输入捕获极性
 * @param pwm PWM 实例
 * @param channel 通道
 * @param polarity 极性
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_pwm_set_capture_polarity(void *pwm, xy_hal_pwm_channel_t channel,
                                              xy_hal_pwm_polarity_t polarity);

/**
 * @brief 获取输入捕获极性
 * @param pwm PWM 实例
 * @param channel 通道
 * @return 极性，负值表示错误
 */
int32_t xy_hal_pwm_get_capture_polarity(void *pwm, xy_hal_pwm_channel_t channel);

/**
 * @brief 配置 PWM 编码器模式
 * @param pwm PWM 实例
 * @param channel 通道
 * @param mode 编码器模式
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_pwm_config_encoder_mode(void *pwm, xy_hal_pwm_channel_t channel,
                                             uint8_t mode);

/**
 * @brief 获取编码器计数
 * @param pwm PWM 实例
 * @return 编码器计数，负值表示错误
 */
int32_t xy_hal_pwm_get_encoder_count(void *pwm);

/**
 * @brief 设置编码器计数
 * @param pwm PWM 实例
 * @param count 编码器计数
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_pwm_set_encoder_count(void *pwm, int32_t count);

/**
 * @brief 重置编码器计数
 * @param pwm PWM 实例
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_pwm_reset_encoder_count(void *pwm);

/**
 * @brief 配置故障保护
 * @param pwm PWM 实例
 * @param channel 通道
 * @param fault_config 故障配置
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_pwm_config_fault_protection(void *pwm, xy_hal_pwm_channel_t channel,
                                                 const void *fault_config);

/**
 * @brief 清除故障状态
 * @param pwm PWM 实例
 * @param channel 通道
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_pwm_clear_fault(void *pwm, xy_hal_pwm_channel_t channel);

/**
 * @brief 检查故障状态
 * @param pwm PWM 实例
 * @param channel 通道
 * @return 1=故障，0=正常，负值表示错误
 */
int32_t xy_hal_pwm_check_fault(void *pwm, xy_hal_pwm_channel_t channel);

#ifdef __cplusplus
}
#endif

#endif /* XY_HAL_PWM_H */
