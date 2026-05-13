# C语言 PID 控制库 (支持定点/浮点切换)

通过宏定义 `PID_USE_FLOAT` 可在编译时选择使用**浮点数**或**定点数**运算。

## 📁 文件结构

```text
pid_lib/
├── pid_config.h   # 配置文件 (选择定点/浮点)
├── pid.h          # 头文件
├── pid.c          # 实现文件
└── pid_example.c  # 使用示例
```

------

## 📄 pid_config.h - 配置文件

```c
/**
 * @file pid_config.h
 * @brief PID库配置文件
 * @version 2.0
 * 
 * 在此文件中配置PID库的编译选项
 */

#ifndef __PID_CONFIG_H__
#define __PID_CONFIG_H__

/*============================================================================
 *                          编译配置选项
 *===========================================================================*/

/**
 * @brief 选择PID计算模式
 * 
 * 定义 PID_USE_FLOAT=1  : 使用浮点数运算 (需要FPU或软浮点库)
 * 定义 PID_USE_FLOAT=0  : 使用定点数运算 (纯整数运算，适合无FPU的MCU)
 * 
 * 也可以在编译时通过 -DPID_USE_FLOAT=1 或 -DPID_USE_FLOAT=0 指定
 */
#ifndef PID_USE_FLOAT
    #define PID_USE_FLOAT       0       /* 默认使用定点数 */
#endif

/**
 * @brief 定点数Q格式配置
 * 
 * Q16: 16位整数 + 16位小数, 范围约 ±32767.9999
 * Q12: 20位整数 + 12位小数, 范围约 ±524287.9997
 * Q8:  24位整数 + 8位小数,  范围约 ±8388607.996
 */
#ifndef PID_Q_BITS
    #define PID_Q_BITS          16      /* 默认Q16格式 */
#endif

/**
 * @brief 是否启用微分滤波
 */
#ifndef PID_ENABLE_DIFF_FILTER
    #define PID_ENABLE_DIFF_FILTER  1
#endif

/**
 * @brief 是否启用死区功能
 */
#ifndef PID_ENABLE_DEADBAND
    #define PID_ENABLE_DEADBAND     1
#endif

/**
 * @brief 是否启用调试输出
 */
#ifndef PID_ENABLE_DEBUG
    #define PID_ENABLE_DEBUG        0
#endif

#endif /* __PID_CONFIG_H__ */
```

------

## 📄 pid.h - 头文件

