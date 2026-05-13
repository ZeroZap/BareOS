/**
 * @file xy_oled_ssd1306.c
 * @brief SSD1306 OLED Display Driver Implementation (128x64)
 * @version 1.0.0
 * @date 2026-03-01 凌晨
 */

#include "xy_oled_ssd1306.h"
#include "xy_log.h"
#include <string.h>
#include <stdlib.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/**
 * @brief 5x7 字体 (ASCII 32-126)
 */
static const uint8_t g_font5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, /* SPACE */
    {0x00, 0x00, 0x5F, 0x00, 0x00}, /* ! */
    {0x00, 0x07, 0x00, 0x07, 0x00}, /* " */
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, /* # */
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, /* $ */
    {0x23, 0x13, 0x08, 0x64, 0x62}, /* % */
    {0x36, 0x49, 0x55, 0x22, 0x50}, /* & */
    {0x00, 0x05, 0x03, 0x00, 0x00}, /* ' */
    {0x00, 0x1C, 0x22, 0x41, 0x00}, /* ( */
    {0x00, 0x41, 0x22, 0x1C, 0x00}, /* ) */
    {0x14, 0x08, 0x3E, 0x08, 0x14}, /* * */
    {0x08, 0x08, 0x3E, 0x08, 0x08}, /* + */
    {0x00, 0x50, 0x30, 0x00, 0x00}, /* , */
    {0x08, 0x08, 0x08, 0x08, 0x08}, /* - */
    {0x00, 0x60, 0x60, 0x00, 0x00}, /* . */
    {0x20, 0x10, 0x08, 0x04, 0x02}, /* / */
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, /* 0 */
    {0x00, 0x42, 0x7F, 0x40, 0x00}, /* 1 */
    {0x42, 0x61, 0x51, 0x49, 0x46}, /* 2 */
    {0x21, 0x41, 0x45, 0x4B, 0x31}, /* 3 */
    {0x18, 0x14, 0x12, 0x7F, 0x10}, /* 4 */
    {0x27, 0x45, 0x45, 0x45, 0x39}, /* 5 */
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, /* 6 */
    {0x01, 0x71, 0x09, 0x05, 0x03}, /* 7 */
    {0x36, 0x49, 0x49, 0x49, 0x36}, /* 8 */
    {0x06, 0x49, 0x49, 0x29, 0x1E}, /* 9 */
    /* ... 省略其他字符 ... */
};

/**
 * @brief 发送命令
 */
static int xy_oled_send_cmd(xy_oled_ssd1306_t *dev, uint8_t cmd)
{
    uint8_t buf[2] = {0x00, cmd};  /* 0x00 = 命令 */
    return xy_i2c_device_write(&dev->i2c_dev, buf, 2);
}

/**
 * @brief 发送数据
 */
static int xy_oled_send_data(xy_oled_ssd1306_t *dev, const uint8_t *data, uint16_t len)
{
    uint8_t *buf = malloc(len + 1);
    if (!buf) {
        return XY_OLED_ERROR;
    }
    
    buf[0] = 0x40;  /* 0x40 = 数据 */
    memcpy(&buf[1], data, len);
    
    int ret = xy_i2c_device_write(&dev->i2c_dev, buf, len + 1);
    free(buf);
    return ret;
}

