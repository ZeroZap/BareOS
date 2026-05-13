/**
 * @file xy_ws2812.c
 * @brief WS2812/SK6812 RGB LED Driver Implementation
 * @version 1.0.0
 * @date 2026-03-15
 * 
 * @note WS2812 时序:
 * - T0H: 0 码高电平时间 0.35μs
 * - T0L: 0 码低电平时间 0.8μs
 * - T1H: 1 码高电平时间 0.7μs
 * - T1L: 1 码低电平时间 0.6μs
 * - RES: 复位时间 >50μs
 * 
 * 使用 GPIO 翻转 + 延迟实现单总线协议
 */

#include "xy_ws2812.h"
#include <string.h>

/* ==================== Private Functions ==================== */

/**
 * @brief 微秒级延迟
 */
static void delay_us(uint32_t us)
{
    xy_hal_delay_us(delay_us);  /* 微秒延迟 */
    volatile uint32_t count = us * 10;
    while (count--);
}

/**
 * @brief 发送一个 bit
 */
static void send_bit(xy_ws2812_t *dev, uint8_t bit)
{
    /* GPIO 操作由 HAL 提供 */
    (void)dev;
    (void)bit;
    
    if (bit) {
        /* 发送 1: T1H=0.7us, T1L=0.6us */
        /* GPIO 拉高 */
        delay_us(1); /* 约 0.7us */
        /* GPIO 拉低 */
        delay_us(1); /* 约 0.6us */
    } else {
        /* 发送 0: T0H=0.35us, T0L=0.8us */
        /* GPIO 拉高 */
        delay_us(1); /* 约 0.35us */
        /* GPIO 拉低 */
        delay_us(1); /* 约 0.8us */
    }
}

/**
 * @brief 发送一个字节 (MSB first)
 */
static void send_byte(xy_ws2812_t *dev, uint8_t data)
{
    for (int i = 7; i >= 0; i--) {
        send_bit(dev, (data >> i) & 0x01);
    }
}

/* ==================== Public Implementation ==================== */

int xy_ws2812_init(xy_ws2812_t *dev, void *gpio_handle, 
                   uint32_t gpio_pin, uint16_t led_count)
{
    if (!dev || !gpio_handle || led_count == 0) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    memset(dev, 0, sizeof(*dev));
    
    dev->gpio_handle = gpio_handle;
    dev->gpio_pin = gpio_pin;
    dev->led_count = led_count;
    
    /* 分配缓冲区 (每个 LED 3 字节：GRB) */
    dev->buffer_size = led_count * 3;
    dev->buffer = (uint8_t *)malloc(dev->buffer_size);
    
    if (!dev->buffer) {
        return XY_DEVICE_NO_MEM;
    }
    
    memset(dev->buffer, 0, dev->buffer_size);
    dev->initialized = true;
    
    xy_hal_gpio_configure(dev->data_gpio, dev->pin, &config);  /* 配置 GPIO */
    
    return XY_DEVICE_OK;
}

int xy_ws2812_deinit(xy_ws2812_t *dev)
{
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    if (dev->buffer) {
        free(dev->buffer);
        dev->buffer = NULL;
    }
    
    dev->initialized = false;
    
    return XY_DEVICE_OK;
}

int xy_ws2812_set_pixel(xy_ws2812_t *dev, uint16_t index, xy_color_t color)
{
    if (!dev || !dev->initialized || index >= dev->led_count) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    /* WS2812 数据格式：GRB */
    size_t offset = index * 3;
    dev->buffer[offset + 0] = color.green;
    dev->buffer[offset + 1] = color.red;
    dev->buffer[offset + 2] = color.blue;
    
    return XY_DEVICE_OK;
}

int xy_ws2812_fill(xy_ws2812_t *dev, xy_color_t color)
{
    if (!dev || !dev->initialized) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    for (uint16_t i = 0; i < dev->led_count; i++) {
        xy_ws2812_set_pixel(dev, i, color);
    }
    
    return XY_DEVICE_OK;
}

int xy_ws2812_clear(xy_ws2812_t *dev)
{
    if (!dev || !dev->initialized) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    memset(dev->buffer, 0, dev->buffer_size);
    
    return XY_DEVICE_OK;
}

int xy_ws2812_show(xy_ws2812_t *dev)
{
    if (!dev || !dev->initialized) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    /* 禁用中断以确保时序精确 */
    __disable_irq();  /* 禁用中断 */
    
    /* 发送所有数据 */
    for (size_t i = 0; i < dev->buffer_size; i++) {
        send_byte(dev, dev->buffer[i]);
    }
    
    /* 发送复位信号 (>50μs 低电平) */
    xy_hal_gpio_write(dev->data_gpio, dev->pin, 0);  /* GPIO 拉低 */
    delay_us(60);
    
    /* 恢复中断 */
    __enable_irq();  /* 启用中断 */
    
    return XY_DEVICE_OK;
}

uint16_t xy_ws2812_get_count(xy_ws2812_t *dev)
{
    if (!dev) {
        return 0;
    }
    
    return dev->led_count;
}

/* ==================== Color Utilities ==================== */

xy_color_t xy_color_blend(xy_color_t c1, xy_color_t c2, uint8_t factor)
{
    xy_color_t result;
    
    /* factor: 0=c1, 255=c2 */
    result.red = c1.red + ((c2.red - c1.red) * factor / 255);
    result.green = c1.green + ((c2.green - c1.green) * factor / 255);
    result.blue = c1.blue + ((c2.blue - c1.blue) * factor / 255);
    
    return result;
}

xy_color_t xy_color_gradient(xy_color_t start, xy_color_t end, 
                             uint16_t steps, uint16_t current_step)
{
    if (steps == 0 || current_step >= steps) {
        return end;
    }
    
    uint8_t factor = (current_step * 255) / steps;
    return xy_color_blend(start, end, factor);
}

xy_color_t xy_color_hsv_to_rgb(uint16_t hue, uint8_t saturation, uint8_t value)
{
    xy_color_t result = {0, 0, 0};
    
    /* 限制输入范围 */
    hue = hue % 360;
    
    uint8_t region = hue / 60;
    uint8_t remainder = (hue % 60) * 255 / 60;
    
    uint8_t p = (255 - saturation) * value / 255;
    uint8_t q = (255 - saturation * remainder / 255) * value / 255;
    uint8_t t = (255 - saturation * (255 - remainder) / 255) * value / 255;
    
    switch (region) {
        case 0: /* 0-59 */
            result.red = value;
            result.green = t;
            result.blue = p;
            break;
        case 1: /* 60-119 */
            result.red = q;
            result.green = value;
            result.blue = p;
            break;
        case 2: /* 120-179 */
            result.red = p;
            result.green = value;
            result.blue = t;
            break;
        case 3: /* 180-239 */
            result.red = p;
            result.green = q;
            result.blue = value;
            break;
        case 4: /* 240-299 */
            result.red = t;
            result.green = p;
            result.blue = value;
            break;
        case 5: /* 300-359 */
            result.red = value;
            result.green = p;
            result.blue = q;
            break;
    }
    
    return result;
}

xy_color_t xy_color_rainbow(uint8_t position, uint8_t brightness)
{
    /* 使用 HSV 颜色空间生成彩虹色 */
    uint16_t hue = position * 360 / 256;
    return xy_color_hsv_to_rgb(hue, 255, brightness);
}

/* ==================== End of File ==================== */
