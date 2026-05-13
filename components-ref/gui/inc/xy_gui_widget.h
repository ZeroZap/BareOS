/**
 * @file xy_gui_widget.h
 * @brief GUI Widget Base Class - 控件基类
 * @version 1.0.0
 * @date 2026-03-13
 */

#ifndef XY_GUI_WIDGET_H
#define XY_GUI_WIDGET_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "xy_gui_event.h"

/* 前向声明 */
typedef struct xy_gui_widget xy_gui_widget_t;

/**
 * @brief 控件类型
 */
typedef enum {
    XY_GUI_WIDGET_NONE = 0,
    XY_GUI_WIDGET_BUTTON,       /**< 按钮 */
    XY_GUI_WIDGET_LABEL,        /**< 标签 */
    XY_GUI_WIDGET_SLIDER,       /**< 滑块 */
    XY_GUI_WIDGET_CHECKBOX,     /**< 复选框 */
    XY_GUI_WIDGET_RADIO,        /**< 单选框 */
    XY_GUI_WIDGET_PROGRESS,     /**< 进度条 */
    XY_GUI_WIDGET_LIST,         /**< 列表 */
    XY_GUI_WIDGET_DROPDOWN,     /**< 下拉菜单 */
    XY_GUI_WIDGET_TAB,          /**< 标签页 */
    XY_GUI_WIDGET_TEXTBOX,      /**< 文本框 */
    XY_GUI_WIDGET_CONTAINER,    /**< 容器 */
} xy_gui_widget_type_t;

/**
 * @brief 控件状态标志
 */
typedef enum {
    XY_GUI_STATE_NONE     = 0x00,
    XY_GUI_STATE_NORMAL   = 0x01,   /**< 正常状态 */
    XY_GUI_STATE_PRESSED  = 0x02,   /**< 按下状态 */
    XY_GUI_STATE_HOVER    = 0x04,   /**< 悬停状态 */
    XY_GUI_STATE_DISABLED = 0x08,   /**< 禁用状态 */
    XY_GUI_STATE_FOCUSED  = 0x10,   /**< 聚焦状态 */
    XY_GUI_STATE_CHECKED  = 0x20,   /**< 选中状态 */
    XY_GUI_STATE_HIDDEN   = 0x40,   /**< 隐藏状态 */
} xy_gui_state_t;

/**
 * @brief 对齐方式
 */
typedef enum {
    XY_GUI_ALIGN_CENTER = 0,
    XY_GUI_ALIGN_LEFT,
    XY_GUI_ALIGN_RIGHT,
    XY_GUI_ALIGN_TOP,
    XY_GUI_ALIGN_BOTTOM,
    XY_GUI_ALIGN_TOP_LEFT,
    XY_GUI_ALIGN_TOP_RIGHT,
    XY_GUI_ALIGN_BOTTOM_LEFT,
    XY_GUI_ALIGN_BOTTOM_RIGHT,
} xy_gui_align_t;

/**
 * @brief 颜色结构
 */
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} xy_gui_color_t;

/**
 * @brief 预定义颜色
 */
#define XY_GUI_COLOR_BLACK      {0, 0, 0, 255}
#define XY_GUI_COLOR_WHITE      {255, 255, 255, 255}
#define XY_GUI_COLOR_RED        {255, 0, 0, 255}
#define XY_GUI_COLOR_GREEN      {0, 255, 0, 255}
#define XY_GUI_COLOR_BLUE       {0, 0, 255, 255}
#define XY_GUI_COLOR_YELLOW     {255, 255, 0, 255}
#define XY_GUI_COLOR_CYAN       {0, 255, 255, 255}
#define XY_GUI_COLOR_MAGENTA    {255, 0, 255, 255}
#define XY_GUI_COLOR_GRAY       {128, 128, 128, 255}
#define XY_GUI_COLOR_TRANSPARENT {0, 0, 0, 0}

/**
 * @brief 控件样式
 */
typedef struct {
    xy_gui_color_t bg_color;        /**< 背景色 */
    xy_gui_color_t fg_color;        /**< 前景色 (文字) */
    xy_gui_color_t border_color;    /**< 边框色 */
    uint8_t border_width;           /**< 边框宽度 */
    uint8_t corner_radius;          /**< 圆角半径 */
    uint8_t padding;                /**< 内边距 */
    xy_gui_align_t align;           /**< 对齐方式 */
    bool visible;                   /**< 可见性 */
    bool enabled;                   /**< 启用状态 */
} xy_gui_style_t;

/**
 * @brief 控件位置
 */
typedef struct {
    int16_t x;
    int16_t y;
    uint16_t width;
    uint16_t height;
} xy_gui_rect_t;

/**
 * @brief 事件回调类型
 */
