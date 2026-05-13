/**
 * @file xy_gui_effect_rotate.c
 * @brief GUI Rotate Effect Implementation - 旋转效果实现
 * @version 1.0.0
 * @date 2026-03-13
 */

#include "xy_gui_effects.h"
#include <string.h>
#include <math.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* ==================== 旋转效果实现 ==================== */

int xy_effect_rotate_create(xy_effect_rotate_t *rotate, float start_angle,
                           float end_angle, uint32_t duration)
{
    if (!rotate) return -1;

    memset(rotate, 0, sizeof(*rotate));

    rotate->base.type = XY_EFFECT_TYPE_ROTATE;
    rotate->base.state = XY_EFFECT_STATE_STOPPED;
    rotate->base.duration = duration;
    rotate->base.elapsed = 0;
    rotate->base.repeat = 1;
    rotate->base.current_repeat = 0;
    rotate->base.progress = 0.0f;

    rotate->start_angle = start_angle;
    rotate->end_angle = end_angle;
    rotate->center_x = 0.5f;
    rotate->center_y = 0.5f;
    rotate->clockwise = true;

    return 0;
}

void xy_effect_rotate_update(xy_effect_rotate_t *rotate, uint32_t dt)
{
    if (!rotate) return;

    if (rotate->base.state != XY_EFFECT_STATE_RUNNING) return;

    rotate->base.elapsed += dt;

    /* 计算进度 */
    if (rotate->base.duration > 0) {
        rotate->base.progress = (float)rotate->base.elapsed / rotate->base.duration;
    } else {
        rotate->base.progress = 1.0f;
    }

    /* 限制进度范围 */
    if (rotate->base.progress > 1.0f) {
        rotate->base.progress = 1.0f;
    }

    /* 检查是否完成 */
    if (rotate->base.elapsed >= rotate->base.duration) {
        rotate->base.progress = 1.0f;

        /* 处理重复 */
        if (rotate->base.repeat > 0) {
            rotate->base.current_repeat++;
            if (rotate->base.current_repeat >= rotate->base.repeat) {
                rotate->base.state = XY_EFFECT_STATE_STOPPED;
            } else {
                rotate->base.elapsed = 0;
                rotate->base.progress = 0.0f;
            }
        } else {
            rotate->base.state = XY_EFFECT_STATE_STOPPED;
        }
    }
}

float xy_effect_rotate_get_angle(xy_effect_rotate_t *rotate)
{
    if (!rotate) return 0.0f;

    /* 使用缓入缓出效果 */
    float t;
    if (rotate->clockwise) {
        /* 顺时针: 使用缓出效果 */
        t = 1.0f - powf(1.0f - rotate->base.progress, 2.0f);
    } else {
        /* 逆时针: 使用缓入效果 */
        t = powf(rotate->base.progress, 2.0f);
    }

    /* 插值计算当前角度 */
    float delta = rotate->end_angle - rotate->start_angle;
    float current = rotate->start_angle + delta * t;

    /* 归一化到 [0, 360) 范围 */
    while (current < 0.0f) current += 360.0f;
    while (current >= 360.0f) current -= 360.0f;

    return current;
}
