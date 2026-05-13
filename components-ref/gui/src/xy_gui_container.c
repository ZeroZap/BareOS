/**
 * @file xy_gui_container.c
 * @brief GUI Container Widget Implementation
 * @version 1.0.0
 * @date 2026-03-13
 */

#include "../inc/xy_gui_container.h"
#include "../inc/xy_gui_display.h"
#include "../inc/xy_gui_draw.h"
#include "xy_log.h"
#include <string.h>
#include <stdlib.h>

static int container_draw(xy_gui_widget_t *widget, void *fb, uint16_t fb_w, uint16_t fb_h)
{
    if (!widget || !fb) return -1;
    if (!widget->style.visible) return 0;
    
    xy_gui_container_t *container = (xy_gui_container_t*)widget;
    xy_gui_rect_t *r = &widget->rect;
    
    /* 绘制背景 */
    if (widget->style.bg_color.a > 0) {
        xy_gui_draw_rect(r->x, r->y, r->width, r->height, widget->style.bg_color, true, fb, fb_w, fb_h);
    }
    
    /* 绘制边框 */
    if (widget->style.border_width > 0) {
        xy_gui_draw_rect(r->x, r->y, r->width, r->height, widget->style.border_color, false, fb, fb_w, fb_h);
    }
    
    /* 绘制子控件 */
    xy_gui_widget_t *child = container->children;
    while (child) {
        if (child->style.visible) {
            child->ops->draw(child, fb, fb_w, fb_h);
        }
        child = child->next;
    }
    
    widget->need_redraw = false;
    return 0;
}

static int container_update(xy_gui_widget_t *widget, xy_gui_event_t *event)
{
    if (!widget || !event) return -1;
    
    xy_gui_container_t *container = (xy_gui_container_t*)widget;
    
    /* 更新所有子控件 (反向遍历，先更新上层控件) */
    xy_gui_widget_t *child = container->children;
    while (child) {
        if (child->style.visible && child->ops->update) {
            child->ops->update(child, event);
        }
        child = child->next;
    }
    
    return 0;
}

static const xy_gui_widget_ops_t container_ops = {
    .draw = container_draw,
    .update = container_update,
};

int xy_gui_container_create(xy_gui_container_t *container, int16_t x, int16_t y, uint16_t w, uint16_t h)
{
    if (!container) return -1;
    memset(container, 0, sizeof(*container));
    
    xy_gui_widget_init(&container->base, XY_GUI_WIDGET_CONTAINER, x, y, w, h);
    container->base.ops = &container_ops;
    container->base.style.bg_color = (xy_gui_color_t){0, 0, 0, 0};
    container->auto_layout = false;
    container->padding = 0;
    container->spacing = 0;
    
    xy_log_i("Container created: (%d,%d) %dx%d\n", x, y, w, h);
    return 0;
}

int xy_gui_container_add_child(xy_gui_container_t *container, xy_gui_widget_t *child)
{
    if (!container || !child) return -1;
    
    /* 添加到链表末尾 */
    if (!container->children) {
        container->children = child;
    } else {
        xy_gui_widget_t *last = container->children;
        while (last->next) last = last->next;
        last->next = child;
    }
    
    child->parent = &container->base;
    container->child_count++;
    container->base.need_redraw = true;
    
    /* 自动布局 */
    if (container->auto_layout) {
        child->rect.x = container->base.rect.x + container->padding;
        child->rect.y = container->base.rect.y + container->padding + 
                       (container->child_count - 1) * (child->rect.height + container->spacing);
    }
    
    xy_log_d("Container child added: %d children\n", container->child_count);
    return 0;
}

int xy_gui_container_remove_child(xy_gui_container_t *container, xy_gui_widget_t *child)
{
    if (!container || !child) return -1;
    
    xy_gui_widget_t *prev = NULL;
    xy_gui_widget_t *curr = container->children;
    
    while (curr && curr != child) {
        prev = curr;
        curr = curr->next;
    }
    
    if (curr) {
        if (prev) {
            prev->next = curr->next;
        } else {
            container->children = curr->next;
        }
        container->child_count--;
        container->base.need_redraw = true;
    }
    
    return 0;
}

int xy_gui_container_set_auto_layout(xy_gui_container_t *container, bool enable)
{
    if (!container) return -1;
    container->auto_layout = enable;
    return 0;
}

int xy_gui_container_set_padding(xy_gui_container_t *container, uint8_t padding)
{
    if (!container) return -1;
    container->padding = padding;
    return 0;
}

int xy_gui_container_set_spacing(xy_gui_container_t *container, uint8_t spacing)
{
    if (!container) return -1;
    container->spacing = spacing;
    return 0;
}
