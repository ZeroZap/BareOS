/**
 * @file xy_gui_label.c
 * @brief GUI Label Widget Implementation - 标签控件实现
 * @version 1.0.0
 * @date 2026-03-13
 */

#include "../inc/xy_gui_label.h"
#include "../inc/xy_gui_display.h"
#include "../inc/xy_gui_draw.h"
#include "xy_font.h"
#include "xy_log.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* ==================== 内部辅助函数 ==================== */

/**
 * @brief 计算文本行数
 */
static uint16_t count_lines(const char *text, uint16_t max_width, const xy_font_t *font)
{
    if (!text || !font) return 0;
    
    uint16_t lines = 1;
    uint16_t current_width = 0;
    const char *p = text;
    
    while (*p) {
        if (*p == '\n') {
            lines++;
            current_width = 0;
            p++;
            continue;
        }
        
        uint16_t char_width = xy_font_get_text_width(font, p);
        if (current_width + char_width > max_width) {
            lines++;
            current_width = 0;
        } else {
            current_width += char_width;
        }
        p++;
    }
    
    return lines;
}

/**
 * @brief 绘制单行文本
 */
static void draw_text_line(const char *text, uint16_t length,
                          int16_t x, int16_t y, uint16_t max_width,
                          xy_gui_color_t color, const xy_font_t *font,
                          void *fb, uint16_t fb_w, uint16_t fb_h)
{
    if (!text || !font || !fb) return;
    
    xy_gui_draw_string(x, y, text, color, font, fb, fb_w, fb_h);
}

/* ==================== 虚表实现 ==================== */

static int label_init(xy_gui_widget_t *widget)
{
    if (!widget) return -1;
    
    xy_gui_label_t *label = (xy_gui_label_t*)widget;
    
    /* 初始化默认属性 */
    label->auto_size = (widget->rect.width == 0 || widget->rect.height == 0);
    label->word_wrap = false;
    label->multiline = true;
    label->max_lines = 0;
    label->text_align = XY_GUI_ALIGN_LEFT;
    label->font = &g_font_8x12;
    label->ellipsis = false;
    
    /* 默认样式 */
    widget->style = (xy_gui_style_t){
        .bg_color = XY_GUI_COLOR_TRANSPARENT,
        .fg_color = (xy_gui_color_t){0,0,0,255},
        .border_color = XY_GUI_COLOR_TRANSPARENT,
        .border_width = 0,
        .corner_radius = 0,
        .padding = 0,
        .align = XY_GUI_ALIGN_LEFT,
        .visible = true,
        .enabled = true,
    };
    
    xy_log_d("Label initialized: %s\n", widget->text ? widget->text : "NULL");
    return 0;
}

static int label_deinit(xy_gui_widget_t *widget)
{
    if (!widget) return -1;
    
    /* 释放文本 */
    if (widget->text) {
        free(widget->text);
        widget->text = NULL;
    }
    
    xy_log_d("Label destroyed\n");
    return 0;
}

static int label_draw(xy_gui_widget_t *widget,
                      void *fb, uint16_t fb_w, uint16_t fb_h)
{
    if (!widget || !fb) return -1;
    
    /* 检查可见性 */
    if (!widget->style.visible) return 0;
    
    xy_gui_label_t *label = (xy_gui_label_t*)widget;
    xy_gui_rect_t *r = &widget->rect;
    
    /* 绘制背景 (如果有) */
    if (widget->style.bg_color.a > 0) {
        xy_gui_draw_rect(r->x, r->y, r->width, r->height,
                        widget->style.bg_color, true, fb, fb_w, fb_h);
    }
    
    /* 绘制文本 */
    if (widget->text) {
        int16_t text_x = r->x + widget->style.padding;
        int16_t text_y = r->y + widget->style.padding;
        uint16_t line_height = xy_font_get_line_height(label->font);
        uint16_t max_width = r->width - 2 * widget->style.padding;
        
        const char *line_start = widget->text;
        const char *p = widget->text;
        uint16_t line_num = 0;
        
        while (*p && (label->max_lines == 0 || line_num < label->max_lines)) {
            if (*p == '\n' || *(p + 1) == '\0') {
                uint16_t line_len = p - line_start + 1;
                
                /* 计算对齐偏移 */
                uint16_t line_width = xy_font_get_text_width(label->font, line_start);
                switch (label->text_align) {
                    case XY_GUI_ALIGN_CENTER:
                        text_x = r->x + (r->width - line_width) / 2;
                        break;
                    case XY_GUI_ALIGN_RIGHT:
                        text_x = r->x + r->width - line_width - widget->style.padding;
                        break;
                    default:
                        text_x = r->x + widget->style.padding;
                        break;
                }
                
                /* 绘制此行 */
                draw_text_line(line_start, line_len, text_x, text_y,
                             max_width, widget->style.fg_color,
                             label->font, fb, fb_w, fb_h);
                
                text_y += line_height;
                line_num++;
                line_start = p + 1;
            }
            p++;
        }
    }
    
    widget->need_redraw = false;
    return 0;
}

static int label_update(xy_gui_widget_t *widget,
                        xy_gui_event_t *event)
{
    /* 标签通常不处理事件 */
    (void)widget;
    (void)event;
    return 0;
}

