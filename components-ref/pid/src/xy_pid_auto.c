/**
 * @file xy_pid_auto.c
 * @brief PID Auto-Tuning Tool Implementation
 * @version 1.0.0
 * @date 2026-03-01 自主学习
 */

#include "xy_pid_auto.h"
#include "xy_log.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

#define DEFAULT_STEP_AMPLITUDE    50.0F
#define DEFAULT_SAMPLE_INTERVAL   100     /* ms */
#define DEFAULT_NUM_SAMPLES       100
#define DEFAULT_TOLERANCE         0.01F

int xy_pid_auto_init(xy_pid_auto_tuner_t *tuner, xy_pid_t *pid,
                     const xy_pid_auto_config_t *config)
{
    if (!tuner || !pid) {
        return XY_PID_AUTO_INVALID_PARAM;
    }

    memset(tuner, 0, sizeof(*tuner));
    tuner->pid = pid;

    /* 默认配置 */
    tuner->config.method = XY_PID_AUTO_METHOD_ZN;
    tuner->config.step_amplitude = DEFAULT_STEP_AMPLITUDE;
    tuner->config.sample_interval_ms = DEFAULT_SAMPLE_INTERVAL;
    tuner->config.num_samples = DEFAULT_NUM_SAMPLES;
    tuner->config.tolerance = DEFAULT_TOLERANCE;

    if (config) {
        if (config->step_amplitude > 0) {
            tuner->config.step_amplitude = config->step_amplitude;
        }
        if (config->sample_interval_ms > 0) {
            tuner->config.sample_interval_ms = config->sample_interval_ms;
        }
        if (config->num_samples > 0) {
            tuner->config.num_samples = config->num_samples;
        }
        tuner->config.method = config->method;
        tuner->config.tolerance = config->tolerance;
    }

    /* 分配采样缓冲区 */
    tuner->samples = malloc(tuner->config.num_samples * sizeof(float));
    if (!tuner->samples) {
        return XY_PID_AUTO_ERROR;
    }

    tuner->state = XY_PID_AUTO_STATE_IDLE;
    tuner->initialized = true;

    xy_log_i("PID Auto-Tuner initialized (method=%d, samples=%d)\n",
             tuner->config.method, tuner->config.num_samples);
    return XY_PID_AUTO_OK;
}

int xy_pid_auto_deinit(xy_pid_auto_tuner_t *tuner)
{
    if (!tuner) {
        return XY_PID_AUTO_INVALID_PARAM;
    }

    if (tuner->samples) {
        free(tuner->samples);
        tuner->samples = NULL;
    }

    tuner->initialized = false;
    return XY_PID_AUTO_OK;
}

int xy_pid_auto_start(xy_pid_auto_tuner_t *tuner)
{
    if (!tuner || !tuner->initialized) {
        return XY_PID_AUTO_INVALID_PARAM;
    }

    /* 重置 PID 为手动模式 */
    xy_pid_set_mode(tuner->pid, XY_PID_MODE_MANUAL);
    xy_pid_reset(tuner->pid);

    /* 重置整定器状态 */
    tuner->state = XY_PID_AUTO_STATE_MEASURING;
    tuner->sample_count = 0;
    tuner->start_time = xy_os_tick_get();
    tuner->output_step = tuner->config.step_amplitude;
    tuner->process_var = 0;
    tuner->last_pv = 0;

    /* 设置初始输出 (阶跃) */
    xy_pid_set_output_limits(tuner->pid, tuner->output_step, tuner->output_step);

    xy_log_i("PID Auto-Tuning started (step=%.2f)\n", tuner->output_step);
    return XY_PID_AUTO_OK;
}

int xy_pid_auto_stop(xy_pid_auto_tuner_t *tuner)
{
    if (!tuner) {
        return XY_PID_AUTO_INVALID_PARAM;
    }

    tuner->state = XY_PID_AUTO_STATE_IDLE;
    xy_log_i("PID Auto-Tuning stopped\n");
    return XY_PID_AUTO_OK;
}

/**
 * @brief Ziegler-Nichols 整定法计算
 */
