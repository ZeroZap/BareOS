# GUI 和 LED 源码布局整理方案

**日期**: 2026-03-02  
**目标**: 统一源码布局，消除分散

---

## ❌ 当前问题

文件分散在多处:
```
❌ components/gui/xy_gui.h              # GUI 主接口
❌ components/gui/inc/xy_gui_display.h  # GUI 显示接口
❌ components/drivers/led/              # LED 驱动 (旧)
❌ components/drivers/rgb/              # RGB 驱动 (旧)
❌ components/drivers/display/          # 显示驱动 (新)
```

---

## ✅ 最终布局

### 1. GUI 核心层

```
components/gui/
├── inc/
│   ├── xy_gui.h                  # GUI 统一接口 ⭐
│   ├── xy_gui_display.h          # 显示设备接口
│   ├── xy_gui_engine.h           # GUI 引擎
│   ├── xy_gui_font.h             # 字体系统
│   ├── xy_gui_widget.h           # 控件系统
│   ├── xy_gui_effects.h          # GUI 效果库
│   ├── xy_gui_types.h            # 类型定义
│   └── xy_gui_primitives.h       # 绘图原语
├── src/
│   ├── xy_gui.c                  # GUI 核心实现
│   ├── xy_gui_engine.c           # GUI 引擎
│   ├── xy_gui_font.c             # 字体渲染
│   ├── xy_gui_widget.c           # 控件实现
│   ├── xy_gui_effects.c          # GUI 效果
│   ├── xy_gui_primitives.c       # 绘图原语
│   └── xy_gui_utils.c            # 工具函数
├── fonts/
│   ├── xy_font_5x7.c             # 5x7 字体
│   ├── xy_font_8x8.c             # 8x8 字体
│   └── xy_font_16x16.c           # 16x16 字体
├── widgets/                      # 控件库
│   ├── xy_gui_button.c
│   ├── xy_gui_label.c
│   └── xy_gui_slider.c
└── effects/                      # GUI 效果
    ├── xy_gui_scroll.c
    ├── xy_gui_fade.c
    └── xy_gui_zoom.c
```

---

### 2. 显示驱动层

```
components/drivers/display/
├── inc/
│   ├── xy_display.h              # 显示驱动统一接口 ⭐
│   ├── xy_display_lcd.h          # LCD 驱动接口
│   ├── xy_display_led.h          # LED 驱动接口
│   └── xy_display_epaper.h       # 电子纸驱动接口
│
├── lcd_drivers/                  # LCD 屏幕驱动
│   ├── spi_lcd/
│   │   ├── xy_lcd_spi.h
│   │   ├── xy_lcd_spi.c
│   │   └── README.md
│   ├── qspi_lcd/
│   │   ├── xy_lcd_qspi.h
│   │   └── xy_lcd_qspi.c
│   ├── i8080_lcd/
│   │   ├── xy_lcd_i8080.h
│   │   └── xy_lcd_i8080.c
│   └── rgb_lcd/
│       ├── xy_lcd_rgb.h
│       └── xy_lcd_rgb.c
│
├── led_drivers/                  # LED 显示驱动
│   ├── charlieplex/
│   │   ├── xy_charlieplex.h
│   │   ├── xy_charlieplex.c
│   │   └── README.md
│   ├── matrix_scan/
│   │   ├── xy_matrix_scan.h
│   │   ├── xy_matrix_scan.c
│   │   ├── xy_matrix_gpio.c
│   │   ├── xy_matrix_spi.c       # MAX7219
│   │   └── xy_matrix_i2c.c       # HT16K33
│   └── serial_rgb/
│       ├── xy_serial_rgb.h
│       ├── xy_serial_rgb.c
│       ├── xy_serial_rgb_bb.c    # 位模拟
│       ├── xy_serial_rgb_spi.c   # SPI DMA
│       └── xy_serial_rgb_i2s.c   # I2S DMA
│
└── epaper_drivers/               # 电子纸驱动
    ├── xy_epaper.h
    ├── xy_epaper_spi.c
    └── fonts/
```

---

### 3. 删除旧目录

```
❌ 删除: components/drivers/led/
❌ 删除：components/drivers/rgb/
❌ 删除：components/drivers/mux/
```

