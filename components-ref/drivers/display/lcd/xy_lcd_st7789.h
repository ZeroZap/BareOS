/**
 * @file xy_lcd_st7789.h
 * @brief ST7789VW LCD Driver Header
 * @version 1.0.0
 * @date 2026-03-05
 */

#ifndef XY_LCD_ST7789_H
#define XY_LCD_ST7789_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_lcd.h"
#include "xy_lcd_spi.h"
#include <stdint.h>
#include <stdbool.h>

/* ==================== ST7789 Configuration ==================== */

/**
 * @brief ST7789 configuration
 */
typedef struct {
    /* SPI interface config */
    xy_lcd_spi_config_t spi;

    /* ST7789 specific settings */
    uint16_t offset_x;         /**< X offset (for boards with offset) */
    uint16_t offset_y;         /**< Y offset */
    bool rgb_order;            /**< RGB or BGR order */
    bool invert_on_init;       /**< Invert colors on init */
} xy_lcd_st7789_config_t;

/**
 * @brief ST7789 device structure
 */
typedef struct {
    xy_lcd_spi_device_t spi_dev;   /**< SPI device */

    /* ST7789 specific */
    uint16_t offset_x;             /**< X offset */
    uint16_t offset_y;             /**< Y offset */
    bool rgb_order;                /**< RGB order */

    /* State */
    bool initialized;              /**< Init flag */
} xy_lcd_st7789_device_t;

/* ==================== ST7789 Registers ==================== */

/* System function commands */
#define ST7789_CMD_NOP         0x00    /**< No operation */
#define ST7789_CMD_SWRESET     0x01    /**< Software reset */
#define ST7789_CMD_RDDID       0x04    /**< Read display ID */
#define ST7789_CMD_RDDST       0x09    /**< Read display status */
#define ST7789_CMD_RDDPM       0x0A    /**< Read display power mode */
#define ST7789_CMD_RDDMADCTL   0x0B    /**< Read display MADCTL */
#define ST7789_CMD_RDDCOLMOD   0x0C    /**< Read display pixel format */
#define ST7789_CMD_RDDIM       0x0D    /**< Read display image mode */
#define ST7789_CMD_RDDSM       0x0E    /**< Read display signal mode */
#define ST7789_CMD_RDDSDR      0x0F    /**< Read display self diagnostic result */

/* Sleep commands */
#define ST7789_CMD_SLPIN       0x10    /**< Sleep in */
#define ST7789_CMD_SLPOUT      0x11    /**< Sleep out */

/* Normal mode commands */
#define ST7789_CMD_PTLON       0x12    /**< Partial mode ON */
#define ST7789_CMD_NORON       0x13    /**< Normal display mode ON */

/* Inversion commands */
#define ST7789_CMD_INVON       0x20    /**< Display inversion ON */
#define ST7789_CMD_INVOFF      0x21    /**< Display inversion OFF */

/* Gamma commands */
#define ST7789_CMD_GAMSET      0x26    /**< Gamma set */
#define ST7789_CMD_GMCTRP1     0xE0    /**< Gamma curve positive 1 */
#define ST7789_CMD_GMCTRN1     0xE1    /**< Gamma curve negative 1 */

/* Display commands */
#define ST7789_CMD_DISPOFF     0x28    /**< Display OFF */
#define ST7789_CMD_DISPON      0x29    /**< Display ON */
#define ST7789_CMD_CASET       0x2A    /**< Column address set */
#define ST7789_CMD_RASET       0x2B    /**< Row address set */
#define ST7789_CMD_RAMWR       0x2C    /**< Memory write */
#define ST7789_CMD_RGBSET      0x2D    /**< Color set */
#define ST7789_CMD_RAMRD       0x2E    /**< Memory read */

/* Partial area commands */
#define ST7789_CMD_PLTAR       0x30    /**< Partial area */
#define ST7789_CMD_VSCSAD      0x33    /**< Vertical scroll start address */
#define ST7789_CMD_TEOFF       0x34    /**< Tearing line OFF */
#define ST7789_CMD_TEON        0x35    /**< Tearing line ON */
#define ST7789_CMD_MADCTL      0x36    /**< Memory data access control */
#define ST7789_CMD_VSCRAD      0x37    /**< Vertical scroll address */
#define ST7789_CMD_IDMOFF      0x38    /**< Idle mode OFF */
#define ST7789_CMD_IDMON       0x39    /**< Idle mode ON */
#define ST7789_CMD_COLMOD      0x3A    /**< Pixel format set */
#define ST7789_CMD_WRMEMCONT   0x3C    /**< Write memory continue */
#define ST7789_CMD_RDMEMCONT   0x3E    /**< Read memory continue */

/* Brightness commands */
#define ST7789_CMD_WRDBR       0x51    /**< Write brightness */
#define ST7789_CMD_RDBR        0x52    /**< Read brightness */
#define ST7789_CMD_WRCTRLBD    0x53    /**< Write control brightness */
#define ST7789_CMD_RDCTRLBD    0x54    /**< Read control brightness */
#define ST7789_CMD_WRCABC      0x55    /**< Write content adaptive brightness */
#define ST7789_CMD_RDCABC      0x56    /**< Read content adaptive brightness */
#define ST7789_CMD_WRCABCMIN   0x5E    /**< Write CABC minimum brightness */
#define ST7789_CMD_RDCABCMIN   0x5F    /**< Read CABC minimum brightness */

/* Column address */
#define ST7789_CMD_CLRPAMP     0x06    /**< Clear lower panel */

