# SecBoot-N32 使用指导

本文档描述 `Sec-boot-N32` V1 的开发、构建、打包、刷写和排错流程。

当前版本目标是先跑通 UART5 recovery/download 链路，后续再逐步补齐签名验证、回滚计数、App 跳转和生产级密钥管理。

## 角色

| 模块 | 位置 | 作用 |
|---|---|---|
| Bootloader 固件 | `project/Sec-boot-N32` | N32L406 上的 sec-boot V1，使用 UART5 接收镜像 |
| Host 上位机 | `tool/xy_secboot` | Python CLI/GUI，生成 `.sbp` 并通过 UART5 传输 |
| SecBoot 组件 | `components/xy_secboot` | manifest、partition、single-slot verify 框架 |
| Crypto 组件 | `components/crypto` | SHA-256/HMAC-SHA256 等算法基础 |

## 当前状态

| 功能 | 状态 | 说明 |
|---|---|---|
| UART5 HELLO/CAPS | 已实现 | 上位机可探测 bootloader 能力 |
| MANIFEST 接收 | 已实现 | MCU 做基础 manifest 检查并擦除 App 区 |
| DATA 传输 | 已实现 | stop-and-wait，CRC32，seq/offset，Flash 写入后读回 |
| END 校验 | 部分实现 | MCU 调用 `xy_secboot_single_verify_active()` |
| SHA-256 image hash | 已接入 | N32 port 提供 SHA-256 hash ops |
| 签名/公钥验证 | 未完成 | 当前会拒绝未完成验证的镜像 |
| App manifest 写入 | 有条件 | 仅 verify 成功后写入 |
| App 跳转 | 未完成 | 后续添加 |
| Boot state/rollback 持久化 | 未完成 | 后续添加 |

重要：当前 `END` 阶段可能返回 `IMAGE_VERIFY_FAILED`，这是预期行为。当前版本不能把未完成签名验证的镜像标记为可启动，避免形成不安全 bootloader。

## 硬件连接

| 信号 | 引脚 | 用途 |
|---|---|---|
| UART4 TX | PB0 | 调试 log 输出 |
| UART4 RX | PB1 | 调试口预留 |
| UART5 TX | PB8 | SecBoot UART5 transport TX |
| UART5 RX | PB9 | SecBoot UART5 transport RX |

建议使用两个串口工具：一个接 UART4 看 log，一个接 UART5 给上位机刷写。

默认波特率：`115200 8N1`。

## Flash 布局

| 区域 | 地址 | 大小 | 说明 |
|---|---:|---:|---|
| Bootloader | `0x08000000` | `0x6000` | SecBoot 固件 |
| Boot state | `0x08006000` | `0x0800` | 后续保存状态 |
| Rollback | `0x08006800` | `0x0800` | 后续保存回滚计数 |
| App manifest | `0x08007000` | `0x0800` | verify 成功后最后写入 |
| App image | `0x08007800` | `0x16800` | 单槽 App 镜像 |
| EEPROM reserved | `0x0801E000` | `0x1000` | 预留 |
| FEE reserved | `0x0801F000` | `0x1000` | 预留 |

App 工程后续需要把向量表和链接地址放到 `0x08007800`，并确保镜像大小不超过 `0x16800`。

## 环境准备

安装 Python 依赖：

```bash
python -m pip install -r tool/xy_secboot/requirements.txt
```

构建 SecBoot 固件需要 ARM GCC 工具链。若工具链不在 PATH 中，可在 make 命令里传入 `GCC_PATH`。

```bash
make GCC_PATH="D:/Program Files (x86)/GNU Arm Embedded Toolchain/10 2021.10/bin/"
```

## 构建 Bootloader

进入工程 Makefile 目录：

```bash
cd project/Sec-boot-N32/Makefile
make clean
make
```

成功后生成：

```text
build/SecBootN32.elf
build/SecBootN32.hex
build/SecBootN32.bin
```

当前最小工程只编译必要 HAL：`misc`、`flash`、`gpio`、`iwdg`、`rcc`、`usart`。

## 烧录 Bootloader

使用 pyOCD：

```bash
make flash
```

使用 J-Link：

```bash
make download-jlink
```

烧录后 UART4 应能看到类似日志：

```text
SecBoot-N32 UART4 log ready
SecBoot-N32 UART5 secboot V1 transport ready
SecBoot-N32 main loop start
```

UART5 输入 `?` 可返回 banner。UART5 输入 `p` 会在 UART4 log 打印 Flash layout。

## 准备 App 镜像

App binary 需要满足：

| 项目 | 要求 |
|---|---|
| 链接地址 | `0x08007800` |
| 入口地址 | 通常为 `0x08007800` |
| 大小 | 不超过 `0x16800` |
| 对齐 | 上位机打包时会自动补 `0xFF` 到 4 字节边界 |

