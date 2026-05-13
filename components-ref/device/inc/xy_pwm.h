/**
 * @file xy_pwm.h
 * @brief XinYi PWM Device Driver
 * @version 2.0
 * @date 2026-02-28
 */

#ifndef XY_PWM_H
#define XY_PWM_H

#include "xy_device.h"
#include "xy_hal_pwm.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief PWM 事件类型
 */
typedef enum {
    XY_PWM_EVT_UPDATE = 0,             /**< 更新事件 */
    XY_PWM_EVT_COMPARE_CH1,            /**< 比较通道 1 */
    XY_PWM_EVT_COMPARE_CH2,            /**< 比较通道 2 */
    XY_PWM_EVT_COMPARE_CH3,            /**< 比较通道 3 */
    XY_PWM_EVT_COMPARE_CH4,            /**< 比较通道 4 */
    XY_PWM_EVT_FAULT,                  /**< 故障事件 */
    XY_PWM_EVT_ERROR,                  /**< 错误事件 */
} xy_pwm_evt_t;

/**
 * @brief PWM 回调类型
 */
typedef void (*xy_pwm_callback_t)(void *dev, xy_pwm_evt_t evt, void *arg);

/**
 * @brief PWM 设备控制命令
 */
typedef enum {
    XY_PWM_CMD_SET_CONFIG = 0,         /**< 设置配置 */
    XY_PWM_CMD_GET_CONFIG,             /**< 获取配置 */
    XY_PWM_CMD_SET_FREQUENCY,          /**< 设置频率 */
    XY_PWM_CMD_GET_FREQUENCY,          /**< 获取频率 */
    XY_PWM_CMD_SET_DUTY_CYCLE,         /**< 设置占空比 */
    XY_PWM_CMD_GET_DUTY_CYCLE,         /**< 获取占空比 */
    XY_PWM_CMD_SET_POLARITY,           /**< 设置极性 */
    XY_PWM_CMD_GET_POLARITY,           /**< 获取极性 */
    XY_PWM_CMD_SET_DEADTIME,           /**< 设置死区时间 */
    XY_PWM_CMD_GET_DEADTIME,           /**< 获取死区时间 */
    XY_PWM_CMD_SET_MODE,               /**< 设置模式 */
    XY_PWM_CMD_GET_MODE,               /**< 获取模式 */
    XY_PWM_CMD_SET_CHANNEL_CONFIG,     /**< 设置通道配置 */
    XY_PWM_CMD_GET_CHANNEL_CONFIG,     /**< 获取通道配置 */
    XY_PWM_CMD_START,                  /**< 启动 PWM */
    XY_PWM_CMD_STOP,                   /**< 停止 PWM */
    XY_PWM_CMD_ENABLE_CHANNEL,         /**< 使能通道 */
    XY_PWM_CMD_DISABLE_CHANNEL,        /**< 禁用通道 */
    XY_PWM_CMD_SET_COMPARE_VALUE,      /**< 设置比较值 */
    XY_PWM_CMD_GET_COMPARE_VALUE,      /**< 获取比较值 */
    XY_PWM_CMD_SET_AUTORELOAD,         /**< 设置自动重载 */
    XY_PWM_CMD_GET_AUTORELOAD,         /**< 获取自动重载 */
    XY_PWM_CMD_SET_PRESCALER,          /**< 设置预分频 */
    XY_PWM_CMD_GET_PRESCALER,          /**< 获取预分频 */
    XY_PWM_CMD_SET_OUTPUT_STATE,       /**< 设置输出状态 */
    XY_PWM_CMD_GET_OUTPUT_STATE,       /**< 获取输出状态 */
} xy_pwm_cmd_t;

/**
 * @brief PWM 通道配置
 */
