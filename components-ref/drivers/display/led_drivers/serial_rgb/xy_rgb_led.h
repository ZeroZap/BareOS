/**
 * @file xy_rgb_led.h
 * @brief RGB LED Driver - WS2812B/SK6812 Addressable RGB LEDs
 * @version 1.0.0
 * @date 2026-03-02
 */

#ifndef XY_RGB_LED_H
#define XY_RGB_LED_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 配置 ==================== */

#ifndef XY_RGB_LED_MAX
#define XY_RGB_LED_MAX      256         // 最大 LED 数量
#endif

/* ==================== LED 类型 ==================== */

typedef enum {
    XY_RGB_LED_WS2812B = 0,     // WS2812B (GRB)
    XY_RGB_LED_WS2811,          // WS2811 (RGB)
    XY_RGB_LED_SK6812,          // SK6812 (GRB)
    XY_RGB_LED_SK6812_RGBW,     // SK6812 RGBW
    XY_RGB_LED_APA102,          // APA102 (SPI)
    XY_RGB_LED_WS2813,          // WS2813 (GRB)
    XY_RGB_LED_WS2815,          // WS2815 (GRB)
} xy_rgb_led_type_t;

/* ==================== 颜色结构 ==================== */

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t w;      // 白色通道 (RGBW LED)
} xy_rgb_color_t;

/* 预定义颜色 */
#define XY_RGB_BLACK    {0, 0, 0, 0}
#define XY_RGB_WHITE    {255, 255, 255, 0}
#define XY_RGB_RED      {255, 0, 0, 0}
#define XY_RGB_GREEN    {0, 255, 0, 0}
#define XY_RGB_BLUE     {0, 0, 255, 0}
#define XY_RGB_YELLOW   {255, 255, 0, 0}
#define XY_RGB_CYAN     {0, 255, 255, 0}
#define XY_RGB_MAGENTA  {255, 0, 255, 0}

/* ==================== 数据结构 ==================== */

/**
 * @brief RGB LED 配置
 */
typedef struct {
    uint16_t num_leds;          // LED 数量
    xy_rgb_led_type_t type;     // LED 类型
    uint8_t pin;                // GPIO 引脚
    uint8_t pin_clk;            // 时钟引脚 (APA102)
    uint8_t brightness;         // 亮度 (0-255)
    bool use_dma;               // 使用 DMA
} xy_rgb_led_config_t;

/**
 * @brief RGB LED 管理器
 */
typedef struct {
    xy_rgb_led_config_t config;
    xy_rgb_color_t leds[XY_RGB_LED_MAX];    // LED 颜色缓冲区
    uint8_t *tx_buffer;                     // 发送缓冲区
    uint16_t buffer_size;                   // 缓冲区大小
    bool dirty;                             // 需要更新
    
    // 硬件回调
    void (*send_data)(uint8_t *data, uint16_t len);
} xy_rgb_led_t;

/* ==================== 基础 API ==================== */

/**
 * @brief 初始化 RGB LED
 */
int xy_rgb_led_init(xy_rgb_led_t *led, xy_rgb_led_config_t *config);

/**
 * @brief 反初始化
 */
void xy_rgb_led_deinit(xy_rgb_led_t *led);

/**
 * @brief 设置 LED 颜色
 */
void xy_rgb_led_set_pixel(xy_rgb_led_t *led,
                          uint16_t index,
                          xy_rgb_color_t color);

/**
 * @brief 获取 LED 颜色
 */
xy_rgb_color_t xy_rgb_led_get_pixel(xy_rgb_led_t *led, uint16_t index);

/**
 * @brief 填充所有 LED
 */
void xy_rgb_led_fill(xy_rgb_led_t *led, xy_rgb_color_t color);

/**
 * @brief 清除所有 LED
 */
void xy_rgb_led_clear(xy_rgb_led_t *led);

/**
 * @brief 设置亮度
 */
void xy_rgb_led_set_brightness(xy_rgb_led_t *led, uint8_t brightness);

/**
 * @brief 显示更新 (发送数据到 LED)
 */
void xy_rgb_led_show(xy_rgb_led_t *led);

/* ==================== 颜色工具 ==================== */

/**
 * @brief HSV 转 RGB
 */
xy_rgb_color_t xy_rgb_hsv_to_rgb(uint8_t h, uint8_t s, uint8_t v);

/**
 * @brief RGB 转 HSV
 */
void xy_rgb_rgb_to_hsv(xy_rgb_color_t rgb, uint8_t *h, uint8_t *s, uint8_t *v);

/**
 * @brief 颜色混合
 */
xy_rgb_color_t xy_rgb_color_blend(xy_rgb_color_t c1,
                                  xy_rgb_color_t c2,
                                  uint8_t factor);

/**
 * @brief 彩虹颜色
 */
xy_rgb_color_t xy_rgb_rainbow_color(uint8_t hue);

/* ==================== 效果 API ==================== */

/**
 * @brief 效果 ID
 */
typedef enum {
    FX_STATIC = 0,
    FX_BLINK,
    FX_BREATH,
    FX_RAINBOW,
    FX_RAINBOW_CYCLE,
    FX_COLOR_WIPE,
    FX_THEATER_CHASE,
    FX_SCAN,
    FX_COMET,
    FX_FIRE,
    FX_TWINKLE,
    FX_METEOR,
    FX_COUNT,
} xy_rgb_fx_id_t;

/**
 * @brief 设置效果
 */
void xy_rgb_led_set_effect(xy_rgb_led_t *led,
                           xy_rgb_fx_id_t effect_id,
                           uint16_t speed,
                           uint16_t intensity);

/**
 * @brief 更新效果
 */
void xy_rgb_led_update_effect(xy_rgb_led_t *led);

#ifdef __cplusplus
}
#endif

#endif /* XY_RGB_LED_H */
