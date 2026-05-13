/**
 * @file xy_lcd_i8080.h
 * @brief LCD I8080 Interface Driver Header
 * @version 1.0.0
 * @date 2026-03-05
 */

#ifndef XY_LCD_I8080_H
#define XY_LCD_I8080_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_lcd.h"
#include "xy_hal_gpio.h"
#include <stdint.h>
#include <stdbool.h>

/* ==================== I8080 LCD Configuration ==================== */

/**
 * @brief I8080 interface width
 */
typedef enum {
    XY_LCD_I8080_8BIT = 0,   /**< 8-bit parallel */
    XY_LCD_I8080_16BIT = 1,   /**< 16-bit parallel (default) */
    XY_LCD_I8080_18BIT = 2,   /**< 18-bit parallel */
} xy_lcd_i8080_width_t;

/**
 * @brief I8080 LCD configuration
 */
typedef struct {
    /* Common config */
    xy_lcd_config_t base;

    /* Interface width */
    xy_lcd_i8080_width_t data_width;   /**< Data bus width */

    /* GPIO pins - data bus (16-bit: D0-D15) */
    uint8_t db0_pin;      /**< Data bit 0 */
    uint8_t db1_pin;      /**< Data bit 1 */
    uint8_t db2_pin;      /**< Data bit 2 */
    uint8_t db3_pin;      /**< Data bit 3 */
    uint8_t db4_pin;      /**< Data bit 4 */
    uint8_t db5_pin;      /**< Data bit 5 */
    uint8_t db6_pin;      /**< Data bit 6 */
    uint8_t db7_pin;      /**< Data bit 7 */
    uint8_t db8_pin;      /**< Data bit 8 */
    uint8_t db9_pin;      /**< Data bit 9 */
    uint8_t db10_pin;     /**< Data bit 10 */
    uint8_t db11_pin;     /**< Data bit 11 */
    uint8_t db12_pin;     /**< Data bit 12 */
    uint8_t db13_pin;     /**< Data bit 13 */
    uint8_t db14_pin;     /**< Data bit 14 */
    uint8_t db15_pin;     /**< Data bit 15 */

    /* Control pins */
    uint8_t wr_pin;       /**< Write strobe pin */
    uint8_t rd_pin;       /**< Read strobe pin */
    uint8_t cs_pin;       /**< Chip select pin */
    uint8_t rs_pin;       /**< Register select pin (DC) */
    uint8_t rst_pin;      /**< Reset pin */
    uint8_t wrx_pin;      /**< Write enable (alternative naming) */

    /* Additional settings */
    bool use_dma;         /**< Use DMA for transfers */
} xy_lcd_i8080_config_t;

/**
 * @brief I8080 LCD device structure
 */
typedef struct {
    xy_lcd_device_t base;       /**< Base LCD device */

    /* Interface settings */
    xy_lcd_i8080_width_t data_width;   /**< Data bus width */

    /* GPIO pins - data bus */
    uint8_t db_pins[16];        /**< Data pin array */

    /* Control pins */
    uint8_t wr_pin;             /**< Write strobe */
    uint8_t rd_pin;             /**< Read strobe */
    uint8_t cs_pin;             /**< Chip select */
    uint8_t rs_pin;             /**< Register select (DC) */
    uint8_t rst_pin;            /**< Reset pin */

    /* DMA */
    bool use_dma;               /**< Use DMA */

    /* Current state */
    bool initialized;           /**< Initialization flag */
} xy_lcd_i8080_device_t;

/* ==================== I8080 LCD API ==================== */

/**
 * @brief Initialize I8080 LCD device
 * @param lcd LCD device
 * @param config Configuration
 * @return XY_ERR_OK on success
 */
xy_error_t xy_lcd_i8080_init(xy_lcd_i8080_device_t *lcd, const xy_lcd_i8080_config_t *config);

/**
 * @brief Deinitialize I8080 LCD device
 * @param lcd LCD device
 * @return XY_ERR_OK on success
 */
xy_error_t xy_lcd_i8080_deinit(xy_lcd_i8080_device_t *lcd);

/**
 * @brief Send command to LCD via I8080
 * @param lcd LCD device
 * @param cmd Command byte
 */
void xy_lcd_i8080_write_cmd(xy_lcd_i8080_device_t *lcd, uint8_t cmd);

/**
 * @brief Send data to LCD via I8080
 * @param lcd LCD device
 * @param data Data buffer
 * @param len Data length
 */
void xy_lcd_i8080_write_data(xy_lcd_i8080_device_t *lcd, const uint8_t *data, uint32_t len);

/**
 * @brief Send 8-bit data to LCD
 * @param lcd LCD device
 * @param data Data byte
 */
void xy_lcd_i8080_write_data8(xy_lcd_i8080_device_t *lcd, uint8_t data);

/**
 * @brief Send 16-bit data to LCD
 * @param lcd LCD device
 * @param data Data halfword
 */
void xy_lcd_i8080_write_data16(xy_lcd_i8080_device_t *lcd, uint16_t data);

/**
 * @brief Write register
 * @param lcd LCD device
 * @param reg Register address
 * @param data Data to write
 */
void xy_lcd_i8080_write_reg(xy_lcd_i8080_device_t *lcd, uint8_t reg, uint16_t data);

/**
 * @brief Read from LCD
 * @param lcd LCD device
 * @param data Data buffer
 * @param len Read length
 */
uint16_t xy_lcd_i8080_read_data(xy_lcd_i8080_device_t *lcd);

/**
 * @brief Set window to write
 * @param lcd LCD device
 * @param x X start
 * @param y Y start
 * @param w Width
 * @param h Height
 */
void xy_lcd_i8080_set_window(xy_lcd_i8080_device_t *lcd, uint16_t x, uint16_t y,
                             uint16_t w, uint16_t h);

/**
 * @brief Write pixel data to window
 * @param lcd LCD device
 * @param data Pixel data
 * @param len Data length (in pixels)
 */
void xy_lcd_i8080_write_pixel(xy_lcd_i8080_device_t *lcd, const uint16_t *data, uint32_t len);

/**
 * @brief Reset LCD
 * @param lcd LCD device
 */
void xy_lcd_i8080_reset(xy_lcd_i8080_device_t *lcd);

/**
 * @brief Set backlight level
 * @param lcd LCD device
 * @param brightness Brightness 0-100
 */
void xy_lcd_i8080_set_backlight(xy_lcd_i8080_device_t *lcd, uint8_t brightness);

/**
 * @brief Power on/off LCD
 * @param lcd LCD device
 * @param on Power state
 */
void xy_lcd_i8080_power(xy_lcd_i8080_device_t *lcd, bool on);

/* ==================== Driver-Level Operations ==================== */

/**
 * @brief I8080 LCD operations (for registration)
 */
extern const xy_lcd_ops_t xy_lcd_i8080_ops;

#ifdef __cplusplus
}
#endif

#endif /* XY_LCD_I8080_H */
