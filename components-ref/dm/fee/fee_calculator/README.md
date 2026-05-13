XinYi\components\dm\fee\fee_calculator\README.md

````

```markdown
# FEE 配置计算工具 v2.2

Flash EEPROM Emulation 配置计算和代码生成工具

---

## 更新日志

### v2.2 (2026-03-15) - 多实例支持 + RTOS 适配

- ✅ **多实例支持** — 每个组件可拥有独立的 FEE 实例
- ✅ **RTOS/裸机宏控制** — `XY_OS_BACKEND_*` 自动适配
- ✅ **实例级 work_buffer/flash_ops** — 封装到 `fee_handle_t` 中
- ✅ **调用约定速查表** — 明确裸机和 RTOS 的调用上下文
- ✅ **生成符合 xy_fee.h v2.0.0 的代码**

### v2.1 (2024-01-31)

- 强制 FEE 颗粒度 >= 8 字节验证
- 区分 Flash 原生颗粒度和 FEE 颗粒度
- 增强数据库验证功能

### v2.0 (2024-01-30)

- JSON 数据库支持
- 可视化 MCU 配置管理

---

## 核心概念

### 多实例架构

````

+------------------+ +------------------+ +------------------+
| net_fee 实例 | | param_fee 实例 | | app_fee 实例 |
+------------------+ +------------------+ +------------------+
| net_virtual_eeprom | | param_virtual_eeprom| | app_virtual_eeprom|
| net_work_buffer | | param_work_buffer | | app_work_buffer |
+------------------+ +------------------+ +------------------+

````

每个实例独立管理，互不影响。适用于：
- 网络组件使用独立池存储配置
- 参数存储使用独立池
- 不同应用场景使用不同的池

### RTOS/裸机适配

```c
/* 平台自动检测 */
#if XY_OS_BACKEND_BAREMETAL
    #define FEE_ENTER_CRITICAL()    do { } while (0)  /* 调用者保证禁中断 */
#elif XY_OS_BACKEND_FREERTOS
    #define FEE_ENTER_CRITICAL()    taskENTER_CRITICAL()
#elif XY_OS_BACKEND_RTTHREAD
    #define FEE_ENTER_CRITICAL()    rt_enter_critical()
#endif
````

---

## 设计约束

### 为什么颗粒度必须 >= 8 字节？

```
Record 结构:
┌──────────────────────┐
│ Record Header (4B) │ ← 固定 4 字节
│  ├─ addr (2B)        │
│  └─ crc (2B)         │
├──────────────────────┤
│ Data (gran - 4B)     │ ← 有效数据
└──────────────────────┘

如果 gran < 8:
  gran=4: Data=0B  ✗ 没有数据空间
  gran=6: Data=2B  ✗ 数据太少
  gran=8: Data=4B  ✓ 最小合理配置
```

### 2-Page 架构

```
┌─────────────────────────────────┐
│         FEE Page 0              │
│  ┌─────────────────────────┐   │
│  │ Header (FEEA)            │   │
│  ├─────────────────────────┤   │
│  │ Record 0                 │   │
│  │ Record 1                 │   │
│  │ ...                       │   │
│  │ Record N                 │   │
│  └─────────────────────────┘   │
├─────────────────────────────────┤
│         FEE Page 1 (ERASED)     │
└─────────────────────────────────┘

GC 时: Page 0 → RECEIVING → INVALID
      Page 1 → ACTIVE
```

---

## 快速开始

### 1. 选择 MCU

从下拉列表选择 MCU 型号，或选择"自定义"手动输入参数。

### 2. 配置参数

| 参数                  | 说明                            |
| --------------------- | ------------------------------- |
| Flash Page Size       | MCU Flash 页大小（字节）        |
| FEE Write Granularity | 写入颗粒度（必须 >= 8）         |
| 虚拟 EEPROM 大小      | Cache 大小（字节）              |
| Pages/FEE             | 每个 FEE Page 包含的 Flash 页数 |
| 最大擦除次数          | Flash 寿命上限                  |

