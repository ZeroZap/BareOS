# TinyUSB 协议栈集成

**状态**: ⏳ 待集成  
**优先级**: 🔴 高

---

## 📦 库信息

- **名称**: TinyUSB
- **版本**: 0.16.0
- **许可证**: MIT
- **官网**: https://www.tinyusb.org/
- **GitHub**: https://github.com/hathach/tinyusb

---

## 🔧 集成步骤

### 1. 添加 Git Submodule

```bash
cd components/third_party/usb
git submodule add https://github.com/hathach/tinyusb.git
cd ../../../
```

### 2. 配置 CMake

```cmake
# components/third_party/usb/CMakeLists.txt

# TinyUSB 配置
set(TUSB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tinyusb)

# TinyUSB 选项
set(TUSB_DEBUG 0)
set(TUSB_OS OPT_OS_MYMCU)  # 自定义 OS 层

# 添加 TinyUSB
add_subdirectory(${TUSB_DIR} tinyusb)

# TinyUSB 移植层
add_library(xy_tinyusb_port STATIC
    port/xy_tusb_mcu.c
    port/xy_tusb_os.c
    port/xy_tusb_desc.c
)

target_include_directories(xy_tinyusb_port PUBLIC
    ${TUSB_DIR}/src
    ${TUSB_DIR}/hw
    ${CMAKE_CURRENT_SOURCE_DIR}/port
)

target_link_libraries(xy_tinyusb_port PUBLIC tinyusb)
```

### 3. 创建移植层

```c
/* components/third_party/usb/port/xy_tusb_mcu.c */

#include "tusb.h"

/* MCU 初始化 */
void tusb_mcu_init(void)
{
    /* 初始化 USB 时钟 */
    xy_usb_clock_init();
    
    /* 初始化 USB GPIO */
    xy_usb_gpio_init();
    
    /* 初始化 USB 控制器 */
    xy_usb_controller_init();
}

/* USB 中断处理 */
void tusb_mcu_isr(void)
{
    tud_int_handler();
}

/* 设备初始化 */
void tud_init(uint8_t rhport)
{
    tusb_mcu_init();
    tusb_dev_init(rhport);
}
```

### 4. USB 描述符

```c
/* components/third_party/usb/port/xy_tusb_desc.c */

#include "tusb.h"

/* 设备描述符 */
static uint8_t const desc_device[] = {
    18,                             /* bLength */
    TUSB_DESC_DEVICE,               /* bDescriptorType */
    U16_TO_U8S_LE(0x0200),          /* bcdUSB */
    0x00,                           /* bDeviceClass */
    0x00,                           /* bDeviceSubClass */
    0x00,                           /* bDeviceProtocol */
    64,                             /* bMaxPacketSize0 */
    U16_TO_U8S_LE(0xCafe),          /* idVendor */
    U16_TO_U8S_LE(0x4000),          /* idProduct */
    U16_TO_U8S_LE(0x0100),          /* bcdDevice */
    1,                              /* iManufacturer */
    2,                              /* iProduct */
    3,                              /* iSerialNumber */
    1                               /* bNumConfigurations */
};

/* CDC 描述符 */
static uint8_t const desc_configuration[] = {
    /* 配置头 */
    9, TUSB_DESC_CONFIGURATION,
    U16_TO_U8S_LE(67),
    2, 1, 2, 0, 0x80, 100,
    
    /* CDC IAD */
    8, TUSB_DESC_IAD, 0, 2, CDC_COMM_CLASS, CDC_COMM_SUBCLASS_ABSTRACT,
    CDC_COMM_PROTOCOL_ATCOMMAND, 0,
    
    /* CDC 接口 0 */
    9, TUSB_DESC_INTERFACE, 0, 0, 1, CDC_COMM_CLASS,
    CDC_COMM_SUBCLASS_ABSTRACT, CDC_COMM_PROTOCOL_ATCOMMAND, 0,
    
    /* CDC 头 */
    5, CDC_DESC_HEADER, U16_TO_U8S_LE(0x0110),
    
    /* CDC 呼叫管理 */
    5, CDC_DESC_CALL_MANAGEMENT, 0, 1,
    
    /* CDC ACM */
    4, CDC_DESC_ABSTRACT_CONTROL_LIMIT, 0,
    
    /* CDC 联合 */
    5, CDC_DESC_UNION, 0, 1,
    
    /* CDC 端点 */
    7, TUSB_DESC_ENDPOINT, 0x81, XFER_INTERRUPT,
    U16_TO_U8S_LE(8), 16,
    
    /* CDC 数据接口 0 */
    9, TUSB_DESC_INTERFACE, 1, 0, 2, CDC_DATA_CLASS,
    CDC_DATA_SUBCLASS_NONE, CDC_DATA_PROTOCOL_NONE, 0,
    
    /* CDC 数据端点 OUT */
    7, TUSB_DESC_ENDPOINT, 0x02, XFER_BULK,
    U16_TO_U8S_LE(64), 0,
    
    /* CDC 数据端点 IN */
    7, TUSB_DESC_ENDPOINT, 0x82, XFER_BULK,
    U16_TO_U8S_LE(64), 0
};

/* 字符串描述符 */
static uint16_t const desc_str_lang[] = { 3 << 8, 0x0409 };
static uint16_t const desc_str_0[] = { 17 << 8, 'X', 'i', 'n', 'Y', 'i', ' ', 'T', 'e', 'c', 'h', 'n', 'o', 'l', 'o', 'g', 'y' };
static uint16_t const desc_str_1[] = { 13 << 8, 'X', 'Y', ' ', 'D', 'e', 'v', 'i', 'c', 'e' };
static uint16_t const desc_str_2[] = { 13 << 8, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 'A', 'B' };

uint8_t const *tud_descriptor_device_cb(void)
{
    return desc_device;
}

uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
    (void)index;
    return desc_configuration;
}

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void)langid;
    
    static uint8_t str_buf[32];
    
    switch (index) {
        case 0: return desc_str_lang;
        case 1: return desc_str_0;
        case 2: return desc_str_1;
        case 3: return desc_str_2;
        default:
            /* 动态生成序列号 */
            uint8_t serial[12];
            xy_get_chip_serial(serial, sizeof(serial));
            
            for (int i = 0; i < 12 && i < 31; i++) {
                str_buf[i] = "0123456789ABCDEF"[serial[i] >> 4];
                str_buf[++i] = "0123456789ABCDEF"[serial[i] & 0xF];
            }
            str_buf[0] = (32 << 8);
            return (uint16_t*)str_buf;
    }
}
```

