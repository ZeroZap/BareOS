/**
 * SPDX-License-Identifier: MIT
 * @file    xy_actuator.h
 * @brief   Actuator Framework Header - XinYi Framework
 * @version 1.0.0
 *
 * 执行器框架 - 舵机、继电器、电机等输出设备
 */

#ifndef __XY_ACTUATOR_H__
#define __XY_ACTUATOR_H__

#include <stdint.h>
#include <stdbool.h>

/* ==================== 前置声明 ==================== */
struct actuator_device;

/* ==================== 错误码 ==================== */
typedef enum {
    ACTUATOR_EOK       =  0,
    ACTUATOR_ERROR     = -1,
    ACTUATOR_EINVAL    = -2,
    ACTUATOR_ENODEV    = -3,
    ACTUATOR_EBUSY     = -4,
    ACTUATOR_ETIMEOUT  = -5,
    ACTUATOR_ENOMEM    = -6,
    ACTUATOR_ENOSYS    = -7,
    ACTUATOR_EIO       = -8,
    ACTUATOR_ELIMIT    = -9,     ///< 超出限制
    ACTUATOR_EHW       = -10,    ///< 硬件错误
} actuator_err_t;

/* ==================== 执行器类型 ==================== */
typedef enum {
    ACTUATOR_TYPE_NONE         = 0x00,
    ACTUATOR_TYPE_RELAY,                ///< 继电器 (开关)
    ACTUATOR_TYPE_SERVO,                ///< 舵机 (角度控制)
    ACTUATOR_TYPE_MOTOR_DC,             ///< 直流电机
    ACTUATOR_TYPE_MOTOR_STEPPER,        ///< 步进电机
    ACTUATOR_TYPE_SOLENOID,             ///< 电磁阀
    ACTUATOR_TYPE_PWM,                  ///< 通用 PWM 输出
    ACTUATOR_TYPE_LED,                  ///< LED 控制
    ACTUATOR_TYPE_BUZZER,               ///< 蜂鸣器
    ACTUATOR_TYPE_VALVE,                ///< 阀门
    ACTUATOR_TYPE_CUSTOM       = 0xFF,
} actuator_type_t;

/* ==================== 执行器状态 ==================== */
typedef enum {
    ACTUATOR_STATUS_IDLE      = 0x00,
    ACTUATOR_STATUS_READY     = 0x01,
    ACTUATOR_STATUS_BUSY      = 0x02,    ///< 动作执行中
    ACTUATOR_STATUS_ERROR     = 0x03,
    ACTUATOR_STATUS_DISABLED  = 0x04,
} actuator_status_t;

/* ==================== 继电器状态 ==================== */
typedef enum {
    RELAY_STATE_OFF   = 0,
    RELAY_STATE_ON    = 1,
    RELAY_STATE_TOGGLE = 2,   ///< 翻转
} relay_state_t;

/* ==================== 舵机角度 ==================== */
typedef struct {
    float target_angle;       ///< 目标角度 (度)
    float current_angle;      ///< 当前角度 (读回)
    float min_angle;          ///< 最小角度 (默认 -90°)
    float max_angle;          ///< 最大角度 (默认 +90°)
    uint16_t pwm_min;         ///< PWM 最小占空比 (默认 500)
    uint16_t pwm_max;         ///< PWM 最大占空比 (默认 2500)
    uint16_t pwm_center;      ///< PWM 中点 (默认 1500)
    uint32_t speed;           ///< 速度 (度/秒)
} servo_angle_t;

/* ==================== 执行器数据联合 ==================== */
typedef union {
    /* 继电器 */
    struct {
        uint8_t state;        ///< 0=关, 1=开
        uint8_t channel;     ///< 通道号
    } relay;

    /* 舵机 */
    servo_angle_t servo;

    /* PWM */
    struct {
        uint16_t duty;       ///< 占空比 0-65535
        uint32_t period;     ///< 周期 (ns)
    } pwm;

    /* 直流电机 */
    struct {
        int16_t speed;       ///< 速度 -1000~1000 (‰)
        uint8_t dir;         ///< 方向 0=正向, 1=反向
    } motor_dc;

    /* 步进电机 */
    struct {
        int32_t target_pos;      ///< 目标位置
        int32_t current_pos;     ///< 当前位置
        uint32_t steps_per_rev;  ///< 每转步数
    } motor_stepper;

    /* 通用数值 */
    int32_t value_int;
    float value_float;
} actuator_value_t;

