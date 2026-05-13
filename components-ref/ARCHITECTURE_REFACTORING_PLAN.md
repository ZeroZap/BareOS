# XinYi 组件架构重组方案

**日期**: 2024-12-19
**目标**: 剥离 MCU 外设驱动与外设器件驱动，建立清晰的分层架构
**状态**: 📋 待执行

---

## 🎯 问题分析

### 当前存在的问题

1. **命名混淆**
   - `components/driver/` - 包含外设器件驱动（charger, rfid, sensor, storage）
   - `components/drivers/` - 包含高级驱动（display, key, rtc, sys）
   - 两个目录命名相似，容易混淆

2. **职责不清**
   - `components/device/` - 包含具体外设芯片驱动（ADS1115, BMP280, MPU6050等）
   - `components/hal/` - 包含 MCU 外设驱动（UART, SPI, I2C, GPIO等）
   - device 和 drivers 的界限模糊

3. **层次混乱**
   - MCU 外设驱动（HAL层）与外设器件驱动混在一起
   - 缺乏清晰的分层架构

---

## 🏗️ 目标架构

### 分层设计原则

```
┌─────────────────────────────────────────┐
│         应用层 (Application)             │
│     (projects/examples/tests)            │
└─────────────────────────────────────────┘
              ↓
┌─────────────────────────────────────────┐
│         组件层 (Components)              │
│   - sensor (传感器组件)                  │
│   - actuator (执行器组件)                │
│   - charger (充电器组件)                 │
│   - fuel_gauge (电量计组件)              │
│   - display (显示组件)                   │
│   - storage (存储组件)                   │
└─────────────────────────────────────────┘
              ↓
┌─────────────────────────────────────────┐
│      器件驱动层 (Device Drivers)         │
│   - drivers/sensor/ (传感器驱动)         │
│   - drivers/display/ (显示驱动)          │
│   - drivers/storage/ (存储驱动)          │
│   - drivers/power/ (电源驱动)            │
│   - drivers/wireless/ (无线驱动)         │
└─────────────────────────────────────────┘
              ↓
┌─────────────────────────────────────────┐
│      设备抽象层 (Device Layer)           │
│   - device/ (统一设备框架)               │
└─────────────────────────────────────────┘
              ↓
┌─────────────────────────────────────────┐
│   硬件抽象层 (HAL - MCU 外设)            │
│   - hal/gpio  - hal/uart                 │
│   - hal/spi   - hal/i2c                  │
│   - hal/adc   - hal/pwm                  │
│   - hal/timer - hal/dma                  │
└─────────────────────────────────────────┘
              ↓
┌─────────────────────────────────────────┐
│      硬件层 (Hardware)                   │
│   - MCU (STM32/WCH/HC32/TI)              │
│   - 外设器件 (Sensors/Displays/etc)      │
└─────────────────────────────────────────┘
```

---

## 📋 重组方案

### 方案 A: 完全重构（推荐）

#### 1. 目录结构调整

**新目录结构**:
```
components/
├── hal/                    # MCU 硬件抽象层（保持不变）
│   ├── inc/                # HAL 统一接口
│   ├── stm32/              # STM32 HAL 实现
│   ├── wch/                # WCH HAL 实现
│   ├── hc32/               # HC32 HAL 实现
│   └── PC/                 # PC 模拟实现
│
├── device/                 # 设备抽象层（保持核心框架）
│   ├── inc/                # 设备框架接口
│   │   ├── xy_device.h     # 设备基础结构
│   │   └── xy_device_api.h # 设备 API 定义
│   ├── src/                # 设备框架实现
│   │   ├── xy_device_core.c
│   │   └── xy_device_mgr.c
│   └── tests/              # 设备框架测试
│
├── drivers/                # 器件驱动层（重组后的统一驱动）
│   ├── sensor/             # 传感器驱动
│   │   ├── temperature/    # 温湿度传感器
│   │   │   ├── sht30/
│   │   │   ├── aht20/
│   │   │   └── dht11/
│   │   ├── pressure/       # 压力传感器
│   │   │   └── bmp280/
│   │   ├── motion/         # 运动传感器
│   │   │   └── mpu6050/
│   │   └── adc/            # ADC 传感器
│   │       └── ads1115/
│   │
│   ├── display/            # 显示驱动
│   │   ├── oled/
│   │   │   └── ssd1306/
│   │   ├── lcd/
│   │   └── led/
│   │
│   ├── storage/            # 存储驱动
│   │   ├── eeprom/
│   │   │   └── 24xx/
│   │   ├── flash/
│   │   └── sdcard/
│   │
│   ├── power/              # 电源相关驱动
│   │   ├── charger/        # 充电器驱动
│   │   │   ├── bq25895/
│   │   │   └── tp4056/
│   │   └── fuel_gauge/     # 电量计驱动
│   │       ├── max17043/
│   │       └── bq27z561/
│   │
│   ├── wireless/           # 无线通信驱动
│   │   ├── rfid/
│   │   │   └── rc522/
│   │   ├── wifi/
│   │   └── bluetooth/
│   │
│   └── system/             # 系统驱动
│       ├── key/            # 按键驱动
│       ├── rtc/            # RTC 驱动
│       └── watchdog/       # 看门狗驱动
│
├── sensor/                 # 传感器组件（高级封装）
│   ├── inc/
│   │   └── xy_sensor.h     # 统一传感器 API
│   ├── core/
│   │   └── sensor_core.c
│   └── README.md
│
├── actuator/               # 执行器组件
├── charger/                # 充电器组件
├── fuel_gauge/             # 电量计组件
├── gui/                    # GUI 组件
├── net/                    # 网络组件
└── sys/                    # 系统组件
```

