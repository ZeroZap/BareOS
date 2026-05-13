/**
 * @file xy_gui.h
 * @brief Simple GUI Framework for Embedded Systems
 * @version 1.0.0
 * @date 2026-02-28
 */

#ifndef XY_GUI_H
#define XY_GUI_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Configuration ==================== */

#ifndef XY_GUI_MAX_WIDTH
#define XY_GUI_MAX_WIDTH    320
#endif

#ifndef XY_GUI_MAX_HEIGHT
#define XY_GUI_MAX_HEIGHT   240
#endif

#ifndef XY_GUI_MAX_OBJECTS
#define XY_GUI_MAX_OBJECTS  32
#endif

/* ==================== Error Codes ==================== */

#define XY_GUI_OK               0
#define XY_GUI_ERROR            (-1)
#define XY_GUI_INVALID_PARAM    (-2)
#define XY_GUI_NO_MEM           (-3)
#define XY_GUI_NOT_FOUND        (-4)

/* ==================== Color Macros (RGB565) ==================== */

#define XY_GUI_RGB565(r, g, b)  ((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | (((b) & 0xF8) >> 3))
#define XY_GUI_RGB888_TO_565(r, g, b)  ((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3))

/* Common colors */
#define XY_GUI_COLOR_BLACK      XY_GUI_RGB565(0, 0, 0)
#define XY_GUI_COLOR_WHITE      XY_GUI_RGB565(255, 255, 255)
#define XY_GUI_COLOR_RED        XY_GUI_RGB565(255, 0, 0)
#define XY_GUI_COLOR_GREEN      XY_GUI_RGB565(0, 255, 0)
#define XY_GUI_COLOR_BLUE       XY_GUI_RGB565(0, 0, 255)
#define XY_GUI_COLOR_YELLOW     XY_GUI_RGB565(255, 255, 0)
#define XY_GUI_COLOR_CYAN       XY_GUI_RGB565(0, 255, 255)
#define XY_GUI_COLOR_MAGENTA    XY_GUI_RGB565(255, 0, 255)
#define XY_GUI_COLOR_GRAY       XY_GUI_RGB565(128, 128, 128)
#define XY_GUI_COLOR_ORANGE     XY_GUI_RGB565(255, 165, 0)

/* ==================== Data Structures ==================== */

/**
 * @brief 颜色结构 (RGB565)
 */
typedef struct {
    uint16_t color;
} xy_gui_color_t;

/**
 * @brief 矩形区域
 */
typedef struct {
    int16_t x;
    int16_t y;
    int16_t width;
    int16_t height;
} xy_gui_rect_t;

/**
 * @brief 点坐标
 */
typedef struct {
    int16_t x;
    int16_t y;
} xy_gui_point_t;

/**
 * @brief 对象类型
 */
typedef enum {
    XY_GUI_OBJ_NONE = 0,
    XY_GUI_OBJ_LABEL,
    XY_GUI_OBJ_BUTTON,
    XY_GUI_OBJ_BAR,
    XY_GUI_OBJ_CHECKBOX,
    XY_GUI_OBJ_IMAGE,
} xy_gui_obj_type_t;

/**
 * @brief 对象结构
 */
typedef struct {
    xy_gui_obj_type_t type;
    xy_gui_rect_t rect;
    xy_gui_color_t fg_color;
    xy_gui_color_t bg_color;
    bool visible;
    bool enabled;
    const char *text;
    void *user_data;
} xy_gui_obj_t;

/**
 * @brief 显示器驱动接口
 */
