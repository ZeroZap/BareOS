/**
 * @file xy_gui_list.h
 * @brief GUI List Widget - 列表控件
 * @version 1.0.0
 * @date 2026-03-13
 */

#ifndef XY_GUI_LIST_H
#define XY_GUI_LIST_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_gui_widget.h"

typedef struct xy_gui_list_item {
    char *text;
    int32_t value;
    struct xy_gui_list_item *next;
} xy_gui_list_item_t;

typedef struct {
    xy_gui_widget_t base;
    xy_gui_list_item_t *items;
    uint16_t item_count;
    uint16_t visible_count;
    uint16_t item_height;
    int16_t scroll_offset;
    int16_t selected_index;
    bool show_scrollbar;
    xy_gui_style_t item_style;
    xy_gui_style_t selected_style;
    xy_gui_event_cb_t on_select;
} xy_gui_list_t;

int xy_gui_list_create(xy_gui_list_t *list, int16_t x, int16_t y, uint16_t w, uint16_t h);
int xy_gui_list_add_item(xy_gui_list_t *list, const char *text, int32_t value);
int xy_gui_list_remove_item(xy_gui_list_t *list, uint16_t index);
int xy_gui_list_clear(xy_gui_list_t *list);
int xy_gui_list_set_selected(xy_gui_list_t *list, int16_t index);
int16_t xy_gui_list_get_selected(xy_gui_list_t *list);
const char* xy_gui_list_get_item_text(xy_gui_list_t *list, uint16_t index);
int xy_gui_list_set_select_cb(xy_gui_list_t *list, xy_gui_event_cb_t cb, void *user_data);

#ifdef __cplusplus
}
#endif

#endif
