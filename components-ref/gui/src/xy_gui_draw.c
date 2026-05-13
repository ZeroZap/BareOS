/**
 * @file xy_gui_draw.c
 * @brief GUI Drawing Helper Functions Implementation - 绘图辅助函数实现
 * @version 1.0.0
 * @date 2026-03-14
 */

#include "xy_gui_draw.h"
#include "xy_font.h"
#include <string.h>
#include <math.h>

/**
 * @brief 绘制像素 (内部使用)
 */
static inline void draw_pixel(void *fb, uint16_t fb_w, uint16_t fb_h, 
                              int16_t x, int16_t y, xy_gui_color_t color)
{
    if (!fb || x < 0 || x >= fb_w || y < 0 || y >= fb_h) {
        return;
    }
    
    /* 假设帧缓冲是 RGB565 格式 */
    uint16_t *buffer = (uint16_t *)fb;
    uint16_t color565 = ((color.r & 0xF8) << 8) | ((color.g & 0xFC) << 3) | ((color.b & 0xF8) >> 3);
    buffer[y * fb_w + x] = color565;
}

/**
 * @brief 绘制矩形
 */
void xy_gui_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, 
                      xy_gui_color_t color, bool fill,
                      void *fb, uint16_t fb_w, uint16_t fb_h)
{
    if (!fb || w <= 0 || h <= 0) {
        return;
    }
    
    if (fill) {
        /* 填充矩形 */
        for (int16_t dy = 0; dy < h; dy++) {
            for (int16_t dx = 0; dx < w; dx++) {
                draw_pixel(fb, fb_w, fb_h, x + dx, y + dy, color);
            }
        }
    } else {
        /* 边框矩形 */
        /* 上边框 */
        for (int16_t dx = 0; dx < w; dx++) {
            draw_pixel(fb, fb_w, fb_h, x + dx, y, color);
        }
        /* 下边框 */
        for (int16_t dx = 0; dx < w; dx++) {
            draw_pixel(fb, fb_w, fb_h, x + dx, y + h - 1, color);
        }
        /* 左边框 */
        for (int16_t dy = 0; dy < h; dy++) {
            draw_pixel(fb, fb_w, fb_h, x, y + dy, color);
        }
        /* 右边框 */
        for (int16_t dy = 0; dy < h; dy++) {
            draw_pixel(fb, fb_w, fb_h, x + w - 1, y + dy, color);
        }
    }
}

/**
 * @brief 绘制字符串
 */
void xy_gui_draw_string(int16_t x, int16_t y, const char *text,
                        xy_gui_color_t color, const xy_font_t *font,
                        void *fb, uint16_t fb_w, uint16_t fb_h)
{
    if (!text || !font || !fb) {
        return;
    }
    
    /* 使用 xy_font 库绘制 */
    uint16_t color565 = ((color.r & 0xF8) << 8) | ((color.g & 0xFC) << 3) | ((color.b & 0xF8) >> 3);
    xy_font_draw_string(font, text, x, y, color565, fb, fb_w, fb_h);
}

/**
 * @brief 绘制圆角矩形 (简化版 - 实际应使用更复杂的算法)
 */
void xy_gui_draw_rounded_rect(int16_t x, int16_t y, int16_t w, int16_t h,
                               uint8_t radius, xy_gui_color_t color, bool fill,
                               void *fb, uint16_t fb_w, uint16_t fb_h)
{
    /* 简化实现：当 radius=0 时退化为普通矩形 */
    if (radius == 0) {
        xy_gui_draw_rect(x, y, w, h, color, fill, fb, fb_w, fb_h);
        return;
    }
    
    // Rounded rectangle placeholder
    /* 目前先绘制普通矩形 */
    xy_gui_draw_rect(x, y, w, h, color, fill, fb, fb_w, fb_h);
}

/**
 * @brief 绘制水平线
 */
void xy_gui_draw_hline(int16_t x1, int16_t y1, int16_t x2, 
                       xy_gui_color_t color, void *fb, uint16_t fb_w, uint16_t fb_h)
{
    if (!fb || x1 > x2) {
        return;
    }
    
    for (int16_t x = x1; x <= x2; x++) {
        draw_pixel(fb, fb_w, fb_h, x, y1, color);
    }
}

