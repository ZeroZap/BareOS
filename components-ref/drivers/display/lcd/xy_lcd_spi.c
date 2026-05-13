/**
 * @file xy_lcd_spi.c
 * @brief LCD SPI Interface Driver Implementation
 * @version 1.0.0
 * @date 2026-03-05
 */

#include "xy_lcd_spi.h"
#include "xy_hal_spi.h"
#include "xy_hal_gpio.h"
#include "xy_hal_delay.h"
#include <string.h>

/* ==================== Internal Macros ==================== */

#define LCD_SPI_ASSERT_CS(lcd)    xy_gpio_write(lcd->cs_pin, 0)
#define LCD_SPI_DEASSERT_CS(lcd)  xy_gpio_write(lcd->cs_pin, 1)
#define LCD_SPI_SET_DC_CMD(lcd)   xy_gpio_write(lcd->dc_pin, 0)
#define LCD_SPI_SET_DC_DATA(lcd)  xy_gpio_write(lcd->dc_pin, 1)

/* ==================== SPI Low-Level Operations ==================== */

/**
 * @brief Send command to LCD
 */
void xy_lcd_spi_write_cmd(xy_lcd_spi_device_t *lcd, uint8_t cmd)
{
    LCD_SPI_SET_DC_CMD(lcd);
    LCD_SPI_ASSERT_CS(lcd);

    xy_spi_transmit(lcd->spi_handle, &cmd, 1);

    LCD_SPI_DEASSERT_CS(lcd);
}

/**
 * @brief Send data to LCD
 */
void xy_lcd_spi_write_data(xy_lcd_spi_device_t *lcd, const uint8_t *data, uint32_t len)
{
    LCD_SPI_SET_DC_DATA(lcd);
    LCD_SPI_ASSERT_CS(lcd);

    if (lcd->use_dma) {
        xy_spi_transmit_dma(lcd->spi_handle, data, len);
    } else {
        xy_spi_transmit(lcd->spi_handle, data, len);
    }

    LCD_SPI_DEASSERT_CS(lcd);
}

/**
 * @brief Send 8-bit data to LCD
 */
void xy_lcd_spi_write_data8(xy_lcd_spi_device_t *lcd, uint8_t data)
{
    xy_lcd_spi_write_data(lcd, &data, 1);
}

/**
 * @brief Send 16-bit data to LCD
 */
void xy_lcd_spi_write_data16(xy_lcd_spi_device_t *lcd, uint16_t data)
{
    uint8_t buf[2] = { (uint8_t)(data >> 8), (uint8_t)(data & 0xFF) };
    xy_lcd_spi_write_data(lcd, buf, 2);
}

/**
 * @brief Write register
 */
void xy_lcd_spi_write_reg(xy_lcd_spi_device_t *lcd, uint8_t reg, uint8_t data)
{
    LCD_SPI_SET_DC_CMD(lcd);
    LCD_SPI_ASSERT_CS(lcd);

    xy_spi_transmit(lcd->spi_handle, &reg, 1);

    LCD_SPI_SET_DC_DATA(lcd);
    xy_spi_transmit(lcd->spi_handle, &data, 1);

    LCD_SPI_DEASSERT_CS(lcd);
}

/**
 * @brief Read from LCD
 */
void xy_lcd_spi_read(xy_lcd_spi_device_t *lcd, uint8_t *reg, uint8_t *data, uint32_t len)
{
    LCD_SPI_ASSERT_CS(lcd);

    if (reg) {
        LCD_SPI_SET_DC_CMD(lcd);
        xy_spi_transmit(lcd->spi_handle, reg, 1);
    }

    LCD_SPI_SET_DC_DATA(lcd);
    xy_spi_receive(lcd->spi_handle, data, len);

    LCD_SPI_DEASSERT_CS(lcd);
}

/* ==================== Window Operations ==================== */

/**
 * @brief Set write window
 */
void xy_lcd_spi_set_window(xy_lcd_spi_device_t *lcd, uint16_t x, uint16_t y,
                           uint16_t w, uint16_t h)
{
    uint16_t x_end = x + w - 1;
    uint16_t y_end = y + h - 1;

    /* Column address set */
    xy_lcd_spi_write_cmd(lcd, 0x2A);
    xy_lcd_spi_write_data16(lcd, x);
    xy_lcd_spi_write_data16(lcd, x_end);

    /* Row address set */
    xy_lcd_spi_write_cmd(lcd, 0x2B);
    xy_lcd_spi_write_data16(lcd, y);
    xy_lcd_spi_write_data16(lcd, y_end);

    /* Memory write start */
    xy_lcd_spi_write_cmd(lcd, 0x2C);
}