### 5. CDC 类实现

```c
/* components/third_party/usb/port/xy_tusb_cdc.c */

#include "tusb.h"
#include "xy_uart.h"

/* CDC 接收回调 */
void tud_cdc_rx_cb(uint8_t itf)
{
    uint8_t buf[64];
    uint32_t count = tud_cdc_n_read(itf, buf, sizeof(buf));
    
    /* 转发到 UART */
    xy_uart_write(XY_UART_CONSOLE, buf, count);
}

/* CDC 发送回调 */
void tud_cdc_tx_complete_cb(uint8_t itf)
{
    /* 可以继续发送 */
}

/* USB 虚拟串口初始化 */
int xy_usb_cdc_init(void)
{
    tusb_init();
    return 0;
}

/* USB 虚拟串口读取 */
int xy_usb_cdc_read(void *buf, size_t len)
{
    if (tud_cdc_connected()) {
        return tud_cdc_read(buf, len);
    }
    return 0;
}

/* USB 虚拟串口写入 */
int xy_usb_cdc_write(const void *buf, size_t len)
{
    if (tud_cdc_connected()) {
        return tud_cdc_write(buf, len);
    }
    return 0;
}

/* USB 任务处理 */
void xy_usb_cdc_task(void)
{
    tud_task();
    
    /* 自动 flush */
    tud_cdc_write_flush();
}
```

### 6. 使用示例

```c
#include "xy_usb_cdc.h"

int main(void)
{
    /* 初始化系统 */
    xy_os_kernel_init();
    
    /* 初始化 USB CDC */
    xy_usb_cdc_init();
    
    while (1) {
        /* 处理 USB 任务 */
        xy_usb_cdc_task();
        
        /* 从 USB 读取数据 */
        char buf[64];
        int len = xy_usb_cdc_read(buf, sizeof(buf));
        if (len > 0) {
            /* 回显数据 */
            xy_usb_cdc_write(buf, len);
        }
        
        xy_os_delay(10);
    }
}
```

---

## 📊 集成检查清单

- [ ] 添加 Git Submodule
- [ ] 配置 CMake
- [ ] 创建 MCU 移植层
- [ ] 创建 USB 描述符
- [ ] 实现 CDC 类
- [ ] 测试虚拟串口
- [ ] 添加文档

---

## 🎯 集成时间表

| 阶段 | 任务 | 工时 |
|------|------|------|
| **阶段 1** | 添加 Submodule + 配置 | 2h |
| **阶段 2** | MCU 移植层 | 3h |
| **阶段 3** | USB 描述符 | 2h |
| **阶段 4** | CDC 类实现 | 2h |
| **阶段 5** | 测试 + 文档 | 1h |
| **总计** | - | **10h** |

---

## 📚 相关文档

- [TinyUSB 官方文档](https://docs.tinyusb.org/)
- [TinyUSB API 参考](https://tinyusb.org/docs/api/)
- [TinyUSB 移植指南](https://tinyusb.org/docs/porting.html)

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0 (XY) + MIT (TinyUSB)
