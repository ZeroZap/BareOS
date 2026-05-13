# GUI 核心库使用指南

**版本**: 1.0.0  
**日期**: 2026-03-05

---

## 📋 目录

1. [概述](#概述)
2. [快速开始](#快速开始)
3. [绘图 API](#绘图 api)
4. [文本渲染](#文本渲染)
5. [GUI 效果](#gui 效果)
6. [控件系统](#控件系统)
7. [示例](#示例)

---

## 📖 概述

XY_GUI 是一个轻量级嵌入式 GUI 库，提供统一的绘图接口，支持多种显示设备。

### 特性

- ✅ 统一绘图 API
- ✅ 支持多种显示设备
- ✅ 丰富的绘图原语
- ✅ 字体渲染
- ✅ 动画效果
- ✅ 双缓冲支持

### 架构

```
┌─────────────────┐
│   应用代码       │
├─────────────────┤
│   GUI API       │
│  (xy_gui.h)     │
├─────────────────┤
│  显示设备接口   │
│ (xy_display.h)  │
├─────────────────┤
│   硬件驱动       │
└─────────────────┘
```

---

## 🚀 快速开始

### 1. 初始化显示设备

```c
#include "xy_gui.h"
#include "xy_lcd_spi.h"

// LCD 驱动
xy_lcd_spi_driver_t lcd_drv;
xy_lcd_spi_config_t cfg = {
    .spi_handle = hspi1,
    .dc_pin = GPIO_DC,
    .cs_pin = GPIO_CS,
    .rst_pin = GPIO_RST,
    .width = 240,
    .height = 240,
};

// 初始化驱动
xy_lcd_spi_init(&lcd_drv, &cfg);

// 获取 GUI 显示接口
xy_gui_display_t *display = xy_lcd_spi_get_gui(&lcd_drv);

// 初始化 GUI
xy_gui_init(display);
```

### 2. 基本绘图

```c
// 清空屏幕
xy_gui_clear(WHITE);

// 绘制矩形
xy_gui_draw_rect(10, 10, 100, 50, BLUE, true);

// 绘制圆形
xy_gui_draw_circle(120, 120, 30, RED, false);

// 绘制文字
xy_gui_draw_string(20, 20, "Hello GUI!", BLACK, &font_8x8);

// 刷新显示
xy_gui_flush();
```

---

## 🎨 绘图 API

### 像素操作

```c
// 设置像素
xy_gui_set_pixel(10, 10, RED);

// 获取像素
xy_gui_color_t color = xy_gui_get_pixel(10, 10);
```

### 直线

```c
// 绘制直线
xy_gui_draw_line(0, 0, 100, 100, BLACK);

// 绘制虚线
xy_gui_draw_line_dashed(0, 0, 100, 100, BLACK, 5, 3);
```

### 矩形

```c
// 空心矩形
xy_gui_draw_rect(10, 10, 100, 50, BLUE, false);

// 实心矩形
xy_gui_draw_rect(10, 10, 100, 50, BLUE, true);

// 圆角矩形
xy_gui_draw_round_rect(10, 10, 100, 50, 10, BLUE, true);
```

### 圆形

```c
// 空心圆
xy_gui_draw_circle(120, 120, 30, RED, false);

// 实心圆
xy_gui_draw_circle(120, 120, 30, RED, true);

// 圆弧
xy_gui_draw_arc(120, 120, 30, 0, 90, RED);

// 扇形
xy_gui_draw_sector(120, 120, 30, 0, 90, RED, true);
```

### 椭圆

```c
// 绘制椭圆
xy_gui_draw_ellipse(120, 120, 50, 30, BLUE, true);
```

### 三角形

```c
// 绘制三角形
xy_gui_draw_triangle(10, 10, 100, 10, 50, 100, GREEN, true);
```

### 多边形

```c
// 绘制多边形
int16_t points[] = {50, 10, 90, 90, 10, 90};
xy_gui_draw_polygon(points, 3, YELLOW, true);
```

---

## 📝 文本渲染

### 字体

```c
// 可用字体
extern const xy_gui_font_t font_5x7;    // 5x7 字体
extern const xy_gui_font_t font_8x8;    // 8x8 字体
extern const xy_gui_font_t font_16x16;  // 16x16 字体
```

### 绘制文字

```c
// 绘制单个字符
xy_gui_draw_char(10, 10, 'A', BLACK, &font_8x8);

// 绘制字符串
xy_gui_draw_string(10, 10, "Hello", BLACK, &font_8x8);

// 绘制字符串 (自动换行)
xy_gui_draw_string_wrap(10, 10, 200, 
                        "This is a long text that will wrap.",
                        BLACK, &font_8x8);

// 绘制字符串 (居中)
xy_gui_draw_string_center(120, 120, "Center", BLACK, &font_8x8);
```

### 测量文字

```c
// 测量字符串宽度
int16_t width = xy_gui_measure_string("Hello", &font_8x8);

// 测量字符串高度
int16_t height = xy_gui_font_get_height(&font_8x8);
```

---

## 🎬 GUI 效果

### 滚动效果

```c
// 向左滚动
xy_gui_scroll_text("Hello World", SCROLL_LEFT, 2);

// 向上滚动
xy_gui_scroll_text("Hello World", SCROLL_UP, 2);
```

### 淡入淡出

```c
// 淡入
xy_gui_fade_in(5);

// 淡出
xy_gui_fade_out(5);
```

### 缩放效果

```c
// 放大 (从中心)
xy_gui_zoom(200, true);  // 200% 缩放

// 缩小
xy_gui_zoom(50, true);   // 50% 缩放
```

### 波浪效果

```c
// 波浪
xy_gui_wave(20, 5);  // 振幅 20, 频率 5
```

---

## 🎛️ 控件系统

### 按钮

```c
#include "xy_gui_widget.h"

// 创建按钮
xy_gui_button_t btn;
xy_gui_button_create(&btn, 50, 50, 100, 40, "Click", &font_8x8);

// 绘制按钮
xy_gui_button_draw(&btn);

// 检查点击
if (xy_gui_button_check_touch(&btn, touch_x, touch_y)) {
    // 按钮被点击
}
```

### 标签

```c
// 创建标签
xy_gui_label_t label;
xy_gui_label_create(&label, 10, 10, "Status: OK", &font_8x8);

// 更新文本
xy_gui_label_set_text(&label, "Status: Running");

// 绘制标签
xy_gui_label_draw(&label);
```

### 滑块

```c
// 创建滑块
xy_gui_slider_t slider;
xy_gui_slider_create(&slider, 50, 100, 200, 20, 0, 100, 50);

// 获取值
int value = xy_gui_slider_get_value(&slider);

// 设置值
xy_gui_slider_set_value(&slider, 75);

// 绘制滑块
xy_gui_slider_draw(&slider);
```

---

## 📊 示例

### 示例 1: 简单时钟

```c
#include "xy_gui.h"

void draw_clock(int hour, int minute, int second)
{
    // 清空屏幕
    xy_gui_clear(WHITE);
    
    // 绘制表盘
    xy_gui_draw_circle(120, 120, 100, BLACK, false);
    
    // 绘制刻度
    for (int i = 0; i < 12; i++) {
        float angle = i * 30 * 3.14159 / 180;
        int x1 = 120 + (int)(90 * cosf(angle));
        int y1 = 120 + (int)(90 * sinf(angle));
        int x2 = 120 + (int)(100 * cosf(angle));
        int y2 = 120 + (int)(100 * sinf(angle));
        xy_gui_draw_line(x1, y1, x2, y2, BLACK);
    }
    
    // 绘制时针
    float h_angle = (hour % 12 + minute / 60.0f) * 30 * 3.14159 / 180;
    int hx1 = 120 + (int)(40 * cosf(h_angle));
    int hy1 = 120 + (int)(40 * sinf(h_angle));
    xy_gui_draw_line(120, 120, hx1, hy1, BLACK);
    
    // 绘制分针
    float m_angle = (minute + second / 60.0f) * 6 * 3.14159 / 180;
    int mx1 = 120 + (int)(60 * cosf(m_angle));
    int my1 = 120 + (int)(60 * sinf(m_angle));
    xy_gui_draw_line(120, 120, mx1, my1, BLACK);
    
    // 绘制秒针
    float s_angle = second * 6 * 3.14159 / 180;
    int sx1 = 120 + (int)(70 * cosf(s_angle));
    int sy1 = 120 + (int)(70 * sinf(s_angle));
    xy_gui_draw_line(120, 120, sx1, sy1, RED);
    
    // 刷新
    xy_gui_flush();
}
```

### 示例 2: 进度条

```c
void draw_progress_bar(int x, int y, int width, int height, int percent)
{
    // 绘制边框
    xy_gui_draw_rect(x, y, width, height, BLACK, false);
    
    // 计算填充宽度
    int fill_width = (width - 4) * percent / 100;
    
    // 绘制填充
    xy_gui_draw_rect(x + 2, y + 2, fill_width, height - 4, GREEN, true);
    
    // 绘制百分比文字
    char text[8];
    sprintf(text, "%d%%", percent);
    xy_gui_draw_string_center(x + width / 2, y + height / 2 - 4, 
                              text, BLACK, &font_8x8);
    
    // 刷新
    xy_gui_flush();
}
```

### 示例 3: 简单菜单

```c
void draw_menu(void)
{
    const char *items[] = {"Home", "Settings", "About", "Exit"};
    int num_items = 4;
    int selected = 0;
    
    // 清空
    xy_gui_clear(WHITE);
    
    // 绘制标题
    xy_gui_draw_string(10, 10, "Main Menu", BLACK, &font_16x16);
    
    // 绘制菜单项
    for (int i = 0; i < num_items; i++) {
        xy_gui_color_t bg = (i == selected) ? BLUE : WHITE;
        xy_gui_color_t fg = (i == selected) ? WHITE : BLACK;
        
        xy_gui_draw_rect(10, 40 + i * 30, 200, 25, bg, true);
        xy_gui_draw_string(20, 45 + i * 30, items[i], fg, &font_8x8);
    }
    
    // 刷新
    xy_gui_flush();
}
```

---

## ❓ 常见问题

### Q1: 如何切换显示方向？

```c
// 设置旋转角度 (0/90/180/270)
xy_gui_set_rotation(90);
```

### Q2: 如何启用双缓冲？

```c
// 启用双缓冲
xy_gui_enable_double_buffer(true);

// 手动交换缓冲
xy_gui_swap_buffers();
```

### Q3: 如何自定义颜色？

```c
// 创建自定义颜色
xy_gui_color_t my_color = {255, 128, 0, 255};  // R,G,B,A

// 使用颜色
xy_gui_draw_rect(10, 10, 50, 50, my_color, true);
```

---

## 📚 相关文档

- [DISPLAY_ARCHITECTURE.md](../DISPLAY_ARCHITECTURE.md) - 架构设计
- [DISPLAY_DUAL_MODE.md](../DISPLAY_DUAL_MODE.md) - 双模式设计

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
