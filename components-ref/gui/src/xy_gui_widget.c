/**
 * @file xy_gui_widget.c
 * @brief GUI Widget Base Class Implementation
 * @version 1.0.0
 * @date 2026-03-13
 */

#include "xy_gui_widget.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ==================== 内部函数 ==================== */

static int default_init(xy_gui_widget_t *widget)
{
    (void)widget;
    return 0;
}

static int default_deinit(xy_gui_widget_t *widget)
{
    if (widget && widget->text) {
        free(widget->text);
        widget->text = NULL;
    }
    return 0;
}

static int default_draw(xy_gui_widget_t *widget, void *fb, uint16_t fb_w, uint16_t fb_h)
{
    (void)widget;
    (void)fb;
    (void)fb_w;
    (void)fb_h;
    return 0;
}

static int default_update(xy_gui_widget_t *widget, xy_gui_event_t *event)
{
    (void)widget;
    (void)event;
    return 0;
}

static int default_set_value(xy_gui_widget_t *widget, int32_t value)
{
    if (!widget) return -1;
    widget->value = value;
    return 0;
}

static int default_get_value(xy_gui_widget_t *widget)
{
    if (!widget) return 0;
    return widget->value;
}

static int default_set_text(xy_gui_widget_t *widget, const char *text)
{
    if (!widget || !text) return -1;
    
    size_t len = strlen(text);
    if (widget->text) {
        free(widget->text);
    }
    
    widget->text = (char *)malloc(len + 1);
    if (!widget->text) return -1;
    
    strcpy(widget->text, text);
    widget->text_len = len;
    widget->need_redraw = true;
    
    return 0;
}

static const char* default_get_text(xy_gui_widget_t *widget)
{
    if (!widget) return NULL;
    return widget->text;
}

static const xy_gui_widget_ops_t default_ops = {
    .init = default_init,
    .deinit = default_deinit,
    .draw = default_draw,
    .update = default_update,
    .set_value = default_set_value,
    .get_value = default_get_value,
    .set_text = default_set_text,
    .get_text = default_get_text,
};

/* ==================== Widget 基类 API 实现 ==================== */

int xy_gui_widget_init(xy_gui_widget_t *widget, 
                       xy_gui_widget_type_t type,
                       int16_t x, int16_t y,
                       uint16_t width, uint16_t height)
{
    if (!widget) {
        return -1;
    }
    
    memset(widget, 0, sizeof(*widget));
    
    widget->type = type;
    widget->state = XY_GUI_STATE_NORMAL;
    widget->rect.x = x;
    widget->rect.y = y;
    widget->rect.width = width;
    widget->rect.height = height;
    
    /* 默认样式 */
    widget->style.bg_color = (xy_gui_color_t){0, 0, 0, 0};
    widget->style.fg_color = (xy_gui_color_t){0, 0, 0, 255};
    widget->style.border_color = (xy_gui_color_t){0, 0, 0, 255};
    widget->style.border_width = 0;
    widget->style.visible = true;
    widget->style.enabled = true;
    
    /* 默认值范围 */
    widget->min_value = 0;
    widget->max_value = 100;
    
    /* 设置默认操作表 */
    widget->ops = &default_ops;
    
    widget->need_redraw = true;
    
    return 0;
}

int xy_gui_widget_deinit(xy_gui_widget_t *widget)
{
    if (!widget) {
        return -1;
    }
    
    if (widget->ops && widget->ops->deinit) {
        return widget->ops->deinit(widget);
    }
    
    return 0;
}

int xy_gui_widget_draw(xy_gui_widget_t *widget, 
                       void *framebuffer,
                       uint16_t fb_width, 
                       uint16_t fb_height)
{
    if (!widget || !widget->style.visible) {
        return -1;
    }
    
    if (widget->ops && widget->ops->draw) {
        return widget->ops->draw(widget, framebuffer, fb_width, fb_height);
    }
    
    return 0;
}

int xy_gui_widget_update(xy_gui_widget_t *widget, xy_gui_event_t *event)
{
    if (!widget || !widget->style.visible || !widget->style.enabled) {
        return -1;
    }
    
    if (widget->ops && widget->ops->update) {
        return widget->ops->update(widget, event);
    }
    
    return 0;
}

