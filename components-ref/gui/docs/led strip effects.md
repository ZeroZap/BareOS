# WS2812 灯带等待效果库设计 (C语言)

这是一个功能完整的 WS2812 灯带效果库，支持多灯带、多色彩配置和线段操作。

## 1. 头文件定义 (led_strip.h)

```c
#ifndef LED_STRIP_H
#define LED_STRIP_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/*============================================================================
 * 配置宏定义
 *============================================================================*/
#define MAX_STRIPS          8       // 最大支持灯带数量
#define MAX_SEGMENTS        16      // 每条灯带最大线段数
#define MAX_LEDS_PER_STRIP  300     // 每条灯带最大LED数

/*============================================================================
 * 色彩顺序枚举
 *============================================================================*/
typedef enum {
    COLOR_ORDER_RGB = 0,
    COLOR_ORDER_RBG,
    COLOR_ORDER_GRB,    // WS2812 默认
    COLOR_ORDER_GBR,
    COLOR_ORDER_BRG,
    COLOR_ORDER_BGR,
    COLOR_ORDER_RGBW,   // SK6812 RGBW
    COLOR_ORDER_GRBW,
    COLOR_ORDER_MAX
} ColorOrder_t;

/*============================================================================
 * 等待效果类型枚举
 *============================================================================*/
typedef enum {
    EFFECT_NONE = 0,
    EFFECT_STATIC,              // 静态颜色
    EFFECT_BLINK,               // 闪烁
    EFFECT_BREATH,              // 呼吸灯
    EFFECT_COLOR_WIPE,          // 颜色擦除
    EFFECT_COLOR_WIPE_REVERSE,  // 反向颜色擦除
    EFFECT_THEATER_CHASE,       // 剧院追逐
    EFFECT_RAINBOW,             // 彩虹
    EFFECT_RAINBOW_CYCLE,       // 彩虹循环
    EFFECT_RUNNING_LIGHTS,      // 跑马灯
    EFFECT_TWINKLE,             // 闪烁星星
    EFFECT_SPARKLE,             // 火花
    EFFECT_SNOW_SPARKLE,        // 雪花火花
    EFFECT_METEOR,              // 流星
    EFFECT_METEOR_RAIN,         // 流星雨
    EFFECT_FIRE,                // 火焰
    EFFECT_COMET,               // 彗星
    EFFECT_SCANNER,             // 扫描器 (Knight Rider)
    EFFECT_DUAL_SCANNER,        // 双向扫描器
    EFFECT_FADE,                // 渐变
    EFFECT_LOADING_SPINNER,     // 加载旋转
    EFFECT_PROGRESS_BAR,        // 进度条
    EFFECT_PULSE,               // 脉冲
    EFFECT_WAVE,                // 波浪
    EFFECT_GRADIENT,            // 渐变色
    EFFECT_CHASE_RAINBOW,       // 彩虹追逐
    EFFECT_LARSON_SCANNER,      // Larson扫描器
    EFFECT_BOUNCING_BALL,       // 弹跳球
    EFFECT_MAX
} EffectType_t;

/*============================================================================
 * 颜色结构体
 *============================================================================*/
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t w;  // 白色通道 (用于RGBW)
} Color_t;

/*============================================================================
 * 效果参数结构体
 *============================================================================*/
typedef struct {
    Color_t     color1;         // 主颜色
    Color_t     color2;         // 次颜色
    Color_t     color3;         // 第三颜色
    uint16_t    speed;          // 速度 (ms)
    uint8_t     brightness;     // 亮度 (0-255)
    uint8_t     density;        // 密度/间隔
    bool        reverse;        // 反向
    bool        mirror;         // 镜像
    uint8_t     fade_rate;      // 衰减率
    uint8_t     size;           // 效果大小
    uint8_t     trail_length;   // 拖尾长度
} EffectParams_t;

/*============================================================================
 * 线段结构体
 *============================================================================*/
typedef struct {
    uint16_t        start;          // 起始LED索引
    uint16_t        end;            // 结束LED索引
    EffectType_t    effect;         // 当前效果
    EffectParams_t  params;         // 效果参数
    uint32_t        last_update;    // 上次更新时间
    uint16_t        step;           // 当前步骤
    uint16_t        aux0;           // 辅助变量0
    uint16_t        aux1;           // 辅助变量1
    bool            active;         // 是否激活
    bool            paused;         // 是否暂停
} Segment_t;

/*============================================================================
 * 灯带结构体
 *============================================================================*/
typedef struct {
    uint8_t         id;                         // 灯带ID
    uint16_t        num_leds;                   // LED数量
    ColorOrder_t    color_order;                // 色彩顺序
    uint8_t         bytes_per_led;              // 每个LED字节数
    uint8_t*        buffer;                     // 数据缓冲区
    Color_t*        leds;                       // LED颜色数组
    Segment_t       segments[MAX_SEGMENTS];     // 线段数组
    uint8_t         num_segments;               // 线段数量
    uint8_t         global_brightness;          // 全局亮度
    bool            dirty;                      // 是否需要更新
    bool            active;                     // 是否激活
    
    // 硬件相关回调
    void (*send_data)(uint8_t* data, uint16_t len);  // 发送数据回调
} LedStrip_t;

/*============================================================================
 * 灯带管理器
 *============================================================================*/
typedef struct {
    LedStrip_t*     strips[MAX_STRIPS];
    uint8_t         num_strips;
    uint32_t        (*get_tick_ms)(void);       // 获取系统时间回调
} StripManager_t;

/*============================================================================
 * API 函数声明
 *============================================================================*/

// 初始化与销毁
StripManager_t* strip_manager_create(uint32_t (*get_tick_ms)(void));
void strip_manager_destroy(StripManager_t* manager);

LedStrip_t* led_strip_create(uint16_t num_leds, ColorOrder_t order, 
                             void (*send_data)(uint8_t*, uint16_t));
void led_strip_destroy(LedStrip_t* strip);

bool strip_manager_add(StripManager_t* manager, LedStrip_t* strip);
bool strip_manager_remove(StripManager_t* manager, uint8_t strip_id);

// 线段操作
int8_t segment_create(LedStrip_t* strip, uint16_t start, uint16_t end);
bool segment_delete(LedStrip_t* strip, uint8_t seg_id);
bool segment_set_effect(LedStrip_t* strip, uint8_t seg_id, 
                        EffectType_t effect, EffectParams_t* params);
bool segment_set_color(LedStrip_t* strip, uint8_t seg_id, 
                       Color_t color1, Color_t color2, Color_t color3);
bool segment_set_speed(LedStrip_t* strip, uint8_t seg_id, uint16_t speed);
bool segment_pause(LedStrip_t* strip, uint8_t seg_id);
bool segment_resume(LedStrip_t* strip, uint8_t seg_id);

// 颜色操作
void led_set_color(LedStrip_t* strip, uint16_t index, Color_t color);
Color_t led_get_color(LedStrip_t* strip, uint16_t index);
void led_set_range(LedStrip_t* strip, uint16_t start, uint16_t end, Color_t color);
void led_fill(LedStrip_t* strip, Color_t color);
void led_clear(LedStrip_t* strip);

// 亮度控制
void strip_set_brightness(LedStrip_t* strip, uint8_t brightness);
void segment_set_brightness(LedStrip_t* strip, uint8_t seg_id, uint8_t brightness);

// 更新与显示
void strip_update(StripManager_t* manager, LedStrip_t* strip);
void strip_show(LedStrip_t* strip);
void strip_manager_update_all(StripManager_t* manager);

// 颜色工具函数
Color_t color_rgb(uint8_t r, uint8_t g, uint8_t b);
Color_t color_rgbw(uint8_t r, uint8_t g, uint8_t b, uint8_t w);
Color_t color_hsv(uint8_t h, uint8_t s, uint8_t v);
Color_t color_wheel(uint8_t pos);
Color_t color_blend(Color_t c1, Color_t c2, uint8_t blend);
Color_t color_scale(Color_t color, uint8_t scale);

// 预定义颜色
#define COLOR_BLACK     (Color_t){0, 0, 0, 0}
#define COLOR_WHITE     (Color_t){255, 255, 255, 0}
#define COLOR_RED       (Color_t){255, 0, 0, 0}
#define COLOR_GREEN     (Color_t){0, 255, 0, 0}
#define COLOR_BLUE      (Color_t){0, 0, 255, 0}
#define COLOR_YELLOW    (Color_t){255, 255, 0, 0}
#define COLOR_CYAN      (Color_t){0, 255, 255, 0}
#define COLOR_MAGENTA   (Color_t){255, 0, 255, 0}
#define COLOR_ORANGE    (Color_t){255, 128, 0, 0}
#define COLOR_PURPLE    (Color_t){128, 0, 255, 0}
#define COLOR_PINK      (Color_t){255, 105, 180, 0}

// 默认效果参数
#define DEFAULT_EFFECT_PARAMS (EffectParams_t){ \
    .color1 = COLOR_BLUE,       \
    .color2 = COLOR_BLACK,      \
    .color3 = COLOR_BLACK,      \
    .speed = 50,                \
    .brightness = 255,          \
    .density = 5,               \
    .reverse = false,           \
    .mirror = false,            \
    .fade_rate = 128,           \
    .size = 3,                  \
    .trail_length = 5           \
}

#endif // LED_STRIP_H
```

## 2. 核心实现 (led_strip.c)

```c
#include "led_strip.h"

/*============================================================================
 * 内部辅助函数
 *============================================================================*/

// 获取色彩顺序的字节数
static uint8_t get_bytes_per_led(ColorOrder_t order) {
    switch (order) {
        case COLOR_ORDER_RGBW:
        case COLOR_ORDER_GRBW:
            return 4;
        default:
            return 3;
    }
}

// 根据色彩顺序转换颜色到缓冲区
static void color_to_buffer(Color_t color, ColorOrder_t order, 
                           uint8_t brightness, uint8_t* buf) {
    // 应用亮度
    uint8_t r = (color.r * brightness) >> 8;
    uint8_t g = (color.g * brightness) >> 8;
    uint8_t b = (color.b * brightness) >> 8;
    uint8_t w = (color.w * brightness) >> 8;
    
    switch (order) {
        case COLOR_ORDER_RGB:
            buf[0] = r; buf[1] = g; buf[2] = b;
            break;
        case COLOR_ORDER_RBG:
            buf[0] = r; buf[1] = b; buf[2] = g;
            break;
        case COLOR_ORDER_GRB:
            buf[0] = g; buf[1] = r; buf[2] = b;
            break;
        case COLOR_ORDER_GBR:
            buf[0] = g; buf[1] = b; buf[2] = r;
            break;
        case COLOR_ORDER_BRG:
            buf[0] = b; buf[1] = r; buf[2] = g;
            break;
        case COLOR_ORDER_BGR:
            buf[0] = b; buf[1] = g; buf[2] = r;
            break;
        case COLOR_ORDER_RGBW:
            buf[0] = r; buf[1] = g; buf[2] = b; buf[3] = w;
            break;
        case COLOR_ORDER_GRBW:
            buf[0] = g; buf[1] = r; buf[2] = b; buf[3] = w;
            break;
        default:
            buf[0] = g; buf[1] = r; buf[2] = b;
            break;
    }
}

// 随机数生成
static uint8_t random8(void) {
    static uint32_t seed = 12345;
    seed = seed * 1103515245 + 12345;
    return (seed >> 16) & 0xFF;
}

static uint16_t random16(uint16_t max) {
    return random8() * 256 / (256 / max + 1);
}

/*============================================================================
 * 颜色工具函数实现
 *============================================================================*/

Color_t color_rgb(uint8_t r, uint8_t g, uint8_t b) {
    return (Color_t){r, g, b, 0};
}

Color_t color_rgbw(uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
    return (Color_t){r, g, b, w};
}

Color_t color_hsv(uint8_t h, uint8_t s, uint8_t v) {
    Color_t color = {0, 0, 0, 0};
    
    if (s == 0) {
        color.r = color.g = color.b = v;
        return color;
    }
    
    uint8_t region = h / 43;
    uint8_t remainder = (h - (region * 43)) * 6;
    
    uint8_t p = (v * (255 - s)) >> 8;
    uint8_t q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    uint8_t t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;
    
    switch (region) {
        case 0:  color.r = v; color.g = t; color.b = p; break;
        case 1:  color.r = q; color.g = v; color.b = p; break;
        case 2:  color.r = p; color.g = v; color.b = t; break;
        case 3:  color.r = p; color.g = q; color.b = v; break;
        case 4:  color.r = t; color.g = p; color.b = v; break;
        default: color.r = v; color.g = p; color.b = q; break;
    }
    
    return color;
}

Color_t color_wheel(uint8_t pos) {
    pos = 255 - pos;
    if (pos < 85) {
        return color_rgb(255 - pos * 3, 0, pos * 3);
    }
    if (pos < 170) {
        pos -= 85;
        return color_rgb(0, pos * 3, 255 - pos * 3);
    }
    pos -= 170;
    return color_rgb(pos * 3, 255 - pos * 3, 0);
}

Color_t color_blend(Color_t c1, Color_t c2, uint8_t blend) {
    uint8_t inv_blend = 255 - blend;
    return (Color_t){
        .r = (c1.r * inv_blend + c2.r * blend) >> 8,
        .g = (c1.g * inv_blend + c2.g * blend) >> 8,
        .b = (c1.b * inv_blend + c2.b * blend) >> 8,
        .w = (c1.w * inv_blend + c2.w * blend) >> 8
    };
}

Color_t color_scale(Color_t color, uint8_t scale) {
    return (Color_t){
        .r = (color.r * scale) >> 8,
        .g = (color.g * scale) >> 8,
        .b = (color.b * scale) >> 8,
        .w = (color.w * scale) >> 8
    };
}

/*============================================================================
 * 灯带管理器实现
 *============================================================================*/

StripManager_t* strip_manager_create(uint32_t (*get_tick_ms)(void)) {
    StripManager_t* manager = (StripManager_t*)malloc(sizeof(StripManager_t));
    if (!manager) return NULL;
    
    memset(manager, 0, sizeof(StripManager_t));
    manager->get_tick_ms = get_tick_ms;
    
    return manager;
}

void strip_manager_destroy(StripManager_t* manager) {
    if (!manager) return;
    
    for (uint8_t i = 0; i < manager->num_strips; i++) {
        if (manager->strips[i]) {
            led_strip_destroy(manager->strips[i]);
        }
    }
    free(manager);
}

bool strip_manager_add(StripManager_t* manager, LedStrip_t* strip) {
    if (!manager || !strip || manager->num_strips >= MAX_STRIPS) {
        return false;
    }
    
    strip->id = manager->num_strips;
    manager->strips[manager->num_strips++] = strip;
    return true;
}

bool strip_manager_remove(StripManager_t* manager, uint8_t strip_id) {
    if (!manager || strip_id >= manager->num_strips) {
        return false;
    }
    
    led_strip_destroy(manager->strips[strip_id]);
    
    // 移动后面的灯带
    for (uint8_t i = strip_id; i < manager->num_strips - 1; i++) {
        manager->strips[i] = manager->strips[i + 1];
        manager->strips[i]->id = i;
    }
    manager->num_strips--;
    
    return true;
}

/*============================================================================
 * 灯带实现
 *============================================================================*/

LedStrip_t* led_strip_create(uint16_t num_leds, ColorOrder_t order,
                             void (*send_data)(uint8_t*, uint16_t)) {
    if (num_leds == 0 || num_leds > MAX_LEDS_PER_STRIP) {
        return NULL;
    }
    
    LedStrip_t* strip = (LedStrip_t*)malloc(sizeof(LedStrip_t));
    if (!strip) return NULL;
    
    memset(strip, 0, sizeof(LedStrip_t));
    
    strip->num_leds = num_leds;
    strip->color_order = order;
    strip->bytes_per_led = get_bytes_per_led(order);
    strip->global_brightness = 255;
    strip->send_data = send_data;
    strip->active = true;
    
    // 分配LED颜色数组
    strip->leds = (Color_t*)calloc(num_leds, sizeof(Color_t));
    if (!strip->leds) {
        free(strip);
        return NULL;
    }
    
    // 分配数据缓冲区
    strip->buffer = (uint8_t*)calloc(num_leds * strip->bytes_per_led, 1);
    if (!strip->buffer) {
        free(strip->leds);
        free(strip);
        return NULL;
    }
    
    // 创建默认全灯带线段
    segment_create(strip, 0, num_leds - 1);
    
    return strip;
}

void led_strip_destroy(LedStrip_t* strip) {
    if (!strip) return;
    
    if (strip->leds) free(strip->leds);
    if (strip->buffer) free(strip->buffer);
    free(strip);
}

/*============================================================================
 * 线段操作实现
 *============================================================================*/

int8_t segment_create(LedStrip_t* strip, uint16_t start, uint16_t end) {
    if (!strip || strip->num_segments >= MAX_SEGMENTS) {
        return -1;
    }
    
    if (start > end || end >= strip->num_leds) {
        return -1;
    }
    
    uint8_t seg_id = strip->num_segments;
    Segment_t* seg = &strip->segments[seg_id];
    
    memset(seg, 0, sizeof(Segment_t));
    seg->start = start;
    seg->end = end;
    seg->effect = EFFECT_STATIC;
    seg->params = DEFAULT_EFFECT_PARAMS;
    seg->active = true;
    seg->paused = false;
    
    strip->num_segments++;
    
    return seg_id;
}

bool segment_delete(LedStrip_t* strip, uint8_t seg_id) {
    if (!strip || seg_id >= strip->num_segments) {
        return false;
    }
    
    // 移动后面的线段
    for (uint8_t i = seg_id; i < strip->num_segments - 1; i++) {
        strip->segments[i] = strip->segments[i + 1];
    }
    strip->num_segments--;
    
    return true;
}

bool segment_set_effect(LedStrip_t* strip, uint8_t seg_id,
                        EffectType_t effect, EffectParams_t* params) {
    if (!strip || seg_id >= strip->num_segments) {
        return false;
    }
    
    Segment_t* seg = &strip->segments[seg_id];
    seg->effect = effect;
    
    if (params) {
        seg->params = *params;
    }
    
    // 重置效果状态
    seg->step = 0;
    seg->aux0 = 0;
    seg->aux1 = 0;
    seg->last_update = 0;
    
    return true;
}

bool segment_set_color(LedStrip_t* strip, uint8_t seg_id,
                       Color_t color1, Color_t color2, Color_t color3) {
    if (!strip || seg_id >= strip->num_segments) {
        return false;
    }
    
    strip->segments[seg_id].params.color1 = color1;
    strip->segments[seg_id].params.color2 = color2;
    strip->segments[seg_id].params.color3 = color3;
    
    return true;
}

bool segment_set_speed(LedStrip_t* strip, uint8_t seg_id, uint16_t speed) {
    if (!strip || seg_id >= strip->num_segments) {
        return false;
    }
    
    strip->segments[seg_id].params.speed = speed;
    return true;
}

bool segment_pause(LedStrip_t* strip, uint8_t seg_id) {
    if (!strip || seg_id >= strip->num_segments) {
        return false;
    }
    strip->segments[seg_id].paused = true;
    return true;
}

bool segment_resume(LedStrip_t* strip, uint8_t seg_id) {
    if (!strip || seg_id >= strip->num_segments) {
        return false;
    }
    strip->segments[seg_id].paused = false;
    return true;
}

/*============================================================================
 * LED颜色操作实现
 *============================================================================*/

void led_set_color(LedStrip_t* strip, uint16_t index, Color_t color) {
    if (!strip || index >= strip->num_leds) return;
    strip->leds[index] = color;
    strip->dirty = true;
}

Color_t led_get_color(LedStrip_t* strip, uint16_t index) {
    if (!strip || index >= strip->num_leds) {
        return COLOR_BLACK;
    }
    return strip->leds[index];
}

void led_set_range(LedStrip_t* strip, uint16_t start, uint16_t end, Color_t color) {
    if (!strip) return;
    
    if (end >= strip->num_leds) end = strip->num_leds - 1;
    if (start > end) return;
    
    for (uint16_t i = start; i <= end; i++) {
        strip->leds[i] = color;
    }
    strip->dirty = true;
}

void led_fill(LedStrip_t* strip, Color_t color) {
    if (!strip) return;
    led_set_range(strip, 0, strip->num_leds - 1, color);
}

void led_clear(LedStrip_t* strip) {
    led_fill(strip, COLOR_BLACK);
}

/*============================================================================
 * 亮度控制实现
 *============================================================================*/

void strip_set_brightness(LedStrip_t* strip, uint8_t brightness) {
    if (!strip) return;
    strip->global_brightness = brightness;
    strip->dirty = true;
}

void segment_set_brightness(LedStrip_t* strip, uint8_t seg_id, uint8_t brightness) {
    if (!strip || seg_id >= strip->num_segments) return;
    strip->segments[seg_id].params.brightness = brightness;
}
```

## 3. 效果实现 (led_effects.c)