---

## 🔄 迁移计划

### 阶段 1: 整理 GUI

```bash
# 移动 GUI 文件
mv components/gui/xy_gui.h components/gui/inc/
mv components/gui/inc/xy_gui_display.h components/gui/inc/

# 创建 GUI 子目录
mkdir -p components/gui/src
mkdir -p components/gui/fonts
mkdir -p components/gui/widgets
mkdir -p components/gui/effects
```

### 阶段 2: 整理显示驱动

```bash
# 创建统一显示驱动目录
mkdir -p components/drivers/display/lcd_drivers
mkdir -p components/drivers/display/led_drivers
mkdir -p components/drivers/display/epaper_drivers

# 移动 LCD 驱动
mv components/drivers/rgb/shapes/xy_rgb_matrix.h components/drivers/display/lcd_drivers/
mv components/drivers/rgb/inc/xy_rgb_drv.h components/drivers/display/

# 移动 LED 驱动
mv components/drivers/led/matrix_led/ components/drivers/display/led_drivers/
mv components/drivers/led/rgb_led/ components/drivers/display/led_drivers/
mv components/drivers/led/mono_led/ components/drivers/display/led_drivers/

# 移动 RGB 串行驱动
mv components/drivers/rgb/ shapes/* components/drivers/display/led_drivers/serial_rgb/
```

### 阶段 3: 删除旧目录

```bash
# 删除旧目录
rm -rf components/drivers/led/
rm -rf components/drivers/rgb/
rm -rf components/drivers/mux/
```

---

## 📊 完整布局

```
components/
├── gui/                          # GUI 核心 ⭐
│   ├── inc/
│   │   ├── xy_gui.h              # 统一接口
│   │   ├── xy_gui_display.h      # 显示接口
│   │   ├── xy_gui_engine.h       # 引擎
│   │   ├── xy_gui_font.h         # 字体
│   │   ├── xy_gui_widget.h       # 控件
│   │   ├── xy_gui_effects.h      # 效果
│   │   ├── xy_gui_types.h        # 类型
│   │   └── xy_gui_primitives.h   # 原语
│   ├── src/
│   │   ├── xy_gui.c
│   │   ├── xy_gui_engine.c
│   │   ├── xy_gui_font.c
│   │   ├── xy_gui_widget.c
│   │   ├── xy_gui_effects.c
│   │   ├── xy_gui_primitives.c
│   │   └── xy_gui_utils.c
│   ├── fonts/
│   ├── widgets/
│   └── effects/
│
└── drivers/
    ├── display/                  # 显示驱动 ⭐
    │   ├── inc/
    │   │   ├── xy_display.h      # 统一接口
    │   │   ├── xy_display_lcd.h
    │   │   ├── xy_display_led.h
    │   │   └── xy_display_epaper.h
    │   ├── lcd_drivers/
    │   │   ├── spi_lcd/
    │   │   ├── qspi_lcd/
    │   │   ├── i8080_lcd/
    │   │   └── rgb_lcd/
    │   ├── led_drivers/
    │   │   ├── charlieplex/
    │   │   ├── matrix_scan/
    │   │   └── serial_rgb/
    │   └── epaper_drivers/
    │
    ├── hal/                      # HAL 层
    ├── net/                      # 网络协议
    └── ...
```

---

## 📦 头文件包含关系

```c
// 应用层包含
#include "xy_gui.h"              // GUI 统一接口

// GUI 内部包含
#include "xy_gui_display.h"      // 显示设备接口
#include "xy_gui_font.h"         // 字体
#include "xy_gui_widget.h"       // 控件

// 显示驱动包含
#include "xy_display.h"          // 显示驱动接口
#include "xy_display_lcd.h"      // LCD 驱动
#include "xy_display_led.h"      // LED 驱动
```

---

## ✅ 优势

| 优势 | 说明 |
|------|------|
| **布局清晰** | GUI 在 gui/，驱动在 drivers/display/ |
| **职责明确** | GUI 绘图，驱动刷新 |
| **易于查找** | 相关文件在同一目录 |
| **易于维护** | 修改不影响其他模块 |
| **向后兼容** | 头文件路径不变 |

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
