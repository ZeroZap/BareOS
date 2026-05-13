/**
 * @file xy_rgb_drv.h
 * @brief RGB LED Driver Interface
 * @version 1.0.0
 * @date 2026-03-02
 */

#ifndef XY_RGB_DRV_H
#define XY_RGB_DRV_H

#include <stdint.h>
#include <stdbool.h>
#include "xy_rgb.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 驱动接口
 */
typedef struct {
    int32_t (*init)(void *handle);
    int32_t (*deinit)(void *handle);
    int32_t (*send)(void *handle, rgb_color_t *leds, uint16_t num_leds);
} xy_rgb_drv_if_t;

/**
 * @brief GPIO 驱动配置
 */
typedef struct {
    uint8_t data_pin;     /* 数据引脚 */
    uint8_t color_order;  /* 颜色顺序：0=GRB, 1=RGB */
} xy_rgb_gpio_config_t;

/**
 * @brief SPI 驱动配置
 */
typedef struct {
    void *spi_handle;     /* SPI 句柄 */
    uint8_t color_order;  /* 颜色顺序 */
} xy_rgb_spi_config_t;

/**
 * @brief I2S 驱动配置
 */
typedef struct {
    void *i2s_handle;     /* I2S 句柄 */
    uint8_t color_order;  /* 颜色顺序 */
} xy_rgb_i2s_config_t;

/**
 * @brief 获取 GPIO 驱动
 */
const xy_rgb_drv_if_t* xy_rgb_drv_gpio(void);

/**
 * @brief 获取 SPI 驱动
 */
const xy_rgb_drv_if_t* xy_rgb_drv_spi(void);

/**
 * @brief 获取 I2S 驱动
 */
const xy_rgb_drv_if_t* xy_rgb_drv_i2s(void);

/**
 * @brief 颜色顺序
 */
#define XY_RGB_GRB    0
#define XY_RGB_RGB    1
#define XY_RGB_GBR    2
#define XY_RGB_BRG    3
#define XY_RGB_RBG    4
#define XY_RGB_BGR    5

#ifdef __cplusplus
}
#endif

#endif /* XY_RGB_DRV_H */