#### 2. 迁移计划

**阶段 1: 目录重组（1-2 小时）**

```bash
# 1. 创建新的 drivers 子目录结构
mkdir -p components/drivers/{sensor,display,storage,power,wireless,system}
mkdir -p components/drivers/sensor/{temperature,pressure,motion,adc}
mkdir -p components/drivers/display/{oled,lcd,led}
mkdir -p components/drivers/storage/{eeprom,flash,sdcard}
mkdir -p components/drivers/power/{charger,fuel_gauge}
mkdir -p components/drivers/wireless/rfid
mkdir -p components/drivers/system/{key,rtc,watchdog}

# 2. 迁移 device/ 中的具体器件驱动到 drivers/
# 从 device/ 迁移传感器驱动
mv components/device/xy_sht30.* components/drivers/sensor/temperature/sht30/
mv components/device/xy_bmp280.* components/drivers/sensor/pressure/bmp280/
mv components/device/xy_mpu6050.* components/drivers/sensor/motion/mpu6050/
mv components/device/xy_ads1115.* components/drivers/sensor/adc/ads1115/

# 从 device/ 迁移显示驱动
mv components/device/xy_oled_ssd1306.* components/drivers/display/oled/ssd1306/

# 从 device/ 迁移存储驱动
mv components/device/xy_eeprom_24xx.* components/drivers/storage/eeprom/24xx/

# 3. 迁移 driver/ 中的驱动
mv components/driver/charger/* components/drivers/power/charger/
mv components/driver/rfid/* components/drivers/wireless/rfid/
mv components/driver/sensor/* components/drivers/sensor/
mv components/driver/storage/* components/drivers/storage/

# 4. 迁移 drivers/ 中的驱动
mv components/drivers/display/* components/drivers/display/
mv components/drivers/xy_key components/drivers/system/key/
mv components/drivers/xy_rtc components/drivers/system/rtc/

# 5. 删除旧目录
rm -rf components/driver
rm -rf components/drivers/xy_*
rm -rf components/drivers/inc  # 如果是空的
```

**阶段 2: 代码调整（2-4 小时）**

1. **更新头文件路径**
   ```c
   // 旧路径
   #include "xy_sht30.h"

   // 新路径
   #include "drivers/sensor/temperature/sht30/xy_sht30.h"
   ```

2. **更新 CMakeLists.txt**
   ```cmake
   # components/drivers/CMakeLists.txt
   add_subdirectory(sensor)
   add_subdirectory(display)
   add_subdirectory(storage)
   add_subdirectory(power)
   add_subdirectory(wireless)
   add_subdirectory(system)
   ```

3. **更新 Kconfig**
   ```kconfig
   menu "Device Drivers"
       source "components/drivers/sensor/Kconfig"
       source "components/drivers/display/Kconfig"
       source "components/drivers/storage/Kconfig"
       source "components/drivers/power/Kconfig"
       source "components/drivers/wireless/Kconfig"
       source "components/drivers/system/Kconfig"
   endmenu
   ```

**阶段 3: 清理与验证（1 小时）**

1. **清理 device/ 目录**
   ```bash
   # 保留设备框架核心文件
   components/device/
   ├── inc/
   │   ├── xy_device.h
   │   └── xy_device_api.h
   ├── src/
   │   ├── xy_device_core.c
   │   └── xy_device_mgr.c
   └── tests/
   ```

2. **验证编译**
   ```bash
   # 清理构建目录
   rm -rf build_*

   # 重新编译各平台
   ./build.sh stm32f4
   ./build.sh stm32u5
   ./build.sh wch
   ```

3. **运行测试**
   ```bash
   # 运行单元测试
   cd components/drivers/sensor/temperature/sht30/tests
   ./test_sht30
   ```

---

