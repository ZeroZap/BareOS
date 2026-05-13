/**
 * @file xy_gui_list.c
 * @brief GUI List Widget Implementation
 * @version 1.0.0
 * @date 2026-03-13
 */

#include "../inc/xy_gui_list.h"
#include "../inc/xy_gui_display.h"
#include "../inc/xy_gui_draw.h"
#include "xy_font.h"
#include "xy_log.h"
#include <string.h>
#include <stdlib.h>

static xy_gui_list_item_t* create_item(const char *text, int32_t value)
{
    xy_gui_list_item_t *item = malloc(sizeof(*item));
    if (!item) return NULL;
    
    item->text = strdup(text);
    item->value = value;
    item->next = NULL;
    return item;
}

static void free_item(xy_gui_list_item_t *item)
{
    if (item) {
        free(item->text);
        free(item);
    }
}

static int list_draw(xy_gui_widget_t *widget, void *fb, uint16_t fb_w, uint16_t fb_h)
{
    if (!widget || !fb) return -1;
    if (!widget->style.visible) return 0;
    
    xy_gui_list_t *list = (xy_gui_list_t*)widget;
    xy_gui_rect_t *r = &widget->rect;
    
    /* 绘制背景 */
    xy_gui_draw_rect(r->x, r->y, r->width, r->height, list->item_style.bg_color, true, fb, fb_w, fb_h);
    
    /* 绘制可见项 */
    xy_gui_list_item_t *item = list->items;
    int16_t y = r->y - list->scroll_offset;
    uint16_t index = 0;
    
    while (item && y < r->y + r->height) {
        if (y + list->item_height > r->y) {
            xy_gui_style_t *style = (index == list->selected_index) ? &list->selected_style : &list->item_style;
            
            /* 绘制项背景 */
            xy_gui_draw_rect(r->x + 1, y, r->width - 2, list->item_height - 1, style->bg_color, true, fb, fb_w, fb_h);
            
            /* 绘制文本 */
            xy_gui_draw_string(r->x + 5, y + 3, item->text, style->fg_color, &g_font_8x12, fb, fb_w, fb_h);
        }
        
        item = item->next;
        y += list->item_height;
        index++;
    }
    
    /* 绘制滚动条 */
    if (list->show_scrollbar && list->item_count > list->visible_count) {
        uint16_t thumb_h = r->height * list->visible_count / list->item_count;
        if (thumb_h < 20) thumb_h = 20;
        int16_t thumb_y = r->y + list->scroll_offset * (r->height - thumb_h) / (list->item_count * list->item_height - r->height);
        xy_gui_color_t scroll_color = {150, 150, 150, 255};
        xy_gui_draw_rect(r->x + r->width - 5, thumb_y, 4, thumb_h, scroll_color, true, fb, fb_w, fb_h);
    }
    
    widget->need_redraw = false;
    return 0;
}

static int list_update(xy_gui_widget_t *widget, xy_gui_event_t *event)
{
    if (!widget || !event) return -1;
    if (!widget->style.enabled) return 0;
    
    xy_gui_list_t *list = (xy_gui_list_t*)widget;
    xy_gui_rect_t *r = &widget->rect;
    
    if (!xy_gui_widget_hit_test(widget, event->data.point.x, event->data.point.y)) return 0;
    
    if (event->type == XY_GUI_EVENT_PRESS || event->type == XY_GUI_EVENT_TOUCH_DOWN) {
        int16_t rel_y = event->data.point.y - r->y + list->scroll_offset;
        int16_t index = rel_y / list->item_height;
        
        if (index >= 0 && index < list->item_count) {
            list->selected_index = index;
            widget->need_redraw = true;
            
            if (list->on_select) {
                event->data.value = index;
                list->on_select(widget, event, widget->user_data);
            }
            event->handled = true;
        }
    }
    
    return 0;
}

static const xy_gui_widget_ops_t list_ops = {
    .draw = list_draw,
    .update = list_update,
    .get_value = xy_gui_widget_get_value,
};

