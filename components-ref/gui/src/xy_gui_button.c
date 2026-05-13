/**
 * @file xy_gui_button.c
 * @brief GUI Button Widget Implementation - 按钮控件实现
 * @version 1.0.0
 * @date 2026-03-13
 */

#include "../inc/xy_gui_button.h"
#include "../inc/xy_gui_display.h"
#include "../inc/xy_gui_draw.h"
#include "xy_font.h"
#include "xy_log.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* 默认长按时间阈值 (ms) */
#define DEFAULT_LONG_PRESS_TIME  800

/* Forward declaration for tick function */
extern uint32_t xy_os_tick_get(void);

/* ==================== 内部辅助函数 ==================== */

/**
 * @brief 绘制按钮背景
 */
static void draw_button_background(xy_gui_button_t *button,
                                   void *fb, uint16_t fb_w, uint16_t fb_h)
{
    if (!button || !fb) return;
    
    xy_gui_rect_t *r = &button->base.rect;
    xy_gui_style_t *style;
    
    /* 根据状态选择样式 */
    if (!button->base.style.enabled) {
        style = &button->style_disabled;
    } else if (button->is_pressed) {
        style = &button->style_pressed;
    } else {
        style = &button->style_normal;
    }
    
    /* 绘制圆角矩形背景 */
    if (style->corner_radius > 0) {
        xy_gui_draw_rounded_rect(r->x, r->y, r->width, r->height, 
                        style->corner_radius, style->bg_color, true, fb, fb_w, fb_h);
    } else {
        xy_gui_draw_rect(r->x, r->y, r->width, r->height,
                        style->bg_color, true, fb, fb_w, fb_h);
    }
    
    /* 绘制边框 */
    if (style->border_width > 0) {
        xy_gui_draw_rect(r->x, r->y, r->width, r->height,
                        style->border_color, false, fb, fb_w, fb_h);
    }
}

/**
 * @brief 绘制按钮文本
 */
static void draw_button_text(xy_gui_button_t *button,
                             void *fb, uint16_t fb_w, uint16_t fb_h)
{
    if (!button || !button->base.text || !fb) return;
    
    xy_gui_rect_t *r = &button->base.rect;
    xy_gui_style_t *style;
    
    /* 根据状态选择前景色 */
    if (!button->base.style.enabled) {
        style = &button->style_disabled;
    } else if (button->is_pressed) {
        style = &button->style_pressed;
    } else {
        style = &button->style_normal;
    }
    
    /* 计算文本位置 */
    int16_t text_x, text_y;
    uint16_t text_width = xy_font_get_text_width(&g_font_8x12, button->base.text);
    uint16_t text_height = xy_font_get_text_height(&g_font_8x12);
    
    switch (style->align) {
        case XY_GUI_ALIGN_CENTER:
            text_x = r->x + (r->width - text_width) / 2;
            text_y = r->y + (r->height - text_height) / 2;
            break;
        case XY_GUI_ALIGN_LEFT:
            text_x = r->x + style->padding;
            text_y = r->y + (r->height - text_height) / 2;
            break;
        case XY_GUI_ALIGN_RIGHT:
            text_x = r->x + r->width - text_width - style->padding;
            text_y = r->y + (r->height - text_height) / 2;
            break;
        default:
            text_x = r->x + (r->width - text_width) / 2;
            text_y = r->y + (r->height - text_height) / 2;
            break;
    }
    
    /* 绘制文本 */
    xy_font_draw_string(&g_font_8x12, button->base.text,
                      text_x, text_y, 0xFFFF, fb, fb_w, fb_h);
}

/**
 * @brief 绘制按钮图标
 */
static void draw_button_icon(xy_gui_button_t *button,
                             void *fb, uint16_t fb_w, uint16_t fb_h)
{
    if (!button || !button->icon_data || !fb) return;
    
    xy_gui_rect_t *r = &button->base.rect;
    
    /* 计算图标位置 (居中) */
    int16_t icon_x = r->x + (r->width - button->icon_width) / 2;
    int16_t icon_y = r->y + (r->height - button->icon_height) / 2;
    
    /* 图标绘制待实现 - 需要图标资源 */
    /* xy_gui_draw_icon(icon_x, icon_y, button->icon_data, 
                      button->icon_width, button->icon_height, fb, fb_w, fb_h); */
}

/* ==================== 虚表实现 ==================== */