```c
#include "led_strip.h"

/*============================================================================
 * 内部辅助函数
 *============================================================================*/

// 线段内设置LED颜色
static void seg_set_pixel(LedStrip_t* strip, Segment_t* seg, 
                          uint16_t index, Color_t color) {
    uint16_t actual_index;
    uint16_t seg_len = seg->end - seg->start + 1;
    
    if (index >= seg_len) return;
    
    if (seg->params.reverse) {
        actual_index = seg->end - index;
    } else {
        actual_index = seg->start + index;
    }
    
    // 应用线段亮度
    color = color_scale(color, seg->params.brightness);
    strip->leds[actual_index] = color;
}

// 线段内获取LED颜色
static Color_t seg_get_pixel(LedStrip_t* strip, Segment_t* seg, uint16_t index) {
    uint16_t seg_len = seg->end - seg->start + 1;
    if (index >= seg_len) return COLOR_BLACK;
    
    uint16_t actual_index;
    if (seg->params.reverse) {
        actual_index = seg->end - index;
    } else {
        actual_index = seg->start + index;
    }
    
    return strip->leds[actual_index];
}

// 线段填充颜色
static void seg_fill(LedStrip_t* strip, Segment_t* seg, Color_t color) {
    uint16_t seg_len = seg->end - seg->start + 1;
    for (uint16_t i = 0; i < seg_len; i++) {
        seg_set_pixel(strip, seg, i, color);
    }
}

// 线段清除
static void seg_clear(LedStrip_t* strip, Segment_t* seg) {
    seg_fill(strip, seg, COLOR_BLACK);
}

// 线段淡出
static void seg_fade_all(LedStrip_t* strip, Segment_t* seg, uint8_t fade_value) {
    for (uint16_t i = seg->start; i <= seg->end; i++) {
        Color_t c = strip->leds[i];
        c.r = (c.r > fade_value) ? c.r - fade_value : 0;
        c.g = (c.g > fade_value) ? c.g - fade_value : 0;
        c.b = (c.b > fade_value) ? c.b - fade_value : 0;
        c.w = (c.w > fade_value) ? c.w - fade_value : 0;
        strip->leds[i] = c;
    }
}

// 线段模糊
static void seg_blur(LedStrip_t* strip, Segment_t* seg, uint8_t blur_amount) {
    uint16_t seg_len = seg->end - seg->start + 1;
    if (seg_len < 2) return;
    
    Color_t* temp = (Color_t*)malloc(seg_len * sizeof(Color_t));
    if (!temp) return;
    
    for (uint16_t i = 0; i < seg_len; i++) {
        Color_t c = seg_get_pixel(strip, seg, i);
        Color_t prev = (i > 0) ? seg_get_pixel(strip, seg, i - 1) : c;
        Color_t next = (i < seg_len - 1) ? seg_get_pixel(strip, seg, i + 1) : c;
        
        temp[i].r = (prev.r + c.r * 2 + next.r) / 4;
        temp[i].g = (prev.g + c.g * 2 + next.g) / 4;
        temp[i].b = (prev.b + c.b * 2 + next.b) / 4;
        temp[i].w = (prev.w + c.w * 2 + next.w) / 4;
    }
    
    for (uint16_t i = 0; i < seg_len; i++) {
        seg_set_pixel(strip, seg, i, temp[i]);
    }
    
    free(temp);
}

/*============================================================================
 * 效果实现函数
 *============================================================================*/

// 静态颜色
static void effect_static(LedStrip_t* strip, Segment_t* seg) {
    seg_fill(strip, seg, seg->params.color1);
}

// 闪烁效果
static void effect_blink(LedStrip_t* strip, Segment_t* seg) {
    if (seg->step % 2 == 0) {
        seg_fill(strip, seg, seg->params.color1);
    } else {
        seg_fill(strip, seg, seg->params.color2);
    }
    seg->step++;
}

// 呼吸灯效果
static void effect_breath(LedStrip_t* strip, Segment_t* seg) {
    // 使用正弦波模拟呼吸效果
    static const uint8_t breath_table[] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 2,
        2, 3, 4, 5, 6, 8, 10, 12, 15, 18, 22, 27, 32, 38, 45, 53,
        62, 72, 83, 95, 108, 122, 137, 153, 170, 187, 205, 223, 241, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
    };
    
    uint8_t breath_idx = seg->step % 128;
    uint8_t brightness;
    
    if (breath_idx < 64) {
        brightness = breath_table[breath_idx];
    } else {
        brightness = breath_table[127 - breath_idx];
    }
    
    Color_t c = color_scale(seg->params.color1, brightness);
    seg_fill(strip, seg, c);
    
    seg->step++;
}

// 颜色擦除效果
static void effect_color_wipe(LedStrip_t* strip, Segment_t* seg) {
    uint16_t seg_len = seg->end - seg->start + 1;
    uint16_t pos = seg->step % (seg_len * 2);
    
    if (pos < seg_len) {
        seg_set_pixel(strip, seg, pos, seg->params.color1);
    } else {
        seg_set_pixel(strip, seg, pos - seg_len, seg->params.color2);
    }
    
    seg->step++;
}

// 剧院追逐效果
static void effect_theater_chase(LedStrip_t* strip, Segment_t* seg) {
    uint16_t seg_len = seg->end - seg->start + 1;
    uint8_t spacing = seg->params.density > 0 ? seg->params.density : 3;
    
    seg_clear(strip, seg);
    
    for (uint16_t i = seg->step % spacing; i < seg_len; i += spacing) {
        seg_set_pixel(strip, seg, i, seg->params.color1);
    }
    
    seg->step++;
}

// 彩虹效果
static void effect_rainbow(LedStrip_t* strip, Segment_t* seg) {
    uint16_t seg_len = seg->end - seg->start + 1;
    
    for (uint16_t i = 0; i < seg_len; i++) {
        uint8_t hue = (i * 256 / seg_len + seg->step) & 0xFF;
        seg_set_pixel(strip, seg, i, color_wheel(hue));
    }
    
    seg->step++;
}

// 彩虹循环效果
static void effect_rainbow_cycle(LedStrip_t* strip, Segment_t* seg) {
    uint16_t seg_len = seg->end - seg->start + 1;
    
    for (uint16_t i = 0; i < seg_len; i++) {
        uint8_t hue = ((i * 256 / seg_len) + seg->step) & 0xFF;
        seg_set_pixel(strip, seg, i, color_wheel(hue));
    }
    
    seg->step += 2;
}

// 跑马灯效果
static void effect_running_lights(LedStrip_t* strip, Segment_t* seg) {
    uint16_t seg_len = seg->end - seg->start + 1;
    
    for (uint16_t i = 0; i < seg_len; i++) {
        // 使用正弦波计算亮度
        int16_t wave = (i + seg->step) % seg_len;
        uint8_t brightness = (uint8_t)(128 + 127 * 
            ((wave < seg_len/2) ? (wave * 2.0 / seg_len) : (2.0 - wave * 2.0 / seg_len)));
        
        Color_t c = color_scale(seg->params.color1, brightness);
        seg_set_pixel(strip, seg, i, c);
    }
    
    seg->step++;
}

// 闪烁星星效果
static void effect_twinkle(LedStrip_t* strip, Segment_t* seg) {
    uint16_t seg_len = seg->end - seg->start + 1;
    
    // 淡出所有LED
    seg_fade_all(strip, seg, seg->params.fade_rate / 4);
    
    // 随机点亮一些LED
    uint8_t num_twinkle = seg->params.density > 0 ? seg->params.density : 3;
    for (uint8_t i = 0; i < num_twinkle; i++) {
        if ((random8() % 10) == 0) {
            uint16_t pos = random8() % seg_len;
            seg_set_pixel(strip, seg, pos, seg->params.color1);
        }
    }
    
    seg->step++;
}

// 火花效果
static void effect_sparkle(LedStrip_t* strip, Segment_t* seg) {
    uint16_t seg_len = seg->end - seg->start + 1;
    
    // 背景色
    seg_fill(strip, seg, seg->params.color2);
    
    // 随机火花
    uint16_t pos = random8() % seg_len;
    seg_set_pixel(strip, seg, pos, seg->params.color1);
    
    seg->step++;
}

// 雪花火花效果
static void effect_snow_sparkle(LedStrip_t* strip, Segment_t* seg) {
    uint16_t seg_len = seg->end - seg->start + 1;
    
    // 白色背景
    seg_fill(strip, seg, seg->params.color1);
    
    // 随机闪烁
    if (seg->step % 5 == 0) {
        uint16_t pos = random8() % seg_len;
        seg_set_pixel(strip, seg, pos, COLOR_WHITE);
    }
    
    seg->step++;
}

// 流星效果
static void effect_meteor(LedStrip_t* strip, Segment_t* seg) {
    uint16_t seg_len = seg->end - seg->start + 1;
    uint8_t meteor_size = seg->params.size > 0 ? seg->params.size : 5;
    uint8_t trail_decay = seg->params.fade_rate > 0 ? seg->params.fade_rate : 64;
    
    // 随机衰减拖尾
    for (uint16_t i = 0; i < seg_len; i++) {
        if (random8() % 10 > 5) {
            Color_t c = seg_get_pixel(strip, seg, i);
            c.r = (c.r > trail_decay) ? c.r - trail_decay : 0;
            c.g = (c.g > trail_decay) ? c.g - trail_decay : 0;
            c.b = (c.b > trail_decay) ? c.b - trail_decay : 0;
            seg_set_pixel(strip, seg, i, c);
        }
    }
    
    // 绘制流星
    uint16_t pos = seg->step % (seg_len + meteor_size);
    for (uint8_t j = 0; j < meteor_size; j++) {
        if (pos >= j && (pos - j) < seg_len) {
            seg_set_pixel(strip, seg, pos - j, seg->params.color1);
        }
    }
    
    seg->step++;
}

// 流星雨效果
static void effect_meteor_rain(LedStrip_t* strip, Segment_t* seg) {
    uint16_t seg_len = seg->end - seg->start + 1;
    uint8_t meteor_size = seg->params.size > 0 ? seg->params.size : 3;
    uint8_t trail_decay = seg->params.fade_rate > 0 ? seg->params.fade_rate : 48;
    
    // 衰减所有LED
    for (uint16_t i = 0; i < seg_len; i++) {
        if (random8() % 10 > 3) {
            Color_t c = seg_get_pixel(strip, seg, i);
            c.r = (c.r > trail_decay) ? c.r - trail_decay : 0;
            c.g = (c.g > trail_decay) ? c.g - trail_decay : 0;
            c.b = (c.b > trail_decay) ? c.b - trail_decay : 0;
            seg_set_pixel(strip, seg, i, c);
        }
    }
    
    // 多个流星
    uint8_t num_meteors = 3;
    for (uint8_t m = 0; m < num_meteors; m++) {
        uint16_t pos = (seg->step + m * seg_len / num_meteors) % (seg_len + meteor_size);
        for (uint8_t j = 0; j < meteor_size; j++) {
            if (pos >= j && (pos - j) < seg_len) {
                Color_t c = seg->params.color1;
                c = color_scale(c, 255 - j * (255 / meteor_size));
                seg_set_pixel(strip, seg, pos - j, c);
            }
        }
    }
    
    seg->step++;
}

// 火焰效果
static void effect_fire(LedStrip_t* strip, Segment_t* seg) {
    uint16_t seg_len = seg->end - seg->start + 1;
    
    // 热量数组
    static uint8_t heat[MAX_LEDS_PER_STRIP];
    
    // 冷却
    for (uint16_t i = 0; i < seg_len; i++) {
        uint8_t cool = random8() % ((55 * 10 / seg_len) + 2);
        heat[i] = (heat[i] > cool) ? heat[i] - cool : 0;
    }
    
    // 热量上升
    for (uint16_t i = seg_len - 1; i >= 2; i--) {
        heat[i] = (heat[i - 1] + heat[i - 2] + heat[i - 2]) / 3;
    }
    
    // 随机点火
    if (random8() % 255 < 120) {
        uint8_t y = random8() % 7;
        heat[y] = heat[y] + random8() % (255 - 160) + 160;
    }
    
    // 转换为颜色
    for (uint16_t i = 0; i < seg_len; i++) {
        uint8_t t192 = (heat[i] * 191) / 255;
        uint8_t heatramp = t192 & 0x3F;
        heatramp <<= 2;
        
        Color_t c;
        if (t192 > 0x80) {
            c = color_rgb(255, 255, heatramp);
        } else if (t192 > 0x40) {
            c = color_rgb(255, heatramp, 0);
        } else {
            c = color_rgb(heatramp, 0, 0);
        }
        
        seg_set_pixel(strip, seg, i, c);
    }
    
    seg->step++;
}

// 彗星效果
static void effect_comet(LedStrip_t* strip, Segment_t* seg) {
    uint16_t seg_len = seg->end - seg->start + 1;
    uint8_t comet_size = seg->params.size > 0 ? seg->params.size : 5;
    
    // 淡出
    seg_fade_all(strip, seg, seg->params.fade_rate / 2);
    
    // 绘制彗星
    uint16_t pos = seg->step % seg_len;
    for (uint8_t i = 0; i < comet_size; i++) {
        uint16_t idx = (pos + seg_len - i) % seg_len;
        uint8_t brightness = 255 - (i * 255 / comet_size);
        Color_t c = color_scale(seg->params.color1, brightness);
        seg_set_pixel(strip, seg, idx, c);
    }
    
    seg->step++;
}

// 扫描器效果 (Knight Rider)
static void effect_scanner(LedStrip_t* strip, Segment_t* seg) {
    uint16_t seg_len = seg->end - seg->start + 1;
    uint8_t scanner_size = seg->params.size > 0 ? seg->params.size : 3;
    
    // 淡出
    seg_fade_all(strip, seg, seg->params.fade_rate);
    
    // 计算位置 (来回移动)
    uint16_t cycle = (seg_len - scanner_size) * 2;
    uint16_t pos = seg->step % cycle;
    
    if (pos >= seg_len - scanner_size) {
        pos = cycle - pos;
    }
    
    // 绘制扫描器
    for (uint8_t i = 0; i < scanner_size; i++) {
        seg_set_pixel(strip, seg, pos + i, seg->params.color1);
    }
    
    seg->step++;
}

// 双向扫描器效果
static void effect_dual_scanner(LedStrip_t* strip, Segment_t* seg) {
    uint16_t seg_len = seg->end - seg->start + 1;
    uint8_t scanner_size = seg->params.size > 0 ? seg->params.size : 3;
    
    // 淡出
    seg_fade_all(strip, seg, seg->params.fade_rate);
    
    // 计算位置
    uint16_t half_len = seg_len / 2;
    uint16_t cycle = half_len * 2;
    uint16_t pos = seg->step % cycle;
    
    if (pos >= half_len) {
        pos = cycle - pos;
    }
    
    // 绘制两个扫描器
    for (uint8_t i = 0; i < scanner_size; i++) {
        if (pos + i < half_len) {
            seg_set_pixel(strip, seg, pos + i, seg->params.color1);
            seg_set_pixel(strip, seg, seg_len - 1 - pos - i, seg->params.color2);
        }
    }
    
    seg->step++;
}

// 渐变效果
static void effect_fade(LedStrip_t* strip, Segment_t* seg) {
    uint8_t fade_step = seg->step % 512;
    uint8_t blend;
    
    if (fade_step < 256) {
        blend = fade_step;
    } else {
        blend = 511 - fade_step;
    }
    
    Color_t c = color_blend(seg->params.color1, seg->params.color2, blend);
    seg_fill(strip, seg, c);
    
    seg->step++;
}

// 加载旋转效果
static void effect_loading_spinner(LedStrip_t* strip, Segment_t* seg) {
    uint16_t seg_len = seg->end - seg->start + 1;
    uint8_t spinner_size = seg->params.size > 0 ? seg->params.size : 5;
    
    seg_fill(strip, seg, seg->params.color2);
    
    uint16_t pos = seg->step % seg_len;
    
    for (uint8_t i = 0; i < spinner_size; i++) {
        uint16_t idx = (pos + i) % seg_len;
        uint8_t brightness = 255 - (i * 255 / spinner_size);
        Color_t c = color_scale(seg->params.color1, brightness);
        seg_set_pixel(strip, seg, idx, c);
    }
    
    seg->step++;
}

// 进度条效果
static void effect_progress_bar(LedStrip_t* strip, Segment_t* seg) {
    uint16_t seg_len = seg->end - seg->start + 1;
    uint16_t progress = seg->aux0;  // 使用aux0存储进度 (0-100)
    
    uint16_t filled = (seg_len * progress) / 100;
    
    for (uint16_t i = 0; i < seg_len; i++) {
        if (i < filled) {
            seg_set_pixel(strip, seg, i, seg->params.color1);
        } else {
            seg_set_pixel(strip, seg, i, seg->params.color2);
        }
    }
}

// 脉冲效果
static void effect_pulse(LedStrip_t* strip, Segment_t* seg) {
    uint16_t seg_len = seg->end - seg->start + 1;
    uint16_t center = seg_len / 2;
    
    // 脉冲扩展
    uint16_t radius = seg->step % (center + 1);
    
    seg_fill(strip, seg, seg->params.color2);
    
    // 绘制脉冲
    for (uint16_t i = 0; i < seg_len; i++) {
        int16_t dist = (i > center) ? (i - center) : (center - i);
        if (dist <= radius && dist >= radius - 2) {
            uint8_t brightness = 255 - (radius - dist) * 85;
            Color_t c = color_scale(seg->params.color1, brightness);
            seg_set_pixel(strip, seg, i, c);
        }
    }
    
    seg->step++;
}

// 波浪效果
static void effect_wave(LedStrip_t* strip, Segment_t* seg) {
    uint16_t seg_len = seg->end - seg->start + 1;
    
    for (uint16_t i = 0; i < seg_len; i++) {
        // 正弦波计算
        uint16_t phase = (i * 256 / seg_len + seg->step * 4) % 256;
        uint8_t wave_val;
        
        if (phase < 128) {
            wave_val = phase * 2;
        } else {
            wave_val = (255 - phase) * 2;
        }
        
        Color_t c = color_blend(seg->params.color1, seg->params.color2, wave_val);
        seg_set_pixel(strip, seg, i, c);
    }
    
    seg->step++;
}

// 渐变色效果
static void effect_gradient(LedStrip_t* strip, Segment_t* seg) {
    uint16_t seg_len = seg->end - seg->start + 1;
    
    for (uint16_t i = 0; i < seg_len; i++) {
        uint8_t blend = (i * 255) / (seg_len - 1);
        Color_t c = color_blend(seg->params.color1, seg->params.color2, blend);
        seg_set_pixel(strip, seg, i, c);
    }
}

// 彩虹追逐效果
static void effect_chase_rainbow(LedStrip_t* strip, Segment_t* seg) {
    uint16_t seg_len = seg->end - seg->start + 1;
    uint8_t spacing = seg->params.density > 0 ? seg->params.density : 3;
    
    seg_fill(strip, seg, seg->params.color2);
    
    for (uint16_t i = seg->step % spacing; i < seg_len; i += spacing) {
        uint8_t hue = (i * 256 / seg_len + seg->step * 2) & 0xFF;
        seg_set_pixel(strip, seg, i, color_wheel(hue));
    }
    
    seg->step++;
}

// Larson扫描器效果
static void effect_larson_scanner(LedStrip_t* strip, Segment_t* seg) {
    uint16_t seg_len = seg->end - seg->start + 1;
    uint8_t eye_size = seg->params.size > 0 ? seg->params.size : 4;
    
    // 淡出
    seg_fade_all(strip, seg, seg->params.fade_rate);
    
    // 计算眼睛位置
    uint16_t range = seg_len - eye_size;
    uint16_t cycle = range * 2;
    uint16_t pos = seg->step % cycle;
    
    if (pos >= range) {
        pos = cycle - pos;
    }
    
    // 绘制眼睛
    for (uint8_t i = 0; i < eye_size; i++) {
        uint8_t brightness;
        if (i < eye_size / 2) {
            brightness = 255 * (i + 1) / (eye_size / 2 + 1);
        } else {
            brightness = 255 * (eye_size - i) / (eye_size / 2 + 1);
        }
        Color_t c = color_scale(seg->params.color1, brightness);
        seg_set_pixel(strip, seg, pos + i, c);
    }
    
    seg->step++;
}

// 弹跳球效果
static void effect_bouncing_ball(LedStrip_t* strip, Segment_t* seg) {
    uint16_t seg_len = seg->end - seg->start + 1;
    
    // 物理参数
    static float height = 0;
    static float velocity = 0;
    static const float gravity = 0.5f;
    static const float bounce = 0.9f;
    
    // 更新物理
    velocity += gravity;
    height += velocity;
    
    if (height >= seg_len - 1) {
        height = seg_len - 1;
        velocity = -velocity * bounce;
    }
    
    // 清除并绘制球
    seg_fade_all(strip, seg, 100);
    
    uint16_t pos = (uint16_t)(seg_len - 1 - height);
    seg_set_pixel(strip, seg, pos, seg->params.color1);
    if (pos > 0) {
        seg_set_pixel(strip, seg, pos - 1, color_scale(seg->params.color1, 128));
    }
    
    seg->step++;
}

/*============================================================================
 * 效果调度器
 *============================================================================*/

typedef void (*EffectFunc_t)(LedStrip_t*, Segment_t*);

static const EffectFunc_t effect_functions[EFFECT_MAX] = {
    [EFFECT_NONE]           = NULL,
    [EFFECT_STATIC]         = effect_static,
    [EFFECT_BLINK]          = effect_blink,
    [EFFECT_BREATH]         = effect_breath,
    [EFFECT_COLOR_WIPE]     = effect_color_wipe,
    [EFFECT_THEATER_CHASE]  = effect_theater_chase,
    [EFFECT_RAINBOW]        = effect_rainbow,
    [EFFECT_RAINBOW_CYCLE]  = effect_rainbow_cycle,
    [EFFECT_RUNNING_LIGHTS] = effect_running_lights,
    [EFFECT_TWINKLE]        = effect_twinkle,
    [EFFECT_SPARKLE]        = effect_sparkle,
    [EFFECT_SNOW_SPARKLE]   = effect_snow_sparkle,
    [EFFECT_METEOR]         = effect_meteor,
    [EFFECT_METEOR_RAIN]    = effect_meteor_rain,
    [EFFECT_FIRE]           = effect_fire,
    [EFFECT_COMET]          = effect_comet,
    [EFFECT_SCANNER]        = effect_scanner,
    [EFFECT_DUAL_SCANNER]   = effect_dual_scanner,
    [EFFECT_FADE]           = effect_fade,
    [EFFECT_LOADING_SPINNER]= effect_loading_spinner,
    [EFFECT_PROGRESS_BAR]   = effect_progress_bar,
    [EFFECT_PULSE]          = effect_pulse,
    [EFFECT_WAVE]           = effect_wave,
    [EFFECT_GRADIENT]       = effect_gradient,
    [EFFECT_CHASE_RAINBOW]  = effect_chase_rainbow,
    [EFFECT_LARSON_SCANNER] = effect_larson_scanner,
    [EFFECT_BOUNCING_BALL]  = effect_bouncing_ball,
};

// 更新单个线段
static void segment_update(LedStrip_t* strip, Segment_t* seg, uint32_t now) {
    if (!seg->active || seg->paused) return;
    
    // 检查是否需要更新
    if (now - seg->last_update < seg->params.speed) {
        return;
    }
    seg->last_update = now;
    
    // 执行效果函数
    if (seg->effect < EFFECT_MAX && effect_functions[seg->effect]) {
        effect_functions[seg->effect](strip, seg);
        strip->dirty = true;
    }
}

/*============================================================================
 * 更新与显示
 *============================================================================*/

void strip_update(StripManager_t* manager, LedStrip_t* strip) {
    if (!manager || !strip || !strip->active) return;
    
    uint32_t now = manager->get_tick_ms();
    
    // 更新所有线段
    for (uint8_t i = 0; i < strip->num_segments; i++) {
        segment_update(strip, &strip->segments[i], now);
    }
}

void strip_show(LedStrip_t* strip) {
    if (!strip || !strip->dirty || !strip->send_data) return;
    
    // 将LED颜色转换到缓冲区
    for (uint16_t i = 0; i < strip->num_leds; i++) {
        color_to_buffer(strip->leds[i], strip->color_order,
                       strip->global_brightness,
                       &strip->buffer[i * strip->bytes_per_led]);
    }
    
    // 发送数据
    strip->send_data(strip->buffer, strip->num_leds * strip->bytes_per_led);
    
    strip->dirty = false;
}

void strip_manager_update_all(StripManager_t* manager) {
    if (!manager) return;
    
    for (uint8_t i = 0; i < manager->num_strips; i++) {
        if (manager->strips[i]) {
            strip_update(manager, manager->strips[i]);
            strip_show(manager->strips[i]);
        }
    }
}
```

## 4. 使用示例 (main.c)

```c
#include "led_strip.h"
#include <stdio.h>

/*============================================================================
 * 平台相关实现 (示例)
 *============================================================================*/

// 模拟系统时间
static uint32_t system_tick = 0;
uint32_t get_system_tick(void) {
    return system_tick;
}

// 模拟数据发送 (实际应用中替换为硬件驱动)
void strip1_send_data(uint8_t* data, uint16_t len) {
    // 实际实现: 通过SPI/DMA发送WS2812数据
    printf("Strip1: Sending %d bytes\n", len);
}

void strip2_send_data(uint8_t* data, uint16_t len) {
    printf("Strip2: Sending %d bytes\n", len);
}

/*============================================================================
 * 主程序示例
 *============================================================================*/

int main(void) {
    // 创建灯带管理器
    StripManager_t* manager = strip_manager_create(get_system_tick);
    if (!manager) {
        printf("Failed to create strip manager\n");
        return -1;
    }
    
    // 创建灯带1: 60个LED, WS2812 (GRB顺序)
    LedStrip_t* strip1 = led_strip_create(60, COLOR_ORDER_GRB, strip1_send_data);
    if (!strip1) {
        printf("Failed to create strip1\n");
        return -1;
    }
    strip_manager_add(manager, strip1);
    
    // 创建灯带2: 30个LED, SK6812 RGBW
    LedStrip_t* strip2 = led_strip_create(30, COLOR_ORDER_GRBW, strip2_send_data);
    if (!strip2) {
        printf("Failed to create strip2\n");
        return -1;
    }
    strip_manager_add(manager, strip2);
    
    // 设置全局亮度
    strip_set_brightness(strip1, 128);
    strip_set_brightness(strip2, 200);
    
    /*------------------------------------------------------------------------
     * 灯带1: 分段效果演示
     *------------------------------------------------------------------------*/
    
    // 删除默认线段
    segment_delete(strip1, 0);
    
    // 创建线段1: LED 0-19, 彩虹效果
    int8_t seg1_id = segment_create(strip1, 0, 19);
    if (seg1_id >= 0) {
        EffectParams_t params = DEFAULT_EFFECT_PARAMS;
        params.speed = 30;
        segment_set_effect(strip1, seg1_id, EFFECT_
```





```
继续生成
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-4.5-Opus

已完成生成

## 4. 使用示例 (main.c) - 续

```c
#include "led_strip.h"
#include <stdio.h>

/*============================================================================
 * 平台相关实现 (示例)
 *============================================================================*/

// 模拟系统时间
static uint32_t system_tick = 0;
uint32_t get_system_tick(void) {
    return system_tick;
}

// 模拟数据发送 (实际应用中替换为硬件驱动)
void strip1_send_data(uint8_t* data, uint16_t len) {
    // 实际实现: 通过SPI/DMA发送WS2812数据
    // printf("Strip1: Sending %d bytes\n", len);
}

void strip2_send_data(uint8_t* data, uint16_t len) {
    // printf("Strip2: Sending %d bytes\n", len);
}

/*============================================================================
 * 主程序示例
 *============================================================================*/

int main(void) {
    // 创建灯带管理器
    StripManager_t* manager = strip_manager_create(get_system_tick);
    if (!manager) {
        printf("Failed to create strip manager\n");
        return -1;
    }
    
    // 创建灯带1: 60个LED, WS2812 (GRB顺序)
    LedStrip_t* strip1 = led_strip_create(60, COLOR_ORDER_GRB, strip1_send_data);
    if (!strip1) {
        printf("Failed to create strip1\n");
        return -1;
    }
    strip_manager_add(manager, strip1);
    
    // 创建灯带2: 30个LED, SK6812 RGBW
    LedStrip_t* strip2 = led_strip_create(30, COLOR_ORDER_GRBW, strip2_send_data);
    if (!strip2) {
        printf("Failed to create strip2\n");
        return -1;
    }
    strip_manager_add(manager, strip2);
    
    // 设置全局亮度
    strip_set_brightness(strip1, 128);
    strip_set_brightness(strip2, 200);
    
    /*------------------------------------------------------------------------
     * 灯带1: 分段效果演示
     *------------------------------------------------------------------------*/
    
    // 删除默认线段
    segment_delete(strip1, 0);
    
    // 创建线段1: LED 0-19, 彩虹效果
    int8_t seg1_id = segment_create(strip1, 0, 19);
    if (seg1_id >= 0) {
        EffectParams_t params = DEFAULT_EFFECT_PARAMS;
        params.speed = 30;
        segment_set_effect(strip1, seg1_id, EFFECT_RAINBOW_CYCLE, &params);
    }
    
    // 创建线段2: LED 20-39, 流星效果
    int8_t seg2_id = segment_create(strip1, 20, 39);
    if (seg2_id >= 0) {
        EffectParams_t params = DEFAULT_EFFECT_PARAMS;
        params.color1 = COLOR_CYAN;
        params.speed = 40;
        params.size = 5;
        params.fade_rate = 80;
        segment_set_effect(strip1, seg2_id, EFFECT_METEOR, &params);
    }
    
    // 创建线段3: LED 40-59, 呼吸灯效果
    int8_t seg3_id = segment_create(strip1, 40, 59);
    if (seg3_id >= 0) {
        EffectParams_t params = DEFAULT_EFFECT_PARAMS;
        params.color1 = COLOR_PURPLE;
        params.speed = 20;
        segment_set_effect(strip1, seg3_id, EFFECT_BREATH, &params);
    }
    
    /*------------------------------------------------------------------------
     * 灯带2: 单一效果演示 (RGBW)
     *------------------------------------------------------------------------*/
    
    // 使用默认线段, 设置火焰效果
    EffectParams_t fire_params = DEFAULT_EFFECT_PARAMS;
    fire_params.speed = 25;
    segment_set_effect(strip2, 0, EFFECT_FIRE, &fire_params);
    
    /*------------------------------------------------------------------------
     * 主循环
     *------------------------------------------------------------------------*/
    
    printf("LED Strip Effects Demo Started\n");
    printf("Strip1: 60 LEDs (GRB) - 3 segments with different effects\n");
    printf("Strip2: 30 LEDs (GRBW) - Fire effect\n\n");
    
    while (1) {
        // 更新所有灯带
        strip_manager_update_all(manager);
        
        // 模拟时间流逝 (实际应用中由系统定时器更新)
        system_tick++;
        
        // 模拟延时 (实际应用中可能不需要)
        // delay_ms(1);
        
        // 演示: 每5秒切换一次效果
        if (system_tick % 5000 == 0) {
            static uint8_t effect_index = 0;
            effect_index = (effect_index + 1) % 5;
            
            EffectParams_t params = DEFAULT_EFFECT_PARAMS;
            params.speed = 40;
            
            switch (effect_index) {
                case 0:
                    params.color1 = COLOR_RED;
                    segment_set_effect(strip2, 0, EFFECT_SCANNER, &params);
                    printf("Strip2: Scanner effect\n");
                    break;
                case 1:
                    params.color1 = COLOR_GREEN;
                    params.color2 = COLOR_BLUE;
                    segment_set_effect(strip2, 0, EFFECT_DUAL_SCANNER, &params);
                    printf("Strip2: Dual Scanner effect\n");
                    break;
                case 2:
                    segment_set_effect(strip2, 0, EFFECT_RAINBOW, &params);
                    printf("Strip2: Rainbow effect\n");
                    break;
                case 3:
                    params.color1 = COLOR_ORANGE;
                    segment_set_effect(strip2, 0, EFFECT_COMET, &params);
                    printf("Strip2: Comet effect\n");
                    break;
                case 4:
                    segment_set_effect(strip2, 0, EFFECT_FIRE, &params);
                    printf("Strip2: Fire effect\n");
                    break;
            }
        }
        
        // 演示退出条件
        if (system_tick > 30000) break;
    }
    
    // 清理资源
    strip_manager_destroy(manager);
    
    printf("\nDemo completed\n");
    return 0;
}
```

## 5. 硬件驱动层示例 (hw_ws2812.c)

```c
/*============================================================================
 * WS2812 硬件驱动层 - STM32 示例
 * 使用 SPI + DMA 方式驱动
 *============================================================================*/

#include "led_strip.h"

