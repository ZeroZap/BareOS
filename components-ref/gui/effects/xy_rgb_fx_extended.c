/**
 * @file xy_rgb_fx_extended.c
 * @brief RGB LED Extended Effects (Collection from WS2812FX/FastLED/WLED)
 * @version 1.0.0
 * @date 2026-03-02
 */

#include "xy_rgb_fx.h"
#include "xy_rgb_color.h"
#include "xy_log.h"
#include <stdlib.h>
#include <math.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* ==================== 经典效果 ==================== */

/**
 * @brief 颜色清扫效果 (Color Wipe)
 */
void xy_rgb_fx_color_wipe(xy_rgb_segment_t *seg)
{
    uint16_t len = seg->stop - seg->start;
    uint16_t pos = (g_frame_count * seg->speed / 256) % (len * 2);
    
    if (pos < len) {
        /* 清扫 */
        xy_rgb_set_pixel(seg->start + pos, seg->color1);
    } else {
        /* 返回 */
        xy_rgb_set_pixel(seg->start + (len * 2 - 1 - pos), seg->color2);
    }
}

/**
 * @brief 剧院追逐效果 (Theater Chase)
 */
void xy_rgb_fx_theater_chase(xy_rgb_segment_t *seg)
{
    uint16_t len = seg->stop - seg->start;
    uint16_t offset = (g_frame_count * seg->speed / 512) % 3;
    
    for (uint16_t i = 0; i < len; i++) {
        if ((i + offset) % 3 == 0) {
            xy_rgb_set_pixel(seg->start + i, seg->color1);
        } else {
            xy_rgb_set_pixel(seg->start + i, seg->color2);
        }
    }
}

/**
 * @brief 彩虹循环效果 (Rainbow Cycle)
 */
void xy_rgb_fx_rainbow_cycle(xy_rgb_segment_t *seg)
{
    uint16_t len = seg->stop - seg->start;
    uint8_t hue = (g_frame_count * seg->speed / 256) % 256;
    
    for (uint16_t i = 0; i < len; i++) {
        uint8_t h = (hue + i * 256 / len) % 256;
        xy_rgb_set_pixel(seg->start + i, xy_rainbow_color(h));
    }
}

/**
 * @brief 流动灯效果 (Running Lights)
 */
void xy_rgb_fx_running_lights(xy_rgb_segment_t *seg)
{
    uint16_t len = seg->stop - seg->start;
    uint16_t offset = (g_frame_count * seg->speed / 256) % len;
    
    for (uint16_t i = 0; i < len; i++) {
        uint16_t idx = (i + offset) % len;
        uint8_t brightness = (uint8_t)(128 + 127 * sinf(i * 3.14159f * 2 / len));
        rgb_color_t color = xy_color_darken(seg->color1, 255 - brightness);
        xy_rgb_set_pixel(seg->start + idx, color);
    }
}

/**
 * @brief 激光扫描效果 (Larson Scanner)
 */
void xy_rgb_fx_larson_scanner(xy_rgb_segment_t *seg)
{
    uint16_t len = seg->stop - seg->start;
    uint16_t pos = (g_frame_count * seg->speed / 256) % (len * 2 - 2);
    
    xy_rgb_clear();
    
    /* 头部 */
    uint16_t head_pos = (pos < len) ? pos : (len * 2 - 2 - pos);
    xy_rgb_set_pixel(seg->start + head_pos, seg->color1);
    
    /* 尾部渐变 */
    for (int i = 1; i <= 5; i++) {
        uint16_t tail_pos;
        if (pos < len) {
            tail_pos = (head_pos >= i) ? (head_pos - i) : (head_pos + i);
        } else {
            tail_pos = (head_pos >= i) ? (head_pos - i) : (head_pos + i);
        }
        
        if (tail_pos < len) {
            rgb_color_t color = xy_color_darken(seg->color1, i * 50);
            xy_rgb_set_pixel(seg->start + tail_pos, color);
        }
    }
}