/* Read ID */
#define ST7789_CMD_RDID1       0xDA    /**< Read ID1 */
#define ST7789_CMD_RDID2       0xDB    /**< Read ID2 */
#define ST7789_CMD_RDID3       0xDC    /**< Read ID3 */

/* ==================== ST7789 API ==================== */

/**
 * @brief Initialize ST7789 LCD device
 * @param lcd LCD device
 * @param config Configuration
 * @return XY_ERR_OK on success
 */
xy_error_t xy_lcd_st7789_init(xy_lcd_st7789_device_t *lcd, const xy_lcd_st7789_config_t *config);

/**
 * @brief Deinitialize ST7789 LCD device
 * @param lcd LCD device
 * @return XY_ERR_OK on success
 */
xy_error_t xy_lcd_st7789_deinit(xy_lcd_st7789_device_t *lcd);

/**
 * @brief Reset ST7789
 * @param lcd LCD device
 */
void xy_lcd_st7789_reset(xy_lcd_st7789_device_t *lcd);

/**
 * @brief Send command to ST7789
 * @param lcd LCD device
 * @param cmd Command
 */
void xy_lcd_st7789_write_cmd(xy_lcd_st7789_device_t *lcd, uint8_t cmd);

/**
 * @brief Send data to ST7789
 * @param lcd LCD device
 * @param data Data buffer
 * @param len Data length
 */
void xy_lcd_st7789_write_data(xy_lcd_st7789_device_t *lcd, const uint8_t *data, uint32_t len);

/**
 * @brief Send 8-bit data to ST7789
 * @param lcd LCD device
 * @param data Data byte
 */
void xy_lcd_st7789_write_data8(xy_lcd_st7789_device_t *lcd, uint8_t data);

/**
 * @brief Set pixel format
 * @param lcd LCD device
 * @param format 16-bit or 18-bit
 */
void xy_lcd_st7789_set_pixel_format(xy_lcd_st7789_device_t *lcd, uint8_t format);

/**
 * @brief Set memory access control (rotation, order)
 * @param lcd LCD device
 * @param madctl Control value
 */
void xy_lcd_st7789_set_madctl(xy_lcd_st7789_device_t *lcd, uint8_t madctl);

/**
 * @brief Set column address
 * @param lcd LCD device
 * @param x X start
 * @param w Width
 */
void xy_lcd_st7789_set_column(xy_lcd_st7789_device_t *lcd, uint16_t x, uint16_t w);

/**
 * @brief Set row address
 * @param lcd LCD device
 * @param y Y start
 * @param h Height
 */
void xy_lcd_st7789_set_row(xy_lcd_st7789_device_t *lcd, uint16_t y, uint16_t h);

/**
 * @brief Set window
 * @param lcd LCD device
 * @param x X start
 * @param y Y start
 * @param w Width
 * @param h Height
 */
void xy_lcd_st7789_set_window(xy_lcd_st7789_device_t *lcd, uint16_t x, uint16_t y,
                              uint16_t w, uint16_t h);

/**
 * @brief Write pixels to window
 * @param lcd LCD device
 * @param data Pixel data
 * @param len Number of pixels
 */
void xy_lcd_st7789_write_pixel(xy_lcd_st7789_device_t *lcd, const uint16_t *data, uint32_t len);

/**
 * @brief Clear screen with color
 * @param lcd LCD device
 * @param color Color (RGB565)
 */
void xy_lcd_st7789_clear(xy_lcd_st7789_device_t *lcd, uint16_t color);

/**
 * @brief Draw single pixel
 * @param lcd LCD device
 * @param x X coordinate
 * @param y Y coordinate
 * @param color Color (RGB565)
 */
void xy_lcd_st7789_draw_pixel(xy_lcd_st7789_device_t *lcd, uint16_t x, uint16_t y, uint16_t color);

/**
 * @brief Fill rectangle
 * @param lcd LCD device
 * @param x X position
 * @param y Y position
 * @param w Width
 * @param h Height
 * @param color Color (RGB565)
 */
void xy_lcd_st7789_fill(xy_lcd_st7789_device_t *lcd, uint16_t x, uint16_t y,
                        uint16_t w, uint16_t h, uint16_t color);

/**
 * @brief Refresh display from framebuffer
 * @param lcd LCD device
 */
void xy_lcd_st7789_refresh(xy_lcd_st7789_device_t *lcd);

/**
 * @brief Set backlight
 * @param lcd LCD device
 * @param brightness Brightness 0-100
 */
void xy_lcd_st7789_set_backlight(xy_lcd_st7789_device_t *lcd, uint8_t brightness);

/**
 * @brief Sleep in
 * @param lcd LCD device
 */
void xy_lcd_st7789_sleep_in(xy_lcd_st7789_device_t *lcd);

/**
 * @brief Sleep out
 * @param lcd LCD device
 */
void xy_lcd_st7789_sleep_out(xy_lcd_st7789_device_t *lcd);

/**
 * @brief Set inversion
 * @param lcd LCD device
 * @param invert Invert or not
 */
void xy_lcd_st7789_set_inversion(xy_lcd_st7789_device_t *lcd, bool invert);

/* ==================== Driver-Level Operations ==================== */

/**
 * @brief ST7789 LCD operations (for registration)
 */
extern const xy_lcd_ops_t xy_lcd_st7789_ops;

#ifdef __cplusplus
}
#endif

#endif /* XY_LCD_ST7789_H */
