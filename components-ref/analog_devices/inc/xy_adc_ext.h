/**
 * @file xy_adc_ext.h
 * @brief External ADC Chip Drivers
 * @version 1.0.0
 * @date 2026-03-02
 */

#ifndef XY_ADC_EXT_H
#define XY_ADC_EXT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== ADS1115 ==================== */
#include "xy_ads1115.h"

/* ==================== ADS1015 (12-bit ADS1115) ==================== */

/**
 * @brief ADS1015 句柄
 */
typedef struct {
    void *i2c_handle;
    uint8_t addr;
    uint8_t gain;
    uint8_t rate;
} xy_ads1015_t;

int xy_ads1015_init(xy_ads1015_t *ads, void *i2c_handle, uint8_t addr);
int xy_ads1015_read(xy_ads1015_t *ads, uint8_t channel, int16_t *value);

/* ==================== ADS1118 (SPI, 16-bit) ==================== */

/**
 * @brief ADS1118 句柄
 */
typedef struct {
    void *spi_handle;
    uint8_t cs_pin;
    uint8_t mode;
} xy_ads1118_t;

int xy_ads1118_init(xy_ads1118_t *ads, void *spi_handle, uint8_t cs_pin);
int xy_ads1118_read(xy_ads1118_t *ads, uint8_t channel, int16_t *value);

/* ==================== ADS1248 (24-bit, SPI) ==================== */

/**
 * @brief ADS1248 句柄
 */
typedef struct {
    void *spi_handle;
    uint8_t cs_pin;
    uint8_t drdy_pin;
    uint8_t rate;
} xy_ads1248_t;

int xy_ads1248_init(xy_ads1248_t *ads, void *spi_handle, uint8_t cs_pin, uint8_t drdy_pin);
int xy_ads1248_read(xy_ads1248_t *ads, int32_t *value);

/* ==================== MAX11100 (16-bit, SPI) ==================== */

typedef struct {
    void *spi_handle;
    uint8_t cs_pin;
} xy_max11100_t;

int xy_max11100_init(xy_max11100_t *max, void *spi_handle, uint8_t cs_pin);
int xy_max11100_read(xy_max11100_t *max, uint16_t *value);

/* ==================== MCP3008 (10-bit, 8-channel, SPI) ==================== */

typedef struct {
    void *spi_handle;
    uint8_t cs_pin;
} xy_mcp3008_t;

int xy_mcp3008_init(xy_mcp3008_t *mcp, void *spi_handle, uint8_t cs_pin);
int xy_mcp3008_read(xy_mcp3008_t *mcp, uint8_t channel, uint16_t *value);

/* ==================== MCP3208 (12-bit, 8-channel, SPI) ==================== */

typedef struct {
    void *spi_handle;
    uint8_t cs_pin;
} xy_mcp3208_t;

int xy_mcp3208_init(xy_mcp3208_t *mcp, void *spi_handle, uint8_t cs_pin);
int xy_mcp3208_read(xy_mcp3208_t *mcp, uint8_t channel, uint16_t *value);

/* ==================== HX711 (24-bit, 称重传感器) ==================== */

typedef struct {
    uint8_t pd_sck_pin;
    uint8_t dout_pin;
    uint8_t gain;
    int32_t offset;
    float scale;
} xy_hx711_t;

int xy_hx711_init(xy_hx711_t *hx, uint8_t pd_sck_pin, uint8_t dout_pin);
int xy_hx711_read(xy_hx711_t *hx, int32_t *value);
int xy_hx711_tare(xy_hx711_t *hx);
int xy_hx711_calibrate(xy_hx711_t *hx, float known_weight);
float xy_hx711_get_weight(xy_hx711_t *hx);

#ifdef __cplusplus
}
#endif

#endif
