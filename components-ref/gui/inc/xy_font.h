/**
 * @file xy_font.h
 * @brief Font Rendering Engine for GUI
 * @version 1.0.0
 * @date 2026-03-01 自主学习
 */

#ifndef XY_FONT_H
#define XY_FONT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief 字体类型
 */
typedef enum {
    XY_FONT_TYPE_BITMAP = 0,    /* 点阵字体 */
    XY_FONT_TYPE_VECTOR,        /* 矢量字体 */
} xy_font_type_t;

/**
 * @brief 字体风格
 */
typedef struct {
    uint8_t size;               /* 字号 (像素) */
    uint8_t weight;             /* 字重 (0-100) */
    bool italic;                /* 斜体 */
    bool underline;             /* 下划线 */
} xy_font_style_t;

/**
 * @brief 字符位图
 */
typedef struct {
    const uint8_t *data;        /* 点阵数据 */
    uint8_t width;              /* 宽度 (像素) */
    uint8_t height;             /* 高度 (像素) */
    int8_t advance;             /* 前进距离 */
    int8_t offset_x;            /* X 偏移 */
    int8_t offset_y;            /* Y 偏移 */
} xy_glyph_t;

/**
 * @brief 字符缓存条目 - ✅ GUI-001
 */
typedef struct {
    char ch;                    /* 字符 */
    uint8_t *cached_data;       /* 缓存的位图数据 */
    uint32_t last_access;       /* 最后访问时间 */
    bool valid;                 /* 缓存有效标志 */
} xy_font_cache_entry_t;

/**
 * @brief 字符缓存配置
 */
typedef struct {
    bool enabled;               /* 缓存使能 */
    uint8_t max_entries;        /* 最大缓存条目数 */
    xy_font_cache_entry_t *entries;  /* 缓存数组 */
} xy_font_cache_t;

/**
 * @brief 字体句柄
 */
typedef struct {
    const char *name;           /* 字体名称 */
    xy_font_type_t type;        /* 字体类型 */
    xy_font_style_t style;      /* 字体风格 */
    
    /* 点阵字体数据 */
    const xy_glyph_t *glyphs;   /* 字符集 */
    uint16_t glyph_count;       /* 字符数量 */
    uint8_t first_char;         /* 起始字符 */
    uint8_t last_char;          /* 结束字符 */
    
    /* 字距调整 */
    const int8_t *kerning;      /* 字距表 */
    
    uint8_t line_height;        /* 行高 */
    uint8_t baseline;           /* 基线位置 */
    
    /* 字符缓存 - ✅ GUI-001 */
    xy_font_cache_t cache;
    
    bool initialized;           /* 初始化标志 */
} xy_font_t;

/**
 * @brief 文本对齐方式
 */
typedef enum {
    XY_ALIGN_LEFT = 0,
    XY_ALIGN_CENTER,
    XY_ALIGN_RIGHT,
    XY_ALIGN_TOP,
    XY_ALIGN_MIDDLE,
    XY_ALIGN_BOTTOM,
} xy_align_t;

/**
 * @brief 文本度量
 */
typedef struct {
    uint16_t width;             /* 文本宽度 */
    uint16_t height;            /* 文本高度 */
    uint16_t lines;             /* 行数 */
} xy_text_metrics_t;

/**
 * @brief 初始化字体
 */
int xy_font_init(xy_font_t *font, const char *name, xy_font_type_t type);

/**
 * @brief 加载字体
 */
int xy_font_load(xy_font_t *font, const void *data, uint32_t size);

/**
 * @brief 获取字符位图
 */
const xy_glyph_t* xy_font_get_glyph(const xy_font_t *font, char ch);

/**
 * @brief 测量文本尺寸
 */
int xy_font_measure_text(const xy_font_t *font, const char *text, 
                         xy_text_metrics_t *metrics);

/**
 * @brief 获取文本宽度
 */
uint16_t xy_font_get_text_width(const xy_font_t *font, const char *text);

/**
 * @brief 获取文本高度
 */
uint16_t xy_font_get_text_height(const xy_font_t *font);

/**
 * @brief 计算行高
 */
uint8_t xy_font_get_line_height(const xy_font_t *font);

/**
 * @brief 绘制字符
 */
int xy_font_draw_char(const xy_font_t *font, char ch, int16_t x, int16_t y,
                      uint16_t color, void *framebuffer, 
                      uint16_t fb_width, uint16_t fb_height);

/**
 * @brief 绘制字符串
 */
int xy_font_draw_string(const xy_font_t *font, const char *text,
                        int16_t x, int16_t y, uint16_t color,
                        void *framebuffer, uint16_t fb_width, uint16_t fb_height);

/**
 * @brief 绘制文本 (带对齐)
 */
int xy_font_draw_text(const xy_font_t *font, const char *text,
                      int16_t x, int16_t y, int16_t max_width,
                      uint16_t color, xy_align_t align,
                      void *framebuffer, uint16_t fb_width, uint16_t fb_height);

/**
 * @brief 初始化字体缓存 - ✅ GUI-001
 * @param font 字体句柄
 * @param max_entries 最大缓存条目数 (0=禁用)
 * @return 0 成功，-1 失败
 */
int xy_font_cache_init(xy_font_t *font, uint8_t max_entries);

/**
 * @brief 预渲染字符到缓存 - ✅ GUI-001
 * @param font 字体句柄
 * @param ch 字符
 * @return 0 成功，-1 失败
 */
int xy_font_cache_glyph(xy_font_t *font, char ch);

/**
 * @brief 从缓存获取字符位图 - ✅ GUI-001
 * @param font 字体句柄
 * @param ch 字符
 * @return 缓存的位图数据，NULL=未缓存
 */
const uint8_t* xy_font_cache_get(xy_font_t *font, char ch);

/**
 * @brief 清空字体缓存
 * @param font 字体句柄
 */
void xy_font_cache_clear(xy_font_t *font);

/* ==================== 内置字体 ==================== */

/**
 * @brief 5x7 点阵字体 (ASCII 32-126)
 */
extern const xy_font_t g_font_5x7;

/**
 * @brief 8x12 点阵字体 (ASCII 32-126)
 */
extern const xy_font_t g_font_8x12;

/**
 * @brief 12x18 点阵字体 (ASCII 32-126)
 */
extern const xy_font_t g_font_12x18;

/**
 * @brief 16x24 点阵字体 (ASCII 32-126)
 */
extern const xy_font_t g_font_16x24;

#ifdef __cplusplus
}
#endif

#endif