#ifdef STM32F4XX
#include "stm32f4xx_hal.h"

// SPI句柄
extern SPI_HandleTypeDef hspi1;
extern SPI_HandleTypeDef hspi2;

// DMA缓冲区 (每个bit需要1字节SPI数据)
#define WS2812_RESET_BYTES  50
static uint8_t dma_buffer1[MAX_LEDS_PER_STRIP * 24 + WS2812_RESET_BYTES];
static uint8_t dma_buffer2[MAX_LEDS_PER_STRIP * 24 + WS2812_RESET_BYTES];

// WS2812 时序: 0码 = 0xC0, 1码 = 0xF8 (SPI 6.4MHz)
#define WS2812_BIT_0    0xC0
#define WS2812_BIT_1    0xF8

/*----------------------------------------------------------------------------
 * 将原始数据转换为SPI格式
 *----------------------------------------------------------------------------*/
static void convert_to_spi_format(uint8_t* src, uint16_t src_len, 
                                   uint8_t* dst, uint16_t* dst_len) {
    uint16_t idx = 0;
    
    // 添加复位信号 (低电平)
    for (uint16_t i = 0; i < WS2812_RESET_BYTES; i++) {
        dst[idx++] = 0x00;
    }
    
    // 转换数据
    for (uint16_t i = 0; i < src_len; i++) {
        uint8_t byte = src[i];
        for (int8_t bit = 7; bit >= 0; bit--) {
            dst[idx++] = (byte & (1 << bit)) ? WS2812_BIT_1 : WS2812_BIT_0;
        }
    }
    
    *dst_len = idx;
}

/*----------------------------------------------------------------------------
 * 灯带1数据发送 (SPI1)
 *----------------------------------------------------------------------------*/
void strip1_send_data_hw(uint8_t* data, uint16_t len) {
    uint16_t dma_len;
    convert_to_spi_format(data, len, dma_buffer1, &dma_len);
    
    // 等待上次传输完成
    while (HAL_SPI_GetState(&hspi1) != HAL_SPI_STATE_READY);
    
    // 启动DMA传输
    HAL_SPI_Transmit_DMA(&hspi1, dma_buffer1, dma_len);
}

/*----------------------------------------------------------------------------
 * 灯带2数据发送 (SPI2)
 *----------------------------------------------------------------------------*/
void strip2_send_data_hw(uint8_t* data, uint16_t len) {
    uint16_t dma_len;
    convert_to_spi_format(data, len, dma_buffer2, &dma_len);
    
    while (HAL_SPI_GetState(&hspi2) != HAL_SPI_STATE_READY);
    HAL_SPI_Transmit_DMA(&hspi2, dma_buffer2, dma_len);
}

/*----------------------------------------------------------------------------
 * 获取系统时间 (ms)
 *----------------------------------------------------------------------------*/
uint32_t get_system_tick_hw(void) {
    return HAL_GetTick();
}

#endif // STM32F4XX


/*============================================================================
 * ESP32 示例 - 使用 RMT 外设
 *============================================================================*/

#ifdef ESP32

#include "driver/rmt.h"
#include "esp_timer.h"

#define RMT_TX_CHANNEL_1    RMT_CHANNEL_0
#define RMT_TX_CHANNEL_2    RMT_CHANNEL_1
#define WS2812_T0H_NS       350
#define WS2812_T0L_NS       900
#define WS2812_T1H_NS       900
#define WS2812_T1L_NS       350

static rmt_item32_t ws2812_bit0;
static rmt_item32_t ws2812_bit1;

/*----------------------------------------------------------------------------
 * RMT 初始化
 *----------------------------------------------------------------------------*/
void ws2812_rmt_init(uint8_t gpio_num1, uint8_t gpio_num2) {
    // 配置RMT通道1
    rmt_config_t config1 = RMT_DEFAULT_CONFIG_TX(gpio_num1, RMT_TX_CHANNEL_1);
    config1.clk_div = 2;
    rmt_config(&config1);
    rmt_driver_install(RMT_TX_CHANNEL_1, 0, 0);
    
    // 配置RMT通道2
    rmt_config_t config2 = RMT_DEFAULT_CONFIG_TX(gpio_num2, RMT_TX_CHANNEL_2);
    config2.clk_div = 2;
    rmt_config(&config2);
    rmt_driver_install(RMT_TX_CHANNEL_2, 0, 0);
    
    // 计算时序
    uint32_t counter_clk_hz = 0;
    rmt_get_counter_clock(RMT_TX_CHANNEL_1, &counter_clk_hz);
    float ratio = (float)counter_clk_hz / 1e9;
    
    ws2812_bit0.level0 = 1;
    ws2812_bit0.duration0 = ratio * WS2812_T0H_NS;
    ws2812_bit0.level1 = 0;
    ws2812_bit0.duration1 = ratio * WS2812_T0L_NS;
    
    ws2812_bit1.level0 = 1;
    ws2812_bit1.duration0 = ratio * WS2812_T1H_NS;
    ws2812_bit1.level1 = 0;
    ws2812_bit1.duration1 = ratio * WS2812_T1L_NS;
}

/*----------------------------------------------------------------------------
 * 数据发送
 *----------------------------------------------------------------------------*/
static void ws2812_rmt_send(rmt_channel_t channel, uint8_t* data, uint16_t len) {
    rmt_item32_t* items = malloc(len * 8 * sizeof(rmt_item32_t));
    if (!items) return;
    
    uint32_t idx = 0;
    for (uint16_t i = 0; i < len; i++) {
        uint8_t byte = data[i];
        for (int8_t bit = 7; bit >= 0; bit--) {
            items[idx++] = (byte & (1 << bit)) ? ws2812_bit1 : ws2812_bit0;
        }
    }
    
    rmt_write_items(channel, items, idx, true);
    rmt_wait_tx_done(channel, portMAX_DELAY);
    
    free(items);
}

void strip1_send_data_hw(uint8_t* data, uint16_t len) {
    ws2812_rmt_send(RMT_TX_CHANNEL_1, data, len);
}

void strip2_send_data_hw(uint8_t* data, uint16_t len) {
    ws2812_rmt_send(RMT_TX_CHANNEL_2, data, len);
}

uint32_t get_system_tick_hw(void) {
    return (uint32_t)(esp_timer_get_time() / 1000);
}

#endif // ESP32
```

## 6. 高级功能扩展 (led_strip_advanced.c)

```c
#include "led_strip.h"

/*============================================================================
 * 线段组操作 - 同步控制多个线段
 *============================================================================*/

typedef struct {
    LedStrip_t*     strip;
    uint8_t         segment_ids[MAX_SEGMENTS];
    uint8_t         count;
} SegmentGroup_t;

// 创建线段组
SegmentGroup_t* segment_group_create(void) {
    SegmentGroup_t* group = (SegmentGroup_t*)malloc(sizeof(SegmentGroup_t));
    if (group) {
        memset(group, 0, sizeof(SegmentGroup_t));
    }
    return group;
}

// 添加线段到组
bool segment_group_add(SegmentGroup_t* group, LedStrip_t* strip, uint8_t seg_id) {
    if (!group || !strip || group->count >= MAX_SEGMENTS) {
        return false;
    }
    
    if (group->strip && group->strip != strip) {
        return false;  // 组内线段必须属于同一灯带
    }
    
    group->strip = strip;
    group->segment_ids[group->count++] = seg_id;
    return true;
}

// 设置组内所有线段的效果
bool segment_group_set_effect(SegmentGroup_t* group, EffectType_t effect, 
                              EffectParams_t* params) {
    if (!group || !group->strip) return false;
    
    for (uint8_t i = 0; i < group->count; i++) {
        segment_set_effect(group->strip, group->segment_ids[i], effect, params);
    }
    return true;
}

// 设置组内所有线段的颜色
bool segment_group_set_color(SegmentGroup_t* group, Color_t color1, 
                             Color_t color2, Color_t color3) {
    if (!group || !group->strip) return false;
    
    for (uint8_t i = 0; i < group->count; i++) {
        segment_set_color(group->strip, group->segment_ids[i], color1, color2, color3);
    }
    return true;
}

// 暂停组内所有线段
bool segment_group_pause(SegmentGroup_t* group) {
    if (!group || !group->strip) return false;
    
    for (uint8_t i = 0; i < group->count; i++) {
        segment_pause(group->strip, group->segment_ids[i]);
    }
    return true;
}

// 恢复组内所有线段
bool segment_group_resume(SegmentGroup_t* group) {
    if (!group || !group->strip) return false;
    
    for (uint8_t i = 0; i < group->count; i++) {
        segment_resume(group->strip, group->segment_ids[i]);
    }
    return true;
}

/*============================================================================
 * 效果过渡 - 平滑切换效果
 *============================================================================*/

typedef struct {
    LedStrip_t*     strip;
    uint8_t         seg_id;
    Color_t*        snapshot;       // 当前状态快照
    EffectType_t    target_effect;
    EffectParams_t  target_params;
    uint16_t        transition_time;
    uint16_t        elapsed;
    bool            active;
} EffectTransition_t;

// 创建效果过渡
EffectTransition_t* effect_transition_create(LedStrip_t* strip, uint8_t seg_id) {
    if (!strip || seg_id >= strip->num_segments) return NULL;
    
    Segment_t* seg = &strip->segments[seg_id];
    uint16_t seg_len = seg->end - seg->start + 1;
    
    EffectTransition_t* trans = (EffectTransition_t*)malloc(sizeof(EffectTransition_t));
    if (!trans) return NULL;
    
    trans->snapshot = (Color_t*)malloc(seg_len * sizeof(Color_t));
    if (!trans->snapshot) {
        free(trans);
        return NULL;
    }
    
    trans->strip = strip;
    trans->seg_id = seg_id;
    trans->active = false;
    
    return trans;
}

// 开始过渡到新效果
bool effect_transition_start(EffectTransition_t* trans, EffectType_t effect,
                            EffectParams_t* params, uint16_t duration_ms) {
    if (!trans || !trans->strip) return false;
    
    Segment_t* seg = &trans->strip->segments[trans->seg_id];
    uint16_t seg_len = seg->end - seg->start + 1;
    
    // 保存当前状态快照
    for (uint16_t i = 0; i < seg_len; i++) {
        trans->snapshot[i] = trans->strip->leds[seg->start + i];
    }
    
    trans->target_effect = effect;
    if (params) {
        trans->target_params = *params;
    }
    trans->transition_time = duration_ms;
    trans->elapsed = 0;
    trans->active = true;
    
    return true;
}

// 更新过渡
void effect_transition_update(EffectTransition_t* trans, uint32_t delta_ms) {
    if (!trans || !trans->active) return;
    
    trans->elapsed += delta_ms;
    
    if (trans->elapsed >= trans->transition_time) {
        // 过渡完成, 切换到目标效果
        segment_set_effect(trans->strip, trans->seg_id, 
                          trans->target_effect, &trans->target_params);
        trans->active = false;
        return;
    }
    
    // 计算混合比例
    uint8_t blend = (trans->elapsed * 255) / trans->transition_time;
    
    Segment_t* seg = &trans->strip->segments[trans->seg_id];
    uint16_t seg_len = seg->end - seg->start + 1;
    
    // 临时应用目标效果
    segment_set_effect(trans->strip, trans->seg_id, 
                      trans->target_effect, &trans->target_params);
    
    // 混合快照和新效果
    for (uint16_t i = 0; i < seg_len; i++) {
        Color_t current = trans->strip->leds[seg->start + i];
        Color_t blended = color_blend(trans->snapshot[i], current, blend);
        trans->strip->leds[seg->start + i] = blended;
    }
}

// 销毁过渡
void effect_transition_destroy(EffectTransition_t* trans) {
    if (!trans) return;
    if (trans->snapshot) free(trans->snapshot);
    free(trans);
}

/*============================================================================
 * 效果播放列表 - 自动循环播放多个效果
 *============================================================================*/

#define MAX_PLAYLIST_ITEMS  16

typedef struct {
    EffectType_t    effect;
    EffectParams_t  params;
    uint32_t        duration_ms;    // 持续时间
    uint16_t        transition_ms;  // 过渡时间
} PlaylistItem_t;

typedef struct {
    LedStrip_t*     strip;
    uint8_t         seg_id;
    PlaylistItem_t  items[MAX_PLAYLIST_ITEMS];
    uint8_t         count;
    uint8_t         current_index;
    uint32_t        item_elapsed;
    bool            loop;
    bool            playing;
    EffectTransition_t* transition;
} EffectPlaylist_t;

// 创建播放列表
EffectPlaylist_t* playlist_create(LedStrip_t* strip, uint8_t seg_id) {
    EffectPlaylist_t* playlist = (EffectPlaylist_t*)malloc(sizeof(EffectPlaylist_t));
    if (!playlist) return NULL;
    
    memset(playlist, 0, sizeof(EffectPlaylist_t));
    playlist->strip = strip;
    playlist->seg_id = seg_id;
    playlist->loop = true;
    playlist->transition = effect_transition_create(strip, seg_id);
    
    return playlist;
}

// 添加效果到播放列表
bool playlist_add(EffectPlaylist_t* playlist, EffectType_t effect,
                  EffectParams_t* params, uint32_t duration_ms, 
                  uint16_t transition_ms) {
    if (!playlist || playlist->count >= MAX_PLAYLIST_ITEMS) {
        return false;
    }
    
    PlaylistItem_t* item = &playlist->items[playlist->count];
    item->effect = effect;
    item->params = params ? *params : DEFAULT_EFFECT_PARAMS;
    item->duration_ms = duration_ms;
    item->transition_ms = transition_ms;
    
    playlist->count++;
    return true;
}

// 开始播放
void playlist_play(EffectPlaylist_t* playlist) {
    if (!playlist || playlist->count == 0) return;
    
    playlist->current_index = 0;
    playlist->item_elapsed = 0;
    playlist->playing = true;
    
    // 应用第一个效果
    PlaylistItem_t* item = &playlist->items[0];
    segment_set_effect(playlist->strip, playlist->seg_id, 
                      item->effect, &item->params);
}

// 停止播放
void playlist_stop(EffectPlaylist_t* playlist) {
    if (!playlist) return;
    playlist->playing = false;
}

// 更新播放列表
void playlist_update(EffectPlaylist_t* playlist, uint32_t delta_ms) {
    if (!playlist || !playlist->playing || playlist->count == 0) return;
    
    // 更新过渡效果
    if (playlist->transition && playlist->transition->active) {
        effect_transition_update(playlist->transition, delta_ms);
        return;
    }
    
    playlist->item_elapsed += delta_ms;
    
    PlaylistItem_t* current = &playlist->items[playlist->current_index];
    
    // 检查是否需要切换到下一个效果
    if (playlist->item_elapsed >= current->duration_ms) {
        playlist->current_index++;
        
        if (playlist->current_index >= playlist->count) {
            if (playlist->loop) {
                playlist->current_index = 0;
            } else {
                playlist->playing = false;
                return;
            }
        }
        
        playlist->item_elapsed = 0;
        PlaylistItem_t* next = &playlist->items[playlist->current_index];
        
        // 使用过渡效果切换
        if (next->transition_ms > 0 && playlist->transition) {
            effect_transition_start(playlist->transition, next->effect,
                                   &next->params, next->transition_ms);
        } else {
            segment_set_effect(playlist->strip, playlist->seg_id,
                              next->effect, &next->params);
        }
    }
}

// 销毁播放列表
void playlist_destroy(EffectPlaylist_t* playlist) {
    if (!playlist) return;
    if (playlist->transition) {
        effect_transition_destroy(playlist->transition);
    }
    free(playlist);
}

/*============================================================================
 * 音频响应效果 (需要外部音频输入)
 *============================================================================*/

typedef struct {
    uint8_t     bass;       // 低频 (0-255)
    uint8_t     mid;        // 中频 (0-255)
    uint8_t     high;       // 高频 (0-255)
    uint8_t     volume;     // 总音量 (0-255)
} AudioData_t;

// VU表效果
void effect_vu_meter(LedStrip_t* strip, Segment_t* seg, AudioData_t* audio) {
    if (!audio) return;
    
    uint16_t seg_len = seg->end - seg->start + 1;
    uint16_t level = (seg_len * audio->volume) / 255;
    
    for (uint16_t i = 0; i < seg_len; i++) {
        Color_t c;
        if (i < level) {
            // 渐变色: 绿 -> 黄 -> 红
            if (i < seg_len / 3) {
                c = COLOR_GREEN;
            } else if (i < seg_len * 2 / 3) {
                c = COLOR_YELLOW;
            } else {
                c = COLOR_RED;
            }
        } else {
            c = COLOR_BLACK;
        }
        strip->leds[seg->start + i] = c;
    }
}

// 频谱效果
void effect_spectrum(LedStrip_t* strip, Segment_t* seg, AudioData_t* audio) {
    if (!audio) return;
    
    uint16_t seg_len = seg->end - seg->start + 1;
    uint16_t third = seg_len / 3;
    
    // 低频 - 红色
    uint16_t bass_level = (third * audio->bass) / 255;
    for (uint16_t i = 0; i < third; i++) {
        Color_t c = (i < bass_level) ? COLOR_RED : COLOR_BLACK;
        strip->leds[seg->start + i] = c;
    }
    
    // 中频 - 绿色
    uint16_t mid_level = (third * audio->mid) / 255;
    for (uint16_t i = 0; i < third; i++) {
        Color_t c = (i < mid_level) ? COLOR_GREEN : COLOR_BLACK;
        strip->leds[seg->start + third + i] = c;
    }
    
    // 高频 - 蓝色
    uint16_t high_level = (third * audio->high) / 255;
    for (uint16_t i = 0; i < third; i++) {
        Color_t c = (i < high_level) ? COLOR_BLUE : COLOR_BLACK;
        strip->leds[seg->start + third * 2 + i] = c;
    }
}

// 节拍脉冲效果
void effect_beat_pulse(LedStrip_t* strip, Segment_t* seg, AudioData_t* audio,
                       uint8_t* last_volume) {
    if (!audio || !last_volume) return;
    
    // 检测节拍 (音量突然增加)
    bool beat = (audio->volume > *last_volume + 30) && (audio->volume > 100);
    *last_volume = audio->volume;
    
    if (beat) {
        // 闪烁
        Color_t c = color_wheel(random8());
        for (uint16_t i = seg->start; i <= seg->end; i++) {
            strip->leds[i] = c;
        }
    } else {
        // 淡出
        for (uint16_t i = seg->start; i <= seg->end; i++) {
            Color_t c = strip->leds[i];
            c.r = (c.r > 10) ? c.r - 10 : 0;
            c.g = (c.g > 10) ? c.g - 10 : 0;
            c.b = (c.b > 10) ? c.b - 10 : 0;
            strip->leds[i] = c;
        }
    }
}
```

## 7. 完整使用示例 - 高级功能

```c
#include "led_strip.h"

// 外部声明
extern uint32_t get_system_tick_hw(void);
extern void strip1_send_data_hw(uint8_t* data, uint16_t len);

/*============================================================================
 * 高级功能演示
 *============================================================================*/

void advanced_demo(void) {
    // 创建管理器和灯带
    StripManager_t* manager = strip_manager_create(get_system_tick_hw);
    LedStrip_t* strip = led_strip_create(120, COLOR_ORDER_GRB, strip1_send_data_hw);
    strip_manager_add(manager, strip);
    
    // 删除默认线段, 创建4个线段
    segment_delete(strip, 0);
    
    int8_t seg_a = segment_create(strip, 0, 29);    // 线段A: 0-29
    int8_t seg_b = segment_create(strip, 30, 59);   // 线段B: 30-59
    int8_t seg_c = segment_create(strip, 60, 89);   // 线段C: 60-89
    int8_t seg_d = segment_create(strip, 90, 119);  // 线段D: 90-119
    
    /*------------------------------------------------------------------------
     * 示例1: 线段组 - 同步控制线段A和C
     *------------------------------------------------------------------------*/
    SegmentGroup_t* group_ac = segment_group_create();
    segment_group_add(group_ac, strip, seg_a);
    segment_group_add(group_ac, strip, seg_c);
    
    EffectParams_t rainbow_params = DEFAULT_EFFECT_PARAMS;
    rainbow_params.speed = 30;
    segment_group_set_effect(group_ac, EFFECT_RAINBOW_CYCLE, &rainbow_params);
    
    /*------------------------------------------------------------------------
     * 示例2: 线段组 - 同步控制线段B和D
     *------------------------------------------------------------------------*/
    SegmentGroup_t* group_bd = segment_group_create();
    segment_group_add(group_bd, strip, seg_b);
    segment_group_add(group_bd, strip, seg_d);
    
    EffectParams_t meteor_params = DEFAULT_EFFECT_PARAMS;
    meteor_params.color1 = COLOR_CYAN;
    meteor_params.speed = 40;
    meteor_params.size = 5;
    segment_group_set_effect(group_bd, EFFECT_METEOR, &meteor_params);
    
    /*------------------------------------------------------------------------
     * 示例3: 效果播放列表 - 线段A自动循环多个效果
     *------------------------------------------------------------------------*/
    EffectPlaylist_t* playlist = playlist_create(strip, seg_a);
    
    // 添加效果序列
    EffectParams_t p1 = DEFAULT_EFFECT_PARAMS;
    p1.color1 = COLOR_RED;
    p1.speed = 50;
    playlist_add(playlist, EFFECT_BREATH, &p1, 5000, 500);  // 呼吸5秒
    
    EffectParams_t p2 = DEFAULT_EFFECT_PARAMS;
    p2.speed = 30;
    playlist_add(playlist, EFFECT_RAINBOW, &p2, 5000, 500); // 彩虹5秒
    
    EffectParams_t p3 = DEFAULT_EFFECT_PARAMS;
    p3.color1 = COLOR_BLUE;
    p3.speed = 40;
    playlist_add(playlist, EFFECT_SCANNER, &p3, 5000, 500); // 扫描5秒
    
    playlist_play(playlist);
    
    /*------------------------------------------------------------------------
     * 主循环
     *------------------------------------------------------------------------*/
    uint32_t last_tick = get_system_tick_hw();
    
    while (1) {
        uint32_t now = get_system_tick_hw();
        uint32_t delta = now - last_tick;
        last_tick = now;
        
        // 更新播放列表
        playlist_update(playlist, delta);
        
        // 更新所有灯带
        strip_manager_update_all(manager);
        
        // 延时
        // delay_ms(1);
    }
    
    // 清理
    playlist_destroy(playlist);
    free(group_ac);
    free(group_bd);
    strip_manager_destroy(manager);
}

/*============================================================================
 * 进度条效果演示 - 适用于加载/等待场景
 *============================================================================*/

void loading_demo(void) {
    StripManager_t* manager = strip_manager_create(get_system_tick_hw);
    LedStrip_t* strip = led_strip_create(30, COLOR_ORDER_GRB, strip1_send_data_hw);
    strip_manager_add(manager, strip);
    
    // 设置加载旋转效果
    EffectParams_t spinner_params = DEFAULT_EFFECT_PARAMS;
    spinner_params.color1 = COLOR_BLUE;
    spinner_params.color2 = color_rgb(20, 20, 40);  // 暗蓝色背景
    spinner_params.speed = 30;
    spinner_params.size = 6;
    
    segment_set_effect(strip, 0, EFFECT_LOADING_SPINNER, &spinner_params);
    
    // 模拟加载过程
    for (int progress = 0; progress <= 100; progress++) {
        // 更新进度 (存储在aux0中)
        strip->segments[0].aux0 = progress;
        
        // 当进度达到100%时切换到进度条效果
        if (progress == 50) {
            EffectParams_t bar_params = DEFAULT_EFFECT_PARAMS;
            bar_params.color1 = COLOR_GREEN;
            bar_params.color2 = color_rgb(30, 30, 30);
            segment_set_effect(strip, 0, EFFECT_PROGRESS_BAR, &bar_params);
        }
        
        strip_manager_update_all(manager);
        // delay_ms(100);
    }
    
    // 完成后显示静态绿色
    EffectParams_t done_params = DEFAULT_EFFECT_PARAMS;
    done_params.color1 = COLOR_GREEN;
    segment_set_effect(strip, 0, EFFECT_STATIC, &done_params);
    strip_manager_update_all(manager);
    
    strip_manager_destroy(manager);
}
```

## 8. 架构总结

```text
┌─────────────────────────────────────────────────────────────────┐
│                      应用层 (Application)                        │
│  - 效果播放列表 (Playlist)                                       │
│  - 线段组控制 (Segment Group)                                    │
│  - 效果过渡 (Transition)                                         │
│  - 音频响应 (Audio Reactive)                                     │
├─────────────────────────────────────────────────────────────────┤
│                      效果层 (Effects)                            │
│  - 28+ 内置效果                                                  │
│  - 可扩展效果函数接口                                            │
├─────────────────────────────────────────────────────────────────┤
│                      核心层 (Core)                               │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐          │
│  │ StripManager │──│   LedStrip   │──│   Segment    │          │
│  │  (管理器)    │  │   (灯带)     │  │   (线段)     │          │
│  └──────────────┘  └──────────────┘  └──────────────┘          │
│  - 多灯带管理      - 多色彩顺序      - 独立效果参数              │
│  - 统一时间源      - 全局亮度        - 暂停/恢复                 │
├─────────────────────────────────────────────────────────────────┤
│                      颜色工具 (Color Utils)                      │
│  - RGB/HSV转换  - 色轮  - 混合  - 缩放                          │
├─────────────────────────────────────────────────────────────────┤
│                      硬件抽象层 (HAL)                            │
│  - STM32 SPI+DMA  - ESP32 RMT  - 其他平台...                    │
└─────────────────────────────────────────────────────────────────┘
```

## 主要特性

|      特性      |              说明              |
| :------------: | :----------------------------: |
| **多灯带支持** |      最多8条灯带独立控制       |
| **多色彩顺序** |  RGB, GRB, BGR, RGBW, GRBW等   |
|  **线段操作**  |    每条灯带最多16个独立线段    |
|  **28+效果**   | 呼吸、彩虹、流星、火焰、扫描等 |
|  **效果参数**  |  速度、亮度、颜色、大小等可调  |
|   **线段组**   |        同步控制多个线段        |
|  **播放列表**  |      自动循环播放多个效果      |
|  **平滑过渡**  |       效果切换时平滑过渡       |
|  **硬件抽象**  |     支持STM32、ESP32等平台     |





```
可用参考WS2812FX 的效果，增加其效果嚒
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-4.5-Opus

已完成生成

# WS2812FX 风格效果库扩展

参考 WS2812FX 库，我将大幅扩展效果数量，增加更多经典和创意效果。

## 1. 扩展效果枚举 (led_effects_def.h)

