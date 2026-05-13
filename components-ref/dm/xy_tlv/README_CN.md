# XY TLV 管理系统

## 概述

XY TLV (Type-Length-Value) 管理系统为嵌入式系统提供高效的二进制数据编码和解码功能。它专为资源受限环境设计，零动态内存分配，具有全面的错误检查。

## 特性

- ✅ **零动态内存分配** - 仅使用提供的缓冲区
- ✅ **类型安全 API** - 每种数据类型都有专用函数
- ✅ **网络字节序** - 大端序编码，跨平台兼容
- ✅ **嵌套容器** - 支持分层数据结构
- ✅ **迭代器模式** - 高效遍历，无需拷贝
- ✅ **数据验证** - 全面的边界和完整性检查
- ✅ **统计信息** - 内置性能监控
- ✅ **可扩展性** - 轻松添加自定义类型

## TLV 格式

每个 TLV 元素由以下部分组成：

```
+--------+--------+--------+--------+------------------+
|  类型  |  类型  |  长度  |  长度  |      值          |
| (高字节)| (低字节)| (高字节)| (低字节)|   (0-65535字节)  |
+--------+--------+--------+--------+------------------+
   0        1        2        3         4 ... N
```

- **类型（Type）**: 2 字节 (uint16_t) - 标识数据类型
- **长度（Length）**: 2 字节 (uint16_t) - 值的有效载荷长度 (0-65535)
- **值（Value）**: 可变长度 - 实际数据有效载荷

## 快速入门

### 基本编码

```c
#include "xy_tlv.h"

uint8_t buffer[256];
xy_tlv_buffer_t tlv_buf;

/* 初始化缓冲区 */
xy_tlv_buffer_init(&tlv_buf, buffer, sizeof(buffer));

/* 编码不同类型的数据 */
xy_tlv_encode_uint32(&tlv_buf, 0x1001, 12345);
xy_tlv_encode_string(&tlv_buf, 0x1002, "你好");
xy_tlv_encode_bool(&tlv_buf, 0x1003, true);

/* 获取编码后的大小 */
uint16_t size = xy_tlv_buffer_get_used(&tlv_buf);
```

### 基本解码

```c
xy_tlv_iterator_t iter;
xy_tlv_t tlv;

/* 初始化迭代器 */
xy_tlv_iterator_init(&iter, buffer, size);

/* 遍历所有 TLV */
while (xy_tlv_iterator_next(&iter, &tlv) == XY_TLV_OK) {
    switch (tlv.type) {
        case 0x1001: {
            uint32_t value;
            xy_tlv_decode_uint32(&tlv, &value);
            printf("UINT32: %u\n", value);
            break;
        }
        case 0x1002: {
            char str[64];
            xy_tlv_decode_string(&tlv, str, sizeof(str));
            printf("字符串: %s\n", str);
            break;
        }
        case 0x1003: {
            bool value;
            xy_tlv_decode_bool(&tlv, &value);
            printf("布尔值: %d\n", value);
            break;
        }
    }
}
```

## API 参考

### 缓冲区管理

#### `xy_tlv_buffer_init`
初始化 TLV 缓冲区用于编码。

```c
int xy_tlv_buffer_init(xy_tlv_buffer_t *tlv_buf, uint8_t *buffer, uint16_t capacity);
```

#### `xy_tlv_buffer_reset`
重置缓冲区到初始状态（清除所有数据）。

```c
int xy_tlv_buffer_reset(xy_tlv_buffer_t *tlv_buf);
```

#### `xy_tlv_buffer_get_used`
获取缓冲区当前使用的字节数。

```c
uint16_t xy_tlv_buffer_get_used(const xy_tlv_buffer_t *tlv_buf);
```

#### `xy_tlv_buffer_get_free`
获取缓冲区剩余的可用空间。

```c
uint16_t xy_tlv_buffer_get_free(const xy_tlv_buffer_t *tlv_buf);
```

### 编码函数

#### 通用编码
```c
int xy_tlv_encode(xy_tlv_buffer_t *tlv_buf, uint16_t type,
                  const void *value, uint16_t length);
```