/* ==================== 执行器配置 ==================== */
typedef struct {
    /* 硬件配置 */
    uint8_t gpio_pin;                ///< GPIO 引脚
    uint8_t gpio_port;               ///< GPIO 端口
    uint8_t channel;                ///< 通道号 (多路继电器/舵机)
    uint8_t pwm_channel;            ///< PWM 通道

    /* PWM 配置 */
    uint32_t pwm_freq;               ///< PWM 频率 (Hz)
    uint16_t pwm_resolution;         ///< PWM 分辨率 (位数)

    /* 舵机配置 */
    float servo_min_angle;           ///< 舵机最小角度
    float servo_max_angle;           ///< 舵机最大角度
    uint16_t servo_pwm_min;          ///< 舵机 PWM 最小 (us)
    uint16_t servo_pwm_max;          ///< 舵机 PWM 最大 (us)
    uint32_t servo_speed;            ///< 舵机速度 (度/秒)

    /* 继电器配置 */
    bool active_high;                ///< 高电平激活 (true) 或低电平激活 (false)

    /* 安全限制 */
    uint32_t timeout_ms;             ///< 操作超时时间
    bool safety_limit_enable;        ///< 使能安全限制

    /* 电源配置 */
    uint8_t power_domain;          ///< 电源域

    /* 用户数据 */
    void *priv_data;
} actuator_config_t;

/* ==================== 执行器设备 ==================== */
typedef struct actuator_device {
    /* 基本信息 */
    char name[32];
    actuator_type_t type;
    actuator_status_t status;
    uint8_t id;

    /* 配置 */
    actuator_config_t config;

    /* 当前值 */
    actuator_value_t value;

    /* 操作接口 */
    const struct actuator_ops *ops;

    /* 总线/硬件句柄 */
    void *bus;

    /* 私有数据 */
    void *priv_data;

    /* 回调 */
    void (*callback)(struct actuator_device *dev, actuator_err_t result, void *user_data);
    void *user_data;

    /* 链表 */
    struct actuator_device *next;
} actuator_device_t;

/* ==================== 执行器操作接口 ==================== */
typedef struct actuator_ops {
    /* 基础操作 (必须实现) */
    actuator_err_t (*init)(actuator_device_t *dev);
    actuator_err_t (*deinit)(actuator_device_t *dev);

    /* 控制操作 (必须实现) */
    actuator_err_t (*write)(actuator_device_t *dev, const actuator_value_t *value);
    actuator_err_t (*read)(actuator_device_t *dev, actuator_value_t *value);  // 可选

    /* 使能控制 */
    actuator_err_t (*enable)(actuator_device_t *dev, bool enable);

    /* 配置 */
    actuator_err_t (*config)(actuator_device_t *dev, const actuator_config_t *config);
    actuator_err_t (*get_config)(actuator_device_t *dev, actuator_config_t *config);

    /* 状态查询 */
    actuator_status_t (*get_status)(actuator_device_t *dev);
    bool (*is_ready)(actuator_device_t *dev);

    /* 功耗管理 */
    actuator_err_t (*sleep)(actuator_device_t *dev);
    actuator_err_t (*wakeup)(actuator_device_t *dev);

    /* 特殊操作 */
    actuator_err_t (*reset)(actuator_device_t *dev);        // 复位到默认
    actuator_err_t (*emergency_stop)(actuator_device_t *dev); // 急停
} actuator_ops_t;

/* ==================== 继电器特定接口 ==================== */
typedef struct relay_ops {
    actuator_err_t (*init)(actuator_device_t *dev);
    actuator_err_t (*set)(actuator_device_t *dev, uint8_t state);
    actuator_err_t (*get)(actuator_device_t *dev, uint8_t *state);
    actuator_err_t (*toggle)(actuator_device_t *dev);
    actuator_err_t (*pulse)(actuator_device_t *dev, uint32_t pulse_width_ms);
} relay_ops_t;

/* ==================== 舵机特定接口 ==================== */
typedef struct servo_ops {
    actuator_err_t (*init)(actuator_device_t *dev);
    actuator_err_t (*set_angle)(actuator_device_t *dev, float angle);
    actuator_err_t (*get_angle)(actuator_device_t *dev, float *angle);
    actuator_err_t (*set_range)(actuator_device_t *dev, float min_angle, float max_angle);
    actuator_err_t (*set_speed)(actuator_device_t *dev, uint32_t speed_deg_per_sec);
    actuator_err_t (*sweep)(actuator_device_t *dev, float start_angle, float end_angle,
                            uint32_t step_ms);
    actuator_err_t (*stop)(actuator_device_t *dev);
    actuator_err_t (*center)(actuator_device_t *dev);
} servo_ops_t;

/* ==================== 设备注册宏 ==================== */
#define ACTUATOR_DEVICE_INIT(_name, _type, _ops, _bus, _priv) \
    {                                                           \
        .name     = _name,                                     \
        .type     = _type,                                      \
        .ops      = _ops,                                       \
        .bus      = _bus,                                       \
        .priv_data = _priv,                                     \
        .status   = ACTUATOR_STATUS_IDLE,                       \
    }