typedef void (*xy_gui_event_cb_t)(xy_gui_widget_t *widget, xy_gui_event_t *event, void *user_data);

/* Event types and structures are defined in xy_gui_event.h */

/**
 * @brief 控件操作虚表
 */
typedef struct {
    int (*init)(xy_gui_widget_t *widget);
    int (*deinit)(xy_gui_widget_t *widget);
    int (*draw)(xy_gui_widget_t *widget, void *fb, uint16_t fb_w, uint16_t fb_h);
    int (*update)(xy_gui_widget_t *widget, xy_gui_event_t *event);
    int (*set_value)(xy_gui_widget_t *widget, int32_t value);
    int (*get_value)(xy_gui_widget_t *widget);
    int (*set_text)(xy_gui_widget_t *widget, const char *text);
    const char* (*get_text)(xy_gui_widget_t *widget);
} xy_gui_widget_ops_t;

/**
 * @brief GUI 控件基类
 */
struct xy_gui_widget {
    /* 基础属性 */
    xy_gui_widget_type_t type;      /**< 控件类型 */
    xy_gui_state_t state;           /**< 状态 */
    xy_gui_rect_t rect;             /**< 位置 */
    xy_gui_style_t style;           /**< 样式 */
    
    /* 文本 */
    char *text;                     /**< 文本内容 */
    size_t text_len;                /**< 文本长度 */
    
    /* 值 */
    int32_t value;                  /**< 当前值 */
    int32_t min_value;              /**< 最小值 */
    int32_t max_value;              /**< 最大值 */
    
    /* 事件 */
    xy_gui_event_cb_t on_event;     /**< 事件回调 */
    void *user_data;                /**< 用户数据 */
    
    /* 父子关系 */
    xy_gui_widget_t *parent;        /**< 父控件 */
    xy_gui_widget_t *next;          /**< 下一个兄弟控件 */
    xy_gui_widget_t *child;         /**< 第一个子控件 */
    
    /* 操作虚表 */
    const xy_gui_widget_ops_t *ops; /**< 操作函数表 */
    
    /* 标志 */
    bool need_redraw;               /**< 需要重绘 */
    bool is_dirty;                  /**< 已修改 */
};

/* ==================== 基类 API ==================== */

/**
 * @brief 初始化控件
 */
int xy_gui_widget_init(xy_gui_widget_t *widget, 
                       xy_gui_widget_type_t type,
                       int16_t x, int16_t y,
                       uint16_t width, uint16_t height);

/**
 * @brief 反初始化控件
 */
int xy_gui_widget_deinit(xy_gui_widget_t *widget);

/**
 * @brief 绘制控件
 */
int xy_gui_widget_draw(xy_gui_widget_t *widget, 
                       void *framebuffer,
                       uint16_t fb_width, 
                       uint16_t fb_height);

/**
 * @brief 更新控件状态
 */
int xy_gui_widget_update(xy_gui_widget_t *widget, 
                         xy_gui_event_t *event);

/**
 * @brief 设置控件位置
 */
int xy_gui_widget_set_pos(xy_gui_widget_t *widget, 
                          int16_t x, int16_t y);

/**
 * @brief 设置控件大小
 */
int xy_gui_widget_set_size(xy_gui_widget_t *widget, 
                           uint16_t width, uint16_t height);

/**
 * @brief 设置控件样式
 */
int xy_gui_widget_set_style(xy_gui_widget_t *widget, 
                            const xy_gui_style_t *style);

/**
 * @brief 设置文本
 */
int xy_gui_widget_set_text(xy_gui_widget_t *widget, 
                           const char *text);

/**
 * @brief 获取文本
 */
const char* xy_gui_widget_get_text(xy_gui_widget_t *widget);

/**
 * @brief 设置值
 */
int xy_gui_widget_set_value(xy_gui_widget_t *widget, 
                            int32_t value);

/**
 * @brief 获取值
 */
int32_t xy_gui_widget_get_value(xy_gui_widget_t *widget);

/**
 * @brief 设置事件回调
 */
int xy_gui_widget_set_event_cb(xy_gui_widget_t *widget, 
                               xy_gui_event_cb_t cb, 
                               void *user_data);

/**
 * @brief 检查点是否在控件内
 */
bool xy_gui_widget_hit_test(xy_gui_widget_t *widget, 
                            int16_t x, int16_t y);

/**
 * @brief 设置控件启用状态
 */
int xy_gui_widget_set_enabled(xy_gui_widget_t *widget, 
                              bool enabled);

/**
 * @brief 设置控件可见状态
 */
int xy_gui_widget_set_visible(xy_gui_widget_t *widget, 
                              bool visible);

#ifdef __cplusplus
}
#endif

#endif /* XY_GUI_WIDGET_H */
