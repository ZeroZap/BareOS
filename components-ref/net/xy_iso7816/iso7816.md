# XY ISO7816 Protocol Implementation

完整的 ISO7816-3 T=0 协议实现，用于智能卡（SIM/USIM 卡）通信。

## 特性

- ✅ **完整的 ISO7816-3 T=0 协议支持**
- ✅ **ATR (Answer To Reset) 解析**
- ✅ **APDU 命令/响应处理**
- ✅ **SIM 卡基础操作**
  - 卡类型检测 (SIM/USIM/ISIM)
  - ICCID 读取
  - IMSI 读取
  - PIN 验证
- ✅ **2G 认证** (GET CHALLENGE)
- ✅ **3G/4G 双向认证** (AUTHENTICATE)
- ✅ **文件系统操作** (SELECT, READ BINARY)
- ✅ **基于 HAL UART 抽象层**，跨平台移植

## 架构

```
xy_iso7816/
├── xy_iso7816.h          # 头文件 - API 定义
├── xy_iso7816.c          # 实现文件
├── xy_iso7816_example.c  # 使用示例
└── README.md             # 本文档
```

## 依赖

- `bsp/xy_hal/inc/xy_hal_uart.h` - UART 硬件抽象层
- `components/xy_clib/xy_typedef.h` - 基础类型定义
- `components/xy_clib/xy_string.h` - 字符串工具

## 快速开始

### 1. 初始化和复位

```c
#include "xy_iso7816.h"

// 配置 UART (ISO7816 标准配置)
xy_hal_uart_config_t uart_config = {
    .baudrate = 9600,                      // 9600 bps
    .wordlen = XY_HAL_UART_WORDLEN_8B,     // 8 数据位
    .stopbits = XY_HAL_UART_STOPBITS_2,    // 2 停止位
    .parity = XY_HAL_UART_PARITY_EVEN,     // 偶校验
    .flowctrl = XY_HAL_UART_FLOWCTRL_NONE, // 无流控
    .mode = XY_HAL_UART_MODE_TX_RX,
};

void *uart = /* 平台相关的 UART 句柄 */;
xy_hal_uart_init(uart, &uart_config);

// 初始化 ISO7816
xy_iso7816_handle_t handle;
xy_iso7816_init(&handle, uart);

// 复位卡片并获取 ATR
xy_iso7816_atr_t atr;
xy_iso7816_reset(&handle, &atr);
```

### 2. 读取 SIM 卡信息

```c
xy_iso7816_sim_info_t sim_info;
xy_iso7816_get_sim_info(&handle, &sim_info);

// 转换 ICCID 为字符串
char iccid_str[21];
xy_iso7816_bcd_to_ascii(sim_info.iccid, sim_info.iccid_len, 
                        iccid_str, sizeof(iccid_str));
printf("ICCID: %s\n", iccid_str);

// 转换 IMSI 为字符串
char imsi_str[16];
xy_iso7816_bcd_to_ascii(&sim_info.imsi[1], sim_info.imsi[0],
                        imsi_str, sizeof(imsi_str));
printf("IMSI: %s\n", imsi_str);
```

### 3. PIN 验证

```c
xy_u8 remaining_tries;
xy_iso7816_error_t ret = xy_iso7816_verify_pin(&handle, "1234", &remaining_tries);

if (ret == XY_ISO7816_OK) {
    printf("PIN 验证成功\n");
} else {
    printf("PIN 错误，剩余尝试次数: %d\n", remaining_tries);
}
```

### 4. 3G/4G 双向认证

```c
// 从网络获得的认证向量
xy_u8 rand[16] = { /* 16 字节随机数 */ };
xy_u8 autn[16] = { /* 16 字节认证令牌 */ };

// 响应缓冲区
xy_u8 res[8];   // 认证响应
xy_u8 ck[16];   // 加密密钥
xy_u8 ik[16];   // 完整性密钥

ret = xy_iso7816_authenticate(&handle, rand, autn, res, ck, ik);

if (ret == XY_ISO7816_OK) {
    // 将 RES 发送给网络验证
    // 使用 CK 和 IK 进行加密/完整性保护
}
```

## API 参考

### 核心协议函数

| 函数 | 描述 |
|------|------|
| `xy_iso7816_init()` | 初始化 ISO7816 接口 |
| `xy_iso7816_deinit()` | 反初始化 ISO7816 接口 |
| `xy_iso7816_reset()` | 卡片复位并获取 ATR |
| `xy_iso7816_transceive()` | 发送 APDU 命令并接收响应 |
| `xy_iso7816_is_success()` | 检查响应是否成功 (SW=0x9000) |
| `xy_iso7816_get_sw()` | 获取状态字 (SW1/SW2) |

### SIM 卡操作函数

| 函数 | 描述 |
|------|------|
| `xy_iso7816_detect_card_type()` | 检测卡类型 (SIM/USIM/ISIM) |
| `xy_iso7816_read_iccid()` | 读取 ICCID (卡号) |
| `xy_iso7816_read_imsi()` | 读取 IMSI |
| `xy_iso7816_get_sim_info()` | 获取完整 SIM 卡信息 |
| `xy_iso7816_verify_pin()` | PIN 验证 |
| `xy_iso7816_get_challenge()` | 获取认证挑战 (2G) |
| `xy_iso7816_authenticate()` | 双向认证 (3G/4G) |
| `xy_iso7816_select_file()` | 选择文件 |
| `xy_iso7816_read_binary()` | 读取二进制数据 |

### 工具函数

| 函数 | 描述 |
|------|------|
| `xy_iso7816_bcd_to_ascii()` | BCD 转 ASCII 字符串 |
| `xy_iso7816_parse_atr()` | 解析 ATR |

