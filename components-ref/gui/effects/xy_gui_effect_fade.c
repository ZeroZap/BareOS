/**
 * @file xy_gui_effect_fade.c
 * @brief GUI Fade Effect Implementation - 淡入淡出效果实现
 * @version 1.0.0
 * @date 2026-03-13
 */

#include "xy_gui_effects.h"
#include <string.h>
#include <math.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* ==================== 内部函数 ==================== */

/**
 * @brief 获取系统运行时间(ms)
 */
static uint32_t get_time_ms(void)
{
    /* 简单实现: 使用静态计数器 */
    static uint32_t tick_count = 0;
    return tick_count++;
}

/**
 * @brief 线性插值
 */
static float lerp(float a, float b, float t)
{
    if (t <= 0.0f) return a;
    if (t >= 1.0f) return b;
    return a + (b - a) * t;
}

/**
 * @brief 缓出函数
 */
static float ease_out(float t)
{
    return 1.0f - powf(1.0f - t, 2.0f);
}

/* ==================== 淡入淡出效果实现 ==================== */

int xy_effect_fade_create(xy_effect_fade_t *fade, bool fade_in, uint32_t duration)
{
    if (!fade) return -1;

    memset(fade, 0, sizeof(*fade));

    fade->base.type = XY_EFFECT_TYPE_FADE;
    fade->base.state = XY_EFFECT_STATE_STOPPED;
    fade->base.duration = duration;
    fade->base.elapsed = 0;
    fade->base.repeat = 1;
    fade->base.current_repeat = 0;
    fade->base.progress = 0.0f;

    fade->fade_in = fade_in;
    fade->start_alpha = fade_in ? 0.0f : 1.0f;
    fade->end_alpha = fade_in ? 1.0f : 0.0f;

    return 0;
}

void xy_effect_fade_update(xy_effect_fade_t *fade, uint32_t dt)
{
    if (!fade) return;

    if (fade->base.state != XY_EFFECT_STATE_RUNNING) return;

    fade->base.elapsed += dt;

    /* 计算进度 */
    if (fade->base.duration > 0) {
        fade->base.progress = (float)fade->base.elapsed / fade->base.duration;
    } else {
        fade->base.progress = 1.0f;
    }

    /* 限制进度范围 */
    if (fade->base.progress > 1.0f) {
        fade->base.progress = 1.0f;
    }

    /* 检查是否完成 */
    if (fade->base.elapsed >= fade->base.duration) {
        fade->base.progress = 1.0f;

        /* 处理重复 */
        if (fade->base.repeat > 0) {
            fade->base.current_repeat++;
            if (fade->base.current_repeat >= fade->base.repeat) {
                fade->base.state = XY_EFFECT_STATE_STOPPED;
            } else {
                fade->base.elapsed = 0;
                fade->base.progress = 0.0f;
            }
        } else {
            /* 无限循环: 反转方向 */
            fade->base.elapsed = 0;
            fade->base.progress = 0.0f;
            fade->fade_in = !fade->fade_in;
            float temp = fade->start_alpha;
            fade->start_alpha = fade->end_alpha;
            fade->end_alpha = temp;
        }
    }
}

float xy_effect_fade_get_alpha(xy_effect_fade_t *fade)
{
    if (!fade) return 0.0f;

    /* 使用缓出效果 */
    float t = ease_out(fade->base.progress);
    return lerp(fade->start_alpha, fade->end_alpha, t);
}

/* ==================== 基础效果操作 ==================== */

void xy_effect_start(xy_effect_t *effect)
{
    if (!effect) return;
    effect->state = XY_EFFECT_STATE_RUNNING;
}

void xy_effect_stop(xy_effect_t *effect)
{
    if (!effect) return;
    effect->state = XY_EFFECT_STATE_STOPPED;
    effect->elapsed = 0;
    effect->progress = 0.0f;
    effect->current_repeat = 0;
}

void xy_effect_pause(xy_effect_t *effect)
{
    if (!effect) return;
    if (effect->state == XY_EFFECT_STATE_RUNNING) {
        effect->state = XY_EFFECT_STATE_PAUSED;
    }
}

void xy_effect_resume(xy_effect_t *effect)
{
    if (!effect) return;
    if (effect->state == XY_EFFECT_STATE_PAUSED) {
        effect->state = XY_EFFECT_STATE_RUNNING;
    }
}

void xy_effect_reset(xy_effect_t *effect)
{
    if (!effect) return;
    effect->elapsed = 0;
    effect->progress = 0.0f;
    effect->current_repeat = 0;
    effect->state = XY_EFFECT_STATE_STOPPED;
}

bool xy_effect_is_running(xy_effect_t *effect)
{
    if (!effect) return false;
    return effect->state == XY_EFFECT_STATE_RUNNING;
}

float xy_effect_get_progress(xy_effect_t *effect)
{
    if (!effect) return 0.0f;
    return effect->progress;
}

/* ==================== 工具函数 ==================== */

uint32_t xy_effect_get_time_ms(void)
{
    return get_time_ms();
}

float xy_effect_lerp(float a, float b, float t)
{
    return lerp(a, b, t);
}

float xy_effect_ease_in(float t)
{
    return t * t;
}

float xy_effect_ease_out(float t)
{
    return 1.0f - powf(1.0f - t, 2.0f);
}

float xy_effect_ease_in_out(float t)
{
    if (t < 0.5f) {
        return 2.0f * t * t;
    } else {
        return 1.0f - powf(-2.0f * t + 2.0f, 2.0f) / 2.0f;
    }
}

/* ==================== 模块初始化 ==================== */

static bool g_effects_initialized = false;

int xy_effect_init(void)
{
    if (g_effects_initialized) return 0;

    g_effects_initialized = true;
    return 0;
}

int xy_effect_deinit(void)
{
    g_effects_initialized = false;
    return 0;
}