/**
 * @brief 涟漪效果 (Ripple)
 */
void xy_rgb_fx_ripple(xy_rgb_segment_t *seg)
{
    uint16_t len = seg->stop - seg->start;
    
    /* 随机生成涟漪中心 */
    if (rand() % 256 < seg->intensity) {
        uint16_t center = seg->start + (rand() % len);
        xy_rgb_set_pixel(center, seg->color1);
    }
    
    /* 涟漪扩散 */
    for (uint16_t i = 0; i < len; i++) {
        rgb_color_t color = xy_rgb_get_pixel(seg->start + i);
        if (!xy_color_equal(color, seg->color2)) {
            rgb_color_t faded = xy_color_darken(color, 20);
            xy_rgb_set_pixel(seg->start + i, faded);
        }
    }
}

/**
 * @brief 颜色波浪效果 (Color Waves)
 */
void xy_rgb_fx_color_waves(xy_rgb_segment_t *seg)
{
    uint16_t len = seg->stop - seg->start;
    
    for (uint16_t i = 0; i < len; i++) {
        float phase = g_frame_count * seg->speed / 4096.0f + i * 0.1f;
        uint8_t hue = (uint8_t)(128 + 127 * sinf(phase));
        xy_rgb_set_pixel(seg->start + i, xy_rainbow_color(hue));
    }
}

/**
 * @brief 警灯效果 (Police)
 */
void xy_rgb_fx_police(xy_rgb_segment_t *seg)
{
    uint16_t len = seg->stop - seg->start;
    uint16_t period = 256 * 256 / seg->speed;
    bool red_phase = (g_frame_count % period) < (period / 2);
    
    for (uint16_t i = 0; i < len; i++) {
        if (i < len / 2) {
            xy_rgb_set_pixel(seg->start + i, red_phase ? seg->color1 : seg->color2);
        } else {
            xy_rgb_set_pixel(seg->start + i, red_phase ? seg->color2 : seg->color1);
        }
    }
}

/**
 * @brief 烟花效果 (Fireworks)
 */
void xy_rgb_fx_fireworks(xy_rgb_segment_t *seg)
{
    uint16_t len = seg->stop - seg->start;
    
    /* 随机爆炸 */
    if (rand() % 256 < seg->intensity / 10) {
        uint16_t burst_pos = seg->start + (rand() % len);
        uint8_t burst_size = 3 + (rand() % 3);
        
        for (int i = -burst_size; i <= burst_size; i++) {
            if (burst_pos + i >= seg->start && burst_pos + i < seg->stop) {
                rgb_color_t color = xy_color_random();
                xy_rgb_set_pixel(burst_pos + i, color);
            }
        }
    }
    
    /* 重力衰减 */
    for (uint16_t i = 0; i < len; i++) {
        rgb_color_t color = xy_rgb_get_pixel(seg->start + i);
        if (!xy_color_equal(color, (rgb_color_t){0,0,0})) {
            rgb_color_t faded = xy_color_darken(color, 30);
            xy_rgb_set_pixel(seg->start + i, faded);
        }
    }
}

/**
 * @brief 闪电效果 (Lightning)
 */
void xy_rgb_fx_lightning(xy_rgb_segment_t *seg)
{
    uint16_t len = seg->stop - seg->start;
    
    /* 随机闪电 */
    if (rand() % 256 < seg->intensity / 20) {
        /* 全亮 */
        rgb_color_t flash = (rgb_color_t){255, 255, 255};
        xy_rgb_set_all(flash);
        
        /* 闪烁 */
        for (int i = 0; i < 3; i++) {
            xy_rgb_show();
            xy_os_delay(30);
            xy_rgb_clear();
            xy_rgb_show();
            xy_os_delay(30);
        }
    }
}

/**
 * @brief 弹球效果 (Bouncing Balls)
 */
