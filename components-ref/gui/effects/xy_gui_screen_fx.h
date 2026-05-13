/**
 * @file xy_led_screen_fx.h
 * @brief LED Screen Effects Engine
 * @version 1.0.0
 * @date 2026-03-02
 */

#ifndef XY_LED_SCREEN_FX_H
#define XY_LED_SCREEN_FX_H

#include "xy_led_screen.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 效果 ID
 */
typedef enum {
    FX_SCREEN_NONE = 0,
    
    /* 基础效果 */
    FX_SCREEN_SCROLL_TEXT,
    FX_SCREEN_FADE_IN,
    FX_SCREEN_FADE_OUT,
    FX_SCREEN_ZOOM,
    FX_SCREEN_ROTATE,
    
    /* 过渡效果 */
    FX_SCREEN_BLINDS,
    FX_SCREEN_DISSOLVE,
    FX_SCREEN_WIPE,
    
    /* 变形效果 */
    FX_SCREEN_WAVE,
    FX_SCREEN_RIPPLE,
    FX_SCREEN_SWIRL,
    
    /* 粒子效果 */
    FX_SCREEN_RAIN,
    FX_SCREEN_SNOW,
    FX_SCREEN_FIRE,
    FX_SCREEN_METEOR,
    
    /* 3D 效果 */
    FX_SCREEN_FLIP_3D,
    FX_SCREEN_CUBE_ROTATE,
    FX_SCREEN_PAGE_TURN,
    
    /* 特殊效果 */
    FX_SCREEN_MATRIX_CODE,
    FX_SCREEN_PLASMA,
    FX_SCREEN_GAME_OF_LIFE,
    
    FX_SCREEN_COUNT,
} xy_led_screen_fx_id_t;

/**
 * @brief 效果配置
 */
typedef struct {
    xy_led_screen_fx_id_t fx_id;
    uint16_t speed;
    uint16_t intensity;
    uint8_t param1;
    uint8_t param2;
    uint32_t color;
} xy_led_screen_fx_config_t;

/**
 * @brief 效果引擎
 */
typedef struct {
    xy_led_screen_t *screen;
    xy_led_screen_fx_id_t current_fx;
    xy_led_screen_fx_config_t config;
    uint32_t frame;
    uint32_t last_update;
    bool running;
} xy_led_screen_fx_engine_t;

/* ==================== API ==================== */

/**
 * @brief 初始化效果引擎
 */
int xy_led_screen_fx_init(xy_led_screen_fx_engine_t *engine,
                          xy_led_screen_t *screen);

/**
 * @brief 设置效果
 */
int xy_led_screen_fx_set(xy_led_screen_fx_engine_t *engine,
                         xy_led_screen_fx_id_t fx_id,
                         uint16_t speed,
                         uint16_t intensity,
                         uint32_t color);

/**
 * @brief 停止效果
 */
void xy_led_screen_fx_stop(xy_led_screen_fx_engine_t *engine);

/**
 * @brief 更新效果
 */
void xy_led_screen_fx_update(xy_led_screen_fx_engine_t *engine);

/**
 * @brief 获取效果数量
 */
uint8_t xy_led_screen_fx_get_count(void);

/**
 * @brief 获取效果名称
 */
const char* xy_led_screen_fx_get_name(xy_led_screen_fx_id_t fx_id);

/**
 * @brief 下一个效果
 */
xy_led_screen_fx_id_t xy_led_screen_fx_next(xy_led_screen_fx_id_t current);

/**
 * @brief 上一个效果
 */
xy_led_screen_fx_id_t xy_led_screen_fx_prev(xy_led_screen_fx_id_t current);

/**
 * @brief 随机效果
 */
xy_led_screen_fx_id_t xy_led_screen_fx_random(void);

#ifdef __cplusplus
}
#endif

#endif /* XY_LED_SCREEN_FX_H */
