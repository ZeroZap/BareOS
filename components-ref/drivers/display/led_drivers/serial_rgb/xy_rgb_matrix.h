/**
 * @file xy_rgb_matrix.h
 * @brief RGB LED Matrix Driver Header
 * @version 1.0.0
 * @date 2026-03-02
 */

#ifndef XY_RGB_MATRIX_H
#define XY_RGB_MATRIX_H

#include <stdint.h>
#include <stdbool.h>
#include "xy_ws2812.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Matrix Configuration ==================== */

/**
 * @brief Common LED matrix sizes
 */
typedef enum {
    XY_RGB_MATRIX_8x8 = 0,       /**< 8x8 matrix (64 LEDs) */
    XY_RGB_MATRIX_16x16,         /**< 16x16 matrix (256 LEDs) */
    XY_RGB_MATRIX_32x8,          /**< 32x8 matrix (256 LEDs) */
    XY_RGB_MATRIX_64x64,         /**< 64x64 matrix (4096 LEDs) */
    XY_RGB_MATRIX_CUSTOM,        /**< Custom size */
} xy_rgb_matrix_size_t;

/**
 * @brief Matrix layout orientation
 */
typedef enum {
    XY_RGB_MATRIX_LAYOUT_ZIGZAG = 0,  /**< Alternating row direction */
    XY_RGB_MATRIX_LAYOUT_LINEAR,       /**< Linear left-to-right */
    XY_RGB_MATRIX_LAYOUT_SNAKE,        /**< Snake pattern */
} xy_rgb_matrix_layout_t;

/**
 * @brief LED matrix configuration
 */
typedef struct {
    uint16_t width;                     /**< Matrix width (columns) */
    uint16_t height;                    /**< Matrix height (rows) */
    xy_rgb_matrix_layout_t layout;      /**< LED arrangement pattern */
    xy_ws2812_color_order_t color_order; /**< Color byte order */
    uint8_t brightness;                 /**< Global brightness (0-255) */
} xy_rgb_matrix_config_t;

/* ==================== Handle Structure ==================== */

/**
 * @brief RGB LED Matrix handle
 */
typedef struct {
    xy_ws2812_handle_t ws2812;         /**< WS2812 driver handle */
    xy_rgb_matrix_config_t config;      /**< Matrix configuration */
    bool initialized;                   /**< Initialization flag */
} xy_rgb_matrix_handle_t;

/* ==================== Error Codes ==================== */

typedef enum {
    XY_RGB_MATRIX_OK = 0,
    XY_RGB_MATRIX_ERROR,
    XY_RGB_MATRIX_ERROR_INVALID_PARAM,
    XY_RGB_MATRIX_ERROR_NOT_INITIALIZED,
} xy_rgb_matrix_error_t;

/* ==================== Effect Types ==================== */

/**
 * @brief Matrix effect types
 */
typedef enum {
    XY_RGB_MATRIX_EFFECT_SOLID = 0,     /**< Solid single color */
    XY_RGB_MATRIX_EFFECT_RAINBOW,       /**< Rainbow cycling */
    XY_RGB_MATRIX_EFFECT_BREATHING,      /**< Breathing effect */
    XY_RGB_MATRIX_EFFECT_FLASH,          /**< Flash/strobe effect */
    XY_RGB_MATRIX_EFFECT_COLOR_WIPE,    /**< Color wipe transition */
    XY_RGB_MATRIX_EFFECT_COUNT,
} xy_rgb_matrix_effect_t;

/* ==================== Default Configurations ==================== */

#define RGB_MATRIX_DEFAULT_WIDTH    8
#define RGB_MATRIX_DEFAULT_HEIGHT   8
#define RGB_MATRIX_DEFAULT_BRIGHT   128

/* ==================== API Functions ==================== */

/**
 * @brief Initialize RGB LED Matrix
 * @param handle Matrix handle
 * @param config Matrix configuration
 * @param ws2812_config WS2812 configuration
 * @return Matrix error code
 */
xy_rgb_matrix_error_t xy_rgb_matrix_init(xy_rgb_matrix_handle_t *handle,
                                         xy_rgb_matrix_config_t *config,
                                         xy_ws2812_config_t *ws2812_config);

/**
 * @brief Deinitialize RGB LED Matrix
 * @param handle Matrix handle
 */
void xy_rgb_matrix_deinit(xy_rgb_matrix_handle_t *handle);

/**
 * @brief Set pixel at (x, y) coordinates
 * @param handle Matrix handle
 * @param x X coordinate (0 = left)
 * @param y Y coordinate (0 = top)
 * @param color RGB color
 */
void xy_rgb_matrix_set_pixel(xy_rgb_matrix_handle_t *handle,
                             uint16_t x, uint16_t y,
                             xy_ws2812_color_t color);

