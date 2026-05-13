/**
 * @file xy_oled_ssd1306.h
 * @brief SSD1306 OLED Display Device Driver
 * @version 1.0.0
 * @date 2026-02-28
 */

#ifndef XY_OLED_SSD1306_H
#define XY_OLED_SSD1306_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_device.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief SSD1306 设备结构
 */
typedef struct {
    xy_i2c_device_t i2c_dev;
    uint16_t width;
    uint16_t height;
    uint8_t *buffer;
    bool inverted;
} xy_oled_ssd1306_t;

/**
 * @brief 初始化 OLED
 * @param oled OLED 设备
 * @param i2c_handle I2C 句柄
 * @param width 宽度
 * @param height 高度
 * @return XY_DEVICE_OK 成功
 */
int xy_oled_ssd1306_init(xy_oled_ssd1306_t *oled, void *i2c_handle, 
                         uint16_t width, uint16_t height);

/**
 * @brief 清空屏幕
 * @param oled OLED 设备
 */
void xy_oled_ssd1306_clear(xy_oled_ssd1306_t *oled);

/**
 * @brief 刷新屏幕
 * @param oled OLED 设备
 */
void xy_oled_ssd1306_refresh(xy_oled_ssd1306_t *oled);

/**
 * @brief 绘制像素
 * @param oled OLED 设备
 * @param x X 坐标
 * @param y Y 坐标
 * @param color 颜色 (0=黑，1=白)
 */
void xy_oled_ssd1306_draw_pixel(xy_oled_ssd1306_t *oled, 
                                int16_t x, int16_t y, bool color);

/**
 * @brief 绘制直线
 * @param oled OLED 设备
 * @param x0 起点 X
 * @param y0 起点 Y
 * @param x1 终点 X
 * @param y1 终点 Y
 * @param color 颜色
 */
void xy_oled_ssd1306_draw_line(xy_oled_ssd1306_t *oled, 
                               int16_t x0, int16_t y0, 
                               int16_t x1, int16_t y1, bool color);

/**
 * @brief 绘制字符
 * @param oled OLED 设备
 * @param x X 坐标
 * @param y Y 坐标
 * @param c 字符
 * @param color 颜色
 */
void xy_oled_ssd1306_draw_char(xy_oled_ssd1306_t *oled, 
                               int16_t x, int16_t y, 
                               char c, bool color);

/**
 * @brief 绘制字符串
 * @param oled OLED 设备
 * @param x X 坐标
 * @param y Y 坐标
 * @param str 字符串
 * @param color 颜色
 */
void xy_oled_ssd1306_draw_string(xy_oled_ssd1306_t *oled, 
                                 int16_t x, int16_t y, 
                                 const char *str, bool color);

#ifdef __cplusplus
}
#endif

#endif /* XY_OLED_SSD1306_H */