/**
 * @brief Write pixel data
 */
void xy_lcd_spi_write_pixel(xy_lcd_spi_device_t *lcd, const uint16_t *data, uint32_t len)
{
    LCD_SPI_SET_DC_DATA(lcd);
    LCD_SPI_ASSERT_CS(lcd);

    /* Convert RGB565 to bytes for SPI transfer */
    uint8_t *buf = (uint8_t *)data;
    uint32_t byte_len = len * 2;

    if (lcd->use_dma) {
        xy_spi_transmit_dma(lcd->spi_handle, buf, byte_len);
    } else {
        xy_spi_transmit(lcd->spi_handle, buf, byte_len);
    }

    LCD_SPI_DEASSERT_CS(lcd);
}

/* ==================== Control Operations ==================== */

/**
 * @brief Reset LCD
 */
void xy_lcd_spi_reset(xy_lcd_spi_device_t *lcd)
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
void xy_lcd_spi_set_backlight(xy_lcd_spi_device_t *lcd, uint8_t brightness)
{
    if (lcd->bl_pin == 0) {
        return;
    }

    if (lcd->bl_inverted) {
        brightness = 100 - brightness;
    }

    /* Use PWM for backlight control if available */
    /* For now, just toggle pin based on brightness threshold */
    if (brightness > 0) {
        xy_gpio_write(lcd->bl_pin, 1);
    } else {
        xy_gpio_write(lcd->bl_pin, 0);
    }
}

/**
 * @brief Power on/off
 */
void xy_lcd_spi_power(xy_lcd_spi_device_t *lcd, bool on)
{
    if (on) {
        xy_lcd_spi_write_cmd(lcd, 0x29);  /* Display ON */
        xy_lcd_spi_set_backlight(lcd, 100);
    } else {
        xy_lcd_spi_write_cmd(lcd, 0x28);  /* Display OFF */
        xy_lcd_spi_set_backlight(lcd, 0);
    }
}

/* ==================== Driver Operations ==================== */

static xy_error_t xy_lcd_spi_driver_init(xy_lcd_device_t *dev)
{
    /* Initialization is done in xy_lcd_spi_init */
    (void)dev;
    return XY_ERR_OK;
}

static xy_error_t xy_lcd_spi_driver_deinit(xy_lcd_device_t *dev)
{
    xy_lcd_spi_device_t *lcd = (xy_lcd_spi_device_t *)dev;
    if (lcd->initialized) {
        xy_lcd_spi_deinit(lcd);
    }
    return XY_ERR_OK;
}

static void xy_lcd_spi_driver_clear(xy_lcd_device_t *dev, uint16_t color)
{
    xy_lcd_spi_device_t *lcd = (xy_lcd_spi_device_t *)dev;

    xy_lcd_spi_set_window(lcd, 0, 0, dev->width, dev->height);

    /* Fill with color */
    for (uint32_t i = 0; i < dev->width * dev->height; i++) {
        xy_lcd_spi_write_data16(lcd, color);
    }
}

static void xy_lcd_spi_driver_draw_pixel(xy_lcd_device_t *dev, uint16_t x, uint16_t y, uint16_t color)
{
    xy_lcd_spi_device_t *lcd = (xy_lcd_spi_device_t *)dev;

    xy_lcd_spi_set_window(lcd, x, y, 1, 1);
    xy_lcd_spi_write_data16(lcd, color);
}

