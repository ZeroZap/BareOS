/**
 * @file xy_gui_effect_breath.h
 * @brief GUI Breath Effect Header - 呼吸灯效果头文件
 * @version 1.0.0
 * @date 2026-03-13
 */

#ifndef XY_GUI_EFFECT_BREATH_H
#define XY_GUI_EFFECT_BREATH_H

#include "xy_gui_effects.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 创建呼吸灯效果
 * @param breath 效果结构指针
 * @param period 呼吸周期时间(ms)
 * @param min_bright 最小亮度 [0, 255]
 * @param max_bright 最大亮度 [0, 255]
 * @return 0=成功, -1=失败
 */
int xy_effect_breath_create(xy_effect_breath_t *breath, uint16_t period,
                            uint8_t min_bright, uint8_t max_bright);

/**
 * @brief 更新呼吸灯效果
 * @param breath 效果结构指针
 * @param dt 距上次更新的时间(ms)
 */
void xy_effect_breath_update(xy_effect_breath_t *breath, uint32_t dt);

/**
 * @brief 获取当前亮度
 * @param breath 效果结构指针
 * @return 当前亮度 [0, 255]
 */
uint8_t xy_effect_breath_get_brightness(xy_effect_breath_t *breath);

/**
 * @brief 获取当前正弦值
 * @param breath 效果结构指针
 * @return 正弦值 [-1.0, 1.0]
 */
float xy_effect_breath_get_sine_value(xy_effect_breath_t *breath);

#ifdef __cplusplus
}
#endif

#endif /* XY_GUI_EFFECT_BREATH_H */
