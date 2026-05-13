# MUX Component - Universal Multiplexing Interface

**状态**: 🟢 完善

## 📋 功能概述

通用 MUX 接口库实现软件定义硬件的核心思想：

1. **单一物理接口复用** - 通过 USB/UART/SPI 单一接口复用多种外设
2. **TLV 协议封装** - 统一的 Type-Length-Value 数据包格式
3. **插件式驱动管理** - 支持运行时注册/注销外设
4. **跨平台支持** - MCU + PC SDK

## 📁 目录结构

```
mux/
├── inc/                    # 头文件
│   ├── xy_mux.h           # 核心类型和API定义
│   ├── xy_mux_gpio.h      # GPIO multiplexing API
│   ├── xy_mux_i2c.h       # I2C multiplexing API
│   ├── xy_mux_spi.h       # SPI multiplexing API
│   └── xy_mux_uart.h      # UART multiplexing API
├── src/                    # 源文件
│   ├── xy_mux.c           # 核心实现
│   ├── xy_mux_gpio.c      # GPIO 实现
│   ├── xy_mux_i2c.c       # I2C 实现
│   ├── xy_mux_spi.c       # SPI 实现
│   └── xy_mux_uart.c      # UART 实现
├── CMakeLists.txt
├── Kconfig
└── README.md
```

## 🔌 TLV 协议

### 数据包格式

```c
// TLV 包头 (4字节)
typedef struct {
    uint8_t  type;      // 外设类型 (XY_MUX_TYPE_*)
    uint8_t  channel;   // 通道号
    uint16_t length;    // 数据长度
} xy_mux_header_t;
```

### 外设类型

| 类型             | 值  | 说明 |
| ---------------- | --- | ---- |
| XY_MUX_TYPE_GPIO | 1   | GPIO |
| XY_MUX_TYPE_UART | 2   | UART |
| XY_MUX_TYPE_I2C  | 3   | I2C  |
| XY_MUX_TYPE_SPI  | 4   | SPI  |
| XY_MUX_TYPE_PWM  | 5   | PWM  |
| XY_MUX_TYPE_ADC  | 6   | ADC  |

## 📡 核心 API

### 初始化与管理

```c
// 初始化 MUX 管理器
int32_t xy_mux_init(xy_mux_manager_t *mgr,
                    uint8_t *tx_buffer, uint8_t *rx_buffer,
                    size_t buffer_size);

// 反初始化
int32_t xy_mux_deinit(xy_mux_manager_t *mgr);

// 注册外设
int32_t xy_mux_register(xy_mux_manager_t *mgr,
                        xy_mux_type_t type, uint8_t channel,
                        const xy_mux_ops_t *ops, void *user_data);

// 注销外设
int32_t xy_mux_unregister(xy_mux_manager_t *mgr,
                          xy_mux_type_t type, uint8_t channel);
```

### 数据传输

```c
// 处理接收数据包
int32_t xy_mux_process_packet(xy_mux_manager_t *mgr,
                              const uint8_t *packet, size_t len);

// 构建发送数据包
int32_t xy_mux_build_packet(xy_mux_manager_t *mgr,
                            xy_mux_type_t type, uint8_t channel,
                            const void *data, size_t len,
                            uint8_t *out_packet, size_t *out_len);

// 读写接口
int32_t xy_mux_read(xy_mux_manager_t *mgr,
                    xy_mux_type_t type, uint8_t channel,
                    void *data, size_t len);

int32_t xy_mux_write(xy_mux_manager_t *mgr,
                     xy_mux_type_t type, uint8_t channel,
                     const void *data, size_t len);

// 控制接口
int32_t xy_mux_ioctl(xy_mux_manager_t *mgr,
                     xy_mux_type_t type, uint8_t channel,
                     int cmd, void *arg);
```

## 🎛 GPIO API

### 类型定义

```c
// GPIO 方向
typedef enum {
    XY_MUX_GPIO_INPUT = 0,
    XY_MUX_GPIO_OUTPUT,
} xy_mux_gpio_dir_t;

// GPIO 电平
typedef enum {
    XY_MUX_GPIO_LOW = 0,
    XY_MUX_GPIO_HIGH,
} xy_mux_gpio_level_t;

// GPIO 配置
typedef struct {
    xy_mux_gpio_dir_t dir;
    xy_mux_gpio_level_t pull;
    bool interrupt_enable;
} xy_mux_gpio_config_t;
```

### API 函数

