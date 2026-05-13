/**
 * @file xy_gui_effect_rotate.h
 * @brief GUI Rotate Effect Header - 旋转效果头文件
 * @version 1.0.0
 * @date 2026-03-13
 */

#ifndef XY_GUI_EFFECT_ROTATE_H
#define XY_GUI_EFFECT_ROTATE_H

#include "xy_gui_effects.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 创建旋转效果
 * @param rotate 效果结构指针
 * @param start_angle 起始角度(度)
 * @param end_angle 结束角度(度)
 * @param duration 持续时间(ms)
 * @return 0=成功, -1=失败
 */
int xy_effect_rotate_create(xy_effect_rotate_t *rotate, float start_angle,
                             float end_angle, uint32_t duration);

/**
 * @brief 更新旋转效果
 * @param rotate 效果结构指针
 * @param dt 距上次更新的时间(ms)
 */
void xy_effect_rotate_update(xy_effect_rotate_t *rotate, uint32_t dt);

/**
 * @brief 获取当前角度
 * @param rotate 效果结构指针
 * @return 当前角度(度) [0, 360)
 */
float xy_effect_rotate_get_angle(xy_effect_rotate_t *rotate);

#ifdef __cplusplus
}
#endif

#endif /* XY_GUI_EFFECT_ROTATE_H */
