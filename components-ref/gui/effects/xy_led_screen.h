/**
 * @file xy_led_screen.h
 * @brief LED Screen Effects Library - Matrix/Display Effects
 * @version 1.0.0
 * @date 2026-03-02
 * 
 * 参考 GUI 文档：screen_effects-1to32bit.md
 * 支持：1bit/8bit/32bit 色彩深度
 */

#ifndef XY_LED_SCREEN_H
#define XY_LED_SCREEN_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 配置 ==================== */

#ifndef SCREEN_MAX_WIDTH
#define SCREEN_MAX_WIDTH    64      // 最大宽度
#endif

#ifndef SCREEN_MAX_HEIGHT
#define SCREEN_MAX_HEIGHT   64      // 最大高度
#endif

/* ==================== 数据结构 ==================== */

/**
 * @brief 色彩格式
 */
typedef enum {
    XY_SCREEN_1BPP = 0,     // 1 位单色
    XY_SCREEN_8BPP,         // 8 位灰度/索引色
    XY_SCREEN_RGB565,       // 16 位 RGB
    XY_SCREEN_RGB888,       // 24 位 RGB
    XY_SCREEN_ARGB8888,     // 32 位 ARGB
} xy_screen_format_t;

/**
 * @brief 颜色结构
 */
typedef struct {
    uint8_t a;      // Alpha (0-255)
    uint8_t r;      // 红色 (0-255)
    uint8_t g;      // 绿色 (0-255)
    uint8_t b;      // 蓝色 (0-255)
} xy_screen_color_t;

/**
 * @brief LED 屏幕缓冲区
 */
typedef struct {
    uint32_t *data;             // 像素数据
    uint16_t width;             // 宽度
    uint16_t height;            // 高度
    uint16_t stride;            // 每行字节数
    xy_screen_format_t format;  // 色彩格式
    uint32_t palette[256];      // 调色板 (索引色用)
    uint8_t palette_size;       // 调色板大小
} xy_screen_buffer_t;

/**
 * @brief 屏幕效果管理器
 */
typedef struct {
    xy_screen_buffer_t buffer[2];   // 双缓冲
    uint8_t current_buffer;         // 当前缓冲
    uint16_t width;
    uint16_t height;
    xy_screen_format_t format;
    bool dirty;                     // 需要更新
    
    // 效果状态
    uint16_t effect_frame;
    uint16_t effect_progress;
    
    // 硬件回调
    void (*send_buffer)(uint32_t *data, uint16_t width, uint16_t height);
} xy_led_screen_t;

/* ==================== 基础 API ==================== */

/**
 * @brief 初始化 LED 屏幕
 */
int xy_led_screen_init(xy_led_screen_t *screen,
                       uint16_t width, uint16_t height,
                       xy_screen_format_t format,
                       void (*send_buffer)(uint32_t*, uint16_t, uint16_t));

/**
 * @brief 设置像素
 */
void xy_led_screen_set_pixel(xy_led_screen_t *screen,
                             int16_t x, int16_t y,
                             xy_screen_color_t color);

/**
 * @brief 获取像素
 */
xy_screen_color_t xy_led_screen_get_pixel(xy_led_screen_t *screen,
                                          int16_t x, int16_t y);

/**
 * @brief 清除屏幕
 */
void xy_led_screen_clear(xy_led_screen_t *screen);

/**
 * @brief 填充屏幕
 */
void xy_led_screen_fill(xy_led_screen_t *screen,
                        xy_screen_color_t color);

/**
 * @brief 交换缓冲 (显示)
 */
void xy_led_screen_swap(xy_led_screen_t *screen);

/**
 * @brief 强制更新
 */
void xy_led_screen_update(xy_led_screen_t *screen);

/* ==================== 图形 API ==================== */

/**
 * @brief 绘制水平线
 */
void xy_led_screen_draw_hline(xy_led_screen_t *screen,
                              int16_t x0, int16_t y, int16_t x1,
                              xy_screen_color_t color);

/**
 * @brief 绘制垂直线
 */
void xy_led_screen_draw_vline(xy_led_screen_t *screen,
                              int16_t x, int16_t y0, int16_t y1,
                              xy_screen_color_t color);

/**
 * @brief 绘制直线 (Bresenham)
 */