int xy_oled_ssd1306_init(xy_oled_ssd1306_t *dev, void *i2c_handle)
{
    int ret;
    
    if (!dev || !i2c_handle) {
        return XY_OLED_INVALID_PARAM;
    }
    
    memset(dev, 0, sizeof(*dev));
    
    /* 初始化 I2C */
    xy_i2c_device_init(&dev->i2c_dev, i2c_handle, SSD1306_ADDR, 400);
    dev->width = 128;
    dev->height = 64;
    dev->buffer_size = (dev->width * dev->height) / 8;
    
    /* 分配缓冲区 */
    dev->buffer = malloc(dev->buffer_size);
    if (!dev->buffer) {
        return XY_OLED_ERROR;
    }
    
    xy_os_delay(100);
    
    /* 初始化序列 */
    const uint8_t init_cmds[] = {
        SSD1306_CMD_DISPLAY_OFF,
        SSD1306_CMD_SET_CLOCK_DIV, 0x80,
        SSD1306_CMD_SET_MUX_RATIO, 0x3F,
        SSD1306_CMD_SET_DISPLAY_OFFSET, 0x00,
        SSD1306_CMD_SET_START_LINE | 0x00,
        SSD1306_CMD_SET_CHARGE_PUMP, 0x14,  /* 开启电荷泵 */
        SSD1306_CMD_MEMORY_MODE, 0x00,      /* 水平寻址 */
        SSD1306_CMD_SET_SEGMENT_REMAP,      /* 段重映射 */
        SSD1306_CMD_SET_COM_SCAN_DEC,       /* COM 扫描方向 */
        SSD1306_CMD_SET_COM_PINS, 0x12,
        SSD1306_CMD_SET_CONTRAST, 0xCF,
        SSD1306_CMD_SET_PRECHARGE, 0xF1,
        SSD1306_CMD_SET_VCOMH, 0x40,
        SSD1306_CMD_NORMAL_DISPLAY,
        SSD1306_CMD_DISPLAY_ON,
    };
    
    for (size_t i = 0; i < sizeof(init_cmds); i++) {
        ret = xy_oled_send_cmd(dev, init_cmds[i]);
        if (ret != XY_DEVICE_OK) {
            xy_log_e("Failed to send init command %d\n", i);
            free(dev->buffer);
            return XY_OLED_NOT_FOUND;
        }
        xy_os_delay(1);
    }
    
    xy_oled_ssd1306_clear(dev);
    xy_oled_ssd1306_refresh(dev);
    
    dev->initialized = 1;
    xy_log_i("SSD1306 OLED initialized (128x64)\n");
    return XY_OLED_OK;
}

int xy_oled_ssd1306_deinit(xy_oled_ssd1306_t *dev)
{
    if (!dev) {
        return XY_OLED_INVALID_PARAM;
    }
    
    xy_oled_send_cmd(dev, SSD1306_CMD_DISPLAY_OFF);
    
    if (dev->buffer) {
        free(dev->buffer);
        dev->buffer = NULL;
    }
    
    dev->initialized = 0;
    return XY_OLED_OK;
}

int xy_oled_ssd1306_clear(xy_oled_ssd1306_t *dev)
{
    if (!dev || !dev->buffer) {
        return XY_OLED_INVALID_PARAM;
    }
    
    memset(dev->buffer, 0x00, dev->buffer_size);
    return XY_OLED_OK;
}

int xy_oled_ssd1306_refresh(xy_oled_ssd1306_t *dev)
{
    int ret;
    
    if (!dev || !dev->initialized) {
        return XY_OLED_INVALID_PARAM;
    }
    
    /* 设置列地址 */
    ret = xy_oled_send_cmd(dev, SSD1306_CMD_COLUMN_ADDR);
    ret |= xy_oled_send_cmd(dev, 0);
    ret |= xy_oled_send_cmd(dev, 127);
    
    /* 设置页地址 */
    ret |= xy_oled_send_cmd(dev, SSD1306_CMD_PAGE_ADDR);
    ret |= xy_oled_send_cmd(dev, 0);
    ret |= xy_oled_send_cmd(dev, 7);
    
    /* 发送数据 */
    return xy_oled_send_data(dev, dev->buffer, dev->buffer_size);
}

int xy_oled_ssd1306_draw_pixel(xy_oled_ssd1306_t *dev, int16_t x, int16_t y, uint8_t color)
{
    if (!dev || !dev->buffer) {
        return XY_OLED_INVALID_PARAM;
    }
    
    if (x < 0 || x >= dev->width || y < 0 || y >= dev->height) {
        return XY_OLED_INVALID_PARAM;
    }
    
    uint16_t index = x + (y / 8) * dev->width;
    
    if (color) {
        dev->buffer[index] |= (1 << (y % 8));
    } else {
        dev->buffer[index] &= ~(1 << (y % 8));
    }
    
    return XY_OLED_OK;
}

