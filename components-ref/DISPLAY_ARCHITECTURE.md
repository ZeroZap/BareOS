# GUI 与显示驱动架构重构

**日期**: 2026-03-02  
**原则**: GUI 为核心，所有显示驱动为 GUI 服务

---

## ❌ 当前问题

1. **布局混乱** - LED 驱动在 drivers/led，GUI 在 gui，屏幕驱动分散
2. **职责不清** - LED 屏幕效果应该在 GUI 层
3. **缺少统一** - SPI/QSPI 屏幕驱动没有整合

---

## ✅ 最终架构

```
components/
├── gui/                           # GUI 核心
│   ├── inc/
│   │   ├── xy_gui.h              # GUI 统一接口
│   │   ├── xy_gui_display.h      # 显示设备接口 (抽象层)
│   │   ├── xy_gui_engine.h       # GUI 引擎
│   │   ├── xy_gui_font.h         # 字体系统
│   │   ├── xy_gui_widget.h       # 控件系统
│   │   ├── xy_gui_effects.h      # GUI 效果库
│   │   └── xy_gui_types.h        # 类型定义
│   ├── src/
│   │   ├── xy_gui.c              # GUI 核心实现
│   │   ├── xy_gui_engine.c       # GUI 引擎
│   │   ├── xy_gui_font.c         # 字体渲染
│   │   ├── xy_gui_widget.c       # 控件实现
│   │   ├── xy_gui_effects.c      # GUI 效果
│   │   └── xy_gui_primitives.c   # 绘图原语
│   └── fonts/                    # 字库
│       ├── xy_font_5x7.c
│       ├── xy_font_8x8.c
│       └── xy_font_16x16.c
│
└── drivers/
    └── display/                   # 显示驱动层 (所有显示设备)
        ├── inc/
        │   ├── xy_display.h      # 显示驱动统一接口
        │   ├── xy_display_lcd.h  # LCD 屏幕驱动
        │   ├── xy_display_led.h  # LED 显示驱动
        │   └── xy_display_epaper.h # 电子纸驱动
        │
        ├── lcd_drivers/           # LCD 屏幕驱动
        │   ├── spi_lcd/
        │   │   ├── xy_lcd_spi.h
        │   │   └── xy_lcd_spi.c
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
        ├── led_drivers/           # LED 显示驱动
        │   ├── charlieplex/
        │   │   ├── xy_charlieplex.h
        │   │   └── xy_charlieplex.c
        │   ├── matrix_scan/
        │   │   ├── xy_matrix_scan.h
        │   │   ├── xy_matrix_gpio.c
        │   │   ├── xy_matrix_spi.c  (MAX7219)
        │   │   └── xy_matrix_i2c.c  (HT16K33)
        │   └── serial_rgb/
        │       ├── xy_serial_rgb.h
        │       ├── xy_serial_rgb_bb.c  (位模拟)
        │       ├── xy_serial_rgb_spi.c (SPI DMA)
        │       └── xy_serial_rgb_i2s.c (I2S DMA)
        │
        └── epaper_drivers/        # 电子纸驱动
            ├── xy_epaper_spi.h
            └── xy_epaper_spi.c
```

---

## 🏗️ 架构层次

```
┌─────────────────────────────────────────┐
│          应用层 (Application)            │
│  main.c / app.c                         │
└─────────────────┬───────────────────────┘
                  │ 使用
┌─────────────────▼───────────────────────┐
│          GUI 核心层                      │
│  xy_gui.h / xy_gui_engine.h             │
│  ┌───────────────────────────────────┐  │
│  │ 绘图 API │ 控件 │ 动画 │ 字体     │  │
│  └───────────────────────────────────┘  │
└─────────────────┬───────────────────────┘
                  │ 使用显示接口
┌─────────────────▼───────────────────────┐
│       显示设备抽象层 (xy_display.h)      │
│  统一接口：set_pixel / flush / rotate   │
└─────────────────┬───────────────────────┘
                  │ 实现接口
        ┌─────────┼─────────┬─────────────┐
        │         │         │             │
┌───────▼───┐ ┌──▼──────┐ ┌▼────────┐ ┌──▼────────┐
│ LCD 屏幕  │ │ LED 显示 │ │电子纸   │ │ 其他显示  │
│ SPI/QSPI  │ │ 矩阵/RGB│ │         │ │           │
└───────────┘ └─────────┘ └─────────┘ └───────────┘
```

