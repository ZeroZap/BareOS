/**
 * @file xy_lcd_i8080.c
 * @brief LCD I8080 Interface Driver Implementation
 * @version 1.0.0
 * @date 2026-03-05
 */

#include "xy_lcd_i8080.h"
#include "xy_hal_gpio.h"
#include "xy_hal_delay.h"
#include <string.h>

/* ==================== Internal Macros ==================== */

/* I8080 control signal operations */
#define LCD_I8080_ASSERT_CS(lcd)      xy_gpio_write(lcd->cs_pin, 0)
#define LCD_I8080_DEASSERT_CS(lcd)    xy_gpio_write(lcd->cs_pin, 1)
#define LCD_I8080_SET_RS_CMD(lcd)     xy_gpio_write(lcd->rs_pin, 0)
#define LCD_I8080_SET_RS_DATA(lcd)    xy_gpio_write(lcd->rs_pin, 1)
#define LCD_I8080_ASSERT_WR(lcd)      xy_gpio_write(lcd->wr_pin, 0)
#define LCD_I8080_DEASSERT_WR(lcd)    xy_gpio_write(lcd->wr_pin, 1)
#define LCD_I8080_ASSERT_RD(lcd)      xy_gpio_write(lcd->rd_pin, 0)
#define LCD_I8080_DEASSERT_RD(lcd)    xy_gpio_write(lcd->rd_pin, 1)

/* ==================== Internal Functions ==================== */

/**
 * @brief Write data to data bus
 */
static void xy_lcd_i8080_write_bus(xy_lcd_i8080_device_t *lcd, uint16_t data)
{
    /* Write 16-bit data to GPIO pins */
    for (int i = 0; i < 16; i++) {
        xy_gpio_write(lcd->db_pins[i], (data >> i) & 1);
    }
}

/**
 * @brief Read data from data bus
 */
static uint16_t xy_lcd_i8080_read_bus(xy_lcd_i8080_device_t *lcd)
{
    uint16_t data = 0;

    /* Read 16-bit data from GPIO pins */
    for (int i = 0; i < 16; i++) {
        int state = xy_gpio_read(lcd->db_pins[i]);
        if (state) {
            data |= (1 << i);
        }
    }

    return data;
}

/**
 * @brief Generate write strobe
 */
static void xy_lcd_i8080_write_strobe(xy_lcd_i8080_device_t *lcd)
{
    /* WR pulse: low -> high */
    LCD_I8080_ASSERT_WR(lcd);
    xy_hal_delay_ns(10);  /* Min write pulse width */
    LCD_I8080_DEASSERT_WR(lcd);
}

/**
 * @brief Generate read strobe
 */
static void xy_lcd_i8080_read_strobe(xy_lcd_i8080_device_t *lcd)
{
    /* RD pulse: low -> high */
    LCD_I8080_ASSERT_RD(lcd);
    xy_hal_delay_ns(10);  /* Min read pulse width */
    LCD_I8080_DEASSERT_RD(lcd);
}

/* ==================== Low-Level Operations ==================== */

/**
 * @brief Send command to LCD
 */
void xy_lcd_i8080_write_cmd(xy_lcd_i8080_device_t *lcd, uint8_t cmd)
{
    LCD_I8080_SET_RS_CMD(lcd);
    LCD_I8080_ASSERT_CS(lcd);

    /* Set lower 8 bits for 8-bit command */
    xy_lcd_i8080_write_bus(lcd, cmd);
    xy_lcd_i8080_write_strobe(lcd);

    LCD_I8080_DEASSERT_CS(lcd);
}

/**
 * @brief Send data to LCD
 */
void xy_lcd_i8080_write_data(xy_lcd_i8080_device_t *lcd, const uint8_t *data, uint32_t len)
{
    LCD_I8080_SET_RS_DATA(lcd);
    LCD_I8080_ASSERT_CS(lcd);

    for (uint32_t i = 0; i < len; i++) {
        /* Send 8-bit data on lower byte */
        xy_lcd_i8080_write_bus(lcd, data[i]);
        xy_lcd_i8080_write_strobe(lcd);
    }

    LCD_I8080_DEASSERT_CS(lcd);
}