```c
/**
 * @file pid.h
 * @brief 高性能PID控制库 (支持定点/浮点切换)
 * @version 2.0
 * 
 * 特性:
 * - 编译时选择定点或浮点运算
 * - 支持多种PID算法
 * - 积分抗饱和
 * - 微分滤波
 * - 输出限幅
 */

#ifndef __PID_H__
#define __PID_H__

#include <stdint.h>
#include <stdbool.h>
#include "pid_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 *                          数据类型抽象层
 *===========================================================================*/

#if PID_USE_FLOAT
    /*---------- 浮点数模式 ----------*/
    typedef float pid_real_t;
    
    #define PID_REAL(x)             (x##f)
    #define PID_REAL_FROM_INT(x)    ((pid_real_t)(x))
    #define PID_REAL_TO_INT(x)      ((int32_t)(x))
    #define PID_REAL_MUL(a, b)      ((a) * (b))
    #define PID_REAL_DIV(a, b)      ((a) / (b))
    #define PID_REAL_ABS(x)         (((x) >= 0) ? (x) : -(x))
    
    /* 浮点数无需转换 */
    #define PID_FLOAT_TO_REAL(x)    (x)
    #define PID_REAL_TO_FLOAT(x)    (x)
    
    #define PID_MODE_STRING         "FLOAT"

#else
    /*---------- 定点数模式 ----------*/
    typedef int32_t pid_real_t;
    
    /* Q格式缩放因子 */
    #define PID_Q_SCALE             (1 << PID_Q_BITS)
    #define PID_Q_HALF              (1 << (PID_Q_BITS - 1))
    
    /* 定点数字面量 */
    #define PID_REAL(x)             ((pid_real_t)((x) * PID_Q_SCALE))
    #define PID_REAL_FROM_INT(x)    ((pid_real_t)((x) << PID_Q_BITS))
    #define PID_REAL_TO_INT(x)      ((int32_t)((x) >> PID_Q_BITS))
    
    /* 定点数乘法 (使用64位中间结果防止溢出) */
    #define PID_REAL_MUL(a, b)      ((pid_real_t)(((int64_t)(a) * (b)) >> PID_Q_BITS))
    
    /* 定点数除法 */
    #define PID_REAL_DIV(a, b)      ((pid_real_t)(((int64_t)(a) << PID_Q_BITS) / (b)))
    
    /* 绝对值 */
    #define PID_REAL_ABS(x)         (((x) >= 0) ? (x) : -(x))
    
    /* 浮点数与定点数转换 */
    #define PID_FLOAT_TO_REAL(x)    ((pid_real_t)((x) * PID_Q_SCALE))
    #define PID_REAL_TO_FLOAT(x)    ((float)(x) / PID_Q_SCALE)
    
    #define PID_MODE_STRING         "FIXED-POINT Q" #PID_Q_BITS

#endif /* PID_USE_FLOAT */

/* 通用常量 */
#define PID_ZERO                    PID_REAL(0.0)
#define PID_ONE                     PID_REAL(1.0)

/*============================================================================
 *                              类型定义
 *===========================================================================*/

/**
 * @brief PID算法类型枚举
 */
typedef enum {
    PID_TYPE_POSITIONAL = 0,    /* 位置式PID */
    PID_TYPE_INCREMENTAL,       /* 增量式PID */
    PID_TYPE_INTEGRAL_SEP,      /* 积分分离PID */
    PID_TYPE_ANTI_WINDUP,       /* 抗积分饱和PID (Back-calculation) */
    PID_TYPE_SETPOINT_WEIGHT,   /* 设定值加权PID */
    PID_TYPE_PESSEN_INTEGRAL,   /* Pessen积分规则 */
    PID_TYPE_MAX
} pid_type_t;

/**
 * @brief 微分模式枚举
 */
typedef enum {
    PID_DIFF_ON_ERROR = 0,      /* 对误差微分 */
    PID_DIFF_ON_MEASUREMENT     /* 对测量值微分 (避免设定值突变) */
} pid_diff_mode_t;

/**
 * @brief PID参数结构体
 */
typedef struct {
    pid_real_t kp;              /* 比例系数 */
    pid_real_t ki;              /* 积分系数 */
    pid_real_t kd;              /* 微分系数 */
} pid_params_t;

/**
 * @brief PID控制器结构体
 */
typedef struct {
    /* 算法配置 */
    pid_type_t      type;           /* 算法类型 */
    pid_diff_mode_t diff_mode;      /* 微分模式 */
    
    /* PID参数 */
    pid_params_t    params;         /* Kp, Ki, Kd */
    
    /* 状态变量 */
    pid_real_t      setpoint;       /* 设定值 */
    pid_real_t      feedback;       /* 反馈值 */
    pid_real_t      error;          /* 当前误差 */
    pid_real_t      error_prev;     /* 上次误差 */
    pid_real_t      error_prev2;    /* 上上次误差 (增量式用) */
    pid_real_t      integral;       /* 积分累积 */
    pid_real_t      derivative;     /* 微分项 */
    pid_real_t      output;         /* 输出值 */
    pid_real_t      output_prev;    /* 上次输出 (增量式用) */
    
    /* 限幅参数 */
    pid_real_t      output_max;     /* 输出上限 */
    pid_real_t      output_min;     /* 输出下限 */
    pid_real_t      integral_max;   /* 积分上限 */
    pid_real_t      integral_min;   /* 积分下限 */
    
    /* 高级功能参数 */
    pid_real_t      integral_sep_thresh;  /* 积分分离阈值 */
    pid_real_t      kb;             /* 抗饱和反馈系数 */
    pid_real_t      weight_b;       /* 比例项设定值权重 */
    pid_real_t      weight_c;       /* 微分项设定值权重 */
    
#if PID_ENABLE_DIFF_FILTER
    pid_real_t      diff_filter_alpha;    /* 微分滤波系数 (0~1) */
    pid_real_t      diff_filtered;        /* 滤波后的微分 */
#endif

#if PID_ENABLE_DEADBAND
    pid_real_t      deadband;       /* 死区范围 */
#endif
    
    /* 采样时间 */
    pid_real_t      dt;             /* 采样周期 (秒) */
    
    /* 状态标志 */
    uint8_t         initialized : 1;    /* 初始化标志 */
    uint8_t         enabled     : 1;    /* 使能标志 */
    uint8_t         saturated   : 1;    /* 饱和标志 */
    uint8_t         first_run   : 1;    /* 首次运行标志 */
    
} pid_controller_t;

/**
 * @brief PID初始化配置结构体 (使用浮点数便于用户配置)
 */
typedef struct {
    pid_type_t  type;           /* 算法类型 */
    float       kp;             /* 比例系数 */
    float       ki;             /* 积分系数 */
    float       kd;             /* 微分系数 */
    float       dt;             /* 采样周期(秒) */
    float       output_max;     /* 输出上限 */
    float       output_min;     /* 输出下限 */
    float       integral_max;   /* 积分上限 */
    float       integral_min;   /* 积分下限 */
} pid_config_t;

/*============================================================================
 *                              API函数声明
 *===========================================================================*/

/* 初始化与重置 */
int  pid_init(pid_controller_t *pid, const pid_config_t *config);
void pid_reset(pid_controller_t *pid);
void pid_enable(pid_controller_t *pid, bool enable);

/* 参数设置 - 浮点数接口 (便于用户使用) */
void pid_set_params(pid_controller_t *pid, float kp, float ki, float kd);
void pid_set_setpoint(pid_controller_t *pid, float setpoint);
void pid_set_output_limits(pid_controller_t *pid, float min, float max);
void pid_set_integral_limits(pid_controller_t *pid, float min, float max);
void pid_set_dt(pid_controller_t *pid, float dt);

/* 参数设置 - 原生类型接口 (高性能) */
void pid_set_params_real(pid_controller_t *pid, pid_real_t kp, pid_real_t ki, pid_real_t kd);
void pid_set_setpoint_real(pid_controller_t *pid, pid_real_t setpoint);

/* 高级功能设置 */
void pid_set_type(pid_controller_t *pid, pid_type_t type);
void pid_set_diff_mode(pid_controller_t *pid, pid_diff_mode_t mode);
void pid_set_integral_separation(pid_controller_t *pid, float threshold);
void pid_set_anti_windup(pid_controller_t *pid, float kb);
void pid_set_setpoint_weight(pid_controller_t *pid, float b, float c);

#if PID_ENABLE_DIFF_FILTER
void pid_set_diff_filter(pid_controller_t *pid, float alpha);
#endif

#if PID_ENABLE_DEADBAND
void pid_set_deadband(pid_controller_t *pid, float deadband);
#endif

/* PID计算 - 浮点数接口 */
float pid_compute(pid_controller_t *pid, float feedback);

/* PID计算 - 原生类型接口 (高性能) */
pid_real_t pid_compute_real(pid_controller_t *pid, pid_real_t feedback);

/* 状态获取 */
float pid_get_error(pid_controller_t *pid);
float pid_get_output(pid_controller_t *pid);
float pid_get_integral(pid_controller_t *pid);
void  pid_get_components(pid_controller_t *pid, float *p_out, float *i_out, float *d_out);
bool  pid_is_saturated(pid_controller_t *pid);
const char* pid_get_mode_string(void);
const char* pid_get_type_string(pid_type_t type);

/*============================================================================
 *                          各算法独立函数 (可单独调用)
 *===========================================================================*/

pid_real_t pid_calc_positional(pid_controller_t *pid);
pid_real_t pid_calc_incremental(pid_controller_t *pid);
pid_real_t pid_calc_integral_separation(pid_controller_t *pid);
pid_real_t pid_calc_anti_windup(pid_controller_t *pid);
pid_real_t pid_calc_setpoint_weight(pid_controller_t *pid);
pid_real_t pid_calc_pessen_integral(pid_controller_t *pid);

#ifdef __cplusplus
}
#endif

#endif /* __PID_H__ */
```

