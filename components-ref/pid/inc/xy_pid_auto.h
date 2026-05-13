#include "xy_pid_types.h"
/**
 * @file xy_pid_auto.h
 * @brief PID Auto-Tuning Tool - Ziegler-Nichols Method
 * @version 1.0.0
 * @date 2026-03-01 自主学习
 */

#ifndef XY_PID_AUTO_H
#define XY_PID_AUTO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_pid.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief 错误码
 */
#define XY_PID_AUTO_OK          0
#define XY_PID_AUTO_ERROR       (-1)
#define XY_PID_AUTO_INVALID_PARAM (-2)
#define XY_PID_AUTO_NOT_READY   (-3)
#define XY_PID_AUTO_TIMEOUT     (-4)

/**
 * @brief 自整定状态
 */
typedef enum {
    XY_PID_AUTO_STATE_IDLE = 0,
    XY_PID_AUTO_STATE_MEASURING,
    XY_PID_AUTO_STATE_CALCULATING,
    XY_PID_AUTO_STATE_COMPLETE,
    XY_PID_AUTO_STATE_ERROR,
} xy_pid_auto_state_t;

/**
 * @brief 整定方法
 */
typedef enum {
    XY_PID_AUTO_METHOD_ZN = 0,      /* Ziegler-Nichols */
    XY_PID_AUTO_METHOD_COHEN = 1,   /* Cohen-Coon */
    XY_PID_AUTO_METHOD_IMC = 2,     /* Internal Model Control */
} xy_pid_auto_method_t;

/**
 * @brief 自整定配置
 */
typedef struct {
    xy_pid_auto_method_t method;    /* 整定方法 */
    float step_amplitude;           /* 阶跃幅度 */
    uint32_t sample_interval_ms;    /* 采样间隔 (ms) */
    uint16_t num_samples;           /* 采样数量 */
    float tolerance;                /* 收敛容差 */
} xy_pid_auto_config_t;

/**
 * @brief 自整定结果
 */
typedef struct {
    float kp;                       /* 比例增益 */
    float ki;                       /* 积分增益 */
    float kd;                       /* 微分增益 */
    float ultimate_gain;            /* 临界增益 Ku */
    float ultimate_period;          /* 临界周期 Tu */
    float rise_time;                /* 上升时间 */
    float overshoot;                /* 超调量 */
    float settling_time;            /* 调节时间 */
} xy_pid_auto_result_t;

/**
 * @brief 自整定器句柄
 */
typedef struct {
    xy_pid_t *pid;                      /* PID 控制器 */
    xy_pid_auto_config_t config;        /* 配置 */
    xy_pid_auto_state_t state;          /* 状态 */
    xy_pid_auto_result_t result;        /* 结果 */
    
    float *samples;                     /* 采样数据 */
    uint16_t sample_count;              /* 当前采样数 */
    float output_step;                  /* 输出阶跃值 */
    uint32_t start_time;                /* 开始时间 */
    
    float process_var;                  /* 过程变量 */
    float setpoint;                     /* 设定点 */
    float last_pv;                      /* 上次过程变量 */
    
    bool initialized;                   /* 初始化标志 */
} xy_pid_auto_tuner_t;

/**
 * @brief 初始化自整定器
 */
int xy_pid_auto_init(xy_pid_auto_tuner_t *tuner, xy_pid_t *pid,
                     const xy_pid_auto_config_t *config);

/**
 * @brief 反初始化
 */
int xy_pid_auto_deinit(xy_pid_auto_tuner_t *tuner);

/**
 * @brief 启动自整定
 */
int xy_pid_auto_start(xy_pid_auto_tuner_t *tuner);

/**
 * @brief 停止自整定
 */
int xy_pid_auto_stop(xy_pid_auto_tuner_t *tuner);

/**
 * @brief 自整定主循环 (定期调用)
 */
int xy_pid_auto_loop(xy_pid_auto_tuner_t *tuner, float process_var);

/**
 * @brief 获取状态
 */
xy_pid_auto_state_t xy_pid_auto_get_state(const xy_pid_auto_tuner_t *tuner);

/**
 * @brief 获取结果
 */
int xy_pid_auto_get_result(const xy_pid_auto_tuner_t *tuner, 
                           xy_pid_auto_result_t *result);

/**
 * @brief 应用整定结果到 PID
 */
int xy_pid_auto_apply(xy_pid_auto_tuner_t *tuner);

/**
 * @brief 获取进度 (0-100%)
 */
float xy_pid_auto_get_progress(const xy_pid_auto_tuner_t *tuner);

#ifdef __cplusplus
}
#endif

#endif
