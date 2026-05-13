# XY HAL 重构说明 / Restructuring Notes

## 版本 2.0 更新 / Version 2.0 Updates

**更新日期**: 2025-10-26

### 主要变更 / Major Changes

#### 1. 目录结构重组 / Directory Restructure

**旧结构 (v1.0)**:
```
bsp/xy_hal/stm32/
├── xy_hal_pin_stm32.c
├── xy_hal_uart_stm32.c
├── xy_hal_spi_stm32.c
└── ...
```

**新结构 (v2.0)**:
```
bsp/xy_hal/stm32/
├── stm32f4/          # STM32F4 系列实现
│   ├── xy_hal_pin.c
│   ├── xy_hal_uart.c
│   ├── xy_hal_spi.c
│   └── ...
├── stm32u0/          # STM32U0 系列实现
│   ├── xy_hal_pin.c
│   ├── xy_hal_uart.c
│   └── ...
├── stm32l4/          # STM32L4 系列实现
├── stm32h7/          # STM32H7 系列实现
└── stm32_hal.h       # 公共头文件
```

#### 2. 函数命名统一 / Function Naming Convention

**移除平台后缀 / Removed Platform Suffix**:

- ❌ 旧命名: `xy_hal_pin_init_stm32()`
- ✅ 新命名: `xy_hal_pin_init()`

所有实现文件中的函数名与 `inc/` 中的接口声明完全一致，不包含平台或系列后缀。

**原因 / Rationale**:
- 简化函数命名
- 通过目录结构区分不同系列
- 便于跨平台代码编写
- 降低学习成本

### 支持的 STM32 系列 / Supported STM32 Series

当前框架支持以下 STM32 子系列目录结构：

| 系列目录 | 说明 | 状态 |
|---------|------|------|
| `stm32f4/` | STM32F4 系列 (F401/F407/F411等) | ✅ 已实现 |
| `stm32u0/` | STM32U0 系列 (超低功耗) | ⏳ 待实现 |
| `stm32l4/` | STM32L4 系列 (低功耗) | ⏳ 待实现 |
| `stm32h7/` | STM32H7 系列 (高性能) | ⏳ 待实现 |
| `stm32f1/` | STM32F1 系列 (经典) | 📋 可扩展 |
| `stm32f7/` | STM32F7 系列 (高性能) | 📋 可扩展 |
| `stm32g0/` | STM32G0 系列 (主流) | 📋 可扩展 |
| `stm32g4/` | STM32G4 系列 (数字电源) | 📋 可扩展 |
| `stm32wb/` | STM32WB 系列 (无线) | 📋 可扩展 |
| `stm32wl/` | STM32WL 系列 (LoRa) | 📋 可扩展 |

### 编译系统更新 / Build System Updates

#### CMakeLists.txt

新增 `STM32_SERIES` 变量：

```cmake
set(STM32_SERIES "stm32f4" CACHE STRING "STM32 Series")

file(GLOB HAL_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/${STM32_SERIES}/*.c"
)
```

使用方法:
```bash
cmake .. -DSTM32_SERIES=stm32f4
# or
cmake .. -DSTM32_SERIES=stm32u0
```

#### Makefile

新增系列选择：

```makefile
STM32_SERIES ?= stm32f4
SRCS = $(wildcard $(SERIES_DIR)/*.c)
```

使用方法:
```bash
make STM32_SERIES=stm32f4
# or
make STM32_SERIES=stm32u0
```

### 新增系列实现指南 / Adding New Series Implementation

要添加新的 STM32 子系列支持：

1. **创建目录**:
   ```bash
   mkdir bsp/xy_hal/stm32/stm32xx
   ```

2. **实现所有外设**:
   复制参考实现（如 `stm32f4/`）并根据具体芯片修改：
   ```
   stm32xx/
   ├── xy_hal_pin.c
   ├── xy_hal_uart.c
   ├── xy_hal_spi.c
   ├── xy_hal_i2c.c
   ├── xy_hal_timer.c
   ├── xy_hal_pwm.c
   ├── xy_hal_rtc.c
   ├── xy_hal_dma.c
   └── xy_hal_lp_timer.c
   ```

3. **函数名保持一致**:
   - 函数声明：`inc/xy_hal_pin.h` → `int xy_hal_pin_init(...)`
   - 函数实现：`stm32xx/xy_hal_pin.c` → `int xy_hal_pin_init(...)`
   - **不要**添加任何后缀或前缀

