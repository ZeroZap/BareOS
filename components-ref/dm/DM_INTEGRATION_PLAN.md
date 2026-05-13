# DM 组件整合方案

**日期**: 2026-03-05  
**目标**: 整合 EEPROM 模拟相关代码，消除重复

---

## 📊 当前问题分析

### 重复的 EEPROM 模拟方案

| 组件 | 状态 | 问题 |
|------|------|------|
| **EEPROM/** | ⚠️ 基础实现 | 功能简单，无磨损均衡 |
| **fee/** | ✅ 最完整 | 完整 FEE 实现，支持磨损均衡 |
| **lwfee/** | ⚠️ 简化版 | 功能不完整 |
| **xy_eeprom/** | ⚠️ 重复 | 与 fee 功能重叠 |
| **xy_eflash_v1/** | ⚠️ nano fee | 仅适配 32bit 颗粒度 |
| **xy_flash/** | ✅ 基础 Flash | 底层接口 |

### 问题总结

1. **代码重复** - 5 套 EEPROM 模拟方案
2. **维护困难** - 多处修改，容易遗漏
3. **功能分散** - 相似功能分散在不同目录
4. **命名混乱** - fee/eeprom/eflash 混用

---

## ✅ 整合方案

### 最终架构

```
components/dm/
├── fee/                    # FEE 核心 (保留并优化)
│   ├── inc/
│   │   ├── xy_fee.h        # 统一 FEE 接口
│   │   └── xy_fee_nano.h   # Nano FEE (32bit 颗粒)
│   ├── src/
│   │   ├── xy_fee.c        # 完整 FEE 实现
│   │   └── xy_fee_nano.c   # Nano FEE 实现
│   └── README.md
│
├── eeprom/                 # EEPROM 接口层 (统一封装)
│   ├── inc/
│   │   └── xy_eeprom.h     # 统一 EEPROM 接口
│   ├── src/
│   │   └── xy_eeprom.c     # 基于 FEE 的实现
│   └── README.md
│
└── flash/                  # Flash 底层 (保留)
    ├── inc/
    │   └── xy_flash.h
    └── src/
        └── xy_flash.c
```

### 删除目录

```
❌ 删除：dm/EEPROM/          # 被 eeprom 替代
❌ 删除：dm/lwfee/           # 被 fee_nano 替代
❌ 删除：dm/xy_eeprom/       # 被 eeprom 替代
❌ 删除：dm/xy_eflash_v1/    # 被 fee_nano 替代
```

---

## 📦 详细设计

### 1. fee/ - FEE 核心层

#### xy_fee.h - 完整 FEE

```c
/**
 * @brief 完整 FEE 实现
 * @特点:
 * - 支持磨损均衡
 * - 支持多页管理
 * - 可配置颗粒度 (8/16/32/64/128 字节)
 * - 掉电保护
 */

typedef struct {
    uint8_t *flash_base;        // Flash 基地址
    uint16_t pages_per_fee;     // 每个 FEE 页的 Flash 页数
    uint16_t flash_page_size;   // Flash 页大小
    uint16_t cache_size;        // 缓存大小
    uint8_t write_granularity;  // 写入颗粒度
    uint16_t max_erase_count;   // 最大擦除次数
    fee_flash_ops_t flash_ops;  // Flash 操作接口
} xy_fee_config_t;

fee_status_t xy_fee_init(xy_fee_t *fee, const xy_fee_config_t *cfg);
fee_status_t xy_fee_write(xy_fee_t *fee, uint16_t addr, const void *data, uint16_t len);
fee_status_t xy_fee_read(xy_fee_t *fee, uint16_t addr, void *data, uint16_t len);
fee_status_t xy_fee_gc(xy_fee_t *fee);  // 垃圾回收
```

#### xy_fee_nano.h - Nano FEE

```c
/**
 * @brief Nano FEE (简化版)
 * @特点:
 * - 仅适配 32bit 颗粒度
 * - 代码精简 (~500 行)
 * - 适合资源受限场景
 * - 无磨损均衡
 */

typedef struct {
    uint32_t *flash_base;   // Flash 基地址 (32bit 对齐)
    uint16_t flash_size;    // Flash 大小
    uint16_t cache_size;    // 缓存大小
} xy_fee_nano_config_t;

