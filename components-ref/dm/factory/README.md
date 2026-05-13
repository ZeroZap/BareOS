# 工厂数据管理组件 (xy_factory)

TLV 格式存储 + 双份备份 + CRC16 校验，支持 Bootloader 和 App 共用

---

## 特性

- **TLV (Type-Length-Value) 格式**：紧凑的数据存储格式
- **双份备份模式**：Active + Backup，提高可靠性
- **CRC16 完整性校验**：确保数据有效，检测损坏
- **直接 Flash 底层操作**：无 Cache，确保掉电不丢失
- **写时双备份**：先写 Active，成功后再写 Backup
- **损坏检测与恢复**：启动时自动检测并修复
- **支持裸机（Bare-metal）和 RTOS**

---

## TLV 数据结构

### 启用 CRC16 校验（默认）

```
+------------+--------+--------+-------+-------+------------------+
| Magic(1B) | Type(1B)| Len(2B) | CRC_H | CRC_L | Data ...         |
+------------+--------+--------+-------+-------+------------------+
    0x7E       1-254    N        CRC     CRC      N bytes
```

**Entry 总大小** = `4 + N`（按 `XY_FACTORY_ALIGN_SIZE` 对齐）

**CRC16 校验范围** = `Magic + Type + Len + Data`

### 禁用 CRC16 校验

```
+------------+--------+--------+------------------+
| Magic(1B) | Type(1B)| Len(2B) | Data ...         |
+------------+--------+--------+------------------+
    0x7E       1-254    N        N bytes
```

**Entry 总大小** = `3 + N`（按 `XY_FACTORY_ALIGN_SIZE` 对齐）

### Magic 标记

| 值     | 含义                           |
| ------ | ------------------------------ |
| `0x7E` | 有效条目                       |
| `0x00` | 已删除（标记删除，不回收空间） |
| `0xFF` | 空（擦除态，数据结束）         |

### 类型范围

| 类型     | 范围   | 说明     |
| -------- | ------ | -------- |
| 有效类型 | 1-254  | 用户定义 |
| 保留类型 | 0, 255 | 系统保留 |

---

## 双份备份架构

```
+-------------------+     +-------------------+
|   Factory A       |     |   Factory B       |
|   (Active)       |     |   (Backup)       |
+-------------------+     +-------------------+
| Header (FACT)     |     | Header (FACT)     |
| Entry[0]: Type=1 |     | Entry[0]: Type=1 |
| Entry[1]: Type=2 |     | Entry[1]: Type=2 |
| ...               |     | ...               |
| Free Space        |     | Free Space        |
+-------------------+     +-------------------+
```

**写入流程**：

1. 计算 CRC16（`Magic + Type + Len + Data`）
2. 写入 Active Region (A)
3. 写入 Backup Region (B)
4. 如果任何一步失败，返回错误

**读取流程**：

1. 从 Active Region (A) 读取
2. 验证 CRC16
3. 如果 CRC 错误，从 Backup Region (B) 恢复 Active

---

## 编译配置

| 宏                      | 默认值 | 说明                      |
| ----------------------- | ------ | ------------------------- |
| `XY_FACTORY_DUAL_COPY`  | 1      | 启用双份备份（0=单区域）  |
| `XY_FACTORY_USE_CRC16`  | 1      | 启用 CRC16 校验（0=禁用） |
| `XY_FACTORY_SAFE_CHECK` | 1      | 启用安全检查              |
| `XY_FACTORY_ALIGN_SIZE` | 8      | Flash 写入对齐字节数      |
| `XY_FACTORY_MAX_TYPES`  | 32     | 最大数据类型数            |

---

## 快速开始

### 1. 定义 Flash 操作接口

```c
#include "xy_factory.h"

/* Flash 操作函数实现 */
static int flash_erase(uint32_t addr, uint32_t size)
{
    /* 实现 Flash 擦除 */
    return 0;
}

static int flash_write(uint32_t addr, const uint8_t *data, uint32_t size)
{
    /* 实现 Flash 写入（按 ALIGN_SIZE 对齐） */
    return 0;
}

static int flash_read(uint32_t addr, uint8_t *data, uint32_t size)
{
    /* 实现 Flash 读取 */
    memcpy(data, (void *)addr, size);
    return 0;
}

static const factory_flash_ops_t flash_ops = {
    .erase = flash_erase,
    .write = flash_write,
    .read  = flash_read
};
```

### 2. 配置和初始化

