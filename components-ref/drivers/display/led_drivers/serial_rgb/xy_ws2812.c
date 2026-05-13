/**
 * @file xy_ws2812.c
 * @brief WS2812/SK6812 RGB LED Driver Implementation
 * @version 1.0.0
 * @date 2026-03-02
 */

#include "xy_ws2812.h"
#include <string.h>

/* ==================== Static Variables ==================== */

static xy_ws2812_gpio_write_t s_gpio_write = NULL;
static uint8_t s_gpio_pin = 0;

/* ==================== Internal Functions ==================== */

/**
 * @brief Simple delay using busy wait (for bit-banging)
 * @param ns Delay time in nanoseconds
 * @note This is a simple implementation; for precise timing,
 *       use hardware timer or assembly-level optimization
 */
static void xy_ws2812_delay_ns(uint32_t ns)
{
    volatile uint32_t count = ns / 100;  /* Approximate for 100ns per iteration */
    while (count > 0) {
        count--;
    }
}

/**
 * @brief Set GPIO pin high
 */
static void gpio_set_high(void)
{
    if (s_gpio_write) {
        s_gpio_write(s_gpio_pin, 1);
    }
}

/**
 * @brief Set GPIO pin low
 */
static void gpio_set_low(void)
{
    if (s_gpio_write) {
        s_gpio_write(s_gpio_pin, 0);
    }
}

/**
 * @brief Encode single bit into timing
 * @param bit Value to send (0 or 1)
 */
static void send_bit(uint8_t bit)
{
    if (bit) {
        /* 1 bit: 700ns high, 550ns low */
        gpio_set_high();
        xy_ws2812_delay_ns(WS2812_T1H_NS);
        gpio_set_low();
        xy_ws2812_delay_ns(WS2812_T1L_NS);
    } else {
        /* 0 bit: 350ns high, 900ns low */
        gpio_set_high();
        xy_ws2812_delay_ns(WS2812_T0H_NS);
        gpio_set_low();
        xy_ws2812_delay_ns(WS2812_T0L_NS);
    }
}

/**
 * @brief Encode single byte (8 bits)
 * @param byte Byte to send
 */
static void send_byte(uint8_t byte)
{
    uint8_t i;
    for (i = 0; i < 8; i++) {
        send_bit((byte >> (7 - i)) & 0x01);
    }
}

/**
 * @brief Encode color based on color order
 * @param color Color to encode
 * @param order Color order
 * @param buf Output buffer (3 or 4 bytes)
 */
static void encode_color(xy_ws2812_color_t color,
                         xy_ws2812_color_order_t order,
                         uint8_t *buf)
{
    switch (order) {
        case WS2812_COLOR_GRB:
            buf[0] = color.g;
            buf[1] = color.r;
            buf[2] = color.b;
            break;
        case WS2812_COLOR_RGB:
            buf[0] = color.r;
            buf[1] = color.g;
            buf[2] = color.b;
            break;
        case WS2812_COLOR_GBR:
            buf[0] = color.g;
            buf[1] = color.b;
            buf[2] = color.r;
            break;
        case WS2812_COLOR_RGBW:
            buf[0] = color.r;
            buf[1] = color.g;
            buf[2] = color.b;
            buf[3] = color.w;
            break;
    }
}

/* ==================== Public API Implementation ==================== */

xy_ws2812_error_t xy_ws2812_init(xy_ws2812_handle_t *handle,
                                  xy_ws2812_config_t *config)
{
    uint16_t buffer_size;

    if (handle == NULL || config == NULL) {
        return WS2812_ERROR_INVALID_PARAM;
    }

    if (config->num_leds == 0 || config->num_leds > 1024) {
        return WS2812_ERROR_INVALID_PARAM;
    }

    /* Calculate buffer size: 3 bytes per LED for RGB, 4 for RGBW */
    if (config->color_order == WS2812_COLOR_RGBW) {
        buffer_size = config->num_leds * 4;
    } else {
        buffer_size = config->num_leds * 3;
    }

    /* Initialize handle */
    memset(handle, 0, sizeof(xy_ws2812_handle_t));
    handle->config.num_leds = config->num_leds;
    handle->config.led_type = config->led_type;
    handle->config.data_pin = config->data_pin;
    handle->config.color_order = config->color_order;
    handle->config.use_dma = config->use_dma;
    handle->config.brightness = config->brightness > 0 ? config->brightness : WS2812_DEFAULT_BRIGHTNESS;

    /* Allocate LED buffer */
    handle->leds = (xy_ws2812_color_t *)malloc(config->num_leds * sizeof(xy_ws2812_color_t));
    if (handle->leds == NULL) {
        return WS2812_ERROR_NO_MEMORY;
    }

    /* Allocate TX buffer */
    handle->tx_buffer = (uint8_t *)malloc(buffer_size);
    if (handle->tx_buffer == NULL) {
        free(handle->leds);
        handle->leds = NULL;
        return WS2812_ERROR_NO_MEMORY;
    }

    handle->buffer_size = buffer_size;
    handle->initialized = true;
    handle->dirty = false;

    /* Clear all LEDs */
    memset(handle->leds, 0, config->num_leds * sizeof(xy_ws2812_color_t));

    /* Set default GPIO pin */
    s_gpio_pin = config->data_pin;

    return WS2812_OK;
}