/**
 * @brief Send 8-bit data to LCD
 */
void xy_lcd_i8080_write_data8(xy_lcd_i8080_device_t *lcd, uint8_t data)
{
    LCD_I8080_SET_RS_DATA(lcd);
    LCD_I8080_ASSERT_CS(lcd);

    xy_lcd_i8080_write_bus(lcd, data);
    xy_lcd_i8080_write_strobe(lcd);

    LCD_I8080_DEASSERT_CS(lcd);
}

/**
 * @brief Send 16-bit data to LCD
 */
void xy_lcd_i8080_write_data16(xy_lcd_i8080_device_t *lcd, uint16_t data)
{
    LCD_I8080_SET_RS_DATA(lcd);
    LCD_I8080_ASSERT_CS(lcd);

    xy_lcd_i8080_write_bus(lcd, data);
    xy_lcd_i8080_write_strobe(lcd);

    LCD_I8080_DEASSERT_CS(lcd);
}

/**
 * @brief Write register
 */
void xy_lcd_i8080_write_reg(xy_lcd_i8080_device_t *lcd, uint8_t reg, uint16_t data)
{
    /* Register write: cmd + data */
    LCD_I8080_SET_RS_CMD(lcd);
    LCD_I8080_ASSERT_CS(lcd);

    xy_lcd_i8080_write_bus(lcd, reg);
    xy_lcd_i8080_write_strobe(lcd);

    LCD_I8080_SET_RS_DATA(lcd);
    xy_lcd_i8080_write_bus(lcd, data);
    xy_lcd_i8080_write_strobe(lcd);

    LCD_I8080_DEASSERT_CS(lcd);
}

/**
 * @brief Read data from LCD
 */
uint16_t xy_lcd_i8080_read_data(xy_lcd_i8080_device_t *lcd)
{
    uint16_t data;

    LCD_I8080_SET_RS_DATA(lcd);
    LCD_I8080_ASSERT_CS(lcd);

    /* Set pins as input */
    /* Note: In real implementation, would configure GPIO as input */

    xy_lcd_i8080_read_strobe(lcd);
    data = xy_lcd_i8080_read_bus(lcd);

    LCD_I8080_DEASSERT_CS(lcd);

    return data;
}

/* ==================== Window Operations ==================== */

/**
 * @brief Set write window
 */
void xy_lcd_i8080_set_window(xy_lcd_i8080_device_t *lcd, uint16_t x, uint16_t y,
                             uint16_t w, uint16_t h)
{
    uint16_t x_end = x + w - 1;
    uint16_t y_end = y + h - 1;

    /* Column address set */
    xy_lcd_i8080_write_cmd(lcd, 0x2A);
    xy_lcd_i8080_write_data16(lcd, x);
    xy_lcd_i8080_write_data16(lcd, x_end);

    /* Row address set */
    xy_lcd_i8080_write_cmd(lcd, 0x2B);
    xy_lcd_i8080_write_data16(lcd, y);
    xy_lcd_i8080_write_data16(lcd, y_end);

    /* Memory write start */
    xy_lcd_i8080_write_cmd(lcd, 0x2C);
}

/**
 * @brief Write pixel data
 */
void xy_lcd_i8080_write_pixel(xy_lcd_i8080_device_t *lcd, const uint16_t *data, uint32_t len)
{
    LCD_I8080_SET_RS_DATA(lcd);
    LCD_I8080_ASSERT_CS(lcd);

    for (uint32_t i = 0; i < len; i++) {
        xy_lcd_i8080_write_bus(lcd, data[i]);
        xy_lcd_i8080_write_strobe(lcd);
    }

    LCD_I8080_DEASSERT_CS(lcd);
}

/* ==================== Control Operations ==================== */

/**
 * @brief Reset LCD
 */
void xy_lcd_i8080_reset(xy_lcd_i8080_device_t *lcd)
{
    if (lcd->rst_pin == 0) {
        return;
    }

    xy_gpio_write(lcd->rst_pin, 1);
    xy_hal_delay_ms(10);
    xy_gpio_write(lcd->rst_pin, 0);
    xy_hal_delay_ms(10);
    xy_gpio_write(lcd->rst_pin, 1);
    xy_hal_delay_ms(120);
}

