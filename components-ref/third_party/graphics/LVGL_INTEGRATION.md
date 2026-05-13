# LVGL 图形库集成

**状态**: ⏳ 待集成  
**优先级**: 🟡 中

---

## 📦 库信息

- **名称**: LVGL (Light and Versatile Graphics Library)
- **版本**: 8.3.9
- **许可证**: MIT
- **官网**: https://lvgl.io/
- **GitHub**: https://github.com/lvgl/lvgl

---

## 🔧 集成步骤

### 1. 添加 Git Submodule

```bash
cd components/third_party/graphics
git submodule add https://github.com/lvgl/lvgl.git
cd ../../../
```

### 2. 配置 CMake

```cmake
# components/third_party/graphics/CMakeLists.txt

# LVGL 配置
set(LVGL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lvgl)

# LVGL 选项
set(LV_COLOR_DEPTH 16)
set(LV_DPI_DEF 130)
set(LV_MEM_SIZE (32 * 1024))

# 添加 LVGL
add_subdirectory(${LVGL_DIR} lvgl)

# LVGL 移植层
add_library(xy_lvgl_port STATIC
    port/xy_lvgl_disp.c
    port/xy_lvgl_indev.c
    port/xy_lvgl_tick.c
)

target_include_directories(xy_lvgl_port PUBLIC
    ${LVGL_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/port
)

target_link_libraries(xy_lvgl_port PUBLIC lvgl)
```

### 3. 创建显示驱动

```c
/* components/third_party/graphics/port/xy_lvgl_disp.c */

#include "lvgl.h"
#include "xy_lcd.h"

/* 显示缓冲区 */
#define DISP_BUF_SIZE (80 * 1024)
static lv_color_t *disp_buf1;
static lv_color_t *disp_buf2;

/* 显示刷新回调 */
static void xy_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    
    /* 调用 LCD 驱动 */
    xy_lcd_set_window(area->x1, area->y1, area->x2, area->y2);
    xy_lcd_write_data((uint16_t*)color_p, w * h);
    
    /* 通知 LVGL 刷新完成 */
    lv_disp_flush_ready(disp);
}

/* LVGL 显示初始化 */
void xy_lvgl_disp_init(void)
{
    /* 分配缓冲区 */
    disp_buf1 = malloc(DISP_BUF_SIZE * sizeof(lv_color_t));
    disp_buf2 = malloc(DISP_BUF_SIZE * sizeof(lv_color_t));
    
    /* 初始化显示驱动 */
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    
    disp_drv.hor_res = 320;
    disp_drv.ver_res = 240;
    disp_drv.flush_cb = xy_disp_flush;
    disp_drv.draw_buf = disp_buf1;
    disp_drv.draw_buf2 = disp_buf2;
    disp_drv.draw_buf_size = DISP_BUF_SIZE;
    
    lv_disp_drv_register(&disp_drv);
}
```

### 4. 创建输入驱动

```c
/* components/third_party/graphics/port/xy_lvgl_indev.c */

#include "lvgl.h"
#include "xy_touch.h"

/* 触摸输入回调 */
static void xy_touch_read_cb(lv_indev_drv_t *indev, lv_indev_data_t *data)
{
    static int16_t last_x = 0;
    static int16_t last_y = 0;
    
    /* 读取触摸坐标 */
    xy_touch_point_t point;
    if (xy_touch_read(&point)) {
        data->point.x = point.x;
        data->point.y = point.y;
        data->state = LV_INDEV_STATE_PR;
        
        last_x = point.x;
        last_y = point.y;
    } else {
        data->point.x = last_x;
        data->point.y = last_y;
        data->state = LV_INDEV_STATE_REL;
    }
}

/* LVGL 输入初始化 */
void xy_lvgl_indev_init(void)
{
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = xy_touch_read_cb;
    
    lv_indev_drv_register(&indev_drv);
}
```

### 5. 系统节拍

```c
/* components/third_party/graphics/port/xy_lvgl_tick.c */

#include "lvgl.h"
#include "xy_os.h"

/* LVGL 节拍定时器 (1ms) */
static void xy_lvgl_tick_timer(void *arg)
{
    (void)arg;
    lv_tick_inc(1);
}

/* LVGL 节拍初始化 */
void xy_lvgl_tick_init(void)
{
    /* 创建 1ms 定时器 */
    xy_os_timer_create("lvgl_tick", xy_lvgl_tick_timer, NULL, 1);
}
```

### 6. 使用示例

```c
#include "xy_lvgl.h"

/* 按钮点击回调 */
static void btn_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        xy_log_i("Button clicked!\n");
    }
}

int main(void)
{
    /* 初始化系统 */
    xy_os_kernel_init();
    
    /* 初始化 LVGL */
    lv_init();
    xy_lvgl_disp_init();
    xy_lvgl_indev_init();
    xy_lvgl_tick_init();
    
    /* 创建按钮 */
    lv_obj_t *btn = lv_btn_create(lv_scr_act());
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_ALL, NULL);
    
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, "Hello LVGL!");
    lv_obj_center(label);
    
    /* 启动 LVGL 任务 */
    xy_os_task_create("lvgl_task", xy_lvgl_task, NULL, 1024, 5);
    
    xy_os_kernel_start();
}
```

---

## 📊 集成检查清单

- [ ] 添加 Git Submodule
- [ ] 配置 CMake
- [ ] 创建显示驱动
- [ ] 创建输入驱动
- [ ] 实现系统节拍
- [ ] 测试 UI 显示
- [ ] 添加文档

---

## 🎯 集成时间表

| 阶段 | 任务 | 工时 |
|------|------|------|
| **阶段 1** | 添加 Submodule + 配置 | 2h |
| **阶段 2** | 显示驱动 | 3h |
| **阶段 3** | 输入驱动 | 2h |
| **阶段 4** | 系统节拍 | 1h |
| **阶段 5** | 测试 + 文档 | 2h |
| **总计** | - | **10h** |

---

## 📚 相关文档

- [LVGL 官方文档](https://docs.lvgl.io/)
- [LVGL API 参考](https://docs.lvgl.io/master/api.html)
- [LVGL 移植指南](https://docs.lvgl.io/master/porting.html)

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0 (XY) + MIT (LVGL)
