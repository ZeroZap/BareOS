/**
 * @file xy_gui_tab.c
 * @brief GUI Tab Widget Implementation
 * @version 1.0.0
 * @date 2026-03-13
 */

#include "../inc/xy_gui_tab.h"
#include "../inc/xy_gui_display.h"
#include "../inc/xy_gui_draw.h"
#include "xy_font.h"
#include "xy_log.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static void on_tab_click(xy_gui_widget_t *widget, xy_gui_event_t *event, void *user_data)
{
    xy_gui_tab_t *tab = (xy_gui_tab_t*)user_data;
    xy_gui_tab_page_t *page = (xy_gui_tab_page_t*)widget;
    
    /* 查找索引 */
    for (int8_t i = 0; i < tab->page_count; i++) {
        if (&tab->pages[i] == page) {
            xy_gui_tab_set_active(tab, i);
            break;
        }
    }
}

static int tab_draw(xy_gui_widget_t *widget, void *fb, uint16_t fb_w, uint16_t fb_h)
{
    if (!widget || !fb) return -1;
    if (!widget->style.visible) return 0;
    
    xy_gui_tab_t *tab = (xy_gui_tab_t*)widget;
    xy_gui_rect_t *r = &widget->rect;
    
    /* 绘制内容区域背景 */
    xy_gui_draw_rect(r->x, r->y + tab->tab_height, r->width, r->height - tab->tab_height,
                    tab->tab_style.bg_color, true, fb, fb_w, fb_h);
    
    /* 绘制标签页按钮 */
    int16_t btn_x = r->x;
    uint16_t btn_width = r->width / tab->page_count;
    
    for (uint8_t i = 0; i < tab->page_count; i++) {
        xy_gui_tab_page_t *page = &tab->pages[i];
        xy_gui_style_t *style = page->active ? &tab->active_tab_style : &tab->tab_style;
        
        /* 绘制标签背景 */
        xy_gui_draw_rect(btn_x, r->y, btn_width, tab->tab_height, style->bg_color, true, fb, fb_w, fb_h);
        
        /* 绘制标签文本 */
        if (page->title) {
            xy_gui_draw_string_center(btn_x + btn_width/2, r->y + (tab->tab_height-8)/2,
                                     page->title, style->fg_color, &g_font_8x12, fb, fb_w, fb_h);
        }
        
        /* 绘制分隔线 */
        if (i < tab->page_count - 1) {
            xy_gui_draw_line(btn_x + btn_width, r->y, btn_x + btn_width, r->y + tab->tab_height,
                           style->border_color, fb, fb_w, fb_h);
        }
        
        btn_x += btn_width;
    }
    
    /* 绘制当前页内容 */
    if (tab->active_index >= 0 && tab->pages[tab->active_index].content) {
        xy_gui_widget_t *content = tab->pages[tab->active_index].content;
        content->rect.x = r->x;
        content->rect.y = r->y + tab->tab_height;
        content->rect.width = r->width;
        content->rect.height = r->height - tab->tab_height;
        content->ops->draw(content, fb, fb_w, fb_h);
    }
    
    widget->need_redraw = false;
    return 0;
}

static int tab_update(xy_gui_widget_t *widget, xy_gui_event_t *event)
{
    if (!widget || !event) return -1;
    
    xy_gui_tab_t *tab = (xy_gui_tab_t*)widget;
    
    /* 更新所有标签按钮 */
    for (uint8_t i = 0; i < tab->page_count; i++) {
        xy_gui_button_update(&tab->pages[i].tab_btn, event);
    }
    
    /* 更新当前页内容 */
    if (tab->active_index >= 0 && tab->pages[tab->active_index].content) {
        xy_gui_widget_t *content = tab->pages[tab->active_index].content;
        content->ops->update(content, event);
    }
    
    return 0;
}

static const xy_gui_widget_ops_t tab_ops = {
    .draw = tab_draw,
    .update = tab_update,
};

int xy_gui_tab_create(xy_gui_tab_t *tab, int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t tab_h)
{
    if (!tab) return -1;
    memset(tab, 0, sizeof(*tab));
    
    xy_gui_widget_init(&tab->base, XY_GUI_WIDGET_TAB, x, y, w, h);
    tab->base.ops = &tab_ops;
    tab->tab_height = tab_h;
    tab->active_index = -1;
    tab->page_count = 0;
    
    /* 默认样式 */
    tab->tab_style.bg_color = (xy_gui_color_t){230, 230, 230, 255};
    tab->tab_style.fg_color = (xy_gui_color_t){0, 0, 0, 255};
    tab->tab_style.border_color = (xy_gui_color_t){200, 200, 200, 255};
    tab->active_tab_style.bg_color = (xy_gui_color_t){255, 255, 255, 255};
    tab->active_tab_style.fg_color = (xy_gui_color_t){0, 0, 255, 255};
    tab->active_tab_style.border_color = (xy_gui_color_t){0, 0, 255, 255};
    
    xy_log_i("Tab created: (%d,%d) %dx%d tab_h=%d\n", x, y, w, h, tab_h);
    return 0;
}

int xy_gui_tab_add_page(xy_gui_tab_t *tab, const char *title, xy_gui_widget_t *content)
{
    if (!tab || tab->page_count >= XY_GUI_TAB_MAX_PAGES) return -1;
    
    xy_gui_tab_page_t *page = &tab->pages[tab->page_count];
    memset(page, 0, sizeof(*page));
    
    page->title = strdup(title);
    page->content = content;
    page->active = false;
    
    /* 计算按钮位置 */
    int16_t btn_x = tab->base.rect.x + (tab->page_count * tab->base.rect.width / (tab->page_count + 1));
    
    /* 创建标签按钮 */
    xy_gui_button_create(&page->tab_btn, btn_x, tab->base.rect.y,
                        tab->base.rect.width / XY_GUI_TAB_MAX_PAGES, tab->tab_height,
                        title, XY_GUI_BUTTON_NORMAL);
    xy_gui_button_set_click_cb(&page->tab_btn, on_tab_click, tab);
    
    tab->page_count++;
    
    /* 如果是第一个页面，自动激活 */
    if (tab->page_count == 1) {
        xy_gui_tab_set_active(tab, 0);
    }
    
    tab->base.need_redraw = true;
    xy_log_d("Tab page added: \"%s\" (total=%d)\n", title, tab->page_count);
    return 0;
}

int xy_gui_tab_set_active(xy_gui_tab_t *tab, int8_t index)
{
    if (!tab || index < 0 || index >= tab->page_count) return -1;
    
    /* 取消所有激活状态 */
    for (uint8_t i = 0; i < tab->page_count; i++) {
        tab->pages[i].active = false;
    }
    
    /* 激活选中页 */
    tab->pages[index].active = true;
    tab->active_index = index;
    tab->base.need_redraw = true;
    
    /* 触发回调 */
    if (tab->on_page_changed) {
        xy_gui_event_t event = {.type = XY_GUI_EVENT_VALUE_CHANGED, .data.value = index};
        tab->on_page_changed(&tab->base, &event, tab->base.user_data);
    }
    
    xy_log_d("Tab page changed to: %d\n", index);
    return 0;
}

int8_t xy_gui_tab_get_active(xy_gui_tab_t *tab)
{
    return tab ? tab->active_index : -1;
}

int xy_gui_tab_set_changed_cb(xy_gui_tab_t *tab, xy_gui_event_cb_t cb, void *user_data)
{
    if (!tab) return -1;
    tab->on_page_changed = cb;
    tab->base.user_data = user_data;
    return 0;
}