------

## 📄 pid.c - 实现文件

```c
/**
 * @file pid.c
 * @brief PID控制库实现 (支持定点/浮点切换)
 */

#include "pid.h"
#include <string.h>

/*============================================================================
 *                              内部辅助函数
 *===========================================================================*/

/**
 * @brief 限幅函数
 */
static inline pid_real_t pid_clamp(pid_real_t value, pid_real_t min, pid_real_t max)
{
    if (value > max) return max;
    if (value < min) return min;
    return value;
}

#if PID_ENABLE_DEADBAND
/**
 * @brief 死区处理
 */
static inline pid_real_t pid_apply_deadband(pid_real_t error, pid_real_t deadband)
{
    if (PID_REAL_ABS(error) < deadband) {
        return PID_ZERO;
    }
    return error;
}
#endif

#if PID_ENABLE_DIFF_FILTER
/**
 * @brief 微分滤波 (一阶低通滤波)
 * y[n] = alpha * x[n] + (1 - alpha) * y[n-1]
 */
static inline pid_real_t pid_filter_derivative(pid_controller_t *pid, pid_real_t raw_diff)
{
    pid_real_t one_minus_alpha = PID_ONE - pid->diff_filter_alpha;
    pid->diff_filtered = PID_REAL_MUL(pid->diff_filter_alpha, raw_diff) + 
                         PID_REAL_MUL(one_minus_alpha, pid->diff_filtered);
    return pid->diff_filtered;
}
#endif

/*============================================================================
 *                              初始化与控制
 *===========================================================================*/

int pid_init(pid_controller_t *pid, const pid_config_t *config)
{
    if (pid == NULL || config == NULL) {
        return -1;
    }
    
    /* 清零结构体 */
    memset(pid, 0, sizeof(pid_controller_t));
    
    /* 设置算法类型 */
    pid->type = config->type;
    pid->diff_mode = PID_DIFF_ON_ERROR;
    
    /* 转换PID参数 */
    pid->params.kp = PID_FLOAT_TO_REAL(config->kp);
    pid->params.ki = PID_FLOAT_TO_REAL(config->ki);
    pid->params.kd = PID_FLOAT_TO_REAL(config->kd);
    
    /* 采样周期 */
    pid->dt = PID_FLOAT_TO_REAL(config->dt);
    
    /* 输出限幅 */
    pid->output_max = PID_FLOAT_TO_REAL(config->output_max);
    pid->output_min = PID_FLOAT_TO_REAL(config->output_min);
    
    /* 积分限幅 */
    pid->integral_max = PID_FLOAT_TO_REAL(config->integral_max);
    pid->integral_min = PID_FLOAT_TO_REAL(config->integral_min);
    
    /* 默认高级参数 */
    pid->integral_sep_thresh = PID_FLOAT_TO_REAL(100.0f);
    pid->kb = PID_ONE;
    pid->weight_b = PID_ONE;
    pid->weight_c = PID_ONE;
    
#if PID_ENABLE_DIFF_FILTER
    pid->diff_filter_alpha = PID_FLOAT_TO_REAL(0.1f);
    pid->diff_filtered = PID_ZERO;
#endif

#if PID_ENABLE_DEADBAND
    pid->deadband = PID_ZERO;
#endif
    
    pid->initialized = 1;
    pid->enabled = 1;
    pid->first_run = 1;
    
    return 0;
}

void pid_reset(pid_controller_t *pid)
{
    if (pid == NULL) return;
    
    pid->error = PID_ZERO;
    pid->error_prev = PID_ZERO;
    pid->error_prev2 = PID_ZERO;
    pid->integral = PID_ZERO;
    pid->derivative = PID_ZERO;
    pid->output = PID_ZERO;
    pid->output_prev = PID_ZERO;
    pid->saturated = 0;
    pid->first_run = 1;
    
#if PID_ENABLE_DIFF_FILTER
    pid->diff_filtered = PID_ZERO;
#endif
}

void pid_enable(pid_controller_t *pid, bool enable)
{
    if (pid == NULL) return;
    pid->enabled = enable ? 1 : 0;
    if (!enable) {
        pid_reset(pid);
    }
}

/*============================================================================
 *                              参数设置函数
 *===========================================================================*/

void pid_set_params(pid_controller_t *pid, float kp, float ki, float kd)
{
    if (pid == NULL) return;
    pid->params.kp = PID_FLOAT_TO_REAL(kp);
    pid->params.ki = PID_FLOAT_TO_REAL(ki);
    pid->params.kd = PID_FLOAT_TO_REAL(kd);
}

void pid_set_params_real(pid_controller_t *pid, pid_real_t kp, pid_real_t ki, pid_real_t kd)
{
    if (pid == NULL) return;
    pid->params.kp = kp;
    pid->params.ki = ki;
    pid->params.kd = kd;
}

void pid_set_setpoint(pid_controller_t *pid, float setpoint)
{
    if (pid == NULL) return;
    pid->setpoint = PID_FLOAT_TO_REAL(setpoint);
}

void pid_set_setpoint_real(pid_controller_t *pid, pid_real_t setpoint)
{
    if (pid == NULL) return;
    pid->setpoint = setpoint;
}

void pid_set_output_limits(pid_controller_t *pid, float min, float max)
{
    if (pid == NULL) return;
    pid->output_min = PID_FLOAT_TO_REAL(min);
    pid->output_max = PID_FLOAT_TO_REAL(max);
}

void pid_set_integral_limits(pid_controller_t *pid, float min, float max)
{
    if (pid == NULL) return;
    pid->integral_min = PID_FLOAT_TO_REAL(min);
    pid->integral_max = PID_FLOAT_TO_REAL(max);
}

void pid_set_dt(pid_controller_t *pid, float dt)
{
    if (pid == NULL) return;
    pid->dt = PID_FLOAT_TO_REAL(dt);
}

void pid_set_type(pid_controller_t *pid, pid_type_t type)
{
    if (pid == NULL || type >= PID_TYPE_MAX) return;
    pid->type = type;
    pid_reset(pid);  /* 切换算法时重置状态 */
}

void pid_set_diff_mode(pid_controller_t *pid, pid_diff_mode_t mode)
{
    if (pid == NULL) return;
    pid->diff_mode = mode;
}

void pid_set_integral_separation(pid_controller_t *pid, float threshold)
{
    if (pid == NULL) return;
    pid->integral_sep_thresh = PID_FLOAT_TO_REAL(threshold);
}

void pid_set_anti_windup(pid_controller_t *pid, float kb)
{
    if (pid == NULL) return;
    pid->kb = PID_FLOAT_TO_REAL(kb);
}

void pid_set_setpoint_weight(pid_controller_t *pid, float b, float c)
{
    if (pid == NULL) return;
    pid->weight_b = PID_FLOAT_TO_REAL(b);
    pid->weight_c = PID_FLOAT_TO_REAL(c);
}

#if PID_ENABLE_DIFF_FILTER
void pid_set_diff_filter(pid_controller_t *pid, float alpha)
{
    if (pid == NULL) return;
    if (alpha < 0.0f) alpha = 0.0f;
    if (alpha > 1.0f) alpha = 1.0f;
    pid->diff_filter_alpha = PID_FLOAT_TO_REAL(alpha);
}
#endif

#if PID_ENABLE_DEADBAND
void pid_set_deadband(pid_controller_t *pid, float deadband)
{
    if (pid == NULL) return;
    pid->deadband = PID_FLOAT_TO_REAL(deadband);
}
#endif

/*============================================================================
 *                              PID算法实现
 *===========================================================================*/

/**
 * @brief 位置式PID
 * u(k) = Kp*e(k) + Ki*∫e(k)dt + Kd*de(k)/dt
 */
pid_real_t pid_calc_positional(pid_controller_t *pid)
{
    pid_real_t p_term, i_term, d_term;
    pid_real_t raw_diff;
    
    /* P项 */
    p_term = PID_REAL_MUL(pid->params.kp, pid->error);
    
    /* I项 */
    pid->integral += PID_REAL_MUL(pid->error, pid->dt);
    pid->integral = pid_clamp(pid->integral, pid->integral_min, pid->integral_max);
    i_term = PID_REAL_MUL(pid->params.ki, pid->integral);
    
    /* D项 */
    if (pid->diff_mode == PID_DIFF_ON_MEASUREMENT) {
        /* 对测量值微分 */
        raw_diff = PID_REAL_DIV(pid->feedback - pid->error_prev, pid->dt);
        raw_diff = PID_ZERO - raw_diff;
    } else {
        /* 对误差微分 */
        raw_diff = PID_REAL_DIV(pid->error - pid->error_prev, pid->dt);
    }
    
#if PID_ENABLE_DIFF_FILTER
    pid->derivative = pid_filter_derivative(pid, raw_diff);
#else
    pid->derivative = raw_diff;
#endif
    d_term = PID_REAL_MUL(pid->params.kd, pid->derivative);
    
    /* 输出 */
    pid->output = p_term + i_term + d_term;
    
    return pid->output;
}

/**
 * @brief 增量式PID
 * Δu(k) = Kp*(e(k)-e(k-1)) + Ki*e(k)*dt + Kd*(e(k)-2*e(k-1)+e(k-2))/dt
 * u(k) = u(k-1) + Δu(k)
 */
pid_real_t pid_calc_incremental(pid_controller_t *pid)
{
    pid_real_t delta_p, delta_i, delta_d;
    pid_real_t delta_output;
    
    /* ΔP */
    delta_p = PID_REAL_MUL(pid->params.kp, pid->error - pid->error_prev);
    
    /* ΔI */
    delta_i = PID_REAL_MUL(pid->params.ki, PID_REAL_MUL(pid->error, pid->dt));
    
    /* ΔD */
    pid_real_t diff_term = pid->error - pid->error_prev - pid->error_prev + pid->error_prev2;
    delta_d = PID_REAL_MUL(pid->params.kd, PID_REAL_DIV(diff_term, pid->dt));
    
    /* 增量 */
    delta_output = delta_p + delta_i + delta_d;
    
    /* 累加输出 */
    pid->output = pid->output_prev + delta_output;
    
    return pid->output;
}

/**
 * @brief 积分分离PID
 * 误差大于阈值时禁用积分，避免积分饱和
 */
pid_real_t pid_calc_integral_separation(pid_controller_t *pid)
{
    pid_real_t p_term, i_term, d_term;
    pid_real_t raw_diff;
    bool integral_enable;
    
    /* 判断是否启用积分 */
    integral_enable = (PID_REAL_ABS(pid->error) < pid->integral_sep_thresh);
    
    /* P项 */
    p_term = PID_REAL_MUL(pid->params.kp, pid->error);
    
    /* I项 (条件累积) */
    if (integral_enable) {
        pid->integral += PID_REAL_MUL(pid->error, pid->dt);
        pid->integral = pid_clamp(pid->integral, pid->integral_min, pid->integral_max);
    }
    i_term = PID_REAL_MUL(pid->params.ki, pid->integral);
    
    /* D项 */
    raw_diff = PID_REAL_DIV(pid->error - pid->error_prev, pid->dt);
#if PID_ENABLE_DIFF_FILTER
    pid->derivative = pid_filter_derivative(pid, raw_diff);
#else
    pid->derivative = raw_diff;
#endif
    d_term = PID_REAL_MUL(pid->params.kd, pid->derivative);
    
    pid->output = p_term + i_term + d_term;
    
    return pid->output;
}

/**
 * @brief 抗积分饱和PID (Back-calculation方法)
 * 输出饱和时通过反馈修正积分项
 */
pid_real_t pid_calc_anti_windup(pid_controller_t *pid)
{
    pid_real_t p_term, i_term, d_term;
    pid_real_t raw_diff;
    pid_real_t output_unsat;
    pid_real_t saturation_error;
    
    /* P项 */
    p_term = PID_REAL_MUL(pid->params.kp, pid->error);
    
    /* I项 */
    i_term = PID_REAL_MUL(pid->params.ki, pid->integral);
    
    /* D项 */
    raw_diff = PID_REAL_DIV(pid->error - pid->error_prev, pid->dt);
#if PID_ENABLE_DIFF_FILTER
    pid->derivative = pid_filter_derivative(pid, raw_diff);
#else
    pid->derivative = raw_diff;
#endif
    d_term = PID_REAL_MUL(pid->params.kd, pid->derivative);
    
    /* 未限幅输出 */
    output_unsat = p_term + i_term + d_term;
    
    /* 限幅 */
    pid->output = pid_clamp(output_unsat, pid->output_min, pid->output_max);
    
    /* 饱和误差 */
    saturation_error = pid->output - output_unsat;
    pid->saturated = (saturation_error != PID_ZERO) ? 1 : 0;
    
    /* 抗饱和积分更新: integral += (error + kb * sat_error) * dt */
    pid_real_t integral_input = pid->error + PID_REAL_MUL(pid->kb, saturation_error);
    pid->integral += PID_REAL_MUL(integral_input, pid->dt);
    pid->integral = pid_clamp(pid->integral, pid->integral_min, pid->integral_max);
    
    return pid->output;
}

/**
 * @brief 设定值加权PID
 * P项: Kp * (b*r - y)
 * I项: Ki * ∫(r - y)dt  
 * D项: Kd * d(c*r - y)/dt
 */
pid_real_t pid_calc_setpoint_weight(pid_controller_t *pid)
{
    pid_real_t p_term, i_term, d_term;
    pid_real_t p_error, d_error;
    pid_real_t raw_diff;
    static pid_real_t d_error_prev = 0;
    
    /* 加权误差 */
    p_error = PID_REAL_MUL(pid->weight_b, pid->setpoint) - pid->feedback;
    d_error = PID_REAL_MUL(pid->weight_c, pid->setpoint) - pid->feedback;
    
    /* P项 */
    p_term = PID_REAL_MUL(pid->params.kp, p_error);
    
    /* I项 (使用完整误差) */
    pid->integral += PID_REAL_MUL(pid->error, pid->dt);
    pid->integral = pid_clamp(pid->integral, pid->integral_min, pid->integral_max);
    i_term = PID_REAL_MUL(pid->params.ki, pid->integral);
    
    /* D项 */
    raw_diff = PID_REAL_DIV(d_error - d_error_prev, pid->dt);
    d_error_prev = d_error;
#if PID_ENABLE_DIFF_FILTER
    pid->derivative = pid_filter_derivative(pid, raw_diff);
#else
    pid->derivative = raw_diff;
#endif
    d_term = PID_REAL_MUL(pid->params.kd, pid->derivative);
    
    pid->output = p_term + i_term + d_term;
    
    return pid->output;
}

/**
 * @brief Pessen积分规则PID
 * 误差符号改变时才累积积分，减少超调
 */
pid_real_t pid_calc_pessen_integral(pid_controller_t *pid)
{
    pid_real_t p_term, i_term, d_term;
    pid_real_t raw_diff;
    bool sign_changed;
    
    /* 检测误差符号变化 */
    sign_changed = ((pid->error > PID_ZERO && pid->error_prev < PID_ZERO) ||
                    (pid->error < PID_ZERO && pid->error_prev > PID_ZERO));
    
    /* P项 */
    p_term = PID_REAL_MUL(pid->params.kp, pid->error);
    
    /* I项 (仅在符号改变时累积) */
    if (sign_changed || PID_REAL_ABS(pid->error) < pid->integral_sep_thresh) {
        pid->integral += PID_REAL_MUL(pid->error, pid->dt);
        pid->integral = pid_clamp(pid->integral, pid->integral_min, pid->integral_max);
    }
    i_term = PID_REAL_MUL(pid->params.ki, pid->integral);
    
    /* D项 */
    raw_diff = PID_REAL_DIV(pid->error - pid->error_prev, pid->dt);
#if PID_ENABLE_DIFF_FILTER
    pid->derivative = pid_filter_derivative(pid, raw_diff);
#else
    pid->derivative = raw_diff;
#endif
    d_term = PID_REAL_MUL(pid->params.kd, pid->derivative);
    
    pid->output = p_term + i_term + d_term;
    
    return pid->output;
}

/*============================================================================
 *                              主计算函数
 *===========================================================================*/

pid_real_t pid_compute_real(pid_controller_t *pid, pid_real_t feedback)
{
    if (pid == NULL || !pid->initialized || !pid->enabled) {
        return PID_ZERO;
    }
    
    /* 保存历史误差 */
    pid->error_prev2 = pid->error_prev;
    pid->error_prev = pid->error;
    
    /* 更新反馈值 */
    pid->feedback = feedback;
    
    /* 计算误差 */
    pid->error = pid->setpoint - pid->feedback;
    
    /* 首次运行时初始化历史值 */
    if (pid->first_run) {
        pid->error_prev = pid->error;
        pid->error_prev2 = pid->error;
        pid->first_run = 0;
    }
    
#if PID_ENABLE_DEADBAND
    /* 死区处理 */
    pid->error = pid_apply_deadband(pid->error, pid->deadband);
#endif
    
    /* 根据算法类型计算 */
    switch (pid->type) {
        case PID_TYPE_POSITIONAL:
            pid_calc_positional(pid);
            break;
            
        case PID_TYPE_INCREMENTAL:
            pid_calc_incremental(pid);
            break;
            
        case PID_TYPE_INTEGRAL_SEP:
            pid_calc_integral_separation(pid);
            break;
            
        case PID_TYPE_ANTI_WINDUP:
            pid_calc_anti_windup(pid);
            break;
            
        case PID_TYPE_SETPOINT_WEIGHT:
            pid_calc_setpoint_weight(pid);
            break;
            
        case PID_TYPE_PESSEN_INTEGRAL:
            pid_calc_pessen_integral(pid);
            break;
            
        default:
            pid_calc_positional(pid);
            break;
    }
    
    /* 输出限幅 (抗饱和模式已内部处理) */
    if (pid->type != PID_TYPE_ANTI_WINDUP) {
        pid_real_t output_before = pid->output;
        pid->output = pid_clamp(pid->output, pid->output_min, pid->output_max);
        pid->saturated = (pid->output != output_before) ? 1 : 0;
    }
    
    /* 保存输出 */
    pid->output_prev = pid->output;
    
    return pid->output;
}

float pid_compute(pid_controller_t *pid, float feedback)
{
    pid_real_t feedback_real = PID_FLOAT_TO_REAL(feedback);
    pid_real_t output_real = pid_compute_real(pid, feedback_real);
    return PID_REAL_TO_FLOAT(output_real);
}

/*============================================================================
 *                              状态获取函数
 *===========================================================================*/

float pid_get_error(pid_controller_t *pid)
{
    if (pid == NULL) return 0.0f;
    return PID_REAL_TO_FLOAT(pid->error);
}

float pid_get_output(pid_controller_t *pid)
{
    if (pid == NULL) return 0.0f;
    return PID_REAL_TO_FLOAT(pid->output);
}

float pid_get_integral(pid_controller_t *pid)
{
    if (pid == NULL) return 0.0f;
    return PID_REAL_TO_FLOAT(pid->integral);
}

void pid_get_components(pid_controller_t *pid, float *p_out, float *i_out, float *d_out)
{
    if (pid == NULL) return;
    
    if (p_out) {
        *p_out = PID_REAL_TO_FLOAT(PID_REAL_MUL(pid->params.kp, pid->error));
    }
    if (i_out) {
        *i_out = PID_REAL_TO_FLOAT(PID_REAL_MUL(pid->params.ki, pid->integral));
    }
    if (d_out) {
        *d_out = PID_REAL_TO_FLOAT(PID_REAL_MUL(pid->params.kd, pid->derivative));
    }
}

bool pid_is_saturated(pid_controller_t *pid)
{
    if (pid == NULL) return false;
    return pid->saturated ? true : false;
}

const char* pid_get_mode_string(void)
{
#if PID_USE_FLOAT
    return "FLOAT";
#else
    static char buf[24];
    snprintf(buf, sizeof(buf), "FIXED-POINT Q%d", PID_Q_BITS);
    return buf;
#endif
}

const char* pid_get_type_string(pid_type_t type)
{
    static const char* type_names[] = {
        "Positional",
        "Incremental", 
        "Integral Separation",
        "Anti-Windup",
        "Setpoint Weight",
        "Pessen Integral",
        "Unknown"
    };
    
    if (type >= PID_TYPE_MAX) {
        return type_names[PID_TYPE_MAX];
    }
    return type_names[type];
}
```