static int button_init(xy_gui_widget_t *widget)
{
    if (!widget) return -1;
    
    xy_gui_button_t *button = (xy_gui_button_t*)widget;
    
    /* 初始化默认样式 */
    button->style_normal = (xy_gui_style_t){
        .bg_color = (xy_gui_color_t){0,0,255,255},
        .fg_color = (xy_gui_color_t){255,255,255,255},
        .border_color = (xy_gui_color_t){0,0,0,255},
        .border_width = 1,
        .corner_radius = 4,
        .padding = 8,
        .align = XY_GUI_ALIGN_CENTER,
        .visible = true,
        .enabled = true,
    };
    
    button->style_pressed = (xy_gui_style_t){
        .bg_color = {0, 100, 200, 255},  /* 深蓝色 */
        .fg_color = (xy_gui_color_t){255,255,255,255},
        .border_color = (xy_gui_color_t){0,0,0,255},
        .border_width = 1,
        .corner_radius = 4,
        .padding = 8,
        .align = XY_GUI_ALIGN_CENTER,
        .visible = true,
        .enabled = true,
    };
    
    button->style_disabled = (xy_gui_style_t){
        .bg_color = (xy_gui_color_t){128,128,128,255},
        .fg_color = {200, 200, 200, 255},
        .border_color = {150, 150, 150, 255},
        .border_width = 1,
        .corner_radius = 4,
        .padding = 8,
        .align = XY_GUI_ALIGN_CENTER,
        .visible = true,
        .enabled = false,
    };
    
    button->long_press_time = DEFAULT_LONG_PRESS_TIME;
    button->is_toggle = (widget->type == XY_GUI_BUTTON_TOGGLE);
    
    xy_log_d("Button initialized: %s\n", widget->text ? widget->text : "NULL");
    return 0;
}

static int button_deinit(xy_gui_widget_t *widget)
{
    if (!widget) return -1;
    
    xy_gui_button_t *button = (xy_gui_button_t*)widget;
    
    /* 释放文本 */
    if (widget->text) {
        free(widget->text);
        widget->text = NULL;
    }
    
    xy_log_d("Button destroyed\n");
    return 0;
}

static int button_draw(xy_gui_widget_t *widget,
                       void *fb, uint16_t fb_w, uint16_t fb_h)
{
    if (!widget || !fb) return -1;
    
    /* 检查可见性 */
    if (!widget->style.visible) return 0;
    
    xy_gui_button_t *button = (xy_gui_button_t*)widget;
    
    /* 绘制背景 */
    draw_button_background(button, fb, fb_w, fb_h);
    
    /* 绘制图标 (如果有) */
    if (button->icon_data) {
        draw_button_icon(button, fb, fb_w, fb_h);
    }
    
    /* 绘制文本 */
    draw_button_text(button, fb, fb_w, fb_h);
    
    widget->need_redraw = false;
    return 0;
}

static int button_update(xy_gui_widget_t *widget,
                         xy_gui_event_t *event)
{
    if (!widget || !event) return -1;
    
    xy_gui_button_t *button = (xy_gui_button_t*)widget;
    
    /* 检查事件是否已处理 */
    if (event->handled) return 0;
    
    /* 检查是否在按钮区域内 */
    if (!xy_gui_widget_hit_test(widget, event->data.point.x, event->data.point.y)) {
        return 0;
    }
    
    switch (event->type) {
        case XY_GUI_EVENT_PRESS:
            if (widget->style.enabled) {
                button->is_pressed = true;
                button->press_time = event->timestamp;
                widget->need_redraw = true;
            }
            break;
            
        case XY_GUI_EVENT_RELEASE:
            if (widget->style.enabled && button->is_pressed) {
                button->is_pressed = false;
                widget->need_redraw = true;
                
                /* 检查是否是点击 (按下时间短于长按阈值) */
                if (event->timestamp - button->press_time < button->long_press_time) {
                    /* 触发点击事件 */
                    if (button->is_toggle) {
                        /* 切换按钮：切换状态 */
                        xy_gui_button_set_checked(button, !xy_gui_button_is_checked(button));
                    }
                    
                    /* 调用点击回调 */
                    if (button->on_click) {
                        button->on_click(widget, event, widget->user_data);
                    }
                    
                    event->handled = true;
                }
            }
            break;
            
        case XY_GUI_EVENT_LONG_PRESS:
            if (widget->style.enabled && button->is_pressed) {
                /* 触发长按回调 */
                if (button->on_long_press) {
                    button->on_long_press(widget, event, widget->user_data);
                }
                event->handled = true;
            }
            break;
            
        default:
            break;
    }
    
    return 0;
}

static int button_set_value(xy_gui_widget_t *widget, int32_t value)
{
    if (!widget) return -1;
    
    widget->value = value;
    widget->need_redraw = true;
    return 0;
}

static int button_get_value(xy_gui_widget_t *widget)
{
    if (!widget) return 0;
    return widget->value;
}

static int button_set_text(xy_gui_widget_t *widget, const char *text)
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
    } else {
        widget->text_len = 0;
    }
    
    widget->need_redraw = true;
    return 0;
}

static const char* button_get_text(xy_gui_widget_t *widget)
{
    if (!widget) return NULL;
    return widget->text;
}

/* 按钮操作虚表 */
static const xy_gui_widget_ops_t button_ops = {
    .init = button_init,
    .deinit = button_deinit,
    .draw = button_draw,
    .update = button_update,
    .set_value = button_set_value,
    .get_value = button_get_value,
    .set_text = button_set_text,
    .get_text = button_get_text,
};

/* ==================== 公开 API 实现 ==================== */

