/**
 * @file xy_rgb_fx.h
 * @brief RGB LED Effects Interface
 * @version 1.0.0
 * @date 2026-03-02
 */

#ifndef XY_RGB_FX_H
#define XY_RGB_FX_H

#include "xy_rgb.h"
#include "xy_rgb_segment.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 效果处理函数类型
 */
typedef void (*xy_rgb_fx_handler_t)(xy_rgb_segment_t *seg);

/**
 * @brief 效果信息结构
 */
typedef struct {
    xy_rgb_effect_t id;           /* 效果 ID */
    const char *name;             /* 效果名称 */
    xy_rgb_fx_handler_t handler;  /* 处理函数 */
    uint16_t min_speed;           /* 最小速度 */
    uint16_t max_speed;           /* 最大速度 */
} xy_rgb_fx_info_t;

/**
 * @brief 初始化效果引擎
 */
void xy_rgb_fx_init(void);

/**
 * @brief 获取效果信息
 * @param effect 效果 ID
 * @return 效果信息
 */
const xy_rgb_fx_info_t* xy_rgb_fx_get_info(xy_rgb_effect_t effect);

/**
 * @brief 获取效果数量
 * @return 效果数量
 */
uint8_t xy_rgb_fx_get_count(void);

/**
 * @brief 获取效果名称
 * @param effect 效果 ID
 * @return 效果名称
 */
const char* xy_rgb_fx_get_name(xy_rgb_effect_t effect);

/**
 * @brief 效果服务 (内部调用)
 */
void xy_rgb_fx_service(void);

/* ==================== 静态效果 ==================== */

/**
 * @brief 静态颜色效果
 */
void xy_rgb_fx_static(xy_rgb_segment_t *seg);

/**
 * @brief 彩虹效果
 */
void xy_rgb_fx_rainbow(xy_rgb_segment_t *seg);

/**
 * @brief 渐变效果
 */
void xy_rgb_fx_gradient(xy_rgb_segment_t *seg);

/* ==================== 动态效果 ==================== */

/**
 * @brief 扫描灯效果
 */
void xy_rgb_fx_scan(xy_rgb_segment_t *seg);

/**
 * @brief 追逐灯效果
 */
void xy_rgb_fx_chase(xy_rgb_segment_t *seg);

/**
 * @brief 淡入淡出效果
 */
void xy_rgb_fx_fade(xy_rgb_segment_t *seg);

/**
 * @brief 闪烁效果
 */
void xy_rgb_fx_blink(xy_rgb_segment_t *seg);

/**
 * @brief 闪烁星光效果
 */
void xy_rgb_fx_twinkle(xy_rgb_segment_t *seg);

/**
 * @brief 彗星效果
 */
void xy_rgb_fx_comet(xy_rgb_segment_t *seg);

/**
 * @brief 火焰效果
 */
void xy_rgb_fx_fire(xy_rgb_segment_t *seg);

/**
 * @brief 水流效果
 */
void xy_rgb_fx_water(xy_rgb_segment_t *seg);

/**
 * @brief 呼吸效果
 */
void xy_rgb_fx_breath(xy_rgb_segment_t *seg);

/**
 * @brief 流星效果
 */
void xy_rgb_fx_meteor(xy_rgb_segment_t *seg);

/* ==================== 音乐效果 ==================== */

/**
 * @brief 频谱效果
 */
void xy_rgb_fx_spectrum(xy_rgb_segment_t *seg, uint8_t *spectrum, uint8_t bands);

/**
 * @brief 音量表效果
 */
void xy_rgb_fx_vu_meter(xy_rgb_segment_t *seg, uint8_t level);

#ifdef __cplusplus
}
#endif

#endif /* XY_RGB_FX_H */
