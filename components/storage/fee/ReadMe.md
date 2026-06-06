# Flash EEPROM Emulation KV 存储（FEE）

`xy_fee_nano` 是一个面向裸机 MCU 的 Flash EEPROM Emulation 组件，用于在 Flash 上保存少量 `key -> value` 类型的持久化数据。

它适合保存：

- 设备参数
- IP/端口
- 服务器地址
- 校准参数
- 小型结构体
- 低频更新的配置项

不适合保存：

- 高频日志
- 大量小计数器
- 持续追加的事件流水
- 大文件或长数据流

## 基本模型

FEE 使用追加写 record 的方式保存数据。

每次写入：

```c
eflash_write(fee, key, data, len);
```

都会追加一条新记录。读取时：

```c
eflash_read(fee, key, data, len);
```

会扫描 Flash，找到该 `key` 最新的记录。

## Record 格式

当前 record header 为 20 字节：

```c
typedef struct {
    uint32_t magic;
    uint32_t key;
    uint16_t len;
    uint16_t total;
    uint32_t seq;
    uint32_t crc;
} fee_record_hdr_t;
```

实际占用：

```text
20 字节 header + value 长度 + 4 字节对齐 padding
```

示例：

```text
4 字节 value   -> 24 字节
16 字节 value  -> 36 字节
32 字节 value  -> 52 字节
128 字节 value -> 148 字节
```

## 两页 GC

FEE 至少需要 2 个 Flash page。

当当前 page 写满时：

1. 擦除另一个 page。
2. 搬迁每个 key 的最新记录。
3. 追加当前新记录。
4. 擦除旧 page。

这样可以避免原地覆盖 Flash。

## 初始化方式

### RAM 模拟模式

用于 PC 或测试场景：

```c
static eflash_t fee;
static uint8_t flash_buf[4096];
static bool page_erased[2];

eflash_config_t cfg = {
    .total_size = sizeof(flash_buf),
    .page_size = 2048,
    .page_count = 2,
    .write_unit = EFLASH_WRITE_UNIT_32BIT,
    .auto_erase = false,
};

eflash_init_with_buffer(&fee, &cfg, flash_buf, page_erased);
```

### 硬件 Flash 模式

用于 MCU 片上 Flash 或外部 NOR Flash：

```c
static eflash_t fee;
static bool page_erased[2];

static const eflash_ops_t ops = {
    flash_read,
    flash_write,
    flash_erase_page,
};

eflash_config_t cfg = {
    .total_size = 4096,
    .page_size = 2048,
    .page_count = 2,
    .write_unit = EFLASH_WRITE_UNIT_32BIT,
    .auto_erase = false,
};

eflash_init_with_ops(&fee, &cfg, (uint8_t *)FLASH_BASE, page_erased, &ops, ctx);
```

硬件 backend 需要保证：

- `read()` 可以从指定 offset 读取。
- `write()` 按 MCU 最小写入单元写入。
- `erase_page()` 擦除指定 page。
- 写入前目标区域必须已经是擦除态。

## API

```c
eflash_result_t eflash_read(eflash_handle_t *handle,
                            uint32_t key,
                            uint8_t *data,
                            size_t size);

eflash_result_t eflash_write(eflash_handle_t *handle,
                             uint32_t key,
                             const uint8_t *data,
                             size_t size);

eflash_result_t eflash_erase_all(eflash_handle_t *handle);
```

在硬件 FEE 模式下，`address` 参数作为逻辑 `key` 使用。

## Key 规划

建议使用 namespace：

```c
#define FEE_NS_SYS      0x00010000u
#define FEE_NS_NET      0x00020000u
#define FEE_NS_PARAM    0x00030000u

#define FEE_KEY_BOOT    (FEE_NS_SYS | 0x0001u)
#define FEE_KEY_SERVER  (FEE_NS_NET | 0x0001u)
```

## 适用建议

适合用 FEE：

- `server_ip + port`
- 设备序列号
- 配置结构体
- 校准参数
- 低频更新状态

不建议用 FEE：

- 高频 `boot_count`
- 高频事件日志
- 每秒写入的数据

这类小定长高频数据建议使用 `components/storage/eeprom`。

## 与 EEPROM 组件的区别

| 组件 | 数据模型 | RAM 占用 | Flash 占用 | 适合场景 |
|---|---|---:|---:|---|
| FEE | key -> 变长 value | 低 | 较高 | 配置/结构体/字符串 |
| EEPROM | index -> 定长 value | 较高 | 低 | 小变量/计数器/状态字 |

## PLB-N32 示例

PLB-N32 当前使用最后 2 个 page 作为 FEE：

```text
FEE base: 0x0801F000
FEE size: 0x1000
Page 62: 0x0801F000
Page 63: 0x0801F800
```

保存服务器地址：

```c
typedef struct {
    uint8_t ip[4];
    uint16_t port;
    uint16_t reserved;
} plb_n32_server_endpoint_t;
```