## 常用文件 ID (FID)

| 常量 | 值 | 描述 |
|------|-----|------|
| `XY_ISO7816_FID_MF` | 0x3F00 | 主文件 (Master File) |
| `XY_ISO7816_FID_DF_TELECOM` | 0x7F10 | 电信目录 |
| `XY_ISO7816_FID_DF_GSM` | 0x7F20 | GSM 目录 |
| `XY_ISO7816_FID_EF_ICCID` | 0x2FE2 | ICCID 文件 |
| `XY_ISO7816_FID_EF_IMSI` | 0x6F07 | IMSI 文件 |
| `XY_ISO7816_FID_EF_LOCI` | 0x6F7E | 位置信息 |
| `XY_ISO7816_FID_EF_AD` | 0x6FAD | 管理数据 |
| `XY_ISO7816_FID_EF_SPN` | 0x6F46 | 服务提供商名称 |

## 状态字 (Status Words)

| 常量 | 值 | 描述 |
|------|-----|------|
| `XY_ISO7816_SW_SUCCESS` | 0x9000 | 成功 |
| `XY_ISO7816_SW_MORE_DATA` | 0x61XX | 有更多数据 (XX=数据长度) |
| `XY_ISO7816_SW_WRONG_LENGTH` | 0x6700 | 错误的长度 |
| `XY_ISO7816_SW_SECURITY_STATUS` | 0x6982 | 安全状态不满足 |
| `XY_ISO7816_SW_AUTH_BLOCKED` | 0x6983 | 认证方法被锁定 |
| `XY_ISO7816_SW_FILE_NOT_FOUND` | 0x6A82 | 文件未找到 |
| `XY_ISO7816_SW_INS_NOT_SUPPORTED` | 0x6D00 | 指令不支持 |
| `XY_ISO7816_SW_WRONG_PIN` | 0x63CX | PIN 错误 (X=剩余次数) |

## APDU 命令示例

### SELECT 命令

```c
xy_iso7816_select_file(&handle, XY_ISO7816_FID_MF);
```

### READ BINARY 命令

```c
xy_u8 data[10];
xy_iso7816_read_binary(&handle, 0, data, 10);
```

### 自定义 APDU

```c
xy_iso7816_apdu_cmd_t cmd = {
    .cla = XY_ISO7816_CLA_GSM,
    .ins = 0xB0,  // READ BINARY
    .p1 = 0x00,
    .p2 = 0x00,
    .lc = 0,
    .le = 10,
};

xy_iso7816_apdu_resp_t resp;
xy_iso7816_transceive(&handle, &cmd, &resp);
```

## 时序要求

- **ATR 超时**: 20 秒 (符合 ISO7816-3 标准)
- **默认命令超时**: 1 秒
- **字节间超时**: 100 毫秒

## 错误处理

所有函数返回 `xy_iso7816_error_t` 类型：

```c
typedef enum {
    XY_ISO7816_OK                  = 0,   // 成功
    XY_ISO7816_ERROR               = -1,  // 通用错误
    XY_ISO7816_ERROR_INVALID_PARAM = -2,  // 无效参数
    XY_ISO7816_ERROR_TIMEOUT       = -3,  // 超时
    XY_ISO7816_ERROR_IO            = -4,  // I/O 错误
    XY_ISO7816_ERROR_PROTOCOL      = -5,  // 协议错误
    XY_ISO7816_ERROR_ATR           = -6,  // ATR 错误
    XY_ISO7816_ERROR_NOT_INIT      = -7,  // 未初始化
    XY_ISO7816_ERROR_CARD          = -8,  // 卡片错误 (检查 SW1/SW2)
} xy_iso7816_error_t;
```

示例：

```c
xy_iso7816_error_t ret = xy_iso7816_read_imsi(&handle, imsi, &len);

if (ret == XY_ISO7816_OK) {
    // 成功处理
} else if (ret == XY_ISO7816_ERROR_TIMEOUT) {
    // 超时处理
} else if (ret == XY_ISO7816_ERROR_CARD) {
    // 检查 SW1/SW2
}
```

## 平台移植

该实现基于 HAL UART 抽象层，移植到新平台只需：

1. 实现 `xy_hal_uart.h` 中定义的 UART 接口
2. 配置 UART 为 ISO7816 标准参数：
   - 9600 bps
   - 8 数据位，偶校验，2 停止位
3. 提供平台相关的 UART 句柄

## 注意事项

1. **卡片供电**: 确保 SIM 卡供电正常 (通常 3V 或 5V)
2. **复位时序**: 在调用 `xy_iso7816_reset()` 前需要正确的硬件复位时序
3. **PIN 安全**: PIN 验证失败超过限制次数会锁卡，请谨慎使用
4. **协议支持**: 当前实现仅支持 T=0 协议（大多数 SIM 卡使用）
5. **线程安全**: 当前实现非线程安全，多线程环境需添加互斥保护

## 测试

编译并运行示例：

```bash
# 定义 ISO7816_EXAMPLE_MAIN 宏
gcc -DISO7816_EXAMPLE_MAIN xy_iso7816.c xy_iso7816_example.c -o iso7816_test

# 运行
./iso7816_test
```

## 参考标准

- **ISO/IEC 7816-3**: 接触式卡片 - 电气接口和传输协议
- **ETSI TS 102.221**: UICC-终端接口；物理和逻辑特性
- **3GPP TS 31.102**: USIM 应用特性
- **GSM 11.11**: SIM-ME 接口规范

## 许可

本代码为 XinYi 项目的一部分，遵循项目许可协议。

## 贡献

欢迎提交问题和改进建议！
