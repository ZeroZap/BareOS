/**
 * @file xy_font_chinese_16x16.h
 * @brief 16x16 Chinese Font (常用汉字)
 * @version 1.0.0
 * @date 2026-03-02
 */

#ifndef XY_FONT_CHINESE_16X16_H
#define XY_FONT_CHINESE_16X16_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief 常用汉字数量
 */
#define FONT_CHINESE_CHAR_COUNT 150

/**
 * @brief 汉字字体宽度
 */
#define FONT_CHINESE_WIDTH 16

/**
 * @brief 汉字字体高度
 */
#define FONT_CHINESE_HEIGHT 16

/**
 * @brief 汉字信息结构
 */
typedef struct {
    uint16_t unicode;          /* Unicode 编码 */
    const uint8_t *data;       /* 32字节点阵数据 */
} xy_chinese_char_t;

/**
 * @brief 汉字字体信息
 */
typedef struct {
    const uint8_t *data;
    uint8_t width;
    uint8_t height;
    uint16_t char_count;
} xy_font_chinese_t;

/**
 * @brief 获取汉字字体句柄
 */
const xy_font_chinese_t* xy_font_chinese_16x16_get(void);

/**
 * @brief 获取汉字点阵数据
 * @param unicode Unicode编码
 * @return 32字节点阵数据，NULL表示字符不存在
 */
const uint8_t* xy_font_chinese_16x16_get_char(uint16_t unicode);

/**
 * @brief 获取汉字点阵数据(GB2312编码)
 * @param gb2312 GB2312编码高字节先
 * @return 32字节点阵数据，NULL表示字符不存在
 */
const uint8_t* xy_font_chinese_16x16_get_gb2312(uint16_t gb2312);

/**
 * @brief 测量中文字符串宽度(粗略)
 * @param str 字符串(包含中文和ASCII)
 * @return 字符串宽度(像素)
 */
uint16_t xy_font_chinese_16x16_measure(const char *str);

/**
 * @brief 获取所有支持的中文字符列表
 * @return 中文字符数组
 */
const xy_chinese_char_t* xy_font_chinese_16x16_get_chars(void);

#ifdef __cplusplus
}
#endif

#endif /* XY_FONT_CHINESE_16X16_H */
