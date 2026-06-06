# 固定长度 EEPROM 模拟组件

`xy_eeprom` 是参考 `components-ref/EEPROM-Nano` 实现的固定长度 EEPROM 模拟组件。

它适合保存：

- 计数器
- flags
- 小状态字
- 固定长度参数
- `uint16_t` / `uint32_t` / `uint64_t` 小变量

不适合保存：

- 变长字符串
- 大结构体
- 大块 blob 数据
- 高频日志流

## 核心思想

EEPROM 组件把 Flash 模拟成一个固定长度数组：

```text
index -> fixed-size value
```

每个 EEPROM 句柄有自己的：

- Flash 区域
- page 数量
- value_size
- RAM shadow
- valid bitmap

读取时直接从 RAM shadow 取值，因此读取很快。

写入时追加一条 record 到 Flash。

## 多实例

支持多个 EEPROM 句柄，每个句柄绑定不同 Flash 区域：

```c
xy_eeprom_t eep_u16;
xy_eeprom_t eep_u32;
xy_eeprom_t eep_blob16;
```

例如：

```text
EEPROM A: page 56-57, value_size=2
EEPROM B: page 58-59, value_size=4
EEPROM C: page 60-63, value_size=16
```

每个句柄互相独立。

## Flash Page 要求

每个 EEPROM 实例需要 `2*n` 个 page：

```text
page_count >= 2
page_count 必须为偶数
```

当前实现按 page 环形轮换。

## Record 格式

每条记录：

```text
uint16_t index
uint16_t crc16
value[value_size]
padding 到 write_unit 对齐
```

实际大小：

```text
record_size = align_up(2 + 2 + value_size, write_unit)
```

示例：

```text
value_size=2, write_unit=4  -> 8 bytes
value_size=4, write_unit=4  -> 8 bytes
value_size=8, write_unit=4  -> 12 bytes
value_size=16, write_unit=4 -> 20 bytes
```

## CRC16

默认启用 CRC16：

```c
#define XY_EEPROM_ENABLE_CRC16 1
```

CRC 覆盖：

```text
index + value
```

启动扫描时，CRC 错误的记录不会进入 RAM shadow。

## MCU 最小写入单元适配

不同 MCU 的 Flash 最小写入单元不同：

```text
32-bit  -> 4 bytes
64-bit  -> 8 bytes
128-bit -> 16 bytes
256-bit -> 32 bytes
```

组件通过宏控制资源上限：

```c
#define XY_EEPROM_MAX_WRITE_UNIT  32u
#define XY_EEPROM_MAX_RECORD_SIZE 128u
#define XY_EEPROM_ENABLE_CRC16    1u
```

工程可以按 MCU 覆盖这些宏。

例如 32-bit 写入 MCU：

```make
DEFS += -DXY_EEPROM_MAX_WRITE_UNIT=4
DEFS += -DXY_EEPROM_MAX_RECORD_SIZE=16
DEFS += -DXY_EEPROM_ENABLE_CRC16=1
```

64-bit 写入 MCU：

```make
DEFS += -DXY_EEPROM_MAX_WRITE_UNIT=8
DEFS += -DXY_EEPROM_MAX_RECORD_SIZE=32
```

128-bit 写入 MCU：

```make
DEFS += -DXY_EEPROM_MAX_WRITE_UNIT=16
DEFS += -DXY_EEPROM_MAX_RECORD_SIZE=64
```

256-bit 写入 MCU：

```make
DEFS += -DXY_EEPROM_MAX_WRITE_UNIT=32
DEFS += -DXY_EEPROM_MAX_RECORD_SIZE=128
```

## 初始化

调用者需要提供 RAM shadow 和 valid bitmap：

```c
#define ITEM_COUNT 16

static xy_eeprom_t eep;
static uint32_t shadow[ITEM_COUNT];
static uint8_t valid[XY_EEPROM_BITMAP_SIZE(ITEM_COUNT)];

static const xy_eeprom_config_t cfg = {
    .page_size = 2048,
    .page_count = 2,
    .item_count = ITEM_COUNT,
    .value_size = sizeof(uint32_t),
    .write_unit = 4,
};

xy_eeprom_init(&eep, &cfg, &ops, ctx,
               (uint8_t *)shadow, valid, sizeof(valid));
```

## 读写

```c
uint32_t value;

xy_eeprom_read(&eep, 0, &value);

value++;
xy_eeprom_write(&eep, 0, &value);
```

## Page Transfer

当当前 page 写满：

1. 擦除下一个 page。
2. 写 page header 为 `TRANSFER`。
3. 把 RAM shadow 中所有有效 index 搬迁到新 page。
4. 写 page header 为 `VALID`。
5. 擦除旧 page。

## RAM 占用

EEPROM 组件使用 RAM shadow：

```text
RAM = item_count * value_size + bitmap + handle
```

例如 16 个 `uint32_t`：

```text
16 * 4 = 64 bytes shadow
bitmap = 2 bytes
```

## Flash 占用

Flash 占用取决于 record_size。

例如 N32 32-bit 写入，`value_size=4`：

```text
record_size = 8 bytes
2048 byte page 可写约 255 条记录
```

## PLB-N32 示例

PLB-N32 当前使用倒数第 4~3 页作为 EEPROM：

```text
EEPROM base: 0x0801E000
EEPROM size: 0x1000
Page 60: 0x0801E000
Page 61: 0x0801E800
```

用于保存：

```text
index 0 -> boot_count, uint32_t
```

PLB-N32 当前宏配置：

```make
DEFS += -DXY_EEPROM_MAX_WRITE_UNIT=4
DEFS += -DXY_EEPROM_MAX_RECORD_SIZE=16
DEFS += -DXY_EEPROM_ENABLE_CRC16=1
```

## 与 FEE 的选择

| 组件 | 数据模型 | RAM | Flash | 适合 |
|---|---|---:|---:|---|
| EEPROM | index -> 定长 value | 较高 | 低 | 小变量、计数器 |
| FEE | key -> 变长 value | 低 | 较高 | 配置、字符串、结构体 |
