/**
 * @file xy_lcd_st7789.c
 * @brief ST7789VW LCD Driver Implementation
 * @version 1.0.0
 * @date 2026-03-05
 */

#include "xy_lcd_st7789.h"
#include "xy_hal_delay.h"
#include <string.h>

/* ==================== MADCTL Bits ==================== */

#define ST7789_MADCTL_MY      0x80    /**< Row address order */
#define ST7789_MADCTL_MX      0x40    /**< Column address order */
#define ST7789_MADCTL_MV      0x20    /**< Row/Column exchange */
#define ST7789_MADCTL_ML      0x10    /**< Vertical refresh order */
#define ST7789_MADCTL_BGR     0x08    /**< RGB-BGR order */
#define ST7789_MADCTL_MH      0x04    /**< Horizontal refresh order */

/* ==================== Initialization Sequence ==================== */

/**
 * @brief ST7789 initialization sequence
 */
static const uint8_t st7789_init_sequence[] = {
    /* Command, data length, data... */
    0x01, 0,              /* Software reset */
    0x11, 0,              /* Sleep out */
    0x3A, 1, 0x55,        /* Pixel format: 16-bit (RGB565) */
    0xB2, 5, 0x0C, 0x0C, 0x00, 0x33, 0x33,  /* Porch control */
    0xB7, 1, 0x35,        /* Gate control */
    0xBB, 1, 0x28,        /* VCOMS setting */
    0xC0, 1, 0x2C,        /* LCM control */
    0xC2, 1, 0x01,        /* VAP and VAN */
    0xC3, 1, 0x12,        /* VRH0 */
    0xC4, 1, 0x20,        /* VRH1 */
    0xC6, 1, 0x0F,        /* Frame rate */
    0xD0, 2, 0xA4, 0xA1,  /* Power control */
    0xE0, 14, 0xD0, 0x00, 0x05, 0x0E, 0x15, 0x0D, 0x37, 0x43, 0x47, 0x09, 0x15, 0x12, 0x16, 0x19,  /* Gamma positive */
    0xE1, 14, 0xD0, 0x00, 0x05, 0x0E, 0x15, 0x0D, 0x37, 0x43, 0x47, 0x09, 0x15, 0x12, 0x16, 0x19,  /* Gamma negative */
    0x21, 0,              /* Display inversion on */
    0x29, 0,              /* Display on */
};

/**
 * @brief Send initialization sequence
 */
static void xy_lcd_st7789_send_init_sequence(xy_lcd_st7789_device_t *lcd)
{
    const uint8_t *ptr = st7789_init_sequence;

    while (ptr < st7789_init_sequence + sizeof(st7789_init_sequence)) {
        uint8_t cmd = *ptr++;
        uint8_t len = *ptr++;

        xy_lcd_st7789_write_cmd(lcd, cmd);

        for (uint8_t i = 0; i < len; i++) {
            xy_lcd_st7789_write_data8(lcd, *ptr++);
        }

        /* Delay after certain commands */
        if (cmd == 0x01 || cmd == 0x11) {
            xy_hal_delay_ms(120);
        } else if (cmd == 0x29) {
            xy_hal_delay_ms(50);
        }
    }
}

/* ==================== Low-Level Operations ==================== */

/**
 * @brief Write command to ST7789
 */
void xy_lcd_st7789_write_cmd(xy_lcd_st7789_device_t *lcd, uint8_t cmd)
{
    xy_lcd_spi_device_t *spi = &lcd->spi_dev;

    /* Set DC low for command */
    xy_gpio_write(spi->dc_pin, 0);
    xy_gpio_write(spi->cs_pin, 0);

    /* Transfer command */
    xy_spi_transmit(spi->spi_handle, &cmd, 1);

    xy_gpio_write(spi->cs_pin, 1);
}

/**
 * @brief Write data to ST7789
 */
