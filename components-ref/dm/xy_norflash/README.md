# XinYi NOR Flash 驱动

这是一个完整的NOR Flash驱动程序，支持可配置的电气参数初始化。

## 功能特性

- ✅ 可配置的电气参数（时钟频率、驱动强度、摆率等）
- ✅ 支持标准SPI模式和四线（QSPI）模式
- ✅ 完整的读写擦除操作
- ✅ 写保护配置
- ✅ 低功耗模式支持
- ✅ 多厂商芯片识别
- ✅ 硬件抽象层设计，易于移植

## 文件结构

```
xy_norflash/
├── xy_nor.h           - 驱动头文件
├── xy_nor.c           - 驱动实现文件
├── xy_nor_port.c      - 硬件抽象层实现（需要适配）
└── README.md          - 本文档
```

## 使用方法

### 1. 初始化驱动

```c
#include "xy_nor.h"

xy_nor_handle_t nor_handle;
xy_nor_config_t config;

// 获取默认配置
xy_nor_get_default_config(&config);

// 自定义配置（可选）
config.clock_freq = 50000000;        // 50MHz
config.dummy_cycles = 8;
config.drive_strength = 5;           // 驱动强度0-7
config.slew_rate = 2;                // 摆率0-3
config.quad_enable = true;           // 使能四线模式
config.write_protection = false;     // 禁用写保护
config.timeout_ms = 5000;            // 超时5秒
config.voltage_range = 1;            // 3.3V

// 初始化
xy_nor_status_t status = xy_nor_init(&nor_handle, &config);
if (status != XY_NOR_OK) {
    // 初始化失败处理
}
```

### 2. 读取设备信息

```c
xy_nor_info_t info;
xy_nor_get_info(&nor_handle, &info);

printf("Manufacturer: %s\n", info.manufacturer);
printf("Model: %s\n", info.model);
printf("JEDEC ID: 0x%06X\n", info.jedec_id);
printf("Capacity: %d bytes\n", info.capacity);
printf("Page Size: %d bytes\n", info.page_size);
printf("Sector Size: %d bytes\n", info.sector_size);
printf("Block Size: %d bytes\n", info.block_size);
```

### 3. 读取数据

```c
uint8_t buffer[256];
uint32_t address = 0x1000;

// 标准读取
status = xy_nor_read(&nor_handle, address, buffer, sizeof(buffer));

// 快速读取
status = xy_nor_fast_read(&nor_handle, address, buffer, sizeof(buffer));

// 四线模式读取（需要先使能quad_enable）
status = xy_nor_quad_read(&nor_handle, address, buffer, sizeof(buffer));
```

### 4. 写入数据

```c
uint8_t data[256] = {0x01, 0x02, 0x03, ...};
uint32_t address = 0x1000;

// 注意：写入前需要先擦除对应的扇区
status = xy_nor_sector_erase(&nor_handle, address);
if (status != XY_NOR_OK) {
    // 错误处理
}

// 页编程（最多256字节）
status = xy_nor_page_program(&nor_handle, address, data, sizeof(data));
if (status != XY_NOR_OK) {
    // 错误处理
}
```

### 5. 擦除操作

```c
// 扇区擦除（4KB）
status = xy_nor_sector_erase(&nor_handle, 0x1000);

// 块擦除（64KB）
status = xy_nor_block_erase(&nor_handle, 0x10000);

// 整片擦除
status = xy_nor_chip_erase(&nor_handle);
```

### 6. 低功耗模式

```c
// 进入掉电模式
xy_nor_power_down(&nor_handle);

// 退出掉电模式
xy_nor_release_power_down(&nor_handle);
```

### 7. 去初始化

```c
xy_nor_deinit(&nor_handle);
```

## 电气参数说明

### 时钟频率（clock_freq）
- 范围：通常1MHz - 133MHz
- 默认：25MHz
- 说明：SPI时钟频率，过高可能导致信号完整性问题

### 虚拟周期（dummy_cycles）
- 范围：0-15
- 默认：8
- 说明：快速读取时的虚拟时钟周期数，补偿传输延迟

### 驱动强度（drive_strength）
- 范围：0-7（0最弱，7最强）
- 默认：4（中等强度）
- 说明：控制GPIO输出驱动能力，影响信号边沿和EMI

### 摆率（slew_rate）
- 范围：0-3（0最慢，3最快）
- 默认：2（中等速度）
- 说明：控制信号边沿速率，慢速率可降低EMI

### 电压范围（voltage_range）
- 0: 1.8V
- 1: 3.3V（默认）
- 说明：工作电压范围，需与芯片规格匹配

## 硬件适配

需要在 `xy_nor_port.c` 中实现以下函数：

1. **xy_nor_hw_init()** - 初始化SPI接口和GPIO
2. **xy_nor_hw_deinit()** - 释放硬件资源
3. **xy_nor_hw_command()** - 发送SPI命令和数据
4. **xy_nor_hw_delay_ms()** - 毫秒级延时

示例代码已在文件中提供，请根据您的硬件平台修改。

## 支持的芯片厂商

- Winbond (华邦)
- Macronix (旺宏)
- Micron (美光)
- Spansion (飞索)
- SST
- 其他兼容JEDEC标准的NOR Flash

## 注意事项

1. **写入前必须擦除**：NOR Flash的特性决定写入前必须先擦除对应区域
2. **页边界对齐**：页编程不能跨页，单次最多256字节
3. **扇区/块对齐**：擦除操作的地址必须对齐到扇区/块边界
4. **电压匹配**：确保配置的电压范围与芯片规格匹配
5. **时序要求**：操作后需等待芯片就绪（驱动已自动处理）
6. **四线模式**：不同厂商的QE位配置方式不同，需根据datasheet调整

## 错误处理

函数返回值类型：`xy_nor_status_t`

- `XY_NOR_OK` (0) - 操作成功
- `XY_NOR_ERROR` (1) - 一般错误
- `XY_NOR_BUSY` (2) - 设备忙
- `XY_NOR_TIMEOUT` (3) - 操作超时
- `XY_NOR_INVALID_PARAM` (4) - 无效参数

## 性能优化建议

1. 使用四线模式可提高读取速度约4倍
2. 快速读取比标准读取快30-50%
3. 适当提高时钟频率（注意信号完整性）
4. 批量操作时注意页边界对齐
5. 不使用时进入低功耗模式

## 示例代码

完整的使用示例请参考上述"使用方法"章节。

## 许可证

请根据您的项目需求添加相应的许可证信息。
