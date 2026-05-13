/**
 * @file xy_gui_progress.c
 * @brief GUI Progress Bar Implementation
 * @version 1.0.0
 * @date 2026-03-13
 */

#include "../inc/xy_gui_progress.h"
#include "../inc/xy_gui_display.h"
#include "../inc/xy_gui_draw.h"
#include "xy_font.h"
#include "xy_log.h"
#include <string.h>
#include <stdio.h>

static int progress_draw(xy_gui_widget_t *widget, void *fb, uint16_t fb_w, uint16_t fb_h)
{
    if (!widget || !fb) return -1;
    if (!widget->style.visible) return 0;
    
    xy_gui_progress_t *p = (xy_gui_progress_t*)widget;
    xy_gui_rect_t *r = &widget->rect;
    
    /* 绘制背景 */
    xy_gui_color_t bg_color = {200, 200, 200, 255};
    xy_gui_draw_rect(r->x, r->y, r->width, r->height, bg_color, true, fb, fb_w, fb_h);
    
    /* 计算进度宽度 */
    int32_t range = p->max_value - p->min_value;
    uint16_t fill_width = (range > 0) ? (widget->value - p->min_value) * r->width / range : 0;
    
    /* 绘制进度条 */
    if (fill_width > 0) {
        xy_gui_draw_rect(r->x, r->y, fill_width, r->height, p->bar_style.bg_color, true, fb, fb_w, fb_h);
    }
    
    /* 绘制文本 */
    if (p->show_text && range > 0) {
        char text[16];
        uint8_t percent = (widget->value - p->min_value) * 100 / range;
        snprintf(text, sizeof(text), "%d%%", percent);
        xy_gui_draw_string_center(r->x + r->width/2, r->y + (r->height-8)/2,
                                 text, p->bar_style.fg_color, &g_font_8x12, fb, fb_w, fb_h);
    }
    
    widget->need_redraw = false;
    return 0;
}

static int progress_set_value(xy_gui_widget_t *widget, int32_t value)
{
    if (!widget) return -1;
    xy_gui_progress_t *p = (xy_gui_progress_t*)widget;
    
    if (value < p->min_value) value = p->min_value;
    if (value > p->max_value) value = p->max_value;
    
    widget->value = value;
    widget->need_redraw = true;
    return 0;
}

static const xy_gui_widget_ops_t progress_ops = {
    .draw = progress_draw,
    .set_value = progress_set_value,
    .get_value = xy_gui_widget_get_value,
};

int xy_gui_progress_create(xy_gui_progress_t *progress, int16_t x, int16_t y,
                           uint16_t width, uint16_t height, int32_t min, int32_t max)
{
    if (!progress) return -1;
    memset(progress, 0, sizeof(*progress));
    
    xy_gui_widget_init(&progress->base, XY_GUI_WIDGET_PROGRESS, x, y, width, height);
    progress->base.ops = &progress_ops;
    progress->min_value = min;
    progress->max_value = max;
    progress->base.value = min;
    progress->show_text = true;
    progress->bar_style.bg_color = (xy_gui_color_t){0, 0, 255, 255};
    progress->bar_style.fg_color = (xy_gui_color_t){255, 255, 255, 255};
    
    xy_log_i("Progress created: (%d,%d) %dx%d\n", x, y, width, height);
    return 0;
}

int xy_gui_progress_set_value(xy_gui_progress_t *progress, int32_t value)
{
    return progress_set_value(&progress->base, value);
}

int32_t xy_gui_progress_get_value(xy_gui_progress_t *progress)
{
    return progress ? progress->base.value : 0;
}

int xy_gui_progress_set_type(xy_gui_progress_t *progress, xy_gui_progress_type_t type)
{
    if (!progress) return -1;
    progress->type = type;
    return 0;
}
