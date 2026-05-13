/**
 * @file xy_gui_slider.c
 * @brief GUI Slider Widget Implementation - 滑块控件实现
 * @version 1.0.0
 * @date 2026-03-13
 */

#include "../inc/xy_gui_slider.h"
#include "../inc/xy_gui_display.h"
#include "../inc/xy_gui_draw.h"
#include "xy_font.h"
#include "xy_log.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* 默认尺寸 */
#define DEFAULT_TRACK_WIDTH  4
#define DEFAULT_THUMB_SIZE   16

/* ==================== 内部辅助函数 ==================== */

/**
 * @brief 计算滑块位置
 */
static int16_t calculate_thumb_pos(xy_gui_slider_t *slider)
{
    if (!slider) return 0;
    
    int32_t range = slider->max_value - slider->min_value;
    if (range == 0) return 0;
    
    int32_t value_range = slider->base.value - slider->min_value;
    
    if (slider->direction == XY_GUI_SLIDER_HORIZONTAL) {
        uint16_t track_len = slider->base.rect.width - slider->thumb_size;
        return (int16_t)((value_range * track_len) / range);
    } else {
        uint16_t track_len = slider->base.rect.height - slider->thumb_size;
        return (int16_t)((value_range * track_len) / range);
    }
}

/**
 * @brief 根据位置计算值
 */
static int32_t calculate_value_from_pos(xy_gui_slider_t *slider, int16_t pos)
{
    if (!slider) return 0;
    
    int32_t range = slider->max_value - slider->min_value;
    if (range == 0) return slider->min_value;
    
    uint16_t track_len;
    if (slider->direction == XY_GUI_SLIDER_HORIZONTAL) {
        track_len = slider->base.rect.width - slider->thumb_size;
        if (pos < 0) pos = 0;
        if (pos > track_len) pos = track_len;
    } else {
        track_len = slider->base.rect.height - slider->thumb_size;
        if (pos < 0) pos = 0;
        if (pos > track_len) pos = track_len;
    }
    
    int32_t value = slider->min_value + (pos * range) / track_len;
    
    /* 对齐到步长 */
    if (slider->step > 1) {
        value = (value / slider->step) * slider->step;
    }
    
    return value;
}

/**
 * @brief 绘制轨道
 */
static void draw_track(xy_gui_slider_t *slider,
                       void *fb, uint16_t fb_w, uint16_t fb_h)
{
    if (!slider || !fb) return;
    
    xy_gui_rect_t *r = &slider->base.rect;
    xy_gui_style_t *style = &slider->track_style;
    
    if (slider->direction == XY_GUI_SLIDER_HORIZONTAL) {
        int16_t track_y = r->y + (r->height - slider->track_width) / 2;
        xy_gui_draw_rect(r->x, track_y, r->width, slider->track_width,
                        style->bg_color, true, fb, fb_w, fb_h);
    } else {
        int16_t track_x = r->x + (r->width - slider->track_width) / 2;
        xy_gui_draw_rect(track_x, r->y, slider->track_width, r->height,
                        style->bg_color, true, fb, fb_w, fb_h);
    }
}

/**
 * @brief 绘制滑块
 */
static void draw_thumb(xy_gui_slider_t *slider,
                       void *fb, uint16_t fb_w, uint16_t fb_h)
{
    if (!slider || !fb) return;
    
    xy_gui_rect_t *r = &slider->base.rect;
    xy_gui_style_t *style = &slider->thumb_style;
    int16_t thumb_pos = calculate_thumb_pos(slider);
    
    if (slider->direction == XY_GUI_SLIDER_HORIZONTAL) {
        int16_t thumb_x = r->x + thumb_pos;
        int16_t thumb_y = r->y + (r->height - slider->thumb_size) / 2;
        
        /* 绘制圆形滑块 */
        xy_gui_draw_circle(thumb_x + slider->thumb_size/2,
                          thumb_y + slider->thumb_size/2,
                          slider->thumb_size/2,
                          style->bg_color, true, fb, fb_w, fb_h);
    } else {
        int16_t thumb_x = r->x + (r->width - slider->thumb_size) / 2;
        int16_t thumb_y = r->y + thumb_pos;
        
        /* 绘制圆形滑块 */
        xy_gui_draw_circle(thumb_x + slider->thumb_size/2,
                          thumb_y + slider->thumb_size/2,
                          slider->thumb_size/2,
                          style->bg_color, true, fb, fb_w, fb_h);
    }
}