typedef struct {
    uint8_t channel;                   /**< 通道号 */
    uint32_t duty_cycle;               /**< 占空比 (0-10000 = 0.00%-100.00%) */
    xy_pwm_polarity_t polarity;        /**< 输出极性 */
    uint8_t enabled;                   /**< 是否使能 */
    uint8_t output_state;              /**< 输出状态 */
} xy_pwm_channel_config_t;

/**
 * @brief PWM 互补输出配置
 */
typedef struct {
    uint8_t enable;                    /**< 使能互补输出 */
    xy_pwm_polarity_t polarity;        /**< 互补输出极性 */
    uint32_t deadtime;                 /**< 死区时间 (ns) */
} xy_pwm_complementary_config_t;

/**
 * @brief 初始化 PWM 设备
 * @param dev 设备指针
 * @param config 配置结构
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_pwm_dev_init(void *dev, const xy_pwm_config_t *config);

/**
 * @brief 配置 PWM 通道
 * @param dev 设备指针
 * @param channel 通道配置
 * @param count 通道数量
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_pwm_dev_config_channels(void *dev, 
                                     const xy_pwm_channel_config_t *channels,
                                     size_t count);

/**
 * @brief 启动 PWM 输出
 * @param dev 设备指针
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_pwm_dev_start(void *dev);

/**
 * @brief 停止 PWM 输出
 * @param dev 设备指针
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_pwm_dev_stop(void *dev);

/**
 * @brief 设置 PWM 频率
 * @param dev 设备指针
 * @param freq 频率 (Hz)
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_pwm_dev_set_frequency(void *dev, uint32_t freq);

/**
 * @brief 获取 PWM 频率
 * @param dev 设备指针
 * @return 频率 (Hz)，负值表示错误
 */
int32_t xy_pwm_dev_get_frequency(void *dev);

/**
 * @brief 设置 PWM 占空比
 * @param dev 设备指针
 * @param channel 通道号
 * @param duty_cycle 占空比 (0-10000 = 0.00%-100.00%)
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_pwm_dev_set_duty_cycle(void *dev, uint8_t channel, uint32_t duty_cycle);

/**
 * @brief 获取 PWM 占空比
 * @param dev 设备指针
 * @param channel 通道号
 * @return 占空比 (0-10000)，负值表示错误
 */
int32_t xy_pwm_dev_get_duty_cycle(void *dev, uint8_t channel);

/**
 * @brief 设置 PWM 极性
 * @param dev 设备指针
 * @param channel 通道号
 * @param polarity 极性
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_pwm_dev_set_polarity(void *dev, uint8_t channel, 
                                  xy_pwm_polarity_t polarity);

/**
 * @brief 获取 PWM 极性
 * @param dev 设备指针
 * @param channel 通道号
 * @return 极性，负值表示错误
 */
int32_t xy_pwm_dev_get_polarity(void *dev, uint8_t channel);

/**
 * @brief 设置死区时间
 * @param dev 设备指针
 * @param channel 通道号
 * @param deadtime 死区时间 (ns)
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_pwm_dev_set_deadtime(void *dev, uint8_t channel, uint32_t deadtime);

/**
 * @brief 获取死区时间
 * @param dev 设备指针
 * @param channel 通道号
 * @return 死区时间 (ns)，负值表示错误
 */
int32_t xy_pwm_dev_get_deadtime(void *dev, uint8_t channel);

/**
 * @brief 使能 PWM 通道
 * @param dev 设备指针
 * @param channel 通道号
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_pwm_dev_enable_channel(void *dev, uint8_t channel);

/**
 * @brief 禁用 PWM 通道
 * @param dev 设备指针
 * @param channel 通道号
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_pwm_dev_disable_channel(void *dev, uint8_t channel);

/**
 * @brief 设置比较值
 * @param dev 设备指针
 * @param channel 通道号
 * @param compare 比较值
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_pwm_dev_set_compare(void *dev, uint8_t channel, uint32_t compare);

/**
 * @brief 获取比较值
 * @param dev 设备指针
 * @param channel 通道号
 * @return 比较值，负值表示错误
 */
