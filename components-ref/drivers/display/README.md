# 显示系统开发指南

**版本**: 1.0.0
**日期**: 2026-03-05

---

## 📋 目录

1. [快速开始](#快速开始)
2. [架构概述](#架构概述)
3. [GUI 核心](#gui-核心)
4. [LCD 屏幕驱动](#lcd-屏幕驱动)
5. [LED 显示驱动](#led-显示驱动)
6. [效果库](#效果库)
7. [常见问题](#常见问题)

---

## ✅ 驱动实现状态

| 组件               | 状态    | 实现文件                                         |
| ------------------ | ------- | ------------------------------------------------ |
| **SSD1306 OLED**   | ✅ 完整 | `oled/ssd1306/xy_oled_ssd1306.c`                 |
| **ST7789 LCD**     | ✅ 完整 | `lcd/xy_lcd_st7789.c`                            |
| **LCD 框架**       | ✅ 完整 | `lcd/xy_lcd.c`, `xy_lcd_spi.c`, `xy_lcd_i8080.c` |
| **WS2812 RGB LED** | ✅ 完整 | `led_drivers/serial_rgb/xy_ws2812.c`             |
| **RGB Matrix**     | ✅ 完整 | `led_drivers/serial_rgb/xy_rgb_matrix.c`         |
| **LCD 接口**       | ✅ 完整 | SPI, I8080                                       |
| **GUI 效果**       | ✅ 完整 | `gui/effects/`                                   |
| **GUI 字体**       | ✅ 完整 | `gui/fonts/`                                     |

## 🚀 快速开始

### 1. 选择显示设备

| 设备类型     | 推荐场景          | 驱动位置                                   |
| ------------ | ----------------- | ------------------------------------------ |
| **LCD 屏幕** | 图形界面/复杂显示 | `drivers/display/lcd_drivers/`             |
| **LED 点阵** | 文字显示/简单图形 | `drivers/display/led_drivers/matrix_scan/` |
| **RGB LED**  | 氛围灯/装饰灯     | `drivers/display/led_drivers/serial_rgb/`  |
| **查理复用** | 指示灯面板        | `drivers/display/led_drivers/charlieplex/` |

### 2. 初始化示例

#### LCD 屏幕 (SPI)

```c
#include "xy_gui.h"
#include "xy_lcd_spi.h"

// 配置
xy_lcd_spi_driver_t lcd_drv;
xy_lcd_spi_config_t cfg = {
    .spi_handle = hspi1,
    .dc_pin = GPIO_DC,
    .cs_pin = GPIO_CS,
    .rst_pin = GPIO_RST,
    .width = 240,
    .height = 240,
};

// 初始化
xy_lcd_spi_init(&lcd_drv, &cfg);

// 注册到 GUI
xy_display_register(xy_lcd_spi_get_device(&lcd_drv));

// 使用 GUI
xy_gui_clear(WHITE);
xy_gui_draw_string(10, 10, "Hello!", BLACK, &font_8x8);
xy_gui_flush();
```

#### LED 点阵 (MAX7219)

```c
#include "xy_gui.h"
#include "xy_matrix_scan.h"

// 配置
xy_matrix_scan_driver_t mtx_drv;
xy_matrix_scan_config_t cfg = {
    .type = MATRIX_TYPE_SPI,  // MAX7219
    .spi.num_devices = 4,
    .spi.spi_handle = hspi2,
};

// 初始化
xy_matrix_scan_init(&mtx_drv, &cfg);

// 注册到 GUI (可选)
xy_display_register(xy_matrix_scan_get_device(&mtx_drv));

// 使用 GUI
xy_gui_clear(0);
xy_gui_draw_string(0, 0, "Hi", 1, &font_5x7);
xy_gui_flush();
```

#### RGB LED 灯带

```c
#include "xy_serial_rgb.h"

// 配置
xy_serial_rgb_driver_t rgb_drv;
xy_serial_rgb_config_t cfg = {
    .num_leds = 60,
    .pin = GPIO5,
    .type = XY_SERIAL_RGB_WS2812B,
};

// 初始化 (独立模式)
xy_serial_rgb_init(&rgb_drv, &cfg);

// 直接使用效果
xy_serial_rgb_effect_rainbow(&rgb_drv, 128);
```

---

## 🏗️ 架构概述

### 分层设计

```
┌─────────────────────────────────┐
│       应用层 (Application)       │
│  可选择：直接使用驱动 OR GUI     │
└───────────────┬─────────────────┘
                │
      ┌─────────┴─────────┐
      │                   │
┌─────▼─────┐     ┌──────▼──────┐
│ 独立模式   │     │  GUI 模式    │
│ 直接控制  │     │  统一接口    │
└─────┬─────┘     └──────┬──────┘
      │                   │
      └─────────┬─────────┘
                │
      ┌─────────▼─────────┐
      │   显示设备抽象层   │
      │   xy_display.h    │
      └─────────┬─────────┘
                │
      ┌─────────┼─────────┬─────────┐
      │         │         │         │
┌─────▼───┐ ┌──▼────┐ ┌──▼────┐ ┌─▼────────┐
│ LCD 驱动 │ │LED 矩阵│ │RGB LED│ │ 电子纸   │
└─────────┘ └───────┘ └───────┘ └──────────┘
```

### 双模式设计

| 模式         | 特点                   | 适用场景          |
| ------------ | ---------------------- | ----------------- |
| **独立模式** | 直接控制硬件，内置效果 | 简单应用/指示灯   |
| **GUI 模式** | 统一绘图接口，帧缓冲   | 复杂显示/图形界面 |

---

## 📺 GUI 核心

### 绘图 API

```c
// 设置像素
void xy_gui_set_pixel(int16_t x, int16_t y, xy_gui_color_t color);

// 绘制直线
void xy_gui_draw_line(int16_t x0, int16_t y0,
                      int16_t x1, int16_t y1,
                      xy_gui_color_t color);

// 绘制矩形
void xy_gui_draw_rect(int16_t x, int16_t y,
                      int16_t w, int16_t h,
                      xy_gui_color_t color,
                      bool filled);

// 绘制圆形
void xy_gui_draw_circle(int16_t x0, int16_t y0,
                        int16_t radius,
                        xy_gui_color_t color,
                        bool filled);

// 绘制文字
void xy_gui_draw_string(int16_t x, int16_t y,
                        const char *str,
                        xy_gui_color_t color,
                        const xy_gui_font_t *font);
```

### 效果 API

```c
// 滚动文字
void xy_gui_scroll_text(const char *text,
                        xy_gui_scroll_dir_t dir,
                        uint8_t speed);

// 淡入淡出
void xy_gui_fade_in(uint8_t speed);
void xy_gui_fade_out(uint8_t speed);

// 波浪效果
void xy_gui_wave(uint8_t amplitude, uint8_t frequency);
```

---

## 🖥️ LCD 屏幕驱动

### 支持的接口

| 接口      | 驱动文件     | 特点             |
| --------- | ------------ | ---------------- |
| **SPI**   | `spi_lcd/`   | 4 线制，通用性强 |
| **QSPI**  | `qspi_lcd/`  | 高速，适合大尺寸 |
| **I8080** | `i8080_lcd/` | 并行，速度最快   |
| **RGB**   | `rgb_lcd/`   | 直接驱动，高性能 |

### SPI LCD 配置

```c
typedef struct {
    void *spi_handle;       // SPI 句柄
    uint8_t dc_pin;         // 数据/命令引脚
    uint8_t cs_pin;         // 片选引脚
    uint8_t rst_pin;        // 复位引脚
    uint8_t bl_pin;         // 背光引脚
    uint16_t width;         // 宽度
    uint16_t height;        // 高度
    bool use_dma;           // 使用 DMA
} xy_lcd_spi_config_t;
```

### 支持的 LCD 芯片

| 芯片        | 分辨率  | 色彩   | 驱动 |
| ----------- | ------- | ------ | ---- |
| **ST7735**  | 128x160 | RGB565 | ✅   |
| **ST7789**  | 240x240 | RGB565 | ✅   |
| **ILI9341** | 240x320 | RGB565 | ✅   |
| **ILI9488** | 320x480 | RGB565 | ✅   |
| **GC9A01**  | 240x240 | RGB565 | ✅   |

---

## 💡 LED 显示驱动

### 1. 查理复用 (Charlieplex)

**特点**: N 个 IO 控制 N×(N-1) 个 LED

| IO 数 | LED 数 |
| ----- | ------ |
| 3     | 6      |
| 4     | 12     |
| 5     | 20     |
| 6     | 30     |
| 8     | 56     |

**适用**: 单色指示灯面板

```c
// 初始化
xy_charlieplex_init(&drv, io_pins, 4);

// 设置亮度
xy_charlieplex_set_brightness(&drv, 0, 128);

// 呼吸灯效果
xy_charlieplex_effect_breath(&drv, 0, 1000);
```

### 2. 矩阵扫描 (Matrix Scan)

**支持类型**:

- GPIO 直接驱动
- SPI (MAX7219)
- I2C (HT16K33)

**适用**: 8x8/16x16 点阵屏

```c
// MAX7219 配置
xy_matrix_scan_config_t cfg = {
    .type = MATRIX_TYPE_SPI,
    .spi.num_devices = 4,  // 4 个 MAX7219 级联
};

xy_matrix_scan_init(&drv, &cfg);

// 显示文字
xy_matrix_scan_draw_string(&drv, "Hello");
```

### 3. 串行 RGB (Serial RGB)

**支持芯片**:

- WS2812B / WS2811
- SK6812 / SK6812-RGBW
- APA102 / SK9822

**驱动方式**:

- GPIO 位模拟 (通用)
- SPI DMA (高性能)
- I2S DMA (ESP32)

**适用**: LED 灯带/RGB 灯阵

```c
// WS2812B 配置
xy_serial_rgb_config_t cfg = {
    .num_leds = 60,
    .pin = GPIO5,
    .type = XY_SERIAL_RGB_WS2812B,
    .use_dma = true,
};

xy_serial_rgb_init(&drv, &cfg);

// 彩虹效果
xy_serial_rgb_effect_rainbow(&drv, 128);
```

---

## 🎨 效果库

### 基础效果 (10 种)

| 效果         | API                    | 说明     |
| ------------ | ---------------------- | -------- |
| **呼吸灯**   | `effect_breath()`      | 亮度渐变 |
| **闪烁**     | `effect_blink()`       | 开关闪烁 |
| **彩虹**     | `effect_rainbow()`     | 彩虹循环 |
| **滚动文字** | `effect_scroll_text()` | 文字滚动 |
| **流星**     | `effect_meteor()`      | 流星划过 |

### 屏幕效果 (30+ 种)

| 类别     | 效果           | 说明     |
| -------- | -------------- | -------- |
| **基础** | 滚动/淡入/缩放 | 基本动画 |
| **过渡** | 百叶窗/溶解    | 画面切换 |
| **变形** | 波浪/水波纹    | 动态效果 |
| **粒子** | 雨/雪/火       | 氛围营造 |
| **3D**   | 翻转/旋转      | 高级展示 |

---

## ❓ 常见问题

### Q1: 如何选择驱动模式？

**A**:

- 简单应用 (指示灯) → 独立模式
- 复杂显示 (图形界面) → GUI 模式

### Q2: LCD 屏幕不显示？

**A**: 检查:

1. SPI 配置是否正确
2. 复位序列是否执行
3. 背光是否开启
4. 色彩格式是否匹配

### Q3: WS2812 颜色不正确？

**A**:

- 检查色彩顺序 (GRB vs RGB)
- 检查数据时序
- 检查电源是否充足

### Q4: 如何添加新效果？

**A**:

1. 在 `gui/effects/` 创建效果文件
2. 实现效果函数
3. 在 `xy_gui_effects.h` 声明
4. 注册到效果引擎

---

## 📚 相关文档

- [DISPLAY_ARCHITECTURE.md](DISPLAY_ARCHITECTURE.md) - 架构设计
- [DISPLAY_DUAL_MODE.md](DISPLAY_DUAL_MODE.md) - 双模式设计
- [SOURCE_LAYOUT_PLAN.md](SOURCE_LAYOUT_PLAN.md) - 源码布局

---

**维护者**: XinYi Team
**许可证**: Apache License 2.0