---

## 📦 详细设计

### 1. GUI 核心层 (xy_gui.h)

```c
#ifndef XY_GUI_H
#define XY_GUI_H

#include "xy_gui_display.h"
#include "xy_gui_types.h"
#include "xy_gui_font.h"

/* ==================== GUI 初始化 ==================== */

/**
 * @brief 初始化 GUI
 * @param display 显示设备接口
 * @return 0 成功
 */
int xy_gui_init(xy_gui_display_t *display);

/**
 * @brief 获取显示设备
 */
xy_gui_display_t* xy_gui_get_display(void);

/* ==================== 绘图原语 ==================== */

/**
 * @brief 设置像素
 */
void xy_gui_set_pixel(int16_t x, int16_t y, xy_gui_color_t color);

/**
 * @brief 获取像素
 */
xy_gui_color_t xy_gui_get_pixel(int16_t x, int16_t y);

/**
 * @brief 绘制直线
 */
void xy_gui_draw_line(int16_t x0, int16_t y0, 
                      int16_t x1, int16_t y1,
                      xy_gui_color_t color);

/**
 * @brief 绘制矩形
 */
void xy_gui_draw_rect(int16_t x, int16_t y,
                      int16_t w, int16_t h,
                      xy_gui_color_t color,
                      bool filled);

/**
 * @brief 绘制圆形
 */
void xy_gui_draw_circle(int16_t x0, int16_t y0,
                        int16_t radius,
                        xy_gui_color_t color,
                        bool filled);

/**
 * @brief 绘制三角形
 */
void xy_gui_draw_triangle(int16_t x0, int16_t y0,
                          int16_t x1, int16_t y1,
                          int16_t x2, int16_t y2,
                          xy_gui_color_t color,
                          bool filled);

/**
 * @brief 绘制椭圆
 */
void xy_gui_draw_ellipse(int16_t x0, int16_t y0,
                         int16_t rx, int16_t ry,
                         xy_gui_color_t color,
                         bool filled);

/* ==================== 文本渲染 ==================== */

/**
 * @brief 绘制字符
 */
void xy_gui_draw_char(int16_t x, int16_t y,
                      char c,
                      xy_gui_color_t color,
                      const xy_gui_font_t *font);

/**
 * @brief 绘制字符串
 */
void xy_gui_draw_string(int16_t x, int16_t y,
                        const char *str,
                        xy_gui_color_t color,
                        const xy_gui_font_t *font);

/**
 * @brief 绘制字符串 (自动换行)
 */
void xy_gui_draw_string_wrap(int16_t x, int16_t y,
                             int16_t max_width,
                             const char *str,
                             xy_gui_color_t color,
                             const xy_gui_font_t *font);

/**
 * @brief 测量字符串宽度
 */
int16_t xy_gui_measure_string(const char *str,
                              const xy_gui_font_t *font);

/* ==================== 高级绘图 ==================== */

/**
 * @brief 渐变填充
 */
void xy_gui_gradient_fill(int16_t x, int16_t y,
                          int16_t w, int16_t h,
                          xy_gui_color_t color_start,
                          xy_gui_color_t color_end,
                          bool vertical);

/**
 * @brief 绘制图像
 */
void xy_gui_draw_image(int16_t x, int16_t y,
                       int16_t w, int16_t h,
                       const uint8_t *data,
                       xy_gui_color_format_t format);

/**
 * @brief 缩放图像
 */
void xy_gui_draw_image_scaled(int16_t x, int16_t y,
                              int16_t src_w, int16_t src_h,
                              int16_t dst_w, int16_t dst_h,
                              const uint8_t *data);

/* ==================== 画布操作 ==================== */

/**
 * @brief 清空画布
 */
void xy_gui_clear(xy_gui_color_t color);

/**
 * @brief 填充画布
 */
void xy_gui_fill(xy_gui_color_t color);

/**
 * @brief 刷新显示
 */
void xy_gui_flush(void);

/**
 * @brief 刷新指定区域
 */
void xy_gui_flush_rect(int16_t x, int16_t y,
                       int16_t w, int16_t h);

/* ==================== 坐标变换 ==================== */

/**
 * @brief 设置旋转角度
 */
void xy_gui_set_rotation(uint8_t angle);  // 0/90/180/270

/**
 * @brief 设置镜像
 */
void xy_gui_set_mirror(bool horizontal, bool vertical);

/**
 * @brief 设置偏移
 */
void xy_gui_set_offset(int16_t x, int16_t y);

/* ==================== 双缓冲 ==================== */

/**
 * @brief 启用双缓冲
 */
void xy_gui_enable_double_buffer(bool enable);

/**
 * @brief 交换缓冲
 */
void xy_gui_swap_buffers(void);

#endif /* XY_GUI_H */
```