#### 类型专用编码
```c
int xy_tlv_encode_uint8(xy_tlv_buffer_t *tlv_buf, uint16_t type, uint8_t value);
int xy_tlv_encode_uint16(xy_tlv_buffer_t *tlv_buf, uint16_t type, uint16_t value);
int xy_tlv_encode_uint32(xy_tlv_buffer_t *tlv_buf, uint16_t type, uint32_t value);
int xy_tlv_encode_uint64(xy_tlv_buffer_t *tlv_buf, uint16_t type, uint64_t value);
int xy_tlv_encode_int8(xy_tlv_buffer_t *tlv_buf, uint16_t type, int8_t value);
int xy_tlv_encode_int16(xy_tlv_buffer_t *tlv_buf, uint16_t type, int16_t value);
int xy_tlv_encode_int32(xy_tlv_buffer_t *tlv_buf, uint16_t type, int32_t value);
int xy_tlv_encode_int64(xy_tlv_buffer_t *tlv_buf, uint16_t type, int64_t value);
int xy_tlv_encode_bool(xy_tlv_buffer_t *tlv_buf, uint16_t type, bool value);
int xy_tlv_encode_string(xy_tlv_buffer_t *tlv_buf, uint16_t type, const char *str);
int xy_tlv_encode_bytes(xy_tlv_buffer_t *tlv_buf, uint16_t type,
                        const uint8_t *bytes, uint16_t length);
```

### 解码函数

#### 迭代器管理
```c
int xy_tlv_iterator_init(xy_tlv_iterator_t *iter, const uint8_t *buffer,
                         uint16_t buffer_len);
int xy_tlv_iterator_next(xy_tlv_iterator_t *iter, xy_tlv_t *tlv);
bool xy_tlv_iterator_has_next(const xy_tlv_iterator_t *iter);
int xy_tlv_iterator_reset(xy_tlv_iterator_t *iter);
```

#### 类型专用解码
```c
int xy_tlv_decode_uint8(const xy_tlv_t *tlv, uint8_t *value);
int xy_tlv_decode_uint16(const xy_tlv_t *tlv, uint16_t *value);
int xy_tlv_decode_uint32(const xy_tlv_t *tlv, uint32_t *value);
int xy_tlv_decode_uint64(const xy_tlv_t *tlv, uint64_t *value);
int xy_tlv_decode_int8(const xy_tlv_t *tlv, int8_t *value);
int xy_tlv_decode_int16(const xy_tlv_t *tlv, int16_t *value);
int xy_tlv_decode_int32(const xy_tlv_t *tlv, int32_t *value);
int xy_tlv_decode_int64(const xy_tlv_t *tlv, int64_t *value);
int xy_tlv_decode_bool(const xy_tlv_t *tlv, bool *value);
int xy_tlv_decode_string(const xy_tlv_t *tlv, char *str, uint16_t str_len);
int xy_tlv_decode_bytes(const xy_tlv_t *tlv, uint8_t *bytes, uint16_t *bytes_len);
```

### 高级功能

#### 搜索功能
```c
/* 查找指定类型的第一个 TLV */
int xy_tlv_find(const uint8_t *buffer, uint16_t buffer_len,
                uint16_t type, xy_tlv_t *tlv);

/* 查找指定类型的所有 TLV */
int xy_tlv_find_all(const uint8_t *buffer, uint16_t buffer_len,
                    uint16_t type, xy_tlv_t *tlv_array, uint16_t *array_size);

/* 统计缓冲区中的 TLV 总数 */
int xy_tlv_count(const uint8_t *buffer, uint16_t buffer_len);
```

#### 容器（嵌套 TLV）
```c
/* 开始编码容器 */
int xy_tlv_container_begin(xy_tlv_buffer_t *tlv_buf, uint16_t type);

/* 结束编码容器 */
int xy_tlv_container_end(xy_tlv_buffer_t *tlv_buf);

/* 进入容器进行迭代 */
int xy_tlv_container_enter(const xy_tlv_iterator_t *iter,
                           const xy_tlv_t *tlv,
                           xy_tlv_iterator_t *child_iter);
```

#### 验证与工具
```c
/* 验证 TLV 缓冲区结构 */
int xy_tlv_validate(const uint8_t *buffer, uint16_t buffer_len);

/* 计算校验和（CRC16）*/
uint16_t xy_tlv_checksum(const uint8_t *buffer, uint16_t buffer_len);

/* 获取类型名称（用于调试）*/
const char *xy_tlv_get_type_name(uint16_t type);

/* 获取错误消息字符串 */
const char *xy_tlv_get_error_string(int error_code);

/* 获取统计信息 */
int xy_tlv_get_stats(xy_tlv_stats_t *stats);
void xy_tlv_reset_stats(void);
```

## 预定义类型

### 基本类型 (0x0001-0x00FF)
- `XY_TLV_TYPE_UINT8` (0x0001)
- `XY_TLV_TYPE_UINT16` (0x0002)
- `XY_TLV_TYPE_UINT32` (0x0003)
- `XY_TLV_TYPE_UINT64` (0x0004)
- `XY_TLV_TYPE_INT8` (0x0005)
- `XY_TLV_TYPE_INT16` (0x0006)
- `XY_TLV_TYPE_INT32` (0x0007)
- `XY_TLV_TYPE_INT64` (0x0008)
- `XY_TLV_TYPE_FLOAT` (0x0009)
- `XY_TLV_TYPE_DOUBLE` (0x000A)
- `XY_TLV_TYPE_BOOL` (0x000B)