```c
#ifndef LED_EFFECTS_DEF_H
#define LED_EFFECTS_DEF_H

/*============================================================================
 * WS2812FX 风格效果枚举 - 70+ 效果
 *============================================================================*/
typedef enum {
    // ==================== 基础效果 (0-9) ====================
    FX_STATIC = 0,              // 静态颜色
    FX_BLINK,                   // 闪烁
    FX_BREATH,                  // 呼吸
    FX_COLOR_WIPE,              // 颜色擦除
    FX_COLOR_WIPE_INV,          // 反向颜色擦除
    FX_COLOR_WIPE_REV,          // 颜色擦除反向
    FX_COLOR_WIPE_REV_INV,      // 反向颜色擦除反向
    FX_COLOR_WIPE_RANDOM,       // 随机颜色擦除
    FX_RANDOM_COLOR,            // 随机颜色
    FX_SINGLE_DYNAMIC,          // 单点动态
    
    // ==================== 多点动态效果 (10-19) ====================
    FX_MULTI_DYNAMIC,           // 多点动态
    FX_RAINBOW,                 // 彩虹
    FX_RAINBOW_CYCLE,           // 彩虹循环
    FX_SCAN,                    // 扫描
    FX_DUAL_SCAN,               // 双向扫描
    FX_FADE,                    // 渐变
    FX_THEATER_CHASE,           // 剧院追逐
    FX_THEATER_CHASE_RAINBOW,   // 剧院追逐彩虹
    FX_RUNNING_LIGHTS,          // 跑马灯
    FX_TWINKLE,                 // 闪烁
    
    // ==================== 闪烁效果 (20-29) ====================
    FX_TWINKLE_RANDOM,          // 随机闪烁
    FX_TWINKLE_FADE,            // 闪烁渐隐
    FX_TWINKLE_FADE_RANDOM,     // 随机闪烁渐隐
    FX_SPARKLE,                 // 火花
    FX_FLASH_SPARKLE,           // 闪光火花
    FX_HYPER_SPARKLE,           // 超级火花
    FX_STROBE,                  // 频闪
    FX_STROBE_RAINBOW,          // 彩虹频闪
    FX_MULTI_STROBE,            // 多重频闪
    FX_BLINK_RAINBOW,           // 彩虹闪烁
    
    // ==================== 追逐效果 (30-39) ====================
    FX_CHASE_WHITE,             // 白色追逐
    FX_CHASE_COLOR,             // 颜色追逐
    FX_CHASE_RANDOM,            // 随机追逐
    FX_CHASE_RAINBOW,           // 彩虹追逐
    FX_CHASE_FLASH,             // 闪光追逐
    FX_CHASE_FLASH_RANDOM,      // 随机闪光追逐
    FX_CHASE_RAINBOW_WHITE,     // 彩虹白色追逐
    FX_CHASE_BLACKOUT,          // 黑暗追逐
    FX_CHASE_BLACKOUT_RAINBOW,  // 彩虹黑暗追逐
    FX_COLOR_SWEEP_RANDOM,      // 随机颜色扫描
    
    // ==================== 跑动效果 (40-49) ====================
    FX_RUNNING_COLOR,           // 跑动颜色
    FX_RUNNING_RED_BLUE,        // 红蓝跑动
    FX_RUNNING_RANDOM,          // 随机跑动
    FX_LARSON_SCANNER,          // Larson扫描器
    FX_COMET,                   // 彗星
    FX_FIREWORKS,               // 烟花
    FX_FIREWORKS_RANDOM,        // 随机烟花
    FX_MERRY_CHRISTMAS,         // 圣诞节
    FX_HALLOWEEN,               // 万圣节
    FX_FIRE_FLICKER,            // 火焰闪烁
    
    // ==================== 火焰效果 (50-59) ====================
    FX_FIRE_FLICKER_SOFT,       // 柔和火焰
    FX_FIRE_FLICKER_INTENSE,    // 强烈火焰
    FX_FIRE_2012,               // Fire2012效果
    FX_CIRCUS_COMBUSTUS,        // 马戏团火焰
    FX_BICOLOR_CHASE,           // 双色追逐
    FX_TRICOLOR_CHASE,          // 三色追逐
    FX_TWINKLEFOX,              // TwinkleFOX
    FX_RAIN,                    // 雨滴
    FX_BLOCKS,                  // 色块
    FX_ICU,                     // ICU效果
    
    // ==================== 流星效果 (60-69) ====================
    FX_METEOR,                  // 流星
    FX_METEOR_SMOOTH,           // 平滑流星
    FX_METEOR_RAIN,             // 流星雨
    FX_DISSOLVE,                // 溶解
    FX_DISSOLVE_RANDOM,         // 随机溶解
    FX_POPCORN,                 // 爆米花
    FX_PLASMA,                  // 等离子
    FX_RIPPLE,                  // 涟漪
    FX_RIPPLE_RAINBOW,          // 彩虹涟漪
    FX_HEARTBEAT,               // 心跳
    
    // ==================== 特殊效果 (70-79) ====================
    FX_PACIFICA,                // 太平洋
    FX_PRIDE,                   // Pride效果
    FX_CANDLE,                  // 蜡烛
    FX_CANDLE_MULTI,            // 多蜡烛
    FX_SOLID_GLITTER,           // 闪光固体
    FX_SUNRISE,                 // 日出
    FX_PHASED,                  // 相位
    FX_NOISE,                   // 噪声
    FX_PERLIN_NOISE,            // 柏林噪声
    FX_WASHING_MACHINE,         // 洗衣机
    
    // ==================== 渐变效果 (80-89) ====================
    FX_GRADIENT,                // 渐变
    FX_GRADIENT_CYCLE,          // 渐变循环
    FX_LOADING,                 // 加载
    FX_LOADING_DUAL,            // 双向加载
    FX_POLICE,                  // 警灯
    FX_POLICE_ALL,              // 全警灯
    FX_TWO_DOTS,                // 双点
    FX_TWO_AREAS,               // 双区域
    FX_OSCILLATE,               // 振荡
    FX_SINELON,                 // 正弦波
    
    // ==================== 高级效果 (90-99) ====================
    FX_SINELON_DUAL,            // 双正弦波
    FX_SINELON_RAINBOW,         // 彩虹正弦波
    FX_JUGGLE,                  // 杂耍
    FX_PALETTE,                 // 调色板
    FX_PALETTE_CYCLE,           // 调色板循环
    FX_BPM,                     // BPM节拍
    FX_FILL_NOISE,              // 噪声填充
    FX_COLORFUL,                // 多彩
    FX_TRAFFIC_LIGHT,           // 交通灯
    FX_SPOTS,                   // 光斑
    
    // ==================== 扩展效果 (100+) ====================
    FX_SPOTS_FADE,              // 光斑渐隐
    FX_GLITTER,                 // 闪光
    FX_PERCENT,                 // 百分比
    FX_STACKER,                 // 堆叠
    FX_BOUNCING_BALLS,          // 弹跳球
    FX_BOUNCING_BALLS_MULTI,    // 多弹跳球
    FX_LIGHTHOUSE,              // 灯塔
    FX_FAIRY,                   // 仙女
    FX_FAIRY_TWINKLE,           // 仙女闪烁
    FX_DRIP,                    // 水滴
    
    FX_MODE_COUNT               // 效果总数
} FxMode_t;

/*============================================================================
 * 效果名称字符串 (用于调试/显示)
 *============================================================================*/
static const char* const FX_MODE_NAMES[] = {
    "Static", "Blink", "Breath", "Color Wipe", "Color Wipe Inv",
    "Color Wipe Rev", "Color Wipe Rev Inv", "Color Wipe Random",
    "Random Color", "Single Dynamic", "Multi Dynamic", "Rainbow",
    "Rainbow Cycle", "Scan", "Dual Scan", "Fade", "Theater Chase",
    "Theater Chase Rainbow", "Running Lights", "Twinkle",
    "Twinkle Random", "Twinkle Fade", "Twinkle Fade Random",
    "Sparkle", "Flash Sparkle", "Hyper Sparkle", "Strobe",
    "Strobe Rainbow", "Multi Strobe", "Blink Rainbow",
    "Chase White", "Chase Color", "Chase Random", "Chase Rainbow",
    "Chase Flash", "Chase Flash Random", "Chase Rainbow White",
    "Chase Blackout", "Chase Blackout Rainbow", "Color Sweep Random",
    "Running Color", "Running Red Blue", "Running Random",
    "Larson Scanner", "Comet", "Fireworks", "Fireworks Random",
    "Merry Christmas", "Halloween", "Fire Flicker",
    "Fire Flicker Soft", "Fire Flicker Intense", "Fire 2012",
    "Circus Combustus", "Bicolor Chase", "Tricolor Chase",
    "TwinkleFOX", "Rain", "Blocks", "ICU",
    "Meteor", "Meteor Smooth", "Meteor Rain", "Dissolve",
    "Dissolve Random", "Popcorn", "Plasma", "Ripple",
    "Ripple Rainbow", "Heartbeat", "Pacifica", "Pride",
    "Candle", "Candle Multi", "Solid Glitter", "Sunrise",
    "Phased", "Noise", "Perlin Noise", "Washing Machine",
    "Gradient", "Gradient Cycle", "Loading", "Loading Dual",
    "Police", "Police All", "Two Dots", "Two Areas",
    "Oscillate", "Sinelon", "Sinelon Dual", "Sinelon Rainbow",
    "Juggle", "Palette", "Palette Cycle", "BPM",
    "Fill Noise", "Colorful", "Traffic Light", "Spots",
    "Spots Fade", "Glitter", "Percent", "Stacker",
    "Bouncing Balls", "Bouncing Balls Multi", "Lighthouse",
    "Fairy", "Fairy Twinkle", "Drip"
};

#endif // LED_EFFECTS_DEF_H
```

## 2. 扩展效果实现 (led_effects_ext.c)

```c
#include "led_strip.h"
#include "led_effects_def.h"
#include <math.h>

/*============================================================================
 * 数学工具函数
 *============================================================================*/

// 快速正弦查找表 (0-255 映射到 0-255)
static const uint8_t sin8_table[256] = {
    128,131,134,137,140,143,146,149,152,155,158,162,165,167,170,173,
    176,179,182,185,188,190,193,196,198,201,203,206,208,211,213,215,
    218,220,222,224,226,228,230,232,234,235,237,238,240,241,243,244,
    245,246,248,249,250,250,251,252,253,253,254,254,254,255,255,255,
    255,255,255,255,254,254,254,253,253,252,251,250,250,249,248,246,
    245,244,243,241,240,238,237,235,234,232,230,228,226,224,222,220,
    218,215,213,211,208,206,203,201,198,196,193,190,188,185,182,179,
    176,173,170,167,165,162,158,155,152,149,146,143,140,137,134,131,
    128,124,121,118,115,112,109,106,103,100,97,93,90,88,85,82,
    79,76,73,70,67,65,62,59,57,54,52,49,47,44,42,40,
    37,35,33,31,29,27,25,23,21,20,18,17,15,14,12,11,
    10,9,7,6,5,5,4,3,2,2,1,1,1,0,0,0,
    0,0,0,0,1,1,1,2,2,3,4,5,5,6,7,9,
    10,11,12,14,15,17,18,20,21,23,25,27,29,31,33,35,
    37,40,42,44,47,49,52,54,57,59,62,65,67,70,73,76,
    79,82,85,88,90,93,97,100,103,106,109,112,115,118,121,124
};

static inline uint8_t sin8(uint8_t x) {
    return sin8_table[x];
}

static inline uint8_t cos8(uint8_t x) {
    return sin8_table[(uint8_t)(x + 64)];
}

// 三角波
static inline uint8_t triwave8(uint8_t x) {
    if (x < 128) {
        return x * 2;
    } else {
        return 255 - (x - 128) * 2;
    }
}

// 立方波 (更平滑的呼吸效果)
static inline uint8_t cubicwave8(uint8_t x) {
    return (triwave8(x) * triwave8(x)) >> 8;
}

// 四次波
static inline uint8_t quadwave8(uint8_t x) {
    return sin8(x);
}

// 随机数
static uint16_t random_seed = 12345;

static inline uint8_t random8(void) {
    random_seed = random_seed * 2053 + 13849;
    return (uint8_t)(random_seed >> 8);
}

static inline uint8_t random8_max(uint8_t max) {
    return (random8() * max) >> 8;
}

static inline uint8_t random8_range(uint8_t min, uint8_t max) {
    return min + random8_max(max - min);
}

static inline uint16_t random16(void) {
    random_seed = random_seed * 2053 + 13849;
    return random_seed;
}

// 缩放函数
static inline uint8_t scale8(uint8_t val, uint8_t scale) {
    return ((uint16_t)val * (uint16_t)scale) >> 8;
}

static inline uint8_t qadd8(uint8_t a, uint8_t b) {
    uint16_t sum = a + b;
    return (sum > 255) ? 255 : sum;
}

static inline uint8_t qsub8(uint8_t a, uint8_t b) {
    return (a > b) ? a - b : 0;
}

// 线性插值
static inline uint8_t lerp8(uint8_t a, uint8_t b, uint8_t frac) {
    return a + scale8(b - a, frac);
}

/*============================================================================
 * 内部辅助函数
 *============================================================================*/

// 线段长度
static inline uint16_t seg_len(Segment_t* seg) {
    return seg->end - seg->start + 1;
}

// 设置线段内像素
static void seg_set_pixel(LedStrip_t* strip, Segment_t* seg, 
                          uint16_t i, Color_t color) {
    uint16_t len = seg_len(seg);
    if (i >= len) return;
    
    uint16_t idx = seg->params.reverse ? (seg->end - i) : (seg->start + i);
    
    // 镜像模式
    if (seg->params.mirror && i >= len / 2) {
        idx = seg->params.reverse ? 
              (seg->start + (i - len/2)) : (seg->end - (i - len/2));
    }
    
    color = color_scale(color, seg->params.brightness);
    strip->leds[idx] = color;
}

// 获取线段内像素
static Color_t seg_get_pixel(LedStrip_t* strip, Segment_t* seg, uint16_t i) {
    uint16_t len = seg_len(seg);
    if (i >= len) return COLOR_BLACK;
    
    uint16_t idx = seg->params.reverse ? (seg->end - i) : (seg->start + i);
    return strip->leds[idx];
}

// 填充线段
static void seg_fill(LedStrip_t* strip, Segment_t* seg, Color_t color) {
    for (uint16_t i = seg->start; i <= seg->end; i++) {
        strip->leds[i] = color_scale(color, seg->params.brightness);
    }
}

// 清除线段
static void seg_clear(LedStrip_t* strip, Segment_t* seg) {
    seg_fill(strip, seg, COLOR_BLACK);
}

// 淡出线段
static void seg_fade_out(LedStrip_t* strip, Segment_t* seg, uint8_t rate) {
    for (uint16_t i = seg->start; i <= seg->end; i++) {
        Color_t c = strip->leds[i];
        c.r = scale8(c.r, 255 - rate);
        c.g = scale8(c.g, 255 - rate);
        c.b = scale8(c.b, 255 - rate);
        c.w = scale8(c.w, 255 - rate);
        strip->leds[i] = c;
    }
}

// 模糊线段
static void seg_blur(LedStrip_t* strip, Segment_t* seg, uint8_t blur_amount) {
    uint8_t keep = 255 - blur_amount;
    uint8_t seep = blur_amount >> 1;
    
    Color_t carryover = COLOR_BLACK;
    
    for (uint16_t i = seg->start; i <= seg->end; i++) {
        Color_t cur = strip->leds[i];
        Color_t part;
        
        part.r = scale8(cur.r, seep);
        part.g = scale8(cur.g, seep);
        part.b = scale8(cur.b, seep);
        
        cur.r = scale8(cur.r, keep);
        cur.g = scale8(cur.g, keep);
        cur.b = scale8(cur.b, keep);
        
        cur.r = qadd8(cur.r, carryover.r);
        cur.g = qadd8(cur.g, carryover.g);
        cur.b = qadd8(cur.b, carryover.b);
        
        if (i > seg->start) {
            strip->leds[i - 1].r = qadd8(strip->leds[i - 1].r, part.r);
            strip->leds[i - 1].g = qadd8(strip->leds[i - 1].g, part.g);
            strip->leds[i - 1].b = qadd8(strip->leds[i - 1].b, part.b);
        }
        
        strip->leds[i] = cur;
        carryover = part;
    }
}

/*============================================================================
 * 基础效果实现
 *============================================================================*/

// FX_STATIC - 静态颜色
static void fx_static(LedStrip_t* strip, Segment_t* seg) {
    seg_fill(strip, seg, seg->params.color1);
}

// FX_BLINK - 闪烁
static void fx_blink(LedStrip_t* strip, Segment_t* seg) {
    Color_t c = (seg->step & 1) ? seg->params.color2 : seg->params.color1;
    seg_fill(strip, seg, c);
    seg->step++;
}

// FX_BREATH - 呼吸灯
static void fx_breath(LedStrip_t* strip, Segment_t* seg) {
    uint8_t breath = cubicwave8(seg->step);
    Color_t c = color_scale(seg->params.color1, breath);
    seg_fill(strip, seg, c);
    seg->step += 2;
}

// FX_COLOR_WIPE - 颜色擦除
static void fx_color_wipe(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    uint16_t pos = seg->step % (len * 2);
    
    if (pos < len) {
        seg_set_pixel(strip, seg, pos, seg->params.color1);
    } else {
        seg_set_pixel(strip, seg, pos - len, seg->params.color2);
    }
    seg->step++;
}

// FX_COLOR_WIPE_RANDOM - 随机颜色擦除
static void fx_color_wipe_random(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    uint16_t pos = seg->step % len;
    
    if (pos == 0) {
        seg->aux0 = random8();  // 保存随机色相
    }
    
    seg_set_pixel(strip, seg, pos, color_wheel(seg->aux0));
    seg->step++;
}

// FX_RANDOM_COLOR - 随机颜色
static void fx_random_color(LedStrip_t* strip, Segment_t* seg) {
    seg->aux0 = random8_range(seg->aux0, seg->aux0 + 32);
    seg_fill(strip, seg, color_wheel(seg->aux0));
    seg->step++;
}

// FX_SINGLE_DYNAMIC - 单点动态
static void fx_single_dynamic(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    uint16_t pos = random16() % len;
    seg_set_pixel(strip, seg, pos, color_wheel(random8()));
    seg->step++;
}

// FX_MULTI_DYNAMIC - 多点动态
static void fx_multi_dynamic(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    for (uint16_t i = 0; i < len; i++) {
        if (random8() < 16) {
            seg_set_pixel(strip, seg, i, color_wheel(random8()));
        }
    }
    seg->step++;
}

/*============================================================================
 * 彩虹效果
 *============================================================================*/

// FX_RAINBOW - 彩虹
static void fx_rainbow(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    for (uint16_t i = 0; i < len; i++) {
        uint8_t hue = (i * 256 / len) + seg->step;
        seg_set_pixel(strip, seg, i, color_wheel(hue));
    }
    seg->step++;
}

// FX_RAINBOW_CYCLE - 彩虹循环
static void fx_rainbow_cycle(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    for (uint16_t i = 0; i < len; i++) {
        uint8_t hue = ((i * 256 / len) + seg->step) & 0xFF;
        seg_set_pixel(strip, seg, i, color_wheel(hue));
    }
    seg->step += 2;
}

/*============================================================================
 * 扫描效果
 *============================================================================*/

// FX_SCAN - 扫描
static void fx_scan(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    uint8_t size = seg->params.size > 0 ? seg->params.size : 1;
    
    seg_fade_out(strip, seg, seg->params.fade_rate);
    
    uint16_t cycle = (len - size) * 2;
    uint16_t pos = seg->step % cycle;
    
    if (pos >= len - size) {
        pos = cycle - pos;
    }
    
    for (uint8_t i = 0; i < size; i++) {
        seg_set_pixel(strip, seg, pos + i, seg->params.color1);
    }
    
    seg->step++;
}

// FX_DUAL_SCAN - 双向扫描
static void fx_dual_scan(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    uint8_t size = seg->params.size > 0 ? seg->params.size : 1;
    
    seg_fade_out(strip, seg, seg->params.fade_rate);
    
    uint16_t cycle = (len - size) * 2;
    uint16_t pos = seg->step % cycle;
    
    if (pos >= len - size) {
        pos = cycle - pos;
    }
    
    for (uint8_t i = 0; i < size; i++) {
        seg_set_pixel(strip, seg, pos + i, seg->params.color1);
        seg_set_pixel(strip, seg, len - 1 - pos - i, seg->params.color2);
    }
    
    seg->step++;
}

/*============================================================================
 * 剧院追逐效果
 *============================================================================*/

// FX_THEATER_CHASE - 剧院追逐
static void fx_theater_chase(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    uint8_t gap = seg->params.density > 0 ? seg->params.density : 3;
    
    seg_fill(strip, seg, seg->params.color2);
    
    for (uint16_t i = seg->step % gap; i < len; i += gap) {
        seg_set_pixel(strip, seg, i, seg->params.color1);
    }
    
    seg->step++;
}

// FX_THEATER_CHASE_RAINBOW - 剧院追逐彩虹
static void fx_theater_chase_rainbow(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    uint8_t gap = seg->params.density > 0 ? seg->params.density : 3;
    
    seg_fill(strip, seg, COLOR_BLACK);
    
    for (uint16_t i = seg->step % gap; i < len; i += gap) {
        uint8_t hue = (i * 256 / len + seg->step * 2) & 0xFF;
        seg_set_pixel(strip, seg, i, color_wheel(hue));
    }
    
    seg->step++;
}

/*============================================================================
 * 跑马灯效果
 *============================================================================*/

// FX_RUNNING_LIGHTS - 跑马灯
static void fx_running_lights(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    for (uint16_t i = 0; i < len; i++) {
        uint8_t level = sin8((i * 256 / len) + seg->step * 4);
        Color_t c = color_scale(seg->params.color1, level);
        seg_set_pixel(strip, seg, i, c);
    }
    
    seg->step++;
}

// FX_RUNNING_COLOR - 跑动颜色
static void fx_running_color(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    uint8_t size = seg->params.size > 0 ? seg->params.size : 4;
    
    for (uint16_t i = 0; i < len; i++) {
        uint16_t pos = (i + seg->step) % (size * 2);
        Color_t c = (pos < size) ? seg->params.color1 : seg->params.color2;
        seg_set_pixel(strip, seg, i, c);
    }
    
    seg->step++;
}

// FX_RUNNING_RED_BLUE - 红蓝跑动
static void fx_running_red_blue(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    for (uint16_t i = 0; i < len; i++) {
        uint16_t pos = (i + seg->step) % 6;
        Color_t c;
        if (pos < 2) c = COLOR_RED;
        else if (pos < 4) c = COLOR_WHITE;
        else c = COLOR_BLUE;
        seg_set_pixel(strip, seg, i, c);
    }
    
    seg->step++;
}

// FX_RUNNING_RANDOM - 随机跑动
static void fx_running_random(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    // 移动所有像素
    for (uint16_t i = len - 1; i > 0; i--) {
        strip->leds[seg->start + i] = strip->leds[seg->start + i - 1];
    }
    
    // 添加新的随机颜色
    if ((seg->step % 4) == 0) {
        seg->aux0 = random8();
    }
    strip->leds[seg->start] = color_wheel(seg->aux0);
    
    seg->step++;
}

/*============================================================================
 * 闪烁效果
 *============================================================================*/

// FX_TWINKLE - 闪烁
static void fx_twinkle(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    seg_fade_out(strip, seg, 32);
    
    uint8_t num = seg->params.density > 0 ? seg->params.density : 3;
    for (uint8_t i = 0; i < num; i++) {
        if (random8() < 50) {
            uint16_t pos = random16() % len;
            seg_set_pixel(strip, seg, pos, seg->params.color1);
        }
    }
    
    seg->step++;
}

// FX_TWINKLE_RANDOM - 随机闪烁
static void fx_twinkle_random(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    seg_fade_out(strip, seg, 32);
    
    if (random8() < 50) {
        uint16_t pos = random16() % len;
        seg_set_pixel(strip, seg, pos, color_wheel(random8()));
    }
    
    seg->step++;
}

// FX_TWINKLE_FADE - 闪烁渐隐
static void fx_twinkle_fade(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    seg_fade_out(strip, seg, 16);
    
    if (random8() < 80) {
        uint16_t pos = random16() % len;
        seg_set_pixel(strip, seg, pos, seg->params.color1);
    }
    
    seg->step++;
}

// FX_SPARKLE - 火花
static void fx_sparkle(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    seg_fill(strip, seg, seg->params.color2);
    
    uint16_t pos = random16() % len;
    seg_set_pixel(strip, seg, pos, seg->params.color1);
    
    seg->step++;
}

// FX_FLASH_SPARKLE - 闪光火花
static void fx_flash_sparkle(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    seg_fill(strip, seg, seg->params.color1);
    
    if (random8() < 50) {
        uint16_t pos = random16() % len;
        seg_set_pixel(strip, seg, pos, COLOR_WHITE);
    }
    
    seg->step++;
}

// FX_HYPER_SPARKLE - 超级火花
static void fx_hyper_sparkle(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    seg_fill(strip, seg, seg->params.color1);
    
    for (uint8_t i = 0; i < 8; i++) {
        uint16_t pos = random16() % len;
        seg_set_pixel(strip, seg, pos, COLOR_WHITE);
    }
    
    seg->step++;
}

/*============================================================================
 * 频闪效果
 *============================================================================*/

// FX_STROBE - 频闪
static void fx_strobe(LedStrip_t* strip, Segment_t* seg) {
    if ((seg->step % 6) == 0) {
        seg_fill(strip, seg, seg->params.color1);
    } else {
        seg_fill(strip, seg, seg->params.color2);
    }
    seg->step++;
}

// FX_STROBE_RAINBOW - 彩虹频闪
static void fx_strobe_rainbow(LedStrip_t* strip, Segment_t* seg) {
    if ((seg->step % 6) == 0) {
        seg_fill(strip, seg, color_wheel(seg->aux0));
        seg->aux0 += 16;
    } else {
        seg_fill(strip, seg, COLOR_BLACK);
    }
    seg->step++;
}

// FX_MULTI_STROBE - 多重频闪
static void fx_multi_strobe(LedStrip_t* strip, Segment_t* seg) {
    uint8_t phase = seg->step % 20;
    
    if (phase < 2 || (phase >= 4 && phase < 6) || (phase >= 8 && phase < 10)) {
        seg_fill(strip, seg, seg->params.color1);
    } else {
        seg_fill(strip, seg, COLOR_BLACK);
    }
    
    seg->step++;
}

/*============================================================================
 * 追逐效果
 *============================================================================*/

// FX_CHASE_COLOR - 颜色追逐
static void fx_chase_color(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    uint8_t size = seg->params.size > 0 ? seg->params.size : 3;
    
    seg_fill(strip, seg, seg->params.color2);
    
    uint16_t pos = seg->step % len;
    for (uint8_t i = 0; i < size; i++) {
        seg_set_pixel(strip, seg, (pos + i) % len, seg->params.color1);
    }
    
    seg->step++;
}

// FX_CHASE_RANDOM - 随机追逐
static void fx_chase_random(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    uint8_t size = seg->params.size > 0 ? seg->params.size : 3;
    
    seg_fill(strip, seg, COLOR_BLACK);
    
    uint16_t pos = seg->step % len;
    
    if (pos == 0) {
        seg->aux0 = random8();
    }
    
    for (uint8_t i = 0; i < size; i++) {
        seg_set_pixel(strip, seg, (pos + i) % len, color_wheel(seg->aux0));
    }
    
    seg->step++;
}

// FX_CHASE_RAINBOW - 彩虹追逐
static void fx_chase_rainbow(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    uint8_t size = seg->params.size > 0 ? seg->params.size : 3;
    
    seg_fill(strip, seg, COLOR_BLACK);
    
    uint16_t pos = seg->step % len;
    
    for (uint8_t i = 0; i < size; i++) {
        uint8_t hue = ((pos + i) * 256 / len + seg->step * 2) & 0xFF;
        seg_set_pixel(strip, seg, (pos + i) % len, color_wheel(hue));
    }
    
    seg->step++;
}

// FX_CHASE_FLASH - 闪光追逐
static void fx_chase_flash(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    uint8_t flash_step = seg->step % 10;
    
    seg_fill(strip, seg, seg->params.color2);
    
    uint16_t pos = (seg->step / 10) % len;
    
    if (flash_step < 2) {
        for (uint8_t i = 0; i < 3; i++) {
            seg_set_pixel(strip, seg, (pos + i) % len, seg->params.color1);
        }
    } else if (flash_step < 4) {
        seg_fill(strip, seg, COLOR_WHITE);
    }
    
    seg->step++;
}

// FX_CHASE_BLACKOUT - 黑暗追逐
static void fx_chase_blackout(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    uint8_t size = seg->params.size > 0 ? seg->params.size : 3;
    
    seg_fill(strip, seg, seg->params.color1);
    
    uint16_t pos = seg->step % len;
    for (uint8_t i = 0; i < size; i++) {
        seg_set_pixel(strip, seg, (pos + i) % len, COLOR_BLACK);
    }
    
    seg->step++;
}

/*============================================================================
 * Larson扫描器 (Knight Rider)
 *============================================================================*/

// FX_LARSON_SCANNER - Larson扫描器
static void fx_larson_scanner(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    uint8_t size = seg->params.size > 0 ? seg->params.size : 4;
    
    seg_fade_out(strip, seg, seg->params.fade_rate);
    
    uint16_t range = len - size;
    uint16_t cycle = range * 2;
    uint16_t pos = seg->step % cycle;
    
    if (pos >= range) {
        pos = cycle - pos;
    }
    
    // 绘制眼睛
    for (uint8_t i = 0; i < size; i++) {
        uint8_t brightness;
        uint8_t half = size / 2;
        if (i < half) {
            brightness = 255 * (i + 1) / (half + 1);
        } else {
            brightness = 255 * (size - i) / (half + 1);
        }
        Color_t c = color_scale(seg->params.color1, brightness);
        seg_set_pixel(strip, seg, pos + i, c);
    }
    
    seg->step++;
}

/*============================================================================
 * 彗星效果
 *============================================================================*/

// FX_COMET - 彗星
static void fx_comet(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    uint8_t size = seg->params.size > 0 ? seg->params.size : 8;
    
    seg_fade_out(strip, seg, seg->params.fade_rate);
    
    uint16_t pos = seg->step % len;
    
    for (uint8_t i = 0; i < size; i++) {
        uint16_t idx = (pos + len - i) % len;
        uint8_t brightness = 255 - (i * 255 / size);
        Color_t c = color_scale(seg->params.color1, brightness);
        seg_set_pixel(strip, seg, idx, c);
    }
    
    seg->step++;
}

/*============================================================================
 * 烟花效果
 *============================================================================*/

// FX_FIREWORKS - 烟花
static void fx_fireworks(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    seg_fade_out(strip, seg, 32);
    
    if (random8() < 20) {
        uint16_t pos = random16() % len;
        seg_set_pixel(strip, seg, pos, seg->params.color1);
    }
    
    seg->step++;
}

// FX_FIREWORKS_RANDOM - 随机烟花
static void fx_fireworks_random(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    seg_fade_out(strip, seg, 32);
    
    if (random8() < 20) {
        uint16_t pos = random16() % len;
        seg_set_pixel(strip, seg, pos, color_wheel(random8()));
    }
    
    seg->step++;
}

/*============================================================================
 * 节日效果
 *============================================================================*/

// FX_MERRY_CHRISTMAS - 圣诞节
static void fx_merry_christmas(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    for (uint16_t i = 0; i < len; i++) {
        uint16_t pos = (i + seg->step) % 4;
        Color_t c;
        switch (pos) {
            case 0: c = COLOR_RED; break;
            case 1: c = COLOR_RED; break;
            case 2: c = COLOR_GREEN; break;
            default: c = COLOR_GREEN; break;
        }
        seg_set_pixel(strip, seg, i, c);
    }
    
    seg->step++;
}

// FX_HALLOWEEN - 万圣节
static void fx_halloween(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    for (uint16_t i = 0; i < len; i++) {
        uint16_t pos = (i + seg->step) % 4;
        Color_t c;
        switch (pos) {
            case 0: c = COLOR_ORANGE; break;
            case 1: c = COLOR_ORANGE; break;
            case 2: c = COLOR_PURPLE; break;
            default: c = COLOR_PURPLE; break;
        }
        seg_set_pixel(strip, seg, i, c);
    }
    
    seg->step++;
}

/*============================================================================
 * 火焰效果
 *============================================================================*/

// FX_FIRE_FLICKER - 火焰闪烁
static void fx_fire_flicker(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    for (uint16_t i = 0; i < len; i++) {
        uint8_t flicker = random8_range(150, 255);
        Color_t c = color_rgb(flicker, flicker / 3, 0);
        seg_set_pixel(strip, seg, i, c);
    }
    
    seg->step++;
}

// FX_FIRE_FLICKER_SOFT - 柔和火焰
static void fx_fire_flicker_soft(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    for (uint16_t i = 0; i < len; i++) {
        uint8_t flicker = random8_range(180, 255);
        Color_t c = color_rgb(flicker, flicker / 3, 0);
        seg_set_pixel(strip, seg, i, c);
    }
    
    seg->step++;
}

// FX_FIRE_FLICKER_INTENSE - 强烈火焰
static void fx_fire_flicker_intense(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    for (uint16_t i = 0; i < len; i++) {
        uint8_t flicker = random8_range(100, 255);
        Color_t c = color_rgb(flicker, flicker / 4, 0);
        seg_set_pixel(strip, seg, i, c);
    }
    
    seg->step++;
}

// FX_FIRE_2012 - Fire2012效果 (经典火焰模拟)
static void fx_fire_2012(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    static uint8_t heat[MAX_LEDS_PER_STRIP];
    
    // 冷却
    for (uint16_t i = 0; i < len; i++) {
        uint8_t cool = random8_max((55 * 10 / len) + 2);
        heat[i] = qsub8(heat[i], cool);
    }
    
    // 热量上升
    for (uint16_t i = len - 1; i >= 2; i--) {
        heat[i] = (heat[i - 1] + heat[i - 2] + heat[i - 2]) / 3;
    }
    
    // 随机点火
    if (random8() < 120) {
        uint8_t y = random8_max(7);
        heat[y] = qadd8(heat[y], random8_range(160, 255));
    }
    
    // 转换为颜色
    for (uint16_t i = 0; i < len; i++) {
        uint8_t t192 = scale8(heat[i], 191);
        uint8_t heatramp = t192 & 0x3F;
        heatramp <<= 2;
        
        Color_t c;
        if (t192 > 0x80) {
            c = color_rgb(255, 255, heatramp);
        } else if (t192 > 0x40) {
            c = color_rgb(255, heatramp, 0);
        } else {
            c = color_rgb(heatramp, 0, 0);
        }
        
        seg_set_pixel(strip, seg, i, c);
    }
    
    seg->step++;
}

/*============================================================================
 * 流星效果
 *============================================================================*/

// FX_METEOR - 流星
static void fx_meteor(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    uint8_t size = seg->params.size > 0 ? seg->params.size : 5;
    uint8_t decay = seg->params.fade_rate > 0 ? seg->params.fade_rate : 64;
    
    // 随机衰减
    for (uint16_t i = 0; i < len; i++) {
        if (random8() > 64) {
            Color_t c = seg_get_pixel(strip, seg, i);
            c.r = qsub8(c.r, decay);
            c.g = qsub8(c.g, decay);
            c.b = qsub8(c.b, decay);
            seg_set_pixel(strip, seg, i, c);
        }
    }
    
    // 绘制流星
    uint16_t pos = seg->step % (len + size);
    for (uint8_t j = 0; j < size; j++) {
        if (pos >= j && (pos - j) < len) {
            seg_set_pixel(strip, seg, pos - j, seg->params.color1);
        }
    }
    
    seg->step++;
}

// FX_METEOR_SMOOTH - 平滑流星
static void fx_meteor_smooth(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    uint8_t size = seg->params.size > 0 ? seg->params.size : 8;
    
    seg_fade_out(strip, seg, 48);
    seg_blur(strip, seg, 64);
    
    uint16_t pos = seg->step % (len + size);
    
    for (uint8_t j = 0; j < size; j++) {
        if (pos >= j && (pos - j) < len) {
            uint8_t brightness = 255 - (j * 255 / size);
            Color_t c = color_scale(seg->params.color1, brightness);
            seg_set_pixel(strip, seg, pos - j, c);
        }
    }
    
    seg->step++;
}

// FX_METEOR_RAIN - 流星雨
static void fx_meteor_rain(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    uint8_t size = seg->params.size > 0 ? seg->params.size : 4;
    uint8_t num_meteors = 3;
    
    // 衰减
    for (uint16_t i = 0; i < len; i++) {
        if (random8() > 32) {
            Color_t c = seg_get_pixel(strip, seg, i);
            c.r = qsub8(c.r, 48);
            c.g = qsub8(c.g, 48);
            c.b = qsub8(c.b, 48);
            seg_set_pixel(strip, seg, i, c);
        }
    }
    
    // 多个流星
    for (uint8_t m = 0; m < num_meteors; m++) {
        uint16_t offset = (len / num_meteors) * m;
        uint16_t pos = (seg->step + offset) % (len + size);
        
        for (uint8_t j = 0; j < size; j++) {
            if (pos >= j && (pos - j) < len) {
                uint8_t brightness = 255 - (j * 255 / size);
                Color_t c = color_scale(seg->params.color1, brightness);
                seg_set_pixel(strip, seg, pos - j, c);
            }
        }
    }
    
    seg->step++;
}

/*============================================================================
 * 溶解效果
 *============================================================================*/

// FX_DISSOLVE - 溶解
static void fx_dissolve(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    if (seg->aux0 == 0) {
        seg_fill(strip, seg, seg->params.color1);
        seg->aux0 = 1;
        seg->aux1 = 0;
    }
    
    // 随机溶解像素
    for (uint8_t i = 0; i < 5; i++) {
        uint16_t pos = random16() % len;
        Color_t c = seg_get_pixel(strip, seg, pos);
        if (c.r > 0 || c.g > 0 || c.b > 0) {
            seg_set_pixel(strip, seg, pos, seg->params.color2);
            seg->aux1++;
        }
    }
    
    // 重置
    if (seg->aux1 >= len) {
        seg->aux0 = 0;
    }
    
    seg->step++;
}

// FX_DISSOLVE_RANDOM - 随机溶解
static void fx_dissolve_random(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    if (seg->aux0 == 0) {
        seg->aux0 = random8();  // 随机颜色
        seg_fill(strip, seg, color_wheel(seg->aux0));
        seg->aux1 = 0;
    }
    
    for (uint8_t i = 0; i < 5; i++) {
        uint16_t pos = random16() % len;
        Color_t c = seg_get_pixel(strip, seg, pos);
        if (c.r > 0 || c.g > 0 || c.b > 0) {
            seg_set_pixel(strip, seg, pos, COLOR_BLACK);
            seg->aux1++;
        }
    }
    
    if (seg->aux1 >= len) {
        seg->aux0 = 0;
    }
    
    seg->step++;
}

/*============================================================================
 * 涟漪效果
 *============================================================================*/

// FX_RIPPLE - 涟漪
static void fx_ripple(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    seg_fade_out(strip, seg, 64);
    
    // 涟漪中心和半径
    static uint16_t center = 0;
    static uint8_t radius = 0;
    static uint8_t hue = 0;
    
    if (radius == 0) {
        center = random16() % len;
        hue = random8();
    }
    
    // 绘制涟漪
    uint8_t brightness = 255 - (radius * 8);
    if (brightness > 0) {
        Color_t c = color_scale(color_wheel(hue), brightness);
        
        if (center >= radius) {
            seg_set_pixel(strip, seg, center - radius, c);
        }
        if (center + radius < len) {
            seg_set_pixel(strip, seg, center + radius, c);
        }
    }
    
    radius++;
    if (radius > len / 2) {
        radius = 0;
    }
    
    seg->step++;
}

// FX_RIPPLE_RAINBOW - 彩虹涟漪
static void fx_ripple_rainbow(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    seg_fade_out(strip, seg, 48);
    
    static uint16_t center = 0;
    static uint8_t radius = 0;
    
    if (radius == 0) {
        center = random16() % len;
    }
    
    uint8_t brightness = 255 - (radius * 6);
    if (brightness > 0) {
        uint8_t hue = (seg->step * 2 + radius * 8) & 0xFF;
        Color_t c = color_scale(color_wheel(hue), brightness);
        
        if (center >= radius) {
            seg_set_pixel(strip, seg, center - radius, c);
        }
        if (center + radius < len) {
            seg_set_pixel(strip, seg, center + radius, c);
        }
    }
    
    radius++;
    if (radius > len / 2) {
        radius = 0;
    }
    
    seg->step++;
}

/*============================================================================
 * 心跳效果
 *============================================================================*/

// FX_HEARTBEAT - 心跳
static void fx_heartbeat(LedStrip_t* strip, Segment_t* seg) {
    // 心跳波形: 两次快速脉冲后暂停
    static const uint8_t heartbeat_wave[] = {
        0, 32, 64, 128, 255, 255, 128, 64, 32, 0,
        0, 0, 0, 0, 0,
        0, 32, 64, 128, 200, 200, 128, 64, 32, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    
    uint8_t idx = seg->step % sizeof(heartbeat_wave);
    uint8_t brightness = heartbeat_wave[idx];
    
    Color_t c = color_scale(seg->params.color1, brightness);
    seg_fill(strip, seg, c);
    
    seg->step++;
}

/*============================================================================
 * 等离子效果
 *============================================================================*/

// FX_PLASMA - 等离子
static void fx_plasma(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    for (uint16_t i = 0; i < len; i++) {
        uint8_t val1 = sin8(i * 10 + seg->step);
        uint8_t val2 = sin8(i * 5 + seg->step * 2);
        uint8_t val3 = sin8(i * 2 + seg->step / 2);
        
        uint8_t hue = (val1 + val2 + val3) / 3;
        seg_set_pixel(strip, seg, i, color_wheel(hue));
    }
    
    seg->step++;
}

/*============================================================================
 * 蜡烛效果
 *============================================================================*/

// FX_CANDLE - 蜡烛
static void fx_candle(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    for (uint16_t i = 0; i < len; i++) {
        uint8_t flicker = random8_range(180, 255);
        uint8_t r = scale8(255, flicker);
        uint8_t g = scale8(100, flicker);
        uint8_t b = scale8(20, flicker);
        seg_set_pixel(strip, seg, i, color_rgb(r, g, b));
    }
    
    seg->step++;
}

// FX_CANDLE_MULTI - 多蜡烛
static void fx_candle_multi(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    uint8_t candle_width = 5;
    uint8_t num_candles = len / candle_width;
    
    for (uint8_t c = 0; c < num_candles; c++) {
        uint8_t flicker = random8_range(150, 255);
        
        for (uint8_t i = 0; i < candle_width; i++) {
            uint16_t pos = c * candle_width + i;
            if (pos < len) {
                uint8_t r = scale8(255, flicker);
                uint8_t g = scale8(80, flicker);
                uint8_t b = scale8(10, flicker);
                seg_set_pixel(strip, seg, pos, color_rgb(r, g, b));
            }
        }
    }
    
    seg->step++;
}

/*============================================================================
 * 太平洋效果 (Pacifica)
 *============================================================================*/

// FX_PACIFICA - 太平洋 (海洋波浪效果)
static void fx_pacifica(LedStrip_t* strip, Segment_t*
```





