/**
 * @file xy_gui_effect_blink.c
 * @brief GUI Blink Effect Implementation - 闪烁/频闪效果实现
 * @version 1.0.0
 * @date 2026-03-13
 */

#include "xy_gui_effect_fade.h"  /* 复用基础结构 */
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* ==================== 闪烁效果实现 ==================== */

int xy_effect_blink_create(xy_effect_blink_t *blink, uint16_t on_time, uint16_t off_time,
                           uint8_t min_bright, uint8_t max_bright)
{
    if (!blink) return -1;

    memset(blink, 0, sizeof(*blink));

    blink->base.type = XY_EFFECT_TYPE_BLINK;
    blink->base.state = XY_EFFECT_STATE_STOPPED;
    blink->base.duration = 0;  /* 闪烁无固定持续时间 */
    blink->base.elapsed = 0;
    blink->base.repeat = 0;    /* 默认无限重复 */
    blink->base.current_repeat = 0;
    blink->base.progress = 0.0f;

    blink->on_time = on_time;
    blink->off_time = off_time;
    blink->min_brightness = min_bright;
    blink->max_brightness = max_bright;

    return 0;
}

void xy_effect_blink_update(xy_effect_blink_t *blink, uint32_t dt)
{
    if (!blink) return;

    if (blink->base.state != XY_EFFECT_STATE_RUNNING) return;

    blink->base.elapsed += dt;
}

bool xy_effect_blink_is_on(xy_effect_blink_t *blink)
{
    if (!blink) return false;

    if (blink->base.state != XY_EFFECT_STATE_RUNNING) return false;

    uint32_t cycle_time = blink->on_time + blink->off_time;
    if (cycle_time == 0) return false;

    uint32_t elapsed = blink->base.elapsed % cycle_time;
    return elapsed < blink->on_time;
}

uint8_t xy_effect_blink_get_brightness(xy_effect_blink_t *blink)
{
    if (!blink) return 0;

    if (xy_effect_blink_is_on(blink)) {
        return blink->max_brightness;
    } else {
        return blink->min_brightness;
    }
}
