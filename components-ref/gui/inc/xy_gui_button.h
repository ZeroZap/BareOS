/**
 * @file xy_gui_button.h
 * @brief GUI Button Widget - 按钮控件
 * @version 1.0.0
 * @date 2026-03-13
 */

#ifndef XY_GUI_BUTTON_H
#define XY_GUI_BUTTON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_gui_widget.h"
#include "xy_font.h"

/**
 * @brief 按钮控件结构
 */
typedef struct {
    xy_gui_widget_t base;           /**< 继承自基类 */
    
    /* 按钮特定属性 */
    bool is_toggle;                 /**< 是否为切换按钮 */
    bool is_pressed;                /**< 是否被按下 */
    uint32_t press_time;            /**< 按下时间 */
    uint32_t long_press_time;       /**< 长按阈值 (ms) */
    
    /* 按钮样式 */
    xy_gui_style_t style_normal;    /**< 正常样式 */
    xy_gui_style_t style_pressed;   /**< 按下样式 */
    xy_gui_style_t style_disabled;  /**< 禁用样式 */
    
    /* 图标 */
    const uint8_t *icon_data;       /**< 图标数据 */
    uint16_t icon_width;            /**< 图标宽度 */
    uint16_t icon_height;           /**< 图标高度 */
    
    /* 回调 */
    xy_gui_event_cb_t on_click;     /**< 点击回调 */
    xy_gui_event_cb_t on_long_press; /**< 长按回调 */
} xy_gui_button_t;

/**
 * @brief 按钮创建标志
 */
typedef enum {
    XY_GUI_BUTTON_NORMAL = 0,       /**< 普通按钮 */
    XY_GUI_BUTTON_TOGGLE = 1,       /**< 切换按钮 */
    XY_GUI_BUTTON_RADIO = 2,        /**< 单选按钮 */
} xy_gui_button_flag_t;

/* ==================== 按钮 API ==================== */

/**
 * @brief 创建按钮
 * @param button 按钮指针
 * @param x X 坐标
 * @param y Y 坐标
 * @param width 宽度
 * @param height 高度
 * @param text 按钮文本
 * @param flags 创建标志
 * @return 0 成功，其他失败
 */
int xy_gui_button_create(xy_gui_button_t *button,
                         int16_t x, int16_t y,
                         uint16_t width, uint16_t height,
                         const char *text,
                         xy_gui_button_flag_t flags);

/**
 * @brief 销毁按钮
 */
int xy_gui_button_destroy(xy_gui_button_t *button);

/**
 * @brief 绘制按钮
 */
int xy_gui_button_draw(xy_gui_button_t *button,
                       void *framebuffer,
                       uint16_t fb_width,
                       uint16_t fb_height);

/**
 * @brief 更新按钮状态
 */
int xy_gui_button_update(xy_gui_button_t *button,
                         xy_gui_event_t *event);

/**
 * @brief 设置按钮文本
 */
int xy_gui_button_set_text(xy_gui_button_t *button,
                           const char *text);

/**
 * @brief 获取按钮文本
 */
const char* xy_gui_button_get_text(xy_gui_button_t *button);

/**
 * @brief 设置按钮图标
 */
int xy_gui_button_set_icon(xy_gui_button_t *button,
                           const uint8_t *icon_data,
                           uint16_t width,
                           uint16_t height);

/**
 * @brief 设置按钮样式
 */
int xy_gui_button_set_style(xy_gui_button_t *button,
                            const xy_gui_style_t *normal,
                            const xy_gui_style_t *pressed,
                            const xy_gui_style_t *disabled);

/**
 * @brief 设置长按时间阈值
 */
int xy_gui_button_set_long_press_time(xy_gui_button_t *button,
                                      uint32_t time_ms);

/**
 * @brief 设置点击回调
 */
int xy_gui_button_set_click_cb(xy_gui_button_t *button,
                               xy_gui_event_cb_t cb,
                               void *user_data);

/**
 * @brief 设置长按回调
 */
int xy_gui_button_set_long_press_cb(xy_gui_button_t *button,
                                    xy_gui_event_cb_t cb,
                                    void *user_data);

/**
 * @brief 设置按钮按下状态
 */
int xy_gui_button_set_pressed(xy_gui_button_t *button,
                              bool pressed);

/**
 * @brief 获取按钮按下状态
 */
bool xy_gui_button_is_pressed(xy_gui_button_t *button);

/**
 * @brief 设置按钮切换状态
 */
int xy_gui_button_set_checked(xy_gui_button_t *button,
                              bool checked);

/**
 * @brief 获取按钮切换状态
 */
bool xy_gui_button_is_checked(xy_gui_button_t *button);

/**
 * @brief 触发点击事件
 */
int xy_gui_button_trigger_click(xy_gui_button_t *button);

#ifdef __cplusplus
}
#endif

#endif /* XY_GUI_BUTTON_H */