```
继续生成
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-4.5-Opus

已完成生成

## 2. 扩展效果实现 (led_effects_ext.c) - 续

```c
/*============================================================================
 * 太平洋效果 (Pacifica) - 续
 *============================================================================*/

// FX_PACIFICA - 太平洋 (海洋波浪效果)
static void fx_pacifica(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    uint32_t t = seg->step;
    
    // 四层波浪叠加
    for (uint16_t i = 0; i < len; i++) {
        uint16_t idx = i;
        
        // 波浪1
        uint8_t wave1 = sin8(idx * 3 + t);
        // 波浪2
        uint8_t wave2 = sin8(idx * 5 + t * 2);
        // 波浪3
        uint8_t wave3 = sin8(idx * 7 - t);
        // 波浪4
        uint8_t wave4 = sin8(idx * 2 + t / 2);
        
        // 混合波浪
        uint8_t brightness = (wave1 + wave2 + wave3 + wave4) / 4;
        
        // 海洋蓝色调
        uint8_t r = scale8(brightness, 20);
        uint8_t g = scale8(brightness, 100);
        uint8_t b = scale8(brightness, 200);
        
        // 添加白色泡沫
        if (brightness > 220) {
            r = qadd8(r, 50);
            g = qadd8(g, 50);
            b = qadd8(b, 50);
        }
        
        seg_set_pixel(strip, seg, i, color_rgb(r, g, b));
    }
    
    seg->step++;
}

/*============================================================================
 * Pride效果
 *============================================================================*/

// FX_PRIDE - Pride彩虹效果
static void fx_pride(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    uint32_t t = seg->step;
    
    for (uint16_t i = 0; i < len; i++) {
        // 多层正弦波叠加
        uint8_t hue = sin8(i * 2 + t);
        hue = qadd8(hue, sin8(i * 3 - t * 2));
        hue = qadd8(hue, sin8(i + t / 2));
        
        uint8_t sat = 255 - (sin8(i * 4 + t) >> 2);
        uint8_t val = sin8(i * 5 - t) / 2 + 128;
        
        seg_set_pixel(strip, seg, i, color_hsv(hue, sat, val));
    }
    
    seg->step++;
}

/*============================================================================
 * 日出效果
 *============================================================================*/

// FX_SUNRISE - 日出
static void fx_sunrise(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    uint16_t progress = seg->aux0;  // 0-1000
    
    // 日出颜色渐变: 黑 -> 深红 -> 橙 -> 黄 -> 白
    for (uint16_t i = 0; i < len; i++) {
        uint16_t pos_progress = progress + (i * 100 / len);
        if (pos_progress > 1000) pos_progress = 1000;
        
        Color_t c;
        if (pos_progress < 200) {
            // 黑到深红
            uint8_t blend = pos_progress * 255 / 200;
            c = color_rgb(blend / 2, 0, 0);
        } else if (pos_progress < 400) {
            // 深红到橙
            uint8_t blend = (pos_progress - 200) * 255 / 200;
            c = color_rgb(128 + blend / 2, blend / 2, 0);
        } else if (pos_progress < 600) {
            // 橙到黄
            uint8_t blend = (pos_progress - 400) * 255 / 200;
            c = color_rgb(255, 128 + blend / 2, 0);
        } else if (pos_progress < 800) {
            // 黄到暖白
            uint8_t blend = (pos_progress - 600) * 255 / 200;
            c = color_rgb(255, 255, blend);
        } else {
            // 暖白到白
            uint8_t blend = (pos_progress - 800) * 255 / 200;
            c = color_rgb(255, 255, 255);
        }
        
        seg_set_pixel(strip, seg, i, c);
    }
    
    // 缓慢增加进度
    if (seg->aux0 < 1000) {
        seg->aux0++;
    }
    
    seg->step++;
}

/*============================================================================
 * 闪光效果
 *============================================================================*/

// FX_SOLID_GLITTER - 闪光固体
static void fx_solid_glitter(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    // 填充基础颜色
    seg_fill(strip, seg, seg->params.color1);
    
    // 添加随机闪光
    uint8_t num_glitter = seg->params.density > 0 ? seg->params.density : 5;
    for (uint8_t i = 0; i < num_glitter; i++) {
        if (random8() < 80) {
            uint16_t pos = random16() % len;
            seg_set_pixel(strip, seg, pos, COLOR_WHITE);
        }
    }
    
    seg->step++;
}

// FX_GLITTER - 闪光
static void fx_glitter(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    seg_fade_out(strip, seg, 32);
    
    if (random8() < 100) {
        uint16_t pos = random16() % len;
        seg_set_pixel(strip, seg, pos, seg->params.color1);
    }
    
    seg->step++;
}

/*============================================================================
 * 警灯效果
 *============================================================================*/

// FX_POLICE - 警灯
static void fx_police(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    uint16_t half = len / 2;
    uint8_t phase = (seg->step / 4) % 4;
    
    seg_clear(strip, seg);
    
    switch (phase) {
        case 0:
            for (uint16_t i = 0; i < half; i++) {
                seg_set_pixel(strip, seg, i, COLOR_RED);
            }
            break;
        case 1:
            seg_clear(strip, seg);
            break;
        case 2:
            for (uint16_t i = half; i < len; i++) {
                seg_set_pixel(strip, seg, i, COLOR_BLUE);
            }
            break;
        case 3:
            seg_clear(strip, seg);
            break;
    }
    
    seg->step++;
}

// FX_POLICE_ALL - 全警灯
static void fx_police_all(LedStrip_t* strip, Segment_t* seg) {
    uint8_t phase = (seg->step / 3) % 6;
    
    switch (phase) {
        case 0:
        case 1:
            seg_fill(strip, seg, COLOR_RED);
            break;
        case 2:
            seg_fill(strip, seg, COLOR_BLACK);
            break;
        case 3:
        case 4:
            seg_fill(strip, seg, COLOR_BLUE);
            break;
        case 5:
            seg_fill(strip, seg, COLOR_BLACK);
            break;
    }
    
    seg->step++;
}

/*============================================================================
 * 双点/双区域效果
 *============================================================================*/

// FX_TWO_DOTS - 双点
static void fx_two_dots(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    seg_fill(strip, seg, seg->params.color2);
    
    uint16_t pos1 = seg->step % len;
    uint16_t pos2 = (len - 1 - seg->step) % len;
    
    seg_set_pixel(strip, seg, pos1, seg->params.color1);
    seg_set_pixel(strip, seg, pos2, seg->params.color3);
    
    seg->step++;
}

// FX_TWO_AREAS - 双区域
static void fx_two_areas(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    uint16_t half = len / 2;
    
    uint8_t phase = (seg->step / 20) % 2;
    
    for (uint16_t i = 0; i < len; i++) {
        Color_t c;
        if (i < half) {
            c = phase ? seg->params.color1 : seg->params.color2;
        } else {
            c = phase ? seg->params.color2 : seg->params.color1;
        }
        seg_set_pixel(strip, seg, i, c);
    }
    
    seg->step++;
}

/*============================================================================
 * 振荡效果
 *============================================================================*/

// FX_OSCILLATE - 振荡
static void fx_oscillate(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    for (uint16_t i = 0; i < len; i++) {
        uint8_t val = sin8(i * 10 + seg->step * 5);
        
        Color_t c;
        if (val < 85) {
            c = seg->params.color1;
        } else if (val < 170) {
            c = seg->params.color2;
        } else {
            c = seg->params.color3;
        }
        
        seg_set_pixel(strip, seg, i, c);
    }
    
    seg->step++;
}

/*============================================================================
 * 正弦波效果
 *============================================================================*/

// FX_SINELON - 正弦波
static void fx_sinelon(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    seg_fade_out(strip, seg, 20);
    
    uint16_t pos = (sin8(seg->step) * len) >> 8;
    seg_set_pixel(strip, seg, pos, seg->params.color1);
    
    seg->step += 2;
}

// FX_SINELON_DUAL - 双正弦波
static void fx_sinelon_dual(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    seg_fade_out(strip, seg, 20);
    
    uint16_t pos1 = (sin8(seg->step) * len) >> 8;
    uint16_t pos2 = (sin8(seg->step + 128) * len) >> 8;
    
    seg_set_pixel(strip, seg, pos1, seg->params.color1);
    seg_set_pixel(strip, seg, pos2, seg->params.color2);
    
    seg->step += 2;
}

// FX_SINELON_RAINBOW - 彩虹正弦波
static void fx_sinelon_rainbow(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    seg_fade_out(strip, seg, 20);
    
    uint16_t pos = (sin8(seg->step) * len) >> 8;
    seg_set_pixel(strip, seg, pos, color_wheel(seg->step * 2));
    
    seg->step += 2;
}

/*============================================================================
 * 杂耍效果
 *============================================================================*/

// FX_JUGGLE - 杂耍
static void fx_juggle(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    seg_fade_out(strip, seg, 20);
    
    uint8_t dot_hue = 0;
    for (uint8_t i = 0; i < 8; i++) {
        uint16_t pos = (sin8(seg->step + i * 32) * len) >> 8;
        Color_t c = color_wheel(dot_hue);
        
        // 混合颜色
        Color_t existing = seg_get_pixel(strip, seg, pos);
        c.r = qadd8(existing.r, c.r);
        c.g = qadd8(existing.g, c.g);
        c.b = qadd8(existing.b, c.b);
        
        seg_set_pixel(strip, seg, pos, c);
        dot_hue += 32;
    }
    
    seg->step++;
}

/*============================================================================
 * BPM效果
 *============================================================================*/

// FX_BPM - BPM节拍
static void fx_bpm(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    uint8_t bpm = 62;  // 每分钟节拍数
    
    uint8_t beat = (seg->step * bpm * 256 / 60 / 1000) & 0xFF;
    
    for (uint16_t i = 0; i < len; i++) {
        uint8_t hue = (i * 256 / len) + beat;
        uint8_t brightness = sin8(i * 10 + beat);
        
        Color_t c = color_hsv(hue, 255, brightness);
        seg_set_pixel(strip, seg, i, c);
    }
    
    seg->step++;
}

/*============================================================================
 * 噪声效果
 *============================================================================*/

// 简单噪声函数
static uint8_t noise8(uint16_t x, uint16_t y) {
    uint16_t n = x + y * 57;
    n = (n << 13) ^ n;
    return (uint8_t)((n * (n * n * 15731 + 789221) + 1376312589) >> 24);
}

// FX_NOISE - 噪声
static void fx_noise(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    for (uint16_t i = 0; i < len; i++) {
        uint8_t n = noise8(i * 10, seg->step);
        seg_set_pixel(strip, seg, i, color_wheel(n));
    }
    
    seg->step++;
}

