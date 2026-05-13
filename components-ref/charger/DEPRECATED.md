# ⚠️ DEPRECATED - 该组件已过时

**状态**: 🚫 Deprecated（已弃用）
**日期**: 2024-12-19
**替代方案**: 见下文

---

## 迁移说明

该组件已被标记为过时（deprecated）。新代码应该使用新的架构：

### 替代方案

#### 1. 硬件驱动层
```
drivers/power/charger/    # 充电器硬件驱动
drivers/power/fuel_gauge/  # 电量计硬件驱动
```

**用途**: 直接控制硬件芯片

#### 2. 电源管理层
```
components/pm/            # 高级电源管理功能
```

**用途**: 电源策略、电池管理、充电管理等高级功能

---

## 迁移路径

### 旧代码（Deprecated）
```c
#include "xy_charger.h"      // ❌ 过时
#include "xy_fuel_gauge.h"   // ❌ 过时

xy_charger_init();
xy_fuel_gauge_init();
```

### 新代码（推荐）

#### 选项 1: 使用驱动层（直接控制硬件）
```c
#include "drivers/power/charger/xy_bq25620.h"
#include "drivers/power/fuel_gauge/xy_fg_max17043.h"

// 直接控制充电器芯片
xy_bq25620_init();
xy_bq25620_set_current(1000);  // 1A

// 直接控制电量计芯片
xy_fg_max17043_init();
xy_fg_max17043_get_soc(&soc);
```

#### 选项 2: 使用电源管理层（推荐）
```c
#include "pm/inc/xy_pm.h"

// 使用高级电源管理 API
xy_pm_init();
xy_pm_charge_start();
xy_pm_get_battery_status(&status);
```

---

## 时间表

| 版本 | 状态 | 说明 |
|------|------|------|
| v2.0 | ⚠️ Deprecated | 标记为过时，仍可使用 |
| v2.5 | ⚠️ Warning | 编译时警告 |
| v3.0 | 🚫 Removed | 完全移除 |

---

## 为什么废弃？

### 问题

1. **架构混乱**: charger 和 fuel_gauge 在 4 个位置重复
2. **职责不清**: 既有驱动又有组件逻辑
3. **维护困难**: 多个版本不一致
4. **头文件冲突**: xy_charger.h 在多个位置

### 新架构优势

✅ **清晰分层**: 驱动层 → 电源管理层 → 应用层
✅ **单一职责**: 每层有明确的职责
✅ **易于维护**: 统一管理，避免重复
✅ **符合标准**: 与 Linux/Zephyr 等主流架构一致

---

## 需要帮助？

如果您在迁移过程中遇到问题，请参考：
- [drivers/power/README.md](../../drivers/power/README.md) - 驱动层文档
- [pm/README.md](../../pm/README.md) - 电源管理文档
- [ARCHITECTURE_REFACTORING_PLAN.md](../ARCHITECTURE_REFACTORING_PLAN.md) - 架构重组方案

---

**最后更新**: 2024-12-19