void xy_lcd_st7789_write_data(xy_lcd_st7789_device_t *lcd, const uint8_t *data, uint32_t len)
{
    xy_lcd_spi_device_t *spi = &lcd->spi_dev;

    /* Set DC high for data */
    xy_gpio_write(spi->dc_pin, 1);
    xy_gpio_write(spi->cs_pin, 0);

    /* Transfer data */
    xy_spi_transmit(spi->spi_handle, data, len);

    xy_gpio_write(spi->cs_pin, 1);
}

/**
 * @brief Write 8-bit data
 */
void xy_lcd_st7789_write_data8(xy_lcd_st7789_device_t *lcd, uint8_t data)
{
    xy_lcd_st7789_write_data(lcd, &data, 1);
}

/**
 * @brief Set pixel format
 */
void xy_lcd_st7789_set_pixel_format(xy_lcd_st7789_device_t *lcd, uint8_t format)
{
    xy_lcd_st7789_write_cmd(lcd, ST7789_CMD_COLMOD);
    xy_lcd_st7789_write_data8(lcd, format);  /* 0x55 = 16-bit, 0x66 = 18-bit */
}

/**
 * @brief Set memory access control
 */
void xy_lcd_st7789_set_madctl(xy_lcd_st7789_device_t *lcd, uint8_t madctl)
{
    /* Set BGR bit if not RGB order */
    if (!lcd->rgb_order) {
        madctl |= ST7789_MADCTL_BGR;
    }

    xy_lcd_st7789_write_cmd(lcd, ST7789_CMD_MADCTL);
    xy_lcd_st7789_write_data8(lcd, madctl);
}

/**
 * @brief Reset ST7789
 */
void xy_lcd_st7789_reset(xy_lcd_st7789_device_t *lcd)
{
    xy_lcd_spi_device_t *spi = &lcd->spi_dev;

    if (spi->rst_pin == 0) {
        return;
    }

    /* Hardware reset sequence */
    xy_gpio_write(spi->rst_pin, 1);
    xy_hal_delay_ms(10);
    xy_gpio_write(spi->rst_pin, 0);
    xy_hal_delay_ms(10);
    xy_gpio_write(spi->rst_pin, 1);
    xy_hal_delay_ms(120);
}

/* ==================== Window Operations ==================== */

/**
 * @brief Set column address
 */
void xy_lcd_st7789_set_column(xy_lcd_st7789_device_t *lcd, uint16_t x, uint16_t w)
{
    uint16_t x_end = x + w - 1 + lcd->offset_x;

    xy_lcd_st7789_write_cmd(lcd, ST7789_CMD_CASET);
    xy_lcd_st7789_write_data8(lcd, (uint8_t)(lcd->offset_x >> 8));
    xy_lcd_st7789_write_data8(lcd, (uint8_t)(lcd->offset_x & 0xFF));
    xy_lcd_st7789_write_data8(lcd, (uint8_t)(x_end >> 8));
    xy_lcd_st7789_write_data8(lcd, (uint8_t)(x_end & 0xFF));
}

/**
 * @brief Set row address
 */
void xy_lcd_st7789_set_row(xy_lcd_st7789_device_t *lcd, uint16_t y, uint16_t h)
{
    uint16_t y_end = y + h - 1 + lcd->offset_y;

    xy_lcd_st7789_write_cmd(lcd, ST7789_CMD_RASET);
    xy_lcd_st7789_write_data8(lcd, (uint8_t)(lcd->offset_y >> 8));
    xy_lcd_st7789_write_data8(lcd, (uint8_t)(lcd->offset_y & 0xFF));
    xy_lcd_st7789_write_data8(lcd, (uint8_t)(y_end >> 8));
    xy_lcd_st7789_write_data8(lcd, (uint8_t)(y_end & 0xFF));
}

/**
 * @brief Set window
 */
