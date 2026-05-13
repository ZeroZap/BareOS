/**
 * @file xy_gui_theme.h
 * @brief GUI Theme System - 主题系统
 * @version 1.0.0
 * @date 2026-03-14
 * 
 * GUI 阶段 2: 主题系统核心头文件
 */

#ifndef XY_GUI_THEME_H
#define XY_GUI_THEME_H

#include <stdint.h>
#include <stdbool.h>
#include "xy_gui_widget.h"

#ifdef __cplusplus
extern "C" {
#endif

#define XY_GUI_THEME_NAME_MAX   32

/* ==================== 主题颜色 ==================== */

/**
 * @brief 主题调色板
 */
typedef struct {
    /* 主色调 */
    xy_gui_color_t primary;         /**< 主色 */
    xy_gui_color_t primary_light;   /**< 主色 - 浅色 */
    xy_gui_color_t primary_dark;    /**< 主色 - 深色 */
    
    /* 辅助色 */
    xy_gui_color_t secondary;       /**< 辅助色 */
    xy_gui_color_t accent;          /**< 强调色 */
    
    /* 背景色 */
    xy_gui_color_t background;      /**< 背景色 */
    xy_gui_color_t surface;         /**< 表面色 */
    xy_gui_color_t surface_variant; /**< 表面色 - 变体 */
    
    /* 文字色 */
    xy_gui_color_t text_primary;    /**< 主文字色 */
    xy_gui_color_t text_secondary;  /**< 次要文字色 */
    xy_gui_color_t text_disabled;   /**< 禁用文字色 */
    
    /* 边框色 */
    xy_gui_color_t border;          /**< 边框色 */
    xy_gui_color_t border_light;    /**< 边框色 - 浅色 */
    
    /* 状态色 */
    xy_gui_color_t success;         /**< 成功色 */
    xy_gui_color_t warning;         /**< 警告色 */
    xy_gui_color_t error;           /**< 错误色 */
    xy_gui_color_t info;            /**< 信息色 */
    
    /* 交互色 */
    xy_gui_color_t hover;           /**< 悬停色 */
    xy_gui_color_t pressed;         /**< 按下色 */
    xy_gui_color_t selected;        /**< 选中色 */
} xy_gui_theme_palette_t;

/* ==================== 主题样式 ==================== */

/**
 * @brief 主题样式配置
 */
typedef struct {
    /* 圆角 */
    uint8_t corner_radius_small;    /**< 小圆角 (2-4px) */
    uint8_t corner_radius_medium;   /**< 中圆角 (6-8px) */
    uint8_t corner_radius_large;    /**< 大圆角 (12-16px) */
    
    /* 边框 */
    uint8_t border_width;           /**< 边框宽度 */
    
    /* 阴影 (预留) */
    uint8_t shadow_enabled;         /**< 阴影启用 */
    uint8_t shadow_radius;          /**< 阴影半径 */
    
    /* 间距 */
    uint8_t spacing_small;          /**< 小间距 (4px) */
    uint8_t spacing_medium;         /**< 中间距 (8px) */
    uint8_t spacing_large;          /**< 大间距 (16px) */
    
    /* 字体 */
    const void *font_small;         /**< 小字体 */
    const void *font_medium;        /**< 中字体 */
    const void *font_large;         /**< 大字体 */
    
    /* 动画 */
    uint16_t animation_duration;    /**< 动画时长 (ms) */
    uint8_t animation_enabled;      /**< 动画启用 */
} xy_gui_theme_style_t;

/* ==================== 主题结构 ==================== */

/**
 * @brief 主题结构
 */
typedef struct {
    char name[XY_GUI_THEME_NAME_MAX];           /**< 主题名称 */
    xy_gui_theme_palette_t palette;             /**< 颜色调色板 */
    xy_gui_theme_style_t style;                 /**< 样式配置 */
    bool is_active;                             /**< 是否激活 */
} xy_gui_theme_t;

/* ==================== 预定义主题 ==================== */

/**
 * @brief 创建亮色主题
 * @param theme 主题指针
 */
void xy_gui_theme_create_light(xy_gui_theme_t *theme);

/**
 * @brief 创建暗色主题
 * @param theme 主题指针
 */
void xy_gui_theme_create_dark(xy_gui_theme_t *theme);

/**
 * @brief 创建高对比度主题
 * @param theme 主题指针
 */
void xy_gui_theme_create_high_contrast(xy_gui_theme_t *theme);

/* ==================== 主题系统 API ==================== */

/**
 * @brief 初始化主题系统
 * @return 0 成功，负值失败
 */
int xy_gui_theme_system_init(void);

/**
 * @brief 反初始化主题系统
 */
void xy_gui_theme_system_deinit(void);

/**
 * @brief 注册主题
 * @param theme 主题指针
 * @return 0 成功，负值失败
 */
int xy_gui_theme_register(xy_gui_theme_t *theme);

/**
 * @brief 注销主题
 * @param theme_name 主题名称
 * @return 0 成功，负值失败
 */
int xy_gui_theme_unregister(const char *theme_name);

/**
 * @brief 查找主题
 * @param theme_name 主题名称
 * @return 主题指针，NULL 表示未找到
 */
xy_gui_theme_t* xy_gui_theme_find(const char *theme_name);

/**
 * @brief 应用主题
 * @param theme_name 主题名称
 * @return 0 成功，负值失败
 */
int xy_gui_theme_apply(const char *theme_name);

/**
 * @brief 获取当前主题
 * @return 当前主题指针
 */
xy_gui_theme_t* xy_gui_theme_get_current(void);

/**
 * @brief 获取主题列表
 * @param themes 主题指针数组 (输出)
 * @param max_count 最大数量
 * @return 实际主题数量
 */
int xy_gui_theme_get_list(xy_gui_theme_t **themes, int max_count);

/**
 * @brief 应用主题到控件
 * @param widget 控件指针
 * @param theme 主题指针
 * @return 0 成功，负值失败
 */
int xy_gui_theme_apply_to_widget(xy_gui_widget_t *widget, xy_gui_theme_t *theme);

/**
 * @brief 应用主题到所有控件
 * @param root 根控件
 * @param theme 主题指针
 * @return 0 成功，负值失败
 */
int xy_gui_theme_apply_to_all(xy_gui_widget_t *root, xy_gui_theme_t *theme);

/* ==================== 便捷 API ==================== */

/**
 * @brief 切换到亮色主题
 * @return 0 成功，负值失败
 */
int xy_gui_theme_set_light(void);

/**
 * @brief 切换到暗色主题
 * @return 0 成功，负值失败
 */
int xy_gui_theme_set_dark(void);

/**
 * @brief 获取系统是否支持暗色模式
 * @return true 支持，false 不支持
 */
bool xy_gui_theme_is_dark_mode_supported(void);

#ifdef __cplusplus
}
#endif

#endif /* XY_GUI_THEME_H */