```c
// 注册 GPIO
int32_t xy_mux_gpio_register(xy_mux_manager_t *mgr, uint8_t channel,
                             const xy_mux_ops_t *ops, void *user_data);

// 配置 GPIO
int32_t xy_mux_gpio_config(xy_mux_manager_t *mgr, uint8_t channel,
                           const xy_mux_gpio_config_t *config);

// 读取电平 (返回 XY_MUX_GPIO_HIGH 或 XY_MUX_GPIO_LOW)
int32_t xy_mux_gpio_read(xy_mux_manager_t *mgr, uint8_t channel);

// 写入电平
int32_t xy_mux_gpio_write(xy_mux_manager_t *mgr, uint8_t channel,
                          xy_mux_gpio_level_t level);

// 切换电平
int32_t xy_mux_gpio_toggle(xy_mux_manager_t *mgr, uint8_t channel);
```

### GPIO 使用示例

```c
// GPIO 配置和使用
xy_mux_gpio_config_t gpio_config = {
    .dir = XY_MUX_GPIO_OUTPUT,
    .pull = XY_MUX_GPIO_LOW,
    .interrupt_enable = false,
};

xy_mux_gpio_config(mgr, 0, &gpio_config);
xy_mux_gpio_write(mgr, 0, XY_MUX_GPIO_HIGH);
xy_mux_gpio_toggle(mgr, 0);
```

## 📜 I2C API

### 类型定义

```c
// I2C 配置
typedef struct {
    uint32_t speed;     // 速度 (100000/400000/1000000)
    uint8_t addr_bits; // 地址位数 (7/10)
} xy_mux_i2c_config_t;

// I2C 消息
typedef struct {
    uint16_t addr;     // 设备地址
    uint16_t flags;    // 标志 (XY_MUX_I2C_M_RD)
    uint16_t len;      // 数据长度
    uint8_t *buf;      // 数据缓冲区
} xy_mux_i2c_msg_t;

#define XY_MUX_I2C_M_RD 0x0001  // 读标志
```

### API 函数

```c
// 注册 I2C
int32_t xy_mux_i2c_register(xy_mux_manager_t *mgr, uint8_t channel,
                            const xy_mux_ops_t *ops, void *user_data);

// 配置 I2C
int32_t xy_mux_i2c_config(xy_mux_manager_t *mgr, uint8_t channel,
                          const xy_mux_i2c_config_t *config);

// I2C 传输 (支持复合读/写)
int32_t xy_mux_i2c_transfer(xy_mux_manager_t *mgr, uint8_t channel,
                            xy_mux_i2c_msg_t *msgs, int count);

// 简单读写
int32_t xy_mux_i2c_read(xy_mux_manager_t *mgr, uint8_t channel,
                        uint16_t addr, void *data, size_t len);

int32_t xy_mux_i2c_write(xy_mux_manager_t *mgr, uint8_t channel,
                         uint16_t addr, const void *data, size_t len);

// 扫描总线上的设备
int32_t xy_mux_i2c_scan(xy_mux_manager_t *mgr, uint8_t channel,
                        uint16_t *addrs, size_t max_count);
```

### I2C 使用示例

```c
// 配置 I2C 总线
xy_mux_i2c_config_t i2c_config = {
    .speed = 400000,
    .addr_bits = 7,
};
xy_mux_i2c_config(mgr, 0, &i2c_config);

// 写入数据
uint8_t data[] = {0x01, 0x02, 0x03};
xy_mux_i2c_write(mgr, 0, 0x50, data, sizeof(data));

// 读取数据
uint8_t read_buf[10];
xy_mux_i2c_read(mgr, 0, 0x50, read_buf, sizeof(read_buf));

// 复合传输 (先写后读)
xy_mux_i2c_msg_t msgs[2] = {
    {.addr = 0x50, .flags = 0, .len = 1, .buf = data},
    {.addr = 0x50, .flags = XY_MUX_I2C_M_RD, .len = 10, .buf = read_buf},
};
xy_mux_i2c_transfer(mgr, 0, msgs, 2);
```

## ⚡ SPI API

### 类型定义

```c
// SPI 模式
typedef enum {
    XY_MUX_SPI_MODE0 = 0,  // CPOL=0, CPHA=0
    XY_MUX_SPI_MODE1,      // CPOL=0, CPHA=1
    XY_MUX_SPI_MODE2,      // CPOL=1, CPHA=0
    XY_MUX_SPI_MODE3,      // CPOL=1, CPHA=1
} xy_mux_spi_mode_t;

// SPI 配置
typedef struct {
    uint32_t speed;           // 速度 (Hz)
    xy_mux_spi_mode_t mode;   // SPI 模式
    uint8_t bits;            // 数据位 (8/16)
    uint8_t cs_pin;          // 片选引脚
} xy_mux_spi_config_t;
```

### API 函数

