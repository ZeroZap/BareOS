/**
 * @file xy_gui_effect_breath.c
 * @brief GUI Breath Effect Implementation - 呼吸灯效果实现
 * @version 1.0.0
 * @date 2026-03-13
 */

#include "xy_gui_effects.h"
#include <string.h>
#include <math.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* ==================== 呼吸灯效果实现 ==================== */

int xy_effect_breath_create(xy_effect_breath_t *breath, uint16_t period,
                            uint8_t min_bright, uint8_t max_bright)
{
    if (!breath) return -1;

    memset(breath, 0, sizeof(*breath));

    breath->base.type = XY_EFFECT_TYPE_BREATH;
    breath->base.state = XY_EFFECT_STATE_STOPPED;
    breath->base.duration = 0;  /* 呼吸灯是连续循环的 */
    breath->base.elapsed = 0;
    breath->base.repeat = 0;    /* 默认无限重复 */
    breath->base.current_repeat = 0;
    breath->base.progress = 0.0f;

    breath->period = period;
    breath->min_brightness = min_bright;
    breath->max_brightness = max_bright;
    breath->smooth = true;

    return 0;
}

void xy_effect_breath_update(xy_effect_breath_t *breath, uint32_t dt)
{
    if (!breath) return;

    if (breath->base.state != XY_EFFECT_STATE_RUNNING) return;

    breath->base.elapsed += dt;

    /* 呼吸灯周期循环 */
    if (breath->period > 0) {
        breath->base.progress = (float)(breath->base.elapsed % breath->period) / breath->period;
    } else {
        breath->base.progress = 0.0f;
    }
}

uint8_t xy_effect_breath_get_brightness(xy_effect_breath_t *breath)
{
    if (!breath) return 0;

    /* 使用正弦波实现平滑呼吸效果 */
    float sine_value = xy_effect_breath_get_sine_value(breath);

    /* 将 sine 值从 [-1, 1] 映射到 [min, max] */
    float t = (sine_value + 1.0f) / 2.0f;  /* 转换为 [0, 1] */
    return (uint8_t)(breath->min_brightness + t * (breath->max_brightness - breath->min_brightness));
}

float xy_effect_breath_get_sine_value(xy_effect_breath_t *breath)
{
    if (!breath) return 0.0f;

    /* 正弦波: 0 -> 上升 -> 1 -> 下降 -> 0 */
    float phase = breath->base.progress * 2.0f * 3.14159265f;

    if (breath->smooth) {
        /* 平滑正弦波 */
        return sinf(phase);
    } else {
        /* 线性锯齿波 */
        if (breath->base.progress < 0.5f) {
            return 2.0f * breath->base.progress;
        } else {
            return 2.0f * (1.0f - breath->base.progress);
        }
    }
}
