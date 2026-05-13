/**
 * @file xy_lcd_spi.h
 * @brief LCD SPI Interface Driver Header
 * @version 1.0.0
 * @date 2026-03-05
 */

#ifndef XY_LCD_SPI_H
#define XY_LCD_SPI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_lcd.h"
#include "xy_hal_spi.h"
#include "xy_hal_gpio.h"
#include <stdint.h>
#include <stdbool.h>

/* ==================== SPI LCD Configuration ==================== */

/**
 * @brief SPI LCD configuration
 */
typedef struct {
    /* Common config */
    xy_lcd_config_t base;

    /* SPI settings */
    void *spi_handle;           /**< SPI handle */
    uint32_t spi_speed;         /**< SPI speed in Hz */
    uint8_t spi_mode;           /**< SPI mode (0-3) */

    /* GPIO pins */
    uint8_t dc_pin;              /**< Data/Command pin */
    uint8_t cs_pin;              /**< Chip select pin */
    uint8_t rst_pin;             /**< Reset pin */
    uint8_t bl_pin;              /**< Backlight pin */

    /* Additional settings */
    bool use_dma;                /**< Use DMA for transfers */
    bool dc_is_on_data;         /**< DC pin state for data (1=data, 0=cmd) */
} xy_lcd_spi_config_t;

/**
 * @brief SPI LCD device structure
 */
typedef struct {
    xy_lcd_device_t base;       /**< Base LCD device */

    /* SPI interface */
    void *spi_handle;            /**< SPI handle */
    uint32_t spi_speed;          /**< SPI speed */
    uint8_t spi_mode;            /**< SPI mode */

    /* GPIO pins */
    uint8_t dc_pin;              /**< DC pin */
    uint8_t cs_pin;              /**< CS pin */
    uint8_t rst_pin;             /**< Reset pin */
    uint8_t bl_pin;              /**< Backlight pin */
    bool bl_inverted;           /**< Backlight inverted */

    /* DMA */
    bool use_dma;                /**< Use DMA */

    /* Current state */
    bool initialized;            /**< Initialization flag */
} xy_lcd_spi_device_t;

/* ==================== SPI LCD API ==================== */

/**
 * @brief Initialize SPI LCD device
 * @param lcd LCD device
 * @param config Configuration
 * @return XY_ERR_OK on success
 */
xy_error_t xy_lcd_spi_init(xy_lcd_spi_device_t *lcd, const xy_lcd_spi_config_t *config);

/**
 * @brief Deinitialize SPI LCD device
 * @param lcd LCD device
 * @return XY_ERR_OK on success
 */
xy_error_t xy_lcd_spi_deinit(xy_lcd_spi_device_t *lcd);

/**
 * @brief Send command to LCD via SPI
 * @param lcd LCD device
 * @param cmd Command byte
 */
void xy_lcd_spi_write_cmd(xy_lcd_spi_device_t *lcd, uint8_t cmd);

/**
 * @brief Send data to LCD via SPI
 * @param lcd LCD device
 * @param data Data buffer
 * @param len Data length
 */
void xy_lcd_spi_write_data(xy_lcd_spi_device_t *lcd, const uint8_t *data, uint32_t len);

/**
 * @brief Send data to LCD via SPI (8-bit)
 * @param lcd LCD device
 * @param data Data byte
 */
void xy_lcd_spi_write_data8(xy_lcd_spi_device_t *lcd, uint8_t data);

/**
 * @brief Send data to LCD via SPI (16-bit)
 * @param lcd LCD device
 * @param data Data halfword
 */
void xy_lcd_spi_write_data16(xy_lcd_spi_device_t *lcd, uint16_t data);

/**
 * @brief Write register via SPI
 * @param lcd LCD device
 * @param reg Register address
 * @param data Data to write
 */
void xy_lcd_spi_write_reg(xy_lcd_spi_device_t *lcd, uint8_t reg, uint8_t data);

/**
 * @brief Read from LCD via SPI
 * @param lcd LCD device
 * @param reg Register to read (NULL for data only)
 * @param data Data buffer
 * @param len Read length
 */
void xy_lcd_spi_read(xy_lcd_spi_device_t *lcd, uint8_t *reg, uint8_t *data, uint32_t len);

/**
 * @brief Set window to write
 * @param lcd LCD device
 * @param x X start
 * @param y Y start
 * @param w Width
 * @param h Height
 */
void xy_lcd_spi_set_window(xy_lcd_spi_device_t *lcd, uint16_t x, uint16_t y,
                           uint16_t w, uint16_t h);

/**
 * @brief Write pixel data to window
 * @param lcd LCD device
 * @param data Pixel data
 * @param len Data length
 */
void xy_lcd_spi_write_pixel(xy_lcd_spi_device_t *lcd, const uint16_t *data, uint32_t len);

/**
 * @brief Reset LCD
 * @param lcd LCD device
 */
void xy_lcd_spi_reset(xy_lcd_spi_device_t *lcd);

/**
 * @brief Set backlight level
 * @param lcd LCD device
 * @param brightness Brightness 0-100
 */
void xy_lcd_spi_set_backlight(xy_lcd_spi_device_t *lcd, uint8_t brightness);

/**
 * @brief Power on/off LCD
 * @param lcd LCD device
 * @param on Power state
 */
void xy_lcd_spi_power(xy_lcd_spi_device_t *lcd, bool on);

/* ==================== Driver-Level Operations ==================== */

/**
 * @brief SPI LCD operations (for registration)
 */
extern const xy_lcd_ops_t xy_lcd_spi_ops;

#ifdef __cplusplus
}
#endif

#endif /* XY_LCD_SPI_H */
