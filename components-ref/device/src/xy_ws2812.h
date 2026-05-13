/**
 * @file xy_ws2812.h
 * @brief WS2812/SK6812 RGB LED Driver
 * @version 1.0.0
 * @date 2026-03-15
 * 
 * @note 支持 WS2812B/SK6812 等单总线 RGB LED
 */

#ifndef XY_WS2812_H
#define XY_WS2812_H

#include "xy_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== WS2812 Device Structure ==================== */

/**
 * @brief WS2812 设备结构
 */
typedef struct {
    xy_device_t base;              /**< 设备基类 */
    void *gpio_handle;             /**< GPIO 句柄 */
    uint32_t gpio_pin;             /**< GPIO 引脚 */
    uint16_t led_count;            /**< LED 数量 */
    uint8_t *buffer;               /**< 数据缓冲区 */
    size_t buffer_size;            /**< 缓冲区大小 */
    bool initialized;              /**< 初始化标志 */
} xy_ws2812_t;

/* ==================== Color Structure ==================== */

/**
 * @brief RGB 颜色
 */
typedef struct {
    uint8_t red;                   /**< 红色 (0-255) */
    uint8_t green;                 /**< 绿色 (0-255) */
    uint8_t blue;                  /**< 蓝色 (0-255) */
} xy_color_t;

/* ==================== WS2812 API ==================== */

/**
 * @brief 初始化 WS2812
 * @param dev WS2812 设备句柄
 * @param gpio_handle GPIO 句柄
 * @param gpio_pin GPIO 引脚
 * @param led_count LED 数量
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_ws2812_init(xy_ws2812_t *dev, void *gpio_handle, 
                   uint32_t gpio_pin, uint16_t led_count);

/**
 * @brief 反初始化 WS2812
 * @param dev WS2812 设备句柄
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_ws2812_deinit(xy_ws2812_t *dev);

/**
 * @brief 设置单个 LED 颜色
 * @param dev WS2812 设备句柄
 * @param index LED 索引 (0 到 led_count-1)
 * @param color 颜色值
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_ws2812_set_pixel(xy_ws2812_t *dev, uint16_t index, xy_color_t color);

/**
 * @brief 设置所有 LED 颜色
 * @param dev WS2812 设备句柄
 * @param color 颜色值
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_ws2812_fill(xy_ws2812_t *dev, xy_color_t color);

/**
 * @brief 清空所有 LED (关闭)
 * @param dev WS2812 设备句柄
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_ws2812_clear(xy_ws2812_t *dev);

/**
 * @brief 刷新显示 (发送数据到 LED)
 * @param dev WS2812 设备句柄
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_ws2812_show(xy_ws2812_t *dev);

/**
 * @brief 获取 LED 数量
 * @param dev WS2812 设备句柄
 * @return LED 数量
 */
uint16_t xy_ws2812_get_count(xy_ws2812_t *dev);

/**
 * @brief 颜色混合
 * @param c1 颜色 1
 * @param c2 颜色 2
 * @param factor 混合因子 (0-255, 0=c1, 255=c2)
 * @return 混合后的颜色
 */
xy_color_t xy_color_blend(xy_color_t c1, xy_color_t c2, uint8_t factor);

/**
 * @brief 颜色渐变
 * @param start 起始颜色
 * @param end 结束颜色
 * @param steps 步数
 * @param current_step 当前步 (0 到 steps-1)
 * @return 当前颜色
 */
xy_color_t xy_color_gradient(xy_color_t start, xy_color_t end, 
                             uint16_t steps, uint16_t current_step);

/**
 * @brief HSV 转 RGB
 * @param hue 色调 (0-360)
 * @param saturation 饱和度 (0-255)
 * @param value 亮度 (0-255)
 * @return RGB 颜色
 */
xy_color_t xy_color_hsv_to_rgb(uint16_t hue, uint8_t saturation, uint8_t value);

/**
 * @brief 彩虹颜色
 * @param position 位置 (0-255)
 * @param brightness 亮度 (0-255)
 * @return 彩虹颜色
 */
xy_color_t xy_color_rainbow(uint8_t position, uint8_t brightness);

#ifdef __cplusplus
}
#endif

#endif /* XY_WS2812_H */