### 字符串和二进制类型 (0x0100-0x01FF)
- `XY_TLV_TYPE_STRING` (0x0101) - NULL 结尾的字符串
- `XY_TLV_TYPE_BYTES` (0x0102) - 原始二进制数据
- `XY_TLV_TYPE_BLOB` (0x0103) - 二进制大对象

### 容器类型 (0x0200-0x02FF)
- `XY_TLV_TYPE_CONTAINER` (0x0201) - 嵌套 TLV 容器
- `XY_TLV_TYPE_ARRAY` (0x0202) - TLV 数组
- `XY_TLV_TYPE_LIST` (0x0203) - 链表

### 特殊类型 (0x0300-0x03FF)
- `XY_TLV_TYPE_TIMESTAMP` (0x0301) - Unix 时间戳
- `XY_TLV_TYPE_UUID` (0x0302) - UUID（16 字节）
- `XY_TLV_TYPE_MAC_ADDR` (0x0303) - MAC 地址（6 字节）
- `XY_TLV_TYPE_IPV4_ADDR` (0x0304) - IPv4 地址（4 字节）
- `XY_TLV_TYPE_IPV6_ADDR` (0x0305) - IPv6 地址（16 字节）
- `XY_TLV_TYPE_CHECKSUM` (0x0306) - 校验和/CRC 值

### 用户自定义类型 (0x1000-0xFFFF)
从 `XY_TLV_TYPE_USER_BASE` (0x1000) 开始定义自定义类型。

## 返回码

- `XY_TLV_OK` (0) - 成功
- `XY_TLV_ERROR` (-1) - 一般错误
- `XY_TLV_INVALID_PARAM` (-2) - 无效参数
- `XY_TLV_BUFFER_OVERFLOW` (-3) - 缓冲区太小
- `XY_TLV_BUFFER_UNDERFLOW` (-4) - 数据不足
- `XY_TLV_TYPE_MISMATCH` (-5) - 类型不匹配
- `XY_TLV_NOT_FOUND` (-6) - 未找到 TLV
- `XY_TLV_INVALID_LENGTH` (-7) - 长度无效
- `XY_TLV_NESTING_OVERFLOW` (-8) - 超过最大嵌套层级
- `XY_TLV_CHECKSUM_ERROR` (-9) - 校验和失败

## 示例

### 示例 1: 设备配置

```c
/* 定义自定义类型 */
#define CFG_DEVICE_ID    0x1001
#define CFG_DEVICE_NAME  0x1002
#define CFG_WIFI_SSID    0x1003
#define CFG_WIFI_PASS    0x1004
#define CFG_ENABLED      0x1005

/* 编码配置 */
uint8_t config_buf[512];
xy_tlv_buffer_t cfg;

xy_tlv_buffer_init(&cfg, config_buf, sizeof(config_buf));
xy_tlv_encode_uint32(&cfg, CFG_DEVICE_ID, 0xDEADBEEF);
xy_tlv_encode_string(&cfg, CFG_DEVICE_NAME, "我的设备");
xy_tlv_encode_string(&cfg, CFG_WIFI_SSID, "我的网络");
xy_tlv_encode_string(&cfg, CFG_WIFI_PASS, "password123");
xy_tlv_encode_bool(&cfg, CFG_ENABLED, true);

uint16_t cfg_size = xy_tlv_buffer_get_used(&cfg);

/* 解码配置 */
xy_tlv_iterator_t iter;
xy_tlv_t tlv;

xy_tlv_iterator_init(&iter, config_buf, cfg_size);

while (xy_tlv_iterator_next(&iter, &tlv) == XY_TLV_OK) {
    switch (tlv.type) {
        case CFG_DEVICE_ID: {
            uint32_t id;
            xy_tlv_decode_uint32(&tlv, &id);
            break;
        }
        case CFG_DEVICE_NAME: {
            char name[32];
            xy_tlv_decode_string(&tlv, name, sizeof(name));
            break;
        }
        /* ... 处理其他字段 ... */
    }
}
```

### 示例 2: 传感器数据包