/**
 * @brief Set backlight
 */
void xy_lcd_i8080_set_backlight(xy_lcd_i8080_device_t *lcd, uint8_t brightness)
{
    /* I8080 interface doesn't directly support backlight control */
    /* External GPIO or PWM control would be needed */
    (void)lcd;
    (void)brightness;
}

/**
 * @brief Power on/off
 */
void xy_lcd_i8080_power(xy_lcd_i8080_device_t *lcd, bool on)
{
    if (on) {
        xy_lcd_i8080_write_cmd(lcd, 0x29);  /* Display ON */
    } else {
        xy_lcd_i8080_write_cmd(lcd, 0x28);  /* Display OFF */
    }
    (void)lcd;
    (void)on;
}

/* ==================== Driver Operations ==================== */

static xy_error_t xy_lcd_i8080_driver_init(xy_lcd_device_t *dev)
{
    (void)dev;
    return XY_ERR_OK;
}

static xy_error_t xy_lcd_i8080_driver_deinit(xy_lcd_device_t *dev)
{
    xy_lcd_i8080_device_t *lcd = (xy_lcd_i8080_device_t *)dev;
    if (lcd->initialized) {
        xy_lcd_i8080_deinit(lcd);
    }
    return XY_ERR_OK;
}

static void xy_lcd_i8080_driver_clear(xy_lcd_device_t *dev, uint16_t color)
{
    xy_lcd_i8080_device_t *lcd = (xy_lcd_i8080_device_t *)dev;

    xy_lcd_i8080_set_window(lcd, 0, 0, dev->width, dev->height);

    /* Fill with color */
    for (uint32_t i = 0; i < dev->width * dev->height; i++) {
        xy_lcd_i8080_write_data16(lcd, color);
    }
}

static void xy_lcd_i8080_driver_draw_pixel(xy_lcd_device_t *dev, uint16_t x, uint16_t y, uint16_t color)
{
    xy_lcd_i8080_device_t *lcd = (xy_lcd_i8080_device_t *)dev;

    xy_lcd_i8080_set_window(lcd, x, y, 1, 1);
    xy_lcd_i8080_write_data16(lcd, color);
}

static void xy_lcd_i8080_driver_draw_rect(xy_lcd_device_t *dev, uint16_t x, uint16_t y,
                                         uint16_t w, uint16_t h, uint16_t color, bool filled)
{
    xy_lcd_i8080_device_t *lcd = (xy_lcd_i8080_device_t *)dev;

    if (filled) {
        xy_lcd_i8080_set_window(lcd, x, y, w, h);

        uint32_t total = (uint32_t)w * h;
        for (uint32_t i = 0; i < total; i++) {
            xy_lcd_i8080_write_data16(lcd, color);
        }
    } else {
        /* Draw outline */
        xy_lcd_i8080_set_window(lcd, x, y, w, 1);
        for (uint16_t i = 0; i < w; i++) {
            xy_lcd_i8080_write_data16(lcd, color);
        }

        xy_lcd_i8080_set_window(lcd, x, y + h - 1, w, 1);
        for (uint16_t i = 0; i < w; i++) {
            xy_lcd_i8080_write_data16(lcd, color);
        }

        xy_lcd_i8080_set_window(lcd, x, y, 1, h);
        for (uint16_t i = 0; i < h; i++) {
            xy_lcd_i8080_write_data16(lcd, color);
        }

        xy_lcd_i8080_set_window(lcd, x + w - 1, y, 1, h);
        for (uint16_t i = 0; i < h; i++) {
            xy_lcd_i8080_write_data16(lcd, color);
        }
    }
}

static void xy_lcd_i8080_driver_refresh(xy_lcd_device_t *dev)
{
    xy_lcd_i8080_device_t *lcd = (xy_lcd_i8080_device_t *)dev;
    uint16_t *fb = dev->framebuffer;
    uint32_t size = dev->fb_width * dev->fb_height;

    if (!fb) {
        return;
    }

    xy_lcd_i8080_set_window(lcd, 0, 0, dev->fb_width, dev->fb_height);
    xy_lcd_i8080_write_pixel(lcd, fb, size);
}

