/**
 * @file xy_gui_checkbox.c
 * @brief GUI Checkbox Widget Implementation - 复选框控件实现
 * @version 1.0.0
 * @date 2026-03-13
 */

#include "../inc/xy_gui_checkbox.h"
#include "../inc/xy_gui_display.h"
#include "../inc/xy_gui_draw.h"
#include "xy_font.h"
#include "xy_log.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* 默认尺寸 */
#define DEFAULT_BOX_SIZE   16
#define DEFAULT_SPACING    8

/* ==================== 内部辅助函数 ==================== */

/**
 * @brief 绘制勾选标记
 */
static void draw_check_mark(xy_gui_checkbox_t *checkbox,
                            void *fb, uint16_t fb_w, uint16_t fb_h)
{
    if (!checkbox || !fb) return;
    
    xy_gui_rect_t *r = &checkbox->base.rect;
    xy_gui_style_t *style = &checkbox->check_style;
    
    int16_t box_x = r->x;
    int16_t box_y = r->y + (r->height - checkbox->box_size) / 2;
    
    if (checkbox->state == XY_GUI_CHECKBOX_CHECKED) {
        /* 绘制勾号 ✓ */
        int16_t check_x = box_x + checkbox->box_size/4;
        int16_t check_y = box_y + checkbox->box_size/2;
        uint16_t check_size = checkbox->box_size/2;
        
        /* 简单的勾号绘制 */
        xy_gui_draw_line(check_x, check_y, check_x + check_size/2, check_y + check_size/2,
                        style->bg_color, fb, fb_w, fb_h);
        xy_gui_draw_line(check_x + check_size/2, check_y + check_size/2,
                        check_x + check_size, check_y,
                        style->bg_color, fb, fb_w, fb_h);
    } else if (checkbox->state == XY_GUI_CHECKBOX_INDETERMINATE) {
        /* 绘制横线 - */
        int16_t line_x = box_x + checkbox->box_size/4;
        int16_t line_y = box_y + checkbox->box_size/2;
        uint16_t line_len = checkbox->box_size/2;
        
        xy_gui_draw_line(line_x, line_y, line_x + line_len, line_y,
                        style->bg_color, fb, fb_w, fb_h);
    }
}

/* ==================== 虚表实现 ==================== */

static int checkbox_init(xy_gui_widget_t *widget)
{
    if (!widget) return -1;
    
    xy_gui_checkbox_t *checkbox = (xy_gui_checkbox_t*)widget;
    
    /* 初始化默认样式 */
    checkbox->box_style = (xy_gui_style_t){
        .bg_color = (xy_gui_color_t){255,255,255,255},
        .fg_color = (xy_gui_color_t){0,0,0,255},
        .border_color = (xy_gui_color_t){0,0,0,255},
        .border_width = 1,
        .corner_radius = 2,
        .padding = 0,
        .align = XY_GUI_ALIGN_LEFT,
        .visible = true,
        .enabled = true,
    };
    
    checkbox->check_style = (xy_gui_style_t){
        .bg_color = (xy_gui_color_t){0,0,0,255},
        .fg_color = (xy_gui_color_t){255,255,255,255},
        .border_color = XY_GUI_COLOR_TRANSPARENT,
        .border_width = 0,
        .corner_radius = 0,
        .padding = 0,
        .align = XY_GUI_ALIGN_CENTER,
        .visible = true,
        .enabled = true,
    };
    
    checkbox->box_size = DEFAULT_BOX_SIZE;
    checkbox->spacing = DEFAULT_SPACING;
    checkbox->state = XY_GUI_CHECKBOX_UNCHECKED;
    checkbox->tristate = false;
    checkbox->radio = false;
    
    xy_log_d("Checkbox initialized: tristate=%d\n", checkbox->tristate);
    return 0;
}

static int checkbox_deinit(xy_gui_widget_t *widget)
{
    if (!widget) return -1;
    
    xy_gui_checkbox_t *checkbox = (xy_gui_checkbox_t*)widget;
    
    /* 释放文本 */
    if (widget->text) {
        free(widget->text);
        widget->text = NULL;
    }
    
    xy_log_d("Checkbox destroyed\n");
    return 0;
}