// FX_FILL_NOISE - 噪声填充
static void fx_fill_noise(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    for (uint16_t i = 0; i < len; i++) {
        uint8_t n = noise8(i * 5, seg->step / 2);
        uint8_t brightness = n;
        
        Color_t c = color_scale(seg->params.color1, brightness);
        seg_set_pixel(strip, seg, i, c);
    }
    
    seg->step++;
}

/*============================================================================
 * 渐变效果
 *============================================================================*/

// FX_GRADIENT - 渐变
static void fx_gradient(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    for (uint16_t i = 0; i < len; i++) {
        uint8_t blend = (i * 255) / (len - 1);
        Color_t c = color_blend(seg->params.color1, seg->params.color2, blend);
        seg_set_pixel(strip, seg, i, c);
    }
}

// FX_GRADIENT_CYCLE - 渐变循环
static void fx_gradient_cycle(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    for (uint16_t i = 0; i < len; i++) {
        uint8_t blend = ((i * 255 / len) + seg->step) & 0xFF;
        Color_t c = color_blend(seg->params.color1, seg->params.color2, blend);
        seg_set_pixel(strip, seg, i, c);
    }
    
    seg->step++;
}

/*============================================================================
 * 加载效果
 *============================================================================*/

// FX_LOADING - 加载
static void fx_loading(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    uint8_t size = seg->params.size > 0 ? seg->params.size : 5;
    
    seg_fill(strip, seg, seg->params.color2);
    
    uint16_t pos = seg->step % len;
    
    for (uint8_t i = 0; i < size; i++) {
        uint16_t idx = (pos + i) % len;
        uint8_t brightness = 255 - (i * 255 / size);
        Color_t c = color_scale(seg->params.color1, brightness);
        seg_set_pixel(strip, seg, idx, c);
    }
    
    seg->step++;
}

// FX_LOADING_DUAL - 双向加载
static void fx_loading_dual(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    uint8_t size = seg->params.size > 0 ? seg->params.size : 4;
    
    seg_fill(strip, seg, seg->params.color2);
    
    uint16_t pos = seg->step % len;
    
    // 顺时针
    for (uint8_t i = 0; i < size; i++) {
        uint16_t idx = (pos + i) % len;
        uint8_t brightness = 255 - (i * 255 / size);
        Color_t c = color_scale(seg->params.color1, brightness);
        seg_set_pixel(strip, seg, idx, c);
    }
    
    // 逆时针
    for (uint8_t i = 0; i < size; i++) {
        uint16_t idx = (len - 1 - pos + len - i) % len;
        uint8_t brightness = 255 - (i * 255 / size);
        Color_t c = color_scale(seg->params.color3, brightness);
        seg_set_pixel(strip, seg, idx, c);
    }
    
    seg->step++;
}

/*============================================================================
 * 光斑效果
 *============================================================================*/

// FX_SPOTS - 光斑
static void fx_spots(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    uint8_t spacing = seg->params.density > 0 ? seg->params.density : 8;
    uint8_t width = seg->params.size > 0 ? seg->params.size : 3;
    
    seg_fill(strip, seg, seg->params.color2);
    
    uint16_t offset = seg->step % spacing;
    
    for (uint16_t i = offset; i < len; i += spacing) {
        for (uint8_t w = 0; w < width && (i + w) < len; w++) {
            seg_set_pixel(strip, seg, i + w, seg->params.color1);
        }
    }
    
    seg->step++;
}

// FX_SPOTS_FADE - 光斑渐隐
static void fx_spots_fade(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    uint8_t spacing = seg->params.density > 0 ? seg->params.density : 8;
    uint8_t width = seg->params.size > 0 ? seg->params.size : 5;
    
    seg_fill(strip, seg, seg->params.color2);
    
    uint16_t offset = seg->step % spacing;
    
    for (uint16_t i = offset; i < len; i += spacing) {
        for (uint8_t w = 0; w < width && (i + w) < len; w++) {
            // 中心最亮，边缘渐暗
            uint8_t dist = (w < width / 2) ? w : (width - 1 - w);
            uint8_t brightness = 255 - (dist * 255 / (width / 2 + 1));
            Color_t c = color_scale(seg->params.color1, brightness);
            seg_set_pixel(strip, seg, i + w, c);
        }
    }
    
    seg->step++;
}

/*============================================================================
 * 百分比/进度条效果
 *============================================================================*/

// FX_PERCENT - 百分比
static void fx_percent(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    uint8_t percent = seg->aux0;  // 0-100
    
    uint16_t filled = (len * percent) / 100;
    
    for (uint16_t i = 0; i < len; i++) {
        Color_t c;
        if (i < filled) {
            // 渐变色: 绿 -> 黄 -> 红
            if (i < len / 3) {
                c = COLOR_GREEN;
            } else if (i < len * 2 / 3) {
                c = COLOR_YELLOW;
            } else {
                c = COLOR_RED;
            }
        } else {
            c = seg->params.color2;
        }
        seg_set_pixel(strip, seg, i, c);
    }
}

/*============================================================================
 * 堆叠效果
 *============================================================================*/

// FX_STACKER - 堆叠
static void fx_stacker(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    static uint16_t stack_height = 0;
    static uint16_t ball_pos = 0;
    static bool ball_moving = true;
    
    seg_fill(strip, seg, seg->params.color2);
    
    // 绘制已堆叠的部分
    for (uint16_t i = 0; i < stack_height; i++) {
        seg_set_pixel(strip, seg, len - 1 - i, seg->params.color1);
    }
    
    // 移动的球
    if (ball_moving) {
        seg_set_pixel(strip, seg, ball_pos, seg->params.color1);
        ball_pos++;
        
        if (ball_pos >= len - stack_height) {
            ball_moving = false;
            stack_height++;
            
            if (stack_height >= len) {
                stack_height = 0;
            }
        }
    } else {
        ball_pos = 0;
        ball_moving = true;
    }
    
    seg->step++;
}

/*============================================================================
 * 弹跳球效果
 *============================================================================*/

#define MAX_BOUNCING_BALLS 5

typedef struct {
    float height;
    float velocity;
    float damping;
    Color_t color;
} BouncingBall_t;

static BouncingBall_t balls[MAX_BOUNCING_BALLS];
static bool balls_initialized = false;

// FX_BOUNCING_BALLS - 弹跳球
static void fx_bouncing_balls(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    const float gravity = 0.5f;
    
    if (!balls_initialized) {
        balls[0].height = 0;
        balls[0].velocity = 0;
        balls[0].damping = 0.9f;
        balls[0].color = seg->params.color1;
        balls_initialized = true;
    }
    
    seg_fade_out(strip, seg, 64);
    
    // 更新物理
    balls[0].velocity += gravity;
    balls[0].height += balls[0].velocity;
    
    if (balls[0].height >= len - 1) {
        balls[0].height = len - 1;
        balls[0].velocity = -balls[0].velocity * balls[0].damping;
        
        // 重置
        if (balls[0].velocity > -1.0f && balls[0].velocity < 1.0f) {
            balls[0].velocity = -15.0f;
        }
    }
    
    // 绘制球
    uint16_t pos = (uint16_t)(len - 1 - balls[0].height);
    seg_set_pixel(strip, seg, pos, balls[0].color);
    
    seg->step++;
}

// FX_BOUNCING_BALLS_MULTI - 多弹跳球
static void fx_bouncing_balls_multi(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    const float gravity = 0.4f;
    uint8_t num_balls = 3;
    
    if (!balls_initialized) {
        for (uint8_t i = 0; i < num_balls; i++) {
            balls[i].height = 0;
            balls[i].velocity = -10.0f - i * 3;
            balls[i].damping = 0.85f + i * 0.02f;
            balls[i].color = color_wheel(i * 85);
        }
        balls_initialized = true;
    }
    
    seg_fade_out(strip, seg, 48);
    
    for (uint8_t b = 0; b < num_balls; b++) {
        balls[b].velocity += gravity;
        balls[b].height += balls[b].velocity;
        
        if (balls[b].height >= len - 1) {
            balls[b].height = len - 1;
            balls[b].velocity = -balls[b].velocity * balls[b].damping;
            
            if (balls[b].velocity > -1.0f && balls[b].velocity < 1.0f) {
                balls[b].velocity = -12.0f - random8_max(5);
            }
        }
        
        uint16_t pos = (uint16_t)(len - 1 - balls[b].height);
        if (pos < len) {
            seg_set_pixel(strip, seg, pos, balls[b].color);
        }
    }
    
    seg->step++;
}

/*============================================================================
 * 灯塔效果
 *============================================================================*/

// FX_LIGHTHOUSE - 灯塔
static void fx_lighthouse(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    uint8_t beam_width = seg->params.size > 0 ? seg->params.size : 10;
    
    seg_fill(strip, seg, seg->params.color2);
    
    uint16_t pos = seg->step % len;
    
    for (uint8_t i = 0; i < beam_width; i++) {
        uint16_t idx = (pos + i) % len;
        
        // 光束中心最亮
        uint8_t dist = (i < beam_width / 2) ? i : (beam_width - 1 - i);
        uint8_t brightness = 255 - (dist * 255 / (beam_width / 2 + 1));
        
        Color_t c = color_scale(seg->params.color1, brightness);
        seg_set_pixel(strip, seg, idx, c);
    }
    
    seg->step++;
}

/*============================================================================
 * 仙女效果
 *============================================================================*/

// FX_FAIRY - 仙女
static void fx_fairy(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    seg_fade_out(strip, seg, 16);
    
    // 多个仙女光点
    for (uint8_t i = 0; i < 3; i++) {
        uint16_t pos = (sin8(seg->step * 2 + i * 85) * len) >> 8;
        uint8_t brightness = sin8(seg->step * 3 + i * 50);
        
        Color_t c = color_scale(seg->params.color1, brightness);
        seg_set_pixel(strip, seg, pos, c);
    }
    
    seg->step++;
}

// FX_FAIRY_TWINKLE - 仙女闪烁
static void fx_fairy_twinkle(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    seg_fade_out(strip, seg, 24);
    
    // 仙女光点
    for (uint8_t i = 0; i < 3; i++) {
        uint16_t pos = (sin8(seg->step * 2 + i * 85) * len) >> 8;
        uint8_t brightness = sin8(seg->step * 3 + i * 50);
        
        Color_t c = color_scale(seg->params.color1, brightness);
        seg_set_pixel(strip, seg, pos, c);
    }
    
    // 随机闪烁
    if (random8() < 60) {
        uint16_t pos = random16() % len;
        seg_set_pixel(strip, seg, pos, COLOR_WHITE);
    }
    
    seg->step++;
}

/*============================================================================
 * 水滴效果
 *============================================================================*/

// FX_DRIP - 水滴
static void fx_drip(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    static uint16_t drip_pos = 0;
    static uint8_t drip_state = 0;  // 0=积累, 1=下落, 2=溅射
    static uint8_t splash_frame = 0;
    
    seg_fade_out(strip, seg, 32);
    
    switch (drip_state) {
        case 0:  // 积累
            seg_set_pixel(strip, seg, 0, seg->params.color1);
            if ((seg->step % 30) == 0) {
                drip_state = 1;
                drip_pos = 0;
            }
            break;
            
        case 1:  // 下落
            drip_pos += 2;
            if (drip_pos < len) {
                seg_set_pixel(strip, seg, drip_pos, seg->params.color1);
                // 拖尾
                if (drip_pos > 0) {
                    seg_set_pixel(strip, seg, drip_pos - 1, 
                                 color_scale(seg->params.color1, 128));
                }
            } else {
                drip_state = 2;
                splash_frame = 0;
            }
            break;
            
        case 2:  // 溅射
            splash_frame++;
            if (splash_frame < 10) {
                uint8_t spread = splash_frame;
                uint8_t brightness = 255 - splash_frame * 25;
                Color_t c = color_scale(seg->params.color1, brightness);
                
                if (len - 1 >= spread) {
                    seg_set_pixel(strip, seg, len - 1 - spread, c);
                }
                seg_set_pixel(strip, seg, len - 1, c);
            } else {
                drip_state = 0;
            }
            break;
    }
    
    seg->step++;
}

/*============================================================================
 * 雨滴效果
 *============================================================================*/

// FX_RAIN - 雨滴
static void fx_rain(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    // 移动所有像素向下
    for (uint16_t i = len - 1; i > 0; i--) {
        strip->leds[seg->start + i] = strip->leds[seg->start + i - 1];
    }
    
    // 随机生成新雨滴
    if (random8() < 30) {
        strip->leds[seg->start] = seg->params.color1;
    } else {
        strip->leds[seg->start] = COLOR_BLACK;
    }
    
    // 淡出效果
    for (uint16_t i = 0; i < len; i++) {
        Color_t c = strip->leds[seg->start + i];
        c.r = qsub8(c.r, 8);
        c.g = qsub8(c.g, 8);
        c.b = qsub8(c.b, 8);
        strip->leds[seg->start + i] = c;
    }
    
    seg->step++;
}

/*============================================================================
 * 色块效果
 *============================================================================*/

// FX_BLOCKS - 色块
static void fx_blocks(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    uint8_t block_size = seg->params.size > 0 ? seg->params.size : 5;
    
    for (uint16_t i = 0; i < len; i++) {
        uint16_t block_idx = ((i + seg->step) / block_size) % 3;
        Color_t c;
        
        switch (block_idx) {
            case 0: c = seg->params.color1; break;
            case 1: c = seg->params.color2; break;
            default: c = seg->params.color3; break;
        }
        
        seg_set_pixel(strip, seg, i, c);
    }
    
    seg->step++;
}

/*============================================================================
 * ICU效果
 *============================================================================*/

// FX_ICU - ICU (两个眼睛来回看)
static void fx_icu(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    uint8_t eye_size = 2;
    uint8_t eye_space = len / 4;
    
    seg_fill(strip, seg, seg->params.color2);
    
    // 眼睛位置 (来回移动)
    uint16_t range = len / 2 - eye_space - eye_size * 2;
    uint16_t cycle = range * 2;
    uint16_t pos = seg->step % cycle;
    
    if (pos >= range) {
        pos = cycle - pos;
    }
    
    // 左眼
    for (uint8_t i = 0; i < eye_size; i++) {
        seg_set_pixel(strip, seg, pos + i, seg->params.color1);
    }
    
    // 右眼
    for (uint8_t i = 0; i < eye_size; i++) {
        seg_set_pixel(strip, seg, len - 1 - pos - i, seg->params.color1);
    }
    
    seg->step++;
}

/*============================================================================
 * 双色/三色追逐效果
 *============================================================================*/

// FX_BICOLOR_CHASE - 双色追逐
static void fx_bicolor_chase(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    uint8_t size = seg->params.size > 0 ? seg->params.size : 3;
    
    for (uint16_t i = 0; i < len; i++) {
        uint16_t pos = (i + seg->step) % (size * 2);
        Color_t c = (pos < size) ? seg->params.color1 : seg->params.color2;
        seg_set_pixel(strip, seg, i, c);
    }
    
    seg->step++;
}

// FX_TRICOLOR_CHASE - 三色追逐
static void fx_tricolor_chase(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    uint8_t size = seg->params.size > 0 ? seg->params.size : 2;
    
    for (uint16_t i = 0; i < len; i++) {
        uint16_t pos = (i + seg->step) % (size * 3);
        Color_t c;
        
        if (pos < size) {
            c = seg->params.color1;
        } else if (pos < size * 2) {
            c = seg->params.color2;
        } else {
            c = seg->params.color3;
        }
        
        seg_set_pixel(strip, seg, i, c);
    }
    
    seg->step++;
}

/*============================================================================
 * TwinkleFOX效果
 *============================================================================*/

// FX_TWINKLEFOX - TwinkleFOX
static void fx_twinklefox(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    for (uint16_t i = 0; i < len; i++) {
        // 每个LED有自己的闪烁相位
        uint8_t phase = (i * 37 + seg->step) & 0xFF;
        uint8_t brightness = sin8(phase);
        
        // 随机决定是否闪烁
        uint8_t seed = (i * 17) & 0xFF;
        if (seed < 128) {
            brightness = scale8(brightness, seed * 2);
        }
        
        Color_t c = color_scale(seg->params.color1, brightness);
        
        // 偶尔添加白色闪光
        if (brightness > 240 && random8() < 30) {
            c = COLOR_WHITE;
        }
        
        seg_set_pixel(strip, seg, i, c);
    }
    
    seg->step++;
}

/*============================================================================
 * 马戏团火焰效果
 *============================================================================*/

// FX_CIRCUS_COMBUSTUS - 马戏团火焰
static void fx_circus_combustus(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    for (uint16_t i = 0; i < len; i++) {
        uint16_t pos = (i + seg->step) % 6;
        Color_t c;
        
        switch (pos) {
            case 0:
            case 1:
                c = COLOR_RED;
                break;
            case 2:
            case 3:
                c = COLOR_WHITE;
                break;
            default:
                c = COLOR_BLACK;
                break;
        }
        
        // 添加闪烁
        if (random8() < 20) {
            c = color_scale(c, random8_range(180, 255));
        }
        
        seg_set_pixel(strip, seg, i, c);
    }
    
    seg->step++;
}

/*============================================================================
 * 爆米花效果
 *============================================================================*/

#define MAX_POPCORN 8

typedef struct {
    float pos;
    float vel;
    bool active;
    Color_t color;
} Popcorn_t;

static Popcorn_t popcorn[MAX_POPCORN];

// FX_POPCORN - 爆米花
static void fx_popcorn(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    const float gravity = 0.3f;
    
    seg_fade_out(strip, seg, 48);
    
    // 更新爆米花
    for (uint8_t i = 0; i < MAX_POPCORN; i++) {
        if (popcorn[i].active) {
            popcorn[i].vel -= gravity;
            popcorn[i].pos += popcorn[i].vel;
            
            if (popcorn[i].pos <= 0) {
                popcorn[i].active = false;
            } else if (popcorn[i].pos < len) {
                seg_set_pixel(strip, seg, (uint16_t)popcorn[i].pos, popcorn[i].color);
            }
        }
    }
    
    // 随机生成新爆米花
    if (random8() < 40) {
        for (uint8_t i = 0; i < MAX_POPCORN; i++) {
            if (!popcorn[i].active) {
                popcorn[i].pos = 0;
                popcorn[i].vel = 5.0f + random8_max(10) / 2.0f;
                popcorn[i].active = true;
                popcorn[i].color = color_wheel(random8());
                break;
            }
        }
    }
    
    seg->step++;
}

/*============================================================================
 * 交通灯效果
 *============================================================================*/

// FX_TRAFFIC_LIGHT - 交通灯
static void fx_traffic_light(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    uint8_t phase = (seg->step / 50) % 4;
    
    seg_fill(strip, seg, COLOR_BLACK);
    
    uint16_t third = len / 3;
    
    switch (phase) {
        case 0:  // 红灯
            for (uint16_t i = 0; i < third; i++) {
                seg_set_pixel(strip, seg, i, COLOR_RED);
            }
            break;
        case 1:  // 红黄
            for (uint16_t i = 0; i < third; i++) {
                seg_set_pixel(strip, seg, i, COLOR_RED);
            }
            for (uint16_t i = third; i < third * 2; i++) {
                seg_set_pixel(strip, seg, i, COLOR_YELLOW);
            }
            break;
        case 2:  // 绿灯
            for (uint16_t i = third * 2; i < len; i++) {
                seg_set_pixel(strip, seg, i, COLOR_GREEN);
            }
            break;
        case 3:  // 黄灯
            for (uint16_t i = third; i < third * 2; i++) {
                seg_set_pixel(strip, seg, i, COLOR_YELLOW);
            }
            break;
    }
    
    seg->step++;
}

/*============================================================================
 * 多彩效果
 *============================================================================*/

// FX_COLORFUL - 多彩
static void fx_colorful(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    for (uint16_t i = 0; i < len; i++) {
        uint8_t hue = (i * 10 + seg->step) & 0xFF;
        uint8_t sat = 255;
        uint8_t val = sin8(i * 5 + seg->step * 2);
        
        seg_set_pixel(strip, seg, i, color_hsv(hue, sat, val));
    }
    
    seg->step++;
}

/*============================================================================
 * 洗衣机效果
 *============================================================================*/

// FX_WASHING_MACHINE - 洗衣机
static void fx_washing_machine(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    uint8_t size = seg->params.size > 0 ? seg->params.size : 10;
    
    seg_fill(strip, seg, seg->params.color2);
    
    // 来回摆动
    uint16_t range = len - size;
    uint16_t cycle = range * 2;
    uint16_t raw_pos = seg->step % cycle;
    uint16_t pos;
    
    if (raw_pos < range) {
        pos = raw_pos;
    } else {
        pos = cycle - raw_pos;
    }
    
    // 绘制"衣物"
    for (uint8_t i = 0; i < size; i++) {
        uint8_t brightness = sin8(i * 255 / size);
        Color_t c = color_scale(seg->params.color1, brightness);
        seg_set_pixel(strip, seg, pos + i, c);
    }
    
    seg->step += 2;
}

/*============================================================================
 * 相位效果
 *============================================================================*/

// FX_PHASED - 相位
static void fx_phased(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    for (uint16_t i = 0; i < len; i++) {
        uint8_t phase1 = sin8(i * 8 + seg->step);
        uint8_t phase2 = sin8(i * 12 - seg->step * 2);
        uint8_t combined = (phase1 + phase2) / 2;
        
        Color_t c = color_blend(seg->params.color1, seg->params.color2, combined);
        seg_set_pixel(strip, seg, i, c);
    }
    
    seg->step++;
}
```

## 3. 效果函数表和调度器 (led_effects_table.c)

```c
#include "led_strip.h"
#include "led_effects_def.h"

/*============================================================================
 * 效果函数声明
 *============================================================================*/

// 基础效果
extern void fx_static(LedStrip_t* strip, Segment_t* seg);
extern void fx_blink(LedStrip_t* strip, Segment_t* seg);
extern void fx_breath(LedStrip_t* strip, Segment_t* seg);
extern void fx_color_wipe(LedStrip_t* strip, Segment_t* seg);
extern void fx_color_wipe_random(LedStrip_t* strip, Segment_t* seg);
extern void fx_random_color(LedStrip_t* strip, Segment_t* seg);
extern void fx_single_dynamic(LedStrip_t* strip, Segment_t* seg);
extern void fx_multi_dynamic(LedStrip_t* strip, Segment_t* seg);

// 彩虹效果
extern void fx_rainbow(LedStrip_t* strip, Segment_t* seg);
extern void fx_rainbow_cycle(LedStrip_t* strip, Segment_t* seg);

// 扫描效果
extern void fx_scan(LedStrip_t* strip, Segment_t* seg);
extern void fx_dual_scan(LedStrip_t* strip, Segment_t* seg);

// 剧院追逐
extern void fx_theater_chase(LedStrip_t* strip, Segment_t* seg);
extern void fx_theater_chase_rainbow(LedStrip_t* strip, Segment_t* seg);

// 跑马灯
extern void fx_running_lights(LedStrip_t* strip, Segment_t* seg);
extern void fx_running_color(LedStrip_t* strip, Segment_t* seg);
extern void fx_running_red_blue(LedStrip_t* strip, Segment_t* seg);
extern void fx_running_random(LedStrip_t* strip, Segment_t* seg);

// 闪烁效果
extern void fx_twinkle(LedStrip_t* strip, Segment_t* seg);
extern void fx_twinkle_random(LedStrip_t* strip, Segment_t* seg);
extern void fx_twinkle_fade(LedStrip_t* strip, Segment_t* seg);
extern void fx_sparkle(LedStrip_t* strip, Segment_t* seg);
extern void fx_flash_sparkle(LedStrip_t* strip, Segment_t* seg);
extern void fx_hyper_sparkle(LedStrip_t* strip, Segment_t* seg);

// 频闪效果
extern void fx_strobe(LedStrip_t* strip, Segment_t* seg);
extern void fx_strobe_rainbow(LedStrip_t* strip, Segment_t* seg);
extern void fx_multi_strobe(LedStrip_t* strip, Segment_t* seg);

// 追逐效果
extern void fx_chase_color(LedStrip_t* strip, Segment_t* seg);
extern void fx_chase_random(LedStrip_t* strip, Segment_t* seg);
extern void fx_chase_rainbow(LedStrip_t* strip, Segment_t* seg);
extern void fx_chase_flash(LedStrip_t* strip, Segment_t* seg);
extern void fx_chase_blackout(LedStrip_t* strip, Segment_t* seg);

// Larson扫描器
extern void fx_larson_scanner(LedStrip_t* strip, Segment_t* seg);

// 彗星
extern void fx_comet(LedStrip_t* strip, Segment_t* seg);

// 烟花
extern void fx_fireworks(LedStrip_t* strip, Segment_t* seg);
extern void fx_fireworks_random(LedStrip_t* strip, Segment_t* seg);

// 节日效果
extern void fx_merry_christmas(LedStrip_t* strip, Segment_t* seg);
extern void fx_halloween(LedStrip_t* strip, Segment_t* seg);

// 火焰效果
extern void fx_fire_flicker(LedStrip_t* strip, Segment_t* seg);
extern void fx_fire_flicker_soft(LedStrip_t* strip, Segment_t* seg);
extern void fx_fire_flicker_intense(LedStrip_t* strip, Segment_t* seg);
extern void fx_fire_2012(LedStrip_t* strip, Segment_t* seg);

// 流星效果
extern void fx_meteor(LedStrip_t* strip, Segment_t* seg);
extern void fx_meteor_smooth(LedStrip_t* strip, Segment_t* seg);
extern void fx_meteor_rain(LedStrip_t* strip, Segment_t* seg);

// 溶解效果
extern void fx_dissolve(LedStrip_t* strip, Segment_t* seg);
extern void fx_dissolve_random(LedStrip_t* strip, Segment_t* seg);

// 涟漪效果
extern void fx_ripple(LedStrip_t* strip, Segment_t* seg);
extern void fx_ripple_rainbow(LedStrip_t* strip, Segment_t* seg);

// 心跳
extern void fx_heartbeat(LedStrip_t* strip, Segment_t* seg);

// 等离子
extern void fx_plasma(LedStrip_t* strip, Segment_t* seg);

// 蜡烛
extern void fx_candle(LedStrip_t* strip, Segment_t* seg);
extern void fx_candle_multi(LedStrip_t* strip, Segment_t* seg);

// 太平洋
extern void fx_pacifica(LedStrip_t* strip, Segment_t* seg);

// Pride
extern void fx_pride(LedStrip_t* strip, Segment_t* seg);

// 日出
extern void fx_sunrise(LedStrip_t* strip, Segment_t* seg);

// 闪光
extern void fx_solid_glitter(LedStrip_t* strip, Segment_t* seg);
extern void fx_glitter(LedStrip_t* strip, Segment_t* seg);

