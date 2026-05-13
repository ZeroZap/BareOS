/**
 * @file xy_gui_effect_slide.h
 * @brief GUI Slide Effect Header - 滑动转场效果头文件
 * @version 1.0.0
 * @date 2026-03-13
 */

#ifndef XY_GUI_EFFECT_SLIDE_H
#define XY_GUI_EFFECT_SLIDE_H

#include "xy_gui_effects.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 创建滑动效果
 * @param slide 效果结构指针
 * @param direction 滑动方向
 * @param offset 滑动距离(像素)
 * @param duration 持续时间(ms)
 * @return 0=成功, -1=失败
 */
int xy_effect_slide_create(xy_effect_slide_t *slide, xy_effect_dir_t direction,
                           int16_t offset, uint32_t duration);

/**
 * @brief 更新滑动效果
 * @param slide 效果结构指针
 * @param dt 距上次更新的时间(ms)
 */
void xy_effect_slide_update(xy_effect_slide_t *slide, uint32_t dt);

/**
 * @brief 获取当前偏移量
 * @param slide 效果结构指针
 * @return 当前偏移量(像素)
 */
int16_t xy_effect_slide_get_offset(xy_effect_slide_t *slide);

#ifdef __cplusplus
}
#endif

#endif /* XY_GUI_EFFECT_SLIDE_H */
