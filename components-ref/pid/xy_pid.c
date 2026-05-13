/**
 * @file xy_pid.c
 * @brief PID Controller with Advanced Features Implementation
 * @version 2.0.0
 * @date 2026-03-01 早晨
 */

#include "xy_pid.h"
#include "xy_log.h"
#include <string.h>
#include <math.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/**
 * @brief 限幅函数
 */
static inline float xy_pid_clamp(float value, float min, float max)
{
    if (value < min) {
        return min;
    } else if (value > max) {
        return max;
    }
    return value;
}

int xy_pid_init(xy_pid_t *pid, const xy_pid_config_t *config)
{
    if (!pid || !config) {
        return XY_PID_INVALID_PARAM;
    }
    
    memset(pid, 0, sizeof(*pid));
    
    /* 复制配置 */
    memcpy(&pid->config, config, sizeof(xy_pid_config_t));
    
    /* 默认设置 */
    pid->mode = XY_PID_MODE_MANUAL;
    pid->first_run = true;
    pid->anti_windup = true;
    pid->derivative_filter = true;
    
    if (pid->config.derivative_filter <= 0) {
        pid->config.derivative_filter = 0.1F;  /* 默认 0.1 */
    }
    
    xy_log_i("PID initialized: Kp=%.3f, Ki=%.3f, Kd=%.3f\n",
             pid->config.kp, pid->config.ki, pid->config.kd);
    
    return XY_PID_OK;
}

int xy_pid_reset(xy_pid_t *pid)
{
    if (!pid) {
        return XY_PID_INVALID_PARAM;
    }
    
    pid->setpoint = 0;
    pid->input = 0;
    pid->output = 0;
    pid->error = 0;
    pid->error_prev = 0;
    pid->error_sum = 0;
    pid->derivative = 0;
    pid->integral = 0;
    pid->integral_raw = 0;
    pid->first_run = true;
    pid->update_count = 0;
    
    return XY_PID_OK;
}

int xy_pid_set_tuning(xy_pid_t *pid, float kp, float ki, float kd)
{
    if (!pid) {
        return XY_PID_INVALID_PARAM;
    }
    
    pid->config.kp = kp;
    pid->config.ki = ki;
    pid->config.kd = kd;
    
    xy_log_d("PID tuning updated: Kp=%.3f, Ki=%.3f, Kd=%.3f\n", kp, ki, kd);
    return XY_PID_OK;
}

int xy_pid_set_output_limits(xy_pid_t *pid, float min, float max)
{
    if (!pid || min >= max) {
        return XY_PID_INVALID_PARAM;
    }
    
    pid->config.output_min = min;
    pid->config.output_max = max;
    
    /* 同时设置积分限幅 */
    if (pid->anti_windup) {
        pid->config.integral_min = min;
        pid->config.integral_max = max;
    }
    
    return XY_PID_OK;
}

int xy_pid_set_setpoint(xy_pid_t *pid, float setpoint)
{
    if (!pid) {
        return XY_PID_INVALID_PARAM;
    }
    
    pid->setpoint = setpoint;
    return XY_PID_OK;
}

int xy_pid_set_input(xy_pid_t *pid, float input)
{
    if (!pid) {
        return XY_PID_INVALID_PARAM;
    }
    
    pid->input = input;
    return XY_PID_OK;
}

int xy_pid_compute(xy_pid_t *pid, float input, float *output)
{
    float dt;
    float p_term, i_term, d_term;
    uint32_t current_time;
    
    if (!pid || !output) {
        return XY_PID_INVALID_PARAM;
    }
    
    pid->input = input;
    pid->error = pid->setpoint - pid->input;
    
    /* 获取时间间隔 */
    current_time = xy_os_tick_get();
    
    if (pid->first_run) {
        pid->last_update = current_time;
        pid->error_prev = pid->error;
        pid->first_run = false;
        *output = pid->output;
        return XY_PID_OK;
    }
    
    dt = (float)(current_time - pid->last_update);
    
    if (dt <= 0) {
        *output = pid->output;
        return XY_PID_OK;
    }
    
    /* 比例项 */
    p_term = pid->config.kp * pid->error;
    
    /* 积分项 */
    pid->integral_raw += pid->error * dt;
    
    if (pid->anti_windup) {
        /* 抗积分饱和 */
        pid->integral_raw = xy_pid_clamp(pid->integral_raw, 
                                         pid->config.integral_min,
                                         pid->config.integral_max);
    }
    
    i_term = pid->config.ki * pid->integral_raw;
    
    /* 微分项 (带滤波) */
    float derivative_raw = (pid->error - pid->error_prev) / dt;
    
    if (pid->derivative_filter) {
        /* 一阶低通滤波 */
        pid->derivative = (1.0F - pid->config.derivative_filter) * pid->derivative +
                         pid->config.derivative_filter * derivative_raw;
    } else {
        pid->derivative = derivative_raw;
    }
    
    d_term = pid->config.kd * pid->derivative;
    
    /* 计算总输出 */
    pid->output = p_term + i_term + d_term;
    
    /* 输出限幅 */
    pid->output = xy_pid_clamp(pid->output, 
                               pid->config.output_min, 
                               pid->config.output_max);
    
    /* 更新状态 */
    pid->error_prev = pid->error;
    pid->last_update = current_time;
    pid->update_count++;
    
    *output = pid->output;
    
    return XY_PID_OK;
}

float xy_pid_get_error(const xy_pid_t *pid)
{
    if (!pid) {
        return 0;
    }
    return pid->error;
}

float xy_pid_get_integral(const xy_pid_t *pid)
{
    if (!pid) {
        return 0;
    }
    return pid->integral;
}

float xy_pid_get_derivative(const xy_pid_t *pid)
{
    if (!pid) {
        return 0;
    }
    return pid->derivative;
}

int xy_pid_set_mode(xy_pid_t *pid, xy_pid_mode_t mode)
{
    if (!pid) {
        return XY_PID_INVALID_PARAM;
    }
    
    pid->mode = mode;
    
    if (mode == XY_PID_MODE_AUTO) {
        xy_log_d("PID switched to AUTO mode\n");
    } else {
        xy_log_d("PID switched to MANUAL mode\n");
    }
    
    return XY_PID_OK;
}

xy_pid_mode_t xy_pid_get_mode(const xy_pid_t *pid)
{
    if (!pid) {
        return XY_PID_MODE_MANUAL;
    }
    return pid->mode;
}

int xy_pid_enable_anti_windup(xy_pid_t *pid, bool enable)
{
    if (!pid) {
        return XY_PID_INVALID_PARAM;
    }
    
    pid->anti_windup = enable;
    
    if (enable) {
        /* 启用时设置积分限幅 */
        pid->config.integral_min = pid->config.output_min;
        pid->config.integral_max = pid->config.output_max;
    }
    
    xy_log_d("Anti-windup %s\n", enable ? "enabled" : "disabled");
    return XY_PID_OK;
}

int xy_pid_enable_derivative_filter(xy_pid_t *pid, bool enable, float coef)
{
    if (!pid || coef < 0 || coef > 1) {
        return XY_PID_INVALID_PARAM;
    }
    
    pid->derivative_filter = enable;
    
    if (enable && coef > 0) {
        pid->config.derivative_filter = coef;
    }
    
    xy_log_d("Derivative filter %s (coef=%.3f)\n", 
             enable ? "enabled" : "disabled", pid->config.derivative_filter);
    return XY_PID_OK;
}