------

## 📄 pid_example.c - 使用示例

```c
/**
 * @file pid_example.c
 * @brief PID库使用示例
 */

#include <stdio.h>
#include <string.h>
#include "pid.h"

/*============================================================================
 *                              模拟被控系统
 *===========================================================================*/

typedef struct {
    float value;
    float tau;      /* 时间常数 */
    float gain;     /* 增益 */
} plant_t;

/* 一阶惯性系统仿真 */
float plant_update(plant_t *plant, float input, float dt)
{
    float alpha = dt / (plant->tau + dt);
    plant->value += alpha * (plant->gain * input - plant->value);
    return plant->value;
}

void plant_reset(plant_t *plant)
{
    plant->value = 0.0f;
}

/*============================================================================
 *                              测试函数
 *===========================================================================*/

void print_header(const char* title)
{
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║ %-58s ║\n", title);
    printf("╚════════════════════════════════════════════════════════════╝\n");
}

void test_basic_pid(void)
{
    print_header("基础PID测试 - 位置式");
    
    printf("计算模式: %s\n\n", pid_get_mode_string());
    
    pid_controller_t pid;
    pid_config_t config = {
        .type = PID_TYPE_POSITIONAL,
        .kp = 2.0f,
        .ki = 0.5f,
        .kd = 0.1f,
        .dt = 0.01f,
        .output_max = 100.0f,
        .output_min = -100.0f,
        .integral_max = 50.0f,
        .integral_min = -50.0f
    };
    
    pid_init(&pid, &config);
    pid_set_setpoint(&pid, 50.0f);
    
    plant_t plant = {.value = 0.0f, .tau = 0.5f, .gain = 1.0f};
    
    printf("%-8s %-12s %-12s %-12s %-12s\n", 
           "Time", "Setpoint", "Feedback", "Output", "Error");
    printf("──────────────────────────────────────────────────────────\n");
    
    for (int i = 0; i <= 200; i++) {
        float feedback = plant.value;
        float output = pid_compute(&pid, feedback);
        plant_update(&plant, output, 0.01f);
        
        if (i % 25 == 0) {
            printf("%-8.2f %-12.2f %-12.2f %-12.2f %-12.2f\n",
                   i * 0.01f, 50.0f, feedback, output, pid_get_error(&pid));
        }
    }
}

void test_all_algorithms(void)
{
    print_header("所有PID算法对比测试");
    
    printf("计算模式: %s\n\n", pid_get_mode_string());
    
    pid_config_t config = {
        .kp = 2.5f,
        .ki = 0.8f,
        .kd = 0.15f,
        .dt = 0.01f,
        .output_max = 100.0f,
        .output_min = -100.0f,
        .integral_max = 50.0f,
        .integral_min = -50.0f
    };
    
    plant_t plant = {.tau = 0.4f, .gain = 1.0f};
    
    for (int type = 0; type < PID_TYPE_MAX; type++) {
        pid_controller_t pid;
        config.type = (pid_type_t)type;
        
        pid_init(&pid, &config);
        pid_set_setpoint(&pid, 60.0f);
        
        /* 设置算法特定参数 */
        if (type == PID_TYPE_INTEGRAL_SEP || type == PID_TYPE_PESSEN_INTEGRAL) {
            pid_set_integral_separation(&pid, 15.0f);
        }
        if (type == PID_TYPE_ANTI_WINDUP) {
            pid_set_anti_windup(&pid, 0.8f);
        }
        if (type == PID_TYPE_SETPOINT_WEIGHT) {
            pid_set_setpoint_weight(&pid, 0.6f, 0.0f);
        }
        
        plant_reset(&plant);
        
        printf("\n【%s】\n", pid_get_type_string((pid_type_t)type));
        printf("Time:    ");
        
        float final_value = 0;
        float max_overshoot = 0;
        
        for (int i = 0; i <= 150; i++) {
            float feedback = plant.value;
            float output = pid_compute(&pid, feedback);
            plant_update(&plant, output, 0.01f);
            
            /* 记录超调 */
            if (feedback > 60.0f && (feedback - 60.0f) > max_overshoot) {
                max_overshoot = feedback - 60.0f;
            }
            
            if (i % 30 == 0) {
                printf("%-6.2f ", i * 0.01f);
            }
            
            if (i == 150) {
                final_value = feedback;
            }
        }
        
        printf("\nOutput:  ");
        plant_reset(&plant);
        pid_reset(&pid);
        
        for (int i = 0; i <= 150; i++) {
            float feedback = plant.value;
            float output = pid_compute(&pid, feedback);
            plant_update(&plant, output, 0.01f);
            
            if (i % 30 == 0) {
                printf("%-6.2f ", feedback);
            }
        }
        
        printf("\n稳态值: %.2f, 最大超调: %.2f%%\n", 
               final_value, max_overshoot / 60.0f * 100.0f);
    }
}

void test_performance(void)
{
    print_header("性能测试");
    
    printf("计算模式: %s\n\n", pid_get_mode_string());
    
    pid_controller_t pid;
    pid_config_t config = {
        .type = PID_TYPE_POSITIONAL,
        .kp = 2.0f,
        .ki = 0.5f,
        .kd = 0.1f,
        .dt = 0.01f,
        .output_max = 100.0f,
        .output_min = -100.0f,
        .integral_max = 50.0f,
        .integral_min = -50.0f
    };
    
    pid_init(&pid, &config);
    pid_set_setpoint(&pid, 50.0f);
    
    /* 执行大量计算 */
    const int iterations = 100000;
    
#if PID_USE_FLOAT
    float feedback = 0.0f;
    printf("执行 %d 次浮点PID计算...\n", iterations);
    
    for (int i = 0; i < iterations; i++) {
        float output = pid_compute(&pid, feedback);
        feedback += output * 0.001f;
        if (feedback > 100.0f) feedback = 0.0f;
    }
#else
    pid_real_t feedback = PID_ZERO;
    printf("执行 %d 次定点PID计算...\n", iterations);
    
    for (int i = 0; i < iterations; i++) {
        pid_real_t output = pid_compute_real(&pid, feedback);
        feedback += output / 1000;
        if (feedback > PID_FLOAT_TO_REAL(100.0f)) feedback = PID_ZERO;
    }
#endif
    
    printf("计算完成!\n");
    printf("最终输出: %.4f\n", pid_get_output(&pid));
}

void test_setpoint_change(void)
{
    print_header("设定值阶跃响应测试");
    
    printf("计算模式: %s\n\n", pid_get_mode_string());
    
    pid_controller_t pid;
    pid_config_t config = {
        .type = PID_TYPE_ANTI_WINDUP,
        .kp = 3.0f,
        .ki = 1.0f,
        .kd = 0.2f,
        .dt = 0.01f,
        .output_max = 80.0f,
        .output_min = -80.0f,
        .integral_max = 40.0f,
        .integral_min = -40.0f
    };
    
    pid_init(&pid, &config);
    pid_set_anti_windup(&pid, 0.5f);
    
#if PID_ENABLE_DIFF_FILTER
    pid_set_diff_filter(&pid, 0.2f);
#endif
    
    plant_t plant = {.value = 0.0f, .tau = 0.3f, .gain = 1.0f};
    
    printf("%-8s %-12s %-12s %-12s %-10s\n", 
           "Time", "Setpoint", "Feedback", "Output", "Saturated");
    printf("──────────────────────────────────────────────────────────\n");
    
    float setpoint = 30.0f;
    pid_set_setpoint(&pid, setpoint);
    
    for (int i = 0; i <= 400; i++) {
        /* 在t=1.5s和t=3.0s时改变设定值 */
        if (i == 150) {
            setpoint = 70.0f;
            pid_set_setpoint(&pid, setpoint);
            printf(">>> 设定值变更为 %.1f <<<\n", setpoint);
        }
        if (i == 300) {
            setpoint = 40.0f;
            pid_set_setpoint(&pid, setpoint);
            printf(">>> 设定值变更为 %.1f <<<\n", setpoint);
        }
        
        float feedback = plant.value;
        float output = pid_compute(&pid, feedback);
        plant_update(&plant, output, 0.01f);
        
        if (i % 40 == 0) {
            printf("%-8.2f %-12.2f %-12.2f %-12.2f %-10s\n",
                   i * 0.01f, setpoint, feedback, output,
                   pid_is_saturated(&pid) ? "YES" : "NO");
        }
    }
}

void test_components(void)
{
    print_header("PID分量分析");
    
    printf("计算模式: %s\n\n", pid_get_mode_string());
    
    pid_controller_t pid;
    pid_config_t config = {
        .type = PID_TYPE_POSITIONAL,
        .kp = 2.0f,
        .ki = 0.5f,
        .kd = 0.3f,
        .dt = 0.01f,
        .output_max = 100.0f,
        .output_min = -100.0f,
        .integral_max = 30.0f,
        .integral_min = -30.0f
    };
    
    pid_init(&pid, &config);
    pid_set_setpoint(&pid, 50.0f);
    
    plant_t plant = {.value = 0.0f, .tau = 0.5f, .gain = 1.0f};
    
    printf("%-8s %-10s %-10s %-10s %-10s %-10s\n", 
           "Time", "Feedback", "P", "I", "D", "Output");
    printf("────────────────────────────────────────────────────────────\n");
    
    for (int i = 0; i <= 200; i++) {
        float feedback = plant.value;
        float output = pid_compute(&pid, feedback);
        plant_update(&plant, output, 0.01f);
        
        if (i % 20 == 0) {
            float p, i_comp, d;
            pid_get_components(&pid, &p, &i_comp, &d);
            
            printf("%-8.2f %-10.2f %-10.2f %-10.2f %-10.2f %-10.2f\n",
                   i * 0.01f, feedback, p, i_comp, d, output);
        }
    }
}

void print_config_info(void)
{
    print_header("PID库配置信息");
    
    printf("编译配置:\n");
    printf("  ├─ 计算模式:     %s\n", pid_get_mode_string());
    
#if !PID_USE_FLOAT
    printf("  ├─ Q格式位数:    Q%d\n", PID_Q_BITS);
    printf("  ├─ 缩放因子:     %d\n", PID_Q_SCALE);
#endif
    
#if PID_ENABLE_DIFF_FILTER
    printf("  ├─ 微分滤波:     启用\n");
#else
    printf("  ├─ 微分滤波:     禁用\n");
#endif

#if PID_ENABLE_DEADBAND
    printf("  ├─ 死区功能:     启用\n");
#else
    printf("  ├─ 死区功能:     禁用\n");
#endif
    
    printf("  └─ 支持算法:\n");
    for (int i = 0; i < PID_TYPE_MAX; i++) {
        printf("      %s─ [%d] %s\n", 
               (i == PID_TYPE_MAX - 1) ? "└" : "├",
               i, pid_get_type_string((pid_type_t)i));
    }
    
    printf("\n数据类型大小:\n");
    printf("  ├─ pid_real_t:        %zu bytes\n", sizeof(pid_real_t));
    printf("  ├─ pid_controller_t:  %zu bytes\n", sizeof(pid_controller_t));
    printf("  └─ pid_config_t:      %zu bytes\n", sizeof(pid_config_t));
}

/*============================================================================
 *                              主函数
 *===========================================================================*/

int main(void)
{
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║          PID控制库演示程序 v2.0                              ║\n");
    printf("║          支持定点/浮点编译切换                               ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    
    print_config_info();
    test_basic_pid();
    test_all_algorithms();
    test_setpoint_change();
    test_components();
    test_performance();
    
    printf("\n");
    printf("══════════════════════════════════════════════════════════════\n");
    printf("                      演示完成\n");
    printf("══════════════════════════════════════════════════════════════\n\n");
    
    return 0;
}
```

