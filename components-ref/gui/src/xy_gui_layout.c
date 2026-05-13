/**
 * @file xy_gui_layout.c
 * @brief GUI Layout System Implementation - 布局系统实现
 * @version 1.0.0
 * @date 2026-03-14
 */

#include "xy_gui_layout.h"
#include <string.h>
#include <stdlib.h>

static bool s_layout_system_initialized = false;

/* ==================== 布局系统核心 ==================== */

int xy_gui_layout_system_init(void)
{
    if (s_layout_system_initialized) {
        return -1;  /* 已经初始化 */
    }
    
    s_layout_system_initialized = true;
    return 0;
}

/* ==================== 布局配置 API ==================== */

int xy_gui_layout_set_config(xy_gui_widget_t *container, 
                             const xy_gui_layout_config_t *config)
{
    if (!container || !config) {
        return -1;
    }
    
    /* 将配置存储到容器的 user_data 中 */
    xy_gui_layout_config_t *stored_config = 
        (xy_gui_layout_config_t *)container->user_data;
    
    if (!stored_config) {
        stored_config = (xy_gui_layout_config_t *)malloc(sizeof(*stored_config));
        if (!stored_config) {
            return -1;
        }
        container->user_data = stored_config;
    }
    
    memcpy(stored_config, config, sizeof(*config));
    container->need_redraw = true;
    
    return 0;
}

int xy_gui_layout_get_config(xy_gui_widget_t *container,
                             xy_gui_layout_config_t *config)
{
    if (!container || !config) {
        return -1;
    }
    
    xy_gui_layout_config_t *stored_config = 
        (xy_gui_layout_config_t *)container->user_data;
    
    if (!stored_config) {
        /* 返回默认配置 */
        memset(config, 0, sizeof(*config));
        config->type = XY_GUI_LAYOUT_NONE;
        return 0;
    }
    
    memcpy(config, stored_config, sizeof(*config));
    return 0;
}

int xy_gui_layout_set_item_config(xy_gui_widget_t *child,
                                  const xy_gui_layout_item_config_t *config)
{
    if (!child || !config) {
        return -1;
    }
    
    /* 存储到子控件的 user_data 中 */
    xy_gui_layout_item_config_t *stored_config = 
        (xy_gui_layout_item_config_t *)child->user_data;
    
    if (!stored_config) {
        stored_config = (xy_gui_layout_item_config_t *)malloc(sizeof(*stored_config));
        if (!stored_config) {
            return -1;
        }
        child->user_data = stored_config;
    }
    
    memcpy(stored_config, config, sizeof(*config));
    return 0;
}

/* ==================== 垂直布局 ==================== */

int xy_gui_layout_vertical(xy_gui_widget_t *container)
{
    if (!container || !container->child) {
        return -1;
    }
    
    xy_gui_rect_t *r = &container->rect;
    int16_t y = r->y;
    
    /* 获取配置 */
    xy_gui_layout_config_t config;
    if (xy_gui_layout_get_config(container, &config) == 0) {
        y += config.main_padding;
    } else {
        config.main_spacing = 5;  /* 默认间距 */
        config.main_padding = 5;
        config.cross_align = XY_GUI_LAYOUT_ALIGN_START;
        y += config.main_padding;
    }
    
    /* 遍历子控件 */
    xy_gui_widget_t *child = container->child;
    while (child) {
        if (child->style.visible) {
            /* 获取子控件配置 */
            uint16_t child_height = child->rect.height;
            xy_gui_layout_item_config_t *item_config = 
                (xy_gui_layout_item_config_t *)child->user_data;
            
            if (item_config && item_config->size_mode == XY_GUI_LAYOUT_SIZE_EXPAND) {
                /* 弹性扩展：填充剩余空间 */
                child_height = r->height - (y - r->y) - config.main_padding;
            }
            
            /* 设置位置 */
            child->rect.y = y;
            child->rect.height = child_height;
            
            /* 水平对齐 */
            switch (config.cross_align) {
                case XY_GUI_LAYOUT_ALIGN_CENTER:
                    child->rect.x = r->x + (r->width - child->rect.width) / 2;
                    break;
                case XY_GUI_LAYOUT_ALIGN_END:
                    child->rect.x = r->x + r->width - child->rect.width;
                    break;
                case XY_GUI_LAYOUT_ALIGN_STRETCH:
                    child->rect.x = r->x;
                    child->rect.width = r->width;
                    break;
                default: /* START */
                    child->rect.x = r->x;
                    break;
            }
            
            y += child_height + config.main_spacing;
        }
        
        child = child->next;
    }
    
    return 0;
}

/* ==================== 水平布局 ==================== */

