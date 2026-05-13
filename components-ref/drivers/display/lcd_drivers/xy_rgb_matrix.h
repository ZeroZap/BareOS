/**
 * @file xy_rgb_matrix.h
 * @brief RGB LED Matrix Mode (2D Grid)
 * @version 1.0.0
 * @date 2026-03-02
 */

#ifndef XY_RGB_MATRIX_H
#define XY_RGB_MATRIX_H

#include "xy_rgb.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 矩阵配置
 */
typedef struct {
    uint16_t width;           /* 宽度 */
    uint16_t height;          /* 高度 */
    uint8_t wiring;           /* 布线方式：0=逐行，1=蛇形 */
} xy_rgb_matrix_config_t;

/**
 * @brief 初始化 Matrix 模式
 * @param config 配置
 * @return XY_RGB_OK 成功，其他值失败
 */
int32_t xy_rgb_matrix_init(xy_rgb_matrix_config_t *config);

/**
 * @brief 设置像素颜色
 * @param x X 坐标 (0 ~ width-1)
 * @param y Y 坐标 (0 ~ height-1)
 * @param color 颜色
 */
void xy_rgb_matrix_set_pixel(uint16_t x, uint16_t y, rgb_color_t color);

/**
 * @brief 获取像素颜色
 * @param x X 坐标
 * @param y Y 坐标
 * @return 颜色
 */
rgb_color_t xy_rgb_matrix_get_pixel(uint16_t x, uint16_t y);

/**
 * @brief 绘制水平线
 * @param x0 起始 X
 * @param y Y 坐标
 * @param x1 结束 X
 * @param color 颜色
 */
void xy_rgb_matrix_draw_hline(uint16_t x0, uint16_t y, uint16_t x1, 
                              rgb_color_t color);

/**
 * @brief 绘制垂直线
 * @param x X 坐标
 * @param y0 起始 Y
 * @param y1 结束 Y
 * @param color 颜色
 */
void xy_rgb_matrix_draw_vline(uint16_t x, uint16_t y0, uint16_t y1, 
                              rgb_color_t color);

/**
 * @brief 绘制直线 (Bresenham 算法)
 * @param x0 起始 X
 * @param y0 起始 Y
 * @param x1 结束 X
 * @param y1 结束 Y
 * @param color 颜色
 */
void xy_rgb_matrix_draw_line(uint16_t x0, uint16_t y0, 
                             uint16_t x1, uint16_t y1, 
                             rgb_color_t color);

/**
 * @brief 绘制矩形
 * @param x X 坐标
 * @param y Y 坐标
 * @param w 宽度
 * @param h 高度
 * @param color 颜色
 * @param filled 是否填充
 */
void xy_rgb_matrix_draw_rect(uint16_t x, uint16_t y, 
                             uint16_t w, uint16_t h,
                             rgb_color_t color, bool filled);

/**
 * @brief 绘制圆形 (中点算法)
 * @param x0 圆心 X
 * @param y0 圆心 Y
 * @param radius 半径
 * @param color 颜色
 * @param filled 是否填充
 */
void xy_rgb_matrix_draw_circle(uint16_t x0, uint16_t y0, 
                               uint8_t radius, rgb_color_t color, 
                               bool filled);

/**
 * @brief 绘制字符 (5x7 字体)
 * @param x X 坐标
 * @param y Y 坐标
 * @param c 字符
 * @param color 颜色
 */
void xy_rgb_matrix_draw_char(uint16_t x, uint16_t y, char c, 
                             rgb_color_t color);

/**
 * @brief 绘制字符串
 * @param x X 坐标
 * @param y Y 坐标
 * @param text 字符串
 * @param color 颜色
 */
void xy_rgb_matrix_draw_string(uint16_t x, uint16_t y, 
                               const char *text, rgb_color_t color);

/**
 * @brief 清除屏幕
 */
void xy_rgb_matrix_clear(void);

/**
 * @brief 获取宽度
 * @return 宽度
 */
uint16_t xy_rgb_matrix_get_width(void);

/**
 * @brief 获取高度
 * @return 高度
 */
uint16_t xy_rgb_matrix_get_height(void);

/* ==================== Matrix 效果 ==================== */

/**
 * @brief 等离子效果
 * @param speed 速度
 */
void xy_rgb_matrix_fx_plasma(uint16_t speed);

/**
 * @brief 生命游戏
 * @param speed 速度
 */
void xy_rgb_matrix_fx_game_of_life(uint16_t speed);

/**
 * @brief 文字滚动
 * @param text 文字
 * @param speed 速度
 * @param color 颜色
 */
void xy_rgb_matrix_fx_scroll_text(const char *text, uint16_t speed, 
                                  rgb_color_t color);

/**
 * @brief 频谱分析
 * @param spectrum 频谱数据
 * @param bands 频段数量
 */
void xy_rgb_matrix_fx_spectrum(uint8_t *spectrum, uint8_t bands);

#ifdef __cplusplus
}
#endif

#endif /* XY_RGB_MATRIX_H */
