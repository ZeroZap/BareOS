/**
 * @file xy_lcd.h
 * @brief LCD Device Driver Framework - Common LCD API and Structures
 * @version 1.0.0
 * @date 2026-03-05
 */

#ifndef XY_LCD_H
#define XY_LCD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_device.h"
#include <stdint.h>
#include <stdbool.h>

/* ==================== LCD Configuration ==================== */

/**
 * @brief LCD color format
 */
typedef enum {
    XY_LCD_COLOR_FORMAT_RGB565 = 0,    /**< RGB565 - 16-bit */
    XY_LCD_COLOR_FORMAT_RGB888 = 1,     /**< RGB888 - 24-bit */
    XY_LCD_COLOR_FORMAT_BGR565 = 2,     /**< BGR565 - 16-bit */
    XY_LCD_COLOR_FORMAT_BGR888 = 3,     /**< BGR888 - 24-bit */
} xy_lcd_color_format_t;

/**
 * @brief LCD interface type
 */
typedef enum {
    XY_LCD_IF_SPI = 0,      /**< SPI interface */
    XY_LCD_IF_I8080 = 1,    /**< I8080 parallel interface */
    XY_LCD_IF_RGB = 2,      /**< RGB interface */
    XY_LCD_IF_DBI = 3,      /**< DBI (Display Bus Interface) */
} xy_lcd_interface_t;

/**
 * @brief LCD rotation
 */
typedef enum {
    XY_LCD_ROTATION_0 = 0,      /**< 0 degree rotation */
    XY_LCD_ROTATION_90 = 1,     /**< 90 degree rotation */
    XY_LCD_ROTATION_180 = 2,    /**< 180 degree rotation */
    XY_LCD_ROTATION_270 = 3,    /**< 270 degree rotation */
} xy_lcd_rotation_t;

/**
 * @brief LCD device structure - common for all LCD drivers
 */
typedef struct xy_lcd_device {
    xy_device_t base;               /**< Base device */

    /* Device info */
    uint16_t width;                 /**< LCD width */
    uint16_t height;                /**< LCD height */
    xy_lcd_color_format_t color_fmt; /**< Color format */
    xy_lcd_interface_t interface;   /**< Interface type */

    /* Frame buffer */
    uint16_t *framebuffer;          /**< Frame buffer (RGB565) */
    uint16_t fb_width;              /**< Frame buffer width */
    uint16_t fb_height;            /**< Frame buffer height */

    /* Rotation */
    xy_lcd_rotation_t rotation;     /**< Current rotation */

    /* Configuration */
    void *config;                   /**< Interface-specific config */
    void *priv;                     /**< Private data for driver */
} xy_lcd_device_t;

/**
 * @brief LCD operations - interface for all LCD drivers
 */
typedef struct xy_lcd_ops {
    /**
     * @brief Initialize LCD
     * @param lcd LCD device
     * @return XY_ERR_OK on success
     */
    xy_error_t (*init)(xy_lcd_device_t *lcd);

    /**
     * @brief Deinitialize LCD
     * @param lcd LCD device
     * @return XY_ERR_OK on success
     */
    xy_error_t (*deinit)(xy_lcd_device_t *lcd);

    /**
     * @brief Clear screen with color
     * @param lcd LCD device
     * @param color Clear color (RGB565)
     */
    void (*clear)(xy_lcd_device_t *lcd, uint16_t color);

    /**
     * @brief Draw pixel
     * @param lcd LCD device
     * @param x X coordinate
     * @param y Y coordinate
     * @param color Pixel color (RGB565)
     */
    void (*draw_pixel)(xy_lcd_device_t *lcd, uint16_t x, uint16_t y, uint16_t color);

    /**
     * @brief Draw rectangle
     * @param lcd LCD device
     * @param x X position
     * @param y Y position
     * @param w Width
     * @param h Height
     * @param color Fill color (RGB565)
     * @param filled Filled or not
     */
    void (*draw_rect)(xy_lcd_device_t *lcd, uint16_t x, uint16_t y,
                      uint16_t w, uint16_t h, uint16_t color, bool filled);

    /**
     * @brief Refresh display from framebuffer
     * @param lcd LCD device
     */
    void (*refresh)(xy_lcd_device_t *lcd);

    /**
     * @brief Set backlight
     * @param lcd LCD device
     * @param brightness Brightness 0-100
     */
    void (*set_backlight)(xy_lcd_device_t *lcd, uint8_t brightness);

    /**
     * @brief Power on/off
     * @param lcd LCD device
     * @param on Power state
     */
    void (*power)(xy_lcd_device_t *lcd, bool on);
} xy_lcd_ops_t;

/**
 * @brief LCD interface configuration - common part
 */
typedef struct {
    xy_lcd_interface_t interface;   /**< Interface type */
    uint16_t width;                 /**< Width */
    uint16_t height;               /**< Height */
    xy_lcd_color_format_t color_fmt; /**< Color format */
    xy_lcd_rotation_t rotation;    /**< Initial rotation */
    uint8_t backlight_pin;         /**< Backlight pin */
    bool use_dma;                  /**< Use DMA */
} xy_lcd_config_t;