int xy_gui_layout_horizontal(xy_gui_widget_t *container)
{
    if (!container || !container->child) {
        return -1;
    }
    
    xy_gui_rect_t *r = &container->rect;
    int16_t x = r->x;
    
    /* 获取配置 */
    xy_gui_layout_config_t config;
    if (xy_gui_layout_get_config(container, &config) == 0) {
        x += config.main_padding;
    } else {
        config.main_spacing = 5;
        config.main_padding = 5;
        config.cross_align = XY_GUI_LAYOUT_ALIGN_START;
        x += config.main_padding;
    }
    
    /* 遍历子控件 */
    xy_gui_widget_t *child = container->child;
    while (child) {
        if (child->style.visible) {
            /* 获取子控件配置 */
            uint16_t child_width = child->rect.width;
            xy_gui_layout_item_config_t *item_config = 
                (xy_gui_layout_item_config_t *)child->user_data;
            
            if (item_config && item_config->size_mode == XY_GUI_LAYOUT_SIZE_EXPAND) {
                /* 弹性扩展：填充剩余空间 */
                child_width = r->width - (x - r->x) - config.main_padding;
            }
            
            /* 设置位置 */
            child->rect.x = x;
            child->rect.width = child_width;
            
            /* 垂直对齐 */
            switch (config.cross_align) {
                case XY_GUI_LAYOUT_ALIGN_CENTER:
                    child->rect.y = r->y + (r->height - child->rect.height) / 2;
                    break;
                case XY_GUI_LAYOUT_ALIGN_END:
                    child->rect.y = r->y + r->height - child->rect.height;
                    break;
                case XY_GUI_LAYOUT_ALIGN_STRETCH:
                    child->rect.y = r->y;
                    child->rect.height = r->height;
                    break;
                default: /* START */
                    child->rect.y = r->y;
                    break;
            }
            
            x += child_width + config.main_spacing;
        }
        
        child = child->next;
    }
    
    return 0;
}

/* ==================== 网格布局 ==================== */

int xy_gui_layout_grid(xy_gui_widget_t *container, uint8_t columns, uint8_t rows)
{
    if (!container || !container->child || columns == 0 || rows == 0) {
        return -1;
    }
    
    xy_gui_rect_t *r = &container->rect;
    
    /* 计算单元格尺寸 */
    uint16_t cell_width = r->width / columns;
    uint16_t cell_height = r->height / rows;
    
    uint8_t col = 0, row = 0;
    
    /* 遍历子控件 */
    xy_gui_widget_t *child = container->child;
    while (child) {
        if (child->style.visible) {
            /* 设置位置和尺寸 */
            child->rect.x = r->x + col * cell_width;
            child->rect.y = r->y + row * cell_height;
            child->rect.width = cell_width;
            child->rect.height = cell_height;
            
            /* 移动到下一个单元格 */
            col++;
            if (col >= columns) {
                col = 0;
                row++;
            }
            
            if (row >= rows) {
                break;  /* 网格已满 */
            }
        }
        
        child = child->next;
    }
    
    return 0;
}

/* ==================== 流式布局 ==================== */

int xy_gui_layout_flow(xy_gui_widget_t *container, uint16_t wrap_width)
{
    if (!container || !container->child) {
        return -1;
    }
    
    xy_gui_rect_t *r = &container->rect;
    int16_t x = r->x;
    int16_t y = r->y;
    uint16_t line_height = 0;
    uint16_t actual_wrap_width = (wrap_width > 0) ? wrap_width : r->width;
    
    /* 遍历子控件 */
    xy_gui_widget_t *child = container->child;
    while (child) {
        if (child->style.visible) {
            /* 检查是否需要换行 */
            if (x + child->rect.width > r->x + actual_wrap_width && x > r->x) {
                /* 换行 */
                x = r->x;
                y += line_height + 5;  /* 行间距 */
                line_height = 0;
            }
            
            /* 设置位置 */
            child->rect.x = x;
            child->rect.y = y;
            
            /* 更新行高 */
            if (child->rect.height > line_height) {
                line_height = child->rect.height;
            }
            
            /* 移动到下一个位置 */
            x += child->rect.width + 5;  /* 元素间距 */
        }
        
        child = child->next;
    }
    
    return 0;
}

/* ==================== 堆叠布局 ==================== */

int xy_gui_layout_stack(xy_gui_widget_t *container)
{
    if (!container || !container->child) {
        return -1;
    }
    
    xy_gui_rect_t *r = &container->rect;
    
    /* 所有子控件都填充整个容器 */
    xy_gui_widget_t *child = container->child;
    while (child) {
        if (child->style.visible) {
            child->rect.x = r->x;
            child->rect.y = r->y;
            child->rect.width = r->width;
            child->rect.height = r->height;
        }
        
        child = child->next;
    }
    
    return 0;
}

/* ==================== 通用布局更新 ==================== */

int xy_gui_layout_update(xy_gui_widget_t *container)
{
    if (!container) {
        return -1;
    }
    
    /* 获取布局配置 */
    xy_gui_layout_config_t config;
    if (xy_gui_layout_get_config(container, &config) != 0) {
        return 0;  /* 无布局配置 */
    }
    
    /* 根据类型执行布局 */
    switch (config.type) {
        case XY_GUI_LAYOUT_VERTICAL:
            return xy_gui_layout_vertical(container);
        
        case XY_GUI_LAYOUT_HORIZONTAL:
            return xy_gui_layout_horizontal(container);
        
        case XY_GUI_LAYOUT_GRID:
            return xy_gui_layout_grid(container, config.grid_columns, config.grid_rows);
        
        case XY_GUI_LAYOUT_FLOW:
            return xy_gui_layout_flow(container, config.flow_wrap_width);
        
        case XY_GUI_LAYOUT_STACK:
            return xy_gui_layout_stack(container);
        
        default:
            return 0;  /* 无布局或未知布局 */
    }
}