```c
// 注册 SPI
int32_t xy_mux_spi_register(xy_mux_manager_t *mgr, uint8_t channel,
                            const xy_mux_ops_t *ops, void *user_data);

// 配置 SPI
int32_t xy_mux_spi_config(xy_mux_manager_t *mgr, uint8_t channel,
                          const xy_mux_spi_config_t *config);

// 全双工传输
int32_t xy_mux_spi_transfer(xy_mux_manager_t *mgr, uint8_t channel,
                            const void *tx_data, void *rx_data, size_t len);

// 简单读写
int32_t xy_mux_spi_read(xy_mux_manager_t *mgr, uint8_t channel,
                        void *data, size_t len);

int32_t xy_mux_spi_write(xy_mux_manager_t *mgr, uint8_t channel,
                         const void *data, size_t len);
```

### SPI 使用示例

```c
// 配置 SPI
xy_mux_spi_config_t spi_config = {
    .speed = 10000000,
    .mode = XY_MUX_SPI_MODE0,
    .bits = 8,
    .cs_pin = 0,
};
xy_mux_spi_config(mgr, 0, &spi_config);

// 写入数据
uint8_t tx_buf[] = {0xAA, 0xBB, 0xCC};
xy_mux_spi_write(mgr, 0, tx_buf, sizeof(tx_buf));

// 全双工传输
uint8_t rx_buf[3];
xy_mux_spi_transfer(mgr, 0, tx_buf, rx_buf, sizeof(tx_buf));
```

## 📡 UART API

### 类型定义

```c
// UART 配置
typedef struct {
    uint32_t baudrate;     // 波特率
    uint8_t data_bits;     // 数据位 (5-8)
    uint8_t stop_bits;     // 停止位 (1-2)
    uint8_t parity;        // 校验位 (0=无, 1=奇, 2=偶)
    uint8_t flow_control;   // 流控制 (0=无, 1=RTS/CTS)
} xy_mux_uart_config_t;
```

### API 函数

```c
// 注册 UART
int32_t xy_mux_uart_register(xy_mux_manager_t *mgr, uint8_t channel,
                             const xy_mux_ops_t *ops, void *user_data);

// 配置 UART
int32_t xy_mux_uart_config(xy_mux_manager_t *mgr, uint8_t channel,
                           const xy_mux_uart_config_t *config);

// 读写操作 (带超时)
int32_t xy_mux_uart_read(xy_mux_manager_t *mgr, uint8_t channel,
                         void *data, size_t len, uint32_t timeout);

int32_t xy_mux_uart_write(xy_mux_manager_t *mgr, uint8_t channel,
                          const void *data, size_t len, uint32_t timeout);
```

### UART 使用示例

```c
// 配置 UART
xy_mux_uart_config_t uart_config = {
    .baudrate = 115200,
    .data_bits = 8,
    .stop_bits = 1,
    .parity = 0,
    .flow_control = 0,
};
xy_mux_uart_config(mgr, 0, &uart_config);

// 发送数据
uint8_t tx_data[] = "Hello UART!";
xy_mux_uart_write(mgr, 0, tx_data, sizeof(tx_data), 100);

// 接收数据
uint8_t rx_data[100];
int32_t len = xy_mux_uart_read(mgr, 0, rx_data, sizeof(rx_data), 1000);
```

## 🔧 完整使用示例

```c
#include "xy_mux.h"
#include "xy_mux_gpio.h"
#include "xy_mux_i2c.h"
#include "xy_mux_uart.h"

uint8_t tx_buffer[512];
uint8_t rx_buffer[512];

int main(void)
{
    xy_mux_manager_t mgr;

    // 初始化 MUX 管理器
    xy_mux_init(&mgr, tx_buffer, rx_buffer, sizeof(tx_buffer));

    // 注册外设
    xy_mux_gpio_register(&mgr, 0, NULL, NULL);
    xy_mux_i2c_register(&mgr, 0, NULL, NULL);
    xy_mux_uart_register(&mgr, 0, NULL, NULL);

    // 配置外设
    xy_mux_gpio_config(&mgr, 0, &(xy_mux_gpio_config_t){
        .dir = XY_MUX_GPIO_OUTPUT,
        .pull = XY_MUX_GPIO_LOW,
        .interrupt_enable = false,
    });

    // 使用外设
    xy_mux_gpio_write(&mgr, 0, XY_MUX_GPIO_HIGH);

    // 清理
    xy_mux_deinit(&mgr);
    return 0;
}
```

## 📊 完成度

- [x] 核心 MUX 管理器 (xy_mux.c)
- [x] GPIO 复用 API (xy_mux_gpio.c)
- [x] I2C 复用 API (xy_mux_i2c.c)
- [x] SPI 复用 API (xy_mux_spi.c)
- [x] UART 复用 API (xy_mux_uart.c)
- [x] API 文档
- [x] 使用示例

**完成度**: 100%