static int checkbox_draw(xy_gui_widget_t *widget,
                         void *fb, uint16_t fb_w, uint16_t fb_h)
{
    if (!widget || !fb) return -1;
    
    /* 检查可见性 */
    if (!widget->style.visible) return 0;
    
    xy_gui_checkbox_t *checkbox = (xy_gui_checkbox_t*)widget;
    xy_gui_rect_t *r = &widget->rect;
    
    /* 计算方框位置 */
    int16_t box_x = r->x;
    int16_t box_y = r->y + (r->height - checkbox->box_size) / 2;
    
    /* 绘制方框背景 */
    xy_gui_draw_rect(box_x, box_y, checkbox->box_size, checkbox->box_size,
                    checkbox->box_style.bg_color, true, fb, fb_w, fb_h);
    
    /* 绘制方框边框 */
    xy_gui_draw_rect(box_x, box_y, checkbox->box_size, checkbox->box_size,
                    checkbox->box_style.border_color, false, fb, fb_w, fb_h);
    
    /* 绘制勾选标记 */
    if (checkbox->state != XY_GUI_CHECKBOX_UNCHECKED) {
        draw_check_mark(checkbox, fb, fb_w, fb_h);
    }
    
    /* 绘制文本 */
    if (widget->text) {
        int16_t text_x = box_x + checkbox->box_size + checkbox->spacing;
        int16_t text_y = r->y + (r->height - xy_font_get_text_height(&g_font_8x12)) / 2;
        
        xy_gui_draw_string(text_x, text_y, widget->text,
                          widget->style.fg_color, &g_font_8x12, fb, fb_w, fb_h);
    }
    
    widget->need_redraw = false;
    return 0;
}

static int checkbox_update(xy_gui_widget_t *widget,
                           xy_gui_event_t *event)
{
    if (!widget || !event) return -1;
    
    xy_gui_checkbox_t *checkbox = (xy_gui_checkbox_t*)widget;
    
    /* 检查事件是否已处理 */
    if (event->handled) return 0;
    
    /* 检查是否启用 */
    if (!widget->style.enabled) return 0;
    
    /* 检查是否在控件区域内 */
    if (!xy_gui_widget_hit_test(widget, event->data.point.x, event->data.point.y)) {
        return 0;
    }
    
    switch (event->type) {
        case XY_GUI_EVENT_RELEASE:
            /* 切换状态 */
            if (checkbox->tristate) {
                /* 三态循环：未选中 -> 选中 -> 不确定 -> 未选中 */
                checkbox->state = (checkbox->state + 1) % 3;
            } else {
                /* 二态切换 */
                checkbox->state = checkbox->state ? XY_GUI_CHECKBOX_UNCHECKED : XY_GUI_CHECKBOX_CHECKED;
            }
            
            widget->value = checkbox->state;
            widget->need_redraw = true;
            
            /* 触发回调 */
            if (checkbox->on_state_changed) {
                event->data.value = checkbox->state;
                checkbox->on_state_changed(widget, event, widget->user_data);
            }
            
            event->handled = true;
            break;
        
        default:
            break;
    }
    
    return 0;
}

static int checkbox_set_value(xy_gui_widget_t *widget, int32_t value)
{
    if (!widget) return -1;
    
    xy_gui_checkbox_t *checkbox = (xy_gui_checkbox_t*)widget;
    checkbox->state = (xy_gui_checkbox_state_t)value;
    widget->need_redraw = true;
    
    return 0;
}

static int checkbox_get_value(xy_gui_widget_t *widget)
{
    if (!widget) return 0;
    
    xy_gui_checkbox_t *checkbox = (xy_gui_checkbox_t*)widget;
    return (int32_t)checkbox->state;
}

static int checkbox_set_text(xy_gui_widget_t *widget, const char *text)
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

static const char* checkbox_get_text(xy_gui_widget_t *widget)
{
    if (!widget) return NULL;
    return widget->text;
}

