/**
 * @file xy_oled_ssd1306.c
 * @brief SSD1306 OLED Display Device Driver Implementation
 * @version 1.0.0
 * @date 2026-02-28
 */

#include "xy_oled_ssd1306.h"
#include <string.h>
#include <stdlib.h>

/* SSD1306 Commands */
#define SSD1306_CMD_DISPLAY_OFF         0xAE
#define SSD1306_CMD_DISPLAY_ON          0xAF
#define SSD1306_CMD_SET_CONTRAST        0x81
#define SSD1306_CMD_SET_MUX_RATIO       0xA8
#define SSD1306_CMD_SET_OFFSET          0xD3
#define SSD1306_CMD_SET_START_LINE      0x40
#define SSD1306_CMD_SET_SEGMENT_REMAP   0xA1
#define SSD1306_CMD_SET_COM_SCAN_DEC    0xC8
#define SSD1306_CMD_SET_COM_PINS        0xDA
#define SSD1306_CMD_SET_PRECHARGE       0xD9
#define SSD1306_CMD_SET_VCOMH           0xDB
#define SSD1306_CMD_SET_CHARGE_PUMP     0x8D
#define SSD1306_CMD_MEMORY_MODE         0x20
#define SSD1306_CMD_COLUMN_ADDR         0x21
#define SSD1306_CMD_PAGE_ADDR           0x22

static const uint8_t init_sequence[] = {
    SSD1306_CMD_DISPLAY_OFF,
    SSD1306_CMD_SET_CHARGE_PUMP, 0x14,
    SSD1306_CMD_SET_SEGMENT_REMAP,
    SSD1306_CMD_SET_COM_SCAN_DEC,
    SSD1306_CMD_SET_COM_PINS, 0x12,
    SSD1306_CMD_SET_CONTRAST, 0xCF,
    SSD1306_CMD_SET_PRECHARGE, 0xF1,
    SSD1306_CMD_SET_VCOMH, 0x40,
    SSD1306_CMD_DISPLAY_ON,
};

int xy_oled_ssd1306_init(xy_oled_ssd1306_t *oled, void *i2c_handle, 
                         uint16_t width, uint16_t height)
{
    if (!oled || !i2c_handle) {
        return XY_DEVICE_INVALID_PARAM;
    }

    memset(oled, 0, sizeof(*oled));
    oled->width = width;
    oled->height = height;
    
    xy_i2c_device_init(&oled->i2c_dev, i2c_handle, 0x3C, 1000);

    /* Allocate buffer */
    uint16_t buffer_size = (width * height) / 8;
    oled->buffer = (uint8_t *)calloc(1, buffer_size);
    if (!oled->buffer) {
        return XY_DEVICE_NO_MEM;
    }

    /* Send init commands */
    for (size_t i = 0; i < sizeof(init_sequence); i++) {
        uint8_t cmd[2] = {0x00, init_sequence[i]};
        xy_i2c_device_write(&oled->i2c_dev, cmd, 2);
        xy_hal_delay_ms(1);
    }

    return XY_DEVICE_OK;
}

void xy_oled_ssd1306_clear(xy_oled_ssd1306_t *oled)
{
    if (!oled || !oled->buffer) {
        return;
    }

    memset(oled->buffer, 0, (oled->width * oled->height) / 8);
}

void xy_oled_ssd1306_refresh(xy_oled_ssd1306_t *oled)
{
    if (!oled || !oled->buffer) {
        return;
    }

    /* Set column address */
    uint8_t cmd[] = {
        0x00, SSD1306_CMD_COLUMN_ADDR,
        0x00, 0,
        0x00, oled->width - 1
    };
    xy_i2c_device_write(&oled->i2c_dev, cmd, sizeof(cmd));

    /* Set page address */
    uint8_t page_cmd[] = {
        0x00, SSD1306_CMD_PAGE_ADDR,
        0x00, 0,
        0x00, (oled->height / 8) - 1
    };
    xy_i2c_device_write(&oled->i2c_dev, page_cmd, sizeof(page_cmd));

    /* Send data */
    uint8_t header = 0x40;
    xy_i2c_device_write(&oled->i2c_dev, &header, 1);
    xy_i2c_device_write(&oled->i2c_dev, oled->buffer, (oled->width * oled->height) / 8);
}

void xy_oled_ssd1306_draw_pixel(xy_oled_ssd1306_t *oled, 
                                int16_t x, int16_t y, bool color)
{
    if (!oled || !oled->buffer) {
        return;
    }

    if (x < 0 || x >= oled->width || y < 0 || y >= oled->height) {
        return;
    }

    uint16_t index = (y / 8) * oled->width + x;
    uint8_t bit = y % 8;

    if (color) {
        oled->buffer[index] |= (1 << bit);
    } else {
        oled->buffer[index] &= ~(1 << bit);
    }
}

void xy_oled_ssd1306_draw_line(xy_oled_ssd1306_t *oled, 
                               int16_t x0, int16_t y0, 
                               int16_t x1, int16_t y1, bool color)
{
    int16_t dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int16_t dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = dx - dy;

    while(1) {
        xy_oled_ssd1306_draw_pixel(oled, x0, y0, color);

        if (x0 == x1 && y0 == y1) {
            break;
        }

        int16_t e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

/* Simple 5x7 font */
static const uint8_t font5x7[95][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, /* space */
    {0x00, 0x00, 0x4F, 0x00, 0x00}, /* ! */
    /* ... more characters ... */
};

void xy_oled_ssd1306_draw_char(xy_oled_ssd1306_t *oled, 
                               int16_t x, int16_t y, 
                               char c, bool color)
{
    if (!oled || !oled->buffer) {
        return;
    }

    if (c < 32 || c > 126) {
        c = 32;
    }

    const uint8_t *char_data = font5x7[c - 32];
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 7; j++) {
            if (char_data[i] & (1 << j)) {
                xy_oled_ssd1306_draw_pixel(oled, x + i, y + j, color);
            }
        }
    }
}

void xy_oled_ssd1306_draw_string(xy_oled_ssd1306_t *oled, 
                                 int16_t x, int16_t y, 
                                 const char *str, bool color)
{
    if (!oled || !str) {
        return;
    }

    while (*str) {
        xy_oled_ssd1306_draw_char(oled, x, y, *str++, color);
        x += 6;
    }
}