### 3. 计算并生成代码

点击"计算配置"查看结果，点击"复制代码"或"保存代码"获取 C 代码。

---

## API 调用约定

### 裸机环境

| 函数           | 调用上下文 | 说明                 |
| -------------- | ---------- | -------------------- |
| `fee_init`     | 禁中断     | 系统初始化时调用     |
| `fee_write`    | 禁中断     | 写入数据             |
| `fee_read`     | 任意       | 读 Cache，已内置保护 |
| `fee_gc`       | 禁中断     | 垃圾回收             |
| `fee_format`   | 禁中断     | 格式化               |
| `fee_get_info` | 任意       | 已内置保护           |

### RTOS 环境

| 函数           | 调用上下文     | 说明             |
| -------------- | -------------- | ---------------- |
| `fee_init`     | 持有互斥锁     | 系统初始化时调用 |
| `fee_write`    | 持有互斥锁     | 写入数据         |
| `fee_read`     | 持有锁或禁中断 | 建议持有锁       |
| `fee_gc`       | 持有互斥锁     | 垃圾回收         |
| `fee_format`   | 持有互斥锁     | 格式化           |
| `fee_get_info` | 持有锁或禁中断 | 建议持有锁       |

---

## 生成代码示例

```c
/* ============================================================
 * 多实例配置
 * ============================================================ */

/* 网络组件 FEE 实例 */
static uint8_t net_virtual_eeprom[512];
static uint8_t net_work_buffer[FEE_WORK_SIZE(8)];  /* 16 bytes */
static fee_handle_t net_fee;

/* 参数存储 FEE 实例（可选） */
// static uint8_t param_virtual_eeprom[256];
// static uint8_t param_work_buffer[FEE_WORK_SIZE(8)];
// static fee_handle_t param_fee;

/* ============================================================
 * Flash 操作接口
 * ============================================================ */

static const fee_flash_ops_t fee_flash_ops = {
    .erase = fee_flash_erase,
    .write = fee_flash_write,
    .read  = fee_flash_read
};

/* ============================================================
 * 初始化
 * ============================================================ */

int fee_system_init(void)
{
    fee_status_t status;

    /* 初始化网络组件 FEE */
    status = fee_init(&net_fee, &net_fee_config,
                      net_virtual_eeprom, net_work_buffer);
    if (status != FEE_OK) {
        return -1;
    }

    /* 可选: 初始化其他实例 */
    // status = fee_init(&param_fee, &param_fee_config, ...);

    return 0;
}
```

---

## 使用示例

### 裸机环境

```c
/* 禁中断状态下写入 */
void save_config(void)
{
    struct config cfg = { .baudrate = 115200, .timeout = 1000 };

    xy_enter_critical();
    fee_write(&net_fee, 0, (uint8_t *)&cfg, sizeof(cfg));
    xy_exit_critical();
}

/* 读取（已内置保护，上下文灵活） */
void load_config(void)
{
    struct config cfg;
    fee_read(&net_fee, 0, (uint8_t *)&cfg, sizeof(cfg));
}
```

### RTOS 环境

```c
/* 持有锁状态下写入 */
void save_config(void)
{
    struct config cfg = { .baudrate = 115200, .timeout = 1000 };

    xy_os_mutex_lock(&fee_mutex);
    fee_write(&net_fee, 0, (uint8_t *)&cfg, sizeof(cfg));
    xy_os_mutex_unlock(&fee_mutex);
}
```

---

## 文件说明

```
fee_calculator/
├── fee_calculator.py      # 主程序 (v2.2)
├── mcu_database.json       # MCU 配置数据库
└── README.md             # 本文档

更新:
  fee_calculator.py (v2.2) - 支持多实例和RTOS适配
  mcu_database.json (v2.2) - 数据库格式更新
```

---

## 许可证

MIT License

---

## 贡献

欢迎提交 Issue 和 Pull Request！