------

## 🔧 编译方法

### 使用浮点数模式

```bash
# 方法1: 修改 pid_config.h 中的 PID_USE_FLOAT 为 1

# 方法2: 编译时指定
gcc -DPID_USE_FLOAT=1 -o pid_demo_float pid.c pid_example.c -lm

# ARM Cortex-M4F (带FPU)
arm-none-eabi-gcc -DPID_USE_FLOAT=1 -mfpu=fpv4-sp-d16 -mfloat-abi=hard \
    -o pid_demo pid.c pid_example.c
```

### 使用定点数模式

```bash
# 方法1: 修改 pid_config.h 中的 PID_USE_FLOAT 为 0 (默认)

# 方法2: 编译时指定
gcc -DPID_USE_FLOAT=0 -o pid_demo_fixed pid.c pid_example.c

# 指定Q格式
gcc -DPID_USE_FLOAT=0 -DPID_Q_BITS=12 -o pid_demo_q12 pid.c pid_example.c

# ARM Cortex-M0 (无FPU)
arm-none-eabi-gcc -DPID_USE_FLOAT=0 -mcpu=cortex-m0 -o pid_demo pid.c pid_example.c
```

### Makefile 示例

```makefile
# Makefile for PID Library

CC = gcc
CFLAGS = -Wall -O2

# 选择模式: float 或 fixed
MODE ?= fixed

ifeq ($(MODE), float)
    CFLAGS += -DPID_USE_FLOAT=1
    TARGET = pid_demo_float
else
    CFLAGS += -DPID_USE_FLOAT=0
    TARGET = pid_demo_fixed
endif

# 可选: 指定Q格式位数 (仅定点模式有效)
ifdef Q_BITS
    CFLAGS += -DPID_Q_BITS=$(Q_BITS)
endif

SRCS = pid.c pid_example.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) pid_demo_float pid_demo_fixed

# 使用示例:
# make MODE=float          # 浮点模式
# make MODE=fixed          # 定点模式 (默认Q16)
# make MODE=fixed Q_BITS=12  # 定点Q12模式
```

