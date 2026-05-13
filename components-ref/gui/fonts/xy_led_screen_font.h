/**
 * @file xy_led_screen_font.h
 * @brief LED Screen Font Library - 5x7/8x8 Fonts
 * @version 1.0.0
 * @date 2026-03-02
 */

#ifndef XY_LED_SCREEN_FONT_H
#define XY_LED_SCREEN_FONT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 字体结构
 */
typedef struct {
    const uint8_t *data;      // 字体数据
    uint8_t width;            // 字符宽度
    uint8_t height;           // 字符高度
    uint8_t first_char;       // 第一个字符
    uint8_t char_count;       // 字符数量
} xy_led_font_t;

/**
 * @brief 5x7 字体数据 (ASCII 32-126)
 */
extern const uint8_t g_font_5x7_data[];
extern const xy_led_font_t g_font_5x7;

/**
 * @brief 8x8 字体数据 (ASCII 32-126)
 */
extern const uint8_t g_font_8x8_data[];
extern const xy_led_font_t g_font_8x8;

/**
 * @brief 16x16 中文字体 (常用汉字)
 */
extern const uint8_t g_font_16x16_cn_data[];
extern const xy_led_font_t g_font_16x16_cn;

/**
 * @brief 获取字符宽度
 */
uint8_t xy_led_font_get_char_width(const xy_led_font_t *font, char c);

/**
 * @brief 获取字符高度
 */
uint8_t xy_led_font_get_height(const xy_led_font_t *font);

/**
 * @brief 获取字符数据
 */
const uint8_t* xy_led_font_get_char_data(const xy_led_font_t *font, char c);

/**
 * @brief 测量字符串宽度
 */
uint16_t xy_led_font_measure_string(const xy_led_font_t *font, const char *str);

#ifdef __cplusplus
}
#endif

#endif /* XY_LED_SCREEN_FONT_H */