fee_status_t xy_fee_nano_init(xy_fee_nano_t *fee, const xy_fee_nano_config_t *cfg);
fee_status_t xy_fee_nano_write(xy_fee_nano_t *fee, uint16_t addr, const void *data, uint16_t len);
fee_status_t xy_fee_nano_read(xy_fee_nano_t *fee, uint16_t addr, void *data, uint16_t len);
```

**对比**:

| 特性 | xy_fee (完整) | xy_fee_nano (简化) |
|------|-------------|------------------|
| **颗粒度** | 8/16/32/64/128 | 仅 32bit |
| **磨损均衡** | ✅ | ❌ |
| **代码大小** | ~1500 行 | ~500 行 |
| **RAM 占用** | 中 | 低 |
| **适用场景** | 通用 | 资源受限 |

---

### 2. eeprom/ - EEPROM 接口层

#### xy_eeprom.h - 统一 EEPROM 接口

```c
/**
 * @brief 统一 EEPROM 接口
 * @特点:
 * - 基于 FEE 实现
 * - 字节级读写
 * - 掉电保护
 */

typedef struct {
    xy_fee_t *fee;            // 底层 FEE
    uint8_t *cache;           // EEPROM 缓存
    uint16_t size;            // EEPROM 大小
} xy_eeprom_t;

// 初始化
xy_eeprom_result_t xy_eeprom_init(xy_eeprom_t *eep, xy_fee_t *fee, 
                                   uint8_t *cache, uint16_t size);

// 字节读写
xy_eeprom_result_t xy_eeprom_read(xy_eeprom_t *eep, uint16_t addr, 
                                   uint8_t *data, uint16_t len);
xy_eeprom_result_t xy_eeprom_write(xy_eeprom_t *eep, uint16_t addr, 
                                    const uint8_t *data, uint16_t len);

// 块读写 (优化性能)
xy_eeprom_result_t xy_eeprom_read_block(xy_eeprom_t *eep, uint16_t addr,
                                         uint8_t *data, uint16_t len);
xy_eeprom_result_t xy_eeprom_write_block(xy_eeprom_t *eep, uint16_t addr,
                                          const uint8_t *data, uint16_t len);
```

---

## 🔄 迁移计划

### 阶段 1: 创建新结构 (1 小时)

- [ ] 创建 `fee/inc/` 目录
- [ ] 创建 `fee/src/` 目录
- [ ] 创建 `eeprom/inc/` 目录
- [ ] 创建 `eeprom/src/` 目录

### 阶段 2: 整合代码 (2 小时)

- [ ] 移动 `fee.c` → `fee/src/xy_fee.c`
- [ ] 移动 `fee.h` → `fee/inc/xy_fee.h`
- [ ] 提取 `xy_eflash_v1` → `fee/src/xy_fee_nano.c`
- [ ] 创建 `eeprom/src/xy_eeprom.c`
- [ ] 创建 `eeprom/inc/xy_eeprom.h`

### 阶段 3: 删除旧目录 (30 分钟)

- [ ] 删除 `dm/EEPROM/`
- [ ] 删除 `dm/lwfee/`
- [ ] 删除 `dm/xy_eeprom/`
- [ ] 删除 `dm/xy_eflash_v1/`

### 阶段 4: 更新文档 (30 分钟)

- [ ] 更新 `dm/README.md`
- [ ] 创建 `fee/README.md`
- [ ] 创建 `eeprom/README.md`

---

## 📊 整合后对比

| 项目 | 整合前 | 整合后 | 改善 |
|------|--------|--------|------|
| **目录数** | 6 个 | 2 个 | -67% |
| **代码重复** | ~3000 行 | ~500 行 | -83% |
| **维护成本** | 高 | 低 | ✅ |
| **文档清晰度** | 混乱 | 清晰 | ✅ |

---

## 📚 使用说明

### 场景 1: 需要完整功能

```c
#include "xy_fee.h"

xy_fee_t fee;
xy_fee_config_t cfg = {
    .flash_base = (uint8_t *)0x08010000,
    .pages_per_fee = 4,
    .flash_page_size = 4096,
    .cache_size = 256,
    .write_granularity = 32,
    .max_erase_count = 10000,
};

xy_fee_init(&fee, &cfg);
xy_fee_write(&fee, 0, data, 10);
```

### 场景 2: 资源受限

```c
#include "xy_fee_nano.h"

xy_fee_nano_t fee;
xy_fee_nano_config_t cfg = {
    .flash_base = (uint32_t *)0x08010000,
    .flash_size = 8192,
    .cache_size = 128,
};

xy_fee_nano_init(&fee, &cfg);
xy_fee_nano_write(&fee, 0, data, 8);
```

### 场景 3: EEPROM 模拟

```c
#include "xy_eeprom.h"

xy_eeprom_t eep;
xy_eeprom_init(&eep, &fee, cache_buffer, 256);

xy_eeprom_write(&eep, 10, &value, 1);
xy_eeprom_read(&eep, 10, &value, 1);
```

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
