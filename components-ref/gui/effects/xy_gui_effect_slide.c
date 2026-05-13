/**
 * @file xy_gui_effect_slide.c
 * @brief GUI Slide Effect Implementation - 滑动转场效果实现
 * @version 1.0.0
 * @date 2026-03-13
 */

#include "xy_gui_effects.h"
#include <string.h>
#include <math.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* ==================== 滑动效果实现 ==================== */

int xy_effect_slide_create(xy_effect_slide_t *slide, xy_effect_dir_t direction,
                           int16_t offset, uint32_t duration)
{
    if (!slide) return -1;

    memset(slide, 0, sizeof(*slide));

    slide->base.type = XY_EFFECT_TYPE_SLIDE;
    slide->base.state = XY_EFFECT_STATE_STOPPED;
    slide->base.duration = duration;
    slide->base.elapsed = 0;
    slide->base.repeat = 1;
    slide->base.current_repeat = 0;
    slide->base.progress = 0.0f;

    slide->direction = direction;
    slide->start_offset = 0;
    slide->end_offset = offset;

    return 0;
}

void xy_effect_slide_update(xy_effect_slide_t *slide, uint32_t dt)
{
    if (!slide) return;

    if (slide->base.state != XY_EFFECT_STATE_RUNNING) return;

    slide->base.elapsed += dt;

    /* 计算进度 */
    if (slide->base.duration > 0) {
        slide->base.progress = (float)slide->base.elapsed / slide->base.duration;
    } else {
        slide->base.progress = 1.0f;
    }

    /* 限制进度范围 */
    if (slide->base.progress > 1.0f) {
        slide->base.progress = 1.0f;
    }

    /* 检查是否完成 */
    if (slide->base.elapsed >= slide->base.duration) {
        slide->base.progress = 1.0f;

        /* 处理重复 */
        if (slide->base.repeat > 0) {
            slide->base.current_repeat++;
            if (slide->base.current_repeat >= slide->base.repeat) {
                slide->base.state = XY_EFFECT_STATE_STOPPED;
            } else {
                slide->base.elapsed = 0;
                slide->base.progress = 0.0f;
            }
        } else {
            slide->base.state = XY_EFFECT_STATE_STOPPED;
        }
    }
}

int16_t xy_effect_slide_get_offset(xy_effect_slide_t *slide)
{
    if (!slide) return 0;

    /* 使用缓出效果 */
    float t = 1.0f - powf(1.0f - slide->base.progress, 2.0f);

    /* 根据方向计算偏移量 */
    int16_t offset = 0;
    switch (slide->direction) {
        case XY_EFFECT_DIR_LEFT:
            offset = (int16_t)(slide->start_offset + (slide->end_offset - slide->start_offset) * t);
            break;
        case XY_EFFECT_DIR_RIGHT:
            offset = (int16_t)(slide->start_offset + (slide->end_offset - slide->start_offset) * t);
            break;
        case XY_EFFECT_DIR_UP:
            offset = (int16_t)(slide->start_offset + (slide->end_offset - slide->start_offset) * t);
            break;
        case XY_EFFECT_DIR_DOWN:
            offset = (int16_t)(slide->start_offset + (slide->end_offset - slide->start_offset) * t);
            break;
        default:
            break;
    }

    return offset;
}