// 警灯
extern void fx_police(LedStrip_t* strip, Segment_t* seg);
extern void fx_police_all(LedStrip_t* strip, Segment_t* seg);

// 双点/双区域
extern void fx_two_dots(LedStrip_t* strip, Segment_t* seg);
extern void fx_two_areas(LedStrip_t* strip, Segment_t* seg);

// 振荡
extern void fx_oscillate(LedStrip_t* strip, Segment_t* seg);

// 正弦波
extern void fx_sinelon(LedStrip_t* strip, Segment_t* seg);
extern void fx_sinelon_dual(LedStrip_t* strip, Segment_t* seg);
extern void fx_sinelon_rainbow(LedStrip_t* strip, Segment_t* seg);

// 杂耍
extern void fx_juggle(LedStrip_t* strip, Segment_t* seg);

// BPM
extern void fx_bpm(LedStrip_t* strip, Segment_t* seg);

// 噪声
extern void fx_noise(LedStrip_t* strip, Segment_t* seg);
extern void fx_fill_noise(LedStrip_t* strip, Segment_t* seg);

// 渐变
extern void fx_gradient(LedStrip_t* strip, Segment_t* seg);
extern void fx_gradient_cycle(LedStrip_t* strip, Segment_t* seg);

// 加载
extern void fx_loading(LedStrip_t* strip, Segment_t* seg);
extern void fx_loading_dual(LedStrip_t* strip, Segment_t* seg);

// 光斑
extern void fx_spots(LedStrip_t* strip, Segment_t* seg);
extern void fx_spots_fade(LedStrip_t* strip, Segment_t* seg);

// 百分比
extern void fx_percent(LedStrip_t* strip, Segment_t* seg);

// 堆叠
extern void fx_stacker(LedStrip_t* strip, Segment_t* seg);

// 弹跳球
extern void fx_bouncing_balls(LedStrip_t* strip, Segment_t* seg);
extern void fx_bouncing_balls_multi(LedStrip_t* strip, Segment_t* seg);

// 灯塔
extern void fx_lighthouse(LedStrip_t* strip, Segment_t* seg);

// 仙女
extern void fx_fairy(LedStrip_t* strip, Segment_t* seg);
extern void fx_fairy_twinkle(LedStrip_t* strip, Segment_t* seg);

// 水滴
extern void fx_drip(LedStrip_t* strip, Segment_t* seg);

// 雨滴
extern void fx_rain(LedStrip_t* strip, Segment_t* seg);

// 色块
extern void fx_blocks(LedStrip_t* strip, Segment_t* seg);

// ICU
extern void fx_icu(LedStrip_t* strip, Segment_t* seg);

// 双色/三色追逐
extern void fx_bicolor_chase(LedStrip_t* strip, Segment_t* seg);
extern void fx_tricolor_chase(LedStrip_t* strip, Segment_t* seg);

// TwinkleFOX
extern void fx_twinklefox(LedStrip_t* strip, Segment_t* seg);

// 马戏团火焰
extern void fx_circus_combustus(LedStrip_t* strip, Segment_t* seg);

// 爆米花
extern void fx_popcorn(LedStrip_t* strip, Segment_t* seg);

// 交通灯
extern void fx_traffic_light(LedStrip_t* strip, Segment_t* seg);

// 多彩
extern void fx_colorful(LedStrip_t* strip, Segment_t* seg);

// 洗衣机
extern void fx_washing_machine(LedStrip_t* strip, Segment_t* seg);

// 相位
extern void fx_phased(LedStrip_t* strip, Segment_t* seg);

/*============================================================================
 * 效果函数指针类型
 *============================================================================*/

typedef void (*FxFunc_t)(LedStrip_t*, Segment_t*);

/*============================================================================
 * 效果函数表
 *============================================================================*/

