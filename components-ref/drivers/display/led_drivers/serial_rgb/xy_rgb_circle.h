/**
 * @file xy_rgb_circle.h
 * @brief RGB LED Circle Mode (Circular Display)
 * @version 1.0.0
 * @date 2026-03-02
 */

#ifndef XY_RGB_CIRCLE_H
#define XY_RGB_CIRCLE_H

#include "xy_rgb.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 圆形配置
 */
typedef struct {
    uint16_t num_leds;        /* LED 数量 */
    uint8_t center_x;         /* 圆心 X (虚拟坐标) */
    uint8_t center_y;         /* 圆心 Y (虚拟坐标) */
    uint8_t radius;           /* 半径 (虚拟坐标) */
} xy_rgb_circle_config_t;

/**
 * @brief 初始化 Circle 模式
 * @param config 配置
 * @return XY_RGB_OK 成功，其他值失败
 */
int32_t xy_rgb_circle_init(xy_rgb_circle_config_t *config);

/**
 * @brief 设置角度颜色
 * @param angle 角度 (0-255, 0=0°, 255=360°)
 * @param color 颜色
 */
void xy_rgb_circle_set_angle(uint8_t angle, rgb_color_t color);

/**
 * @brief 获取角度颜色
 * @param angle 角度
 * @return 颜色
 */
rgb_color_t xy_rgb_circle_get_angle(uint8_t angle);

/**
 * @brief 绘制圆弧
 * @param start_angle 起始角度
 * @param end_angle 结束角度
 * @param color 颜色
 */
void xy_rgb_circle_draw_arc(uint8_t start_angle, uint8_t end_angle, 
                            rgb_color_t color);

/**
 * @brief 绘制扇形
 * @param start_angle 起始角度
 * @param end_angle 结束角度
 * @param color 颜色
 */
void xy_rgb_circle_draw_sector(uint8_t start_angle, uint8_t end_angle, 
                               rgb_color_t color);

/**
 * @brief 清除所有 LED
 */
void xy_rgb_circle_clear(void);

/* ==================== Circle 效果 ==================== */

/**
 * @brief 时钟效果
 * @param hour 时 (0-23)
 * @param minute 分 (0-59)
 * @param second 秒 (0-59)
 */
void xy_rgb_circle_fx_clock(uint8_t hour, uint8_t minute, uint8_t second);

/**
 * @brief 仪表效果
 * @param value 值 (0-255)
 * @param color 颜色
 */
void xy_rgb_circle_fx_gauge(uint8_t value, rgb_color_t color);

/**
 * @brief 音量表效果
 * @param level 音量级别 (0-255)
 * @param color_low 低音量颜色
 * @param color_mid 中音量颜色
 * @param color_high 高音量颜色
 */
void xy_rgb_circle_fx_vu_meter(uint8_t level, 
                               rgb_color_t color_low,
                               rgb_color_t color_mid,
                               rgb_color_t color_high);

/**
 * @brief 螺旋效果
 * @param speed 速度
 * @param color 颜色
 */
void xy_rgb_circle_fx_spiral(uint16_t speed, rgb_color_t color);

/**
 * @brief 旋转效果
 * @param speed 速度
 * @param color 颜色
 */
void xy_rgb_circle_fx_rotate(uint16_t speed, rgb_color_t color);

/**
 * @brief 脉冲效果
 * @param speed 速度
 * @param color 颜色
 */
void xy_rgb_circle_fx_pulse(uint16_t speed, rgb_color_t color);

/* ==================== 工具函数 ==================== */

/**
 * @brief 角度转坐标
 * @param angle 角度 (0-255)
 * @param radius 半径
 * @return 坐标 (x, y)
 */
point2d_t xy_rgb_circle_angle_to_coord(uint8_t angle, uint8_t radius);

/**
 * @brief 坐标转角度
 * @param x X 坐标
 * @param y Y 坐标
 * @return 角度 (0-255)
 */
uint8_t xy_rgb_circle_coord_to_angle(int8_t x, int8_t y);

#ifdef __cplusplus
}
#endif

#endif /* XY_RGB_CIRCLE_H */
