/**
 * @file xy_rgb_matrix.c
 * @brief RGB LED Matrix Driver Implementation
 * @version 1.0.0
 * @date 2026-03-02
 */

#include "xy_rgb_matrix.h"
#include <string.h>

/* ==================== Internal State ==================== */

/**
 * @brief Effect state structure
 */
typedef struct {
    xy_rgb_matrix_effect_t current_effect;
    uint8_t speed;
    uint8_t progress;
    uint32_t last_update;
    xy_ws2812_color_t effect_color;
} xy_rgb_matrix_effect_state_t;

/* ==================== Public API Implementation ==================== */

xy_rgb_matrix_error_t xy_rgb_matrix_init(xy_rgb_matrix_handle_t *handle,
                                         xy_rgb_matrix_config_t *config,
                                         xy_ws2812_config_t *ws2812_config)
{
    if (handle == NULL || config == NULL || ws2812_config == NULL) {
        return XY_RGB_MATRIX_ERROR_INVALID_PARAM;
    }

    if (config->width == 0 || config->height == 0) {
        return XY_RGB_MATRIX_ERROR_INVALID_PARAM;
    }

    /* Initialize WS2812 with calculated LED count */
    ws2812_config->num_leds = config->width * config->height;
    ws2812_config->color_order = config->color_order;
    ws2812_config->brightness = config->brightness > 0 ? config->brightness : RGB_MATRIX_DEFAULT_BRIGHT;

    xy_ws2812_error_t ws_err = xy_ws2812_init(&handle->ws2812, ws2812_config);
    if (ws_err != WS2812_OK) {
        return XY_RGB_MATRIX_ERROR;
    }

    /* Store configuration */
    memset(&handle->config, 0, sizeof(xy_rgb_matrix_config_t));
    handle->config.width = config->width;
    handle->config.height = config->height;
    handle->config.layout = config->layout;
    handle->config.color_order = config->color_order;
    handle->config.brightness = config->brightness;

    handle->initialized = true;

    /* Clear matrix on init */
    xy_rgb_matrix_clear(handle);

    return XY_RGB_MATRIX_OK;
}

void xy_rgb_matrix_deinit(xy_rgb_matrix_handle_t *handle)
{
    if (handle == NULL) {
        return;
    }

    xy_ws2812_deinit(&handle->ws2812);
    handle->initialized = false;
}

uint16_t xy_rgb_matrix_xy_to_index(xy_rgb_matrix_handle_t *handle,
                                    uint16_t x, uint16_t y)
{
    if (handle == NULL || !handle->initialized) {
        return 0;
    }

    if (x >= handle->config.width || y >= handle->config.height) {
        return 0;
    }

    uint16_t index;
    uint16_t row = y;

    switch (handle->config.layout) {
        case XY_RGB_MATRIX_LAYOUT_ZIGZAG:
            /* Zigzag: alternating rows reverse direction */
            if (row % 2 == 0) {
                index = row * handle->config.width + x;
            } else {
                index = row * handle->config.width + (handle->config.width - 1 - x);
            }
            break;

        case XY_RGB_MATRIX_LAYOUT_LINEAR:
            /* Linear: left to right, top to bottom */
            index = row * handle->config.width + x;
            break;

        case XY_RGB_MATRIX_LAYOUT_SNAKE:
        default:
            /* Snake: same as zigzag */
            if (row % 2 == 0) {
                index = row * handle->config.width + x;
            } else {
                index = row * handle->config.width + (handle->config.width - 1 - x);
            }
            break;
    }

    return index;
}

void xy_rgb_matrix_index_to_xy(xy_rgb_matrix_handle_t *handle,
                                uint16_t index,
                                uint16_t *x, uint16_t *y)
{
    if (handle == NULL || !handle->initialized || x == NULL || y == NULL) {
        return;
    }

    uint16_t total = handle->config.width * handle->config.height;
    if (index >= total) {
        *x = 0;
        *y = 0;
        return;
    }

    *y = index / handle->config.width;
    uint16_t col = index % handle->config.width;

    switch (handle->config.layout) {
        case XY_RGB_MATRIX_LAYOUT_ZIGZAG:
        case XY_RGB_MATRIX_LAYOUT_SNAKE:
            if (*y % 2 == 1) {
                *x = handle->config.width - 1 - col;
            } else {
                *x = col;
            }
            break;

        case XY_RGB_MATRIX_LAYOUT_LINEAR:
        default:
            *x = col;
            break;
    }
}

