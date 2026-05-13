/**
 * @file xy_rgb_fx_music.c
 * @brief RGB LED Music Reactive Effects
 * @version 1.0.0
 * @date 2026-03-02
 */

#include "xy_rgb_fx.h"
#include "xy_rgb_color.h"
#include "xy_log.h"
#include <stdlib.h>
#include <math.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* 音频数据 */
static uint8_t g_spectrum[16] = {0};
static uint8_t g_volume = 0;
static uint8_t g_beat = 0;

/**
 * @brief 设置频谱数据
 */
void xy_rgb_set_spectrum(uint8_t *spectrum, uint8_t bands)
{
    for (int i = 0; i < 16 && i < bands; i++) {
        g_spectrum[i] = spectrum[i];
    }
}

/**
 * @brief 设置音量
 */
void xy_rgb_set_volume(uint8_t volume)
{
    g_volume = volume;
}

/**
 * @brief 设置节拍
 */
void xy_rgb_set_beat(uint8_t beat)
{
    g_beat = beat;
}

/**
 * @brief 节拍检测效果
 */
void xy_rgb_fx_beat(xy_rgb_segment_t *seg)
{
    uint16_t len = seg->stop - seg->start;
    
    if (g_beat > 0) {
        /* 节拍时全亮 */
        for (uint16_t i = 0; i < len; i++) {
            uint8_t brightness = g_beat;
            rgb_color_t color = xy_color_darken(seg->color1, 255 - brightness);
            xy_rgb_set_pixel(seg->start + i, color);
        }
        g_beat = 0;
    } else {
        /* 衰减 */
        for (uint16_t i = 0; i < len; i++) {
            rgb_color_t color = xy_rgb_get_pixel(seg->start + i);
            rgb_color_t faded = xy_color_darken(color, 20);
            xy_rgb_set_pixel(seg->start + i, faded);
        }
    }
}

/**
 * @brief 频率分析效果
 */
void xy_rgb_fx_frequency(xy_rgb_segment_t *seg)
{
    uint16_t len = seg->stop - seg->start;
    uint8_t bands = 8;
    uint16_t leds_per_band = len / bands;
    
    for (uint8_t b = 0; b < bands; b++) {
        uint8_t level = g_spectrum[b % 16];
        uint8_t height = level * leds_per_band / 256;
        
        for (uint16_t i = 0; i < leds_per_band && i < height; i++) {
            uint16_t idx = seg->start + b * leds_per_band + i;
            
            /* 根据频段选择颜色 */
            rgb_color_t color;
            switch (b) {
                case 0: color = (rgb_color_t){255, 0, 0}; break;     /* 低频 - 红 */
                case 1: color = (rgb_color_t){255, 128, 0}; break;  /* 中低频 - 橙 */
                case 2: color = (rgb_color_t){255, 255, 0}; break;  /* 中频 - 黄 */
                case 3: color = (rgb_color_t){0, 255, 0}; break;    /* 中高频 - 绿 */
                case 4: color = (rgb_color_t){0, 255, 255}; break;  /* 高频 - 青 */
                case 5: color = (rgb_color_t){0, 0, 255}; break;    /* 高频 - 蓝 */
                case 6: color = (rgb_color_t){128, 0, 255}; break;  /* 超高频 - 紫 */
                case 7: color = (rgb_color_t){255, 0, 255}; break;  /* 超高频 - 粉 */
                default: color = seg->color1; break;
            }
            
            xy_rgb_set_pixel(idx, color);
        }
    }
}

/**
 * @brief 自相关效果
 */
void xy_rgb_fx_autocorr(xy_rgb_segment_t *seg)
{
    uint16_t len = seg->stop - seg->start;
    
    /* 使用音量控制亮度 */
    uint8_t brightness = g_volume;
    
    for (uint16_t i = 0; i < len; i++) {
        /* 正弦波模式 */
        float phase = i * 3.14159f * 2 / len + g_frame_count * seg->speed / 4096.0f;
        uint8_t mod = (uint8_t)(128 + 127 * sinf(phase));
        
        uint8_t final_brightness = brightness * mod / 255;
        rgb_color_t color = xy_color_darken(seg->color1, 255 - final_brightness);
        xy_rgb_set_pixel(seg->start + i, color);
    }
}

/**
 * @brief 音乐频谱 + 火焰组合效果
 */
void xy_rgb_fx_music_fire(xy_rgb_segment_t *seg)
{
    uint16_t len = seg->stop - seg->start;
    uint8_t bands = 8;
    uint16_t leds_per_band = len / bands;
    
    for (uint8_t b = 0; b < bands; b++) {
        uint8_t level = g_spectrum[b % 16];
        
        for (uint16_t i = 0; i < leds_per_band; i++) {
            uint16_t idx = seg->start + b * leds_per_band + i;
            
            /* 从底部向上衰减 */
            uint8_t base_intensity = (uint8_t)(255 - i * 255 / leds_per_band);
            
            /* 音乐调制 */
            uint8_t music_mod = level * (b + 1) / bands;
            uint8_t intensity = base_intensity * music_mod / 255;
            
            /* 随机波动 */
            if (rand() % 256 < seg->intensity) {
                intensity = intensity * (150 + rand() % 106) / 255;
            }
            
            /* 火焰颜色 */
            rgb_color_t color;
            if (intensity > 128) {
                color = xy_color_blend((rgb_color_t){255, 0, 0},
                                      (rgb_color_t){255, 255, 0},
                                      intensity - 128);
            } else {
                color = xy_color_darken((rgb_color_t){255, 0, 0}, 128 - intensity);
            }
            
            xy_rgb_set_pixel(idx, color);
        }
    }
}

/**
 * @brief 音乐 VU 表增强版
 */
void xy_rgb_fx_vu_meter_enhanced(xy_rgb_segment_t *seg)
{
    uint16_t len = seg->stop - seg->start;
    uint8_t height = g_volume * len / 256;
    
    for (uint16_t i = 0; i < len; i++) {
        rgb_color_t color;
        
        if (i < height) {
            /* 根据高度选择颜色 */
            if (i < len * 2 / 3) {
                color = seg->color1;  /* 绿色 - 安全 */
            } else if (i < len * 5 / 6) {
                color = seg->color2;  /* 黄色 - 警告 */
            } else {
                color = seg->color3;  /* 红色 - 危险 */
            }
            
            /* 峰值保持 */
            static uint8_t peak = 0;
            if (height > peak) {
                peak = height;
            } else if (g_frame_count % 10 == 0 && peak > 0) {
                peak--;
            }
            
            if (i == peak) {
                color = (rgb_color_t){255, 255, 255};  /* 峰值白色 */
            }
            
            xy_rgb_set_pixel(seg->start + i, color);
        } else {
            xy_rgb_set_pixel(seg->start + i, (rgb_color_t){10, 10, 10});
        }
    }
}
