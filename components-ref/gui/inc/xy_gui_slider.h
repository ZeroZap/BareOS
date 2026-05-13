/**
 * @file xy_gui_slider.h
 * @brief GUI Slider Widget - 滑块控件
 * @version 1.0.0
 * @date 2026-03-13
 */

#ifndef XY_GUI_SLIDER_H
#define XY_GUI_SLIDER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_gui_widget.h"

/**
 * @brief 滑块方向
 */
typedef enum {
    XY_GUI_SLIDER_HORIZONTAL = 0,
    XY_GUI_SLIDER_VERTICAL,
} xy_gui_slider_dir_t;

/**
 * @brief 滑块控件结构
 */
typedef struct {
    xy_gui_widget_t base;           /**< 继承自基类 */
    
    /* 滑块属性 */
    xy_gui_slider_dir_t direction;  /**< 方向 */
    int32_t min_value;              /**< 最小值 */
    int32_t max_value;              /**< 最大值 */
    int32_t step;                   /**< 步长 */
    bool show_ticks;                /**< 显示刻度 */
    bool show_value;                /**< 显示数值 */
    bool continuous;                /**< 连续模式 (实时回调) */
    
    /* 滑块样式 */
    xy_gui_style_t track_style;     /**< 轨道样式 */
    xy_gui_style_t thumb_style;     /**< 滑块样式 */
    
    /* 滑块尺寸 */
    uint16_t track_width;           /**< 轨道宽度/高度 */
    uint16_t thumb_size;            /**< 滑块大小 */
    
    /* 回调 */
    xy_gui_event_cb_t on_value_changed; /**< 值改变回调 */
} xy_gui_slider_t;

/* ==================== 滑块 API ==================== */

/**
 * @brief 创建滑块
 * @param slider 滑块指针
 * @param x X 坐标
 * @param y Y 坐标
 * @param length 长度 (水平为宽度，垂直为高度)
 * @param min 最小值
 * @param max 最大值
 * @param direction 方向
 * @return 0 成功，其他失败
 */
int xy_gui_slider_create(xy_gui_slider_t *slider,
                         int16_t x, int16_t y,
                         uint16_t length,
                         int32_t min, int32_t max,
                         xy_gui_slider_dir_t direction);

/**
 * @brief 销毁滑块
 */
int xy_gui_slider_destroy(xy_gui_slider_t *slider);

/**
 * @brief 绘制滑块
 */
int xy_gui_slider_draw(xy_gui_slider_t *slider,
                       void *framebuffer,
                       uint16_t fb_width,
                       uint16_t fb_height);

/**
 * @brief 更新滑块状态
 */
int xy_gui_slider_update(xy_gui_slider_t *slider,
                         xy_gui_event_t *event);

/**
 * @brief 设置滑块值
 */
int xy_gui_slider_set_value(xy_gui_slider_t *slider,
                            int32_t value);

/**
 * @brief 获取滑块值
 */
int32_t xy_gui_slider_get_value(xy_gui_slider_t *slider);

/**
 * @brief 设置范围
 */
int xy_gui_slider_set_range(xy_gui_slider_t *slider,
                            int32_t min, int32_t max);

/**
 * @brief 设置步长
 */
int xy_gui_slider_set_step(xy_gui_slider_t *slider,
                           int32_t step);

/**
 * @brief 设置方向
 */
int xy_gui_slider_set_direction(xy_gui_slider_t *slider,
                                xy_gui_slider_dir_t direction);

/**
 * @brief 设置刻度显示
 */
int xy_gui_slider_show_ticks(xy_gui_slider_t *slider,
                             bool show);

/**
 * @brief 设置数值显示
 */
int xy_gui_slider_show_value(xy_gui_slider_t *slider,
                             bool show);

/**
 * @brief 设置连续模式
 */
int xy_gui_slider_set_continuous(xy_gui_slider_t *slider,
                                 bool enable);

/**
 * @brief 设置滑块样式
 */
int xy_gui_slider_set_style(xy_gui_slider_t *slider,
                            const xy_gui_style_t *track,
                            const xy_gui_style_t *thumb);

/**
 * @brief 设置值改变回调
 */
int xy_gui_slider_set_value_changed_cb(xy_gui_slider_t *slider,
                                       xy_gui_event_cb_t cb,
                                       void *user_data);

/**
 * @brief 获取滑块位置 (像素)
 */
int16_t xy_gui_slider_get_thumb_pos(xy_gui_slider_t *slider);

/**
 * @brief 根据像素位置计算值
 */
int32_t xy_gui_slider_pos_to_value(xy_gui_slider_t *slider,
                                   int16_t pos);

/**
 * @brief 根据值计算像素位置
 */
int16_t xy_gui_slider_value_to_pos(xy_gui_slider_t *slider,
                                   int32_t value);

#ifdef __cplusplus
}
#endif

#endif /* XY_GUI_SLIDER_H */