---

### 2. 显示设备抽象层 (xy_display.h)

```c
#ifndef XY_DISPLAY_H
#define XY_DISPLAY_H

#include <stdint.h>
#include <stdbool.h>

/* ==================== 显示类型 ==================== */

typedef enum {
    XY_DISPLAY_TYPE_LCD = 0,      // LCD 屏幕
    XY_DISPLAY_TYPE_LED_MATRIX,   // LED 点阵
    XY_DISPLAY_TYPE_LED_RGB,      // RGB LED
    XY_DISPLAY_TYPE_EPAPER,       // 电子纸
    XY_DISPLAY_TYPE_OLED,         // OLED
} xy_display_type_t;

/* ==================== 接口类型 ==================== */

typedef enum {
    XY_DISPLAY_IF_SPI = 0,
    XY_DISPLAY_IF_QSPI,
    XY_DISPLAY_IF_I8080,
    XY_DISPLAY_IF_RGB,
    XY_DISPLAY_IF_I2C,
    XY_DISPLAY_IF_CUSTOM,
} xy_display_if_t;

/* ==================== 色彩格式 ==================== */

typedef enum {
    XY_DISPLAY_COLOR_MONO = 0,    // 1bit
    XY_DISPLAY_COLOR_GRAY4,       // 4 级灰度
    XY_DISPLAY_COLOR_GRAY256,     // 256 级灰度
    XY_DISPLAY_COLOR_RGB565,      // 16 位
    XY_DISPLAY_COLOR_RGB888,      // 24 位
    XY_DISPLAY_COLOR_ARGB8888,    // 32 位
} xy_display_color_t;

/* ==================== 显示设备接口 ==================== */

typedef struct {
    /* 设备信息 */
    xy_display_type_t type;
    xy_display_if_t interface;
    xy_display_color_t color_format;
    uint16_t width;
    uint16_t height;
    uint16_t stride;
    
    /* 硬件句柄 */
    void *hw_handle;      // SPI/I2C/QSPI 句柄
    
    /* 驱动接口 (必须实现) */
    int  (*init)(void *hw);
    void (*set_pixel)(int16_t x, int16_t y, uint32_t color);
    uint32_t (*get_pixel)(int16_t x, int16_t y);
    void (*fill_rect)(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color);
    void (*flush)(void);
    void (*flush_rect)(int16_t x, int16_t y, int16_t w, int16_t h);
    void (*power_off)(void);
    
    /* 可选接口 */
    void (*set_backlight)(uint8_t brightness);
    void (*set_rotation)(uint8_t angle);
    void (*set_sleep)(bool sleep);
    
    /* 用户数据 */
    void *user_data;
} xy_display_dev_t;

/* ==================== GUI 适配层 ==================== */

/**
 * @brief 显示设备注册到 GUI
 */
int xy_display_register(xy_display_dev_t *dev);

/**
 * @brief 获取 GUI 显示接口
 */
xy_gui_display_t* xy_display_get_gui_interface(xy_display_dev_t *dev);

#endif /* XY_DISPLAY_H */
```

