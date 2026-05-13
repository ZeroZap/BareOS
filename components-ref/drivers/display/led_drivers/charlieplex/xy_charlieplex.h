/**
 * @file xy_mono_led.h
 * @brief Monochrome LED Driver - Single Color LED Control
 * @version 1.0.0
 * @date 2026-03-02
 */

#ifndef XY_MONO_LED_H
#define XY_MONO_LED_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 配置 ==================== */

#ifndef XY_MONO_LED_MAX_CHANNELS
#define XY_MONO_LED_MAX_CHANNELS    16      // 最大通道数
#endif

/* ==================== 数据结构 ==================== */

/**
 * @brief LED 通道配置
 */
typedef struct {
    uint8_t pin;            // GPIO 引脚
    bool active_high;       // 高电平有效
    bool pwm_enable;        // 启用 PWM
} xy_mono_led_config_t;

/**
 * @brief LED 通道
 */
typedef struct {
    xy_mono_led_config_t config;
    uint8_t brightness;     // 亮度 (0-255)
    bool enabled;           // 使能
    bool state;             // 当前状态
} xy_mono_led_t;

/**
 * @brief LED 管理器
 */
typedef struct {
    xy_mono_led_t channels[XY_MONO_LED_MAX_CHANNELS];
    uint8_t num_channels;
    void (*gpio_write)(uint8_t pin, bool value);
    void (*pwm_set)(uint8_t pin, uint8_t duty);
} xy_mono_led_manager_t;

/* ==================== 基础 API ==================== */

/**
 * @brief 初始化 LED 管理器
 */
int xy_mono_led_init(xy_mono_led_manager_t *mgr,
                     void (*gpio_write)(uint8_t, bool),
                     void (*pwm_set)(uint8_t, uint8_t));

/**
 * @brief 创建 LED 通道
 */
int xy_mono_led_create(xy_mono_led_manager_t *mgr,
                       xy_mono_led_config_t *config);

/**
 * @brief 删除 LED 通道
 */
int xy_mono_led_delete(xy_mono_led_manager_t *mgr, uint8_t channel);

/**
 * @brief 设置 LED 开关
 */
void xy_mono_led_set(xy_mono_led_manager_t *mgr,
                     uint8_t channel, bool on);

/**
 * @brief 设置 LED 亮度
 */
void xy_mono_led_set_brightness(xy_mono_led_manager_t *mgr,
                                uint8_t channel, uint8_t brightness);

/**
 * @brief 切换 LED 状态
 */
void xy_mono_led_toggle(xy_mono_led_manager_t *mgr, uint8_t channel);

/**
 * @brief 获取 LED 状态
 */
bool xy_mono_led_get(xy_mono_led_manager_t *mgr, uint8_t channel);

/* ==================== 效果 API ==================== */

/**
 * @brief 呼吸灯效果
 */
void xy_mono_led_effect_breath(xy_mono_led_manager_t *mgr,
                               uint8_t channel,
                               uint16_t period_ms);

/**
 * @brief 闪烁效果
 */
void xy_mono_led_effect_blink(xy_mono_led_manager_t *mgr,
                              uint8_t channel,
                              uint16_t interval_ms);

/**
 * @brief 渐变效果
 */
void xy_mono_led_effect_fade(xy_mono_led_manager_t *mgr,
                             uint8_t channel,
                             uint8_t target_brightness,
                             uint16_t duration_ms);

/**
 * @brief 随机闪烁
 */
void xy_mono_led_effect_twinkle(xy_mono_led_manager_t *mgr,
                                uint8_t channel,
                                uint8_t density);

/**
 * @brief 更新所有效果
 */
void xy_mono_led_update_effects(xy_mono_led_manager_t *mgr);

#ifdef __cplusplus
}
#endif

#endif /* XY_MONO_LED_H */