static void xy_lcd_i8080_driver_set_backlight(xy_lcd_device_t *dev, uint8_t brightness)
{
    xy_lcd_i8080_device_t *lcd = (xy_lcd_i8080_device_t *)dev;
    xy_lcd_i8080_set_backlight(lcd, brightness);
}

static void xy_lcd_i8080_driver_power(xy_lcd_device_t *dev, bool on)
{
    xy_lcd_i8080_device_t *lcd = (xy_lcd_i8080_device_t *)dev;
    xy_lcd_i8080_power(lcd, on);
}

/* I8080 LCD operations */
const xy_lcd_ops_t xy_lcd_i8080_ops = {
    .init = xy_lcd_i8080_driver_init,
    .deinit = xy_lcd_i8080_driver_deinit,
    .clear = xy_lcd_i8080_driver_clear,
    .draw_pixel = xy_lcd_i8080_driver_draw_pixel,
    .draw_rect = xy_lcd_i8080_driver_draw_rect,
    .refresh = xy_lcd_i8080_driver_refresh,
    .set_backlight = xy_lcd_i8080_driver_set_backlight,
    .power = xy_lcd_i8080_driver_power,
};

/* ==================== Public API ==================== */

xy_error_t xy_lcd_i8080_init(xy_lcd_i8080_device_t *lcd, const xy_lcd_i8080_config_t *config)
{
    if (!lcd || !config) {
        return XY_ERR_INVALID_PARAM;
    }

    memset(lcd, 0, sizeof(*lcd));

    /* Copy configuration */
    lcd->data_width = config->data_width;
    lcd->db_pins[0] = config->db0_pin;
    lcd->db_pins[1] = config->db1_pin;
    lcd->db_pins[2] = config->db2_pin;
    lcd->db_pins[3] = config->db3_pin;
    lcd->db_pins[4] = config->db4_pin;
    lcd->db_pins[5] = config->db5_pin;
    lcd->db_pins[6] = config->db6_pin;
    lcd->db_pins[7] = config->db7_pin;
    lcd->db_pins[8] = config->db8_pin;
    lcd->db_pins[9] = config->db9_pin;
    lcd->db_pins[10] = config->db10_pin;
    lcd->db_pins[11] = config->db11_pin;
    lcd->db_pins[12] = config->db12_pin;
    lcd->db_pins[13] = config->db13_pin;
    lcd->db_pins[14] = config->db14_pin;
    lcd->db_pins[15] = config->db15_pin;
    lcd->wr_pin = config->wr_pin;
    lcd->rd_pin = config->rd_pin;
    lcd->cs_pin = config->cs_pin;
    lcd->rs_pin = config->rs_pin;
    lcd->rst_pin = config->rst_pin;
    lcd->use_dma = config->use_dma;

    /* Initialize base LCD device */
    xy_lcd_config_t base_config = {
        .interface = XY_LCD_IF_I8080,
        .width = config->base.width,
        .height = config->base.height,
        .color_fmt = config->base.color_fmt,
        .rotation = config->base.rotation,
        .use_dma = config->use_dma,
    };

    xy_error_t err = xy_lcd_init(&lcd->base, &base_config);
    if (err != XY_ERR_OK) {
        return err;
    }

    /* Set operations */
    lcd->base.ops = &xy_lcd_i8080_ops;
    lcd->base.interface = XY_LCD_IF_I8080;
    lcd->base.config = (void *)config;

    /* GPIO initialization would be done here */
    /* For now, assume GPIO is already configured */

    lcd->initialized = true;

    return XY_ERR_OK;
}

xy_error_t xy_lcd_i8080_deinit(xy_lcd_i8080_device_t *lcd)
{
    if (!lcd) {
        return XY_ERR_INVALID_PARAM;
    }

    xy_lcd_deinit(&lcd->base);
    lcd->initialized = false;

    return XY_ERR_OK;
}