/* 继电器设备宏 */
#define RELAY_DEVICE_INIT(_name, _ops, _pin, _port, _ch, _active_high) \
    {                                                                   \
        .name = _name,                                                  \
        .type = ACTUATOR_TYPE_RELAY,                                    \
        .ops  = (const actuator_ops_t *)_ops,                           \
        .config.gpio_pin = _pin,                                         \
        .config.gpio_port = _port,                                       \
        .config.channel = _ch,                                           \
        .config.active_high = _active_high,                             \
    }

/* 舵机设备宏 */
#define SERVO_DEVICE_INIT(_name, _ops, _pwm_ch, _min, _max, _speed) \
    {                                                                 \
        .name = _name,                                                \
        .type = ACTUATOR_TYPE_SERVO,                                 \
        .ops  = (const actuator_ops_t *)_ops,                         \
        .config.pwm_channel = _pwm_ch,                                \
        .config.servo_min_angle = _min,                               \
        .config.servo_max_angle = _max,                              \
        .config.servo_speed = _speed,                                 \
        .config.servo_pwm_min = 500,                                  \
        .config.servo_pwm_max = 2500,                                 \
    }

/* ==================== 全局 API ==================== */
actuator_err_t actuator_register(actuator_device_t *dev);
actuator_err_t actuator_unregister(actuator_device_t *dev);
actuator_device_t *actuator_find(const char *name);
actuator_device_t *actuator_find_by_type(actuator_type_t type);
uint8_t actuator_get_count(void);
uint8_t actuator_get_count_by_type(actuator_type_t type);

/* ==================== 通用控制 API ==================== */
actuator_err_t actuator_init(actuator_device_t *dev);
actuator_err_t actuator_deinit(actuator_device_t *dev);
actuator_err_t actuator_enable(actuator_device_t *dev, bool enable);
actuator_err_t actuator_write(actuator_device_t *dev, const actuator_value_t *value);
actuator_err_t actuator_read(actuator_device_t *dev, actuator_value_t *value);
actuator_err_t actuator_reset(actuator_device_t *dev);
actuator_err_t actuator_emergency_stop(actuator_device_t *dev);
actuator_status_t actuator_get_status(actuator_device_t *dev);
const char *actuator_status_str(actuator_status_t status);

/* ==================== 继电器 API ==================== */
actuator_err_t relay_init(actuator_device_t *dev);
actuator_err_t relay_set(actuator_device_t *dev, uint8_t state);
actuator_err_t relay_on(actuator_device_t *dev);
actuator_err_t relay_off(actuator_device_t *dev);
actuator_err_t relay_toggle(actuator_device_t *dev);
actuator_err_t relay_get(actuator_device_t *dev, uint8_t *state);
actuator_err_t relay_pulse(actuator_device_t *dev, uint32_t pulse_width_ms);

/* ==================== 舵机 API ==================== */
actuator_err_t servo_init(actuator_device_t *dev);
actuator_err_t servo_set_angle(actuator_device_t *dev, float angle);
actuator_err_t servo_get_angle(actuator_device_t *dev, float *angle);
actuator_err_t servo_set_range(actuator_device_t *dev, float min_angle, float max_angle);
actuator_err_t servo_set_speed(actuator_device_t *dev, uint32_t speed);
actuator_err_t servo_sweep(actuator_device_t *dev, float start, float end, uint32_t step_ms);
actuator_err_t servo_stop(actuator_device_t *dev);
actuator_err_t servo_center(actuator_device_t *dev);

/* ==================== PWM API ==================== */
actuator_err_t pwm_set_duty(actuator_device_t *dev, uint16_t duty);
actuator_err_t pwm_set_frequency(actuator_device_t *dev, uint32_t freq);

/* ==================== 批量操作 ==================== */
actuator_err_t actuator_all_off(void);  // 所有执行器关闭
actuator_err_t actuator_emergency_stop_all(void);  // 全部急停

/* ==================== 工具函数 ==================== */
const char *actuator_type_str(actuator_type_t type);
const char *actuator_err_str(actuator_err_t err);

/* 将角度转换为 PWM 占空比 */
static inline uint16_t servo_angle_to_pwm(float angle, float min_angle, float max_angle,
                                          uint16_t pwm_min, uint16_t pwm_max)
{
    float ratio = (angle - min_angle) / (max_angle - min_angle);
    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;
    return (uint16_t)(pwm_min + ratio * (pwm_max - pwm_min));
}

/* 将 PWM 占空比转换为角度 */
static inline float servo_pwm_to_angle(uint16_t pwm, float min_angle, float max_angle,
                                       uint16_t pwm_min, uint16_t pwm_max)
{
    float ratio = (float)(pwm - pwm_min) / (float)(pwm_max - pwm_min);
    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;
    return min_angle + ratio * (max_angle - min_angle);
}

#endif /* __XY_ACTUATOR_H__ */