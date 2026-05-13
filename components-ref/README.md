# XinYi Components

**版本**: 2.0.0
**日期**: 2026-05-01
**状态**: 生产就绪

---

## 🏗️ 组件架构

```
components/
├── hal/              # 硬件抽象层 (MCU 外设)
├── kernel/           # 内核/OSAL
├── device/           # 设备框架层
├── drivers/          # 器件驱动层 (按类别组织)
├── sensor/          # 传感器组件
├── actuator/        # 执行器组件
├── power/           # 电源管理组件
├── gui/             # 图形界面组件
├── net/             # 网络组件
├── crypto/          # 加密组件
├── dm/              # 数据管理组件
└── ...
```

### 分层说明

| 层次 | 目录 | 说明 |
|------|------|------|
| **硬件抽象层** | `hal/` | MCU 外设驱动 (UART/SPI/I2C/GPIO) |
| **设备框架层** | `device/` | 统一设备管理框架 |
| **器件驱动层** | `drivers/` | 外设器件驱动 (传感器/显示/存储) |
| **组件层** | `sensor/` 等 | 高级 API 和业务逻辑 |

---

## 📁 Drivers 目录结构

```
components/drivers/
├── sensor/              # 传感器驱动
│   ├── temperature/     # 温湿度传感器
│   │   ├── sht30/       # SHT30/SHT40
│   │   └── dht11/       # DHT11
│   ├── motion/          # 运动传感器
│   │   └── mpu6050/     # MPU6050
│   ├── pressure/        # 气压传感器
│   │   └── bmp280/      # BMP280
│   ├── adc/             # ADC 芯片
│   │   └── ads1115/     # ADS1115
│   ├── light/           # 光线传感器
│   └── ...
├── display/             # 显示驱动
│   ├── oled/            # OLED 显示
│   │   └── ssd1306/     # SSD1306
│   ├── lcd/            # LCD 显示
│   ├── led/            # LED 驱动
│   └── rgb/            # RGB LED (WS2812)
├── storage/            # 存储驱动
│   ├── eeprom/         # EEPROM 24xx 系列
│   ├── flash/          # W25Qxx Flash
│   └── sdcard/         # SDCard
├── power/              # 电源管理驱动
│   └── bq25620/        # BQ25620 充电器
├── wireless/           # 无线驱动
│   └── rc522/          # RC522 RFID
└── system/            # 系统驱动
    ├── key/            # 按键
    ├── rtc/            # RTC
    └── watchdog/       # 看门狗
```

---

## 🔌 驱动列表

### 传感器驱动 (Sensor Drivers)

| 型号 | 类型 | 接口 | 状态 |
|------|------|------|------|
| SHT30 | 温湿度 | I2C | ✅ |
| SHT40 | 温湿度 | I2C | ✅ |
| DHT11 | 温湿度 | GPIO | ✅ |
| MPU6050 | IMU | I2C | ✅ |
| BMP280 | 气压 | I2C | ✅ |
| ADS1115 | ADC | I2C | ✅ |

### 显示驱动 (Display Drivers)

| 型号 | 类型 | 接口 | 状态 |
|------|------|------|------|
| SSD1306 | OLED | I2C | ✅ |
| WS2812 | RGB LED | GPIO/SPI | ✅ |

### 存储驱动 (Storage Drivers)

| 型号 | 类型 | 接口 | 状态 |
|------|------|------|------|
| 24xx | EEPROM | I2C | ✅ |
| W25Qxx | Flash | SPI | ✅ |

### 电源驱动 (Power Drivers)

| 型号 | 类型 | 功能 | 状态 |
|------|------|------|------|
| BQ25620 | 充电器 | 三段式充电 | ✅ |

---

## 📦 组件列表

### 核心组件

| 组件 | 说明 | 状态 |
|------|------|------|
| `kernel/` | OS 抽象层 (FreeRTOS/RT-Thread/RTX) | ✅ |
| `hal/` | 硬件抽象层 (多平台支持) | ✅ |
| `device/` | 统一设备管理框架 | ✅ |
| `drivers/` | 器件驱动层 (新组织) | ✅ |

### 功能组件

| 组件 | 说明 | 状态 |
|------|------|------|
| `sensor/` | 传感器抽象层 | ✅ |
| `actuator/` | 执行器组件 | ✅ |
| `charger/` | 充电管理 | ✅ |
| `fuel_gauge/` | 电量计 | ✅ |
| `pid/` | PID 控制 | ✅ |
| `fota/` | 固件升级 | ✅ |
| `gui/` | 图形界面 | ✅ |
| `crypto/` | 加密库 | ✅ |
| `dm/` | 数据管理 | ✅ |
| `pm/` | 电源管理 | ✅ |

---

## 🔧 构建配置

### Kconfig 选项

```kconfig
# Device Drivers
config DRIVER_SENSOR
    bool "Sensor Drivers"
    default n

config DRIVER_DISPLAY
    bool "Display Drivers"
    default n

config DRIVER_STORAGE
    bool "Storage Drivers"
    default n

config DRIVER_POWER
    bool "Power Management Drivers"
    default n

config DRIVER_WIRELESS
    bool "Wireless Drivers"
    default n

config DRIVER_SYSTEM
    bool "System Drivers"
    default n
```

### CMake 构建

```cmake
# 自动构建所有类别
add_subdirectory(components/drivers)

# 或选择性构建
add_subdirectory(components/drivers/sensor)
add_subdirectory(components/drivers/display)
```

---

## 📚 相关文档

- [ARCHITECTURE_REFACTORING_PLAN.md](ARCHITECTURE_REFACTORING_PLAN.md) - 架构重组方案
- [REFACTORING_SUMMARY.md](REFACTORING_SUMMARY.md) - 重组工作总结
- [COMPONENT_ARCHITECTURE.md](COMPONENT_ARCHITECTURE.md) - 组件架构分析
- [DISPLAY_ARCHITECTURE.md](DISPLAY_ARCHITECTURE.md) - 显示架构

---

## 🔍 与业界标准对比

| 标准 | 驱动组织方式 | XinYi 采用 |
|------|-------------|-----------|
| Zephyr | 按类型/厂商 | ✅ 类似 |
| Linux Kernel | 按类型 | ✅ 类似 |
| RT-Thread | 按类型/厂商 | ✅ 类似 |

---

**维护者**: XinYi Team
**许可证**: Apache License 2.0