/**
 * @brief Get pixel at (x, y) coordinates
 * @param handle Matrix handle
 * @param x X coordinate
 * @param y Y coordinate
 * @return RGB color at position
 */
xy_ws2812_color_t xy_rgb_matrix_get_pixel(xy_rgb_matrix_handle_t *handle,
                                           uint16_t x, uint16_t y);

/**
 * @brief Fill entire matrix with a color
 * @param handle Matrix handle
 * @param color RGB color
 */
void xy_rgb_matrix_fill(xy_rgb_matrix_handle_t *handle, xy_ws2812_color_t color);

/**
 * @brief Clear all LEDs (set to black)
 * @param handle Matrix handle
 */
void xy_rgb_matrix_clear(xy_rgb_matrix_handle_t *handle);

/**
 * @brief Set matrix brightness
 * @param handle Matrix handle
 * @param brightness Brightness (0-255)
 */
void xy_rgb_matrix_set_brightness(xy_rgb_matrix_handle_t *handle,
                                   uint8_t brightness);

/**
 * @brief Get matrix brightness
 * @param handle Matrix handle
 * @return Current brightness
 */
uint8_t xy_rgb_matrix_get_brightness(xy_rgb_matrix_handle_t *handle);

/**
 * @brief Update matrix display
 * @param handle Matrix handle
 * @note Call after making changes to send data to LEDs
 */
void xy_rgb_matrix_show(xy_rgb_matrix_handle_t *handle);

/**
 * @brief Set a horizontal line
 * @param handle Matrix handle
 * @param y Y coordinate
 * @param color RGB color
 */
void xy_rgb_matrix_draw_hline(xy_rgb_matrix_handle_t *handle,
                               uint16_t y,
                               xy_ws2812_color_t color);

/**
 * @brief Set a vertical line
 * @param handle Matrix handle
 * @param x X coordinate
 * @param color RGB color
 */
void xy_rgb_matrix_draw_vline(xy_rgb_matrix_handle_t *handle,
                               uint16_t x,
                               xy_ws2812_color_t color);

/**
 * @brief Draw a rectangle
 * @param handle Matrix handle
 * @param x1 Top-left X
 * @param y1 Top-left Y
 * @param x2 Bottom-right X
 * @param y2 Bottom-right Y
 * @param color RGB color
 * @param fill Fill rectangle if true
 */
void xy_rgb_matrix_draw_rect(xy_rgb_matrix_handle_t *handle,
                              uint16_t x1, uint16_t y1,
                              uint16_t x2, uint16_t y2,
                              xy_ws2812_color_t color,
                              bool fill);

/* ==================== Effects API ==================== */

/**
 * @brief Set matrix effect
 * @param handle Matrix handle
 * @param effect Effect type
 * @param speed Effect speed (0-255, 0=fast, 255=slow)
 * @param color Effect primary color
 */
void xy_rgb_matrix_set_effect(xy_rgb_matrix_handle_t *handle,
                               xy_rgb_matrix_effect_t effect,
                               uint8_t speed,
                               xy_ws2812_color_t color);

/**
 * @brief Update matrix effect (call in main loop)
 * @param handle Matrix handle
 * @return true if display should be updated
 */
bool xy_rgb_matrix_update_effect(xy_rgb_matrix_handle_t *handle);

/**
 * @brief Apply breathing effect
 * @param handle Matrix handle
 * @param progress Progress (0-255)
 */
void xy_rgb_matrix_effect_breathing(xy_rgb_matrix_handle_t *handle,
                                     uint8_t progress);

/**
 * @brief Apply rainbow effect to entire matrix
 * @param handle Matrix handle
 * @param hue Hue value (0-255)
 */
void xy_rgb_matrix_effect_rainbow(xy_rgb_matrix_handle_t *handle, uint8_t hue);

/**
 * @brief Apply solid color effect
 * @param handle Matrix handle
 * @param color Solid color
 */
void xy_rgb_matrix_effect_solid(xy_rgb_matrix_handle_t *handle,
                                 xy_ws2812_color_t color);

/* ==================== Utility Functions ==================== */

/**
 * @brief Get LED index from x,y coordinates
 * @param handle Matrix handle
 * @param x X coordinate
 * @param y Y coordinate
 * @return LED index in strip
 */
uint16_t xy_rgb_matrix_xy_to_index(xy_rgb_matrix_handle_t *handle,
                                    uint16_t x, uint16_t y);

/**
 * @brief Get coordinates from LED index
 * @param handle Matrix handle
 * @param index LED index
 * @param x Pointer to store X coordinate
 * @param y Pointer to store Y coordinate
 */
void xy_rgb_matrix_index_to_xy(xy_rgb_matrix_handle_t *handle,
                                uint16_t index,
                                uint16_t *x, uint16_t *y);

#ifdef __cplusplus
}
#endif

#endif /* XY_RGB_MATRIX_H */
