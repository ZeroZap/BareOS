# XY ISO7816 Quick Reference Guide

快速参考指南 - 常用操作速查表

## 目录
- [初始化](#初始化)
- [卡片复位](#卡片复位)
- [读取卡信息](#读取卡信息)
- [PIN 验证](#pin-验证)
- [认证操作](#认证操作)
- [文件操作](#文件操作)
- [自定义 APDU](#自定义-apdu)
- [错误处理](#错误处理)

---

## 初始化

### UART 配置 (ISO7816 标准)
```c
xy_hal_uart_config_t uart_config = {
    .baudrate = 9600,                      // 9600 bps
    .wordlen = XY_HAL_UART_WORDLEN_8B,     // 8 位数据
    .stopbits = XY_HAL_UART_STOPBITS_2,    // 2 停止位
    .parity = XY_HAL_UART_PARITY_EVEN,     // 偶校验
    .flowctrl = XY_HAL_UART_FLOWCTRL_NONE,
    .mode = XY_HAL_UART_MODE_TX_RX,
};
xy_hal_uart_init(uart_handle, &uart_config);
```

### ISO7816 初始化
```c
xy_iso7816_handle_t handle;
xy_iso7816_init(&handle, uart_handle);
```

---

## 卡片复位

```c
xy_iso7816_atr_t atr;
xy_iso7816_error_t ret = xy_iso7816_reset(&handle, &atr);

if (ret == XY_ISO7816_OK) {
    printf("ATR: ");
    for (int i = 0; i < atr.length; i++) {
        printf("%02X ", atr.data[i]);
    }
    printf("\n");
}
```

---

## 读取卡信息

### 方法 1: 一次性获取所有信息
```c
xy_iso7816_sim_info_t info;
xy_iso7816_get_sim_info(&handle, &info);

// 卡类型
printf("Card Type: %d\n", info.card_type);  // 1=SIM, 2=USIM

// ICCID
char iccid_str[21];
xy_iso7816_bcd_to_ascii(info.iccid, info.iccid_len, iccid_str, sizeof(iccid_str));
printf("ICCID: %s\n", iccid_str);

// IMSI
char imsi_str[16];
xy_iso7816_bcd_to_ascii(&info.imsi[1], info.imsi[0], imsi_str, sizeof(imsi_str));
printf("IMSI: %s\n", imsi_str);
```

### 方法 2: 单独读取
```c
// 读取 ICCID
xy_u8 iccid[10];
xy_u8 iccid_len;
xy_iso7816_read_iccid(&handle, iccid, &iccid_len);

// 读取 IMSI
xy_u8 imsi[9];
xy_u8 imsi_len;
xy_iso7816_read_imsi(&handle, imsi, &imsi_len);
```

---

## PIN 验证

```c
const char *pin = "1234";
xy_u8 remaining_tries;

xy_iso7816_error_t ret = xy_iso7816_verify_pin(&handle, pin, &remaining_tries);

if (ret == XY_ISO7816_OK) {
    printf("PIN 验证成功\n");
} else {
    printf("PIN 错误，剩余尝试: %d\n", remaining_tries);
}
```

⚠️ **警告**: PIN 错误次数过多会导致卡片锁定！

---

## 认证操作

### 2G 认证 (RUN GSM ALGORITHM)
```c
xy_u8 rand[16];
xy_iso7816_get_challenge(&handle, rand);

// rand 包含 16 字节随机数
// 发送给网络进行认证
```

### 3G/4G 认证 (AUTHENTICATE)
```c
// 从网络获取的认证向量
xy_u8 rand[16] = { /* 网络提供的 RAND */ };
xy_u8 autn[16] = { /* 网络提供的 AUTN */ };

// 响应缓冲区
xy_u8 res[8];   // 认证响应 (发送给网络)
xy_u8 ck[16];   // 加密密钥
xy_u8 ik[16];   // 完整性密钥

xy_iso7816_error_t ret = xy_iso7816_authenticate(&handle, rand, autn, res, ck, ik);

if (ret == XY_ISO7816_OK) {
    // 发送 RES 给网络验证
    // 使用 CK 和 IK 进行加密和完整性保护
}
```

---

## 文件操作

### 选择文件
```c
// 选择主文件
xy_iso7816_select_file(&handle, XY_ISO7816_FID_MF);

// 选择 GSM 目录
xy_iso7816_select_file(&handle, XY_ISO7816_FID_DF_GSM);

// 选择其他文件
xy_iso7816_select_file(&handle, 0x6F46);  // EF_SPN
```

### 读取二进制文件
```c
xy_u8 data[50];
xy_iso7816_error_t ret = xy_iso7816_read_binary(&handle, 0, data, sizeof(data));

if (ret == XY_ISO7816_OK) {
    // 处理读取的数据
}
```

### 完整的文件读取流程
```c
// 1. 选择主文件
xy_iso7816_select_file(&handle, XY_ISO7816_FID_MF);

// 2. 选择目录
xy_iso7816_select_file(&handle, XY_ISO7816_FID_DF_TELECOM);

// 3. 选择文件
xy_iso7816_select_file(&handle, XY_ISO7816_FID_EF_AD);

// 4. 读取数据
xy_u8 ad_data[4];
xy_iso7816_read_binary(&handle, 0, ad_data, sizeof(ad_data));
```

---

## 自定义 APDU

### 构造和发送 APDU 命令
```c
xy_iso7816_apdu_cmd_t cmd;
xy_iso7816_apdu_resp_t resp;

memset(&cmd, 0, sizeof(cmd));
cmd.cla = XY_ISO7816_CLA_GSM;        // 0xA0
cmd.ins = XY_ISO7816_INS_SELECT;     // 0xA4
cmd.p1 = 0x00;
cmd.p2 = 0x04;
cmd.lc = 2;                           // 数据长度
cmd.data[0] = 0x3F;
cmd.data[1] = 0x00;
cmd.le = 0;                           // 期望响应长度

xy_iso7816_error_t ret = xy_iso7816_transceive(&handle, &cmd, &resp);

if (ret == XY_ISO7816_OK) {
    // 检查状态字
    if (xy_iso7816_is_success(&resp)) {
        printf("命令成功!\n");
    } else {
        xy_u16 sw = xy_iso7816_get_sw(&resp);
        printf("命令失败: 0x%04X\n", sw);
    }

    // 处理响应数据
    for (int i = 0; i < resp.length; i++) {
        printf("%02X ", resp.data[i]);
    }
}
```

---

## 错误处理

### 检查返回值
```c
xy_iso7816_error_t ret = xy_iso7816_read_iccid(&handle, iccid, &len);

switch (ret) {
    case XY_ISO7816_OK:
        printf("成功\n");
        break;
    case XY_ISO7816_ERROR_TIMEOUT:
        printf("超时\n");
        break;
    case XY_ISO7816_ERROR_IO:
        printf("I/O 错误\n");
        break;
    case XY_ISO7816_ERROR_CARD:
        printf("卡片错误 (检查 SW1/SW2)\n");
        break;
    default:
        printf("其他错误: %d\n", ret);
        break;
}
```

### 检查状态字
```c
xy_iso7816_apdu_resp_t resp;
// ... 执行命令 ...

xy_u16 sw = xy_iso7816_get_sw(&resp);

switch (sw) {
    case XY_ISO7816_SW_SUCCESS:
        printf("成功\n");
        break;
    case XY_ISO7816_SW_FILE_NOT_FOUND:
        printf("文件未找到\n");
        break;
    case XY_ISO7816_SW_SECURITY_STATUS:
        printf("需要 PIN 验证\n");
        break;
    case XY_ISO7816_SW_WRONG_PIN:
        printf("PIN 错误，剩余次数: %d\n", resp.sw2 & 0x0F);
        break;
    default:
        if ((sw & 0xFF00) == 0x6100) {
            printf("更多数据可用: %d 字节\n", resp.sw2);
        } else {
            printf("未知状态: 0x%04X\n", sw);
        }
        break;
}
```

---

## 常用文件 ID

| 文件 | FID | 描述 |
|------|-----|------|
| MF | 0x3F00 | 主文件 |
| DF_TELECOM | 0x7F10 | 电信目录 |
| DF_GSM | 0x7F20 | GSM 目录 |
| EF_ICCID | 0x2FE2 | SIM 卡号 |
| EF_IMSI | 0x6F07 | 国际移动用户识别码 |
| EF_AD | 0x6FAD | 管理数据 |
| EF_SPN | 0x6F46 | 服务提供商名称 |
| EF_LOCI | 0x6F7E | 位置信息 |

---

## 常用 APDU 指令

| 指令 | INS | 描述 |
|------|-----|------|
| SELECT | 0xA4 | 选择文件 |
| READ BINARY | 0xB0 | 读取二进制 |
| READ RECORD | 0xB2 | 读取记录 |
| UPDATE BINARY | 0xD6 | 更新二进制 |
| UPDATE RECORD | 0xDC | 更新记录 |
| VERIFY PIN | 0x20 | 验证 PIN |
| GET CHALLENGE | 0x84 | 获取挑战 |
| AUTHENTICATE | 0x88 | 认证 |
| GET RESPONSE | 0xC0 | 获取响应 |

---

## 完整工作流程示例

```c
void sim_card_workflow(void) {
    xy_iso7816_handle_t handle;
    xy_iso7816_error_t ret;

    // 1. 初始化
    xy_iso7816_init(&handle, uart_handle);

    // 2. 复位卡片
    xy_iso7816_atr_t atr;
    xy_iso7816_reset(&handle, &atr);

    // 3. 检测卡类型
    xy_iso7816_card_type_t type;
    xy_iso7816_detect_card_type(&handle, &type);

    // 4. 读取 ICCID
    xy_u8 iccid[10];
    xy_u8 len;
    xy_iso7816_read_iccid(&handle, iccid, &len);

    // 5. PIN 验证
    xy_iso7816_verify_pin(&handle, "1234", NULL);

    // 6. 读取 IMSI
    xy_u8 imsi[9];
    xy_iso7816_read_imsi(&handle, imsi, &len);

    // 7. 进行认证 (如需要)
    // ...

    // 8. 清理
    xy_iso7816_deinit(&handle);
}
```

---

## 调试技巧

### 启用调试日志
在 `xy_iso7816_cfg.h` 中设置:
```c
#define XY_ISO7816_CFG_DEBUG_LOG    1
```

### 打印 ATR
```c
printf("ATR (%d bytes): ", atr.length);
for (int i = 0; i < atr.length; i++) {
    printf("%02X ", atr.data[i]);
}
printf("\n");
```

### 打印 APDU
```c
// 命令
printf(">> CLA=%02X INS=%02X P1=%02X P2=%02X Lc=%02X Le=%02X\n",
       cmd.cla, cmd.ins, cmd.p1, cmd.p2, cmd.lc, cmd.le);

// 响应
printf("<< Data(%d): ", resp.length);
for (int i = 0; i < resp.length; i++) {
    printf("%02X ", resp.data[i]);
}
printf("SW=%02X%02X\n", resp.sw1, resp.sw2);
```

---

## 常见问题

### Q: 读取 IMSI 失败？
A: 确保先验证 PIN。某些文件需要 PIN 验证后才能访问。

### Q: 超时错误？
A: 检查 UART 配置和波特率。确保硬件连接正常。

### Q: 状态字 0x6982？
A: 安全状态不满足，需要先验证 PIN。

### Q: 状态字 0x6A82？
A: 文件未找到，检查文件路径和 FID。

---

## 性能优化

1. **批量操作**: 一次性读取所需数据，减少命令往返
2. **超时设置**: 根据实际情况调整超时时间
3. **缓存**: 缓存不变的数据（如 ICCID）避免重复读取

---

## 安全注意事项

⚠️ **重要**:
- PIN 验证失败超过 3 次会锁卡
- 不要在日志中打印敏感信息 (PIN, Ki, 认证密钥)
- 生产环境关闭调试日志
- 妥善保管认证密钥 (CK, IK)

---

更多信息请参考 [README.md](README.md)
