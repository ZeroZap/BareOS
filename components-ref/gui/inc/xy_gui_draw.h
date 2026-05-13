/**
 * @file xy_gui_draw.h
 * @brief GUI Drawing Helper Functions - 绘图辅助函数
 * @version 1.0.0
 * @date 2026-03-14
 * 
 * 提供不依赖 xy_gui_t 句柄的绘图函数，供控件内部使用
 */

#ifndef XY_GUI_DRAW_H
#define XY_GUI_DRAW_H

#include <stdint.h>
#include <stdbool.h>
#include "xy_gui_widget.h"
#include "xy_font.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 绘制矩形
 * @param x X 坐标
 * @param y Y 坐标
 * @param w 宽度
 * @param h 高度
 * @param color 颜色
 * @param fill true=填充，false=边框
 * @param fb 帧缓冲
 * @param fb_w 帧缓冲宽度
 * @param fb_h 帧缓冲高度
 */
void xy_gui_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, 
                      xy_gui_color_t color, bool fill,
                      void *fb, uint16_t fb_w, uint16_t fb_h);

/**
 * @brief 绘制字符串
 * @param x X 坐标
 * @param y Y 坐标
 * @param text 文本
 * @param color 颜色
 * @param font 字体
 * @param fb 帧缓冲
 * @param fb_w 帧缓冲宽度
 * @param fb_h 帧缓冲高度
 */
void xy_gui_draw_string(int16_t x, int16_t y, const char *text,
                        xy_gui_color_t color, const xy_font_t *font,
                        void *fb, uint16_t fb_w, uint16_t fb_h);

/**
 * @brief 绘制圆角矩形
 * @param x X 坐标
 * @param y Y 坐标
 * @param w 宽度
 * @param h 高度
 * @param radius 圆角半径
 * @param color 颜色
 * @param fill true=填充，false=边框
 * @param fb 帧缓冲
 * @param fb_w 帧缓冲宽度
 * @param fb_h 帧缓冲高度
 */
void xy_gui_draw_rounded_rect(int16_t x, int16_t y, int16_t w, int16_t h,
                               uint8_t radius, xy_gui_color_t color, bool fill,
                               void *fb, uint16_t fb_w, uint16_t fb_h);

/**
 * @brief 绘制水平线
 * @param x1 起点 X
 * @param y1 起点 Y
 * @param x2 终点 X
 * @param color 颜色
 * @param fb 帧缓冲
 * @param fb_w 帧缓冲宽度
 * @param fb_h 帧缓冲高度
 */
void xy_gui_draw_hline(int16_t x1, int16_t y1, int16_t x2, 
                       xy_gui_color_t color, void *fb, uint16_t fb_w, uint16_t fb_h);

/**
 * @brief 绘制垂直线
 * @param x X 坐标
 * @param y1 起点 Y
 * @param y2 终点 Y
 * @param color 颜色
 * @param fb 帧缓冲
 * @param fb_w 帧缓冲宽度
 * @param fb_h 帧缓冲高度
 */
void xy_gui_draw_vline(int16_t x, int16_t y1, int16_t y2,
                       xy_gui_color_t color, void *fb, uint16_t fb_w, uint16_t fb_h);

/**
 * @brief 绘制直线
 */
void xy_gui_draw_line(int16_t x1, int16_t y1, int16_t x2, int16_t y2,
                      xy_gui_color_t color, void *fb, uint16_t fb_w, uint16_t fb_h);

/**
 * @brief 绘制圆形
 */
void xy_gui_draw_circle(int16_t cx, int16_t cy, int16_t radius,
                        xy_gui_color_t color, bool fill,
                        void *fb, uint16_t fb_w, uint16_t fb_h);

/**
 * @brief 绘制居中字符串
 */
void xy_gui_draw_string_center(int16_t cx, int16_t y, const char *text,
                                xy_gui_color_t color, const xy_font_t *font,
                                void *fb, uint16_t fb_w, uint16_t fb_h);

#ifdef __cplusplus
}
#endif

#endif /* XY_GUI_DRAW_H */
