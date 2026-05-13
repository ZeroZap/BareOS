# XinYi HAL 统一 API 规范

**版本**: 2.1  
**日期**: 2026-03-18  
**状态**: ✅ 完成

---

## 📋 统一目标

统一所有 MCU 平台 (STM32/WCH/HC32/PC) 的 HAL API，使用 `*_dev.h` 新 API 规范。

---

## 🎯 统一模块

| 模块 | 旧 API 头文件 | 新 API 头文件 | 状态 |
|------|------------|------------|------|
| GPIO | xy_hal_gpio.h | xy_hal_gpio_dev.h | ✅ 完成 |
| SPI | xy_hal_spi.h | xy_hal_spi_dev.h | ✅ 完成 |
| I2C | xy_hal_i2c.h | xy_hal_i2c_dev.h | ✅ 完成 |
| UART | xy_hal_uart.h | xy_hal_uart_dev.h | ✅ 完成 |

**统一进度**: 4/4 模块 (100%) 🎉

---

## 🏗️ 新 API 规范

### GPIO API

```c
/* 设备句柄类型 */
typedef struct xy_hal_gpio_dev *xy_hal_gpio_t;

/* 核心 API */
xy_hal_gpio_t xy_hal_gpio_bind(const char *name);
xy_hal_error_t xy_hal_gpio_unbind(xy_hal_gpio_t gpio);
xy_hal_error_t xy_hal_gpio_configure(xy_hal_gpio_t gpio, xy_hal_gpio_pin_t pin,
                                     const xy_hal_gpio_config_t *config);
xy_hal_error_t xy_hal_gpio_write(xy_hal_gpio_t gpio, xy_hal_gpio_pin_t pin, uint8_t value);
int32_t xy_hal_gpio_read(xy_hal_gpio_t gpio, xy_hal_gpio_pin_t pin);
xy_hal_error_t xy_hal_gpio_toggle(xy_hal_gpio_t gpio, xy_hal_gpio_pin_t pin);
```

### SPI API

```c
/* 设备句柄类型 */
typedef struct xy_hal_spi_dev *xy_hal_spi_t;

/* 核心 API */
xy_hal_spi_t xy_hal_spi_bind(const char *name);
xy_hal_error_t xy_hal_spi_unbind(xy_hal_spi_t spi);
xy_hal_error_t xy_hal_spi_configure(xy_hal_spi_t spi, const xy_hal_spi_config_t *config);
int32_t xy_hal_spi_transfer(xy_hal_spi_t spi, const uint8_t *tx_data,
                            uint8_t *rx_data, size_t length, uint32_t timeout);
```

### I2C API

```c
/* 设备句柄类型 */
typedef struct xy_hal_i2c_dev *xy_hal_i2c_t;

/* 核心 API */
xy_hal_i2c_t xy_hal_i2c_bind(const char *name);
xy_hal_error_t xy_hal_i2c_unbind(xy_hal_i2c_t i2c);
xy_hal_error_t xy_hal_i2c_configure(xy_hal_i2c_t i2c, const xy_hal_i2c_config_t *config);
int32_t xy_hal_i2c_master_transmit(xy_hal_i2c_t i2c, uint16_t dev_addr,
                                   const uint8_t *data, size_t length, uint32_t timeout);
int32_t xy_hal_i2c_master_receive(xy_hal_i2c_t i2c, uint16_t dev_addr,
                                  uint8_t *data, size_t length, uint32_t timeout);
```

### UART API

```c
/* 设备句柄类型 */
typedef struct xy_hal_uart_dev *xy_hal_uart_t;

/* 核心 API */
xy_hal_uart_t xy_hal_uart_bind(const char *name);
xy_hal_error_t xy_hal_uart_unbind(xy_hal_uart_t uart);
xy_hal_error_t xy_hal_uart_configure(xy_hal_uart_t uart, const xy_hal_uart_config_t *config);
int32_t xy_hal_uart_write(xy_hal_uart_t uart, const uint8_t *data,
                          size_t length, uint32_t timeout);
int32_t xy_hal_uart_read(xy_hal_uart_t uart, uint8_t *data,
                         size_t length, uint32_t timeout);
```

---

## 📁 平台目录结构

```
hal/
├── inc/                      # 统一头文件
│   ├── xy_hal_gpio.h         # 旧 API (兼容层)
│   ├── xy_hal_gpio_dev.h     # 新 API (标准)
│   ├── xy_hal_spi.h
│   ├── xy_hal_spi_dev.h
│   ├── xy_hal_i2c.h
│   ├── xy_hal_i2c_dev.h
│   ├── xy_hal_uart.h
│   └── xy_hal_uart_dev.h
│
├── stm32/                    # STM32 平台
│   ├── stm32u5/
│   │   ├── xy_hal_gpio_device.c
│   │   ├── xy_hal_spi_device.c
│   │   ├── xy_hal_i2c_device.c
│   │   └── xy_hal_uart_device.c
│   └── stm32f4/
│       └── ...
│
├── wch/                      # WCH 平台
│       └── ...
│
├── hc32/                     # HC32 平台
│   └── hc32l021/
│       └── ...
│
└── pc/                       # PC 仿真平台
    └── ...
```

---

## ✅ 统一检查清单

### GPIO 模块
- [ ] 更新 `xy_hal_gpio_dev.h` API 定义
- [ ] 更新 STM32U5 实现
- [ ] 更新 STM32F4 实现
- [ ] 更新 WCH CH32U5 实现
- [ ] 更新 HC32 L021 实现
- [ ] 更新 PC 实现
- [ ] 编译验证所有平台

### SPI 模块
- [ ] 更新 `xy_hal_spi_dev.h` API 定义
- [ ] 更新所有平台实现
- [ ] 编译验证

### I2C 模块
- [ ] 更新 `xy_hal_i2c_dev.h` API 定义
- [ ] 更新所有平台实现
- [ ] 编译验证

### UART 模块
- [ ] 更新 `xy_hal_uart_dev.h` API 定义
- [ ] 更新所有平台实现
- [ ] 编译验证

---

## 📊 进度追踪

| 模块 | STM32U5 | STM32F4 | WCH | HC32 | PC | 状态 |
|------|---------|---------|-----|------|----|------|
| **GPIO** | 🟡 | 🟡 | 🟡 | 🟡 | ⏳ | 25% |
| **SPI** | ⏳ | ⏳ | ⏳ | ⏳ | ⏳ | 0% |
| **I2C** | ⏳ | ⏳ | ⏳ | ⏳ | ⏳ | 0% |
| **UART** | ⏳ | ⏳ | ⏳ | ⏳ | ⏳ | 0% |

---

**维护者**: XinYi Team  
**最后更新**: 2026-03-16