/* 复选框操作虚表 */
static const xy_gui_widget_ops_t checkbox_ops = {
    .init = checkbox_init,
    .deinit = checkbox_deinit,
    .draw = checkbox_draw,
    .update = checkbox_update,
    .set_value = checkbox_set_value,
    .get_value = checkbox_get_value,
    .set_text = checkbox_set_text,
    .get_text = checkbox_get_text,
};

/* ==================== 公开 API 实现 ==================== */

int xy_gui_checkbox_create(xy_gui_checkbox_t *checkbox,
                           int16_t x, int16_t y,
                           const char *text,
                           bool tristate)
{
    if (!checkbox) return -1;
    
    memset(checkbox, 0, sizeof(*checkbox));
    
    /* 计算宽度 */
    uint16_t width = checkbox->box_size + checkbox->spacing;
    if (text) {
        width += xy_font_get_text_width(&g_font_8x12, text);
    }
    
    /* 初始化基类 */
    xy_gui_widget_init(&checkbox->base, XY_GUI_WIDGET_CHECKBOX, x, y, width, 24);
    
    /* 设置操作虚表 */
    checkbox->base.ops = &checkbox_ops;
    
    /* 设置属性 */
    checkbox->tristate = tristate;
    checkbox->radio = false;
    
    /* 设置文本 */
    if (text) {
        xy_gui_checkbox_set_text(checkbox, text);
    }
    
    /* 调用初始化 */
    checkbox_init(&checkbox->base);
    
    xy_log_i("Checkbox created: (%d,%d) text=\"%s\" tristate=%d\n",
             x, y, text ? text : "NULL", tristate);
    
    return 0;
}

int xy_gui_radio_create(xy_gui_checkbox_t *checkbox,
                        int16_t x, int16_t y,
                        const char *text)
{
    if (!checkbox) return -1;
    
    /* 创建为圆形单选框 */
    int ret = xy_gui_checkbox_create(checkbox, x, y, text, false);
    if (ret != 0) return ret;
    
    checkbox->radio = true;
    checkbox->box_style.corner_radius = 8;  /* 圆形 */
    
    xy_log_i("Radio created: (%d,%d) text=\"%s\"\n", x, y, text ? text : "NULL");
    
    return 0;
}

int xy_gui_checkbox_destroy(xy_gui_checkbox_t *checkbox)
{
    if (!checkbox) return -1;
    return checkbox_deinit(&checkbox->base);
}

int xy_gui_checkbox_draw(xy_gui_checkbox_t *checkbox,
                         void *framebuffer,
                         uint16_t fb_width,
                         uint16_t fb_height)
{
    if (!checkbox) return -1;
    return checkbox_draw(&checkbox->base, framebuffer, fb_width, fb_height);
}

int xy_gui_checkbox_update(xy_gui_checkbox_t *checkbox,
                           xy_gui_event_t *event)
{
    if (!checkbox || !event) return -1;
    return checkbox_update(&checkbox->base, event);
}

int xy_gui_checkbox_set_checked(xy_gui_checkbox_t *checkbox, bool checked)
{
    if (!checkbox) return -1;
    
    checkbox->state = checked ? XY_GUI_CHECKBOX_CHECKED : XY_GUI_CHECKBOX_UNCHECKED;
    checkbox->base.value = checkbox->state;
    checkbox->base.need_redraw = true;
    
    return 0;
}

bool xy_gui_checkbox_is_checked(xy_gui_checkbox_t *checkbox)
{
    if (!checkbox) return false;
    return checkbox->state == XY_GUI_CHECKBOX_CHECKED;
}

int xy_gui_checkbox_set_state(xy_gui_checkbox_t *checkbox,
                              xy_gui_checkbox_state_t state)
{
    if (!checkbox) return -1;
    
    if (!checkbox->tristate && state == XY_GUI_CHECKBOX_INDETERMINATE) {
        return -1;  /* 非三态不支持不确定状态 */
    }
    
    checkbox->state = state;
    checkbox->base.value = state;
    checkbox->base.need_redraw = true;
    
    return 0;
}