/**
 * @brief 绘制垂直线
 */
void xy_gui_draw_vline(int16_t x, int16_t y1, int16_t y2,
                       xy_gui_color_t color, void *fb, uint16_t fb_w, uint16_t fb_h)
{
    if (!fb || y1 > y2) {
        return;
    }
    
    for (int16_t y = y1; y <= y2; y++) {
        draw_pixel(fb, fb_w, fb_h, x, y, color);
    }
}

/**
 * @brief 绘制直线 (Bresenham 算法)
 */
void xy_gui_draw_line(int16_t x1, int16_t y1, int16_t x2, int16_t y2,
                      xy_gui_color_t color, void *fb, uint16_t fb_w, uint16_t fb_h)
{
    int16_t dx = x2 - x1;
    int16_t dy = y2 - y1;
    int16_t sx = (dx > 0) ? 1 : -1;
    int16_t sy = (dy > 0) ? 1 : -1;
    
    dx = (dx > 0) ? dx : -dx;
    dy = (dy > 0) ? dy : -dy;
    
    if (dx > dy) {
        int16_t err = dx / 2;
        while (x1 != x2) {
            draw_pixel(fb, fb_w, fb_h, x1, y1, color);
            err -= dy;
            if (err < 0) {
                y1 += sy;
                err += dx;
            }
            x1 += sx;
        }
    } else {
        int16_t err = dy / 2;
        while (y1 != y2) {
            draw_pixel(fb, fb_w, fb_h, x1, y1, color);
            err -= dx;
            if (err < 0) {
                x1 += sx;
                err += dy;
            }
            y1 += sy;
        }
    }
    draw_pixel(fb, fb_w, fb_h, x1, y1, color);
}

/**
 * @brief 绘制圆形 (中点圆算法)
 */
void xy_gui_draw_circle(int16_t cx, int16_t cy, int16_t radius,
                        xy_gui_color_t color, bool fill,
                        void *fb, uint16_t fb_w, uint16_t fb_h)
{
    if (!fb || radius <= 0) {
        return;
    }
    
    if (fill) {
        /* 填充圆形：绘制水平线 */
        for (int16_t y = -radius; y <= radius; y++) {
            int16_t x_extent = (int16_t)(sqrt(radius * radius - y * y) + 0.5);
            xy_gui_draw_hline(cx - x_extent, cy + y, cx + x_extent, color, fb, fb_w, fb_h);
        }
    } else {
        /* 空心圆形 */
        int16_t x = radius;
        int16_t y = 0;
        int16_t err = 0;
        
        while (x >= y) {
            draw_pixel(fb, fb_w, fb_h, cx + x, cy + y, color);
            draw_pixel(fb, fb_w, fb_h, cx + y, cy + x, color);
            draw_pixel(fb, fb_w, fb_h, cx - y, cy + x, color);
            draw_pixel(fb, fb_w, fb_h, cx - x, cy + y, color);
            draw_pixel(fb, fb_w, fb_h, cx - x, cy - y, color);
            draw_pixel(fb, fb_w, fb_h, cx - y, cy - x, color);
            draw_pixel(fb, fb_w, fb_h, cx + y, cy - x, color);
            draw_pixel(fb, fb_w, fb_h, cx + x, cy - y, color);
            
            y++;
            err += 1 + 2 * y;
            if (1 + 2 * x - 2 * err > 0) {
                x--;
                err -= 2 * x;
            }
        }
    }
}

/**
 * @brief 绘制居中字符串
 */
void xy_gui_draw_string_center(int16_t cx, int16_t y, const char *text,
                                xy_gui_color_t color, const xy_font_t *font,
                                void *fb, uint16_t fb_w, uint16_t fb_h)
{
    if (!text || !font || !fb) {
        return;
    }
    
    uint16_t text_width = xy_font_get_text_width(font, text);
    int16_t x = cx - text_width / 2;
    
    xy_gui_draw_string(x, y, text, color, font, fb, fb_w, fb_h);
}
