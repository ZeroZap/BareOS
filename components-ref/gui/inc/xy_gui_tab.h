/**
 * @file xy_gui_tab.h
 * @brief GUI Tab Widget - 标签页控件
 * @version 1.0.0
 * @date 2026-03-13
 */

#ifndef XY_GUI_TAB_H
#define XY_GUI_TAB_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_gui_widget.h"
#include "xy_gui_button.h"

#define XY_GUI_TAB_MAX_PAGES 16

typedef struct xy_gui_tab_page {
    char *title;
    xy_gui_widget_t *content;
    xy_gui_button_t tab_btn;
    bool active;
} xy_gui_tab_page_t;

typedef struct {
    xy_gui_widget_t base;
    xy_gui_tab_page_t pages[XY_GUI_TAB_MAX_PAGES];
    uint8_t page_count;
    int8_t active_index;
    uint16_t tab_height;
    xy_gui_style_t tab_style;
    xy_gui_style_t active_tab_style;
    xy_gui_event_cb_t on_page_changed;
} xy_gui_tab_t;

int xy_gui_tab_create(xy_gui_tab_t *tab, int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t tab_h);
int xy_gui_tab_add_page(xy_gui_tab_t *tab, const char *title, xy_gui_widget_t *content);
int xy_gui_tab_set_active(xy_gui_tab_t *tab, int8_t index);
int8_t xy_gui_tab_get_active(xy_gui_tab_t *tab);
int xy_gui_tab_set_changed_cb(xy_gui_tab_t *tab, xy_gui_event_cb_t cb, void *user_data);

#ifdef __cplusplus
}
#endif

#endif
