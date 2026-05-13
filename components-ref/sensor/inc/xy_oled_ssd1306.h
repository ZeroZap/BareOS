/**
 * @file xy_oled_ssd1306.h
 * @brief SSD1306 OLED Display Driver (128x64)
 * @version 1.0.0
 * @date 2026-03-01 凌晨
 */

#ifndef XY_OLED_SSD1306_H
#define XY_OLED_SSD1306_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_dev_i2c.h"
#include <stdint.h>

/**
 * @brief SSD1306 I2C 地址
 */
#define SSD1306_ADDR            0x3C

/**
 * @brief SSD1306 命令
 */
#define SSD1306_CMD_DISPLAY_OFF         0xAE
#define SSD1306_CMD_DISPLAY_ON          0xAF
#define SSD1306_CMD_SET_CONTRAST        0x81
#define SSD1306_CMD_NORMAL_DISPLAY      0xA6
#define SSD1306_CMD_INVERT_DISPLAY      0xA7
#define SSD1306_CMD_SET_MUX_RATIO       0xA8
#define SSD1306_CMD_SET_DISPLAY_OFFSET  0xD3
#define SSD1306_CMD_SET_START_LINE      0x40
#define SSD1306_CMD_SET_SEGMENT_REMAP   0xA1
#define SSD1306_CMD_SET_COM_SCAN_DEC    0xC8
#define SSD1306_CMD_SET_COM_PINS        0xDA
#define SSD1306_CMD_SET_PRECHARGE       0xD9
#define SSD1306_CMD_SET_VCOMH           0xDB
#define SSD1306_CMD_SET_CLOCK_DIV       0xD5
#define SSD1306_CMD_SET_CHARGE_PUMP     0x8D
#define SSD1306_CMD_MEMORY_MODE         0x20
#define SSD1306_CMD_COLUMN_ADDR         0x21
#define SSD1306_CMD_PAGE_ADDR           0x22
#define SSD1306_CMD_SET_PAGE_START      0xB0

/**
 * @brief 错误码
 */
#define XY_OLED_OK              0
#define XY_OLED_ERROR           (-1)
#define XY_OLED_INVALID_PARAM   (-2)
#define XY_OLED_NOT_FOUND       (-3)

/**
 * @brief OLED 颜色
 */
#define OLED_COLOR_BLACK        0
#define OLED_COLOR_WHITE        1

/**
 * @brief OLED 设备结构
 */
typedef struct {
    xy_i2c_device_t i2c_dev;    /**< I2C 设备 */
    uint16_t width;             /**< 宽度 */
    uint16_t height;            /**< 高度 */
    uint8_t *buffer;            /**< 显示缓冲区 */
    uint8_t buffer_size;        /**< 缓冲区大小 */
    uint8_t initialized;        /**< 初始化标志 */
} xy_oled_ssd1306_t;

/**
 * @brief 初始化 OLED
 * @param dev OLED 设备句柄
 * @param i2c_handle I2C 句柄
 * @return XY_OLED_OK 成功，其他值失败
 */
int xy_oled_ssd1306_init(xy_oled_ssd1306_t *dev, void *i2c_handle);

/**
 * @brief 反初始化 OLED
 * @param dev OLED 设备句柄
 * @return XY_OLED_OK 成功，其他值失败
 */
int xy_oled_ssd1306_deinit(xy_oled_ssd1306_t *dev);

/**
 * @brief 清屏
 * @param dev OLED 设备句柄
 * @return XY_OLED_OK 成功，其他值失败
 */
int xy_oled_ssd1306_clear(xy_oled_ssd1306_t *dev);

/**
 * @brief 刷新显示
 * @param dev OLED 设备句柄
 * @return XY_OLED_OK 成功，其他值失败
 */
int xy_oled_ssd1306_refresh(xy_oled_ssd1306_t *dev);

/**
 * @brief 绘制像素
 * @param dev OLED 设备句柄
 * @param x X 坐标 (0-127)
 * @param y Y 坐标 (0-63)
 * @param color 颜色 (0=黑，1=白)
 * @return XY_OLED_OK 成功，其他值失败
 */
int xy_oled_ssd1306_draw_pixel(xy_oled_ssd1306_t *dev, int16_t x, int16_t y, uint8_t color);

/**
 * @brief 绘制字符 (5x7 字体)
 * @param dev OLED 设备句柄
 * @param x X 坐标
 * @param y Y 坐标
 * @param ch 字符
 * @param color 颜色
 * @return XY_OLED_OK 成功，其他值失败
 */
int xy_oled_ssd1306_draw_char(xy_oled_ssd1306_t *dev, int16_t x, int16_t y, char ch, uint8_t color);

/**
 * @brief 绘制字符串
 * @param dev OLED 设备句柄
 * @param x X 坐标
 * @param y Y 坐标
 * @param str 字符串
 * @param color 颜色
 * @return XY_OLED_OK 成功，其他值失败
 */
int xy_oled_ssd1306_draw_string(xy_oled_ssd1306_t *dev, int16_t x, int16_t y, const char *str, uint8_t color);

/**
 * @brief 绘制水平线
 * @param dev OLED 设备句柄
 * @param x1 起点 X
 * @param y Y 坐标
 * @param x2 终点 X
 * @param color 颜色
 * @return XY_OLED_OK 成功，其他值失败
 */
int xy_oled_ssd1306_draw_hline(xy_oled_ssd1306_t *dev, int16_t x1, int16_t y, int16_t x2, uint8_t color);

/**
 * @brief 绘制垂直线
 * @param dev OLED 设备句柄
 * @param x X 坐标
 * @param y1 起点 Y
 * @param y2 终点 Y
 * @param color 颜色
 * @return XY_OLED_OK 成功，其他值失败
 */
int xy_oled_ssd1306_draw_vline(xy_oled_ssd1306_t *dev, int16_t x, int16_t y1, int16_t y2, uint8_t color);

/**
 * @brief 绘制矩形 (空心)
 * @param dev OLED 设备句柄
 * @param x X 坐标
 * @param y Y 坐标
 * @param w 宽度
 * @param h 高度
 * @param color 颜色
 * @return XY_OLED_OK 成功，其他值失败
 */
int xy_oled_ssd1306_draw_rect(xy_oled_ssd1306_t *dev, int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color);

/**
 * @brief 绘制矩形 (实心)
 * @param dev OLED 设备句柄
 * @param x X 坐标
 * @param y Y 坐标
 * @param w 宽度
 * @param h 高度
 * @param color 颜色
 * @return XY_OLED_OK 成功，其他值失败
 */
int xy_oled_ssd1306_fill_rect(xy_oled_ssd1306_t *dev, int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color);

/**
 * @brief 设置对比度
 * @param dev OLED 设备句柄
 * @param contrast 对比度 (0-255)
 * @return XY_OLED_OK 成功，其他值失败
 */
int xy_oled_ssd1306_set_contrast(xy_oled_ssd1306_t *dev, uint8_t contrast);

/**
 * @brief 开启显示
 * @param dev OLED 设备句柄
 * @return XY_OLED_OK 成功，其他值失败
 */
int xy_oled_ssd1306_display_on(xy_oled_ssd1306_t *dev);

/**
 * @brief 关闭显示
 * @param dev OLED 设备句柄
 * @return XY_OLED_OK 成功，其他值失败
 */
int xy_oled_ssd1306_display_off(xy_oled_ssd1306_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* XY_OLED_SSD1306_H */