static const FxFunc_t fx_table[FX_MODE_COUNT] = {
    // 基础效果 (0-9)
    [FX_STATIC]                 = fx_static,
    [FX_BLINK]                  = fx_blink,
    [FX_BREATH]                 = fx_breath,
    [FX_COLOR_WIPE]             = fx_color_wipe,
    [FX_COLOR_WIPE_INV]         = fx_color_wipe,        // 使用相同函数，参数不同
    [FX_COLOR_WIPE_REV]         = fx_color_wipe,
    [FX_COLOR_WIPE_REV_INV]     = fx_color_wipe,
    [FX_COLOR_WIPE_RANDOM]      = fx_color_wipe_random,
    [FX_RANDOM_COLOR]           = fx_random_color,
    [FX_SINGLE_DYNAMIC]         = fx_single_dynamic,
    
    // 多点动态效果 (10-19)
    [FX_MULTI_DYNAMIC]          = fx_multi_dynamic,
    [FX_RAINBOW]                = fx_rainbow,
    [FX_RAINBOW_CYCLE]          = fx_rainbow_cycle,
    [FX_SCAN]                   = fx_scan,
    [FX_DUAL_SCAN]              = fx_dual_scan,
    [FX_FADE]                   = fx_breath,            // 复用呼吸效果
    [FX_THEATER_CHASE]          = fx_theater_chase,
    [FX_THEATER_CHASE_RAINBOW]  = fx_theater_chase_rainbow,
    [FX_RUNNING_LIGHTS]         = fx_running_lights,
    [FX_TWINKLE]                = fx_twinkle,
    
    // 闪烁效果 (20-29)
    [FX_TWINKLE_RANDOM]         = fx_twinkle_random,
    [FX_TWINKLE_FADE]           = fx_twinkle_fade,
    [FX_TWINKLE_FADE_RANDOM]    = fx_twinkle_fade,
    [FX_SPARKLE]                = fx_sparkle,
    [FX_FLASH_SPARKLE]          = fx_flash_sparkle,
    [FX_HYPER_SPARKLE]          = fx_hyper_sparkle,
    [FX_STROBE]                 = fx_strobe,
    [FX_STROBE_RAINBOW]         = fx_strobe_rainbow,
    [FX_MULTI_STROBE]           = fx_multi_strobe,
    [FX_BLINK_RAINBOW]          = fx_strobe_rainbow,
    
    // 追逐效果 (30-39)
    [FX_CHASE_WHITE]            = fx_chase_color,
    [FX_CHASE_COLOR]            = fx_chase_color,
    [FX_CHASE_RANDOM]           = fx_chase_random,
    [FX_CHASE_RAINBOW]          = fx_chase_rainbow,
    [FX_CHASE_FLASH]            = fx_chase_flash,
    [FX_CHASE_FLASH_RANDOM]     = fx_chase_flash,
    [FX_CHASE_RAINBOW_WHITE]    = fx_chase_rainbow,
    [FX_CHASE_BLACKOUT]         = fx_chase_blackout,
    [FX_CHASE_BLACKOUT_RAINBOW] = fx_chase_blackout,
    [FX_COLOR_SWEEP_RANDOM]     = fx_color_wipe_random,
    
    // 跑动效果 (40-49)
    [FX_RUNNING_COLOR]          = fx_running_color,
    [FX_RUNNING_RED_BLUE]       = fx_running_red_blue,
    [FX_RUNNING_RANDOM]         = fx_running_random,
    [FX_LARSON_SCANNER]         = fx_larson_scanner,
    [FX_COMET]                  = fx_comet,
    [FX_FIREWORKS]              = fx_fireworks,
    [FX_FIREWORKS_RANDOM]       = fx_fireworks_random,
    [FX_MERRY_CHRISTMAS]        = fx_merry_christmas,
    [FX_HALLOWEEN]              = fx_halloween,
    [FX_FIRE_FLICKER]           = fx_fire_flicker,
    
    // 火焰效果 (50-59)
    [FX_FIRE_
```





```
继续生成
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-4.5-Opus

已完成生成

## 3. 效果函数表和调度器 (led_effects_table.c) - 续

```c
    // 火焰效果 (50-59)
    [FX_FIRE_FLICKER_SOFT]      = fx_fire_flicker_soft,
    [FX_FIRE_FLICKER_INTENSE]   = fx_fire_flicker_intense,
    [FX_FIRE_2012]              = fx_fire_2012,
    [FX_CIRCUS_COMBUSTUS]       = fx_circus_combustus,
    [FX_BICOLOR_CHASE]          = fx_bicolor_chase,
    [FX_TRICOLOR_CHASE]         = fx_tricolor_chase,
    [FX_TWINKLEFOX]             = fx_twinklefox,
    [FX_RAIN]                   = fx_rain,
    [FX_BLOCKS]                 = fx_blocks,
    [FX_ICU]                    = fx_icu,
    
    // 流星效果 (60-69)
    [FX_METEOR]                 = fx_meteor,
    [FX_METEOR_SMOOTH]          = fx_meteor_smooth,
    [FX_METEOR_RAIN]            = fx_meteor_rain,
    [FX_DISSOLVE]               = fx_dissolve,
    [FX_DISSOLVE_RANDOM]        = fx_dissolve_random,
    [FX_POPCORN]                = fx_popcorn,
    [FX_PLASMA]                 = fx_plasma,
    [FX_RIPPLE]                 = fx_ripple,
    [FX_RIPPLE_RAINBOW]         = fx_ripple_rainbow,
    [FX_HEARTBEAT]              = fx_heartbeat,
    
    // 特殊效果 (70-79)
    [FX_PACIFICA]               = fx_pacifica,
    [FX_PRIDE]                  = fx_pride,
    [FX_CANDLE]                 = fx_candle,
    [FX_CANDLE_MULTI]           = fx_candle_multi,
    [FX_SOLID_GLITTER]          = fx_solid_glitter,
    [FX_SUNRISE]                = fx_sunrise,
    [FX_PHASED]                 = fx_phased,
    [FX_NOISE]                  = fx_noise,
    [FX_PERLIN_NOISE]           = fx_fill_noise,
    [FX_WASHING_MACHINE]        = fx_washing_machine,
    
    // 渐变效果 (80-89)
    [FX_GRADIENT]               = fx_gradient,
    [FX_GRADIENT_CYCLE]         = fx_gradient_cycle,
    [FX_LOADING]                = fx_loading,
    [FX_LOADING_DUAL]           = fx_loading_dual,
    [FX_POLICE]                 = fx_police,
    [FX_POLICE_ALL]             = fx_police_all,
    [FX_TWO_DOTS]               = fx_two_dots,
    [FX_TWO_AREAS]              = fx_two_areas,
    [FX_OSCILLATE]              = fx_oscillate,
    [FX_SINELON]                = fx_sinelon,
    
    // 高级效果 (90-99)
    [FX_SINELON_DUAL]           = fx_sinelon_dual,
    [FX_SINELON_RAINBOW]        = fx_sinelon_rainbow,
    [FX_JUGGLE]                 = fx_juggle,
    [FX_PALETTE]                = fx_gradient_cycle,
    [FX_PALETTE_CYCLE]          = fx_gradient_cycle,
    [FX_BPM]                    = fx_bpm,
    [FX_FILL_NOISE]             = fx_fill_noise,
    [FX_COLORFUL]               = fx_colorful,
    [FX_TRAFFIC_LIGHT]          = fx_traffic_light,
    [FX_SPOTS]                  = fx_spots,
    
    // 扩展效果 (100+)
    [FX_SPOTS_FADE]             = fx_spots_fade,
    [FX_GLITTER]                = fx_glitter,
    [FX_PERCENT]                = fx_percent,
    [FX_STACKER]                = fx_stacker,
    [FX_BOUNCING_BALLS]         = fx_bouncing_balls,
    [FX_BOUNCING_BALLS_MULTI]   = fx_bouncing_balls_multi,
    [FX_LIGHTHOUSE]             = fx_lighthouse,
    [FX_FAIRY]                  = fx_fairy,
    [FX_FAIRY_TWINKLE]          = fx_fairy_twinkle,
    [FX_DRIP]                   = fx_drip,
};

/*============================================================================
 * 效果默认速度表 (毫秒)
 *============================================================================*/

static const uint16_t fx_default_speed[FX_MODE_COUNT] = {
    [FX_STATIC]                 = 1000,
    [FX_BLINK]                  = 500,
    [FX_BREATH]                 = 20,
    [FX_COLOR_WIPE]             = 50,
    [FX_COLOR_WIPE_INV]         = 50,
    [FX_COLOR_WIPE_REV]         = 50,
    [FX_COLOR_WIPE_REV_INV]     = 50,
    [FX_COLOR_WIPE_RANDOM]      = 50,
    [FX_RANDOM_COLOR]           = 200,
    [FX_SINGLE_DYNAMIC]         = 50,
    [FX_MULTI_DYNAMIC]          = 50,
    [FX_RAINBOW]                = 30,
    [FX_RAINBOW_CYCLE]          = 20,
    [FX_SCAN]                   = 30,
    [FX_DUAL_SCAN]              = 30,
    [FX_FADE]                   = 20,
    [FX_THEATER_CHASE]          = 100,
    [FX_THEATER_CHASE_RAINBOW]  = 80,
    [FX_RUNNING_LIGHTS]         = 30,
    [FX_TWINKLE]                = 30,
    [FX_TWINKLE_RANDOM]         = 30,
    [FX_TWINKLE_FADE]           = 30,
    [FX_TWINKLE_FADE_RANDOM]    = 30,
    [FX_SPARKLE]                = 20,
    [FX_FLASH_SPARKLE]          = 30,
    [FX_HYPER_SPARKLE]          = 20,
    [FX_STROBE]                 = 30,
    [FX_STROBE_RAINBOW]         = 30,
    [FX_MULTI_STROBE]           = 30,
    [FX_BLINK_RAINBOW]          = 300,
    [FX_CHASE_WHITE]            = 50,
    [FX_CHASE_COLOR]            = 50,
    [FX_CHASE_RANDOM]           = 50,
    [FX_CHASE_RAINBOW]          = 50,
    [FX_CHASE_FLASH]            = 30,
    [FX_CHASE_FLASH_RANDOM]     = 30,
    [FX_CHASE_RAINBOW_WHITE]    = 50,
    [FX_CHASE_BLACKOUT]         = 50,
    [FX_CHASE_BLACKOUT_RAINBOW] = 50,
    [FX_COLOR_SWEEP_RANDOM]     = 30,
    [FX_RUNNING_COLOR]          = 50,
    [FX_RUNNING_RED_BLUE]       = 50,
    [FX_RUNNING_RANDOM]         = 50,
    [FX_LARSON_SCANNER]         = 30,
    [FX_COMET]                  = 30,
    [FX_FIREWORKS]              = 30,
    [FX_FIREWORKS_RANDOM]       = 30,
    [FX_MERRY_CHRISTMAS]        = 100,
    [FX_HALLOWEEN]              = 100,
    [FX_FIRE_FLICKER]           = 30,
    [FX_FIRE_FLICKER_SOFT]      = 40,
    [FX_FIRE_FLICKER_INTENSE]   = 20,
    [FX_FIRE_2012]              = 20,
    [FX_CIRCUS_COMBUSTUS]       = 80,
    [FX_BICOLOR_CHASE]          = 50,
    [FX_TRICOLOR_CHASE]         = 50,
    [FX_TWINKLEFOX]             = 20,
    [FX_RAIN]                   = 30,
    [FX_BLOCKS]                 = 100,
    [FX_ICU]                    = 50,
    [FX_METEOR]                 = 30,
    [FX_METEOR_SMOOTH]          = 30,
    [FX_METEOR_RAIN]            = 30,
    [FX_DISSOLVE]               = 30,
    [FX_DISSOLVE_RANDOM]        = 30,
    [FX_POPCORN]                = 20,
    [FX_PLASMA]                 = 20,
    [FX_RIPPLE]                 = 30,
    [FX_RIPPLE_RAINBOW]         = 30,
    [FX_HEARTBEAT]              = 30,
    [FX_PACIFICA]               = 20,
    [FX_PRIDE]                  = 20,
    [FX_CANDLE]                 = 50,
    [FX_CANDLE_MULTI]           = 50,
    [FX_SOLID_GLITTER]          = 30,
    [FX_SUNRISE]                = 100,
    [FX_PHASED]                 = 20,
    [FX_NOISE]                  = 30,
    [FX_PERLIN_NOISE]           = 30,
    [FX_WASHING_MACHINE]        = 20,
    [FX_GRADIENT]               = 1000,
    [FX_GRADIENT_CYCLE]         = 30,
    [FX_LOADING]                = 30,
    [FX_LOADING_DUAL]           = 30,
    [FX_POLICE]                 = 50,
    [FX_POLICE_ALL]             = 50,
    [FX_TWO_DOTS]               = 50,
    [FX_TWO_AREAS]              = 50,
    [FX_OSCILLATE]              = 30,
    [FX_SINELON]                = 20,
    [FX_SINELON_DUAL]           = 20,
    [FX_SINELON_RAINBOW]        = 20,
    [FX_JUGGLE]                 = 20,
    [FX_PALETTE]                = 30,
    [FX_PALETTE_CYCLE]          = 30,
    [FX_BPM]                    = 20,
    [FX_FILL_NOISE]             = 30,
    [FX_COLORFUL]               = 20,
    [FX_TRAFFIC_LIGHT]          = 50,
    [FX_SPOTS]                  = 100,
    [FX_SPOTS_FADE]             = 100,
    [FX_GLITTER]                = 20,
    [FX_PERCENT]                = 100,
    [FX_STACKER]                = 30,
    [FX_BOUNCING_BALLS]         = 20,
    [FX_BOUNCING_BALLS_MULTI]   = 20,
    [FX_LIGHTHOUSE]             = 30,
    [FX_FAIRY]                  = 20,
    [FX_FAIRY_TWINKLE]          = 20,
    [FX_DRIP]                   = 30,
};

/*============================================================================
 * 效果调度器API
 *============================================================================*/

// 执行效果
void fx_execute(LedStrip_t* strip, Segment_t* seg) {
    if (!strip || !seg || !seg->active || seg->paused) {
        return;
    }
    
    FxMode_t mode = (FxMode_t)seg->effect;
    
    if (mode < FX_MODE_COUNT && fx_table[mode] != NULL) {
        fx_table[mode](strip, seg);
        strip->dirty = true;
    }
}

// 获取效果名称
const char* fx_get_name(FxMode_t mode) {
    if (mode < FX_MODE_COUNT) {
        return FX_MODE_NAMES[mode];
    }
    return "Unknown";
}

// 获取效果默认速度
uint16_t fx_get_default_speed(FxMode_t mode) {
    if (mode < FX_MODE_COUNT) {
        return fx_default_speed[mode];
    }
    return 50;
}

// 获取效果总数
uint8_t fx_get_count(void) {
    return FX_MODE_COUNT;
}

// 设置效果 (带默认参数)
bool fx_set_mode(LedStrip_t* strip, uint8_t seg_id, FxMode_t mode) {
    if (!strip || seg_id >= strip->num_segments || mode >= FX_MODE_COUNT) {
        return false;
    }
    
    Segment_t* seg = &strip->segments[seg_id];
    seg->effect = mode;
    seg->params.speed = fx_default_speed[mode];
    seg->step = 0;
    seg->aux0 = 0;
    seg->aux1 = 0;
    seg->last_update = 0;
    
    return true;
}

// 设置效果 (带自定义参数)
bool fx_set_mode_params(LedStrip_t* strip, uint8_t seg_id, 
                        FxMode_t mode, EffectParams_t* params) {
    if (!strip || seg_id >= strip->num_segments || mode >= FX_MODE_COUNT) {
        return false;
    }
    
    Segment_t* seg = &strip->segments[seg_id];
    seg->effect = mode;
    
    if (params) {
        seg->params = *params;
    } else {
        seg->params.speed = fx_default_speed[mode];
    }
    
    seg->step = 0;
    seg->aux0 = 0;
    seg->aux1 = 0;
    seg->last_update = 0;
    
    return true;
}

// 获取下一个效果
FxMode_t fx_next_mode(FxMode_t current) {
    return (current + 1) % FX_MODE_COUNT;
}

// 获取上一个效果
FxMode_t fx_prev_mode(FxMode_t current) {
    return (current == 0) ? (FX_MODE_COUNT - 1) : (current - 1);
}

// 获取随机效果
FxMode_t fx_random_mode(void) {
    return (FxMode_t)(random8() % FX_MODE_COUNT);
}
```

## 4. 调色板系统 (led_palette.h / led_palette.c)

```c
/*============================================================================
 * led_palette.h - 调色板系统
 *============================================================================*/

#ifndef LED_PALETTE_H
#define LED_PALETTE_H

#include "led_strip.h"

#define PALETTE_SIZE    16
#define MAX_PALETTES    16

/*============================================================================
 * 调色板结构
 *============================================================================*/

typedef struct {
    Color_t colors[PALETTE_SIZE];
    char    name[16];
} Palette_t;

/*============================================================================
 * 预定义调色板
 *============================================================================*/

typedef enum {
    PAL_RAINBOW = 0,
    PAL_RAINBOW_STRIPE,
    PAL_PARTY,
    PAL_HEAT,
    PAL_FIRE,
    PAL_OCEAN,
    PAL_FOREST,
    PAL_LAVA,
    PAL_CLOUD,
    PAL_SUNSET,
    PAL_RETRO,
    PAL_PASTEL,
    PAL_NEON,
    PAL_ICE,
    PAL_AUTUMN,
    PAL_CUSTOM,
    PAL_COUNT
} PaletteType_t;

/*============================================================================
 * API函数
 *============================================================================*/

// 初始化调色板系统
void palette_init(void);

// 获取调色板
Palette_t* palette_get(PaletteType_t type);

// 从调色板获取颜色 (0-255 索引，自动插值)
Color_t palette_color(Palette_t* pal, uint8_t index);

// 从调色板获取颜色 (带亮度)
Color_t palette_color_brightness(Palette_t* pal, uint8_t index, uint8_t brightness);

// 设置自定义调色板颜色
void palette_set_color(Palette_t* pal, uint8_t index, Color_t color);

// 创建渐变调色板
void palette_create_gradient(Palette_t* pal, Color_t c1, Color_t c2, Color_t c3, Color_t c4);

// 调色板效果
void fx_palette_fill(LedStrip_t* strip, Segment_t* seg, Palette_t* pal);
void fx_palette_wave(LedStrip_t* strip, Segment_t* seg, Palette_t* pal);
void fx_palette_blend(LedStrip_t* strip, Segment_t* seg, Palette_t* pal);

#endif // LED_PALETTE_H
/*============================================================================
 * led_palette.c - 调色板系统实现
 *============================================================================*/

#include "led_palette.h"

/*============================================================================
 * 预定义调色板数据
 *============================================================================*/

static Palette_t palettes[PAL_COUNT];

// 彩虹调色板
static const Color_t rainbow_colors[16] = {
    {255, 0, 0, 0},     {255, 64, 0, 0},    {255, 128, 0, 0},   {255, 191, 0, 0},
    {255, 255, 0, 0},   {128, 255, 0, 0},   {0, 255, 0, 0},     {0, 255, 128, 0},
    {0, 255, 255, 0},   {0, 128, 255, 0},   {0, 0, 255, 0},     {128, 0, 255, 0},
    {255, 0, 255, 0},   {255, 0, 128, 0},   {255, 0, 64, 0},    {255, 0, 0, 0}
};

// 派对调色板
static const Color_t party_colors[16] = {
    {85, 0, 171, 0},    {132, 0, 124, 0},   {181, 0, 75, 0},    {229, 0, 27, 0},
    {232, 23, 0, 0},    {184, 71, 0, 0},    {171, 119, 0, 0},   {171, 171, 0, 0},
    {171, 85, 0, 0},    {221, 34, 0, 0},    {242, 0, 14, 0},    {194, 0, 62, 0},
    {143, 0, 113, 0},   {85, 0, 171, 0},    {36, 0, 220, 0},    {85, 0, 171, 0}
};

// 热力调色板
static const Color_t heat_colors[16] = {
    {0, 0, 0, 0},       {32, 0, 0, 0},      {64, 0, 0, 0},      {96, 0, 0, 0},
    {128, 0, 0, 0},     {160, 0, 0, 0},     {192, 0, 0, 0},     {224, 0, 0, 0},
    {255, 0, 0, 0},     {255, 32, 0, 0},    {255, 64, 0, 0},    {255, 96, 0, 0},
    {255, 128, 0, 0},   {255, 160, 0, 0},   {255, 224, 0, 0},   {255, 255, 0, 0}
};

// 火焰调色板
static const Color_t fire_colors[16] = {
    {0, 0, 0, 0},       {32, 0, 0, 0},      {64, 0, 0, 0},      {128, 0, 0, 0},
    {192, 0, 0, 0},     {255, 0, 0, 0},     {255, 32, 0, 0},    {255, 64, 0, 0},
    {255, 96, 0, 0},    {255, 128, 0, 0},   {255, 160, 0, 0},   {255, 192, 0, 0},
    {255, 224, 0, 0},   {255, 255, 0, 0},   {255, 255, 128, 0}, {255, 255, 255, 0}
};

// 海洋调色板
static const Color_t ocean_colors[16] = {
    {0, 0, 32, 0},      {0, 0, 64, 0},      {0, 0, 96, 0},      {0, 0, 128, 0},
    {0, 32, 160, 0},    {0, 64, 192, 0},    {0, 96, 224, 0},    {0, 128, 255, 0},
    {0, 160, 255, 0},   {0, 192, 255, 0},   {32, 224, 255, 0},  {64, 255, 255, 0},
    {128, 255, 255, 0}, {192, 255, 255, 0}, {224, 255, 255, 0}, {255, 255, 255, 0}
};

// 森林调色板
static const Color_t forest_colors[16] = {
    {0, 32, 0, 0},      {0, 64, 0, 0},      {0, 96, 0, 0},      {0, 128, 0, 0},
    {0, 160, 0, 0},     {32, 192, 0, 0},    {64, 224, 0, 0},    {96, 255, 0, 0},
    {64, 192, 0, 0},    {32, 128, 0, 0},    {64, 160, 32, 0},   {96, 192, 64, 0},
    {128, 224, 96, 0},  {96, 192, 64, 0},   {64, 160, 32, 0},   {32, 128, 0, 0}
};

// 熔岩调色板
static const Color_t lava_colors[16] = {
    {0, 0, 0, 0},       {32, 0, 0, 0},      {64, 0, 0, 0},      {128, 0, 0, 0},
    {192, 0, 0, 0},     {255, 0, 0, 0},     {255, 64, 0, 0},    {255, 128, 0, 0},
    {255, 64, 0, 0},    {255, 0, 0, 0},     {192, 0, 0, 0},     {128, 0, 0, 0},
    {192, 0, 0, 0},     {255, 32, 0, 0},    {255, 96, 0, 0},    {255, 160, 0, 0}
};

// 云彩调色板
static const Color_t cloud_colors[16] = {
    {0, 0, 128, 0},     {0, 0, 160, 0},     {0, 0, 192, 0},     {0, 0, 224, 0},
    {32, 32, 255, 0},   {64, 64, 255, 0},   {128, 128, 255, 0}, {192, 192, 255, 0},
    {255, 255, 255, 0}, {192, 192, 255, 0}, {128, 128, 255, 0}, {64, 64, 255, 0},
    {32, 32, 255, 0},   {0, 0, 224, 0},     {0, 0, 192, 0},     {0, 0, 160, 0}
};

// 日落调色板
static const Color_t sunset_colors[16] = {
    {32, 0, 64, 0},     {64, 0, 96, 0},     {128, 0, 128, 0},   {192, 0, 96, 0},
    {255, 0, 64, 0},    {255, 32, 32, 0},   {255, 64, 0, 0},    {255, 128, 0, 0},
    {255, 192, 0, 0},   {255, 224, 0, 0},   {255, 255, 0, 0},   {255, 224, 64, 0},
    {255, 192, 128, 0}, {255, 160, 96, 0},  {255, 128, 64, 0},  {255, 96, 32, 0}
};

// 复古调色板
static const Color_t retro_colors[16] = {
    {255, 0, 0, 0},     {255, 128, 0, 0},   {255, 255, 0, 0},   {0, 255, 0, 0},
    {0, 255, 255, 0},   {0, 0, 255, 0},     {255, 0, 255, 0},   {255, 0, 128, 0},
    {255, 0, 0, 0},     {255, 128, 0, 0},   {255, 255, 0, 0},   {0, 255, 0, 0},
    {0, 255, 255, 0},   {0, 0, 255, 0},     {255, 0, 255, 0},   {255, 0, 128, 0}
};

// 柔和调色板
static const Color_t pastel_colors[16] = {
    {255, 182, 193, 0}, {255, 218, 185, 0}, {255, 255, 186, 0}, {186, 255, 201, 0},
    {186, 225, 255, 0}, {218, 186, 255, 0}, {255, 186, 255, 0}, {255, 186, 218, 0},
    {255, 182, 193, 0}, {255, 218, 185, 0}, {255, 255, 186, 0}, {186, 255, 201, 0},
    {186, 225, 255, 0}, {218, 186, 255, 0}, {255, 186, 255, 0}, {255, 186, 218, 0}
};

// 霓虹调色板
static const Color_t neon_colors[16] = {
    {255, 0, 102, 0},   {255, 0, 255, 0},   {102, 0, 255, 0},   {0, 102, 255, 0},
    {0, 255, 255, 0},   {0, 255, 102, 0},   {102, 255, 0, 0},   {255, 255, 0, 0},
    {255, 102, 0, 0},   {255, 0, 0, 0},     {255, 0, 102, 0},   {255, 0, 255, 0},
    {102, 0, 255, 0},   {0, 102, 255, 0},   {0, 255, 255, 0},   {0, 255, 102, 0}
};

// 冰雪调色板
static const Color_t ice_colors[16] = {
    {255, 255, 255, 0}, {224, 240, 255, 0}, {192, 224, 255, 0}, {160, 208, 255, 0},
    {128, 192, 255, 0}, {96, 176, 255, 0},  {64, 160, 255, 0},  {32, 144, 255, 0},
    {0, 128, 255, 0},   {32, 144, 255, 0},  {64, 160, 255, 0},  {96, 176, 255, 0},
    {128, 192, 255, 0}, {160, 208, 255, 0}, {192, 224, 255, 0}, {224, 240, 255, 0}
};

// 秋天调色板
static const Color_t autumn_colors[16] = {
    {128, 64, 0, 0},    {160, 80, 0, 0},    {192, 96, 0, 0},    {224, 112, 0, 0},
    {255, 128, 0, 0},   {255, 96, 0, 0},    {255, 64, 0, 0},    {255, 32, 0, 0},
    {224, 0, 0, 0},     {192, 0, 0, 0},     {160, 32, 0, 0},    {128, 64, 0, 0},
    {160, 80, 0, 0},    {192, 96, 0, 0},    {160, 80, 0, 0},    {128, 64, 0, 0}
};

/*============================================================================
 * 调色板初始化
 *============================================================================*/

void palette_init(void) {
    // 复制预定义调色板
    memcpy(palettes[PAL_RAINBOW].colors, rainbow_colors, sizeof(rainbow_colors));
    strcpy(palettes[PAL_RAINBOW].name, "Rainbow");
    
    memcpy(palettes[PAL_RAINBOW_STRIPE].colors, rainbow_colors, sizeof(rainbow_colors));
    strcpy(palettes[PAL_RAINBOW_STRIPE].name, "RainbowStripe");
    
    memcpy(palettes[PAL_PARTY].colors, party_colors, sizeof(party_colors));
    strcpy(palettes[PAL_PARTY].name, "Party");
    
    memcpy(palettes[PAL_HEAT].colors, heat_colors, sizeof(heat_colors));
    strcpy(palettes[PAL_HEAT].name, "Heat");
    
    memcpy(palettes[PAL_FIRE].colors, fire_colors, sizeof(fire_colors));
    strcpy(palettes[PAL_FIRE].name, "Fire");
    
    memcpy(palettes[PAL_OCEAN].colors, ocean_colors, sizeof(ocean_colors));
    strcpy(palettes[PAL_OCEAN].name, "Ocean");
    
    memcpy(palettes[PAL_FOREST].colors, forest_colors, sizeof(forest_colors));
    strcpy(palettes[PAL_FOREST].name, "Forest");
    
    memcpy(palettes[PAL_LAVA].colors, lava_colors, sizeof(lava_colors));
    strcpy(palettes[PAL_LAVA].name, "Lava");
    
    memcpy(palettes[PAL_CLOUD].colors, cloud_colors, sizeof(cloud_colors));
    strcpy(palettes[PAL_CLOUD].name, "Cloud");
    
    memcpy(palettes[PAL_SUNSET].colors, sunset_colors, sizeof(sunset_colors));
    strcpy(palettes[PAL_SUNSET].name, "Sunset");
    
    memcpy(palettes[PAL_RETRO].colors, retro_colors, sizeof(retro_colors));
    strcpy(palettes[PAL_RETRO].name, "Retro");
    
    memcpy(palettes[PAL_PASTEL].colors, pastel_colors, sizeof(pastel_colors));
    strcpy(palettes[PAL_PASTEL].name, "Pastel");
    
    memcpy(palettes[PAL_NEON].colors, neon_colors, sizeof(neon_colors));
    strcpy(palettes[PAL_NEON].name, "Neon");
    
    memcpy(palettes[PAL_ICE].colors, ice_colors, sizeof(ice_colors));
    strcpy(palettes[PAL_ICE].name, "Ice");
    
    memcpy(palettes[PAL_AUTUMN].colors, autumn_colors, sizeof(autumn_colors));
    strcpy(palettes[PAL_AUTUMN].name, "Autumn");
    
    // 自定义调色板初始化为彩虹
    memcpy(palettes[PAL_CUSTOM].colors, rainbow_colors, sizeof(rainbow_colors));
    strcpy(palettes[PAL_CUSTOM].name, "Custom");
}

/*============================================================================
 * 获取调色板
 *============================================================================*/

Palette_t* palette_get(PaletteType_t type) {
    if (type < PAL_COUNT) {
        return &palettes[type];
    }
    return &palettes[PAL_RAINBOW];
}

/*============================================================================
 * 从调色板获取颜色 (带插值)
 *============================================================================*/

Color_t palette_color(Palette_t* pal, uint8_t index) {
    if (!pal) {
        return COLOR_BLACK;
    }
    
    // 计算调色板索引和插值因子
    uint8_t idx1 = index >> 4;              // 0-15
    uint8_t idx2 = (idx1 + 1) & 0x0F;       // 下一个索引
    uint8_t blend = (index & 0x0F) << 4;    // 插值因子 0-240
    
    Color_t c1 = pal->colors[idx1];
    Color_t c2 = pal->colors[idx2];
    
    return color_blend(c1, c2, blend);
}

/*============================================================================
 * 从调色板获取颜色 (带亮度)
 *============================================================================*/

Color_t palette_color_brightness(Palette_t* pal, uint8_t index, uint8_t brightness) {
    Color_t c = palette_color(pal, index);
    return color_scale(c, brightness);
}

/*============================================================================
 * 设置自定义调色板颜色
 *============================================================================*/

void palette_set_color(Palette_t* pal, uint8_t index, Color_t color) {
    if (pal && index < PALETTE_SIZE) {
        pal->colors[index] = color;
    }
}

/*============================================================================
 * 创建渐变调色板
 *============================================================================*/

void palette_create_gradient(Palette_t* pal, Color_t c1, Color_t c2, 
                             Color_t c3, Color_t c4) {
    if (!pal) return;
    
    // 四个颜色均匀分布在16个位置
    for (uint8_t i = 0; i < 4; i++) {
        pal->colors[i] = color_blend(c1, c2, i * 64);
    }
    for (uint8_t i = 0; i < 4; i++) {
        pal->colors[4 + i] = color_blend(c2, c3, i * 64);
    }
    for (uint8_t i = 0; i < 4; i++) {
        pal->colors[8 + i] = color_blend(c3, c4, i * 64);
    }
    for (uint8_t i = 0; i < 4; i++) {
        pal->colors[12 + i] = color_blend(c4, c1, i * 64);
    }
}

/*============================================================================
 * 调色板效果
 *============================================================================*/

// 调色板填充
void fx_palette_fill(LedStrip_t* strip, Segment_t* seg, Palette_t* pal) {
    if (!strip || !seg || !pal) return;
    
    uint16_t len = seg->end - seg->start + 1;
    
    for (uint16_t i = 0; i < len; i++) {
        uint8_t idx = ((i * 256 / len) + seg->step) & 0xFF;
        Color_t c = palette_color(pal, idx);
        c = color_scale(c, seg->params.brightness);
        strip->leds[seg->start + i] = c;
    }
    
    seg->step++;
}

// 调色板波浪
void fx_palette_wave(LedStrip_t* strip, Segment_t* seg, Palette_t* pal) {
    if (!strip || !seg || !pal) return;
    
    uint16_t len = seg->end - seg->start + 1;
    
    for (uint16_t i = 0; i < len; i++) {
        uint8_t idx = ((i * 256 / len) + seg->step) & 0xFF;
        uint8_t brightness = sin8(i * 10 + seg->step * 2);
        
        Color_t c = palette_color_brightness(pal, idx, brightness);
        c = color_scale(c, seg->params.brightness);
        strip->leds[seg->start + i] = c;
    }
    
    seg->step++;
}

// 调色板混合
void fx_palette_blend(LedStrip_t* strip, Segment_t* seg, Palette_t* pal) {
    if (!strip || !seg || !pal) return;
    
    uint16_t len = seg->end - seg->start + 1;
    
    for (uint16_t i = 0; i < len; i++) {
        uint8_t idx1 = ((i * 256 / len) + seg->step) & 0xFF;
        uint8_t idx2 = ((i * 256 / len) - seg->step) & 0xFF;
        
        Color_t c1 = palette_color(pal, idx1);
        Color_t c2 = palette_color(pal, idx2);
        
        uint8_t blend = sin8(i * 5 + seg->step);
        Color_t c = color_blend(c1, c2, blend);
        
        c = color_scale(c, seg->params.brightness);
        strip->leds[seg->start + i] = c;
    }
    
    seg->step++;
}
```

## 5. 完整使用示例 (example_full.c)

```c
/*============================================================================
 * WS2812FX 风格效果库 - 完整使用示例
 *============================================================================*/

#include <stdio.h>
#include "led_strip.h"
#include "led_effects_def.h"
#include "led_palette.h"

/*============================================================================
 * 平台适配 (根据实际硬件修改)
 *============================================================================*/

static volatile uint32_t sys_tick_ms = 0;

uint32_t get_tick_ms(void) {
    return sys_tick_ms;
}

void delay_ms(uint32_t ms) {
    uint32_t start = sys_tick_ms;
    while ((sys_tick_ms - start) < ms);
}

// 模拟发送函数 (实际项目中替换为硬件驱动)
void strip_send_data(uint8_t* data, uint16_t len) {
    // SPI/DMA 发送
}

/*============================================================================
 * 效果演示程序
 *============================================================================*/

// 效果演示 - 循环播放所有效果
void demo_all_effects(StripManager_t* manager, LedStrip_t* strip) {
    printf("\n=== 效果演示模式 ===\n");
    printf("总共 %d 个效果\n\n", fx_get_count());
    
    for (FxMode_t mode = FX_STATIC; mode < FX_MODE_COUNT; mode++) {
        printf("效果 %d: %s\n", mode, fx_get_name(mode));
        
        // 设置效果
        fx_set_mode(strip, 0, mode);
        
        // 运行3秒
        uint32_t start = get_tick_ms();
        while ((get_tick_ms() - start) < 3000) {
            strip_manager_update_all(manager);
            delay_ms(1);
            sys_tick_ms++;
        }
    }
}

// 分段效果演示
void demo_segments(StripManager_t* manager, LedStrip_t* strip) {
    printf("\n=== 分段效果演示 ===\n");
    
    // 删除默认线段
    segment_delete(strip, 0);
    
    // 创建4个线段
    int8_t seg1 = segment_create(strip, 0, 14);     // 0-14
    int8_t seg2 = segment_create(strip, 15, 29);    // 15-29
    int8_t seg3 = segment_create(strip, 30, 44);    // 30-44
    int8_t seg4 = segment_create(strip, 45, 59);    // 45-59
    
    // 每个线段设置不同效果
    EffectParams_t params = DEFAULT_EFFECT_PARAMS;
    
    // 线段1: 彩虹
    params.speed = 30;
    fx_set_mode_params(strip, seg1, FX_RAINBOW_CYCLE, &params);
    
    // 线段2: 流星
    params.color1 = COLOR_CYAN;
    params.speed = 40;
    params.size = 5;
    fx_set_mode_params(strip, seg2, FX_METEOR_SMOOTH, &params);
    
    // 线段3: 火焰
    params.speed = 25;
    fx_set_mode_params(strip, seg3, FX_FIRE_2012, &params);
    
    // 线段4: 心跳
    params.color1 = COLOR_RED;
    params.speed = 30;
    fx_set_mode_params(strip, seg4, FX_HEARTBEAT, &params);
    
    printf("线段1: Rainbow Cycle\n");
    printf("线段2: Meteor Smooth\n");
    printf("线段3: Fire 2012\n");
    printf("线段4: Heartbeat\n");
    
    // 运行10秒
    uint32_t start = get_tick_ms();
    while ((get_tick_ms() - start) < 10000) {
        strip_manager_update_all(manager);
        delay_ms(1);
        sys_tick_ms++;
    }
}

// 调色板演示
void demo_palettes(StripManager_t* manager, LedStrip_t* strip) {
    printf("\n=== 调色板演示 ===\n");
    
    // 重置为单线段
    while (strip->num_segments > 0) {
        segment_delete(strip, 0);
    }
    segment_create(strip, 0, strip->num_leds - 1);
    
    Palette_t* palettes_to_demo[] = {
        palette_get(PAL_RAINBOW),
        palette_get(PAL_OCEAN),
        palette_get(PAL_FIRE),
        palette_get(PAL_FOREST),
        palette_get(PAL_SUNSET),
        palette_get(PAL_NEON)
    };
    
    const char* names[] = {
        "Rainbow", "Ocean", "Fire", "Forest", "Sunset", "Neon"
    };
    
    for (int i = 0; i < 6; i++) {
        printf("调色板: %s\n", names[i]);
        
        uint32_t start = get_tick_ms();
        while ((get_tick_ms() - start) < 3000) {
            fx_palette_wave(strip, &strip->segments[0], palettes_to_demo[i]);
            strip_show(strip);
            delay_ms(20);
            sys_tick_ms += 20;
        }
    }
}

// 等待效果演示 (适用于加载场景)
void demo_waiting_effects(StripManager_t* manager, LedStrip_t* strip) {
    printf("\n=== 等待效果演示 ===\n");
    
    // 重置为单线段
    while (strip->num_segments > 0) {
        segment_delete(strip, 0);
    }
    segment_create(strip, 0, strip->num_leds - 1);
    
    FxMode_t waiting_effects[] = {
        FX_LOADING,
        FX_LOADING_DUAL,
        FX_BREATH,
        FX_SCAN,
        FX_LARSON_SCANNER,
        FX_COMET,
        FX_SINELON,
        FX_JUGGLE,
        FX_PACIFICA
    };
    
    const char* names[] = {
        "Loading", "Loading Dual", "Breath", "Scan",
        "Larson Scanner", "Comet", "Sinelon", "Juggle", "Pacifica"
    };
    
    for (int i = 0; i < 9; i++) {
        printf("等待效果: %s\n", names[i]);
        
        EffectParams_t params = DEFAULT_EFFECT_PARAMS;
        params.color1 = COLOR_BLUE;
        params.color2 = color_rgb(10, 10, 30);
        params.speed = 30;
        
        fx_set_mode_params(strip, 0, waiting_effects[i], &params);
        
        uint32_t start = get_tick_ms();
        while ((get_tick_ms() - start) < 3000) {
            strip_manager_update_all(manager);
            delay_ms(1);
            sys_tick_ms++;
        }
    }
}

// 进度条演示
void demo_progress_bar(StripManager_t* manager, LedStrip_t* strip) {
    printf("\n=== 进度条演示 ===\n");
    
    // 重置为单线段
    while (strip->num_segments > 0) {
        segment_delete(strip, 0);
    }
    segment_create(strip, 0, strip->num_leds - 1);
    
    EffectParams_t params = DEFAULT_EFFECT_PARAMS;
    params.color1 = COLOR_GREEN;
    params.color2 = color_rgb(30, 30, 30);
    
    fx_set_mode_params(strip, 0, FX_PERCENT, &params);
    
    // 模拟进度 0-100%
    for (int progress = 0; progress <= 100; progress++) {
        strip->segments[0].aux0 = progress;
        
        printf("\r进度: %d%%", progress);
        fflush(stdout);
        
        strip_manager_update_all(manager);
        delay_ms(50);
        sys_tick_ms += 50;
    }
    printf("\n完成!\n");
}

// 节日效果演示
void demo_holiday_effects(StripManager_t* manager, LedStrip_t* strip) {
    printf("\n=== 节日效果演示 ===\n");
    
    // 重置为单线段
    while (strip->num_segments > 0) {
        segment_delete(strip, 0);
    }
    segment_create(strip, 0, strip->num_leds - 1);
    
    struct {
        FxMode_t mode;
        const char* name;
    } holiday_effects[] = {
        {FX_MERRY_CHRISTMAS, "圣诞节"},
        {FX_HALLOWEEN, "万圣节"},
        {FX_FIREWORKS_RANDOM, "烟花"},
        {FX_TWINKLEFOX, "星光闪烁"},
        {FX_CANDLE_MULTI, "烛光"},
        {FX_FIRE_2012, "壁炉火焰"}
    };
    
    for (int i = 0; i < 6; i++) {
        printf("节日效果: %s\n", holiday_effects[i].name);
        
        fx_set_mode(strip, 0, holiday_effects[i].mode);
        
        uint32_t start = get_tick_ms();
        while ((get_tick_ms() - start) < 4000) {
            strip_manager_update_all(manager);
            delay_ms(1);
            sys_tick_ms++;
        }
    }
}

// 多灯带同步演示
void demo_multi_strip(void) {
    printf("\n=== 多灯带同步演示 ===\n");
    
    StripManager_t* manager = strip_manager_create(get_tick_ms);
    
    // 创建3条灯带
    LedStrip_t* strip1 = led_strip_create(60, COLOR_ORDER_GRB, strip_send_data);
    LedStrip_t* strip2 = led_strip_create(30, COLOR_ORDER_GRB, strip_send_data);
    LedStrip_t* strip3 = led_strip_create(45, COLOR_ORDER_GRBW, strip_send_data);
    
    strip_manager_add(manager, strip1);
    strip_manager_add(manager, strip2);
    strip_manager_add(manager, strip3);
    
    printf("灯带1: 60 LEDs - Rainbow\n");
    printf("灯带2: 30 LEDs - Meteor\n");
    printf("灯带3: 45 LEDs (RGBW) - Fire\n");
    
    // 设置不同效果
    EffectParams_t params = DEFAULT_EFFECT_PARAMS;
    
    params.speed = 30;
    fx_set_mode_params(strip1, 0, FX_RAINBOW_CYCLE, &params);
    
    params.color1 = COLOR_CYAN;
    params.speed = 40;
    fx_set_mode_params(strip2, 0, FX_METEOR_SMOOTH, &params);
    
    params.speed = 25;
    fx_set_mode_params(strip3, 0, FX_FIRE_2012, &params);
    
    // 运行5秒
    uint32_t start = get_tick_ms();
    while ((get_tick_ms() - start) < 5000) {
        strip_manager_update_all(manager);
        delay_ms(1);
        sys_tick_ms++;
    }
    
    strip_manager_destroy(manager);
}

/*============================================================================
 * 主程序
 *============================================================================*/

int main(void) {
    printf("========================================\n");
    printf("  WS2812FX 风格 LED 效果库演示\n");
    printf("  支持 %d 种效果\n", fx_get_count());
    printf("========================================\n");
    
    // 初始化调色板系统
    palette_init();
    
    // 创建灯带管理器
    StripManager_t* manager = strip_manager_create(get_tick_ms);
    
    // 创建灯带 (60个LED, GRB顺序)
    LedStrip_t* strip = led_strip_create(60, COLOR_ORDER_GRB, strip_send_data);
    strip_manager_add(manager, strip);
    
    // 设置全局亮度
    strip_set_brightness(strip, 128);
    
    // 运行各种演示
    while (1) {
        // 1. 等待效果演示
        demo_waiting_effects(manager, strip);
        
        // 2. 进度条演示
        demo_progress_bar(manager, strip);
        
        // 3. 分段效果演示
        demo_segments(manager, strip);
        
        // 4. 调色板演示
        demo_palettes(manager, strip);
        
        // 5. 节日效果演示
        demo_holiday_effects(manager, strip);
        
        // 6. 所有效果循环演示
        demo_all_effects(manager, strip);
        
        // 7. 多灯带演示
        demo_multi_strip();
    }
    
    // 清理
    strip_manager_destroy(manager);
    
    return 0;
}

/*============================================================================
 * 定时器中断 (1ms)
 *============================================================================*/

void SysTick_Handler(void) {
    sys_tick_ms++;
}
```

## 6. 效果速查表

| 效果ID  |    效果名称     |     描述     | 推荐场景 |
| :-----: | :-------------: | :----------: | :------: |
|    0    |     Static      |   静态颜色   | 常亮指示 |
|    1    |      Blink      |     闪烁     | 警告提示 |
|    2    |     Breath      |    呼吸灯    | 待机状态 |
|   3-8   |   Color Wipe    |   颜色擦除   | 状态切换 |
|  11-12  |     Rainbow     |     彩虹     | 装饰展示 |
|  13-14  |      Scan       |     扫描     | 搜索等待 |
|  16-17  |  Theater Chase  |   剧院追逐   | 娱乐氛围 |
|   18    | Running Lights  |    跑马灯    | 动态展示 |
|  19-22  |     Twinkle     |   闪烁星星   | 夜空效果 |
|  23-25  |     Sparkle     |     火花     | 庆祝效果 |
|  26-29  |     Strobe      |     频闪     | 派对效果 |
|  30-39  |      Chase      |     追逐     | 动态指示 |
|   43    | Larson Scanner  | Knight Rider |  科技感  |
|   44    |      Comet      |     彗星     | 流动效果 |
|  45-46  |    Fireworks    |     烟花     | 庆祝场景 |
|   47    | Merry Christmas |    圣诞节    | 节日装饰 |
|   48    |    Halloween    |    万圣节    | 节日装饰 |
|  49-52  |      Fire       |     火焰     | 壁炉效果 |
|  60-62  |     Meteor      |     流星     | 流动效果 |
|  67-68  |     Ripple      |     涟漪     | 水波效果 |
|   69    |    Heartbeat    |     心跳     | 生命指示 |
|   70    |    Pacifica     |    太平洋    | 海洋效果 |
|  72-73  |     Candle      |     蜡烛     | 温馨氛围 |
|   75    |     Sunrise     |     日出     |  唤醒灯  |
|  82-83  |     Loading     |     加载     | 等待状态 |
|  84-85  |     Police      |     警灯     | 警告效果 |
|  89-91  |     Sinelon     |    正弦波    | 平滑动态 |
|   92    |     Juggle      |     杂耍     | 多点动态 |
|   102   |     Percent     |    百分比    | 进度显示 |
| 104-105 | Bouncing Balls  |    弹跳球    | 物理模拟 |

## 7. 架构总览

```text
┌─────────────────────────────────────────────────────────────────────┐
│                         应用层 (Application)                         │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐   │
│  │  效果演示   │ │  进度显示   │ │  节日模式   │ │  自定义场景 │   │
│  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘   │
├─────────────────────────────────────────────────────────────────────┤
│                         效果层 (Effects) - 110+ 效果                 │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐  │
│  │ 基础效果 │ │ 彩虹效果 │ │ 追逐效果 │ │ 火焰效果 │ │ 特殊效果 │  │
│  │ 0-9     │ │ 10-19   │ │ 30-39   │ │ 50-59   │ │ 70-79   │  │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘ └──────────┘  │
├─────────────────────────────────────────────────────────────────────┤
│                         调色板系统 (Palette)                         │
│  Rainbow │ Ocean │ Fire │ Forest │ Sunset │ Neon │ Custom │ ...   │
├─────────────────────────────────────────────────────────────────────┤
│                         核心层 (Core)                                │
│  ┌───────────────┐  ┌───────────────┐  ┌───────────────┐           │
│  │ StripManager  │──│   LedStrip    │──│   Segment     │           │
│  │ 多灯带管理    │  │ 灯带对象      │  │ 线段对象      │           │
│  └───────────────┘  └───────────────┘  └───────────────┘           │
├─────────────────────────────────────────────────────────────────────┤
│                         工具层 (Utils)                               │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐               │
│  │ 颜色转换 │ │ 数学函数 │ │ 随机数   │ │ 插值混合 │               │
│  │ RGB/HSV │ │ sin8/cos8│ │ random8 │ │ blend   │               │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘               │
├─────────────────────────────────────────────────────────────────────┤
│                         硬件抽象层 (HAL)                             │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐              │
│  │ STM32 SPI/DMA│  │ ESP32 RMT    │  │ 其他平台     │              │
│  └──────────────┘  └──────────────┘  └──────────────┘              │
└─────────────────────────────────────────────────────────────────────┘
```

这个扩展版本包含了 **110+ 种效果**，完全参考 WS2812FX 库的设计，并增加了：

1. **完整的效果分类** - 基础、彩虹、追逐、火焰、流星、特殊效果等
2. **调色板系统** - 16种预定义调色板 + 自定义调色板
3. **丰富的等待效果** - Loading、Breath、Scan、Comet等
4. **节日效果** - 圣诞、万圣节、烟花等
5. **物理模拟效果** - 弹跳球、爆米花、水滴等
6. **多灯带支持** - 独立控制多条灯带
7. **线段操作** - 每条灯带可分多个独立线段