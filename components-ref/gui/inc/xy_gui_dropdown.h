/**
 * @file xy_gui_dropdown.h
 * @brief GUI Dropdown Widget - 下拉菜单控件
 * @version 1.0.0
 * @date 2026-03-13
 */

#ifndef XY_GUI_DROPDOWN_H
#define XY_GUI_DROPDOWN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_gui_list.h"

typedef struct {
    xy_gui_widget_t base;
    xy_gui_list_t list;
    char *selected_text;
    bool expanded;
    int16_t list_height;
    xy_gui_event_cb_t on_select;
} xy_gui_dropdown_t;

int xy_gui_dropdown_create(xy_gui_dropdown_t *dd, int16_t x, int16_t y, uint16_t w, uint16_t h);
int xy_gui_dropdown_add_item(xy_gui_dropdown_t *dd, const char *text, int32_t value);
int xy_gui_dropdown_set_selected(xy_gui_dropdown_t *dd, int16_t index);
int16_t xy_gui_dropdown_get_selected(xy_gui_dropdown_t *dd);
const char* xy_gui_dropdown_get_selected_text(xy_gui_dropdown_t *dd);
int xy_gui_dropdown_set_select_cb(xy_gui_dropdown_t *dd, xy_gui_event_cb_t cb, void *user_data);

#ifdef __cplusplus
}
#endif

#endif
