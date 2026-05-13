/**
 * @file xy_gui_theme.c
 * @brief GUI Theme System Implementation - 主题系统实现
 * @version 1.0.0
 * @date 2026-03-14
 */

#include "xy_gui_theme.h"
#include <string.h>
#include <stdlib.h>

#define XY_GUI_MAX_THEMES  8

/* ==================== 全局状态 ==================== */

static xy_gui_theme_t s_themes[XY_GUI_MAX_THEMES];
static uint8_t s_theme_count = 0;
static xy_gui_theme_t *s_current_theme = NULL;
static bool s_theme_system_initialized = false;

/* ==================== 预定义主题 ==================== */

void xy_gui_theme_create_light(xy_gui_theme_t *theme)
{
    if (!theme) return;
    
    memset(theme, 0, sizeof(*theme));
    strcpy(theme->name, "Light");
    
    /* 主色调 - 蓝色系 */
    theme->palette.primary = (xy_gui_color_t){26, 115, 232, 255};
    theme->palette.primary_light = (xy_gui_color_t){82, 147, 240, 255};
    theme->palette.primary_dark = (xy_gui_color_t){0, 80, 200, 255};
    
    /* 辅助色 */
    theme->palette.secondary = (xy_gui_color_t){149, 77, 232, 255};
    theme->palette.accent = (xy_gui_color_t){0, 191, 165, 255};
    
    /* 背景色 */
    theme->palette.background = (xy_gui_color_t){255, 255, 255, 255};
    theme->palette.surface = (xy_gui_color_t){245, 247, 250, 255};
    theme->palette.surface_variant = (xy_gui_color_t){235, 238, 242, 255};
    
    /* 文字色 */
    theme->palette.text_primary = (xy_gui_color_t){30, 30, 30, 255};
    theme->palette.text_secondary = (xy_gui_color_t){90, 90, 90, 255};
    theme->palette.text_disabled = (xy_gui_color_t){150, 150, 150, 255};
    
    /* 边框色 */
    theme->palette.border = (xy_gui_color_t){200, 200, 200, 255};
    theme->palette.border_light = (xy_gui_color_t){225, 225, 225, 255};
    
    /* 状态色 */
    theme->palette.success = (xy_gui_color_t){34, 197, 94, 255};
    theme->palette.warning = (xy_gui_color_t){245, 158, 11, 255};
    theme->palette.error = (xy_gui_color_t){239, 68, 68, 255};
    theme->palette.info = (xy_gui_color_t){59, 130, 246, 255};
    
    /* 交互色 */
    theme->palette.hover = (xy_gui_color_t){240, 245, 255, 255};
    theme->palette.pressed = (xy_gui_color_t){225, 235, 255, 255};
    theme->palette.selected = (xy_gui_color_t){220, 235, 255, 255};
    
    /* 样式配置 */
    theme->style.corner_radius_small = 4;
    theme->style.corner_radius_medium = 8;
    theme->style.corner_radius_large = 12;
    theme->style.border_width = 1;
    theme->style.shadow_enabled = 1;
    theme->style.shadow_radius = 4;
    theme->style.spacing_small = 4;
    theme->style.spacing_medium = 8;
    theme->style.spacing_large = 16;
    theme->style.animation_duration = 150;
    theme->style.animation_enabled = 1;
}

