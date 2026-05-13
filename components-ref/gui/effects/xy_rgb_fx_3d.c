/**
 * @file xy_rgb_fx_3d.c
 * @brief RGB LED 3D Effects (Cube/Voxel Effects)
 * @version 1.0.0
 * @date 2026-03-02
 */

#include "xy_rgb_fx.h"
#include "xy_rgb_color.h"
#include "xy_log.h"
#include <stdlib.h>
#include <math.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* 3D 配置 */
static uint8_t g_cube_size = 4;

/**
 * @brief 设置立方体尺寸
 */
void xy_rgb_3d_set_size(uint8_t size)
{
    g_cube_size = size;
}

/**
 * @brief 3D 雨滴效果增强版
 */
void xy_rgb_fx_rain_3d(xy_rgb_segment_t *seg)
{
    static uint8_t drops[4][4] = {0};
    
    /* 清除 */
    xy_rgb_clear();
    
    /* 更新雨滴 */
    for (uint8_t x = 0; x < g_cube_size; x++) {
        for (uint8_t y = 0; y < g_cube_size; y++) {
            drops[x][y]++;
            
            if (drops[x][y] >= g_cube_size) {
                if (rand() % 256 < seg->intensity) {
                    drops[x][y] = 0;
                } else {
                    drops[x][y] = g_cube_size;
                }
            }
            
            /* 绘制雨滴 */
            for (uint8_t z = 0; z < drops[x][y]; z++) {
                uint16_t index = z * g_cube_size * g_cube_size + y * g_cube_size + x;
                uint8_t brightness = 255 - z * 255 / g_cube_size;
                rgb_color_t color = xy_color_darken(seg->color1, 255 - brightness);
                xy_rgb_set_pixel(index, color);
            }
        }
    }
}

/**
 * @brief 3D 火焰效果增强版
 */
void xy_rgb_fx_fire_3d(xy_rgb_segment_t *seg)
{
    for (uint8_t z = 0; z < g_cube_size; z++) {
        for (uint8_t y = 0; y < g_cube_size; y++) {
            for (uint8_t x = 0; x < g_cube_size; x++) {
                uint16_t index = z * g_cube_size * g_cube_size + y * g_cube_size + x;
                
                /* 从底部向上衰减 */
                uint8_t intensity = (uint8_t)(255 - z * 255 / g_cube_size);
                
                /* 随机波动 */
                if (rand() % 256 < seg->intensity) {
                    intensity = intensity * (150 + rand() % 106) / 255;
                }
                
                /* 3D 噪声 */
                float noise = sinf(x * 0.5f + g_frame_count / 100.0f) *
                             sinf(y * 0.5f + g_frame_count / 100.0f) *
                             sinf(z * 0.5f + g_frame_count / 100.0f);
                
                intensity = intensity * (128 + 127 * noise) / 255;
                
                /* 火焰颜色 */
                rgb_color_t color;
                if (intensity > 128) {
                    color = xy_color_blend((rgb_color_t){255, 0, 0},
                                          (rgb_color_t){255, 255, 0},
                                          intensity - 128);
                } else {
                    color = xy_color_darken((rgb_color_t){255, 0, 0}, 128 - intensity);
                }
                
                xy_rgb_set_pixel(index, color);
            }
        }
    }
}

/**
 * @brief 3D 等离子效果
 */
void xy_rgb_fx_plasma_3d(xy_rgb_segment_t *seg)
{
    for (uint8_t z = 0; z < g_cube_size; z++) {
        for (uint8_t y = 0; y < g_cube_size; y++) {
            for (uint8_t x = 0; x < g_cube_size; x++) {
                uint16_t index = z * g_cube_size * g_cube_size + y * g_cube_size + x;
                
                /* 3D 等离子公式 */
                float value = sinf(x * 0.3f + g_frame_count * seg->speed / 4096.0f) +
                             sinf(y * 0.3f + g_frame_count * seg->speed / 4096.0f) +
                             sinf(z * 0.3f + g_frame_count * seg->speed / 4096.0f) +
                             sinf(sqrtf(x*x + y*y + z*z) * 0.2f);
                
                uint8_t hue = (uint8_t)(128 + 64 * value);
                xy_rgb_set_pixel(index, xy_rainbow_color(hue));
            }
        }
    }
}

/**
 * @brief 旋转立方体
 */
void xy_rgb_fx_rotate_cube(xy_rgb_segment_t *seg)
{
    static float angle_x = 0, angle_y = 0, angle_z = 0;
    
    /* 更新角度 */
    angle_x += seg->speed / 1000.0f;
    angle_y += seg->speed / 1000.0f;
    angle_z += seg->speed / 1000.0f;
    
    /* 清除 */
    xy_rgb_clear();
    
    /* 绘制立方体边框 */
    float cos_x = cosf(angle_x), sin_x = sinf(angle_x);
    float cos_y = cosf(angle_y), sin_y = sinf(angle_y);
    float cos_z = cosf(angle_z), sin_z = sinf(angle_z);
    
    /* 8 个顶点 */
    float vertices[8][3] = {
        {-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
        {-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1}
    };
    
    /* 旋转并投影 */
    for (int i = 0; i < 8; i++) {
        float x = vertices[i][0], y = vertices[i][1], z = vertices[i][2];
        
        /* 绕 X 轴旋转 */
        float y1 = y * cos_x - z * sin_x;
        float z1 = y * sin_x + z * cos_x;
        
        /* 绕 Y 轴旋转 */
        float x2 = x * cos_y + z1 * sin_y;
        float z2 = -x * sin_y + z1 * cos_y;
        
        /* 绕 Z 轴旋转 */
        float x3 = x2 * cos_z - y1 * sin_z;
        float y3 = x2 * sin_z + y1 * cos_z;
        
        /* 投影到 2D */
        int px = (int)((x3 + 2) * g_cube_size / 4);
        int py = (int)((y3 + 2) * g_cube_size / 4);
        int pz = (int)((z2 + 2) * g_cube_size / 4);
        
        if (px >= 0 && px < g_cube_size && py >= 0 && py < g_cube_size && pz >= 0 && pz < g_cube_size) {
            uint16_t index = pz * g_cube_size * g_cube_size + py * g_cube_size + px;
            xy_rgb_set_pixel(index, seg->color1);
        }
    }
}

/**
 * @brief 3D 噪声效果
 */
void xy_rgb_fx_noise_3d(xy_rgb_segment_t *seg)
{
    for (uint8_t z = 0; z < g_cube_size; z++) {
        for (uint8_t y = 0; y < g_cube_size; y++) {
            for (uint8_t x = 0; x < g_cube_size; x++) {
                uint16_t index = z * g_cube_size * g_cube_size + y * g_cube_size + x;
                
                /* 3D 噪声 */
                float noise = sinf(x * 0.5f + g_frame_count * seg->speed / 4096.0f) *
                             sinf(y * 0.5f + g_frame_count * seg->speed / 4096.0f) *
                             sinf(z * 0.5f + g_frame_count * seg->speed / 4096.0f);
                
                uint8_t hue = (uint8_t)(128 + 127 * noise);
                xy_rgb_set_pixel(index, xy_rainbow_color(hue));
            }
        }
    }
}