void xy_rgb_fx_bouncing_balls(xy_rgb_segment_t *seg)
{
    uint16_t len = seg->stop - seg->start;
    static uint16_t ball_pos[3] = {0, len/3, len*2/3};
    static int8_t ball_dir[3] = {1, 1, 1};
    
    xy_rgb_clear();
    
    /* 更新球位置 */
    for (int b = 0; b < 3; b++) {
        ball_pos[b] += ball_dir[b];
        
        if (ball_pos[b] >= len - 1 || ball_pos[b] <= 0) {
            ball_dir[b] = -ball_dir[b];
        }
        
        rgb_color_t color;
        switch (b) {
            case 0: color = seg->color1; break;
            case 1: color = seg->color2; break;
            case 2: color = seg->color3; break;
        }
        
        xy_rgb_set_pixel(seg->start + ball_pos[b], color);
    }
}

/**
 * @brief 蜡烛效果 (Candle)
 */
void xy_rgb_fx_candle(xy_rgb_segment_t *seg)
{
    uint16_t len = seg->stop - seg->start;
    
    for (uint16_t i = 0; i < len; i++) {
        /* 随机闪烁模拟蜡烛 */
        uint8_t flicker = 180 + (rand() % 76);
        rgb_color_t base = seg->color1;
        rgb_color_t color = {
            .r = base.r * flicker / 255,
            .g = base.g * flicker / 255,
            .b = base.b * flicker / 255,
        };
        xy_rgb_set_pixel(seg->start + i, color);
    }
}

/**
 * @brief 水族馆效果 (Aquarium)
 */
void xy_rgb_fx_aquarium(xy_rgb_segment_t *seg)
{
    uint16_t len = seg->stop - seg->start;
    static uint8_t fish_pos[5] = {0};
    static uint8_t fish_speed[5] = {1, 2, 3, 2, 1};
    
    xy_rgb_clear();
    
    /* 背景蓝色 */
    rgb_color_t bg = (rgb_color_t){0, 0, 50};
    for (uint16_t i = 0; i < len; i++) {
        xy_rgb_set_pixel(seg->start + i, bg);
    }
    
    /* 鱼游动 */
    for (int f = 0; f < 5; f++) {
        fish_pos[f] += fish_speed[f];
        if (fish_pos[f] >= len) {
            fish_pos[f] = 0;
        }
        
        rgb_color_t fish_color;
        switch (f) {
            case 0: fish_color = (rgb_color_t){255, 0, 0}; break;
            case 1: fish_color = (rgb_color_t){0, 255, 0}; break;
            case 2: fish_color = (rgb_color_t){0, 0, 255}; break;
            case 3: fish_color = (rgb_color_t){255, 255, 0}; break;
            case 4: fish_color = (rgb_color_t){255, 0, 255}; break;
        }
        
        xy_rgb_set_pixel(seg->start + fish_pos[f], fish_color);
    }
}

/* ==================== 效果注册表 ==================== */

const xy_rgb_fx_info_t g_extended_fx_info[] = {
    {100, "Color Wipe", xy_rgb_fx_color_wipe, 1, 255},
    {101, "Theater Chase", xy_rgb_fx_theater_chase, 1, 255},
    {102, "Rainbow Cycle", xy_rgb_fx_rainbow_cycle, 1, 255},
    {103, "Running Lights", xy_rgb_fx_running_lights, 1, 255},
    {104, "Larson Scanner", xy_rgb_fx_larson_scanner, 1, 255},
    {105, "Ripple", xy_rgb_fx_ripple, 1, 255},
    {106, "Color Waves", xy_rgb_fx_color_waves, 1, 255},
    {107, "Police", xy_rgb_fx_police, 1, 255},
    {108, "Fireworks", xy_rgb_fx_fireworks, 1, 255},
    {109, "Lightning", xy_rgb_fx_lightning, 1, 255},
    {110, "Bouncing Balls", xy_rgb_fx_bouncing_balls, 1, 255},
    {111, "Candle", xy_rgb_fx_candle, 1, 255},
    {112, "Aquarium", xy_rgb_fx_aquarium, 1, 255},
};