/**
 * @brief 绘制刻度
 */
static void draw_ticks(xy_gui_slider_t *slider,
                       void *fb, uint16_t fb_w, uint16_t fb_h)
{
    if (!slider || !slider->show_ticks || !fb) return;
    
    xy_gui_rect_t *r = &slider->base.rect;
    int32_t range = slider->max_value - slider->min_value;
    
    if (slider->direction == XY_GUI_SLIDER_HORIZONTAL) {
        for (int32_t v = slider->min_value; v <= slider->max_value; v += slider->step) {
            int16_t pos = (int16_t)(((v - slider->min_value) * r->width) / range);
            int16_t x = r->x + pos;
            xy_gui_draw_line(x, r->y + r->height, x, r->y + r->height + 5,
                           slider->track_style.fg_color, fb, fb_w, fb_h);
        }
    } else {
        for (int32_t v = slider->min_value; v <= slider->max_value; v += slider->step) {
            int16_t pos = (int16_t)(((v - slider->min_value) * r->height) / range);
            int16_t y = r->y + pos;
            xy_gui_draw_line(r->x + r->width, y, r->x + r->width + 5, y,
                           slider->track_style.fg_color, fb, fb_w, fb_h);
        }
    }
}

/**
 * @brief 绘制数值
 */
static void draw_value(xy_gui_slider_t *slider,
                       void *fb, uint16_t fb_w, uint16_t fb_h)
{
    if (!slider || !slider->show_value || !fb) return;
    
    char value_str[16];
    snprintf(value_str, sizeof(value_str), "%ld", (long)slider->base.value);
    
    xy_gui_rect_t *r = &slider->base.rect;
    
    if (slider->direction == XY_GUI_SLIDER_HORIZONTAL) {
        xy_gui_draw_string_center(r->x + r->width/2, r->y - 20,
                                 value_str, slider->track_style.fg_color,
                                 &g_font_8x12, fb, fb_w, fb_h);
    } else {
        xy_gui_draw_string(r->x + r->width + 5, r->y,
                          value_str, slider->track_style.fg_color,
                          &g_font_8x12, fb, fb_w, fb_h);
    }
}

/* ==================== 虚表实现 ==================== */

static int slider_init(xy_gui_widget_t *widget)
{
    if (!widget) return -1;
    
    xy_gui_slider_t *slider = (xy_gui_slider_t*)widget;
    
    /* 初始化默认样式 */
    slider->track_style = (xy_gui_style_t){
        .bg_color = {200, 200, 200, 255},
        .fg_color = (xy_gui_color_t){0,0,0,255},
        .border_color = (xy_gui_color_t){0,0,0,255},
        .border_width = 1,
        .corner_radius = 2,
        .padding = 0,
        .align = XY_GUI_ALIGN_CENTER,
        .visible = true,
        .enabled = true,
    };
    
    slider->thumb_style = (xy_gui_style_t){
        .bg_color = (xy_gui_color_t){0,0,255,255},
        .fg_color = (xy_gui_color_t){255,255,255,255},
        .border_color = (xy_gui_color_t){0,0,0,255},
        .border_width = 1,
        .corner_radius = 8,
        .padding = 0,
        .align = XY_GUI_ALIGN_CENTER,
        .visible = true,
        .enabled = true,
    };
    
    slider->track_width = DEFAULT_TRACK_WIDTH;
    slider->thumb_size = DEFAULT_THUMB_SIZE;
    slider->step = 1;
    slider->show_ticks = false;
    slider->show_value = false;
    slider->continuous = false;
    
    xy_log_d("Slider initialized: %d-%d\n", (int)slider->min_value, (int)slider->max_value);
    return 0;
}

static int slider_deinit(xy_gui_widget_t *widget)
{
    (void)widget;
    xy_log_d("Slider destroyed\n");
    return 0;
}

static int slider_draw(xy_gui_widget_t *widget,
                       void *fb, uint16_t fb_w, uint16_t fb_h)
{
    if (!widget || !fb) return -1;
    
    /* 检查可见性 */
    if (!widget->style.visible) return 0;
    
    xy_gui_slider_t *slider = (xy_gui_slider_t*)widget;
    
    /* 绘制轨道 */
    draw_track(slider, fb, fb_w, fb_h);
    
    /* 绘制滑块 */
    draw_thumb(slider, fb, fb_w, fb_h);
    
    /* 绘制刻度 */
    if (slider->show_ticks) {
        draw_ticks(slider, fb, fb_w, fb_h);
    }
    
    /* 绘制数值 */
    if (slider->show_value) {
        draw_value(slider, fb, fb_w, fb_h);
    }
    
    widget->need_redraw = false;
    return 0;
}