---

### 3. LCD 屏幕驱动示例 (spi_lcd)

```c
#ifndef XY_LCD_SPI_H
#define XY_LCD_SPI_H

#include "xy_display.h"

/* ==================== 配置 ==================== */

typedef struct {
    void *spi_handle;           // SPI 句柄
    uint8_t dc_pin;             // 数据/命令引脚
    uint8_t cs_pin;             // 片选引脚
    uint8_t rst_pin;            // 复位引脚
    uint8_t bl_pin;             // 背光引脚
    uint16_t width;
    uint16_t height;
    bool use_dma;               // 使用 DMA
} xy_lcd_spi_config_t;

/* ==================== 驱动结构 ==================== */

typedef struct {
    xy_display_dev_t display;   // 显示设备接口
    xy_lcd_spi_config_t config;
    uint16_t *frame_buffer;     // 帧缓冲 (RGB565)
    bool dirty;
} xy_lcd_spi_driver_t;

/* ==================== API ==================== */

/**
 * @brief 初始化 SPI LCD 驱动
 */
int xy_lcd_spi_init(xy_lcd_spi_driver_t *drv,
                    xy_lcd_spi_config_t *config);

/**
 * @brief 获取显示设备接口 (注册到 GUI)
 */
xy_display_dev_t* xy_lcd_spi_get_device(xy_lcd_spi_driver_t *drv);

/**
 * @brief 设置背光
 */
void xy_lcd_spi_set_backlight(xy_lcd_spi_driver_t *drv, uint8_t brightness);

/**
 * @brief 设置旋转
 */
void xy_lcd_spi_set_rotation(xy_lcd_spi_driver_t *drv, uint8_t angle);

#endif /* XY_LCD_SPI_H */
```

---

### 4. LED 矩阵驱动示例 (matrix_scan)

```c
#ifndef XY_MATRIX_SCAN_H
#define XY_MATRIX_SCAN_H

#include "xy_display.h"

/* ==================== 配置 ==================== */

typedef struct {
    uint8_t type;               // 0=GPIO, 1=SPI(MAX7219), 2=I2C(HT16K33)
    union {
        struct {
            uint8_t *row_pins;
            uint8_t *col_pins;
            uint8_t num_rows;
            uint8_t num_cols;
        } gpio;
        struct {
            void *spi_handle;
            uint8_t num_devices;
        } spi;
        struct {
            void *i2c_handle;
            uint8_t i2c_addr;
        } i2c;
    };
} xy_matrix_scan_config_t;

/* ==================== 驱动结构 ==================== */

typedef struct {
    xy_display_dev_t display;   // 显示设备接口
    xy_matrix_scan_config_t config;
    uint8_t *frame_buffer;      // 帧缓冲 (1bit 或 8bit)
    uint8_t current_row;        // 当前扫描行
    bool scanning;
} xy_matrix_scan_driver_t;

/* ==================== API ==================== */

/**
 * @brief 初始化矩阵扫描驱动
 */
int xy_matrix_scan_init(xy_matrix_scan_driver_t *drv,
                        xy_matrix_scan_config_t *config);

/**
 * @brief 获取显示设备接口
 */
xy_display_dev_t* xy_matrix_scan_get_device(xy_matrix_scan_driver_t *drv);

/**
 * @brief 扫描服务 (在中断或主循环调用)
 */
void xy_matrix_scan_service(xy_matrix_scan_driver_t *drv);

#endif /* XY_MATRIX_SCAN_H */
```

