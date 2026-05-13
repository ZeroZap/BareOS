/**
 * @file xy_gui_effect_blink.h
 * @brief GUI Blink Effect Header - 闪烁/频闪效果头文件
 * @version 1.0.0
 * @date 2026-03-13
 */

#ifndef XY_GUI_EFFECT_BLINK_H
#define XY_GUI_EFFECT_BLINK_H

#include "xy_gui_effects.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 创建闪烁效果
 * @param blink 效果结构指针
 * @param on_time 亮起时间(ms)
 * @param off_time 熄灭时间(ms)
 * @param min_bright 最小亮度 [0, 255]
 * @param max_bright 最大亮度 [0, 255]
 * @return 0=成功, -1=失败
 */
int xy_effect_blink_create(xy_effect_blink_t *blink, uint16_t on_time, uint16_t off_time,
                           uint8_t min_bright, uint8_t max_bright);

/**
 * @brief 更新闪烁效果
 * @param blink 效果结构指针
 * @param dt 距上次更新的时间(ms)
 */
void xy_effect_blink_update(xy_effect_blink_t *blink, uint32_t dt);

/**
 * @brief 检查当前是否为亮起状态
 * @param blink 效果结构指针
 * @return true=亮起, false=熄灭
 */
bool xy_effect_blink_is_on(xy_effect_blink_t *blink);

/**
 * @brief 获取当前亮度
 * @param blink 效果结构指针
 * @return 当前亮度 [0, 255]
 */
uint8_t xy_effect_blink_get_brightness(xy_effect_blink_t *blink);

#ifdef __cplusplus
}
#endif

#endif /* XY_GUI_EFFECT_BLINK_H */