4. **适配 STM32 HAL**:
   ```c
   #include "../inc/xy_hal_pin.h"

   #ifdef STM32_HAL_ENABLED
   #include "stm32_hal.h"

   int xy_hal_pin_init(void *port, uint8_t pin, const xy_hal_pin_config_t *config)
   {
       // 使用对应系列的 STM32 HAL API
       GPIO_InitTypeDef gpio_init = {0};
       // ...
       HAL_GPIO_Init((GPIO_TypeDef *)port, &gpio_init);
       return 0;
   }

   #endif
   ```

5. **测试编译**:
   ```bash
   # CMake
   cmake .. -DSTM32_SERIES=stm32xx -DSTM32_FAMILY=STM32XX

   # Make
   make STM32_SERIES=stm32xx
   ```

### 迁移指南 / Migration Guide

如果你之前使用 v1.0 版本：

#### 代码无需修改
应用层代码**不需要任何修改**，因为函数接口完全一致：

```c
// v1.0 和 v2.0 都适用
#include "xy_hal.h"

xy_hal_pin_config_t config = {...};
xy_hal_pin_init(GPIOA, 5, &config);  // 完全相同
```

#### 构建脚本需要更新

**Makefile**:
```makefile
# v1.0
C_SOURCES += $(XY_HAL_STM32)/xy_hal_pin_stm32.c

# v2.0
STM32_SERIES ?= stm32f4
C_SOURCES += $(wildcard $(XY_HAL_STM32)/$(STM32_SERIES)/*.c)
```

**CMakeLists.txt**:
```cmake
# v1.0
set(HAL_SOURCES
    xy_hal_pin_stm32.c
    xy_hal_uart_stm32.c
    ...
)

# v2.0
set(STM32_SERIES "stm32f4")
file(GLOB HAL_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/${STM32_SERIES}/*.c"
)
```

**Keil/IAR**:
- 移除旧的 `xy_hal_*_stm32.c` 文件
- 添加 `stm32f4/xy_hal_*.c` 文件

### 优势 / Advantages

#### 1. 可扩展性 / Scalability
- ✅ 轻松添加新的 STM32 子系列
- ✅ 不同系列独立维护
- ✅ 避免代码冲突

#### 2. 清晰性 / Clarity
- ✅ 目录结构一目了然
- ✅ 函数命名更简洁
- ✅ 系列差异通过目录隔离

#### 3. 维护性 / Maintainability
- ✅ 每个系列独立演进
- ✅ 便于针对特定系列优化
- ✅ 降低跨系列影响

#### 4. 兼容性 / Compatibility
- ✅ 应用代码无需修改
- ✅ 接口定义保持不变
- ✅ 平滑升级路径

### 设计原则 / Design Principles

1. **接口统一，实现分离**
   - `inc/` 定义统一接口
   - `stm32/stm32xx/` 提供系列特定实现

2. **函数名即是接口名**
   - 实现函数名与声明完全一致
   - 通过目录而非命名区分平台

3. **编译时选择**
   - 通过构建系统选择目标系列
   - 只编译需要的实现文件

4. **易于扩展**
   - 添加新系列只需创建目录
   - 复制参考实现快速启动

### 文档更新 / Documentation Updates

以下文档已更新以反映新结构：

- ✅ `README.md` - 使用说明
- ✅ `INTEGRATION_GUIDE.md` - 集成指南
- ✅ `IMPLEMENTATION_SUMMARY.md` - 实现总结
- ✅ `CMakeLists.txt` - CMake 配置
- ✅ `Makefile.template` - Make 模板

### 常见问题 / FAQ

**Q: 为什么要移除 `_stm32` 后缀？**
A: 因为通过目录结构已经可以清晰区分平台，函数名添加后缀会导致：
- 命名冗长
- 跨平台移植时需要修改大量代码
- 与接口声明不一致，增加理解成本

**Q: 如何选择使用哪个系列？**
A: 通过构建系统的 `STM32_SERIES` 变量选择，例如：
- `STM32_SERIES=stm32f4` - 使用 F4 系列实现
- `STM32_SERIES=stm32u0` - 使用 U0 系列实现

**Q: 可以同时编译多个系列吗？**
A: 不建议。通常一个项目只针对一个 MCU 系列。如需支持多系列，建议创建不同的构建配置。

**Q: 不同系列的实现可以共享代码吗？**
A: 可以。公共代码可以放在 `stm32_hal.h` 或创建 `common/` 目录。

**Q: v1.0 的代码会继续维护吗？**
A: 不会。v2.0 是正式版本，建议尽快迁移。迁移成本极低（仅构建脚本）。

---

**版本**: 2.0
**更新时间**: 2025-10-26
**兼容性**: 应用代码完全兼容 v1.0
