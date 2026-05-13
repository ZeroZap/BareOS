/**
 * @file xy_ws2812.h
 * @brief WS2812/SK6812 RGB LED Driver Header
 * @version 1.0.0
 * @date 2026-03-02
 */

#ifndef XY_WS2812_H
#define XY_WS2812_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== WS2812 Timing Parameters ==================== */
/**
 * @brief WS2812 timing parameters for 800KHz data rate
 * @note All values in nanoseconds (ns)
 *
 * Timing diagram:
 *   T0H: 0.35us (280-450ns) - 0 bit high time
 *   T0L: 0.80us (550-900ns) - 0 bit low time
 *   T1H: 0.70us (550-950ns) - 1 bit high time
 *   T1L: 0.60us (450-850ns) - 1 bit low time
 *   RES: 50us minimum - Reset low time
 */

/* WS2812B timing at 800KHz (1.25us per bit) */
#define WS2812_T0H_NS          350     /**< 0 bit high time: 350ns */
#define WS2812_T0L_NS          800     /**< 0 bit low time: 800ns */
#define WS2812_T1H_NS          700     /**< 1 bit high time: 700ns */
#define WS2812_T1L_NS          600     /**< 1 bit low time: 600ns */
#define WS2812_RESET_NS        50000   /**< Reset low time: 50us minimum */

/* SK6812 timing (similar to WS2812) */
#define SK6812_T0H_NS          300     /**< SK6812 0 bit high time */
#define SK6812_T0L_NS          900     /**< SK6812 0 bit low time */
#define SK6812_T1H_NS          600     /**< SK6812 1 bit high time */
#define SK6812_T1L_NS          650     /**< SK6812 1 bit low time */

/* 800KHz bit period */
#define WS2812_BIT_PERIOD_NS   1250    /**< 1.25us period for 800KHz */

/* ==================== LED Types ==================== */

typedef enum {
    WS2812_TYPE_WS2812B = 0,   /**< WS2812B (GRB format) */
    WS2812_TYPE_WS2811,        /**< WS2811 (RGB format) */
    WS2812_TYPE_SK6812,        /**< SK6812 (GRB format) */
    WS2812_TYPE_WS2813,        /**< WS2813 (GRB format) */
    WS2812_TYPE_WS2815,        /**< WS2815 (GRB format, 12V) */
} xy_ws2812_type_t;

/* ==================== Color Order ==================== */

typedef enum {
    WS2812_COLOR_GRB = 0,      /**< Green-Red-Blue (WS2812B default) */
    WS2812_COLOR_RGB,          /**< Red-Green-Blue */
    WS2812_COLOR_GBR,          /**< Green-Blue-Red */
    WS2812_COLOR_RGBW,        /**< RGB with White channel */
} xy_ws2812_color_order_t;

/* ==================== LED Color Structure ==================== */

/**
 * @brief RGB color with alpha support
 */
typedef struct {
    uint8_t r;                 /**< Red component (0-255) */
    uint8_t g;                 /**< Green component (0-255) */
    uint8_t b;                 /**< Blue component (0-255) */
    uint8_t w;                 /**< White component (0-255, for RGBW LEDs) */
} xy_ws2812_color_t;

/* Predefined colors */
#define WS2812_COLOR_BLACK     {0, 0, 0, 0}
#define WS2812_COLOR_WHITE     {255, 255, 255, 0}
#define WS2812_COLOR_RED       {255, 0, 0, 0}
#define WS2812_COLOR_GREEN     {0, 255, 0, 0}
#define WS2812_COLOR_BLUE      {0, 0, 255, 0}
#define WS2812_COLOR_YELLOW    {255, 255, 0, 0}
#define WS2812_COLOR_CYAN      {0, 255, 255, 0}
#define WS2812_COLOR_MAGENTA   {255, 0, 255, 0}
#define WS2812_COLOR_ORANGE    {255, 128, 0, 0}
#define WS2812_COLOR_PURPLE    {128, 0, 255, 0}

/* ==================== Configuration ==================== */

/**
 * @brief WS2812 configuration structure
 */
typedef struct {
    uint16_t num_leds;                 /**< Number of LEDs in the strip */
    xy_ws2812_type_t led_type;         /**< LED type (WS2812B, SK6812, etc.) */
    uint8_t data_pin;                  /**< GPIO pin for data output */
    xy_ws2812_color_order_t color_order; /**< Color byte order */
    bool use_dma;                      /**< Use DMA for data transfer */
    uint8_t brightness;                /**< Global brightness (0-255) */
} xy_ws2812_config_t;

/* ==================== Handle Structure ==================== */

/**
 * @brief WS2812 driver handle
 */
typedef struct {
    xy_ws2812_config_t config;         /**< Configuration data */
    xy_ws2812_color_t *leds;           /**< LED color buffer */
    uint8_t *tx_buffer;                /**< TX buffer for DMA/Software */
    uint16_t buffer_size;              /**< TX buffer size in bytes */
    bool initialized;                   /**< Initialization flag */
    bool dirty;                        /**< Data changed flag */
} xy_ws2812_handle_t;

/* ==================== Error Codes ==================== */