void xy_rgb_matrix_set_pixel(xy_rgb_matrix_handle_t *handle,
                             uint16_t x, uint16_t y,
                             xy_ws2812_color_t color)
{
    if (handle == NULL || !handle->initialized) {
        return;
    }

    if (x >= handle->config.width || y >= handle->config.height) {
        return;
    }

    uint16_t index = xy_rgb_matrix_xy_to_index(handle, x, y);
    xy_ws2812_set_pixel(&handle->ws2812, index, color);
}

xy_ws2812_color_t xy_rgb_matrix_get_pixel(xy_rgb_matrix_handle_t *handle,
                                           uint16_t x, uint16_t y)
{
    xy_ws2812_color_t black = WS2812_COLOR_BLACK;

    if (handle == NULL || !handle->initialized) {
        return black;
    }

    if (x >= handle->config.width || y >= handle->config.height) {
        return black;
    }

    uint16_t index = xy_rgb_matrix_xy_to_index(handle, x, y);
    return xy_ws2812_get_pixel(&handle->ws2812, index);
}

void xy_rgb_matrix_fill(xy_rgb_matrix_handle_t *handle, xy_ws2812_color_t color)
{
    if (handle == NULL || !handle->initialized) {
        return;
    }

    xy_ws2812_fill(&handle->ws2812, color);
}

void xy_rgb_matrix_clear(xy_rgb_matrix_handle_t *handle)
{
    if (handle == NULL || !handle->initialized) {
        return;
    }

    xy_ws2812_clear(&handle->ws2812);
}

void xy_rgb_matrix_set_brightness(xy_rgb_matrix_handle_t *handle,
                                   uint8_t brightness)
{
    if (handle == NULL || !handle->initialized) {
        return;
    }

    xy_ws2812_set_brightness(&handle->ws2812, brightness);
    handle->config.brightness = brightness;
}

uint8_t xy_rgb_matrix_get_brightness(xy_rgb_matrix_handle_t *handle)
{
    if (handle == NULL || !handle->initialized) {
        return 0;
    }

    return xy_ws2812_get_brightness(&handle->ws2812);
}

void xy_rgb_matrix_show(xy_rgb_matrix_handle_t *handle)
{
    if (handle == NULL || !handle->initialized) {
        return;
    }

    xy_ws2812_show(&handle->ws2812);
}

void xy_rgb_matrix_draw_hline(xy_rgb_matrix_handle_t *handle,
                               uint16_t y,
                               xy_ws2812_color_t color)
{
    uint16_t x;

    if (handle == NULL || !handle->initialized) {
        return;
    }

    if (y >= handle->config.height) {
        return;
    }

    for (x = 0; x < handle->config.width; x++) {
        xy_rgb_matrix_set_pixel(handle, x, y, color);
    }
}

void xy_rgb_matrix_draw_vline(xy_rgb_matrix_handle_t *handle,
                               uint16_t x,
                               xy_ws2812_color_t color)
{
    uint16_t y;

    if (handle == NULL || !handle->initialized) {
        return;
    }

    if (x >= handle->config.width) {
        return;
    }

    for (y = 0; y < handle->config.height; y++) {
        xy_rgb_matrix_set_pixel(handle, x, y, color);
    }
}

void xy_rgb_matrix_draw_rect(xy_rgb_matrix_handle_t *handle,
                              uint16_t x1, uint16_t y1,
                              uint16_t x2, uint16_t y2,
                              xy_ws2812_color_t color,
                              bool fill)
{
    uint16_t x, y;

    if (handle == NULL || !handle->initialized) {
        return;
    }

    /* Ensure coordinates are valid */
    if (x1 > x2) {
        uint16_t tmp = x1; x1 = x2; x2 = tmp;
    }
    if (y1 > y2) {
        uint16_t tmp = y1; y1 = y2; y2 = tmp;
    }

    /* Clamp to matrix bounds */
    if (x1 >= handle->config.width || y1 >= handle->config.height) {
        return;
    }
    if (x2 >= handle->config.width) {
        x2 = handle->config.width - 1;
    }
    if (y2 >= handle->config.height) {
        y2 = handle->config.height - 1;
    }

    if (fill) {
        for (y = y1; y <= y2; y++) {
            for (x = x1; x <= x2; x++) {
                xy_rgb_matrix_set_pixel(handle, x, y, color);
            }
        }
    } else {
        /* Draw top and bottom edges */
        for (x = x1; x <= x2; x++) {
            xy_rgb_matrix_set_pixel(handle, x, y1, color);
            xy_rgb_matrix_set_pixel(handle, x, y2, color);
        }
        /* Draw left and right edges */
        for (y = y1; y <= y2; y++) {
            xy_rgb_matrix_set_pixel(handle, x1, y, color);
            xy_rgb_matrix_set_pixel(handle, x2, y, color);
        }
    }
}