void xy_lcd_st7789_set_window(xy_lcd_st7789_device_t *lcd, uint16_t x, uint16_t y,
                              uint16_t w, uint16_t h)
{
    xy_lcd_st7789_set_column(lcd, x, w);
    xy_lcd_st7789_set_row(lcd, y, h);

    /* Start memory write */
    xy_lcd_st7789_write_cmd(lcd, ST7789_CMD_RAMWR);
}

/**
 * @brief Write pixels to window
 */
void xy_lcd_st7789_write_pixel(xy_lcd_st7789_device_t *lcd, const uint16_t *data, uint32_t len)
{
    xy_lcd_spi_device_t *spi = &lcd->spi_dev;

    /* Set DC high for data */
    xy_gpio_write(spi->dc_pin, 1);
    xy_gpio_write(spi->cs_pin, 0);

    /* Convert RGB565 to bytes and send */
    uint8_t *buf = (uint8_t *)data;
    uint32_t byte_len = len * 2;

    if (spi->use_dma) {
        xy_spi_transmit_dma(spi->spi_handle, buf, byte_len);
    } else {
        xy_spi_transmit(spi->spi_handle, buf, byte_len);
    }

    xy_gpio_write(spi->cs_pin, 1);
}

/* ==================== Control Operations ==================== */

/**
 * @brief Clear screen
 */
void xy_lcd_st7789_clear(xy_lcd_st7789_device_t *lcd, uint16_t color)
{
    xy_lcd_spi_device_t *spi = &lcd->spi_dev;
    uint16_t width = spi->base.width;
    uint16_t height = spi->base.height;

    xy_lcd_st7789_set_window(lcd, 0, 0, width, height);

    /* Fill entire screen with color */
    uint32_t total = (uint32_t)width * height;

    /* For efficiency, we send all data at once */
    uint8_t *fill_buf = (uint8_t *)malloc(total * 2);
    if (fill_buf) {
        for (uint32_t i = 0; i < total; i++) {
            fill_buf[i * 2] = (uint8_t)(color >> 8);
            fill_buf[i * 2 + 1] = (uint8_t)(color & 0xFF);
        }

        xy_lcd_spi_write_data(spi, fill_buf, total * 2);
        free(fill_buf);
    }
}

/**
 * @brief Draw single pixel
 */
void xy_lcd_st7789_draw_pixel(xy_lcd_st7789_device_t *lcd, uint16_t x, uint16_t y, uint16_t color)
{
    /* Apply offset */
    x += lcd->offset_x;
    y += lcd->offset_y;

    xy_lcd_st7789_set_window(lcd, x, y, 1, 1);

    uint8_t data[2] = { (uint8_t)(color >> 8), (uint8_t)(color & 0xFF) };
    xy_lcd_spi_write_data(&lcd->spi_dev, data, 2);
}

/**
 * @brief Fill rectangle
 */
void xy_lcd_st7789_fill(xy_lcd_st7789_device_t *lcd, uint16_t x, uint16_t y,
                        uint16_t w, uint16_t h, uint16_t color)
{
    xy_lcd_st7789_set_window(lcd, x, y, w, h);

    /* For efficiency, prepare buffer and send at once */
    uint32_t total = (uint32_t)w * h;
    uint8_t *fill_buf = (uint8_t *)malloc(total * 2);
    if (fill_buf) {
        for (uint32_t i = 0; i < total; i++) {
            fill_buf[i * 2] = (uint8_t)(color >> 8);
            fill_buf[i * 2 + 1] = (uint8_t)(color & 0xFF);
        }

        xy_lcd_spi_write_data(&lcd->spi_dev, fill_buf, total * 2);
        free(fill_buf);
    }
}

/**
 * @brief Refresh display from framebuffer
 */