static int label_set_text(xy_gui_widget_t *widget, const char *text)
{
    if (!widget) return -1;
    
    /* 释放旧文本 */
    if (widget->text) {
        free(widget->text);
        widget->text = NULL;
    }
    
    /* 复制新文本 */
    if (text) {
        widget->text = strdup(text);
        widget->text_len = strlen(text);
        
        /* 自动调整大小 */
        xy_gui_label_t *label = (xy_gui_label_t*)widget;
        if (label->auto_size) {
            xy_gui_label_resize_to_fit(label);
        }
    } else {
        widget->text_len = 0;
    }
    
    widget->need_redraw = true;
    return 0;
}

static const char* label_get_text(xy_gui_widget_t *widget)
{
    if (!widget) return NULL;
    return widget->text;
}

static int label_set_value(xy_gui_widget_t *widget, int32_t value)
{
    /* 标签支持显示数字 */
    char buf[32];
    snprintf(buf, sizeof(buf), "%ld", (long)value);
    return label_set_text(widget, buf);
}

/* 标签操作虚表 */
static const xy_gui_widget_ops_t label_ops = {
    .init = label_init,
    .deinit = label_deinit,
    .draw = label_draw,
    .update = label_update,
    .set_value = label_set_value,
    .get_value = xy_gui_widget_get_value,
    .set_text = label_set_text,
    .get_text = label_get_text,
};

/* ==================== 公开 API 实现 ==================== */

int xy_gui_label_create(xy_gui_label_t *label,
                        int16_t x, int16_t y,
                        uint16_t width, uint16_t height,
                        const char *text)
{
    if (!label) return -1;
    
    memset(label, 0, sizeof(*label));
    
    /* 初始化基类 */
    xy_gui_widget_init(&label->base, XY_GUI_WIDGET_LABEL, x, y, width, height);
    
    /* 设置操作虚表 */
    label->base.ops = &label_ops;
    
    /* 设置文本 */
    if (text) {
        xy_gui_label_set_text(label, text);
    }
    
    /* 调用初始化 */
    label_init(&label->base);
    
    xy_log_i("Label created: (%d,%d) %dx%d text=\"%s\"\n",
             x, y, width, height, text ? text : "NULL");
    
    return 0;
}

int xy_gui_label_destroy(xy_gui_label_t *label)
{
    if (!label) return -1;
    return label_deinit(&label->base);
}

int xy_gui_label_draw(xy_gui_label_t *label,
                      void *framebuffer,
                      uint16_t fb_width,
                      uint16_t fb_height)
{
    if (!label) return -1;
    return label_draw(&label->base, framebuffer, fb_width, fb_height);
}

int xy_gui_label_update(xy_gui_label_t *label,
                        xy_gui_event_t *event)
{
    if (!label || !event) return -1;
    return label_update(&label->base, event);
}

int xy_gui_label_set_text(xy_gui_label_t *label, const char *text)
{
    if (!label) return -1;
    return label_set_text(&label->base, text);
}

const char* xy_gui_label_get_text(xy_gui_label_t *label)
{
    if (!label) return NULL;
    return label_get_text(&label->base);
}

int xy_gui_label_set_font(xy_gui_label_t *label, const xy_font_t *font)
{
    if (!label || !font) return -1;
    label->font = font;
    label->base.need_redraw = true;
    
    if (label->auto_size) {
        xy_gui_label_resize_to_fit(label);
    }
    
    return 0;
}

int xy_gui_label_set_style(xy_gui_label_t *label, const xy_gui_style_t *style)
{
    if (!label || !style) return -1;
    label->base.style = *style;
    label->base.need_redraw = true;
    return 0;
}

int xy_gui_label_set_word_wrap(xy_gui_label_t *label, bool enable)
{
    if (!label) return -1;
    label->word_wrap = enable;
    label->base.need_redraw = true;
    return 0;
}

int xy_gui_label_set_ellipsis(xy_gui_label_t *label, bool enable)
{
    if (!label) return -1;
    label->ellipsis = enable;
    label->base.need_redraw = true;
    return 0;
}

int xy_gui_label_set_text_align(xy_gui_label_t *label, xy_gui_align_t align)
{
    if (!label) return -1;
    label->text_align = align;
    label->base.need_redraw = true;
    return 0;
}

int xy_gui_label_set_auto_size(xy_gui_label_t *label, bool enable)
{
    if (!label) return -1;
    label->auto_size = enable;
    
    if (enable && label->base.text) {
        xy_gui_label_resize_to_fit(label);
    }
    
    return 0;
}

uint16_t xy_gui_label_get_text_width(xy_gui_label_t *label)
{
    if (!label || !label->base.text) return 0;
    return xy_font_get_text_width(label->font, label->base.text);
}

uint16_t xy_gui_label_get_text_height(xy_gui_label_t *label)
{
    if (!label || !label->base.text) return 0;
    
    uint16_t lines = 1;
    if (label->word_wrap) {
        lines = count_lines(label->base.text, label->base.rect.width, label->font);
    }
    
    return lines * xy_font_get_line_height(label->font);
}

int xy_gui_label_resize_to_fit(xy_gui_label_t *label)
{
    if (!label) return -1;
    
    uint16_t width = xy_gui_label_get_text_width(label);
    uint16_t height = xy_gui_label_get_text_height(label);
    
    label->base.rect.width = width + 2 * label->base.style.padding;
    label->base.rect.height = height + 2 * label->base.style.padding;
    label->base.need_redraw = true;
    
    xy_log_d("Label resized to: %dx%d\n", label->base.rect.width, label->base.rect.height);
    return 0;
}