```c
/* 定义工厂数据区域 */
#define FACTORY_REGION_A_ADDR   0x0801F000
#define FACTORY_REGION_A_SIZE   1024
#define FACTORY_REGION_B_ADDR   0x0801F800  /* 双份模式需要 */
#define FACTORY_REGION_B_SIZE   1024

static factory_handle_t factory_handle;

void factory_system_init(void)
{
    factory_config_t config = {
        .flash_ops = &flash_ops,
        .region_a_addr = FACTORY_REGION_A_ADDR,
        .region_a_size = FACTORY_REGION_A_SIZE,
#if XY_FACTORY_DUAL_COPY
        .region_b_addr = FACTORY_REGION_B_ADDR,
        .region_b_size = FACTORY_REGION_B_SIZE,
#endif
    };

    factory_status_t status = factory_init(&factory_handle, &config);
    if (status != FACTORY_OK) {
        /* 错误处理 */
    }
}
```

### 3. 写入和读取数据

```c
/* 裸机调用方式 */
void save_device_id(void)
{
    uint8_t device_id[16] = {0x01, 0x02, 0x03, /* ... */};

    xy_enter_critical();
    factory_status_t status = factory_write(&factory_handle,
                                            FACTORY_TYPE_DEVICE_ID,
                                            device_id, sizeof(device_id));
    xy_exit_critical();
}

/* RTOS 调用方式 */
void save_device_id_rtos(void)
{
    uint8_t device_id[16] = {0x01, 0x02, 0x03, /* ... */};

    xy_os_mutex_lock(&factory_mutex);
    factory_write(&factory_handle, FACTORY_TYPE_DEVICE_ID,
                  device_id, sizeof(device_id));
    xy_os_mutex_unlock(&factory_mutex);
}

/* 读取数据 */
void load_device_id(void)
{
    uint8_t device_id[16];
    uint16_t len = sizeof(device_id);

    factory_status_t status = factory_read(&factory_handle,
                                          FACTORY_TYPE_DEVICE_ID,
                                          device_id, &len);
    if (status == FACTORY_OK) {
        /* 使用 device_id */
    }
}
```

---

## API 参考

### 初始化

```c
factory_status_t factory_init(factory_handle_t *handle,
                              const factory_config_t *config);
```

初始化工厂数据组件，扫描 Flash 建立索引，检测并修复损坏区域。

### 写入

```c
factory_status_t factory_write(factory_handle_t *handle, uint8_t type,
                               const uint8_t *data, uint16_t len);
```

写入数据（双份模式会同时写入 A 和 B）。

### 读取

```c
factory_status_t factory_read(factory_handle_t *handle, uint8_t type,
                              uint8_t *data, uint16_t *len);
```

读取数据，`*len` 输入缓冲区大小，输出实际长度。读取时会验证 CRC16，错误则返回 `FACTORY_ERROR_CRC`。

### 删除

```c
factory_status_t factory_delete(factory_handle_t *handle, uint8_t type);
```

删除数据（标记为已删除，不回收空间）。

### 检查存在

```c
bool factory_exists(factory_handle_t *handle, uint8_t type, uint16_t *len);
```

检查数据类型是否存在，可选输出数据长度。

### 枚举

```c
factory_status_t factory_enum(factory_handle_t *handle, uint8_t *types,
                              uint8_t max_count, uint8_t *count);
```

枚举所有已存储的数据类型。

### 格式化

```c
factory_status_t factory_format(factory_handle_t *handle);
```

擦除所有数据（擦除 A 和 B）。

### 获取信息

```c
factory_status_t factory_get_info(factory_handle_t *handle,
                                  uint32_t *used, uint32_t *free, uint8_t *count);
```

获取已使用空间、剩余空间、数据项数量。

### 验证与修复

```c
factory_status_t factory_verify_and_repair(factory_handle_t *handle);
```

重新扫描并修复损坏的区域（建议在启动时调用）。会验证每条 TLV 的 CRC16，错误则从备份恢复。

### CRC16 计算

```c
uint16_t factory_crc16(const uint8_t *data, uint16_t len);
uint16_t factory_crc16_update(uint16_t crc, const uint8_t *data, uint16_t len);
```

计算 CRC16-CCITT 校验值。`factory_crc16_update` 用于分步计算。

---

## 预定义数据类型

| 类型                        | 值  | 说明     |
| --------------------------- | --- | -------- |
| `FACTORY_TYPE_DEVICE_ID`    | 1   | 设备 ID  |
| `FACTORY_TYPE_CALIBRATION`  | 2   | 校准数据 |
| `FACTORY_TYPE_CONFIG`       | 3   | 配置参数 |
| `FACTORY_TYPE_SECURITY_KEY` | 4   | 安全密钥 |
| `FACTORY_TYPE_MANUFACTURE`  | 5   | 制造信息 |
| `FACTORY_TYPE_RESERVED`     | 254 | 保留     |
| `FACTORY_TYPE_END_MARK`     | 255 | 结束标记 |