/* ==================== Effects Implementation ==================== */

/* Internal state for effects */
static xy_rgb_matrix_effect_state_t s_effect_state = {
    .current_effect = XY_RGB_MATRIX_EFFECT_SOLID,
    .speed = 128,
    .progress = 0,
    .last_update = 0,
    .effect_color = WS2812_COLOR_WHITE
};

void xy_rgb_matrix_set_effect(xy_rgb_matrix_handle_t *handle,
                               xy_rgb_matrix_effect_t effect,
                               uint8_t speed,
                               xy_ws2812_color_t color)
{
    (void)handle;
    s_effect_state.current_effect = effect;
    s_effect_state.speed = speed;
    s_effect_state.effect_color = color;
    s_effect_state.progress = 0;
    s_effect_state.last_update = 0;
}

bool xy_rgb_matrix_update_effect(xy_rgb_matrix_handle_t *handle)
{
    if (handle == NULL || !handle->initialized) {
        return false;
    }

    uint32_t now = 0;  /* TODO: Use actual time from system */
    uint32_t elapsed = now - s_effect_state.last_update;

    /* Calculate step based on speed (lower speed = faster) */
    uint32_t interval = (256 - s_effect_state.speed) * 10;  /* ms between updates */

    if (elapsed < interval) {
        return false;
    }

    s_effect_state.last_update = now;
    s_effect_state.progress += 4;

    switch (s_effect_state.current_effect) {
        case XY_RGB_MATRIX_EFFECT_RAINBOW:
            xy_rgb_matrix_effect_rainbow(handle, s_effect_state.progress);
            break;

        case XY_RGB_MATRIX_EFFECT_BREATHING:
            xy_rgb_matrix_effect_breathing(handle, s_effect_state.progress);
            break;

        case XY_RGB_MATRIX_EFFECT_SOLID:
        default:
            xy_rgb_matrix_effect_solid(handle, s_effect_state.effect_color);
            break;
    }

    return true;
}

void xy_rgb_matrix_effect_breathing(xy_rgb_matrix_handle_t *handle,
                                     uint8_t progress)
{
    if (handle == NULL || !handle->initialized) {
        return;
    }

    /* Calculate brightness: sine wave breathing */
    uint8_t breath_brightness = (uint8_t)((((progress > 128) ? (256 - progress) : progress) * 2));

    /* Scale base color by breathing brightness */
    xy_ws2812_color_t color;
    color.r = (uint8_t)((s_effect_state.effect_color.r * breath_brightness) >> 8);
    color.g = (uint8_t)((s_effect_state.effect_color.g * breath_brightness) >> 8);
    color.b = (uint8_t)((s_effect_state.effect_color.b * breath_brightness) >> 8);
    color.w = (uint8_t)((s_effect_state.effect_color.w * breath_brightness) >> 8);

    xy_rgb_matrix_fill(handle, color);
}

void xy_rgb_matrix_effect_rainbow(xy_rgb_matrix_handle_t *handle, uint8_t hue)
{
    uint16_t x, y;
    uint8_t row_hue;

    if (handle == NULL || !handle->initialized) {
        return;
    }

    for (y = 0; y < handle->config.height; y++) {
        row_hue = hue + (uint8_t)((y * 256) / handle->config.height);
        for (x = 0; x < handle->config.width; x++) {
            uint8_t pixel_hue = row_hue + (uint8_t)((x * 256) / handle->config.width);
            xy_ws2812_color_t color = xy_ws2812_rainbow_color(pixel_hue);
            xy_rgb_matrix_set_pixel(handle, x, y, color);
        }
    }
}

void xy_rgb_matrix_effect_solid(xy_rgb_matrix_handle_t *handle,
                                 xy_ws2812_color_t color)
{
    if (handle == NULL || !handle->initialized) {
        return;
    }

    xy_rgb_matrix_fill(handle, color);
}
