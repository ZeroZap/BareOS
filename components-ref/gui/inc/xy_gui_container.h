/**
 * @file xy_gui_container.h
 * @brief GUI Container Widget - 容器控件
 * @version 1.0.0
 * @date 2026-03-13
 */

#ifndef XY_GUI_CONTAINER_H
#define XY_GUI_CONTAINER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_gui_widget.h"

typedef struct xy_gui_container {
    xy_gui_widget_t base;
    xy_gui_widget_t *children;
    uint16_t child_count;
    bool auto_layout;
    uint8_t padding;
    uint8_t spacing;
} xy_gui_container_t;

int xy_gui_container_create(xy_gui_container_t *container, int16_t x, int16_t y, uint16_t w, uint16_t h);
int xy_gui_container_add_child(xy_gui_container_t *container, xy_gui_widget_t *child);
int xy_gui_container_remove_child(xy_gui_container_t *container, xy_gui_widget_t *child);
int xy_gui_container_set_auto_layout(xy_gui_container_t *container, bool enable);
int xy_gui_container_set_padding(xy_gui_container_t *container, uint8_t padding);
int xy_gui_container_set_spacing(xy_gui_container_t *container, uint8_t spacing);

#ifdef __cplusplus
}
#endif

#endif