/* ==================== LCD API ==================== */

/**
 * @brief Initialize LCD device
 * @param lcd LCD device
 * @param config Configuration
 * @return XY_ERR_OK on success
 */
xy_error_t xy_lcd_init(xy_lcd_device_t *lcd, const xy_lcd_config_t *config);

/**
 * @brief Deinitialize LCD device
 * @param lcd LCD device
 * @return XY_ERR_OK on success
 */
xy_error_t xy_lcd_deinit(xy_lcd_device_t *lcd);

/**
 * @brief Clear screen
 * @param lcd LCD device
 * @param color Clear color
 */
void xy_lcd_clear(xy_lcd_device_t *lcd, uint16_t color);

/**
 * @brief Draw pixel
 * @param lcd LCD device
 * @param x X coordinate
 * @param y Y coordinate
 * @param color Pixel color
 */
void xy_lcd_draw_pixel(xy_lcd_device_t *lcd, uint16_t x, uint16_t y, uint16_t color);

/**
 * @brief Draw horizontal line
 * @param lcd LCD device
 * @param x X start
 * @param y Y position
 * @param len Line length
 * @param color Line color
 */
void xy_lcd_draw_hline(xy_lcd_device_t *lcd, uint16_t x, uint16_t y,
                       uint16_t len, uint16_t color);

/**
 * @brief Draw vertical line
 * @param lcd LCD device
 * @param x X position
 * @param y Y start
 * @param len Line length
 * @param color Line color
 */
void xy_lcd_draw_vline(xy_lcd_device_t *lcd, uint16_t x, uint16_t y,
                       uint16_t len, uint16_t color);

/**
 * @brief Draw rectangle
 * @param lcd LCD device
 * @param x X position
 * @param y Y position
 * @param w Width
 * @param h Height
 * @param color Fill color
 * @param filled Filled or outline
 */
void xy_lcd_draw_rect(xy_lcd_device_t *lcd, uint16_t x, uint16_t y,
                     uint16_t w, uint16_t h, uint16_t color, bool filled);

/**
 * @brief Fill area with color
 * @param lcd LCD device
 * @param x X position
 * @param y Y position
 * @param w Width
 * @param h Height
 * @param color Fill color
 */
void xy_lcd_fill(xy_lcd_device_t *lcd, uint16_t x, uint16_t y,
                uint16_t w, uint16_t h, uint16_t color);

/**
 * @brief Refresh display
 * @param lcd LCD device
 */
void xy_lcd_refresh(xy_lcd_device_t *lcd);

/**
 * @brief Set backlight
 * @param lcd LCD device
 * @param brightness Brightness 0-100
 */
void xy_lcd_set_backlight(xy_lcd_device_t *lcd, uint8_t brightness);

/**
 * @brief Power on/off display
 * @param lcd LCD device
 * @param on Power state
 */
void xy_lcd_power(xy_lcd_device_t *lcd, bool on);

/**
 * @brief Set rotation
 * @param lcd LCD device
 * @param rotation Rotation
 */
void xy_lcd_set_rotation(xy_lcd_device_t *lcd, xy_lcd_rotation_t rotation);

/**
 * @brief Get device width (considering rotation)
 * @param lcd LCD device
 * @return Width
 */
uint16_t xy_lcd_get_width(xy_lcd_device_t *lcd);

/**
 * @brief Get device height (considering rotation)
 * @param lcd LCD device
 * @return Height
 */
uint16_t xy_lcd_get_height(xy_lcd_device_t *lcd);

/* ==================== Utility Functions ==================== */

/**
 * @brief Convert RGB to RGB565
 * @param r Red (0-255)
 * @param g Green (0-255)
 * @param b Blue (0-255)
 * @return RGB565 value
 */
static inline uint16_t xy_rgb_to_rgb565(uint8_t r, uint8_t g, uint8_t b)
{
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

/**
 * @brief Convert RGB565 to RGB
 * @param rgb565 RGB565 value
 * @param r Red output
 * @param g Green output
 * @param b Blue output
 */
static inline void xy_rgb565_to_rgb(uint16_t rgb565, uint8_t *r, uint8_t *g, uint8_t *b)
{
    *r = (rgb565 >> 8) & 0xF8;
    *g = (rgb565 >> 3) & 0xFC;
    *b = (rgb565 << 3) & 0xF8;
}

/* Common color definitions (RGB565) */
#define XY_LCD_COLOR_BLACK       0x0000
#define XY_LCD_COLOR_WHITE       0xFFFF
#define XY_LCD_COLOR_RED         0xF800
#define XY_LCD_COLOR_GREEN       0x07E0
#define XY_LCD_COLOR_BLUE        0x001F
#define XY_LCD_COLOR_YELLOW      0xFFE0
#define XY_LCD_COLOR_CYAN        0x07FF
#define XY_LCD_COLOR_MAGENTA     0xF81F
#define XY_LCD_COLOR_GRAY        0x8410
#define XY_LCD_COLOR_DARK_GRAY   0x4208
#define XY_LCD_COLOR_LIGHT_GRAY  0xC618

#ifdef __cplusplus
}
#endif

#endif /* XY_LCD_H */