int xy_gui_button_create(xy_gui_button_t *button,
                         int16_t x, int16_t y,
                         uint16_t width, uint16_t height,
                         const char *text,
                         xy_gui_button_flag_t flags)
{
    if (!button) return -1;
    
    memset(button, 0, sizeof(*button));
    
    /* 确定控件类型 */
    xy_gui_widget_type_t type = XY_GUI_WIDGET_BUTTON;
    if (flags == XY_GUI_BUTTON_TOGGLE) {
        type = XY_GUI_BUTTON_TOGGLE;
    } else if (flags == XY_GUI_BUTTON_RADIO) {
        type = XY_GUI_WIDGET_RADIO;
    }
    
    /* 初始化基类 */
    xy_gui_widget_init(&button->base, type, x, y, width, height);
    
    /* 设置操作虚表 */
    button->base.ops = &button_ops;
    
    /* 设置文本 */
    if (text) {
        xy_gui_button_set_text(button, text);
    }
    
    /* 调用初始化 */
    button_init(&button->base);
    
    xy_log_i("Button created: (%d,%d) %dx%d text=\"%s\"\n",
             x, y, width, height, text ? text : "NULL");
    
    return 0;
}

int xy_gui_button_destroy(xy_gui_button_t *button)
{
    if (!button) return -1;
    return button_deinit(&button->base);
}

int xy_gui_button_draw(xy_gui_button_t *button,
                       void *framebuffer,
                       uint16_t fb_width,
                       uint16_t fb_height)
{
    if (!button) return -1;
    return button_draw(&button->base, framebuffer, fb_width, fb_height);
}

int xy_gui_button_update(xy_gui_button_t *button,
                         xy_gui_event_t *event)
{
    if (!button || !event) return -1;
    return button_update(&button->base, event);
}

int xy_gui_button_set_text(xy_gui_button_t *button, const char *text)
{
    if (!button) return -1;
    return button_set_text(&button->base, text);
}

const char* xy_gui_button_get_text(xy_gui_button_t *button)
{
    if (!button) return NULL;
    return button_get_text(&button->base);
}

int xy_gui_button_set_icon(xy_gui_button_t *button,
                           const uint8_t *icon_data,
                           uint16_t width,
                           uint16_t height)
{
    if (!button) return -1;
    
    button->icon_data = icon_data;
    button->icon_width = width;
    button->icon_height = height;
    button->base.need_redraw = true;
    
    return 0;
}

int xy_gui_button_set_style(xy_gui_button_t *button,
                            const xy_gui_style_t *normal,
                            const xy_gui_style_t *pressed,
                            const xy_gui_style_t *disabled)
{
    if (!button) return -1;
    
    if (normal) {
        button->style_normal = *normal;
    }
    if (pressed) {
        button->style_pressed = *pressed;
    }
    if (disabled) {
        button->style_disabled = *disabled;
    }
    
    button->base.need_redraw = true;
    return 0;
}

int xy_gui_button_set_long_press_time(xy_gui_button_t *button,
                                      uint32_t time_ms)
{
    if (!button) return -1;
    button->long_press_time = time_ms;
    return 0;
}

int xy_gui_button_set_click_cb(xy_gui_button_t *button,
                               xy_gui_event_cb_t cb,
                               void *user_data)
{
    if (!button) return -1;
    button->on_click = cb;
    button->base.user_data = user_data;
    return 0;
}

int xy_gui_button_set_long_press_cb(xy_gui_button_t *button,
                                    xy_gui_event_cb_t cb,
                                    void *user_data)
{
    if (!button) return -1;
    button->on_long_press = cb;
    button->base.user_data = user_data;
    return 0;
}

int xy_gui_button_set_pressed(xy_gui_button_t *button, bool pressed)
{
    if (!button) return -1;
    button->is_pressed = pressed;
    button->base.need_redraw = true;
    return 0;
}

bool xy_gui_button_is_pressed(xy_gui_button_t *button)
{
    if (!button) return false;
    return button->is_pressed;
}

int xy_gui_button_set_checked(xy_gui_button_t *button, bool checked)
{
    if (!button) return -1;
    
    if (checked) {
        button->base.state |= XY_GUI_STATE_CHECKED;
        button->base.value = 1;
    } else {
        button->base.state &= ~XY_GUI_STATE_CHECKED;
        button->base.value = 0;
    }
    
    button->base.need_redraw = true;
    return 0;
}

bool xy_gui_button_is_checked(xy_gui_button_t *button)
{
    if (!button) return false;
    return (button->base.state & XY_GUI_STATE_CHECKED) != 0;
}

int xy_gui_button_trigger_click(xy_gui_button_t *button)
{
    if (!button) return -1;
    
    /* 模拟点击事件 */
    xy_gui_event_t event = {
        .type = XY_GUI_EVENT_CLICK,
        .timestamp = xy_os_tick_get(),
        .handled = false,
    };
    
    /* 触发回调 */
    if (button->on_click) {
        button->on_click(&button->base, &event, button->base.user_data);
    }
    
    /* 如果是切换按钮，切换状态 */
    if (button->is_toggle) {
        xy_gui_button_set_checked(button, !xy_gui_button_is_checked(button));
    }
    
    return 0;
}
