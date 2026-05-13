/**
 * @file xy_gui_textbox.c
 * @brief GUI Textbox Widget Implementation
 * @version 1.0.0
 * @date 2026-03-13
 */

#include "../inc/xy_gui_textbox.h"
#include "../inc/xy_gui_display.h"
#include "../inc/xy_gui_draw.h"
#include "xy_font.h"
#include "xy_log.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static void update_caret(xy_gui_textbox_t *tb)
{
    uint32_t now = xy_os_tick_get();
    if (now - tb->last_blink > 500) {
        tb->caret_visible = !tb->caret_visible;
        tb->last_blink = now;
        tb->base.need_redraw = true;
    }
}

static int textbox_draw(xy_gui_widget_t *widget, void *fb, uint16_t fb_w, uint16_t fb_h)
{
    if (!widget || !fb) return -1;
    if (!widget->style.visible) return 0;
    
    xy_gui_textbox_t *tb = (xy_gui_textbox_t*)widget;
    xy_gui_rect_t *r = &widget->rect;
    
    /* 绘制背景 */
    xy_gui_draw_rect(r->x, r->y, r->width, r->height, widget->style.bg_color, true, fb, fb_w, fb_h);
    xy_gui_draw_rect(r->x, r->y, r->width, r->height, widget->style.border_color, false, fb, fb_w, fb_h);
    
    /* 绘制文本 */
    const char *text = tb->buffer ? tb->buffer : "";
    if (tb->password) {
        /* 密码模式 */
        size_t len = strlen(text);
        char *pwd = malloc(len + 1);
        memset(pwd, tb->password_char, len);
        pwd[len] = '\0';
        xy_gui_draw_string(r->x + 5, r->y + 3, pwd + tb->scroll_offset, widget->style.fg_color, &g_font_8x12, fb, fb_w, fb_h);
        free(pwd);
    } else {
        xy_gui_draw_string(r->x + 5, r->y + 3, text + tb->scroll_offset, widget->style.fg_color, &g_font_8x12, fb, fb_w, fb_h);
    }
    
    /* 绘制光标 */
    if (widget->style.enabled && !tb->readonly && tb->caret_visible) {
        size_t visible_cursor = tb->cursor_pos - tb->scroll_offset;
        int16_t caret_x = r->x + 5 + visible_cursor * 8;
        xy_gui_draw_line(caret_x, r->y + 2, caret_x, r->y + r->height - 4, tb->caret_style.fg_color, fb, fb_w, fb_h);
    }
    
    update_caret(tb);
    widget->need_redraw = false;
    return 0;
}

static int textbox_update(xy_gui_widget_t *widget, xy_gui_event_t *event)
{
    if (!widget || !event) return -1;
    if (!widget->style.enabled) return 0;
    
    xy_gui_textbox_t *tb = (xy_gui_textbox_t*)widget;
    if (tb->readonly) return 0;
    
    if (!xy_gui_widget_hit_test(widget, event->data.point.x, event->data.point.y)) return 0;
    
    if (event->type == XY_GUI_EVENT_KEY_DOWN) {
        char key = event->data.key.key;
        
        if (key == '\b' && tb->cursor_pos > 0) {
            /* 退格 */
            tb->buffer[--tb->cursor_pos] = '\0';
            if (tb->scroll_offset > 0) tb->scroll_offset--;
            tb->base.need_redraw = true;
            if (tb->on_text_changed) tb->on_text_changed(widget, event, widget->user_data);
        } else if (key >= 32 && key < 127 && tb->cursor_pos < tb->buffer_size - 1) {
            /* 输入字符 */
            tb->buffer[tb->cursor_pos++] = key;
            tb->buffer[tb->cursor_pos] = '\0';
            
            /* 自动滚动 */
            if (tb->cursor_pos - tb->scroll_offset > (tb->base.rect.width - 10) / 8) {
                tb->scroll_offset++;
            }
            tb->base.need_redraw = true;
            if (tb->on_text_changed) tb->on_text_changed(widget, event, widget->user_data);
        }
        event->handled = true;
    }
    
    return 0;
}

static const xy_gui_widget_ops_t textbox_ops = {
    .draw = textbox_draw,
    .update = textbox_update,
    .set_text = NULL,
    .get_text = NULL,
};

int xy_gui_textbox_create(xy_gui_textbox_t *tb, int16_t x, int16_t y, uint16_t w, uint16_t h, size_t max_len)
{
    if (!tb) return -1;
    memset(tb, 0, sizeof(*tb));
    
    tb->buffer = calloc(1, max_len);
    if (!tb->buffer) return -1;
    
    tb->buffer_size = max_len;
    xy_gui_widget_init(&tb->base, XY_GUI_WIDGET_TEXTBOX, x, y, w, h);
    tb->base.ops = &textbox_ops;
    tb->base.style.bg_color = (xy_gui_color_t){255,255,255,255};
    tb->base.style.fg_color = (xy_gui_color_t){0,0,0,255};
    tb->base.style.border_color = (xy_gui_color_t){128,128,128,255};
    tb->base.style.border_width = 1;
    tb->caret_style.fg_color = (xy_gui_color_t){0,0,0,255};
    tb->cursor_pos = 0;
    tb->scroll_offset = 0;
    tb->multiline = false;
    tb->readonly = false;
    tb->password = false;
    tb->password_char = '*';
    tb->last_blink = xy_os_tick_get();
    tb->caret_visible = true;
    
    xy_log_i("Textbox created: (%d,%d) %dx%d max_len=%d\n", x, y, w, h, (int)max_len);
    return 0;
}

int xy_gui_textbox_set_text(xy_gui_textbox_t *tb, const char *text)
{
    if (!tb || !tb->buffer || !text) return -1;
    strncpy(tb->buffer, text, tb->buffer_size - 1);
    tb->buffer[tb->buffer_size - 1] = '\0';
    tb->cursor_pos = strlen(tb->buffer);
    tb->scroll_offset = 0;
    tb->base.need_redraw = true;
    return 0;
}

const char* xy_gui_textbox_get_text(xy_gui_textbox_t *tb)
{
    return tb ? tb->buffer : NULL;
}

int xy_gui_textbox_set_readonly(xy_gui_textbox_t *tb, bool readonly)
{
    if (!tb) return -1;
    tb->readonly = readonly;
    return 0;
}

int xy_gui_textbox_set_multiline(xy_gui_textbox_t *tb, bool multiline)
{
    if (!tb) return -1;
    tb->multiline = multiline;
    return 0;
}

int xy_gui_textbox_set_password(xy_gui_textbox_t *tb, bool password, char ch)
{
    if (!tb) return -1;
    tb->password = password;
    tb->password_char = ch ? ch : '*';
    return 0;
}

int xy_gui_textbox_set_changed_cb(xy_gui_textbox_t *tb, xy_gui_event_cb_t cb, void *user_data)
{
    if (!tb) return -1;
    tb->on_text_changed = cb;
    tb->base.user_data = user_data;
    return 0;
}