### 方案 B: 渐进式重构（保守方案）

#### 1. 保持现有结构，建立软链接

```bash
# 创建新的 drivers 结构，但通过软链接指向旧位置
# 这样可以平滑过渡，不影响现有代码

# 在 Linux/macOS
ln -s ../../device/xy_sht30.c components/drivers/sensor/temperature/sht30/xy_sht30.c
ln -s ../../device/xy_sht30.h components/drivers/sensor/temperature/sht30/xy_sht30.h

# 在 Windows (需要管理员权限)
mklink components\drivers\sensor\temperature\sht30\xy_sht30.c ..\..\..\..\device\xy_sht30.c
```

#### 2. 逐步迁移

1. 先迁移使用频率低的驱动
2. 在新位置创建驱动副本
3. 更新部分项目使用新路径
4. 验证稳定后删除旧文件
5. 重复上述步骤

---

## 📊 对比分析

| 特性 | 方案 A (完全重构) | 方案 B (渐进式) |
|------|------------------|----------------|
| **实施时间** | 4-7 小时 | 2-4 周 |
| **风险** | 中等 | 低 |
| **代码改动** | 大 | 小 |
| **长期收益** | 高 | 中 |
| **学习曲线** | 陡峭 | 平缓 |
| **可回退性** | 通过 Git | 容易 |
| **推荐度** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ |

---

## 🎯 推荐方案：方案 A（完全重构）

### 理由

1. **一次性解决问题** - 避免长期维护两套结构
2. **清晰的架构** - 符合业界最佳实践（Zephyr/Linux）
3. **易于扩展** - 新驱动有明确的归属
4. **降低维护成本** - 统一的目录结构

### 实施步骤

#### 步骤 1: 备份当前状态（5 分钟）
```bash
git checkout -b backup/before-refactoring
git add .
git commit -m "Backup before architecture refactoring"
git push origin backup/before-refactoring
```

#### 步骤 2: 创建重构分支（5 分钟）
```bash
git checkout -b refactor/component-architecture
```

#### 步骤 3: 执行目录重组（30 分钟）
```bash
# 创建脚本自动化迁移
./scripts/refactor_components.sh
```

#### 步骤 4: 更新构建文件（1 小时）
- 更新所有 CMakeLists.txt
- 更新所有 Kconfig
- 更新 README.md

#### 步骤 5: 更新代码引用（1-2 小时）
- 使用 IDE 全局搜索替换
- 更新 #include 路径
- 更新文档链接

#### 步骤 6: 测试验证（1 小时）
- 编译所有平台配置
- 运行单元测试
- 运行集成测试

#### 步骤 7: 文档更新（30 分钟）
- 更新架构文档
- 更新开发指南
- 更新 API 文档

#### 步骤 8: 合并主分支（30 分钟）
```bash
git add .
git commit -m "Refactor: Reorganize component architecture

- Separate MCU peripherals (HAL) from device drivers
- Reorganize drivers by category (sensor/display/storage/power/wireless/system)
- Clean up device/ to keep only device framework
- Update all CMakeLists.txt and Kconfig
- Update documentation

Closes #XXX"

git push origin refactor/component-architecture
# 创建 Pull Request 进行 Code Review
```

---

## 📝 迁移脚本

创建 `scripts/refactor_components.sh`:

