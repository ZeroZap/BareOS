/**
 * @file xy_gui_dropdown.c
 * @brief GUI Dropdown Widget Implementation
 * @version 1.0.0
 * @date 2026-03-13
 */

#include "../inc/xy_gui_dropdown.h"
#include "../inc/xy_gui_display.h"
#include "../inc/xy_gui_draw.h"
#include "xy_font.h"
#include "xy_log.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static int dropdown_draw(xy_gui_widget_t *widget, void *fb, uint16_t fb_w, uint16_t fb_h)
{
    if (!widget || !fb) return -1;
    if (!widget->style.visible) return 0;
    
    xy_gui_dropdown_t *dd = (xy_gui_dropdown_t*)widget;
    xy_gui_rect_t *r = &widget->rect;
    
    /* 绘制按钮区域 */
    xy_gui_draw_rect(r->x, r->y, r->width, r->height, widget->style.bg_color, true, fb, fb_w, fb_h);
    xy_gui_draw_rect(r->x, r->y, r->width, r->height, widget->style.border_color, false, fb, fb_w, fb_h);
    
    /* 绘制选中文字 */
    if (dd->selected_text) {
        xy_gui_draw_string(r->x + 5, r->y + 3, dd->selected_text, widget->style.fg_color, &g_font_8x12, fb, fb_w, fb_h);
    }
    
    /* 绘制下拉箭头 */
    int16_t arrow_x = r->x + r->width - 15;
    int16_t arrow_y = r->y + r->height/2;
    xy_gui_draw_line(arrow_x, arrow_y-3, arrow_x+5, arrow_y, (xy_gui_color_t){0,0,0,255}, fb, fb_w, fb_h);
    xy_gui_draw_line(arrow_x+5, arrow_y, arrow_x, arrow_y+3, (xy_gui_color_t){0,0,0,255}, fb, fb_w, fb_h);
    
    /* 绘制展开的列表 */
    if (dd->expanded) {
        dd->list.base.style.visible = true;
        dd->list.base.rect.y = r->y + r->height;
        dd->list.base.rect.height = dd->list_height;
        xy_gui_list_draw(&dd->list, fb, fb_w, fb_h);
    } else {
        dd->list.base.style.visible = false;
    }
    
    widget->need_redraw = false;
    return 0;
}

static int dropdown_update(xy_gui_widget_t *widget, xy_gui_event_t *event)
{
    if (!widget || !event) return -1;
    
    xy_gui_dropdown_t *dd = (xy_gui_dropdown_t*)widget;
    xy_gui_rect_t *r = &widget->rect;
    
    /* 处理列表事件 */
    if (dd->expanded) {
        dd->list.base.rect.y = r->y + r->height;
        dd->list.base.rect.height = dd->list_height;
        xy_gui_list_update(&dd->list, event);
        
        if (event->type == XY_GUI_EVENT_RELEASE && !xy_gui_widget_hit_test(&dd->list.base, event->data.point.x, event->data.point.y)) {
            dd->expanded = false;
            widget->need_redraw = true;
        }
        return 0;
    }
    
    /* 处理按钮点击 */
    if (event->type == XY_GUI_EVENT_RELEASE && xy_gui_widget_hit_test(widget, event->data.point.x, event->data.point.y)) {
        dd->expanded = !dd->expanded;
        widget->need_redraw = true;
        event->handled = true;
    }
    
    return 0;
}

static void on_list_select(xy_gui_widget_t *widget, xy_gui_event_t *event, void *user_data)
{
    xy_gui_dropdown_t *dd = (xy_gui_dropdown_t*)user_data;
    int16_t index = xy_gui_list_get_selected(&dd->list);
    
    if (index >= 0) {
        const char *text = xy_gui_list_get_item_text(&dd->list, index);
        if (dd->selected_text) free(dd->selected_text);
        dd->selected_text = strdup(text);
        dd->expanded = false;
        dd->base.need_redraw = true;
        
        if (dd->on_select) {
            event->data.value = index;
            dd->on_select(&dd->base, event, dd->base.user_data);
        }
    }
}

static const xy_gui_widget_ops_t dropdown_ops = {
    .draw = dropdown_draw,
    .update = dropdown_update,
};

int xy_gui_dropdown_create(xy_gui_dropdown_t *dd, int16_t x, int16_t y, uint16_t w, uint16_t h)
{
    if (!dd) return -1;
    memset(dd, 0, sizeof(*dd));
    
    xy_gui_widget_init(&dd->base, XY_GUI_WIDGET_DROPDOWN, x, y, w, h);
    dd->base.ops = &dropdown_ops;
    dd->base.style.bg_color = (xy_gui_color_t){255,255,255,255};
    dd->base.style.fg_color = (xy_gui_color_t){0,0,0,255};
    dd->base.style.border_color = (xy_gui_color_t){128,128,128,255};
    dd->list_height = 100;
    
    /* 创建内部列表 */
    xy_gui_list_create(&dd->list, x, y + h, w, dd->list_height);
    xy_gui_list_set_select_cb(&dd->list, on_list_select, dd);
    dd->list.base.style.visible = false;
    
    xy_log_i("Dropdown created: (%d,%d) %dx%d\n", x, y, w, h);
    return 0;
}

int xy_gui_dropdown_add_item(xy_gui_dropdown_t *dd, const char *text, int32_t value)
{
    if (!dd) return -1;
    return xy_gui_list_add_item(&dd->list, text, value);
}

int xy_gui_dropdown_set_selected(xy_gui_dropdown_t *dd, int16_t index)
{
    if (!dd) return -1;
    
    xy_gui_list_set_selected(&dd->list, index);
    const char *text = xy_gui_list_get_item_text(&dd->list, index);
    
    if (dd->selected_text) free(dd->selected_text);
    dd->selected_text = text ? strdup(text) : NULL;
    dd->base.need_redraw = true;
    
    return 0;
}

int16_t xy_gui_dropdown_get_selected(xy_gui_dropdown_t *dd)
{
    return dd ? xy_gui_list_get_selected(&dd->list) : -1;
}

const char* xy_gui_dropdown_get_selected_text(xy_gui_dropdown_t *dd)
{
    return dd ? dd->selected_text : NULL;
}

int xy_gui_dropdown_set_select_cb(xy_gui_dropdown_t *dd, xy_gui_event_cb_t cb, void *user_data)
{
    if (!dd) return -1;
    dd->on_select = cb;
    dd->base.user_data = user_data;
    return 0;
}
