/**
 * @file xy_rgb_color.h
 * @brief RGB Color Utilities
 * @version 1.0.0
 * @date 2026-03-02
 */

#ifndef XY_RGB_COLOR_H
#define XY_RGB_COLOR_H

#include "xy_rgb.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief RGB 转 HSV
 */
hsv_color_t xy_rgb_to_hsv(rgb_color_t rgb);

/**
 * @brief HSV 转 RGB
 */
rgb_color_t xy_hsv_to_rgb(hsv_color_t hsv);

/**
 * @brief 颜色混合
 */
rgb_color_t xy_color_blend(rgb_color_t color1, rgb_color_t color2, uint8_t factor);

/**
 * @brief 颜色插值
 */
rgb_color_t xy_color_lerp(rgb_color_t color1, rgb_color_t color2, float t);

/**
 * @brief 生成彩虹颜色
 */
rgb_color_t xy_rainbow_color(uint8_t hue);

/**
 * @brief 生成渐变颜色
 */
rgb_color_t xy_gradient_color(rgb_color_t start, rgb_color_t end, uint8_t pos);

/**
 * @brief 颜色 gamma 校正
 */
rgb_color_t xy_color_gamma(rgb_color_t color, float gamma);

/**
 * @brief 限制颜色范围
 */
rgb_color_t xy_color_clamp(rgb_color_t color);

/**
 * @brief 颜色取反
 */
rgb_color_t xy_color_invert(rgb_color_t color);

/**
 * @brief 颜色变亮
 */
rgb_color_t xy_color_brighten(rgb_color_t color, uint8_t amount);

/**
 * @brief 颜色变暗
 */
rgb_color_t xy_color_darken(rgb_color_t color, uint8_t amount);

/**
 * @brief 生成随机颜色
 */
rgb_color_t xy_color_random(void);

/**
 * @brief 颜色相等比较
 */
bool xy_color_equal(rgb_color_t color1, rgb_color_t color2);

/**
 * @brief 计算颜色距离
 */
uint16_t xy_color_distance(rgb_color_t color1, rgb_color_t color2);

#ifdef __cplusplus
}
#endif

#endif /* XY_RGB_COLOR_H */
