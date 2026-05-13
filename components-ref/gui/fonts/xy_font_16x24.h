/**
 * @file xy_font_16x24.h
 * @brief 16x24 Bitmap Font (ASCII)
 * @version 1.0.0
 * @date 2026-03-02
 */

#ifndef XY_FONT_16X24_H
#define XY_FONT_16X24_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief 16x24 字体数据 (ASCII 0x20-0x7E)
 */
extern const uint8_t g_font_16x24_data[];

/**
 * @brief 16x24 字体字符数量
 */
#define FONT_16X24_CHAR_COUNT 94

/**
 * @brief 16x24 字体宽度
 */
#define FONT_16X24_WIDTH 16

/**
 * @brief 16x24 字体高度
 */
#define FONT_16X24_HEIGHT 24

/**
 * @brief 16x24 字体信息
 */
typedef struct {
    const uint8_t *data;
    uint8_t width;
    uint8_t height;
    uint8_t first_char;
    uint8_t char_count;
} xy_font_16x24_t;

/**
 * @brief 获取16x24字体句柄
 */
const xy_font_16x24_t* xy_font_16x24_get(void);

/**
 * @brief 获取字符位图数据
 * @param ch 字符
 * @return 48字节位图数据，NULL表示字符不存在
 */
const uint8_t* xy_font_16x24_get_char(uint8_t ch);

/**
 * @brief 测量ASCII字符串宽度
 * @param str 字符串
 * @return 字符串宽度(像素)
 */
uint16_t xy_font_16x24_measure(const char *str);

#ifdef __cplusplus
}
#endif

#endif /* XY_FONT_16X24_H */
