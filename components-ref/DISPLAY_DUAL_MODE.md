# 显示系统架构 - 双模式设计

**日期**: 2026-03-02  
**原则**: 驱动独立运行 + 可选 GUI 集成

---

## 🎯 核心思想

**双模式设计**:
1. **独立模式** - LED 驱动独立运行，直接控制硬件
2. **GUI 模式** - LED 驱动提供帧缓冲接口给 GUI

**关键**: GUI 集成只是可选项，不是必须

---

## 🏗️ 架构设计

```
┌─────────────────────────────────────────┐
│          应用层 (Application)            │
│  可选择：直接使用驱动 OR 使用 GUI        │
└─────────────────┬───────────────────────┘
                  │
        ┌─────────┴─────────┐
        │                   │
┌───────▼───────┐   ┌──────▼───────┐
│   直接使用     │   │   GUI 核心    │
│   LED 驱动     │   │  xy_gui.h    │
│               │   └──────┬───────┘
│  - 查理复用   │          │ 帧缓冲接口
│  - 矩阵扫描   │   ┌──────▼───────┐
│  - 串行 RGB   │   │ 显示设备抽象 │
│               │   │ xy_display.h │
└───────────────┘   └──────┬───────┘
                           │
                  ┌────────▼────────┐
                  │   LED 驱动层     │
                  │  - charlieplex  │
                  │  - matrix_scan  │
                  │  - serial_rgb   │
                  └─────────────────┘
```

---

## 📦 详细设计

### 1. LED 驱动独立接口

```c
/**
 * @brief LED 驱动独立接口 (不依赖 GUI)
 */
typedef struct {
    /* 设备信息 */
    uint16_t width;
    uint16_t height;
    uint8_t bpp;              // 每像素位数
    
    /* 硬件接口 */
    void *hw_handle;
    
    /* 直接控制 API */
    void (*set_pixel)(uint16_t x, uint16_t y, uint32_t color);
    uint32_t (*get_pixel)(uint16_t x, uint16_t y);
    void (*fill)(uint32_t color);
    void (*show)(void);
    
    /* 效果 API (可选) */
    void (*effect_breath)(uint8_t led, uint16_t period);
    void (*effect_rainbow)(uint16_t speed);
    void (*effect_scroll_text)(const char *text, uint8_t speed);
    
    /* 用户数据 */
    void *user_data;
} xy_led_driver_t;
```

**独立使用示例**:
```c
// 初始化
xy_serial_rgb_init(&rgb_drv, 256, GPIO5, WS2812B);

// 直接使用驱动 API
xy_serial_rgb_set_pixel(&rgb_drv, 10, 10, RED);
xy_serial_rgb_show(&rgb_drv);

// 使用内置效果
xy_serial_rgb_effect_rainbow(&rgb_drv, 128);
```

---

### 2. GUI 帧缓冲接口 (可选)

```c
/**
 * @brief 获取 GUI 显示接口 (可选)
 * @return NULL 表示不支持 GUI
 */
xy_gui_display_t* xy_led_get_gui_interface(xy_led_driver_t *drv);
```

**GUI 模式使用**:
```c
// 1. 初始化驱动
xy_serial_rgb_init(&rgb_drv, 256, GPIO5, WS2812B);

// 2. 获取 GUI 接口 (可选)
xy_gui_display_t *display = xy_led_get_gui_interface(&rgb_drv);
if (display) {
    // 3. 初始化 GUI
    xy_gui_init(display);
    
    // 4. 使用 GUI
    xy_gui_draw_string(0, 0, "Hello", WHITE);
    xy_gui_flush();
}
```

---

### 3. 驱动双模式实现

```c
/**
 * @brief 串行 RGB 驱动结构
 */
typedef struct {
    /* 独立模式接口 */
    xy_led_driver_t led_drv;
    
    /* GUI 模式接口 (可选) */
    xy_gui_display_t gui_display;
    bool gui_enabled;
    
    /* 硬件参数 */
    xy_serial_rgb_config_t config;
    
    /* 帧缓冲 */
    xy_rgb_color_t *led_buffer;    // LED 颜色缓冲
    uint32_t *frame_buffer;        // GUI 帧缓冲 (可选)
    uint8_t *tx_buffer;            // 发送缓冲
    
    /* 效果状态 */
    struct {
        uint8_t current_effect;
        uint16_t speed;
        uint32_t frame;
    } effect;
} xy_serial_rgb_driver_t;

/**
 * @brief 初始化 (双模式)
 */
int xy_serial_rgb_init(xy_serial_rgb_driver_t *drv,
                       xy_serial_rgb_config_t *config)
{
    // 1. 初始化独立模式接口
    drv->led_drv.set_pixel = _serial_rgb_set_pixel;
    drv->led_drv.get_pixel = _serial_rgb_get_pixel;
    drv->led_drv.fill = _serial_rgb_fill;
    drv->led_drv.show = _serial_rgb_show;
    drv->led_drv.effect_breath = _serial_rgb_effect_breath;
    drv->led_drv.effect_rainbow = _serial_rgb_effect_rainbow;
    drv->led_drv.user_data = drv;
    
    // 2. 初始化 GUI 接口 (可选)
    drv->gui_display.width = config->width;
    drv->gui_display.height = config->height;
    drv->gui_display.set_pixel = _gui_set_pixel;
    drv->gui_display.flush = _gui_flush;
    drv->gui_enabled = (config->frame_buffer != NULL);
    
    // 3. 分配缓冲
    drv->led_buffer = malloc(config->num_leds * sizeof(xy_rgb_color_t));
    if (config->frame_buffer) {
        drv->frame_buffer = config->frame_buffer;
    }
    
    return 0;
}

/**
 * @brief 获取 GUI 接口 (运行时决定)
 */
xy_gui_display_t* xy_serial_rgb_get_gui(xy_serial_rgb_driver_t *drv)
{
    if (drv->gui_enabled) {
        return &drv->gui_display;
    }
    return NULL;  // 不支持 GUI
}
```

