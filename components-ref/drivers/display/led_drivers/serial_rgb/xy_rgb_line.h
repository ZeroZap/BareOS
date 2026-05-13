/**
 * @file xy_rgb_line.h
 * @brief RGB LED Line Mode (1D Strip)
 * @version 1.0.0
 * @date 2026-03-02
 */

#ifndef XY_RGB_LINE_H
#define XY_RGB_LINE_H

#include "xy_rgb.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化 Line 模式
 * @param num_leds LED 数量
 * @return XY_RGB_OK 成功，其他值失败
 */
int32_t xy_rgb_line_init(uint16_t num_leds);

/**
 * @brief 设置像素颜色
 * @param x X 坐标 (0 ~ num_leds-1)
 * @param color 颜色
 */
void xy_rgb_line_set_pixel(uint16_t x, rgb_color_t color);

/**
 * @brief 获取像素颜色
 * @param x X 坐标
 * @return 颜色
 */
rgb_color_t xy_rgb_line_get_pixel(uint16_t x);

/**
 * @brief 填充区域
 * @param start 起始位置
 * @param len 长度
 * @param color 颜色
 */
void xy_rgb_line_fill(uint16_t start, uint16_t len, rgb_color_t color);

/**
 * @brief 清除所有 LED
 */
void xy_rgb_line_clear(void);

/* ==================== Line 效果 ==================== */

/**
 * @brief 扫描灯效果
 * @param color 颜色
 * @param speed 速度
 */
void xy_rgb_line_fx_scan(rgb_color_t color, uint16_t speed);

/**
 * @brief 追逐灯效果
 * @param color 颜色
 * @param count 追逐 LED 数量
 * @param speed 速度
 */
void xy_rgb_line_fx_chase(rgb_color_t color, uint8_t count, uint16_t speed);

/**
 * @brief 彗星效果
 * @param head_color 头部颜色
 * @param tail_color 尾部颜色
 * @param speed 速度
 */
void xy_rgb_line_fx_comet(rgb_color_t head_color, rgb_color_t tail_color, 
                          uint16_t speed);

/**
 * @brief 流星效果
 * @param color 颜色
 * @param speed 速度
 */
void xy_rgb_line_fx_meteor(rgb_color_t color, uint16_t speed);

/**
 * @brief 雨滴效果
 * @param color 颜色
 * @param speed 速度
 */
void xy_rgb_line_fx_rain(rgb_color_t color, uint16_t speed);

/**
 * @brief 渐变效果
 * @param color1 颜色 1
 * @param color2 颜色 2
 */
void xy_rgb_line_fx_gradient(rgb_color_t color1, rgb_color_t color2);

/**
 * @brief 彩虹效果
 * @param speed 速度
 */
void xy_rgb_line_fx_rainbow(uint16_t speed);

#ifdef __cplusplus
}
#endif

#endif /* XY_RGB_LINE_H */