int32_t xy_pwm_dev_get_compare(void *dev, uint8_t channel);

/**
 * @brief 设置自动重载值
 * @param dev 设备指针
 * @param value 自动重载值
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_pwm_dev_set_autoreload(void *dev, uint32_t value);

/**
 * @brief 获取自动重载值
 * @param dev 设备指针
 * @return 自动重载值，负值表示错误
 */
int32_t xy_pwm_dev_get_autoreload(void *dev);

/**
 * @brief 设置预分频值
 * @param dev 设备指针
 * @param prescaler 预分频值
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_pwm_dev_set_prescaler(void *dev, uint32_t prescaler);

/**
 * @brief 获取预分频值
 * @param dev 设备指针
 * @return 预分频值，负值表示错误
 */
int32_t xy_pwm_dev_get_prescaler(void *dev);

/**
 * @brief 配置互补输出
 * @param dev 设备指针
 * @param channel 通道号
 * @param config 互补输出配置
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_pwm_dev_config_complementary(void *dev, uint8_t channel,
                                          const xy_pwm_complementary_config_t *config);

/**
 * @brief 设置输出状态
 * @param dev 设备指针
 * @param channel 通道号
 * @param state 输出状态
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_pwm_dev_set_output_state(void *dev, uint8_t channel, uint8_t state);

/**
 * @brief 获取输出状态
 * @param dev 设备指针
 * @param channel 通道号
 * @return 输出状态，负值表示错误
 */
int32_t xy_pwm_dev_get_output_state(void *dev, uint8_t channel);

/**
 * @brief PWM 设备控制
 * @param dev 设备指针
 * @param cmd 控制命令
 * @param args 命令参数
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_pwm_dev_control(void *dev, uint32_t cmd, void *args);

/**
 * @brief 注册 PWM 回调
 * @param dev 设备指针
 * @param callback 回调函数
 * @param arg 用户参数
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_pwm_dev_register_callback(void *dev, 
                                       xy_pwm_callback_t callback, 
                                       void *arg);

/**
 * @brief 占空比转比较值
 * @param dev 设备指针
 * @param duty_cycle 占空比 (0-10000)
 * @return 比较值
 */
uint32_t xy_pwm_dev_duty_to_compare(void *dev, uint32_t duty_cycle);

/**
 * @brief 比较值转占空比
 * @param dev 设备指针
 * @param compare 比较值
 * @return 占空比 (0-10000)
 */
uint32_t xy_pwm_dev_compare_to_duty(void *dev, uint32_t compare);

/**
 * @brief 获取 PWM 周期 (微秒)
 * @param dev 设备指针
 * @return 周期 (μs)
 */
uint32_t xy_pwm_dev_get_period_us(void *dev);

/**
 * @brief 获取 PWM 周期 (纳秒)
 * @param dev 设备指针
 * @return 周期 (ns)
 */
uint64_t xy_pwm_dev_get_period_ns(void *dev);

/**
 * @brief 设置 PWM 波形形状
 * @param dev 设备指针
 * @param shape 波形形状
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_pwm_dev_set_wave_shape(void *dev, xy_pwm_wave_shape_t shape);

/**
 * @brief 获取 PWM 波形形状
 * @param dev 设备指针
 * @return 波形形状，负值表示错误
 */
int32_t xy_pwm_dev_get_wave_shape(void *dev);

/**
 * @brief 配置 PWM 脉冲数模式
 * @param dev 设备指针
 * @param pulses 脉冲数
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_pwm_dev_config_pulse_mode(void *dev, uint32_t pulses);

/**
 * @brief 使能 PWM 脉冲模式
 * @param dev 设备指针
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_pwm_dev_enable_pulse_mode(void *dev);

/**
 * @brief 禁用 PWM 脉冲模式
 * @param dev 设备指针
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_pwm_dev_disable_pulse_mode(void *dev);

#ifdef __cplusplus
}
#endif

#endif /* XY_PWM_H */
