/**
 * @file xy_gui_effect_fade.h
 * @brief GUI Fade Effect Header - 淡入淡出效果头文件
 * @version 1.0.0
 * @date 2026-03-13
 */

#ifndef XY_GUI_EFFECT_FADE_H
#define XY_GUI_EFFECT_FADE_H

#include "xy_gui_effects.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 创建淡入淡出效果
 * @param fade 效果结构指针
 * @param fade_in true=淡入, false=淡出
 * @param duration 持续时间(ms)
 * @return 0=成功, -1=失败
 */
int xy_effect_fade_create(xy_effect_fade_t *fade, bool fade_in, uint32_t duration);

/**
 * @brief 更新淡入淡出效果
 * @param fade 效果结构指针
 * @param dt 距上次更新的时间(ms)
 */
void xy_effect_fade_update(xy_effect_fade_t *fade, uint32_t dt);

/**
 * @brief 获取当前透明度
 * @param fade 效果结构指针
 * @return 当前透明度 [0.0, 1.0]
 */
float xy_effect_fade_get_alpha(xy_effect_fade_t *fade);

#ifdef __cplusplus
}
#endif

#endif /* XY_GUI_EFFECT_FADE_H */
