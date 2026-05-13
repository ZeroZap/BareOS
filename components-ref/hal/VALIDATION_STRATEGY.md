# XinYi HAL 验证策略

**最后更新**: 2026-03-17

---

## 📊 平台支持矩阵

| 平台 | QEMU 支持 | 编译验证 | 单元测试 | 真实硬件 |
|------|----------|---------|---------|---------|
| **STM32F4** | ✅ 完整 | ✅ | ✅ | ✅ |
| **STM32U5** | ⚠️ 有限 | ✅ | ✅ | ✅ |
| **WCH CH32** | ❌ 不支持 | ✅ | ⏳ PC 模拟 | ✅ |
| **HC32** | ❌ 不支持 | ✅ | ⏳ PC 模拟 | ✅ |

---

## 🔧 各平台验证方案

### STM32F4 (QEMU 完整支持)

```bash
# 1. QEMU 自动化测试
cd tests/qemu_stm32f4/hal_test
./run_test.sh

# 2. 真实硬件验证
# 使用 ST-Link 烧录
```

**测试覆盖**:
- ✅ GPIO
- ✅ UART
- ✅ SPI
- ✅ I2C
- ✅ Timer
- ✅ ADC
- ✅ PWM

---

### WCH CH32 (QEMU 不支持)

#### 方案 1: 编译验证 ✅

```bash
cd components/hal/hc32/hc32l021
./test_compile.sh
```

**验证内容**:
- ✅ 语法检查
- ✅ 链接通过
- ✅ 固件生成

#### 方案 2: PC 单元测试 ⏳

```c
/* test_xy_hal_pc.c */
#include <stdio.h>
#include <stdint.h>

/* 模拟寄存器 */
volatile uint32_t GPIOA_BASE = 0x40010000;

/* 包含 HAL 实现 */
#include "xy_hal_gpio_device.c"

/* 测试 */
void test_gpio_bind(void)
{
    xy_hal_gpio_t gpio = xy_hal_gpio_bind("GPIOA.5");
    if (gpio != 0) {
        printf("✓ GPIO bind test passed\n");
    }
}

int main(void)
{
    test_gpio_bind();
    return 0;
}
```

**运行**:
```bash
gcc -I../inc -I. test_xy_hal_pc.c -o test
./test
```

#### 方案 3: 真实硬件 ✅

```bash
# 使用 WCH-Link 烧录
wchisp flash firmware.bin

# 或使用 OpenOCD
openocd -f wch_link.cfg -c "program firmware.bin verify reset"
```

---

### HC32 (QEMU 不支持)

#### 方案 1: 编译验证 ✅

```bash
cd components/hal/hc32
./test_compile.sh
```

#### 方案 2: PC 模拟 ⏳

类似 CH32，创建 PC 测试桩代码。

#### 方案 3: 真实硬件 ✅

```bash
# 使用 J-Link 或专用调试器
JFlash.exe -device HC32L021 -if SWD -speed 4000
```

---

## 📋 推荐验证流程

### 开发阶段

```
1. 编写 HAL 代码
       ↓
2. PC 编译验证 (快速反馈)
       ↓
3. QEMU 测试 (仅 STM32)
       ↓
4. 真实硬件验证
```

### CI/CD 流程

```yaml
# GitHub Actions 示例
name: HAL Tests

on: [push, pull_request]

jobs:
  compile_test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Install ARM GCC
        uses: carlosperate/arm-none-eabi-gcc-action@v1
      
      - name: Compile STM32 HAL
        run: |
          cd components/hal
          mkdir build && cd build
          cmake .. -DHAL_PLATFORM=STM32U5
          make -j4
      
      - name: Compile WCH HAL
        run: |
          cd components/hal/hc32
          ./test_compile.sh
      
      - name: Compile HC32 HAL
        run: |
          cd components/hal/hc32
          ./test_compile.sh
  
  qemu_test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Install QEMU
        run: sudo apt install qemu-system-arm
      
      - name: Run HAL Tests
        run: |
          cd tests/qemu_stm32f4/hal_test
          ./run_test.sh
```

---

## 🎯 当前状态

| 测试类型 | STM32F4 | STM32U5 | WCH | HC32 |
|---------|---------|---------|-----|------|
| **QEMU 测试** | ✅ 11 个用例 | ⏳ 有限 | ❌ | ❌ |
| **编译验证** | ✅ | ✅ | ✅ | ✅ |
| **PC 单元测试** | ⏳ | ⏳ | ⏳ | ⏳ |
| **真实硬件** | ✅ | ✅ | ✅ | ✅ |

---

## 📚 参考资料

- [QEMU ARM Documentation](https://www.qemu.org/docs/master/system/arm/)
- [WCH-Link 编程工具](https://github.com/openwch/ch32isp)
- [HC32 开发工具](https://www.hc32.com/)

---

**总结**: QEMU 仅支持 STM32F1/F4，CH32/HC32 需要编译验证 + 真实硬件测试。