void xy_lcd_st7789_refresh(xy_lcd_st7789_device_t *lcd)
{
    xy_lcd_spi_device_t *spi = &lcd->spi_dev;
    uint16_t *fb = spi->base.framebuffer;
    uint16_t width = spi->base.fb_width;
    uint16_t height = spi->base.fb_height;

    if (!fb) {
        return;
    }

    xy_lcd_st7789_set_window(lcd, 0, 0, width, height);
    xy_lcd_st7789_write_pixel(lcd, fb, (uint32_t)width * height);
}

/**
 * @brief Set backlight
 */
void xy_lcd_st7789_set_backlight(xy_lcd_st7789_device_t *lcd, uint8_t brightness)
{
    xy_lcd_spi_set_backlight(&lcd->spi_dev, brightness);
}

/**
 * @brief Sleep in
 */
void xy_lcd_st7789_sleep_in(xy_lcd_st7789_device_t *lcd)
{
    xy_lcd_st7789_write_cmd(lcd, ST7789_CMD_SLPIN);
    xy_hal_delay_ms(120);
}

/**
 * @brief Sleep out
 */
void xy_lcd_st7789_sleep_out(xy_lcd_st7789_device_t *lcd)
{
    xy_lcd_st7789_write_cmd(lcd, ST7789_CMD_SLPOUT);
    xy_hal_delay_ms(120);
}

/**
 * @brief Set inversion
 */
void xy_lcd_st7789_set_inversion(xy_lcd_st7789_device_t *lcd, bool invert)
{
    if (invert) {
        xy_lcd_st7789_write_cmd(lcd, ST7789_CMD_INVON);
    } else {
        xy_lcd_st7789_write_cmd(lcd, ST7789_CMD_INVOFF);
    }
}

/* ==================== Driver Operations ==================== */

static xy_error_t xy_lcd_st7789_driver_init(xy_lcd_device_t *dev)
{
    (void)dev;
    return XY_ERR_OK;
}

static xy_error_t xy_lcd_st7789_driver_deinit(xy_lcd_device_t *dev)
{
    xy_lcd_st7789_device_t *lcd = (xy_lcd_st7789_device_t *)dev;
    if (lcd->initialized) {
        xy_lcd_st7789_deinit(lcd);
    }
    return XY_ERR_OK;
}

static void xy_lcd_st7789_driver_clear(xy_lcd_device_t *dev, uint16_t color)
{
    xy_lcd_st7789_device_t *lcd = (xy_lcd_st7789_device_t *)dev;
    xy_lcd_st7789_clear(lcd, color);
}

static void xy_lcd_st7789_driver_draw_pixel(xy_lcd_device_t *dev, uint16_t x, uint16_t y, uint16_t color)
{
    xy_lcd_st7789_device_t *lcd = (xy_lcd_st7789_device_t *)dev;
    xy_lcd_st7789_draw_pixel(lcd, x, y, color);
}

static void xy_lcd_st7789_driver_draw_rect(xy_lcd_device_t *dev, uint16_t x, uint16_t y,
                                           uint16_t w, uint16_t h, uint16_t color, bool filled)
{
    xy_lcd_st7789_device_t *lcd = (xy_lcd_st7789_device_t *)dev;

    if (filled) {
        xy_lcd_st7789_fill(lcd, x, y, w, h, color);
    } else {
        /* Draw outline using draw_pixel */
        for (uint16_t i = 0; i < w; i++) {
            xy_lcd_st7789_draw_pixel(lcd, x + i, y, color);
            xy_lcd_st7789_draw_pixel(lcd, x + i, y + h - 1, color);
        }
        for (uint16_t i = 1; i < h - 1; i++) {
            xy_lcd_st7789_draw_pixel(lcd, x, y + i, color);
            xy_lcd_st7789_draw_pixel(lcd, x + w - 1, y + i, color);
        }
    }
}

static void xy_lcd_st7789_driver_refresh(xy_lcd_device_t *dev)
{
    xy_lcd_st7789_device_t *lcd = (xy_lcd_st7789_device_t *)dev;
    xy_lcd_st7789_refresh(lcd);
}

