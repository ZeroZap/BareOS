/**
 * @file xy_gui_layout.h
 * @brief GUI Layout System - 布局系统
 * @version 1.0.0
 * @date 2026-03-14
 * 
 * GUI 阶段 2: 布局系统核心头文件
 */

#ifndef XY_GUI_LAYOUT_H
#define XY_GUI_LAYOUT_H

#include <stdint.h>
#include <stdbool.h>
#include "xy_gui_widget.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 布局类型 ==================== */

typedef enum {
    XY_GUI_LAYOUT_NONE = 0,         /**< 无布局 (手动定位) */
    XY_GUI_LAYOUT_VERTICAL,         /**< 垂直布局 */
    XY_GUI_LAYOUT_HORIZONTAL,       /**< 水平布局 */
    XY_GUI_LAYOUT_GRID,             /**< 网格布局 */
    XY_GUI_LAYOUT_FLOW,             /**< 流式布局 */
    XY_GUI_LAYOUT_STACK,            /**< 堆叠布局 */
    XY_GUI_LAYOUT_CONSTRAINT,       /**< 约束布局 */
} xy_gui_layout_type_t;

/* ==================== 对齐方式 ==================== */

typedef enum {
    XY_GUI_LAYOUT_ALIGN_START = 0,  /**< 起始对齐 */
    XY_GUI_LAYOUT_ALIGN_CENTER,     /**< 居中对齐 */
    XY_GUI_LAYOUT_ALIGN_END,        /**< 结束对齐 */
    XY_GUI_LAYOUT_ALIGN_STRETCH,    /**< 拉伸填充 */
} xy_gui_layout_align_t;

/* ==================== 尺寸模式 ==================== */

typedef enum {
    XY_GUI_LAYOUT_SIZE_FIXED = 0,   /**< 固定尺寸 */
    XY_GUI_LAYOUT_SIZE_WRAP,        /**< 包裹内容 */
    XY_GUI_LAYOUT_SIZE_EXPAND,      /**< 弹性扩展 */
} xy_gui_layout_size_mode_t;

/* ==================== 布局配置 ==================== */

/**
 * @brief 布局配置结构
 */
typedef struct {
    xy_gui_layout_type_t type;      /**< 布局类型 */
    
    /* 主轴配置 */
    xy_gui_layout_align_t main_align;   /**< 主轴对齐 */
    uint16_t main_spacing;              /**< 主轴间距 */
    uint16_t main_padding;              /**< 主轴内边距 */
    
    /* 交叉轴配置 */
    xy_gui_layout_align_t cross_align;  /**< 交叉轴对齐 */
    uint16_t cross_spacing;             /**< 交叉轴间距 */
    
    /* 网格布局专用 */
    uint8_t grid_columns;               /**< 网格列数 */
    uint8_t grid_rows;                  /**< 网格行数 */
    uint16_t grid_column_gap;           /**< 列间距 */
    uint16_t grid_row_gap;              /**< 行间距 */
    
    /* 流式布局专用 */
    uint16_t flow_wrap_width;           /**< 流式换行宽度 */
    
    /* 标志 */
    bool reverse;                       /**< 反向排列 */
    bool wrap;                          /**< 自动换行 */
} xy_gui_layout_config_t;

/**
 * @brief 布局项配置 (子控件特定配置)
 */
typedef struct {
    xy_gui_layout_size_mode_t size_mode;    /**< 尺寸模式 */
    uint16_t fixed_size;                    /**< 固定尺寸值 */
    uint16_t min_size;                      /**< 最小尺寸 */
    uint16_t max_size;                      /**< 最大尺寸 */
    float flex_grow;                        /**< 弹性增长因子 */
    float flex_shrink;                      /**< 弹性收缩因子 */
    uint8_t grid_column;                    /**< 网格列位置 (1-based) */
    uint8_t grid_row;                       /**< 网格行位置 (1-based) */
    uint8_t grid_column_span;               /**< 网格列跨度 */
    uint8_t grid_row_span;                  /**< 网格行跨度 */
} xy_gui_layout_item_config_t;

/* ==================== 布局系统 API ==================== */

/**
 * @brief 初始化布局系统
 * @return 0 成功，负值失败
 */
int xy_gui_layout_system_init(void);

/**
 * @brief 设置容器布局配置
 * @param container 容器控件
 * @param config 布局配置
 * @return 0 成功，负值失败
 */
int xy_gui_layout_set_config(xy_gui_widget_t *container, 
                             const xy_gui_layout_config_t *config);

/**
 * @brief 获取布局配置
 * @param container 容器控件
 * @param config 布局配置 (输出)
 * @return 0 成功，负值失败
 */
int xy_gui_layout_get_config(xy_gui_widget_t *container,
                             xy_gui_layout_config_t *config);

/**
 * @brief 设置子控件布局配置
 * @param child 子控件
 * @param config 布局项配置
 * @return 0 成功，负值失败
 */
int xy_gui_layout_set_item_config(xy_gui_widget_t *child,
                                  const xy_gui_layout_item_config_t *config);

/**
 * @brief 执行布局计算
 * @param container 容器控件
 * @return 0 成功，负值失败
 */
int xy_gui_layout_update(xy_gui_widget_t *container);

/**
 * @brief 垂直布局
 * @param container 容器控件
 * @return 0 成功，负值失败
 */
int xy_gui_layout_vertical(xy_gui_widget_t *container);

/**
 * @brief 水平布局
 * @param container 容器控件
 * @return 0 成功，负值失败
 */
int xy_gui_layout_horizontal(xy_gui_widget_t *container);

/**
 * @brief 网格布局
 * @param container 容器控件
 * @param columns 列数
 * @param rows 行数
 * @return 0 成功，负值失败
 */
int xy_gui_layout_grid(xy_gui_widget_t *container, uint8_t columns, uint8_t rows);

/**
 * @brief 流式布局
 * @param container 容器控件
 * @param wrap_width 换行宽度
 * @return 0 成功，负值失败
 */
int xy_gui_layout_flow(xy_gui_widget_t *container, uint16_t wrap_width);

/**
 * @brief 堆叠布局
 * @param container 容器控件
 * @return 0 成功，负值失败
 */
int xy_gui_layout_stack(xy_gui_widget_t *container);

#ifdef __cplusplus
}
#endif

#endif /* XY_GUI_LAYOUT_H */