typedef enum {
    WS2812_OK = 0,
    WS2812_ERROR,
    WS2812_ERROR_INVALID_PARAM,
    WS2812_ERROR_NO_MEMORY,
    WS2812_ERROR_NOT_INITIALIZED,
    WS2812_ERROR_NOT_SUPPORTED,
} xy_ws2812_error_t;

/* ==================== Default Configuration ==================== */

#define WS2812_DEFAULT_NUM_LEDS    8
#define WS2812_DEFAULT_PIN         0
#define WS2812_DEFAULT_BRIGHTNESS  128
#define WS2812_DEFAULT_COLOR_ORDER WS2812_COLOR_GRB

/* ==================== API Functions ==================== */

/**
 * @brief Initialize WS2812 driver
 * @param handle WS2812 driver handle
 * @param config Configuration structure
 * @return WS2812_OK on success, error code on failure
 */
xy_ws2812_error_t xy_ws2812_init(xy_ws2812_handle_t *handle,
                                  xy_ws2812_config_t *config);

/**
 * @brief Deinitialize WS2812 driver
 * @param handle WS2812 driver handle
 */
void xy_ws2812_deinit(xy_ws2812_handle_t *handle);

/**
 * @brief Set single LED color
 * @param handle WS2812 driver handle
 * @param index LED index (0-based)
 * @param color RGB color
 */
void xy_ws2812_set_pixel(xy_ws2812_handle_t *handle,
                         uint16_t index,
                         xy_ws2812_color_t color);

/**
 * @brief Get single LED color
 * @param handle WS2812 driver handle
 * @param index LED index (0-based)
 * @return RGB color at specified index
 */
xy_ws2812_color_t xy_ws2812_get_pixel(xy_ws2812_handle_t *handle,
                                       uint16_t index);

/**
 * @brief Fill all LEDs with a single color
 * @param handle WS2812 driver handle
 * @param color RGB color
 */
void xy_ws2812_fill(xy_ws2812_handle_t *handle, xy_ws2812_color_t color);

/**
 * @brief Clear all LEDs (set to black)
 * @param handle WS2812 driver handle
 */
void xy_ws2812_clear(xy_ws2812_handle_t *handle);

/**
 * @brief Set global brightness
 * @param handle WS2812 driver handle
 * @param brightness Brightness level (0-255)
 */
void xy_ws2812_set_brightness(xy_ws2812_handle_t *handle, uint8_t brightness);

/**
 * @brief Get global brightness
 * @param handle WS2812 driver handle
 * @return Current brightness level
 */
uint8_t xy_ws2812_get_brightness(xy_ws2812_handle_t *handle);

/**
 * @brief Update LED strip (send data to LEDs)
 * @param handle WS2812 driver handle
 * @note This function uses bit-banging or DMA to send data
 */
void xy_ws2812_show(xy_ws2812_handle_t *handle);

/**
 * @brief Set multiple LEDs from array
 * @param handle WS2812 driver handle
 * @param colors Array of colors
 * @param count Number of colors
 * @param start_index Starting LED index
 */
void xy_ws2812_set_pixels(xy_ws2812_handle_t *handle,
                          xy_ws2812_color_t *colors,
                          uint16_t count,
                          uint16_t start_index);

/* ==================== Color Conversion Utilities ==================== */

/**
 * @brief Convert HSV to RGB color
 * @param h Hue (0-255, maps to 0-360 degrees)
 * @param s Saturation (0-255, maps to 0-100%)
 * @param v Value/Brightness (0-255, maps to 0-100%)
 * @return RGB color structure
 */
xy_ws2812_color_t xy_ws2812_hsv_to_rgb(uint8_t h, uint8_t s, uint8_t v);

/**
 * @brief Convert RGB to HSV color
 * @param rgb RGB color structure
 * @param h Pointer to store hue value
 * @param s Pointer to store saturation value
 * @param v Pointer to store brightness value
 */
void xy_ws2812_rgb_to_hsv(xy_ws2812_color_t rgb, uint8_t *h, uint8_t *s, uint8_t *v);

/**
 * @brief Generate rainbow color based on position
 * @param hue Hue value (0-255)
 * @return RGB color
 */
xy_ws2812_color_t xy_ws2812_rainbow_color(uint8_t hue);

/**
 * @brief Blend two colors together
 * @param c1 First color
 * @param c2 Second color
 * @param factor Blend factor (0-255, 0=c1, 255=c2)
 * @return Blended color
 */
xy_ws2812_color_t xy_ws2812_color_blend(xy_ws2812_color_t c1,
                                        xy_ws2812_color_t c2,
                                        uint8_t factor);

/* ==================== Low-level GPIO Interface ==================== */
/**
 * @brief GPIO write callback type
 */
typedef void (*xy_ws2812_gpio_write_t)(uint8_t pin, uint8_t value);

/**
 * @brief Set GPIO write callback (for custom HAL integration)
 * @param callback GPIO write callback function
 */
void xy_ws2812_set_gpio_callback(xy_ws2812_gpio_write_t callback);

/**
 * @brief Software bit-banging send (fallback when DMA unavailable)
 * @param handle WS2812 driver handle
 */
void xy_ws2812_send_bitbang(xy_ws2812_handle_t *handle);

#ifdef __cplusplus
}
#endif

#endif /* XY_WS2812_H */
