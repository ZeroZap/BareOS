/**
 * @file xy_led_driver.h
 * @brief LED Driver Independent Interface - Dual Mode Support
 * @version 1.0.0
 * @date 2026-03-02
 * 
 * LED 驱动独立接口 - 支持独立运行和 GUI 集成双模式
 */

#ifndef XY_LED_DRIVER_H
#define XY_LED_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== LED 驱动接口 (独立模式) ==================== */

/**
 * @brief LED 驱动接口
 */
typedef struct {
    /* 设备信息 */
    uint16_t width;
    uint16_t height;
    uint8_t bpp;              // 每像素位数
    
    /* 硬件句柄 */
    void *hw_handle;
    
    /* 基础 API (必须实现) */
    void (*set_pixel)(uint16_t x, uint16_t y, uint32_t color);
    uint32_t (*get_pixel)(uint16_t x, uint16_t y);
    void (*fill)(uint32_t color);
    void (*show)(void);
    
    /* 效果 API (可选实现) */
    void (*effect_breath)(uint8_t led, uint16_t period);
    void (*effect_blink)(uint8_t led, uint16_t interval);
    void (*effect_rainbow)(uint16_t speed);
    void (*effect_scroll_text)(const char *text, uint8_t speed);
    void (*effect_custom)(void *params);
    
    /* 用户数据 */
    void *user_data;
} xy_led_driver_t;

/* ==================== GUI 接口 (可选模式) ==================== */

#include "xy_gui_display.h"

/**
 * @brief 获取 GUI 显示接口
 * @param drv LED 驱动
 * @return GUI 接口指针，NULL 表示不支持 GUI
 */
typedef xy_gui_display_t* (*xy_led_get_gui_func_t)(xy_led_driver_t *drv);

/* ==================== 宏定义 ==================== */

/**
 * @brief 检查驱动是否支持 GUI
 */
#define XY_LED_SUPPORTS_GUI(drv) \
    ((drv) && ((xy_led_driver_t*)(drv))->user_data != NULL)

/**
 * @brief 获取 GUI 接口 (如果支持)
 */
#define XY_LED_GET_GUI(drv) \
    (XY_LED_SUPPORTS_GUI(drv) ? xy_led_get_gui_interface(drv) : NULL)

/* ==================== 工具函数 ==================== */

/**
 * @brief 注册 LED 驱动到 GUI
 * @param drv LED 驱动
 * @return 0 成功
 */
int xy_led_register_gui(xy_led_driver_t *drv);

/**
 * @brief 获取 GUI 接口
 * @param drv LED 驱动
 * @return GUI 接口或 NULL
 */
xy_gui_display_t* xy_led_get_gui_interface(xy_led_driver_t *drv);

/**
 * @brief 启用/禁用 GUI 模式
 * @param drv LED 驱动
 * @param enable true=启用 GUI
 */
void xy_led_enable_gui(xy_led_driver_t *drv, bool enable);

/**
 * @brief 检查是否启用 GUI
 * @param drv LED 驱动
 * @return true=GUI 模式
 */
bool xy_led_is_gui_enabled(xy_led_driver_t *drv);

#ifdef __cplusplus
}
#endif

#endif /* XY_LED_DRIVER_H */
