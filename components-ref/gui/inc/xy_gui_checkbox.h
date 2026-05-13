/**
 * @file xy_gui_checkbox.h
 * @brief GUI Checkbox Widget - 复选框控件
 * @version 1.0.0
 * @date 2026-03-13
 */

#ifndef XY_GUI_CHECKBOX_H
#define XY_GUI_CHECKBOX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_gui_widget.h"

/**
 * @brief 复选框状态
 */
typedef enum {
    XY_GUI_CHECKBOX_UNCHECKED = 0,
    XY_GUI_CHECKBOX_CHECKED,
    XY_GUI_CHECKBOX_INDETERMINATE,  /**< 三态 (部分选中) */
} xy_gui_checkbox_state_t;

/**
 * @brief 复选框控件结构
 */
typedef struct {
    xy_gui_widget_t base;           /**< 继承自基类 */
    
    /* 复选框属性 */
    xy_gui_checkbox_state_t state;  /**< 状态 */
    bool tristate;                  /**< 支持三态 */
    bool radio;                     /**< 单选模式 */
    
    /* 复选框样式 */
    xy_gui_style_t box_style;       /**< 方框样式 */
    xy_gui_style_t check_style;     /**< 勾选标记样式 */
    
    /* 尺寸 */
    uint16_t box_size;              /**< 方框大小 */
    uint16_t spacing;               /**< 方框与文本间距 */
    
    /* 回调 */
    xy_gui_event_cb_t on_state_changed; /**< 状态改变回调 */
} xy_gui_checkbox_t;

/**
 * @brief 复选框组 (用于单选互斥)
 */
typedef struct {
    xy_gui_checkbox_t *head;        /**< 第一个复选框 */
    uint8_t count;                  /**< 数量 */
    uint8_t selected_index;         /**< 选中索引 */
} xy_gui_checkbox_group_t;

/* ==================== 复选框 API ==================== */

/**
 * @brief 创建复选框
 * @param checkbox 复选框指针
 * @param x X 坐标
 * @param y Y 坐标
 * @param text 文本
 * @param tristate 是否支持三态
 * @return 0 成功，其他失败
 */
int xy_gui_checkbox_create(xy_gui_checkbox_t *checkbox,
                           int16_t x, int16_t y,
                           const char *text,
                           bool tristate);

/**
 * @brief 创建单选框
 * @param checkbox 单选框指针
 * @param x X 坐标
 * @param y Y 坐标
 * @param text 文本
 * @return 0 成功，其他失败
 */
int xy_gui_radio_create(xy_gui_checkbox_t *checkbox,
                        int16_t x, int16_t y,
                        const char *text);

/**
 * @brief 销毁复选框
 */
int xy_gui_checkbox_destroy(xy_gui_checkbox_t *checkbox);

/**
 * @brief 绘制复选框
 */
int xy_gui_checkbox_draw(xy_gui_checkbox_t *checkbox,
                         void *framebuffer,
                         uint16_t fb_width,
                         uint16_t fb_height);

/**
 * @brief 更新复选框状态
 */
int xy_gui_checkbox_update(xy_gui_checkbox_t *checkbox,
                           xy_gui_event_t *event);

/**
 * @brief 设置选中状态
 */
int xy_gui_checkbox_set_checked(xy_gui_checkbox_t *checkbox,
                                bool checked);

/**
 * @brief 获取选中状态
 */
bool xy_gui_checkbox_is_checked(xy_gui_checkbox_t *checkbox);

/**
 * @brief 设置三态状态
 */
int xy_gui_checkbox_set_state(xy_gui_checkbox_t *checkbox,
                              xy_gui_checkbox_state_t state);

/**
 * @brief 获取状态
 */
xy_gui_checkbox_state_t xy_gui_checkbox_get_state(xy_gui_checkbox_t *checkbox);

/**
 * @brief 切换状态
 */
int xy_gui_checkbox_toggle(xy_gui_checkbox_t *checkbox);

/**
 * @brief 设置文本
 */
int xy_gui_checkbox_set_text(xy_gui_checkbox_t *checkbox,
                             const char *text);

/**
 * @brief 获取文本
 */
const char* xy_gui_checkbox_get_text(xy_gui_checkbox_t *checkbox);

/**
 * @brief 设置样式
 */
int xy_gui_checkbox_set_style(xy_gui_checkbox_t *checkbox,
                              const xy_gui_style_t *box,
                              const xy_gui_style_t *check);

/**
 * @brief 设置状态改变回调
 */
int xy_gui_checkbox_set_state_changed_cb(xy_gui_checkbox_t *checkbox,
                                         xy_gui_event_cb_t cb,
                                         void *user_data);

/* ==================== 复选框组 API ==================== */

/**
 * @brief 初始化复选框组
 */
int xy_gui_checkbox_group_init(xy_gui_checkbox_group_t *group);

/**
 * @brief 添加复选框到组
 */
int xy_gui_checkbox_group_add(xy_gui_checkbox_group_t *group,
                              xy_gui_checkbox_t *checkbox);

/**
 * @brief 获取选中的复选框
 */
xy_gui_checkbox_t* xy_gui_checkbox_group_get_selected(xy_gui_checkbox_group_t *group);

/**
 * @brief 设置选中的索引
 */
int xy_gui_checkbox_group_set_selected(xy_gui_checkbox_group_t *group,
                                       uint8_t index);

#ifdef __cplusplus
}
#endif

#endif /* XY_GUI_CHECKBOX_H */