------

## 📊 定点/浮点模式对比

|     特性     |     浮点模式      |      定点模式      |
| :----------: | :---------------: | :----------------: |
|   **精度**   | 高 (~7位有效数字) |  中 (取决于Q格式)  |
| **数值范围** |  大 (±3.4×10³⁸)   | 有限 (Q16: ±32767) |
| **运算速度** |   需FPU或软浮点   |   纯整数运算，快   |
| **代码大小** |       较大        |        较小        |
| **适用场景** |   有FPU的MCU/PC   |   无FPU的低端MCU   |

## 📋 Q格式选择指南

| Q格式 | 整数位 | 小数位 |   范围   |  精度   |    适用场景    |
| :---: | :----: | :----: | :------: | :-----: | :------------: |
|  Q8   |   24   |   8    | ±8388607 |  0.004  | 大范围、低精度 |
|  Q12  |   20   |   12   | ±524287  | 0.0002  |    平衡选择    |
|  Q16  |   16   |   16   |  ±32767  | 0.00002 | 高精度、小范围 |

------

## ✨ 库特性总结

|      特性      |                           说明                           |
| :------------: | :------------------------------------------------------: |
| **编译时切换** |                   通过宏选择定点或浮点                   |
|  **统一API**   |                   两种模式使用相同接口                   |
| **6种PID算法** | 位置式、增量式、积分分离、抗饱和、设定值加权、Pessen积分 |
| **可配置功能** |                 微分滤波、死区可独立开关                 |
|  **类型抽象**  |                  `pid_real_t` 自动适配                   |
|   **双接口**   |             浮点接口(易用) + 原生接口(高效)              |
|   **零依赖**   |                    定点模式无需数学库                    |