---

### 4. 使用场景

#### 场景 1: 独立使用 (最常见)

```c
// LED 氛围灯带
xy_serial_rgb_driver_t rgb_drv;
xy_serial_rgb_init(&rgb_drv, &cfg);

// 直接使用效果
xy_serial_rgb_effect_rainbow(&rgb_drv, 128);

// 无需 GUI
```

#### 场景 2: 查理复用指示灯

```c
xy_charlieplex_driver_t charlie_drv;
xy_charlieplex_init(&charlie_drv, io_pins, 4);

// 直接控制 LED
xy_charlieplex_set_brightness(&charlie_drv, 0, 128);
xy_charlieplex_effect_breath(&charlie_drv, 0, 1000);

// 不支持 GUI (单色，没必要)
```

#### 场景 3: LED 点阵显示

```c
xy_matrix_scan_driver_t mtx_drv;
xy_matrix_scan_init(&mtx_drv, &cfg);

// 方式 A: 直接使用
xy_matrix_scan_set_pixel(&mtx_drv, x, y, 1);
xy_matrix_scan_show(&mtx_drv);

// 方式 B: 使用 GUI (可选)
xy_gui_display_t *display = xy_matrix_scan_get_gui(&mtx_drv);
if (display) {
    xy_gui_init(display);
    xy_gui_draw_string(0, 0, "Hi", WHITE);
    xy_gui_flush();
}
```

#### 场景 4: RGB 灯阵显示图形

```c
xy_serial_rgb_driver_t rgb_drv;
xy_serial_rgb_config_t cfg = {
    .num_leds = 256,
    .width = 16,
    .height = 16,
    .frame_buffer = malloc(256 * 4),  // 分配帧缓冲
};

xy_serial_rgb_init(&rgb_drv, &cfg);

// 使用 GUI 绘图
xy_gui_display_t *display = xy_serial_rgb_get_gui(&rgb_drv);
xy_gui_init(display);
xy_gui_draw_circle(8, 8, 5, RED, true);
xy_gui_flush();
```

---

## 📊 驱动能力对比

| 驱动类型 | 独立模式 | GUI 模式 | 内置效果 |
|---------|---------|---------|---------|
| **charlieplex** | ✅ | ❌ | ✅ 呼吸/闪烁 |
| **matrix_scan** | ✅ | ✅ | ✅ 滚动文字 |
| **serial_rgb** | ✅ | ✅ | ✅ 彩虹/流星 |
| **spi_lcd** | ❌ | ✅ | N/A |
| **qspi_lcd** | ❌ | ✅ | N/A |

---

## 🎯 设计原则

### 1. 独立优先

```c
// 所有驱动首先保证独立使用
xy_led_driver_t led_drv;
led_drv.set_pixel = ...;
led_drv.show = ...;
led_drv.effect_xxx = ...;  // 内置效果
```

### 2. GUI 可选

```c
// GUI 接口是可选的
xy_gui_display_t* get_gui(void) {
    if (supports_gui) {
        return &gui_display;
    }
    return NULL;  // 不支持 GUI
}
```

### 3. 效果内置

```c
// 驱动自带效果，不依赖 GUI
void effect_rainbow(uint16_t speed);
void effect_breath(uint8_t led, uint16_t period);
void effect_scroll_text(const char *text);
```

### 4. 帧缓冲可选

```c
// 帧缓冲由应用决定是否分配
config.frame_buffer = malloc(...);  // 需要 GUI
config.frame_buffer = NULL;         // 不需要 GUI
```

---

## ✅ 优势

| 优势 | 说明 |
|------|------|
| **灵活性高** | 可选择独立或 GUI 模式 |
| **资源节省** | 不需要 GUI 时不分配帧缓冲 |
| **简单场景简单用** | 指示灯直接调用 API |
| **复杂场景强大** | 需要 GUI 时无缝集成 |
| **向后兼容** | 现有代码无需修改 |

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
