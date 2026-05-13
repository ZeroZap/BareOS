/**
 * @file xy_gui_textbox.h
 * @brief GUI Textbox Widget - 文本框控件
 * @version 1.0.0
 * @date 2026-03-13
 */

#ifndef XY_GUI_TEXTBOX_H
#define XY_GUI_TEXTBOX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_gui_widget.h"

typedef struct {
    xy_gui_widget_t base;
    char *buffer;
    size_t buffer_size;
    size_t cursor_pos;
    size_t scroll_offset;
    bool multiline;
    bool readonly;
    bool password;
    char password_char;
    xy_gui_style_t caret_style;
    uint32_t last_blink;
    bool caret_visible;
    xy_gui_event_cb_t on_text_changed;
} xy_gui_textbox_t;

int xy_gui_textbox_create(xy_gui_textbox_t *tb, int16_t x, int16_t y, uint16_t w, uint16_t h, size_t max_len);
int xy_gui_textbox_set_text(xy_gui_textbox_t *tb, const char *text);
const char* xy_gui_textbox_get_text(xy_gui_textbox_t *tb);
int xy_gui_textbox_set_readonly(xy_gui_textbox_t *tb, bool readonly);
int xy_gui_textbox_set_multiline(xy_gui_textbox_t *tb, bool multiline);
int xy_gui_textbox_set_password(xy_gui_textbox_t *tb, bool password, char ch);
int xy_gui_textbox_set_changed_cb(xy_gui_textbox_t *tb, xy_gui_event_cb_t cb, void *user_data);

#ifdef __cplusplus
}
#endif

#endif
