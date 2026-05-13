/**
 * @file xy_gui_progress.h
 * @brief GUI Progress Bar Widget - 进度条控件
 * @version 1.0.0
 * @date 2026-03-13
 */

#ifndef XY_GUI_PROGRESS_H
#define XY_GUI_PROGRESS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_gui_widget.h"

/**
 * @brief 进度条类型
 */
typedef enum {
    XY_GUI_PROGRESS_BAR = 0,      /**< 普通进度条 */
    XY_GUI_PROGRESS_INDETERMINATE, /**< 不确定进度 (加载动画) */
} xy_gui_progress_type_t;

/**
 * @brief 进度条控件结构
 */
typedef struct {
    xy_gui_widget_t base;
    xy_gui_progress_type_t type;
    int32_t min_value;
    int32_t max_value;
    bool show_text;
    xy_gui_style_t bar_style;
} xy_gui_progress_t;

int xy_gui_progress_create(xy_gui_progress_t *progress, int16_t x, int16_t y,
                           uint16_t width, uint16_t height, int32_t min, int32_t max);
int xy_gui_progress_set_value(xy_gui_progress_t *progress, int32_t value);
int32_t xy_gui_progress_get_value(xy_gui_progress_t *progress);
int xy_gui_progress_set_type(xy_gui_progress_t *progress, xy_gui_progress_type_t type);

#ifdef __cplusplus
}
#endif

#endif