void xy_ws2812_deinit(xy_ws2812_handle_t *handle)
{
    if (handle == NULL) {
        return;
    }

    if (handle->leds != NULL) {
        free(handle->leds);
        handle->leds = NULL;
    }

    if (handle->tx_buffer != NULL) {
        free(handle->tx_buffer);
        handle->tx_buffer = NULL;
    }

    handle->initialized = false;
}

void xy_ws2812_set_pixel(xy_ws2812_handle_t *handle,
                         uint16_t index,
                         xy_ws2812_color_t color)
{
    if (handle == NULL || !handle->initialized) {
        return;
    }

    if (index >= handle->config.num_leds) {
        return;
    }

    /* Apply brightness */
    color.r = (uint8_t)((color.r * handle->config.brightness) >> 8);
    color.g = (uint8_t)((color.g * handle->config.brightness) >> 8);
    color.b = (uint8_t)((color.b * handle->config.brightness) >> 8);
    color.w = (uint8_t)((color.w * handle->config.brightness) >> 8);

    handle->leds[index] = color;
    handle->dirty = true;
}

xy_ws2812_color_t xy_ws2812_get_pixel(xy_ws2812_handle_t *handle,
                                       uint16_t index)
{
    xy_ws2812_color_t black = WS2812_COLOR_BLACK;

    if (handle == NULL || !handle->initialized) {
        return black;
    }

    if (index >= handle->config.num_leds) {
        return black;
    }

    return handle->leds[index];
}

void xy_ws2812_fill(xy_ws2812_handle_t *handle, xy_ws2812_color_t color)
{
    uint16_t i;

    if (handle == NULL || !handle->initialized) {
        return;
    }

    /* Apply brightness */
    color.r = (uint8_t)((color.r * handle->config.brightness) >> 8);
    color.g = (uint8_t)((color.g * handle->config.brightness) >> 8);
    color.b = (uint8_t)((color.b * handle->config.brightness) >> 8);
    color.w = (uint8_t)((color.w * handle->config.brightness) >> 8);

    for (i = 0; i < handle->config.num_leds; i++) {
        handle->leds[i] = color;
    }

    handle->dirty = true;
}

void xy_ws2812_clear(xy_ws2812_handle_t *handle)
{
    xy_ws2812_fill(handle, WS2812_COLOR_BLACK);
}

void xy_ws2812_set_brightness(xy_ws2812_handle_t *handle, uint8_t brightness)
{
    if (handle == NULL || !handle->initialized) {
        return;
    }

    handle->config.brightness = brightness;
    handle->dirty = true;
}

uint8_t xy_ws2812_get_brightness(xy_ws2812_handle_t *handle)
{
    if (handle == NULL || !handle->initialized) {
        return 0;
    }

    return handle->config.brightness;
}

void xy_ws2812_show(xy_ws2812_handle_t *handle)
{
    uint16_t i;
    uint16_t buf_idx = 0;
    uint8_t color_bytes[4];

    if (handle == NULL || !handle->initialized) {
        return;
    }

    if (!handle->dirty) {
        return;
    }

    /* Encode all colors to TX buffer */
    for (i = 0; i < handle->config.num_leds; i++) {
        encode_color(handle->leds[i], handle->config.color_order, color_bytes);

        if (handle->config.color_order == WS2812_COLOR_RGBW) {
            memcpy(&handle->tx_buffer[buf_idx], color_bytes, 4);
            buf_idx += 4;
        } else {
            memcpy(&handle->tx_buffer[buf_idx], color_bytes, 3);
            buf_idx += 3;
        }
    }

    /* Send data via bit-banging (or DMA if configured) */
    if (handle->config.use_dma) {
        /* TODO: Implement DMA transfer */
        xy_ws2812_send_bitbang(handle);
    } else {
        xy_ws2812_send_bitbang(handle);
    }

    handle->dirty = false;
}