int xy_oled_ssd1306_draw_char(xy_oled_ssd1306_t *dev, int16_t x, int16_t y, char ch, uint8_t color)
{
    if (!dev || !dev->buffer) {
        return XY_OLED_INVALID_PARAM;
    }
    
    if (ch < 32 || ch > 126) {
        ch = 32;  /* 显示空格 */
    }
    
    const uint8_t *glyph = g_font5x7[ch - 32];
    
    for (uint8_t col = 0; col < 5; col++) {
        for (uint8_t row = 0; row < 7; row++) {
            if (glyph[col] & (1 << row)) {
                xy_oled_ssd1306_draw_pixel(dev, x + col, y + row, color);
            }
        }
    }
    
    return XY_OLED_OK;
}

int xy_oled_ssd1306_draw_string(xy_oled_ssd1306_t *dev, int16_t x, int16_t y, const char *str, uint8_t color)
{
    if (!dev || !str) {
        return XY_OLED_INVALID_PARAM;
    }
    
    int16_t current_x = x;
    
    while (*str) {
        xy_oled_ssd1306_draw_char(dev, current_x, y, *str, color);
        current_x += 6;  /* 字符宽度 5 + 间距 1 */
        str++;
    }
    
    return XY_OLED_OK;
}

int xy_oled_ssd1306_draw_hline(xy_oled_ssd1306_t *dev, int16_t x1, int16_t y, int16_t x2, uint8_t color)
{
    if (x1 > x2) {
        int16_t tmp = x1;
        x1 = x2;
        x2 = tmp;
    }
    
    for (int16_t x = x1; x <= x2; x++) {
        xy_oled_ssd1306_draw_pixel(dev, x, y, color);
    }
    
    return XY_OLED_OK;
}

int xy_oled_ssd1306_draw_vline(xy_oled_ssd1306_t *dev, int16_t x, int16_t y1, int16_t y2, uint8_t color)
{
    if (y1 > y2) {
        int16_t tmp = y1;
        y1 = y2;
        y2 = tmp;
    }
    
    for (int16_t y = y1; y <= y2; y++) {
        xy_oled_ssd1306_draw_pixel(dev, x, y, color);
    }
    
    return XY_OLED_OK;
}

int xy_oled_ssd1306_draw_rect(xy_oled_ssd1306_t *dev, int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color)
{
    xy_oled_ssd1306_draw_hline(dev, x, y, x + w - 1, color);
    xy_oled_ssd1306_draw_hline(dev, x, y + h - 1, x + w - 1, color);
    xy_oled_ssd1306_draw_vline(dev, x, y, y + h - 1, color);
    xy_oled_ssd1306_draw_vline(dev, x + w - 1, y, y + h - 1, color);
    
    return XY_OLED_OK;
}

int xy_oled_ssd1306_fill_rect(xy_oled_ssd1306_t *dev, int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color)
{
    for (int16_t j = y; j < y + h; j++) {
        xy_oled_ssd1306_draw_hline(dev, x, j, x + w - 1, color);
    }
    
    return XY_OLED_OK;
}

int xy_oled_ssd1306_set_contrast(xy_oled_ssd1306_t *dev, uint8_t contrast)
{
    if (!dev) {
        return XY_OLED_INVALID_PARAM;
    }
    
    xy_oled_send_cmd(dev, SSD1306_CMD_SET_CONTRAST);
    return xy_oled_send_cmd(dev, contrast);
}

int xy_oled_ssd1306_display_on(xy_oled_ssd1306_t *dev)
{
    if (!dev) {
        return XY_OLED_INVALID_PARAM;
    }
    
    return xy_oled_send_cmd(dev, SSD1306_CMD_DISPLAY_ON);
}

int xy_oled_ssd1306_display_off(xy_oled_ssd1306_t *dev)
{
    if (!dev) {
        return XY_OLED_INVALID_PARAM;
    }
    
    return xy_oled_send_cmd(dev, SSD1306_CMD_DISPLAY_OFF);
}