static int slider_update(xy_gui_widget_t *widget,
                         xy_gui_event_t *event)
{
    if (!widget || !event) return -1;
    
    xy_gui_slider_t *slider = (xy_gui_slider_t*)widget;
    
    /* 检查事件是否已处理 */
    if (event->handled) return 0;
    
    /* 检查是否启用 */
    if (!widget->style.enabled) return 0;
    
    int16_t touch_x = event->data.point.x;
    int16_t touch_y = event->data.point.y;
    
    /* 检查是否在滑块区域内 */
    if (!xy_gui_widget_hit_test(widget, touch_x, touch_y)) {
        return 0;
    }
    
    switch (event->type) {
        case XY_GUI_EVENT_PRESS:
        case XY_GUI_EVENT_TOUCH_DOWN: {
            /* 计算新位置 */
            int16_t rel_pos;
            if (slider->direction == XY_GUI_SLIDER_HORIZONTAL) {
                rel_pos = touch_x - widget->rect.x - slider->thumb_size/2;
            } else {
                rel_pos = touch_y - widget->rect.y - slider->thumb_size/2;
            }
            
            /* 计算新值 */
            int32_t new_value = calculate_value_from_pos(slider, rel_pos);
            
            if (new_value != widget->value) {
                slider->base.value = new_value;
                widget->need_redraw = true;
                
                /* 连续模式：实时回调 */
                if (slider->continuous && slider->on_value_changed) {
                    event->data.value = new_value;
                    slider->on_value_changed(widget, event, widget->user_data);
                }
            }
            break;
        }
        
        case XY_GUI_EVENT_RELEASE:
            /* 非连续模式：释放时回调 */
            if (!slider->continuous && slider->on_value_changed) {
                event->data.value = widget->value;
                slider->on_value_changed(widget, event, widget->user_data);
            }
            event->handled = true;
            break;
        
        default:
            break;
    }
    
    return 0;
}

static int slider_set_value(xy_gui_widget_t *widget, int32_t value)
{
    if (!widget) return -1;
    
    xy_gui_slider_t *slider = (xy_gui_slider_t*)widget;
    
    /* 限制在范围内 */
    if (value < slider->min_value) value = slider->min_value;
    if (value > slider->max_value) value = slider->max_value;
    
    /* 对齐到步长 */
    if (slider->step > 1) {
        value = (value / slider->step) * slider->step;
    }
    
    widget->value = value;
    widget->need_redraw = true;
    
    return 0;
}

static int slider_get_value(xy_gui_widget_t *widget)
{
    if (!widget) return 0;
    return widget->value;
}

/* 滑块操作虚表 */
static const xy_gui_widget_ops_t slider_ops = {
    .init = slider_init,
    .deinit = slider_deinit,
    .draw = slider_draw,
    .update = slider_update,
    .set_value = slider_set_value,
    .get_value = slider_get_value,
    .set_text = NULL,
    .get_text = NULL,
};

/* ==================== 公开 API 实现 ==================== */

int xy_gui_slider_create(xy_gui_slider_t *slider,
                         int16_t x, int16_t y,
                         uint16_t length,
                         int32_t min, int32_t max,
                         xy_gui_slider_dir_t direction)
{
    if (!slider) return -1;
    
    memset(slider, 0, sizeof(*slider));
    
    /* 确定尺寸 */
    uint16_t width, height;
    if (direction == XY_GUI_SLIDER_HORIZONTAL) {
        width = length;
        height = 40;  /* 默认高度 */
    } else {
        width = 40;   /* 默认宽度 */
        height = length;
    }
    
    /* 初始化基类 */
    xy_gui_widget_init(&slider->base, XY_GUI_WIDGET_SLIDER, x, y, width, height);
    
    /* 设置操作虚表 */
    slider->base.ops = &slider_ops;
    
    /* 设置范围 */
    slider->min_value = min;
    slider->max_value = max;
    slider->base.value = min;
    slider->direction = direction;
    
    /* 调用初始化 */
    slider_init(&slider->base);
    
    xy_log_i("Slider created: (%d,%d) %dx%d range=%d-%d\n",
             x, y, width, height, (int)min, (int)max);
    
    return 0;
}