typedef struct {
    int (*init)(void);
    int (*draw_pixel)(int16_t x, int16_t y, uint16_t color);
    int (*draw_line)(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
    int (*draw_rect)(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    int (*fill_rect)(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    int (*draw_char)(int16_t x, int16_t y, char c, uint16_t color);
    int (*flush)(void);
} xy_gui_disp_drv_t;

/**
 * @brief GUI 句柄
 */
typedef struct {
    uint16_t width;
    uint16_t height;
    xy_gui_obj_t objects[XY_GUI_MAX_OBJECTS];
    uint8_t obj_count;
    xy_gui_disp_drv_t *disp_drv;
    uint16_t bg_color;
    bool initialized;
} xy_gui_t;

/* ==================== GUI Operations ==================== */

/**
 * @brief 初始化 GUI
 * @param gui GUI 句柄
 * @param width 屏幕宽度
 * @param height 屏幕高度
 * @param drv 显示器驱动
 * @return XY_GUI_OK 成功
 */
int xy_gui_init(xy_gui_t *gui, uint16_t width, uint16_t height, xy_gui_disp_drv_t *drv);

/**
 * @brief 反初始化 GUI
 * @param gui GUI 句柄
 * @return XY_GUI_OK 成功
 */
int xy_gui_deinit(xy_gui_t *gui);

/**
 * @brief 清屏
 * @param gui GUI 句柄
 * @param color 背景色
 * @return XY_GUI_OK 成功
 */
int xy_gui_clear(xy_gui_t *gui, uint16_t color);

/**
 * @brief 刷新屏幕
 * @param gui GUI 句柄
 * @return XY_GUI_OK 成功
 */
int xy_gui_flush(xy_gui_t *gui);

/**
 * @brief 绘制像素
 * @param gui GUI 句柄
 * @param x X 坐标
 * @param y Y 坐标
 * @param color 颜色
 * @return XY_GUI_OK 成功
 */
int xy_gui_draw_pixel(xy_gui_t *gui, int16_t x, int16_t y, uint16_t color);

/**
 * @brief 绘制直线
 * @param gui GUI 句柄
 * @param x1 起点 X
 * @param y1 起点 Y
 * @param x2 终点 X
 * @param y2 终点 Y
 * @param color 颜色
 * @return XY_GUI_OK 成功
 */
int xy_gui_draw_line(xy_gui_t *gui, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);

/**
 * @brief 绘制矩形
 * @param gui GUI 句柄
 * @param x X 坐标
 * @param y Y 坐标
 * @param w 宽度
 * @param h 高度
 * @param color 颜色
 * @return XY_GUI_OK 成功
 */
int xy_gui_draw_rect(xy_gui_t *gui, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

/**
 * @brief 填充矩形
 * @param gui GUI 句柄
 * @param x X 坐标
 * @param y Y 坐标
 * @param w 宽度
 * @param h 高度
 * @param color 颜色
 * @return XY_GUI_OK 成功
 */
int xy_gui_fill_rect(xy_gui_t *gui, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

/**
 * @brief 绘制字符
 * @param gui GUI 句柄
 * @param x X 坐标
 * @param y Y 坐标
 * @param c 字符
 * @param color 颜色
 * @return XY_GUI_OK 成功
 */
int xy_gui_draw_char(xy_gui_t *gui, int16_t x, int16_t y, char c, uint16_t color);

/**
 * @brief 绘制字符串
 * @param gui GUI 句柄
 * @param x X 坐标
 * @param y Y 坐标
 * @param str 字符串
 * @param color 颜色
 * @return XY_GUI_OK 成功
 */
int xy_gui_draw_string(xy_gui_t *gui, int16_t x, int16_t y, const char *str, uint16_t color);

/* ==================== Object Operations ==================== */

/**
 * @brief 创建对象
 * @param gui GUI 句柄
 * @param type 对象类型
 * @param rect 矩形区域
 * @return 对象指针
 */
xy_gui_obj_t *xy_gui_obj_create(xy_gui_t *gui, xy_gui_obj_type_t type, const xy_gui_rect_t *rect);

/**
 * @brief 删除对象
 * @param gui GUI 句柄
 * @param obj 对象指针
 * @return XY_GUI_OK 成功
 */
int xy_gui_obj_delete(xy_gui_t *gui, xy_gui_obj_t *obj);

/**
 * @brief 设置对象文本
 * @param gui GUI 句柄
 * @param obj 对象指针
 * @param text 文本
 * @return XY_GUI_OK 成功
 */
int xy_gui_obj_set_text(xy_gui_t *gui, xy_gui_obj_t *obj, const char *text);

/**
 * @brief 设置对象颜色
 * @param gui GUI 句柄
 * @param obj 对象指针
 * @param fg 前景色
 * @param bg 背景色
 * @return XY_GUI_OK 成功
 */
int xy_gui_obj_set_color(xy_gui_t *gui, xy_gui_obj_t *obj, uint16_t fg, uint16_t bg);

/**
 * @brief 设置对象可见性
 * @param gui GUI 句柄
 * @param obj 对象指针
 * @param visible 可见
 * @return XY_GUI_OK 成功
 */
int xy_gui_obj_set_visible(xy_gui_t *gui, xy_gui_obj_t *obj, bool visible);

/**
 * @brief 重绘对象
 * @param gui GUI 句柄
 * @param obj 对象指针
 * @return XY_GUI_OK 成功
 */
int xy_gui_obj_redraw(xy_gui_t *gui, xy_gui_obj_t *obj);

/* ==================== Helper Macros ==================== */

/* RGB565 颜色宏 */
#define GUI_COLOR_RGB565(r, g, b) \
    ((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | (((b) & 0xF8) >> 3))

/* 预定义颜色 */
#define GUI_COLOR_BLACK     GUI_COLOR_RGB565(0, 0, 0)
#define GUI_COLOR_WHITE     GUI_COLOR_RGB565(255, 255, 255)
#define GUI_COLOR_RED       GUI_COLOR_RGB565(255, 0, 0)
#define GUI_COLOR_GREEN     GUI_COLOR_RGB565(0, 255, 0)
#define GUI_COLOR_BLUE      GUI_COLOR_RGB565(0, 0, 255)
#define GUI_COLOR_YELLOW    GUI_COLOR_RGB565(255, 255, 0)
#define GUI_COLOR_CYAN      GUI_COLOR_RGB565(0, 255, 255)
#define GUI_COLOR_MAGENTA   GUI_COLOR_RGB565(255, 0, 255)

#ifdef __cplusplus
}
#endif

#endif /* XY_GUI_H */