---

### 5. 串行 RGB 驱动示例 (serial_rgb)

```c
#ifndef XY_SERIAL_RGB_H
#define XY_SERIAL_RGB_H

#include "xy_display.h"

/* ==================== LED 类型 ==================== */

typedef enum {
    XY_SERIAL_RGB_WS2812B = 0,
    XY_SERIAL_RGB_WS2811,
    XY_SERIAL_RGB_SK6812,
    XY_SERIAL_RGB_SK6812_RGBW,
    XY_SERIAL_RGB_APA102,
} xy_serial_rgb_type_t;

/* ==================== 配置 ==================== */

typedef struct {
    uint16_t num_leds;          // LED 数量
    uint16_t width;             // 矩阵宽度 (用于 GUI 映射)
    uint16_t height;            // 矩阵高度
    uint8_t pin;                // 数据引脚
    uint8_t pin_clk;            // 时钟引脚 (APA102)
    xy_serial_rgb_type_t type;
    uint8_t color_order;        // GRB/RGB
    bool use_dma;
} xy_serial_rgb_config_t;

/* ==================== 驱动结构 ==================== */

typedef struct {
    xy_display_dev_t display;   // 显示设备接口
    xy_serial_rgb_config_t config;
    xy_rgb_color_t *led_buffer; // LED 颜色缓冲
    uint8_t *tx_buffer;         // 发送缓冲
    uint16_t buffer_size;
} xy_serial_rgb_driver_t;

/* ==================== API ==================== */

/**
 * @brief 初始化串行 RGB 驱动
 */
int xy_serial_rgb_init(xy_serial_rgb_driver_t *drv,
                       xy_serial_rgb_config_t *config);

/**
 * @brief 获取显示设备接口
 */
xy_display_dev_t* xy_serial_rgb_get_device(xy_serial_rgb_driver_t *drv);

/**
 * @brief 发送数据
 */
void xy_serial_rgb_show(xy_serial_rgb_driver_t *drv);

#endif /* XY_SERIAL_RGB_H */
```

---

## 📊 完整目录结构

```
components/
├── gui/
│   ├── inc/
│   │   ├── xy_gui.h              # GUI 统一接口 ⭐
│   │   ├── xy_gui_display.h      # 显示设备接口
│   │   ├── xy_gui_engine.h       # GUI 引擎
│   │   ├── xy_gui_font.h         # 字体系统
│   │   ├── xy_gui_widget.h       # 控件系统
│   │   ├── xy_gui_effects.h      # GUI 效果库
│   │   ├── xy_gui_types.h        # 类型定义
│   │   └── xy_gui_primitives.h   # 绘图原语
│   ├── src/
│   │   ├── xy_gui.c              # GUI 核心
│   │   ├── xy_gui_engine.c       # GUI 引擎
│   │   ├── xy_gui_font.c         # 字体渲染
│   │   ├── xy_gui_widget.c       # 控件实现
│   │   ├── xy_gui_effects.c      # GUI 效果
│   │   ├── xy_gui_primitives.c   # 绘图原语 (直线/圆形/矩形)
│   │   └── xy_gui_utils.c        # 工具函数
│   └── fonts/
│       ├── xy_font_5x7.c         # 5x7 字体
│       ├── xy_font_8x8.c         # 8x8 字体
│       ├── xy_font_16x16.c       # 16x16 字体
│       └── xy_font_chinese.c     # 中文字库 (可选)
│
└── drivers/
    └── display/
        ├── inc/
        │   ├── xy_display.h          # 显示驱动统一接口 ⭐
        │   ├── xy_display_lcd.h      # LCD 驱动接口
        │   ├── xy_display_led.h      # LED 驱动接口
        │   └── xy_display_epaper.h   # 电子纸驱动接口
        │
        ├── lcd_drivers/
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
        ├── led_drivers/
        │   ├── charlieplex/
        │   │   ├── xy_charlieplex.h
        │   │   ├── xy_charlieplex.c
        │   │   └── README.md
        │   ├── matrix_scan/
        │   │   ├── xy_matrix_scan.h
        │   │   ├── xy_matrix_scan.c
        │   │   ├── xy_matrix_gpio.c
        │   │   ├── xy_matrix_spi.c     (MAX7219)
        │   │   └── xy_matrix_i2c.c     (HT16K33)
        │   └── serial_rgb/
        │       ├── xy_serial_rgb.h
        │       ├── xy_serial_rgb.c
        │       ├── xy_serial_rgb_bb.c  (位模拟)
        │       ├── xy_serial_rgb_spi.c (SPI DMA)
        │       └── xy_serial_rgb_i2s.c (I2S DMA)
        │
        └── epaper_drivers/
            ├── xy_epaper.h
            ├── xy_epaper_spi.c
            └── fonts/                  # 电子纸专用字库
```