int xy_gui_list_create(xy_gui_list_t *list, int16_t x, int16_t y, uint16_t w, uint16_t h)
{
    if (!list) return -1;
    memset(list, 0, sizeof(*list));
    
    xy_gui_widget_init(&list->base, XY_GUI_WIDGET_LIST, x, y, w, h);
    list->base.ops = &list_ops;
    list->item_height = 24;
    list->visible_count = h / list->item_height;
    list->selected_index = -1;
    list->show_scrollbar = true;
    list->item_style.bg_color = (xy_gui_color_t){255,255,255,255};
    list->item_style.fg_color = (xy_gui_color_t){0,0,0,255};
    list->selected_style.bg_color = (xy_gui_color_t){0,0,255,255};
    list->selected_style.fg_color = (xy_gui_color_t){255,255,255,255};
    
    xy_log_i("List created: (%d,%d) %dx%d\n", x, y, w, h);
    return 0;
}

int xy_gui_list_add_item(xy_gui_list_t *list, const char *text, int32_t value)
{
    if (!list || !text) return -1;
    
    xy_gui_list_item_t *item = create_item(text, value);
    if (!item) return -1;
    
    if (!list->items) {
        list->items = item;
    } else {
        xy_gui_list_item_t *last = list->items;
        while (last->next) last = last->next;
        last->next = item;
    }
    
    list->item_count++;
    list->base.need_redraw = true;
    
    xy_log_d("List item added: \"%s\" (total=%d)\n", text, list->item_count);
    return 0;
}

int xy_gui_list_remove_item(xy_gui_list_t *list, uint16_t index)
{
    if (!list || !list->items || index >= list->item_count) return -1;
    
    xy_gui_list_item_t *item = list->items;
    if (index == 0) {
        list->items = item->next;
        free_item(item);
    } else {
        xy_gui_list_item_t *prev = NULL;
        for (uint16_t i = 0; i < index; i++) {
            prev = item;
            item = item->next;
        }
        prev->next = item->next;
        free_item(item);
    }
    
    list->item_count--;
    if (list->selected_index >= list->item_count) {
        list->selected_index = list->item_count - 1;
    }
    list->base.need_redraw = true;
    return 0;
}

int xy_gui_list_clear(xy_gui_list_t *list)
{
    if (!list) return -1;
    
    xy_gui_list_item_t *item = list->items;
    while (item) {
        xy_gui_list_item_t *next = item->next;
        free_item(item);
        item = next;
    }
    
    list->items = NULL;
    list->item_count = 0;
    list->selected_index = -1;
    list->scroll_offset = 0;
    list->base.need_redraw = true;
    return 0;
}

int xy_gui_list_set_selected(xy_gui_list_t *list, int16_t index)
{
    if (!list || index < -1 || index >= list->item_count) return -1;
    
    list->selected_index = index;
    list->base.need_redraw = true;
    
    /* 自动滚动到选中项 */
    if (index >= 0) {
        int16_t item_y = index * list->item_height;
        if (item_y < list->scroll_offset) {
            list->scroll_offset = item_y;
        } else if (item_y + list->item_height > list->scroll_offset + list->base.rect.height) {
            list->scroll_offset = item_y + list->item_height - list->base.rect.height;
        }
    }
    
    return 0;
}

int16_t xy_gui_list_get_selected(xy_gui_list_t *list)
{
    return list ? list->selected_index : -1;
}

const char* xy_gui_list_get_item_text(xy_gui_list_t *list, uint16_t index)
{
    if (!list || !list->items || index >= list->item_count) return NULL;
    
    xy_gui_list_item_t *item = list->items;
    for (uint16_t i = 0; i < index; i++) item = item->next;
    return item->text;
}

int xy_gui_list_set_select_cb(xy_gui_list_t *list, xy_gui_event_cb_t cb, void *user_data)
{
    if (!list) return -1;
    list->on_select = cb;
    list->base.user_data = user_data;
    return 0;
}