static void xy_pid_auto_calc_zn(xy_pid_auto_tuner_t *tuner)
{
    float *samples = tuner->samples;
    uint16_t n = tuner->sample_count;

    /* 简化实现：基于阶跃响应 */
    /* 计算稳态增益 */
    float final_value = samples[n - 1];
    float gain = final_value / tuner->output_step;

    /* 计算上升时间 (10%-90%) */
    float t10 = 0, t90 = 0;
    float target_10 = final_value * 0.1F;
    float target_90 = final_value * 0.9F;

    for (uint16_t i = 0; i < n; i++) {
        if (samples[i] >= target_10 && t10 == 0) {
            t10 = i * tuner->config.sample_interval_ms;
        }
        if (samples[i] >= target_90 && t90 == 0) {
            t90 = i * tuner->config.sample_interval_ms;
            break;
        }
    }

    float rise_time = t90 - t10;

    /* Z-N 参数计算 (阶跃响应法) */
    float L = t10;  /* 滞后时间 */
    float T = rise_time * 1.3F;  /* 时间常数 */
    float K = gain;

    /* PID 参数 (Z-N 推荐) */
    tuner->result.kp = 1.2F * T / (K * L);
    tuner->result.ki = 2.0F * L;
    tuner->result.kd = 0.5F * L;

    tuner->result.rise_time = rise_time;

    xy_log_i("Z-N Calculation: Kp=%.3f, Ki=%.3f, Kd=%.3f\n",
             tuner->result.kp, tuner->result.ki, tuner->result.kd);
}

int xy_pid_auto_loop(xy_pid_auto_tuner_t *tuner, float process_var)
{
    if (!tuner || tuner->state != XY_PID_AUTO_STATE_MEASURING) {
        return XY_PID_AUTO_NOT_READY;
    }

    tuner->process_var = process_var;
    uint32_t elapsed = xy_os_tick_get() - tuner->start_time;

    /* 检查采样间隔 */
    if (elapsed < tuner->config.sample_interval_ms) {
        return XY_PID_AUTO_OK;
    }

    /* 存储采样 */
    if (tuner->sample_count < tuner->config.num_samples) {
        tuner->samples[tuner->sample_count++] = process_var;
        tuner->start_time = xy_os_tick_get();  /* 重置计时 */

        xy_log_d("Sample %d: PV=%.2f\n", tuner->sample_count, process_var);
    }

    /* 检查是否完成采样 */
    if (tuner->sample_count >= tuner->config.num_samples) {
        tuner->state = XY_PID_AUTO_STATE_CALCULATING;

        /* 计算 PID 参数 */
        switch (tuner->config.method) {
            case XY_PID_AUTO_METHOD_ZN:
                xy_pid_auto_calc_zn(tuner);
                break;
            /* 其他方法 (Cohen-Coon, IMC 等) */
            /* 简化实现：使用 Z-N 方法 */
            default:
                xy_pid_auto_calc_zn(tuner);
                break;
        }

        tuner->state = XY_PID_AUTO_STATE_COMPLETE;
        xy_log_i("PID Auto-Tuning complete!\n");
    }

    return XY_PID_AUTO_OK;
}

xy_pid_auto_state_t xy_pid_auto_get_state(const xy_pid_auto_tuner_t *tuner)
{
    if (!tuner) {
        return XY_PID_AUTO_STATE_ERROR;
    }
    return tuner->state;
}

int xy_pid_auto_get_result(const xy_pid_auto_tuner_t *tuner,
                           xy_pid_auto_result_t *result)
{
    if (!tuner || !result) {
        return XY_PID_AUTO_INVALID_PARAM;
    }

    if (tuner->state != XY_PID_AUTO_STATE_COMPLETE) {
        return XY_PID_AUTO_NOT_READY;
    }

    *result = tuner->result;
    return XY_PID_AUTO_OK;
}

int xy_pid_auto_apply(xy_pid_auto_tuner_t *tuner)
{
    if (!tuner || tuner->state != XY_PID_AUTO_STATE_COMPLETE) {
        return XY_PID_AUTO_NOT_READY;
    }

    /* 应用整定结果到 PID */
    xy_pid_set_tuning(tuner->pid, tuner->result.kp,
                      tuner->result.kp / tuner->result.ki,
                      tuner->result.kp * tuner->result.kd);

    /* 切换回自动模式 */
    xy_pid_set_mode(tuner->pid, XY_PID_MODE_AUTO);

    xy_log_i("PID tuning parameters applied\n");
    return XY_PID_AUTO_OK;
}

float xy_pid_auto_get_progress(const xy_pid_auto_tuner_t *tuner)
{
    if (!tuner || tuner->sample_count == 0) {
        return 0.0F;
    }

    return (float)tuner->sample_count / tuner->config.num_samples * 100.0F;
}
