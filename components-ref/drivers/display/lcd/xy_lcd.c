/**
 * @file xy_lcd.c
 * @brief LCD Device Driver Framework - Basic LCD Operations
 * @version 1.0.0
 * @date 2026-03-05
 */

#include "xy_lcd.h"
#include <string.h>
#include <stdlib.h>

/* ==================== Internal Functions ==================== */

/**
 * @brief Get coordinate after applying rotation
 */
static void xy_lcd_transform_coords(xy_lcd_device_t *lcd, uint16_t *x, uint16_t *y)
{
    uint16_t temp;
    switch (lcd->rotation) {
        case XY_LCD_ROTATION_90:
            temp = *x;
            *x = *y;
            *y = lcd->height - 1 - temp;
            break;
        case XY_LCD_ROTATION_180:
            *x = lcd->width - 1 - *x;
            *y = lcd->height - 1 - *y;
            break;
        case XY_LCD_ROTATION_270:
            temp = *y;
            *y = *x;
            *x = lcd->width - 1 - temp;
            break;
        default:
            break;
    }
}

/**
 * @brief Get actual width considering rotation
 */
static uint16_t xy_lcd_get_actual_width(xy_lcd_device_t *lcd)
{
    if (lcd->rotation == XY_LCD_ROTATION_90 || lcd->rotation == XY_LCD_ROTATION_270) {
        return lcd->height;
    }
    return lcd->width;
}

/**
 * @brief Get actual height considering rotation
 */
static uint16_t xy_lcd_get_actual_height(xy_lcd_device_t *lcd)
{
    if (lcd->rotation == XY_LCD_ROTATION_90 || lcd->rotation == XY_LCD_ROTATION_270) {
        return lcd->width;
    }
    return lcd->height;
}

/* ==================== LCD Core Operations ==================== */

xy_error_t xy_lcd_init(xy_lcd_device_t *lcd, const xy_lcd_config_t *config)
{
    if (!lcd || !config) {
        return XY_ERR_INVALID_PARAM;
    }

    /* Initialize base device */
    memset(lcd, 0, sizeof(*lcd));
    lcd->width = config->width;
    lcd->height = config->height;
    lcd->color_fmt = config->color_fmt;
    lcd->interface = config->interface;
    lcd->rotation = config->rotation;

    /* Allocate framebuffer */
    uint32_t fb_size = config->width * config->height * sizeof(uint16_t);
    lcd->framebuffer = (uint16_t *)calloc(1, fb_size);
    if (!lcd->framebuffer) {
        return XY_ERR_NO_MEM;
    }
    lcd->fb_width = config->width;
    lcd->fb_height = config->height;

    return XY_ERR_OK;
}

xy_error_t xy_lcd_deinit(xy_lcd_device_t *lcd)
{
    if (!lcd) {
        return XY_ERR_INVALID_PARAM;
    }

    /* Free framebuffer */
    if (lcd->framebuffer) {
        free(lcd->framebuffer);
        lcd->framebuffer = NULL;
    }

    return XY_ERR_OK;
}

void xy_lcd_clear(xy_lcd_device_t *lcd, uint16_t color)
{
    if (!lcd || !lcd->framebuffer) {
        return;
    }

    uint32_t size = lcd->fb_width * lcd->fb_height;
    for (uint32_t i = 0; i < size; i++) {
        lcd->framebuffer[i] = color;
    }
}

void xy_lcd_draw_pixel(xy_lcd_device_t *lcd, uint16_t x, uint16_t y, uint16_t color)
{
    if (!lcd || !lcd->framebuffer) {
        return;
    }

    /* Apply rotation transform */
    xy_lcd_transform_coords(lcd, &x, &y);

    /* Check bounds */
    if (x >= lcd->fb_width || y >= lcd->fb_height) {
        return;
    }

    lcd->framebuffer[y * lcd->fb_width + x] = color;
}

void xy_lcd_draw_hline(xy_lcd_device_t *lcd, uint16_t x, uint16_t y,
                       uint16_t len, uint16_t color)
{
    if (!lcd || !lcd->framebuffer) {
        return;
    }

    for (uint16_t i = 0; i < len; i++) {
        xy_lcd_draw_pixel(lcd, x + i, y, color);
    }
}

void xy_lcd_draw_vline(xy_lcd_device_t *lcd, uint16_t x, uint16_t y,
                       uint16_t len, uint16_t color)
{
    if (!lcd || !lcd->framebuffer) {
        return;
    }

    for (uint16_t i = 0; i < len; i++) {
        xy_lcd_draw_pixel(lcd, x, y + i, color);
    }
}

void xy_lcd_draw_rect(xy_lcd_device_t *lcd, uint16_t x, uint16_t y,
                     uint16_t w, uint16_t h, uint16_t color, bool filled)
{
    if (!lcd) {
        return;
    }

    if (filled) {
        xy_lcd_fill(lcd, x, y, w, h, color);
    } else {
        /* Top and bottom */
        xy_lcd_draw_hline(lcd, x, y, w, color);
        xy_lcd_draw_hline(lcd, x, y + h - 1, w, color);
        /* Left and right */
        xy_lcd_draw_vline(lcd, x, y, h, color);
        xy_lcd_draw_vline(lcd, x + w - 1, y, h, color);
    }
}

void xy_lcd_fill(xy_lcd_device_t *lcd, uint16_t x, uint16_t y,
                uint16_t w, uint16_t h, uint16_t color)
{
    if (!lcd || !lcd->framebuffer) {
        return;
    }

    /* Apply rotation transform to starting point */
    xy_lcd_transform_coords(lcd, &x, &y);

    /* Calculate actual dimensions after rotation */
    uint16_t actual_w = w;
    uint16_t actual_h = h;

    if (lcd->rotation == XY_LCD_ROTATION_90 || lcd->rotation == XY_LCD_ROTATION_270) {
        /* Swap dimensions */
        uint16_t temp = actual_w;
        actual_w = actual_h;
        actual_h = temp;
    }

    /* Fill area */
    for (uint16_t row = 0; row < h; row++) {
        for (uint16_t col = 0; col < w; col++) {
            uint16_t px = x + col;
            uint16_t py = y + row;
            xy_lcd_transform_coords(lcd, &px, &py);

            if (px < lcd->fb_width && py < lcd->fb_height) {
                lcd->framebuffer[py * lcd->fb_width + px] = color;
            }
        }
    }
}

void xy_lcd_refresh(xy_lcd_device_t *lcd)
{
    if (!lcd || !lcd->framebuffer) {
        return;
    }

    /* This function should be implemented by specific interface driver */
    /* e.g., SPI refresh, I8080 refresh, RGB refresh */
}

void xy_lcd_set_backlight(xy_lcd_device_t *lcd, uint8_t brightness)
{
    if (!lcd) {
        return;
    }

    /* This function should be implemented by specific interface driver */
    (void)brightness;
}

void xy_lcd_power(xy_lcd_device_t *lcd, bool on)
{
    if (!lcd) {
        return;
    }

    /* This function should be implemented by specific interface driver */
    (void)on;
}

void xy_lcd_set_rotation(xy_lcd_device_t *lcd, xy_lcd_rotation_t rotation)
{
    if (!lcd) {
        return;
    }

    lcd->rotation = rotation;
}

uint16_t xy_lcd_get_width(xy_lcd_device_t *lcd)
{
    if (!lcd) {
        return 0;
    }

    return xy_lcd_get_actual_width(lcd);
}

uint16_t xy_lcd_get_height(xy_lcd_device_t *lcd)
{
    if (!lcd) {
        return 0;
    }

    return xy_lcd_get_actual_height(lcd);
}