如果 App 原始 `.bin` 不是 4 字节对齐，上位机会在打包阶段补齐，并用补齐后的数据计算 SHA-256 和 manifest 字段。

## CLI 打包

```bash
python tool/xy_secboot/xy_secboot.py pack \
  --input build/app.bin \
  --output build/app.sbp \
  --product-id 0x00010001 \
  --image-addr 0x08007800 \
  --entry-addr 0x08007800 \
  --image-version 1 \
  --security-counter 1
```

查看包信息：

```bash
python tool/xy_secboot/xy_secboot.py inspect build/app.sbp
```

输出会包含：

```text
suite_id
manifest_len
image_len
package_crc32
product_id
image_addr
image_size
entry_addr
image_hash_sha256
signature_head
```

当前 `signature_head` 可能全 0，因为 MCU 签名验证后端还没有完成。

## CLI 刷写

列出串口：

```bash
python tool/xy_secboot/xy_secboot.py ports
```

刷写 `.sbp`：

```bash
python tool/xy_secboot/xy_secboot.py flash \
  --port COM12 \
  --baud 115200 \
  --package build/app.sbp \
  --payload 256 \
  --timeout-ms 1000 \
  --retries 10
```

刷写流程：

```text
HELLO -> CAPS
MANIFEST -> ACK/NACK
DATA seq/offset -> ACK/NACK
END -> ACK/ERROR
```

若 MCU 端签名验证未完成，`END` 可能返回 `ERROR reason=IMAGE_VERIFY_FAILED`。这表示传输链路可继续验证，但镜像不会被接受为可启动。

## GUI 使用

启动 GUI：

```bash
python tool/xy_secboot/xy_secboot.py gui
```

GUI 操作顺序：

1. 选择 UART5 对应串口。
2. 保持波特率 `115200`。
3. 点击 `HELLO`，确认日志窗口出现 `CAPS`。
4. 在 `Package` 区选择 App `.bin`，设置输出 `.sbp`。
5. 点击 `Build Package`。
6. 在 `Flash` 区选择 `.sbp`。
7. 点击 `Flash Package`。
8. 观察进度条、GUI log 和 UART4 调试 log。

## UART v1 帧格式

所有多字节字段均为 little-endian。

```text
magic       2 bytes   'S' 'B'
version     1 byte    1
type        1 byte
flags       1 byte
reserved    1 byte
seq         2 bytes
session_id  4 bytes
offset      4 bytes
length      2 bytes
header_crc  2 bytes   CRC16/CCITT over header with this field zeroed
payload     N bytes
payload_crc 4 bytes   CRC32 over payload
```

ACK/NACK/ERROR payload：

```text
ack_seq       2 bytes
reason        2 bytes
next_offset   4 bytes
detail        4 bytes
```

## 常见问题

| 现象 | 可能原因 | 处理 |
|---|---|---|
| `pyserial is required` | 未安装依赖 | 执行 `python -m pip install -r tool/xy_secboot/requirements.txt` |
| `No serial ports found` | 串口驱动或连接异常 | 检查 USB-UART、线序、电源 |
| `timeout waiting for secboot frame` | UART5 接错或 bootloader 未运行 | 检查 PB8/PB9、波特率、UART4 log |
| `BAD_MANIFEST` | 地址、产品 ID、大小不匹配 | 检查 pack 参数 |
| `BAD_OFFSET` | 传输 offset 不连续 | 重新刷写，检查串口稳定性 |
| `FLASH_WRITE_FAILED` | Flash 未擦除、越界或供电异常 | 确认 App 区地址和大小 |
| `IMAGE_VERIFY_FAILED` | 当前签名验证后端未完成或镜像损坏 | 当前阶段预期可能出现，后续补签名验证 |

## 开发顺序建议

| 阶段 | 目标 |
|---:|---|
| 1 | 固定 UART5 transport，保证 HELLO/CAPS、MANIFEST、DATA 可重复跑通 |
| 2 | 补 HMAC-SHA256 或 ECDSA-P256 验证后端 |
| 3 | 实现 App manifest boot 检查和 App jump |
| 4 | 持久化 boot state 和 rollback counter |
| 5 | 加入 host fault injection 和自动化回归 |
| 6 | 替换为生产密钥/生产算法策略 |

## 安全注意事项

不要提交真实生产密钥。

HMAC 开发密钥只适合实验室验证。如果密钥存放在普通 Flash 且可被 App 读取，它不能作为生产 secure boot root of trust。

生产版本需要明确密钥保护位置、读保护策略、debug lock 策略和回滚计数不可逆策略。