---

## 🔧 使用示例

### 示例 1: SPI LCD 屏幕

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
    .bl_pin = GPIO_BL,
    .width = 240,
    .height = 240,
    .use_dma = true,
};

// 1. 初始化 LCD 驱动
xy_lcd_spi_init(&lcd_drv, &cfg);

// 2. 注册到 GUI
xy_display_register(xy_lcd_spi_get_device(&lcd_drv));

// 3. 使用 GUI
xy_gui_clear(WHITE);
xy_gui_draw_string(10, 10, "Hello GUI!", BLACK, &font_8x8);
xy_gui_draw_circle(120, 120, 50, RED, false);
xy_gui_flush();
```

---

### 示例 2: MAX7219 LED 点阵

```c
#include "xy_gui.h"
#include "xy_matrix_scan.h"

// 矩阵驱动
xy_matrix_scan_driver_t matrix_drv;
xy_matrix_scan_config_t cfg = {
    .type = 1,  // SPI (MAX7219)
    .spi.num_devices = 4,
    .spi.spi_handle = hspi2,
};

// 1. 初始化矩阵驱动
xy_matrix_scan_init(&matrix_drv, &cfg);

// 2. 注册到 GUI
xy_display_register(xy_matrix_scan_get_device(&matrix_drv));

// 3. 使用 GUI
xy_gui_clear(0);
xy_gui_draw_string(0, 0, "Hi", 1, &font_5x7);
xy_gui_flush();

// 4. 扫描服务 (定时器中断中调用)
xy_matrix_scan_service(&matrix_drv);
```

---

### 示例 3: WS2812 RGB 灯阵

```c
#include "xy_gui.h"
#include "xy_serial_rgb.h"

// RGB 驱动
xy_serial_rgb_driver_t rgb_drv;
xy_serial_rgb_config_t cfg = {
    .num_leds = 256,
    .width = 16,
    .height = 16,
    .pin = GPIO5,
    .type = XY_SERIAL_RGB_WS2812B,
    .use_dma = true,
};

// 1. 初始化 RGB 驱动
xy_serial_rgb_init(&rgb_drv, &cfg);

// 2. 注册到 GUI
xy_display_register(xy_serial_rgb_get_device(&rgb_drv));

// 3. 使用 GUI
xy_gui_gradient_fill(0, 0, 16, 16, RED, BLUE, true);
xy_gui_flush();
```

---

## ✅ 优势

1. **架构清晰** - GUI 核心 + 显示驱动层
2. **职责明确** - GUI 绘图，驱动刷新
3. **易于扩展** - 新增显示设备实现接口即可
4. **统一接口** - 所有显示设备使用相同 GUI API
5. **布局合理** - GUI 在 gui/，驱动在 drivers/display/

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