void xy_ws2812_set_pixels(xy_ws2812_handle_t *handle,
                          xy_ws2812_color_t *colors,
                          uint16_t count,
                          uint16_t start_index)
{
    uint16_t i;
    uint16_t end_index;

    if (handle == NULL || !handle->initialized || colors == NULL) {
        return;
    }

    end_index = start_index + count;
    if (end_index > handle->config.num_leds) {
        end_index = handle->config.num_leds;
    }

    for (i = start_index; i < end_index && (i - start_index) < count; i++) {
        xy_ws2812_set_pixel(handle, i, colors[i - start_index]);
    }
}

/* ==================== Color Conversion Utilities ==================== */

xy_ws2812_color_t xy_ws2812_hsv_to_rgb(uint8_t h, uint8_t s, uint8_t v)
{
    xy_ws2812_color_t rgb;
    uint8_t region, remainder, p, q, t;

    if (s == 0) {
        rgb.r = v;
        rgb.g = v;
        rgb.b = v;
        rgb.w = 0;
        return rgb;
    }

    region = h / 43;
    remainder = (h - (region * 43)) * 6;

    p = (v * (255 - s)) >> 8;
    q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

    switch (region) {
        case 0:
            rgb.r = v; rgb.g = t; rgb.b = p;
            break;
        case 1:
            rgb.r = q; rgb.g = v; rgb.b = p;
            break;
        case 2:
            rgb.r = p; rgb.g = v; rgb.b = t;
            break;
        case 3:
            rgb.r = p; rgb.g = q; rgb.b = v;
            break;
        case 4:
            rgb.r = t; rgb.g = p; rgb.b = v;
            break;
        default:
            rgb.r = v; rgb.g = p; rgb.b = q;
            break;
    }

    rgb.w = 0;
    return rgb;
}

void xy_ws2812_rgb_to_hsv(xy_ws2812_color_t rgb, uint8_t *h, uint8_t *s, uint8_t *v)
{
    uint8_t rgb_min, rgb_max;
    int16_t delta;

    rgb_min = (rgb.r < rgb.g) ? ((rgb.r < rgb.b) ? rgb.r : rgb.b) : ((rgb.g < rgb.b) ? rgb.g : rgb.b);
    rgb_max = (rgb.r > rgb.g) ? ((rgb.r > rgb.b) ? rgb.r : rgb.b) : ((rgb.g > rgb.b) ? rgb.g : rgb.b);

    *v = rgb_max;

    delta = rgb_max - rgb_min;
    if (delta == 0) {
        *h = 0;
        *s = 0;
        return;
    }

    *s = (uint8_t)((delta * 255) / rgb_max);

    if (rgb.r == rgb_max) {
        *h = (uint8_t)(((rgb.g - rgb.b) * 43) / delta);
    } else if (rgb.g == rgb_max) {
        *h = (uint8_t)(128 + ((rgb.b - rgb.r) * 43) / delta);
    } else {
        *h = (uint8_t)(85 + ((rgb.r - rgb.g) * 43) / delta);
    }
}

xy_ws2812_color_t xy_ws2812_rainbow_color(uint8_t hue)
{
    return xy_ws2812_hsv_to_rgb(hue, 255, 255);
}

xy_ws2812_color_t xy_ws2812_color_blend(xy_ws2812_color_t c1,
                                        xy_ws2812_color_t c2,
                                        uint8_t factor)
{
    xy_ws2812_color_t result;

    result.r = (uint8_t)(((c1.r * (255 - factor)) + (c2.r * factor)) >> 8);
    result.g = (uint8_t)(((c1.g * (255 - factor)) + (c2.g * factor)) >> 8);
    result.b = (uint8_t)(((c1.b * (255 - factor)) + (c2.b * factor)) >> 8);
    result.w = (uint8_t)(((c1.w * (255 - factor)) + (c2.w * factor)) >> 8);

    return result;
}

/* ==================== GPIO Interface ==================== */

void xy_ws2812_set_gpio_callback(xy_ws2812_gpio_write_t callback)
{
    s_gpio_write = callback;
}

void xy_ws2812_send_bitbang(xy_ws2812_handle_t *handle)
{
    uint16_t i;
    uint8_t bit_count = 0;

    if (handle == NULL || handle->tx_buffer == NULL) {
        return;
    }

    /* Send all bytes in TX buffer */
    for (i = 0; i < handle->buffer_size; i++) {
        send_byte(handle->tx_buffer[i]);
    }

    /* Send reset pulse (50us low) */
    gpio_set_low();
    xy_ws2812_delay_ns(WS2812_RESET_NS);
    (void)bit_count;  /* Avoid unused warning */
}