void xy_led_screen_draw_line(xy_led_screen_t *screen,
                             int16_t x0, int16_t y0,
                             int16_t x1, int16_t y1,
                             xy_screen_color_t color);

/**
 * @brief 绘制矩形
 */
void xy_led_screen_draw_rect(xy_led_screen_t *screen,
                             int16_t x, int16_t y,
                             int16_t w, int16_t h,
                             xy_screen_color_t color,
                             bool filled);

/**
 * @brief 绘制圆形 (中点算法)
 */
void xy_led_screen_draw_circle(xy_led_screen_t *screen,
                               int16_t x0, int16_t y0,
                               int16_t radius,
                               xy_screen_color_t color,
                               bool filled);

/**
 * @brief 绘制字符 (5x7 字体)
 */
void xy_led_screen_draw_char(xy_led_screen_t *screen,
                             int16_t x, int16_t y,
                             char c,
                             xy_screen_color_t color);

/**
 * @brief 绘制字符串
 */
void xy_led_screen_draw_string(xy_led_screen_t *screen,
                               int16_t x, int16_t y,
                               const char *str,
                               xy_screen_color_t color);

/* ==================== 基础效果 ==================== */

/**
 * @brief 滚动效果
 */
typedef enum {
    XY_SCREEN_SCROLL_LEFT = 0,
    XY_SCREEN_SCROLL_RIGHT,
    XY_SCREEN_SCROLL_UP,
    XY_SCREEN_SCROLL_DOWN,
} xy_screen_scroll_dir_t;

/**
 * @brief 滚动文字
 */
void xy_led_screen_scroll_text(xy_led_screen_t *screen,
                               const char *text,
                               xy_screen_scroll_dir_t dir,
                               uint8_t speed,
                               xy_screen_color_t color);

/**
 * @brief 淡入效果
 */
void xy_led_screen_fade_in(xy_led_screen_t *screen,
                           uint8_t speed);

/**
 * @brief 淡出效果
 */
void xy_led_screen_fade_out(xy_led_screen_t *screen,
                            uint8_t speed);

/**
 * @brief 缩放效果
 */
void xy_led_screen_zoom(xy_led_screen_t *screen,
                        uint8_t scale,      // 0-255 (128=100%)
                        bool from_center);

/**
 * @brief 旋转效果 (90 度倍数)
 */
void xy_led_screen_rotate(xy_led_screen_t *screen,
                          uint8_t angle);   // 0/90/180/270

/* ==================== 高级效果 ==================== */

/**
 * @brief 百叶窗效果
 */
void xy_led_screen_blinds(xy_led_screen_t *screen,
                          uint8_t step,
                          uint8_t blind_size,
                          bool horizontal);

/**
 * @brief 溶解效果
 */
void xy_led_screen_dissolve(xy_led_screen_t *screen,
                            uint8_t density);

/**
 * @brief 波浪效果
 */
void xy_led_screen_wave(xy_led_screen_t *screen,
                        uint8_t amplitude,
                        uint8_t frequency,
                        uint8_t phase);

/**
 * @brief 水波纹效果
 */
void xy_led_screen_ripple(xy_led_screen_t *screen,
                          int16_t center_x, int16_t center_y,
                          uint8_t radius,
                          xy_screen_color_t color);

/**
 * @brief 粒子效果 (雨/雪/火)
 */
typedef enum {
    XY_SCREEN_PARTICLE_RAIN = 0,
    XY_SCREEN_PARTICLE_SNOW,
    XY_SCREEN_PARTICLE_FIRE,
} xy_screen_particle_t;

/**
 * @brief 粒子效果
 */
void xy_led_screen_particles(xy_led_screen_t *screen,
                             xy_screen_particle_t type,
                             uint8_t density,
                             uint8_t speed);

/* ==================== 3D 效果 ==================== */

/**
 * @brief 3D 翻转效果
 */
void xy_led_screen_flip_3d(xy_led_screen_t *screen,
                           bool horizontal,
                           uint8_t step);

/**
 * @brief 立方体旋转
 */
void xy_led_screen_cube_rotate(xy_led_screen_t *screen,
                               uint8_t angle_x,
                               uint8_t angle_y,
                               uint8_t angle_z);

#ifdef __cplusplus
}
#endif

#endif /* XY_LED_SCREEN_H */
