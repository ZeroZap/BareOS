/**
 * @file xy_rgb.h
 * @brief RGB LED Effect Library - Main Interface
 * @version 1.0.0
 * @date 2026-03-02
 */

#ifndef XY_RGB_H
#define XY_RGB_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 版本信息 ==================== */

#define XY_RGB_VERSION_MAJOR    1
#define XY_RGB_VERSION_MINOR    0
#define XY_RGB_VERSION_PATCH    0

/* ==================== 配置 ==================== */

#ifndef XY_RGB_MAX_LEDS
#define XY_RGB_MAX_LEDS     256     /* 最大 LED 数量 */
#endif

#ifndef XY_RGB_MAX_SEGMENTS
#define XY_RGB_MAX_SEGMENTS 8       /* 最大分段数 */
#endif

/* ==================== 颜色定义 ==================== */

/**
 * @brief RGB 颜色结构
 */
typedef struct {
    uint8_t r;    /* 红色 (0-255) */
    uint8_t g;    /* 绿色 (0-255) */
    uint8_t b;    /* 蓝色 (0-255) */
} rgb_color_t;

/**
 * @brief HSV 颜色结构
 */
typedef struct {
    uint8_t h;    /* 色相 (0-255, 0-360°) */
    uint8_t s;    /* 饱和度 (0-255, 0-100%) */
    uint8_t v;    /* 亮度 (0-255, 0-100%) */
} hsv_color_t;

/* 预定义颜色 */
#define RGB_COLOR_BLACK     {0, 0, 0}
#define RGB_COLOR_WHITE     {255, 255, 255}
#define RGB_COLOR_RED       {255, 0, 0}
#define RGB_COLOR_GREEN     {0, 255, 0}
#define RGB_COLOR_BLUE      {0, 0, 255}
#define RGB_COLOR_YELLOW    {255, 255, 0}
#define RGB_COLOR_CYAN      {0, 255, 255}
#define RGB_COLOR_MAGENTA   {255, 0, 255}
#define RGB_COLOR_ORANGE    {255, 128, 0}
#define RGB_COLOR_PURPLE    {128, 0, 255}

/* ==================== 驱动类型 ==================== */

typedef enum {
    XY_RGB_DRV_GPIO = 0,    /* GPIO 位 bang */
    XY_RGB_DRV_SPI,         /* SPI 驱动 */
    XY_RGB_DRV_I2S,         /* I2S 驱动 */
} xy_rgb_drv_type_t;

/* ==================== 效果 ID ==================== */

typedef enum {
    /* 静态效果 */
    FX_STATIC = 0,
    FX_RAINBOW,
    FX_GRADIENT,
    
    /* 动态效果 */
    FX_SCAN,
    FX_CHASE,
    FX_FADE,
    FX_BLINK,
    FX_TWINKLE,
    FX_COMET,
    FX_FIRE,
    FX_WATER,
    FX_BREATH,
    FX流星,
    
    /* 特殊效果 */
    FX_SPECTRUM,
    FX_VU_METER,
    
    FX_COUNT,               /* 效果总数 */
} xy_rgb_effect_t;

/* ==================== 错误码 ==================== */

typedef enum {
    XY_RGB_OK = 0,
    XY_RGB_ERROR,
    XY_RGB_ERROR_INVALID_PARAM,
    XY_RGB_ERROR_NO_MEMORY,
    XY_RGB_ERROR_NOT_SUPPORTED,
} xy_rgb_error_t;

/* ==================== 配置结构 ==================== */

/**
 * @brief RGB 配置
 */
typedef struct {
    uint16_t num_leds;          /* LED 数量 */
    uint8_t brightness;         /* 亮度 (0-255) */
    xy_rgb_drv_type_t drv_type; /* 驱动类型 */
    void *drv_handle;           /* 驱动句柄 */
} xy_rgb_config_t;

/* ==================== 核心 API ==================== */

/**
 * @brief 初始化 RGB 库
 * @param config 配置
 * @return XY_RGB_OK 成功，其他值失败
 */
int32_t xy_rgb_init(xy_rgb_config_t *config);

/**
 * @brief 反初始化
 * @return XY_RGB_OK 成功，其他值失败
 */
int32_t xy_rgb_deinit(void);

/**
 * @brief 设置亮度
 * @param brightness 亮度 (0-255)
 */
void xy_rgb_set_brightness(uint8_t brightness);

/**
 * @brief 获取亮度
 * @return 亮度值
 */
uint8_t xy_rgb_get_brightness(void);

/**
 * @brief 设置所有 LED 颜色
 * @param color 颜色
 */
void xy_rgb_set_all(rgb_color_t color);

/**
 * @brief 设置单个 LED 颜色
 * @param index LED 索引
 * @param color 颜色
 */
void xy_rgb_set_pixel(uint16_t index, rgb_color_t color);

/**
 * @brief 获取 LED 颜色
 * @param index LED 索引
 * @return 颜色
 */
rgb_color_t xy_rgb_get_pixel(uint16_t index);

/**
 * @brief 清除所有 LED
 */
void xy_rgb_clear(void);

/**
 * @brief 显示更新 (发送数据到 LED)
 */
void xy_rgb_show(void);

/**
 * @brief 服务循环 (在 main 循环中调用)
 */
void xy_rgb_service(void);

/* ==================== 效果 API ==================== */

/**
 * @brief 设置全局效果
 * @param effect 效果 ID
 */
void xy_rgb_set_effect(xy_rgb_effect_t effect);

/**
 * @brief 设置效果参数
 * @param speed 速度 (0-255)
 * @param intensity 强度 (0-255)
 */
void xy_rgb_set_effect_params(uint16_t speed, uint16_t intensity);

/**
 * @brief 获取当前效果
 * @return 效果 ID
 */
xy_rgb_effect_t xy_rgb_get_effect(void);

/* ==================== 颜色工具 ==================== */

/**
 * @brief RGB 转 HSV
 * @param rgb RGB 颜色
 * @return HSV 颜色
 */
hsv_color_t xy_rgb_to_hsv(rgb_color_t rgb);

/**
 * @brief HSV 转 RGB
 * @param hsv HSV 颜色
 * @return RGB 颜色
 */
rgb_color_t xy_hsv_to_rgb(hsv_color_t hsv);

/**
 * @brief 颜色混合
 * @param color1 颜色 1
 * @param color2 颜色 2
 * @param factor 混合因子 (0-255, 0=color1, 255=color2)
 * @return 混合颜色
 */
rgb_color_t xy_color_blend(rgb_color_t color1, rgb_color_t color2, uint8_t factor);

/**
 * @brief 生成彩虹颜色
 * @param hue 色相 (0-255)
 * @return RGB 颜色
 */
rgb_color_t xy_rainbow_color(uint8_t hue);

#ifdef __cplusplus
}
#endif

#endif /* XY_RGB_H */