void xy_gui_theme_create_dark(xy_gui_theme_t *theme)
{
    if (!theme) return;
    
    memset(theme, 0, sizeof(*theme));
    strcpy(theme->name, "Dark");
    
    /* 主色调 - 蓝色系 */
    theme->palette.primary = (xy_gui_color_t){82, 147, 240, 255};
    theme->palette.primary_light = (xy_gui_color_t){120, 170, 245, 255};
    theme->palette.primary_dark = (xy_gui_color_t){26, 115, 232, 255};
    
    /* 辅助色 */
    theme->palette.secondary = (xy_gui_color_t){170, 115, 240, 255};
    theme->palette.accent = (xy_gui_color_t){34, 211, 238, 255};
    
    /* 背景色 */
    theme->palette.background = (xy_gui_color_t){18, 18, 18, 255};
    theme->palette.surface = (xy_gui_color_t){30, 30, 30, 255};
    theme->palette.surface_variant = (xy_gui_color_t){40, 40, 40, 255};
    
    /* 文字色 */
    theme->palette.text_primary = (xy_gui_color_t){240, 240, 240, 255};
    theme->palette.text_secondary = (xy_gui_color_t){160, 160, 160, 255};
    theme->palette.text_disabled = (xy_gui_color_t){90, 90, 90, 255};
    
    /* 边框色 */
    theme->palette.border = (xy_gui_color_t){60, 60, 60, 255};
    theme->palette.border_light = (xy_gui_color_t){80, 80, 80, 255};
    
    /* 状态色 */
    theme->palette.success = (xy_gui_color_t){74, 222, 128, 255};
    theme->palette.warning = (xy_gui_color_t){251, 191, 36, 255};
    theme->palette.error = (xy_gui_color_t){248, 113, 113, 255};
    theme->palette.info = (xy_gui_color_t){96, 165, 250, 255};
    
    /* 交互色 */
    theme->palette.hover = (xy_gui_color_t){40, 40, 40, 255};
    theme->palette.pressed = (xy_gui_color_t){50, 50, 50, 255};
    theme->palette.selected = (xy_gui_color_t){35, 50, 70, 255};
    
    /* 样式配置 */
    theme->style.corner_radius_small = 4;
    theme->style.corner_radius_medium = 8;
    theme->style.corner_radius_large = 12;
    theme->style.border_width = 1;
    theme->style.shadow_enabled = 0;  /* 暗色模式通常不用阴影 */
    theme->style.shadow_radius = 0;
    theme->style.spacing_small = 4;
    theme->style.spacing_medium = 8;
    theme->style.spacing_large = 16;
    theme->style.animation_duration = 150;
    theme->style.animation_enabled = 1;
}

void xy_gui_theme_create_high_contrast(xy_gui_theme_t *theme)
{
    if (!theme) return;
    
    memset(theme, 0, sizeof(*theme));
    strcpy(theme->name, "High Contrast");
    
    /* 高对比度：纯黑白 */
    theme->palette.primary = (xy_gui_color_t){0, 0, 0, 255};
    theme->palette.primary_light = (xy_gui_color_t){50, 50, 50, 255};
    theme->palette.primary_dark = (xy_gui_color_t){0, 0, 0, 255};
    
    theme->palette.secondary = (xy_gui_color_t){0, 0, 255, 255};
    theme->palette.accent = (xy_gui_color_t){255, 0, 255, 255};
    
    theme->palette.background = (xy_gui_color_t){0, 0, 0, 255};
    theme->palette.surface = (xy_gui_color_t){20, 20, 20, 255};
    theme->palette.surface_variant = (xy_gui_color_t){40, 40, 40, 255};
    
    theme->palette.text_primary = (xy_gui_color_t){255, 255, 255, 255};
    theme->palette.text_secondary = (xy_gui_color_t){200, 200, 200, 255};
    theme->palette.text_disabled = (xy_gui_color_t){100, 100, 100, 255};
    
    theme->palette.border = (xy_gui_color_t){255, 255, 255, 255};
    theme->palette.border_light = (xy_gui_color_t){200, 200, 200, 255};
    
    theme->palette.success = (xy_gui_color_t){0, 255, 0, 255};
    theme->palette.warning = (xy_gui_color_t){255, 255, 0, 255};
    theme->palette.error = (xy_gui_color_t){255, 0, 0, 255};
    theme->palette.info = (xy_gui_color_t){0, 255, 255, 255};
    
    theme->palette.hover = (xy_gui_color_t){50, 50, 50, 255};
    theme->palette.pressed = (xy_gui_color_t){80, 80, 80, 255};
    theme->palette.selected = (xy_gui_color_t){100, 100, 100, 255};
    
    /* 样式：大边框，无圆角 */
    theme->style.corner_radius_small = 0;
    theme->style.corner_radius_medium = 0;
    theme->style.corner_radius_large = 0;
    theme->style.border_width = 2;
    theme->style.shadow_enabled = 0;
    theme->style.shadow_radius = 0;
    theme->style.spacing_small = 6;
    theme->style.spacing_medium = 12;
    theme->style.spacing_large = 20;
    theme->style.animation_duration = 0;  /* 禁用动画 */
    theme->style.animation_enabled = 0;
}

/* ==================== 主题系统 API ==================== */

int xy_gui_theme_system_init(void)
{
    if (s_theme_system_initialized) {
        return -1;
    }
    
    memset(s_themes, 0, sizeof(s_themes));
    s_theme_count = 0;
    s_current_theme = NULL;
    
    /* 注册预定义主题 */
    xy_gui_theme_t light_theme;
    xy_gui_theme_create_light(&light_theme);
    xy_gui_theme_register(&light_theme);
    
    xy_gui_theme_t dark_theme;
    xy_gui_theme_create_dark(&dark_theme);
    xy_gui_theme_register(&dark_theme);
    
    /* 默认应用亮色主题 */
    xy_gui_theme_apply("Light");
    
    s_theme_system_initialized = true;
    return 0;
}

