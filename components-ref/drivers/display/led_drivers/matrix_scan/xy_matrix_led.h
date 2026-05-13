/**
 * @file xy_matrix_led.h
 * @brief LED Matrix Driver - MAX7219/HT16K33 Dot Matrix Display
 * @version 1.0.0
 * @date 2026-03-02
 */

#ifndef XY_MATRIX_LED_H
#define XY_MATRIX_LED_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 配置 ==================== */

#ifndef MATRIX_MAX_WIDTH
#define MATRIX_MAX_WIDTH    32      // 最大宽度
#endif

#ifndef MATRIX_MAX_HEIGHT
#define MATRIX_MAX_HEIGHT   32      // 最大高度
#endif

/* ==================== LED 类型 ==================== */

typedef enum {
    XY_MATRIX_MAX7219 = 0,      // MAX7219 (SPI)
    XY_MATRIX_HT16K33,          // HT16K33 (I2C)
    XY_MATRIX_CUSTOM,           // 自定义矩阵
} xy_matrix_type_t;

/* ==================== 数据结构 ==================== */

/**
 * @brief 矩阵配置
 */
typedef struct {
    xy_matrix_type_t type;      // 矩阵类型
    uint8_t width;              // 宽度
    uint8_t height;             // 高度
    uint8_t num_devices;        // 设备数量 (级联)
    void *bus_handle;           // 总线句柄 (SPI/I2C)
    uint8_t brightness;         // 亮度 (0-15)
} xy_matrix_config_t;

/**
 * @brief 矩阵管理器
 */
typedef struct {
    xy_matrix_config_t config;
    uint8_t buffer[MATRIX_MAX_WIDTH][MATRIX_MAX_HEIGHT / 8]; // 显示缓冲区
    bool dirty;                 // 需要更新
    
    // 硬件回调
    int (*write_cmd)(void *handle, uint8_t cmd, uint8_t data);
    int (*write_data)(void *handle, uint8_t *data, uint16_t len);
} xy_matrix_led_t;

/* ==================== 基础 API ==================== */

/**
 * @brief 初始化矩阵
 */
int xy_matrix_led_init(xy_matrix_led_t *matrix,
                       xy_matrix_config_t *config,
                       int (*write_cmd)(void*, uint8_t, uint8_t),
                       int (*write_data)(void*, uint8_t*, uint16_t));

/**
 * @brief 设置像素
 */
void xy_matrix_led_set_pixel(xy_matrix_led_t *matrix,
                             uint8_t x, uint8_t y,
                             bool on);

/**
 * @brief 获取像素
 */
bool xy_matrix_led_get_pixel(xy_matrix_led_t *matrix,
                             uint8_t x, uint8_t y);

/**
 * @brief 清除屏幕
 */
void xy_matrix_led_clear(xy_matrix_led_t *matrix);

/**
 * @brief 填充屏幕
 */
void xy_matrix_led_fill(xy_matrix_led_t *matrix, bool on);

/**
 * @brief 设置亮度
 */
void xy_matrix_led_set_brightness(xy_matrix_led_t *matrix, uint8_t brightness);

/**
 * @brief 显示更新
 */
void xy_matrix_led_show(xy_matrix_led_t *matrix);

/* ==================== 图形 API ==================== */

/**
 * @brief 绘制水平线
 */
void xy_matrix_draw_hline(xy_matrix_led_t *matrix,
                          uint8_t x0, uint8_t y, uint8_t x1,
                          bool on);

/**
 * @brief 绘制垂直线
 */
void xy_matrix_draw_vline(xy_matrix_led_t *matrix,
                          uint8_t x, uint8_t y0, uint8_t y1,
                          bool on);

/**
 * @brief 绘制直线
 */
void xy_matrix_draw_line(xy_matrix_led_t *matrix,
                         uint8_t x0, uint8_t y0,
                         uint8_t x1, uint8_t y1,
                         bool on);

/**
 * @brief 绘制矩形
 */
void xy_matrix_draw_rect(xy_matrix_led_t *matrix,
                         uint8_t x, uint8_t y,
                         uint8_t w, uint8_t h,
                         bool on, bool filled);

/**
 * @brief 绘制圆形
 */
void xy_matrix_draw_circle(xy_matrix_led_t *matrix,
                           uint8_t x0, uint8_t y0,
                           uint8_t radius,
                           bool on, bool filled);

/* ==================== 文字 API ==================== */

/**
 * @brief 绘制字符 (5x7 字体)
 */
void xy_matrix_draw_char(xy_matrix_led_t *matrix,
                         uint8_t x, uint8_t y,
                         char c, bool on);

/**
 * @brief 绘制字符串
 */
void xy_matrix_draw_string(xy_matrix_led_t *matrix,
                           uint8_t x, uint8_t y,
                           const char *str, bool on);

/**
 * @brief 滚动文字
 */
void xy_matrix_scroll_text(xy_matrix_led_t *matrix,
                           const char *text,
                           uint8_t speed);

/* ==================== 效果 API ==================== */

/**
 * @brief 等离子效果
 */
void xy_matrix_effect_plasma(xy_matrix_led_t *matrix, uint16_t speed);

/**
 * @brief 生命游戏
 */
void xy_matrix_effect_game_of_life(xy_matrix_led_t *matrix, uint16_t speed);

/**
 * @brief 矩阵雨
 */
void xy_matrix_effect_matrix_rain(xy_matrix_led_t *matrix,
                                  uint16_t speed,
                                  bool on);

/**
 * @brief 更新效果
 */
void xy_matrix_update_effects(xy_matrix_led_t *matrix);

#ifdef __cplusplus
}
#endif

#endif /* XY_MATRIX_LED_H */