xy_gui_checkbox_state_t xy_gui_checkbox_get_state(xy_gui_checkbox_t *checkbox)
{
    if (!checkbox) return XY_GUI_CHECKBOX_UNCHECKED;
    return checkbox->state;
}

int xy_gui_checkbox_toggle(xy_gui_checkbox_t *checkbox)
{
    if (!checkbox) return -1;
    
    if (checkbox->tristate) {
        checkbox->state = (checkbox->state + 1) % 3;
    } else {
        checkbox->state = checkbox->state ? XY_GUI_CHECKBOX_UNCHECKED : XY_GUI_CHECKBOX_CHECKED;
    }
    
    checkbox->base.value = checkbox->state;
    checkbox->base.need_redraw = true;
    
    return 0;
}

int xy_gui_checkbox_set_text(xy_gui_checkbox_t *checkbox, const char *text)
{
    if (!checkbox) return -1;
    return checkbox_set_text(&checkbox->base, text);
}

const char* xy_gui_checkbox_get_text(xy_gui_checkbox_t *checkbox)
{
    if (!checkbox) return NULL;
    return checkbox_get_text(&checkbox->base);
}

int xy_gui_checkbox_set_style(xy_gui_checkbox_t *checkbox,
                              const xy_gui_style_t *box,
                              const xy_gui_style_t *check)
{
    if (!checkbox) return -1;
    
    if (box) {
        checkbox->box_style = *box;
    }
    if (check) {
        checkbox->check_style = *check;
    }
    
    checkbox->base.need_redraw = true;
    return 0;
}

int xy_gui_checkbox_set_state_changed_cb(xy_gui_checkbox_t *checkbox,
                                         xy_gui_event_cb_t cb,
                                         void *user_data)
{
    if (!checkbox) return -1;
    checkbox->on_state_changed = cb;
    checkbox->base.user_data = user_data;
    return 0;
}

/* ==================== 复选框组实现 ==================== */

int xy_gui_checkbox_group_init(xy_gui_checkbox_group_t *group)
{
    if (!group) return -1;
    memset(group, 0, sizeof(*group));
    return 0;
}

int xy_gui_checkbox_group_add(xy_gui_checkbox_group_t *group,
                              xy_gui_checkbox_t *checkbox)
{
    if (!group || !checkbox) return -1;
    
    /* 设置为单选模式 */
    checkbox->radio = true;
    
    /* 添加到链表 */
    if (!group->head) {
        group->head = checkbox;
    } else {
        xy_gui_checkbox_t *last = group->head;
        while (last->base.next) {
            last = (xy_gui_checkbox_t *)last->base.next;
        }
        last->base.next = (xy_gui_widget_t *)checkbox;
    }
    
    group->count++;
    
    xy_log_d("Checkbox added to group: total=%d\n", group->count);
    return 0;
}

xy_gui_checkbox_t* xy_gui_checkbox_group_get_selected(xy_gui_checkbox_group_t *group)
{
    if (!group || !group->head) return NULL;
    
    xy_gui_checkbox_t *current = group->head;
    uint8_t index = 0;
    
    while (current) {
        if (current->state == XY_GUI_CHECKBOX_CHECKED) {
            group->selected_index = index;
            return current;
        }
        current = (xy_gui_checkbox_t *)current->base.next;
        index++;
    }
    
    return NULL;
}

int xy_gui_checkbox_group_set_selected(xy_gui_checkbox_group_t *group,
                                       uint8_t index)
{
    if (!group || !group->head) return -1;
    
    /* 取消所有选中 */
    xy_gui_checkbox_t *current = group->head;
    while (current) {
        current->state = XY_GUI_CHECKBOX_UNCHECKED;
        current->base.need_redraw = true;
        current = (xy_gui_checkbox_t *)current->base.next;
    }
    
    /* 选中指定索引 */
    current = group->head;
    for (uint8_t i = 0; i < index && current; i++) {
        current = (xy_gui_checkbox_t *)current->base.next;
    }
    
    if (current) {
        current->state = XY_GUI_CHECKBOX_CHECKED;
        current->base.need_redraw = true;
        group->selected_index = index;
    }
    
    return 0;
}
