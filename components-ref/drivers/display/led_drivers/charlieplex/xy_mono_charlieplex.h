/**
 * @file xy_mono_charlieplex.h
 * @brief Charlieplexing Driver for Monochrome LEDs
 * @version 1.0.0
 * @date 2026-03-02
 * 
 * 查理复用技术：N 个 IO 控制 N*(N-1) 个 LED
 * 
 * IO 数    LED 数
 * 2        2
 * 3        6
 * 4        12
 * 5        20
 * 6        30
 * 8        56
 */

#ifndef XY_MONO_CHARLIEPLEX_H
#define XY_MONO_CHARLIEPLEX_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 配置 ==================== */

#ifndef CHARLIE_IO_NUM
#define CHARLIE_IO_NUM      4           // 4 IO = 12 LED
#endif

#define CHARLIE_LED_NUM     (CHARLIE_IO_NUM * (CHARLIE_IO_NUM - 1))

/* ==================== 数据结构 ==================== */

/**
 * @brief IO 状态
 */
typedef enum {
    CHARLIE_IO_HIZ = 0,     // 高阻态
    CHARLIE_IO_LOW,         // 低电平
    CHARLIE_IO_HIGH,        // 高电平
    CHARLIE_IO_PWM,         // PWM 输出
} xy_charlie_io_state_t;

/**
 * @brief LED 连接定义
 */
typedef struct {
    uint8_t anode_pin;      // 阳极 IO
    uint8_t cathode_pin;    // 阴极 IO
} xy_charlie_led_conn_t;

/**
 * @brief 查理复用管理器
 */
typedef struct {
    xy_charlie_led_conn_t leds[CHARLIE_LED_NUM];    // LED 连接映射
    uint8_t brightness[CHARLIE_LED_NUM];            // 亮度缓冲区
    uint8_t current_led;                            // 当前扫描 LED
    uint8_t io_pins[CHARLIE_IO_NUM];                // IO 引脚
    bool enabled;                                   // 使能
    
    // 硬件回调
    void (*set_io_state)(uint8_t pin, xy_charlie_io_state_t state);
    void (*set_pwm_duty)(uint8_t pin, uint8_t duty);
} xy_charlieplex_t;

/* ==================== 基础 API ==================== */

/**
 * @brief 初始化查理复用
 */
int xy_charlieplex_init(xy_charlieplex_t *ctx,
                        uint8_t *io_pins,
                        void (*set_io_state)(uint8_t, xy_charlie_io_state_t),
                        void (*set_pwm_duty)(uint8_t, uint8_t));

/**
 * @brief 设置 LED 亮度
 */
void xy_charlieplex_set_brightness(xy_charlieplex_t *ctx,
                                   uint8_t led_index,
                                   uint8_t brightness);

/**
 * @brief 设置所有 LED 亮度
 */
void xy_charlieplex_set_all(xy_charlieplex_t *ctx, uint8_t brightness);

/**
 * @brief 清除所有 LED
 */
void xy_charlieplex_clear(xy_charlieplex_t *ctx);

/**
 * @brief 扫描服务 (在中断或主循环调用)
 */
void xy_charlieplex_scan(xy_charlieplex_t *ctx);

/**
 * @brief 使能/禁用
 */
void xy_charlieplex_enable(xy_charlieplex_t *ctx, bool enable);

/**
 * @brief 获取 LED 数量
 */
uint16_t xy_charlieplex_get_led_count(xy_charlieplex_t *ctx);

/* ==================== 效果 API ==================== */

/**
 * @brief 呼吸灯效果参数
 */
typedef struct {
    uint8_t led_index;
    uint16_t period_ms;
    uint8_t min_brightness;
    uint8_t max_brightness;
    uint32_t last_tick;
    int16_t current_brightness;
    int8_t step;
    bool active;
} xy_charlie_breath_t;

/**
 * @brief 启动呼吸灯
 */
int xy_charlieplex_effect_breath_start(xy_charlieplex_t *ctx,
                                       uint8_t led_index,
                                       uint16_t period_ms,
                                       uint8_t min_br,
                                       uint8_t max_br);

/**
 * @brief 停止呼吸灯
 */
void xy_charlieplex_effect_breath_stop(xy_charlieplex_t *ctx, uint8_t led_index);

/**
 * @brief 流水灯效果
 */
void xy_charlieplex_effect_marquee(xy_charlieplex_t *ctx,
                                   uint8_t speed,
                                   bool reverse);

/**
 * @brief 随机闪烁效果
 */
void xy_charlieplex_effect_twinkle(xy_charlieplex_t *ctx, uint8_t density);

/**
 * @brief 更新所有效果
 */
void xy_charlieplex_update_effects(xy_charlieplex_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* XY_MONO_CHARLIEPLEX_H */
