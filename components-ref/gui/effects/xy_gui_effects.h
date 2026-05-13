/**
 * @file xy_gui_effects.h
 * @brief GUI Effects Header - GUI效果模块统一头文件
 * @version 1.0.0
 * @date 2026-03-13
 */

#ifndef XY_GUI_EFFECTS_H
#define XY_GUI_EFFECTS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 效果类型定义 ==================== */

/**
 * @brief 效果类型枚举
 */
typedef enum {
    XY_EFFECT_TYPE_NONE = 0,
    XY_EFFECT_TYPE_FADE,       /**< 淡入淡出效果 */
    XY_EFFECT_TYPE_BLINK,      /**< 闪烁效果 */
    XY_EFFECT_TYPE_BREATH,    /**< 呼吸灯效果 */
    XY_EFFECT_TYPE_SLIDE,     /**< 滑动效果 */
    XY_EFFECT_TYPE_ROTATE,    /**< 旋转效果 */
    XY_EFFECT_TYPE_ZOOM,      /**< 缩放效果 */
    XY_EFFECT_TYPE_SHAKE,     /**< 抖动效果 */
} xy_effect_type_t;

/**
 * @brief 效果状态
 */
typedef enum {
    XY_EFFECT_STATE_STOPPED = 0,
    XY_EFFECT_STATE_RUNNING,
    XY_EFFECT_STATE_PAUSED,
} xy_effect_state_t;

/**
 * @brief 效果方向
 */
typedef enum {
    XY_EFFECT_DIR_NONE = 0,
    XY_EFFECT_DIR_LEFT,
    XY_EFFECT_DIR_RIGHT,
    XY_EFFECT_DIR_UP,
    XY_EFFECT_DIR_DOWN,
} xy_effect_dir_t;

/* ==================== 基础效果结构 ==================== */

/**
 * @brief 基础效果结构
 */
typedef struct {
    xy_effect_type_t type;         /**< 效果类型 */
    xy_effect_state_t state;       /**< 当前状态 */
    uint32_t duration;            /**< 持续时间(ms) */
    uint32_t elapsed;             /**< 已过去时间(ms) */
    uint16_t repeat;               /**< 重复次数, 0=无限 */
    uint16_t current_repeat;      /**< 当前重复计数 */
    float progress;               /**< 进度 [0.0, 1.0] */
} xy_effect_t;

/* ==================== 淡入淡出效果 ==================== */

/**
 * @brief 淡入淡出效果配置
 */
typedef struct {
    xy_effect_t base;              /**< 基础效果 */
    float start_alpha;            /**< 起始透明度 [0.0, 1.0] */
    float end_alpha;              /**< 结束透明度 [0.0, 1.0] */
    bool fade_in;                 /**< true=淡入, false=淡出 */
} xy_effect_fade_t;

/* ==================== 闪烁效果 ==================== */

/**
 * @brief 闪烁效果配置
 */
typedef struct {
    xy_effect_t base;             /**< 基础效果 */
    uint16_t on_time;             /**< 亮起时间(ms) */
    uint16_t off_time;           /**< 熄灭时间(ms) */
    uint8_t min_brightness;      /**< 最小亮度 [0, 255] */
    uint8_t max_brightness;      /**< 最大亮度 [0, 255] */
} xy_effect_blink_t;

/* ==================== 呼吸灯效果 ==================== */

/**
 * @brief 呼吸灯效果配置
 */
typedef struct {
    xy_effect_t base;             /**< 基础效果 */
    uint16_t period;              /**< 周期时间(ms) */
    uint8_t min_brightness;       /**< 最小亮度 [0, 255] */
    uint8_t max_brightness;       /**< 最大亮度 [0, 255] */
    bool smooth;                  /**< 平滑过渡 */
} xy_effect_breath_t;

/* ==================== 滑动效果 ==================== */

/**
 * @brief 滑动效果配置
 */
typedef struct {
    xy_effect_t base;             /**< 基础效果 */
    xy_effect_dir_t direction;    /**< 滑动方向 */
    int16_t start_offset;         /**< 起始偏移量 */
    int16_t end_offset;           /**< 结束偏移量 */
} xy_effect_slide_t;

/* ==================== 旋转效果 ==================== */

/**
 * @brief 旋转效果配置
 */
typedef struct {
    xy_effect_t base;             /**< 基础效果 */
    float start_angle;            /**< 起始角度(度) */
    float end_angle;              /**< 结束角度(度) */
    float center_x;               /**< 旋转中心X */
    float center_y;               /**< 旋转中心Y */
    bool clockwise;               /**< 顺时针方向 */
} xy_effect_rotate_t;

/* ==================== API 函数 ==================== */

/* 初始化/反初始化 */
int xy_effect_init(void);
int xy_effect_deinit(void);

/* 基础效果操作 */
void xy_effect_start(xy_effect_t *effect);
void xy_effect_stop(xy_effect_t *effect);
void xy_effect_pause(xy_effect_t *effect);
void xy_effect_resume(xy_effect_t *effect);
void xy_effect_reset(xy_effect_t *effect);
bool xy_effect_is_running(xy_effect_t *effect);
float xy_effect_get_progress(xy_effect_t *effect);

/* 淡入淡出效果 */
int xy_effect_fade_create(xy_effect_fade_t *fade, bool fade_in, uint32_t duration);
void xy_effect_fade_update(xy_effect_fade_t *fade, uint32_t dt);
float xy_effect_fade_get_alpha(xy_effect_fade_t *fade);

/* 闪烁效果 */
int xy_effect_blink_create(xy_effect_blink_t *blink, uint16_t on_time, uint16_t off_time, uint8_t min_bright, uint8_t max_bright);
void xy_effect_blink_update(xy_effect_blink_t *blink, uint32_t dt);
bool xy_effect_blink_is_on(xy_effect_blink_t *blink);
uint8_t xy_effect_blink_get_brightness(xy_effect_blink_t *blink);

/* 呼吸灯效果 */
int xy_effect_breath_create(xy_effect_breath_t *breath, uint16_t period, uint8_t min_bright, uint8_t max_bright);
void xy_effect_breath_update(xy_effect_breath_t *breath, uint32_t dt);
uint8_t xy_effect_breath_get_brightness(xy_effect_breath_t *breath);
float xy_effect_breath_get_sine_value(xy_effect_breath_t *breath);

/* 滑动效果 */
int xy_effect_slide_create(xy_effect_slide_t *slide, xy_effect_dir_t direction, int16_t offset, uint32_t duration);
void xy_effect_slide_update(xy_effect_slide_t *slide, uint32_t dt);
int16_t xy_effect_slide_get_offset(xy_effect_slide_t *slide);

/* 旋转效果 */
int xy_effect_rotate_create(xy_effect_rotate_t *rotate, float start_angle, float end_angle, uint32_t duration);
void xy_effect_rotate_update(xy_effect_rotate_t *rotate, uint32_t dt);
float xy_effect_rotate_get_angle(xy_effect_rotate_t *rotate);

/* 工具函数 */
uint32_t xy_effect_get_time_ms(void);
float xy_effect_lerp(float a, float b, float t);
float xy_effect_ease_in(float t);
float xy_effect_ease_out(float t);
float xy_effect_ease_in_out(float t);

#ifdef __cplusplus
}
#endif

#endif /* XY_GUI_EFFECTS_H */