int xy_gui_slider_destroy(xy_gui_slider_t *slider)
{
    if (!slider) return -1;
    return slider_deinit(&slider->base);
}

int xy_gui_slider_draw(xy_gui_slider_t *slider,
                       void *framebuffer,
                       uint16_t fb_width,
                       uint16_t fb_height)
{
    if (!slider) return -1;
    return slider_draw(&slider->base, framebuffer, fb_width, fb_height);
}

int xy_gui_slider_update(xy_gui_slider_t *slider,
                         xy_gui_event_t *event)
{
    if (!slider || !event) return -1;
    return slider_update(&slider->base, event);
}

int xy_gui_slider_set_value(xy_gui_slider_t *slider, int32_t value)
{
    if (!slider) return -1;
    return slider_set_value(&slider->base, value);
}

int32_t xy_gui_slider_get_value(xy_gui_slider_t *slider)
{
    if (!slider) return 0;
    return slider_get_value(&slider->base);
}

int xy_gui_slider_set_range(xy_gui_slider_t *slider,
                            int32_t min, int32_t max)
{
    if (!slider || min > max) return -1;
    
    slider->min_value = min;
    slider->max_value = max;
    
    /* 调整当前值到新范围 */
    if (slider->base.value < min) slider->base.value = min;
    if (slider->base.value > max) slider->base.value = max;
    
    slider->base.need_redraw = true;
    return 0;
}

int xy_gui_slider_set_step(xy_gui_slider_t *slider, int32_t step)
{
    if (!slider || step < 1) return -1;
    slider->step = step;
    return 0;
}

int xy_gui_slider_set_direction(xy_gui_slider_t *slider,
                                xy_gui_slider_dir_t direction)
{
    if (!slider) return -1;
    
    if (direction != slider->direction) {
        /* 交换宽高 */
        uint16_t tmp = slider->base.rect.width;
        slider->base.rect.width = slider->base.rect.height;
        slider->base.rect.height = tmp;
        
        slider->direction = direction;
        slider->base.need_redraw = true;
    }
    
    return 0;
}

int xy_gui_slider_show_ticks(xy_gui_slider_t *slider, bool show)
{
    if (!slider) return -1;
    slider->show_ticks = show;
    slider->base.need_redraw = true;
    return 0;
}

int xy_gui_slider_show_value(xy_gui_slider_t *slider, bool show)
{
    if (!slider) return -1;
    slider->show_value = show;
    slider->base.need_redraw = true;
    return 0;
}

int xy_gui_slider_set_continuous(xy_gui_slider_t *slider, bool enable)
{
    if (!slider) return -1;
    slider->continuous = enable;
    return 0;
}

int xy_gui_slider_set_style(xy_gui_slider_t *slider,
                            const xy_gui_style_t *track,
                            const xy_gui_style_t *thumb)
{
    if (!slider) return -1;
    
    if (track) {
        slider->track_style = *track;
    }
    if (thumb) {
        slider->thumb_style = *thumb;
    }
    
    slider->base.need_redraw = true;
    return 0;
}

int xy_gui_slider_set_value_changed_cb(xy_gui_slider_t *slider,
                                       xy_gui_event_cb_t cb,
                                       void *user_data)
{
    if (!slider) return -1;
    slider->on_value_changed = cb;
    slider->base.user_data = user_data;
    return 0;
}

int16_t xy_gui_slider_get_thumb_pos(xy_gui_slider_t *slider)
{
    if (!slider) return 0;
    return calculate_thumb_pos(slider);
}

int32_t xy_gui_slider_pos_to_value(xy_gui_slider_t *slider, int16_t pos)
{
    if (!slider) return 0;
    return calculate_value_from_pos(slider, pos);
}

int16_t xy_gui_slider_value_to_pos(xy_gui_slider_t *slider, int32_t value)
{
    if (!slider) return 0;
    
    int32_t range = slider->max_value - slider->min_value;
    if (range == 0) return 0;
    
    int32_t value_range = value - slider->min_value;
    
    if (slider->direction == XY_GUI_SLIDER_HORIZONTAL) {
        uint16_t track_len = slider->base.rect.width - slider->thumb_size;
        return (int16_t)((value_range * track_len) / range);
    } else {
        uint16_t track_len = slider->base.rect.height - slider->thumb_size;
        return (int16_t)((value_range * track_len) / range);
    }
}
