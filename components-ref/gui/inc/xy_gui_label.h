/**
 * @file xy_gui_label.h
 * @brief GUI Label Widget - 标签控件
 * @version 1.0.0
 * @date 2026-03-13
 */

#ifndef XY_GUI_LABEL_H
#define XY_GUI_LABEL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_gui_widget.h"
#include "xy_font.h"

/**
 * @brief 标签控件结构
 */
typedef struct {
    xy_gui_widget_t base;           /**< 继承自基类 */
    
    /* 标签特定属性 */
    bool auto_size;                 /**< 自动调整大小 */
    bool word_wrap;                 /**< 自动换行 */
    bool multiline;                 /**< 多行文本 */
    uint8_t max_lines;              /**< 最大行数 (0=无限制) */
    
    /* 文本对齐 */
    xy_gui_align_t text_align;      /**< 文本对齐方式 */
    
    /* 字体 */
    const xy_font_t *font;          /**< 使用的字体 */
    
    /* 文本裁剪 */
    bool ellipsis;                  /**< 超出显示省略号 */
} xy_gui_label_t;

/* ==================== 标签 API ==================== */

/**
 * @brief 创建标签
 * @param label 标签指针
 * @param x X 坐标
 * @param y Y 坐标
 * @param width 宽度 (0=自动)
 * @param height 高度 (0=自动)
 * @param text 标签文本
 * @return 0 成功，其他失败
 */
int xy_gui_label_create(xy_gui_label_t *label,
                        int16_t x, int16_t y,
                        uint16_t width, uint16_t height,
                        const char *text);

/**
 * @brief 销毁标签
 */
int xy_gui_label_destroy(xy_gui_label_t *label);

/**
 * @brief 绘制标签
 */
int xy_gui_label_draw(xy_gui_label_t *label,
                      void *framebuffer,
                      uint16_t fb_width,
                      uint16_t fb_height);

/**
 * @brief 更新标签状态
 */
int xy_gui_label_update(xy_gui_label_t *label,
                        xy_gui_event_t *event);

/**
 * @brief 设置标签文本
 */
int xy_gui_label_set_text(xy_gui_label_t *label,
                          const char *text);

/**
 * @brief 获取标签文本
 */
const char* xy_gui_label_get_text(xy_gui_label_t *label);

/**
 * @brief 设置标签字体
 */
int xy_gui_label_set_font(xy_gui_label_t *label,
                          const xy_font_t *font);

/**
 * @brief 设置标签样式
 */
int xy_gui_label_set_style(xy_gui_label_t *label,
                           const xy_gui_style_t *style);

/**
 * @brief 设置自动换行
 */
int xy_gui_label_set_word_wrap(xy_gui_label_t *label,
                               bool enable);

/**
 * @brief 设置省略号
 */
int xy_gui_label_set_ellipsis(xy_gui_label_t *label,
                              bool enable);

/**
 * @brief 设置文本对齐
 */
int xy_gui_label_set_text_align(xy_gui_label_t *label,
                                xy_gui_align_t align);

/**
 * @brief 设置自动调整大小
 */
int xy_gui_label_set_auto_size(xy_gui_label_t *label,
                               bool enable);

/**
 * @brief 获取文本宽度
 */
uint16_t xy_gui_label_get_text_width(xy_gui_label_t *label);

/**
 * @brief 获取文本高度
 */
uint16_t xy_gui_label_get_text_height(xy_gui_label_t *label);

/**
 * @brief 调整标签大小以适应文本
 */
int xy_gui_label_resize_to_fit(xy_gui_label_t *label);

#ifdef __cplusplus
}
#endif

#endif /* XY_GUI_LABEL_H */