static void xy_lcd_st7789_driver_set_backlight(xy_lcd_device_t *dev, uint8_t brightness)
{
    xy_lcd_st7789_device_t *lcd = (xy_lcd_st7789_device_t *)dev;
    xy_lcd_st7789_set_backlight(lcd, brightness);
}

static void xy_lcd_st7789_driver_power(xy_lcd_device_t *dev, bool on)
{
    xy_lcd_st7789_device_t *lcd = (xy_lcd_st7789_device_t *)dev;

    if (on) {
        xy_lcd_st7789_sleep_out(lcd);
        xy_lcd_st7789_write_cmd(lcd, ST7789_CMD_DISPON);
    } else {
        xy_lcd_st7789_write_cmd(lcd, ST7789_CMD_DISPOFF);
        xy_lcd_st7789_sleep_in(lcd);
    }
}

/* ST7789 LCD operations */
const xy_lcd_ops_t xy_lcd_st7789_ops = {
    .init = xy_lcd_st7789_driver_init,
    .deinit = xy_lcd_st7789_driver_deinit,
    .clear = xy_lcd_st7789_driver_clear,
    .draw_pixel = xy_lcd_st7789_driver_draw_pixel,
    .draw_rect = xy_lcd_st7789_driver_draw_rect,
    .refresh = xy_lcd_st7789_driver_refresh,
    .set_backlight = xy_lcd_st7789_driver_set_backlight,
    .power = xy_lcd_st7789_driver_power,
};

/* ==================== Public API ==================== */

xy_error_t xy_lcd_st7789_init(xy_lcd_st7789_device_t *lcd, const xy_lcd_st7789_config_t *config)
{
    if (!lcd || !config) {
        return XY_ERR_INVALID_PARAM;
    }

    memset(lcd, 0, sizeof(*lcd));

    /* Copy ST7789 specific settings */
    lcd->offset_x = config->offset_x;
    lcd->offset_y = config->offset_y;
    lcd->rgb_order = config->rgb_order;

    /* Initialize SPI device */
    xy_error_t err = xy_lcd_spi_init(&lcd->spi_dev, &config->spi);
    if (err != XY_ERR_OK) {
        return err;
    }

    /* Reset hardware */
    xy_lcd_st7789_reset(lcd);

    /* Send initialization sequence */
    xy_lcd_st7789_send_init_sequence(lcd);

    /* Additional configuration */
    xy_lcd_st7789_set_pixel_format(lcd, 0x55);  /* 16-bit RGB565 */

    /* Set MADCTL with rotation from config */
    uint8_t madctl = 0;
    if (config->spi.base.rotation == XY_LCD_ROTATION_90) {
        madctl = ST7789_MADCTL_MX | ST7789_MADCTL_MV;
    } else if (config->spi.base.rotation == XY_LCD_ROTATION_180) {
        madctl = ST7789_MADCTL_MY;
    } else if (config->spi.base.rotation == XY_LCD_ROTATION_270) {
        madctl = ST7789_MADCTL_MY | ST7789_MADCTL_MV;
    }
    xy_lcd_st7789_set_madctl(lcd, madctl);

    /* Set inversion if requested */
    if (config->invert_on_init) {
        xy_lcd_st7789_set_inversion(lcd, true);
    }

    /* Set operations */
    lcd->spi_dev.base.ops = &xy_lcd_st7789_ops;
    lcd->spi_dev.base.priv = lcd;

    lcd->initialized = true;

    return XY_ERR_OK;
}

xy_error_t xy_lcd_st7789_deinit(xy_lcd_st7789_device_t *lcd)
{
    if (!lcd) {
        return XY_ERR_INVALID_PARAM;
    }

    xy_lcd_spi_deinit(&lcd->spi_dev);
    lcd->initialized = false;

    return XY_ERR_OK;
}