void xy_gui_theme_system_deinit(void)
{
    /* 释放动态分配的资源 */
    for (uint8_t i = 0; i < s_theme_count; i++) {
        /* 清理 user_data 等 */
    }
    
    memset(s_themes, 0, sizeof(s_themes));
    s_theme_count = 0;
    s_current_theme = NULL;
    s_theme_system_initialized = false;
}

int xy_gui_theme_register(xy_gui_theme_t *theme)
{
    if (!theme || s_theme_count >= XY_GUI_MAX_THEMES) {
        return -1;
    }
    
    /* 检查是否已存在同名主题 */
    for (uint8_t i = 0; i < s_theme_count; i++) {
        if (strcmp(s_themes[i].name, theme->name) == 0) {
            /* 更新现有主题 */
            memcpy(&s_themes[i], theme, sizeof(*theme));
            return 0;
        }
    }
    
    /* 添加新主题 */
    memcpy(&s_themes[s_theme_count], theme, sizeof(*theme));
    s_theme_count++;
    
    return 0;
}

int xy_gui_theme_unregister(const char *theme_name)
{
    if (!theme_name) {
        return -1;
    }
    
    for (uint8_t i = 0; i < s_theme_count; i++) {
        if (strcmp(s_themes[i].name, theme_name) == 0) {
            /* 移动后续主题 */
            for (uint8_t j = i; j < s_theme_count - 1; j++) {
                memcpy(&s_themes[j], &s_themes[j + 1], sizeof(xy_gui_theme_t));
            }
            s_theme_count--;
            
            if (s_current_theme == &s_themes[i]) {
                s_current_theme = NULL;
            }
            
            return 0;
        }
    }
    
    return -1;  /* 未找到 */
}

xy_gui_theme_t* xy_gui_theme_find(const char *theme_name)
{
    if (!theme_name) {
        return NULL;
    }
    
    for (uint8_t i = 0; i < s_theme_count; i++) {
        if (strcmp(s_themes[i].name, theme_name) == 0) {
            return &s_themes[i];
        }
    }
    
    return NULL;
}

int xy_gui_theme_apply(const char *theme_name)
{
    xy_gui_theme_t *theme = xy_gui_theme_find(theme_name);
    if (!theme) {
        return -1;
    }
    
    s_current_theme = theme;
    theme->is_active = true;
    
    /* 禁用其他主题 */
    for (uint8_t i = 0; i < s_theme_count; i++) {
        if (&s_themes[i] != theme) {
            s_themes[i].is_active = false;
        }
    }
    
    return 0;
}

xy_gui_theme_t* xy_gui_theme_get_current(void)
{
    return s_current_theme;
}

int xy_gui_theme_get_list(xy_gui_theme_t **themes, int max_count)
{
    if (!themes || max_count <= 0) {
        return -1;
    }
    
    int count = (s_theme_count < max_count) ? s_theme_count : max_count;
    
    for (int i = 0; i < count; i++) {
        themes[i] = &s_themes[i];
    }
    
    return count;
}

/* ==================== 主题应用 ==================== */

int xy_gui_theme_apply_to_widget(xy_gui_widget_t *widget, xy_gui_theme_t *theme)
{
    if (!widget || !theme) {
        return -1;
    }
    
    /* 应用颜色 */
    widget->style.fg_color = theme->palette.text_primary;
    widget->style.border_color = theme->palette.border;
    widget->style.border_width = theme->style.border_width;
    widget->style.corner_radius = theme->style.corner_radius_medium;
    widget->style.padding = theme->style.spacing_medium;
    
    return 0;
}

int xy_gui_theme_apply_to_all(xy_gui_widget_t *root, xy_gui_theme_t *theme)
{
    if (!root || !theme) {
        return -1;
    }
    
    /* 应用到根控件 */
    xy_gui_theme_apply_to_widget(root, theme);
    
    /* 递归应用到子控件 */
    xy_gui_widget_t *child = root->child;
    while (child) {
        xy_gui_theme_apply_to_all(child, theme);
        child = child->next;
    }
    
    return 0;
}

/* ==================== 便捷 API ==================== */

int xy_gui_theme_set_light(void)
{
    return xy_gui_theme_apply("Light");
}

int xy_gui_theme_set_dark(void)
{
    return xy_gui_theme_apply("Dark");
}

bool xy_gui_theme_is_dark_mode_supported(void)
{
    return (xy_gui_theme_find("Dark") != NULL);
}
