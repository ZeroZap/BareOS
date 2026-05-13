/**
 * @file xy_rgb_fx_matrix.c
 * @brief RGB LED Matrix Effects (2D Grid Effects)
 * @version 1.0.0
 * @date 2026-03-02
 */

#include "xy_rgb_fx.h"
#include "xy_rgb_color.h"
#include "xy_log.h"
#include <stdlib.h>
#include <math.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* 矩阵配置 */
static uint16_t g_matrix_width = 16;
static uint16_t g_matrix_height = 16;

/**
 * @brief 设置矩阵尺寸
 */
void xy_rgb_matrix_set_size(uint16_t width, uint16_t height)
{
    g_matrix_width = width;
    g_matrix_height = height;
}

/**
 * @brief 2D 等离子效果
 */
void xy_rgb_fx_matrix_plasma(xy_rgb_segment_t *seg)
{
    uint16_t len = seg->stop - seg->start;
    
    for (uint16_t i = 0; i < len; i++) {
        uint16_t x = i % g_matrix_width;
        uint16_t y = i / g_matrix_width;
        
        /* 等离子公式 */
        float value = sinf(x * 0.1f + g_frame_count * seg->speed / 4096.0f) +
                      sinf(y * 0.1f + g_frame_count * seg->speed / 4096.0f) +
                      sinf((x + y) * 0.1f + g_frame_count * seg->speed / 4096.0f) +
                      sinf(sqrtf(x*x + y*y) * 0.1f + g_frame_count * seg->speed / 4096.0f);
        
        uint8_t hue = (uint8_t)(128 + 64 * value);
        xy_rgb_set_pixel(seg->start + i, xy_rainbow_color(hue));
    }
}

/**
 * @brief 生命游戏 (Conway's Game of Life)
 */
void xy_rgb_fx_game_of_life(xy_rgb_segment_t *seg)
{
    static bool grid[16][16] = {false};
    static bool next_grid[16][16] = {false};
    static bool initialized = false;
    
    uint16_t len = seg->stop - seg->start;
    
    /* 初始化 */
    if (!initialized) {
        for (uint16_t y = 0; y < g_matrix_height && y < 16; y++) {
            for (uint16_t x = 0; x < g_matrix_width && x < 16; x++) {
                grid[y][x] = (rand() % 3 == 0);
            }
        }
        initialized = true;
    }
    
    /* 计算下一代 */
    for (uint16_t y = 0; y < g_matrix_height && y < 16; y++) {
        for (uint16_t x = 0; x < g_matrix_width && x < 16; x++) {
            uint8_t neighbors = 0;
            
            /* 计算邻居数量 */
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    if (dx == 0 && dy == 0) continue;
                    
                    int nx = (x + dx + g_matrix_width) % g_matrix_width;
                    int ny = (y + dy + g_matrix_height) % g_matrix_height;
                    
                    if (grid[ny][nx]) neighbors++;
                }
            }
            
            /* 生命游戏规则 */
            if (grid[y][x]) {
                next_grid[y][x] = (neighbors == 2 || neighbors == 3);
            } else {
                next_grid[y][x] = (neighbors == 3);
            }
        }
    }
    
    /* 更新网格 */
    for (uint16_t y = 0; y < g_matrix_height && y < 16; y++) {
        for (uint16_t x = 0; x < g_matrix_width && x < 16; x++) {
            grid[y][x] = next_grid[y][x];
            
            uint16_t i = y * g_matrix_width + x;
            if (i < len) {
                rgb_color_t color = grid[y][x] ? seg->color1 : seg->color2;
                xy_rgb_set_pixel(seg->start + i, color);
            }
        }
    }
    
    /* 控制速度 */
    if (g_frame_count % (256 / seg->speed) != 0) {
        return;
    }
}

/**
 * @brief 矩阵雨效果 (Matrix Rain)
 */
void xy_rgb_fx_matrix_rain(xy_rgb_segment_t *seg)
{
    static uint8_t drops[16] = {0};
    static bool initialized = false;
    
    if (!initialized) {
        for (int i = 0; i < 16 && i < g_matrix_width; i++) {
            drops[i] = rand() % g_matrix_height;
        }
        initialized = true;
    }
    
    uint16_t len = seg->stop - seg->start;
    
    /* 清除 */
    for (uint16_t i = 0; i < len; i++) {
        rgb_color_t current = xy_rgb_get_pixel(seg->start + i);
        rgb_color_t faded = xy_color_darken(current, 20);
        xy_rgb_set_pixel(seg->start + i, faded);
    }
    
    /* 更新雨滴 */
    for (int x = 0; x < 16 && x < g_matrix_width; x++) {
        drops[x]++;
        
        if (drops[x] >= g_matrix_height) {
            drops[x] = 0;
        }
        
        uint16_t i = drops[x] * g_matrix_width + x;
        if (i < len) {
            xy_rgb_set_pixel(seg->start + i, seg->color1);
        }
    }
}

/**
 * @brief 2D 火焰效果
 */
void xy_rgb_fx_fire_2d(xy_rgb_segment_t *seg)
{
    uint16_t len = seg->stop - seg->start;
    
    for (uint16_t i = 0; i < len; i++) {
        uint16_t x = i % g_matrix_width;
        uint16_t y = i / g_matrix_width;
        
        /* 从底部向上衰减 */
        uint8_t intensity = (uint8_t)(255 - y * 255 / g_matrix_height);
        
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
        
        xy_rgb_set_pixel(seg->start + i, color);
    }
}

/**
 * @brief 2D 噪声效果
 */
void xy_rgb_fx_noise_2d(xy_rgb_segment_t *seg)
{
    uint16_t len = seg->stop - seg->start;
    
    for (uint16_t i = 0; i < len; i++) {
        uint16_t x = i % g_matrix_width;
        uint16_t y = i / g_matrix_width;
        
        /* 简单噪声 */
        uint8_t value = rand() % 256;
        
        /* 时间平滑 */
        static uint8_t noise[16][16];
        if (g_frame_count % (256 / seg->speed) == 0) {
            noise[y][x] = value;
        }
        
        uint8_t hue = noise[y][x];
        xy_rgb_set_pixel(seg->start + i, xy_rainbow_color(hue));
    }
}

/**
 * @brief 迷宫效果
 */
void xy_rgb_fx_maze(xy_rgb_segment_t *seg)
{
    static uint8_t maze[16][16];
    static uint8_t px = 0, py = 0;
    static bool initialized = false;
    
    if (!initialized) {
        for (int y = 0; y < 16; y++) {
            for (int x = 0; x < 16; x++) {
                maze[y][x] = (rand() % 3 == 0) ? 255 : 0;
            }
        }
        initialized = true;
    }
    
    uint16_t len = seg->stop - seg->start;
    
    /* 移动粒子 */
    if (g_frame_count % (256 / seg->speed) == 0) {
        uint8_t dir = rand() % 4;
        switch (dir) {
            case 0: if (px > 0) px--; break;
            case 1: if (px < g_matrix_width - 1) px++; break;
            case 2: if (py > 0) py--; break;
            case 3: if (py < g_matrix_height - 1) py++; break;
        }
        
        maze[py][px] = 255;
    }
    
    /* 显示迷宫 */
    for (uint16_t i = 0; i < len; i++) {
        uint16_t x = i % g_matrix_width;
        uint16_t y = i / g_matrix_width;
        
        uint8_t value = maze[y][x];
        rgb_color_t color = {value, value, value};
        xy_rgb_set_pixel(seg->start + i, color);
        
        /* 衰减 */
        if (maze[y][x] > 0) {
            maze[y][x] -= 5;
        }
    }
}