static void xy_lcd_spi_driver_draw_rect(xy_lcd_device_t *dev, uint16_t x, uint16_t y,
                                        uint16_t w, uint16_t h, uint16_t color, bool filled)
{
    xy_lcd_spi_device_t *lcd = (xy_lcd_spi_device_t *)dev;

    if (filled) {
        xy_lcd_spi_set_window(lcd, x, y, w, h);

        uint32_t total = (uint32_t)w * h;
        for (uint32_t i = 0; i < total; i++) {
            xy_lcd_spi_write_data16(lcd, color);
        }
    } else {
        /* Draw outline */
        xy_lcd_spi_set_window(lcd, x, y, w, 1);
        for (uint16_t i = 0; i < w; i++) {
            xy_lcd_spi_write_data16(lcd, color);
        }

        xy_lcd_spi_set_window(lcd, x, y + h - 1, w, 1);
        for (uint16_t i = 0; i < w; i++) {
            xy_lcd_spi_write_data16(lcd, color);
        }

        xy_lcd_spi_set_window(lcd, x, y, 1, h);
        for (uint16_t i = 0; i < h; i++) {
            xy_lcd_spi_write_data16(lcd, color);
        }

        xy_lcd_spi_set_window(lcd, x + w - 1, y, 1, h);
        for (uint16_t i = 0; i < h; i++) {
            xy_lcd_spi_write_data16(lcd, color);
        }
    }
}

static void xy_lcd_spi_driver_refresh(xy_lcd_device_t *dev)
{
    xy_lcd_spi_device_t *lcd = (xy_lcd_spi_device_t *)dev;
    uint16_t *fb = dev->framebuffer;
    uint32_t size = dev->fb_width * dev->fb_height;

    if (!fb) {
        return;
    }

    xy_lcd_spi_set_window(lcd, 0, 0, dev->fb_width, dev->fb_height);
    xy_lcd_spi_write_pixel(lcd, fb, size);
}

static void xy_lcd_spi_driver_set_backlight(xy_lcd_device_t *dev, uint8_t brightness)
{
    xy_lcd_spi_device_t *lcd = (xy_lcd_spi_device_t *)dev;
    xy_lcd_spi_set_backlight(lcd, brightness);
}

static void xy_lcd_spi_driver_power(xy_lcd_device_t *dev, bool on)
{
    xy_lcd_spi_device_t *lcd = (xy_lcd_spi_device_t *)dev;
    xy_lcd_spi_power(lcd, on);
}

/* SPI LCD operations */
const xy_lcd_ops_t xy_lcd_spi_ops = {
    .init = xy_lcd_spi_driver_init,
    .deinit = xy_lcd_spi_driver_deinit,
    .clear = xy_lcd_spi_driver_clear,
    .draw_pixel = xy_lcd_spi_driver_draw_pixel,
    .draw_rect = xy_lcd_spi_driver_draw_rect,
    .refresh = xy_lcd_spi_driver_refresh,
    .set_backlight = xy_lcd_spi_driver_set_backlight,
    .power = xy_lcd_spi_driver_power,
};

/* ==================== Public API ==================== */

xy_error_t xy_lcd_spi_init(xy_lcd_spi_device_t *lcd, const xy_lcd_spi_config_t *config)
{
    if (!lcd || !config) {
        return XY_ERR_INVALID_PARAM;
    }

    memset(lcd, 0, sizeof(*lcd));

    /* Copy configuration */
    lcd->spi_handle = config->spi_handle;
    lcd->spi_speed = config->spi_speed;
    lcd->spi_mode = config->spi_mode;
    lcd->dc_pin = config->dc_pin;
    lcd->cs_pin = config->cs_pin;
    lcd->rst_pin = config->rst_pin;
    lcd->bl_pin = config->bl_pin;
    lcd->use_dma = config->use_dma;

    /* Initialize base LCD device */
    xy_lcd_config_t base_config = {
        .interface = XY_LCD_IF_SPI,
        .width = config->base.width,
        .height = config->base.height,
        .color_fmt = config->base.color_fmt,
        .rotation = config->base.rotation,
        .backlight_pin = config->bl_pin,
        .use_dma = config->use_dma,
    };

    xy_error_t err = xy_lcd_init(&lcd->base, &base_config);
    if (err != XY_ERR_OK) {
        return err;
    }

    /* Set operations */
    lcd->base.ops = &xy_lcd_spi_ops;
    lcd->base.interface = XY_LCD_IF_SPI;
    lcd->base.config = (void *)config;

    /* GPIO initialization would be done here */
    /* For now, assume GPIO is already configured */

    lcd->initialized = true;

    return XY_ERR_OK;
}

xy_error_t xy_lcd_spi_deinit(xy_lcd_spi_device_t *lcd)
{
    if (!lcd) {
        return XY_ERR_INVALID_PARAM;
    }

    xy_lcd_deinit(&lcd->base);
    lcd->initialized = false;

    return XY_ERR_OK;
}