```c
#define SENSOR_TEMP      0x2001
#define SENSOR_HUMIDITY  0x2002
#define SENSOR_PRESSURE  0x2003
#define SENSOR_TIMESTAMP 0x2004

/* 编码传感器数据 */
uint8_t sensor_buf[128];
xy_tlv_buffer_t sensor;

xy_tlv_buffer_init(&sensor, sensor_buf, sizeof(sensor_buf));
xy_tlv_encode_int16(&sensor, SENSOR_TEMP, 2350);        // 23.50°C
xy_tlv_encode_uint16(&sensor, SENSOR_HUMIDITY, 6520);   // 65.20%
xy_tlv_encode_uint32(&sensor, SENSOR_PRESSURE, 101325); // Pa
xy_tlv_encode_uint32(&sensor, SENSOR_TIMESTAMP, time(NULL));

/* 添加校验和 */
uint16_t size = xy_tlv_buffer_get_used(&sensor);
uint16_t crc = xy_tlv_checksum(sensor_buf, size);
xy_tlv_encode_uint16(&sensor, XY_TLV_TYPE_CHECKSUM, crc);
```

### 示例 3: 查找特定的 TLV

```c
/* 查找特定的配置值 */
xy_tlv_t found;

if (xy_tlv_find(config_buf, cfg_size, CFG_WIFI_SSID, &found) == XY_TLV_OK) {
    char ssid[33];
    xy_tlv_decode_string(&found, ssid, sizeof(ssid));
    printf("WiFi SSID: %s\n", ssid);
}
```

### 示例 4: 嵌套容器

```c
#define CONFIG_NETWORK   0x3001
#define CONFIG_SECURITY  0x3002

xy_tlv_buffer_t cfg;
xy_tlv_buffer_init(&cfg, buffer, sizeof(buffer));

/* 开始网络容器 */
xy_tlv_container_begin(&cfg, CONFIG_NETWORK);
xy_tlv_encode_string(&cfg, CFG_WIFI_SSID, "我的网络");
xy_tlv_encode_string(&cfg, CFG_WIFI_PASS, "密码");
xy_tlv_container_end(&cfg);

/* 开始安全容器 */
xy_tlv_container_begin(&cfg, CONFIG_SECURITY);
xy_tlv_encode_bool(&cfg, 0x3101, true);
xy_tlv_container_end(&cfg);
```

### 示例 5: 数据验证

```c
/* 验证接收到的 TLV 缓冲区 */
if (xy_tlv_validate(rx_buffer, rx_len) != XY_TLV_OK) {
    printf("无效的 TLV 结构!\n");
    return;
}

/* 验证校验和 */
xy_tlv_t crc_tlv;
if (xy_tlv_find(rx_buffer, rx_len, XY_TLV_TYPE_CHECKSUM, &crc_tlv) == XY_TLV_OK) {
    uint16_t received_crc;
    xy_tlv_decode_uint16(&crc_tlv, &received_crc);

    /* 计算数据的 CRC（不包括校验和 TLV）*/
    uint16_t data_len = rx_len - (XY_TLV_HEADER_SIZE + 2);
    uint16_t calc_crc = xy_tlv_checksum(rx_buffer, data_len);

    if (received_crc != calc_crc) {
        printf("校验和不匹配!\n");
    }
}
```

## 配置

通过在包含 `xy_tlv.h` 之前定义这些宏来自定义行为：

```c
#define XY_TLV_MAX_NESTING_LEVEL 8  /* 默认值：4 */
#define XY_TLV_ENABLE_VALIDATION 1  /* 默认值：1 */
```

## 最佳实践

1. **始终检查返回码** - 所有函数都返回错误码
2. **验证外部数据** - 对接收到的缓冲区使用 `xy_tlv_validate()`
3. **使用校验和** - 为关键数据添加完整性检查
4. **缓冲区大小** - 分配足够的缓冲区空间（以最坏情况估算）
5. **类型定义** - 将自定义类型定义为常量以便维护
6. **错误处理** - 使用 `xy_tlv_get_error_string()` 进行调试

## 内存使用

- **每个 TLV 元素**：4 字节头部 + 值长度
- **缓冲区上下文**：约 12 字节
- **迭代器**：约 16 字节
- **统计信息**：约 24 字节（全局）

**基本使用的总 RAM**：约 52 字节 + 缓冲区大小

## 性能

- **编码**：每个元素 O(1)
- **解码**：每个元素 O(1)
- **查找**：O(n) 线性搜索
- **验证**：O(n) 单次遍历

## 线程安全

TLV 库默认**不是线程安全的**。对于多线程使用：
- 每个线程使用单独的缓冲区/迭代器
- 为共享缓冲区添加外部同步
- 统计信息是全局的，可能需要互斥锁保护

## 集成

将项目添加到您的工程：

1. 将 `xy_tlv.h` 和 `xy_tlv.c` 复制到源代码树
2. 包含头文件：`#include "xy_tlv.h"`
3. 在构建系统中链接 `xy_tlv.c`
4. 无外部依赖（仅标准 C 库）

## 许可证

XY 嵌入式组件框架的一部分。

## 支持

如有问题或疑问，请参阅 XY 框架主文档。