int xy_gui_widget_set_position(xy_gui_widget_t *widget, int16_t x, int16_t y)
{
    if (!widget) {
        return -1;
    }
    
    widget->rect.x = x;
    widget->rect.y = y;
    widget->need_redraw = true;
    
    return 0;
}

int xy_gui_widget_set_pos(xy_gui_widget_t *widget, int16_t x, int16_t y)
{
    if (!widget) {
        return -1;
    }
    
    widget->rect.x = x;
    widget->rect.y = y;
    widget->need_redraw = true;
    
    return 0;
}

int xy_gui_widget_set_size(xy_gui_widget_t *widget, uint16_t width, uint16_t height)
{
    if (!widget) {
        return -1;
    }
    
    widget->rect.width = width;
    widget->rect.height = height;
    widget->need_redraw = true;
    
    return 0;
}

int xy_gui_widget_set_value(xy_gui_widget_t *widget, int32_t value)
{
    if (!widget) {
        return -1;
    }
    
    if (widget->ops && widget->ops->set_value) {
        return widget->ops->set_value(widget, value);
    }
    
    return default_set_value(widget, value);
}

int xy_gui_widget_get_value(xy_gui_widget_t *widget)
{
    if (!widget) {
        return 0;
    }
    
    if (widget->ops && widget->ops->get_value) {
        return widget->ops->get_value(widget);
    }
    
    return default_get_value(widget);
}

int xy_gui_widget_set_text(xy_gui_widget_t *widget, const char *text)
{
    if (!widget) {
        return -1;
    }
    
    if (widget->ops && widget->ops->set_text) {
        return widget->ops->set_text(widget, text);
    }
    
    return default_set_text(widget, text);
}

const char* xy_gui_widget_get_text(xy_gui_widget_t *widget)
{
    if (!widget) {
        return NULL;
    }
    
    if (widget->ops && widget->ops->get_text) {
        return widget->ops->get_text(widget);
    }
    
    return default_get_text(widget);
}

int xy_gui_widget_set_visible(xy_gui_widget_t *widget, bool visible)
{
    if (!widget) {
        return -1;
    }
    
    widget->style.visible = visible;
    widget->need_redraw = true;
    
    return 0;
}

int xy_gui_widget_set_enabled(xy_gui_widget_t *widget, bool enabled)
{
    if (!widget) {
        return -1;
    }
    
    widget->style.enabled = enabled;
    
    if (!enabled) {
        widget->state = XY_GUI_STATE_DISABLED;
    } else {
        widget->state = XY_GUI_STATE_NORMAL;
    }
    
    widget->need_redraw = true;
    
    return 0;
}

bool xy_gui_widget_contains_point(xy_gui_widget_t *widget, int16_t x, int16_t y)
{
    if (!widget || !widget->style.visible) {
        return false;
    }
    
    return (x >= widget->rect.x && x < widget->rect.x + widget->rect.width &&
            y >= widget->rect.y && y < widget->rect.y + widget->rect.height);
}

int xy_gui_widget_add_child(xy_gui_widget_t *parent, xy_gui_widget_t *child)
{
    if (!parent || !child) {
        return -1;
    }
    
    child->parent = parent;
    child->next = NULL;
    
    /* 添加到子控件链表末尾 */
    if (!parent->child) {
        parent->child = child;
    } else {
        xy_gui_widget_t *last = parent->child;
        while (last->next) {
            last = last->next;
        }
        last->next = child;
    }
    
    return 0;
}

int xy_gui_widget_remove_child(xy_gui_widget_t *parent, xy_gui_widget_t *child)
{
    if (!parent || !child || child->parent != parent) {
        return -1;
    }
    
    if (parent->child == child) {
        parent->child = child->next;
    } else {
        xy_gui_widget_t *prev = parent->child;
        while (prev && prev->next != child) {
            prev = prev->next;
        }
        if (prev) {
            prev->next = child->next;
        }
    }
    
    child->parent = NULL;
    child->next = NULL;
    
    return 0;
}