```bash
#!/bin/bash

set -e  # 遇到错误立即退出

echo "=== XinYi 组件架构重组脚本 ==="
echo "开始时间: $(date)"

# 颜色定义
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# 进入项目根目录
cd "$(dirname "$0")/.."

echo -e "${YELLOW}步骤 1: 创建新目录结构${NC}"
mkdir -p components/drivers/sensor/{temperature,pressure,motion,adc}
mkdir -p components/drivers/display/{oled,lcd,led}
mkdir -p components/drivers/storage/{eeprom,flash,sdcard}
mkdir -p components/drivers/power/{charger,fuel_gauge}
mkdir -p components/drivers/wireless/rfid
mkdir -p components/drivers/system/{key,rtc,watchdog}

echo -e "${YELLOW}步骤 2: 迁移传感器驱动${NC}"
if [ -f "components/device/xy_sht30.c" ]; then
    mkdir -p components/drivers/sensor/temperature/sht30
    mv components/device/xy_sht30.* components/drivers/sensor/temperature/sht30/
    echo -e "${GREEN}✓ SHT30 已迁移${NC}"
fi

if [ -f "components/device/xy_bmp280.c" ]; then
    mkdir -p components/drivers/sensor/pressure/bmp280
    mv components/device/xy_bmp280.* components/drivers/sensor/pressure/bmp280/
    echo -e "${GREEN}✓ BMP280 已迁移${NC}"
fi

if [ -f "components/device/xy_mpu6050.c" ]; then
    mkdir -p components/drivers/sensor/motion/mpu6050
    mv components/device/xy_mpu6050.* components/drivers/sensor/motion/mpu6050/
    echo -e "${GREEN}✓ MPU6050 已迁移${NC}"
fi

if [ -f "components/device/xy_ads1115.c" ]; then
    mkdir -p components/drivers/sensor/adc/ads1115
    mv components/device/xy_ads1115.* components/drivers/sensor/adc/ads1115/
    echo -e "${GREEN}✓ ADS1115 已迁移${NC}"
fi

echo -e "${YELLOW}步骤 3: 迁移显示驱动${NC}"
if [ -f "components/device/xy_oled_ssd1306.c" ]; then
    mkdir -p components/drivers/display/oled/ssd1306
    mv components/device/xy_oled_ssd1306.* components/drivers/display/oled/ssd1306/
    echo -e "${GREEN}✓ SSD1306 OLED 已迁移${NC}"
fi

echo -e "${YELLOW}步骤 4: 迁移存储驱动${NC}"
if [ -f "components/device/xy_eeprom_24xx.c" ]; then
    mkdir -p components/drivers/storage/eeprom/24xx
    mv components/device/xy_eeprom_24xx.* components/drivers/storage/eeprom/24xx/
    echo -e "${GREEN}✓ EEPROM 24xx 已迁移${NC}"
fi

echo -e "${YELLOW}步骤 5: 迁移 driver/ 目录内容${NC}"
if [ -d "components/driver/charger" ]; then
    mv components/driver/charger/* components/drivers/power/charger/ 2>/dev/null || true
    echo -e "${GREEN}✓ 充电器驱动已迁移${NC}"
fi

if [ -d "components/driver/rfid" ]; then
    mv components/driver/rfid/* components/drivers/wireless/rfid/ 2>/dev/null || true
    echo -e "${GREEN}✓ RFID 驱动已迁移${NC}"
fi

echo -e "${YELLOW}步骤 6: 迁移 drivers/ 目录内容${NC}"
if [ -d "components/drivers/xy_key" ]; then
    mv components/drivers/xy_key components/drivers/system/key
    echo -e "${GREEN}✓ 按键驱动已迁移${NC}"
fi

if [ -d "components/drivers/xy_rtc" ]; then
    mv components/drivers/xy_rtc components/drivers/system/rtc
    echo -e "${GREEN}✓ RTC 驱动已迁移${NC}"
fi

echo -e "${YELLOW}步骤 7: 清理空目录${NC}"
rmdir components/driver/charger components/driver/rfid components/driver/sensor components/driver/storage 2>/dev/null || true
rmdir components/driver 2>/dev/null || true
echo -e "${GREEN}✓ 空目录已清理${NC}"

echo -e "${GREEN}=== 重组完成 ===${NC}"
echo "结束时间: $(date)"
echo ""
echo "下一步:"
echo "1. 更新 CMakeLists.txt"
echo "2. 更新 Kconfig"
echo "3. 更新代码中的 #include 路径"
echo "4. 编译测试"
```

---

## 🔍 验证清单

重组完成后，请检查：

- [ ] 所有文件已迁移到新位置
- [ ] 旧目录已删除或清空
- [ ] CMakeLists.txt 已更新
- [ ] Kconfig 已更新
- [ ] 所有 #include 路径已更新
- [ ] 编译通过（所有平台）
- [ ] 单元测试通过
- [ ] 集成测试通过
- [ ] 文档已更新
- [ ] README 已更新

---

## 📚 参考资料

- **Zephyr Project**: https://github.com/zephyrproject-rtos/zephyr
  - `zephyr/drivers/` 目录结构
  - `zephyr/dts/bindings/` 设备树绑定

- **Linux Kernel**: https://github.com/torvalds/linux
  - `drivers/` 目录组织方式

- **RT-Thread**: https://github.com/RT-Thread/rt-thread
  - `components/drivers/` 结构

---

## ⚠️ 注意事项

1. **备份优先** - 执行前务必创建备份分支
2. **逐步验证** - 每个步骤完成后立即编译测试
3. **文档同步** - 及时更新相关文档
4. **团队沟通** - 通知团队成员架构变更
5. **版本标记** - 在 CHANGELOG 中记录此重大变更

---

## 📈 预期收益

1. **清晰的架构** - MCU 外设与器件驱动完全分离
2. **易于维护** - 统一的目录结构和命名规范
3. **便于扩展** - 新驱动有明确的归属位置
4. **提升效率** - 减少查找文件的时间
5. **符合标准** - 与业界主流 RTOS 保持一致

---

**维护者**: XinYi Team
**许可证**: Apache License 2.0
**最后更新**: 2024-12-19