---

## 调用约定

### 裸机环境

| 函数                        | 调用上下文 |
| --------------------------- | ---------- |
| `factory_init`              | 禁中断     |
| `factory_write`             | 禁中断     |
| `factory_read`              | 禁中断     |
| `factory_delete`            | 禁中断     |
| `factory_exists`            | 禁中断     |
| `factory_enum`              | 禁中断     |
| `factory_format`            | 禁中断     |
| `factory_get_info`          | 任意       |
| `factory_verify_and_repair` | 禁中断     |
| `factory_crc16`             | 任意       |

### RTOS 环境

| 函数                        | 调用上下文 |
| --------------------------- | ---------- |
| `factory_init`              | 持有互斥锁 |
| `factory_write`             | 持有互斥锁 |
| `factory_read`              | 持有互斥锁 |
| `factory_delete`            | 持有互斥锁 |
| `factory_exists`            | 持有互斥锁 |
| `factory_enum`              | 持有互斥锁 |
| `factory_format`            | 持有互斥锁 |
| `factory_get_info`          | 持有锁     |
| `factory_verify_and_repair` | 持有互斥锁 |
| `factory_crc16`             | 任意       |

---

## 典型应用场景

### 1. 设备参数存储

```c
typedef struct {
    uint32_t serial_number;
    uint16_t hw_version;
    uint16_t fw_version;
    uint8_t  mac_address[6];
} device_info_t;

void save_device_info(void)
{
    device_info_t info = {
        .serial_number = 0x12345678,
        .hw_version = 0x0100,
        .fw_version = 0x0200,
        .mac_address = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55}
    };

    factory_write(&handle, FACTORY_TYPE_DEVICE_ID,
                 (uint8_t *)&info, sizeof(info));
}
```

### 2. 校准数据存储

```c
typedef struct {
    float offset_adc1;
    float offset_adc2;
    float gain_adc1;
    float gain_adc2;
} calibration_data_t;

void save_calibration(void)
{
    calibration_data_t cal = {
        .offset_adc1 = 0.05f,
        .offset_adc2 = -0.03f,
        .gain_adc1 = 1.002f,
        .gain_adc2 = 0.998f
    };

    factory_write(&handle, FACTORY_TYPE_CALIBRATION,
                 (uint8_t *)&cal, sizeof(cal));
}
```

### 3. Bootloader 安全升级

```c
/* Bootloader 启动时验证 App 数据 */
void boot_verify_factory_data(void)
{
    factory_status_t status = factory_verify_and_repair(&handle);
    if (status != FACTORY_OK) {
        /* 数据损坏，从备份恢复或加载默认配置 */
        load_default_config();
    }
}
```

---

## 空间计算

```
总 Flash 需求 = region_a_size × (XY_FACTORY_DUAL_COPY ? 2 : 1)

示例：
- 单区域模式：region_a = 1024 字节 → 总需求 = 1024 字节
- 双份模式：region_a = 1024, region_b = 1024 → 总需求 = 2048 字节

每条 TLV Entry 占用：
- 启用 CRC16：4 + len（按 ALIGN_SIZE 对齐）
- 禁用 CRC16：3 + len（按 ALIGN_SIZE 对齐）
```

---

## 错误码

| 错误码                    | 说明           |
| ------------------------- | -------------- |
| `FACTORY_OK`              | 成功           |
| `FACTORY_ERROR`           | 通用错误       |
| `FACTORY_ERROR_PARAM`     | 参数错误       |
| `FACTORY_ERROR_NOT_INIT`  | 未初始化       |
| `FACTORY_ERROR_FLASH`     | Flash 操作失败 |
| `FACTORY_ERROR_NO_SPACE`  | 空间不足       |
| `FACTORY_ERROR_NOT_FOUND` | 数据不存在     |
| `FACTORY_ERROR_CRC`       | CRC 校验失败   |
| `FACTORY_ERROR_CORRUPT`   | 数据损坏       |

---

## 文件结构

```
factory/
├── xy_factory.h     # 头文件（API + 数据结构）
├── xy_factory.c     # 实现文件
└── README.md        # 本文档
```

---

## 版本信息

| 版本  | 日期       | 说明            |
| ----- | ---------- | --------------- |
| 1.1.0 | 2026-03-15 | 增加 CRC16 校验 |
| 1.0.0 | 2026-03-15 | 初始版本        |

---

## 许可证

MIT License
