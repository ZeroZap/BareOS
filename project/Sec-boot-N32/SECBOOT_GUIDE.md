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
| END 校验 | 已实现 | MCU 调用 `xy_secboot_single_verify_active()` |
| SHA-256 image hash | 已接入 | N32 port 提供 SHA-256 hash ops |
| 开发 HMAC 验证 | 已接入 | 使用 `tool/xy_secboot/dev_hmac_key.txt`，仅限实验室 |
| 签名/公钥验证 | 未完成 | 生产级公钥验证后续补齐 |
| App manifest 写入 | 有条件 | 仅 verify 成功后写入 |
| App 跳转 | 已实现 | reset-time verify 后设置 MSP/VTOR 并跳转 |
| Rollback counter 持久化 | 已实现 | `0x08006800` append-only CRC 记录 |
| Boot state 持久化 | 已接入 | `PENDING`、boot attempts、SecBoot 受控 `CONFIRMED`、pending 最大尝试次数 |

注意：当前 PLB App 通过 SRAM mailbox `0x20003F00` 发起确认请求并软复位；SecBoot 在下一次启动时验证 App manifest 后写入 `CONFIRMED` boot state。该路径用于 bring-up 验证受控 confirmed 闭环，生产版本仍应结合 MPU/WRP/RDP 确认 App 不能直接擦写 boot state/rollback 区。

重要：只有使用匹配开发 HMAC key 打包的镜像才能通过 `END` 并写入 App manifest。未带 HMAC 或 key 不匹配的包会返回 `IMAGE_VERIFY_FAILED`，避免把未认证镜像标记为可启动。

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
  --security-counter 1 \
  --hmac-key tool/xy_secboot/dev_hmac_key.txt
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

使用开发 HMAC key 打包时，`signature_head` 会显示 HMAC-SHA256 tag 的前 32 字节。若未传 `--hmac-key`，该字段全 0，MCU 会在 `END` 阶段拒绝镜像。

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

若包未使用匹配 HMAC key，`END` 会返回 `ERROR reason=IMAGE_VERIFY_FAILED`。使用 `tool/xy_secboot/dev_hmac_key.txt` 打包后，`END` 应返回 `ACK` 并写入 App manifest。

`END ACK` 只表示镜像写入、校验和 manifest 提交完成；V1 bootloader 不会在 `END ACK` 后立即跳转 App。下一次复位时，bootloader 会打开 1500 ms UART5 recovery 窗口；窗口内没有主机输入时，读取 `0x08007000` manifest，验证 App，然后设置 MSP/VTOR 并跳转 `0x08007800`。

调试阶段推荐在刷写命令后追加 `--reset`，让主机在 `END ACK` 后发送 RESET：

```bash
python tool/xy_secboot/xy_secboot.py flash \
  --port COM12 \
  --baud 115200 \
  --package build/app.sbp \
  --payload 256 \
  --timeout-ms 1000 \
  --retries 10 \
  --reset
```

若需要留在 bootloader 重新下载，在复位后 1500 ms recovery 窗口内向 UART5 发送任意字节，例如重新发起 `flash` 命令或发送 `?`。

手工调试时更推荐使用 `--recover-ms` 拉长主机侧 recovery preamble：先运行命令，然后在该时间窗口内按复位，工具会持续发送 `?` 让 bootloader 留在下载模式，再自动进入 HELLO/CAPS。

```bash
python tool/xy_secboot/xy_secboot.py flash \
  --port COM12 \
  --baud 115200 \
  --package build/app.sbp \
  --payload 256 \
  --timeout-ms 1000 \
  --retries 10 \
  --recover-ms 5000 \
  --reset
```

`--recover-ms` 的具体含义：

1. 上位机先打开串口。
2. 在指定时间内周期发送 `?`。
3. 用户在该时间内按下板子 reset。
4. SecBoot 复位后在 1500 ms recovery 窗口内收到 UART5 字节，因此留在 bootloader。
5. 上位机清空 recovery 期间产生的 banner/杂字节，再发送 `HELLO` 并等待 `CAPS`。

使用场景：板子已经有有效 App，复位后会自动跳 App，普通 `flash` 命令报 `timeout waiting for secboot frame`。如果 UART4 已经在打印 SecBoot heartbeat，说明当前已经停在 bootloader，不需要 `--recover-ms`。

### Pending 尝试次数验证

SecBoot 当前 `PENDING` App 最大尝试次数为 3。正常 PLB App 会在初始化成功后写入实验室 `CONFIRMED` 记录，因此后续复位不会继续增加 attempts。

验证失败恢复策略时，可以构建一个不写 confirmed 的测试 App：

```bash
make NO_CONFIRM=y package
```

然后用更高的 `--security-counter` 重新打包并刷入。预期复位日志：

```text
SecBoot-N32 pending App boot attempt=1
SecBoot-N32 jump App entry=8007800
...
SecBoot-N32 pending App boot attempt=2
...
SecBoot-N32 pending App boot attempt=3
...
SecBoot-N32 pending App attempts exceeded=3
SecBoot-N32 main loop start
```

超过最大尝试次数后，SecBoot 不再跳 App，停留 bootloader recovery，等待重新刷写。

### V1 故障注入验证

V1 主链路跑通后，优先验证坏包、旧 counter 和中断恢复路径。可以用
`fault-package` 从一个已知可刷写的 `.sbp` 派生故障包：

```bash
python tool/xy_secboot/xy_secboot.py fault-package \
  --package "project/PLB -N32/Makefile/build/PLB_confirm_counter4.sbp" \
  --output "project/PLB -N32/Makefile/build/PLB_bad_signature.sbp" \
  --fault bad-signature
```

建议验证矩阵：

| 用例 | 生成方式 | 预期 |
|---|---|---|
| 坏镜像数据 | `--fault bad-image` | `END` 返回 `IMAGE_VERIFY_FAILED` |
| 坏 manifest hash | `--fault bad-hash --hmac-key tool/xy_secboot/dev_hmac_key.txt` | `END` 返回 `IMAGE_VERIFY_FAILED` |
| 坏 HMAC/signature | `--fault bad-signature` | `END` 返回 `IMAGE_VERIFY_FAILED` |
| 坏 manifest CRC | `--fault bad-manifest-crc` | `MANIFEST` 返回 `BAD_MANIFEST` |
| 错 product id | `--fault bad-product --value 0x00010002 --hmac-key tool/xy_secboot/dev_hmac_key.txt` | `MANIFEST` 返回 `BAD_MANIFEST` |
| 错 entry addr | `--fault bad-entry --value 0x08000000 --hmac-key tool/xy_secboot/dev_hmac_key.txt` | `MANIFEST` 返回 `BAD_MANIFEST` |
| 旧 rollback counter | `--fault old-counter --value 3 --hmac-key tool/xy_secboot/dev_hmac_key.txt` | 已持久化 counter 高于 3 时，`END` 返回 `ROLLBACK_REJECTED` |
| 坏 package CRC | `--fault bad-package-crc` | host `inspect` 阶段直接拒绝 |

Host 工具会把 SecBoot verify 失败的内部 detail 值翻译成诊断名：

| detail | 诊断名 |
|---:|---|
| `0xfffffffe` | `IMAGE_HASH_OR_RANGE_FAILED` |
| `0xfffffffd` | `MANIFEST_MAC_FAILED` |
| `0xfffffffc` | `ROLLBACK_FAILED` |
| `0xfffffffb` | `UNSUPPORTED_CRYPTO` |
| `0xfffffffa` | `CRYPTO_HW_FAILED` |
| `0xfffffff9` | `VERIFY_STATE_FAILED` |

刷写故障包后，UART4 应保留在 SecBoot main loop，不能跳转损坏 App。每个失败
用例后都应再刷一个更高 `security_counter` 的正常 confirmed 包，确认设备可恢复。

2026-07-15 V1 验证记录：

| 项目 | 结果 |
|---|---|
| 测试串口 | UART5 `COM24` |
| 正常恢复包 | `PLB_confirm_counter4.sbp` |
| 正常包刷写 | `END ACK`，`detail=0x8007800` |
| App 启动 | 通过 |
| 坏 manifest CRC | `NACK seq=1 reason=BAD_MANIFEST` |
| 坏 HMAC/signature | `ERROR reason=IMAGE_VERIFY_FAILED detail=0xfffffffd` |
| 坏 image data | `ERROR reason=IMAGE_VERIFY_FAILED detail=0xfffffffe` |
| 旧 rollback counter | `NACK seq=1 reason=ROLLBACK_REJECTED detail=0x3` |
| 错 product id | `NACK seq=1 reason=BAD_MANIFEST` |
| 错 entry addr | `NACK seq=1 reason=BAD_MANIFEST` |
| 坏 package CRC | host `inspect` 返回 `package crc32 mismatch` |
| 故障测试后恢复 | 正常包重新刷写并启动 App，通过 |

本轮发现并修复：早期 `manifest_basic_check()` 未检查 `manifest.header_crc32`，导致
`bad-manifest-crc` 被接受。现已在 MANIFEST 阶段加入 header CRC32 校验。

### 受控 Confirmed 验证

PLB App 不再直接写 SecBoot state Flash。确认流程：

1. App 启动后读取当前 App manifest 和 boot state。
2. 若还不是 matching `CONFIRMED`，App 在 SRAM mailbox `0x20003F00` 写入确认请求。
3. App 触发 `NVIC_SystemReset()`。
4. SecBoot 启动后验证 App manifest、image hash、HMAC 和 rollback。
5. SecBoot 检查 mailbox 与当前 manifest 是否匹配。
6. 匹配时由 SecBoot 写 `CONFIRMED` boot state，并清除 mailbox。
7. SecBoot 跳转 App；App 再次启动后读到已 confirmed，不再复位。

当前验证包：

```text
project/PLB -N32/Makefile/build/PLB_confirm_mailbox_counter5.sbp
```

刷写命令：

```bash
python tool/xy_secboot/xy_secboot.py flash \
  --port COM24 \
  --baud 115200 \
  --package "project/PLB -N32/Makefile/build/PLB_confirm_mailbox_counter5.sbp" \
  --payload 256 \
  --timeout-ms 1000 \
  --retries 10 \
  --recover-ms 5000 \
  --reset
```

预期 UART4 日志：

```text
PLB-N32 secboot confirm=0
SecBoot-N32 reset flags: ... SFTRST=1 ...
SecBoot-N32 App confirmed by mailbox counter=5
SecBoot-N32 jump App entry=8007800
PLB-N32 secboot confirm=0
PLB-N32 main loop start
```

第二次 PLB App 启动时 `secboot confirm=0` 表示已经是 matching `CONFIRMED`，App 不再发 mailbox reset。

2026-07-15 受控 confirmed 验证记录：

| 项目 | 结果 |
|---|---|
| 验证包 | `PLB_confirm_mailbox_counter5.sbp` |
| 首次启动 | SecBoot 记录 `pending App boot attempt=1` 后跳 App |
| App 确认请求 | App 写 mailbox 后触发软件复位，SecBoot 日志 `SFTRST=1` |
| SecBoot 写 confirmed | `SecBoot-N32 App confirmed by mailbox counter=5` |
| 再次 App 启动 | `PLB-N32 secboot confirm=0`，进入 main loop |
| 手动再次复位 | 未再出现 pending attempt 和 mailbox confirm，直接 `jump App entry=8007800` |
| 稳定性 | App heartbeat 正常 |

再次复位日志关键片段：

```text
SecBoot-N32 jump App entry=8007800
PLB-N32 secboot confirm=0
PLB-N32 main loop start
PLB-N32 UART4 heartbeat UART5 rx=0 tx=0 rb=0 drop=0 last=0
```

### V1 中断恢复验证

故障包验证后，继续验证升级过程中断不会提交坏状态。host 工具提供两个主动中断
选项，不发送 `END`：

| 用例 | 命令选项 | 预期 |
|---|---|---|
| MANIFEST 后中断 | `--interrupt-after-manifest` | 当前实现已擦除 App 区但未提交新 manifest；复位后应进入 bootloader recovery |
| DATA 中途断开 | `--interrupt-at-offset 0x4000` | 不写 App manifest，不更新 rollback；复位后应进入 bootloader recovery，后续可重新刷正常包恢复 |

COM24 示例：

```bash
python tool/xy_secboot/xy_secboot.py flash \
  --port COM24 \
  --baud 115200 \
  --package "project/PLB -N32/Makefile/build/PLB_confirm_counter4.sbp" \
  --payload 256 \
  --timeout-ms 1000 \
  --retries 10 \
  --interrupt-after-manifest
```

```bash
python tool/xy_secboot/xy_secboot.py flash \
  --port COM24 \
  --baud 115200 \
  --package "project/PLB -N32/Makefile/build/PLB_confirm_counter4.sbp" \
  --payload 256 \
  --timeout-ms 1000 \
  --retries 10 \
  --interrupt-at-offset 0x4000
```

每个中断用例后执行一次正常恢复刷写：

```bash
python tool/xy_secboot/xy_secboot.py flash \
  --port COM24 \
  --baud 115200 \
  --package "project/PLB -N32/Makefile/build/PLB_confirm_counter4.sbp" \
  --payload 256 \
  --timeout-ms 1000 \
  --retries 10 \
  --reset
```

若正常包能 `END ACK` 并成功启动 App，说明升级中断后仍可恢复。

### UART DATA 层故障探测

`probe-transport` 只验证 transport 行为，不发送 `END`，因此不会提交新 manifest。
该命令会发送 MANIFEST 并写入至少一个 DATA 包，所以每次探测后都要重新刷正常包
恢复 App 区。

| 用例 | 命令选项 | 预期 |
|---|---|---|
| 重复 DATA | `--fault duplicate-data` | 重复帧返回 ACK |
| 错 seq | `--fault bad-seq` | 返回 `BAD_SEQ`，detail 为期望 seq |
| 错 offset | `--fault bad-offset` | 返回 `BAD_OFFSET`，detail 为错误 offset |

COM24 示例：

```bash
python tool/xy_secboot/xy_secboot.py probe-transport \
  --port COM24 \
  --baud 115200 \
  --package "project/PLB -N32/Makefile/build/PLB_confirm_mailbox_counter6.sbp" \
  --payload 256 \
  --timeout-ms 1000 \
  --retries 10 \
  --recover-ms 5000 \
  --fault bad-seq
```

探测后恢复：

```bash
python tool/xy_secboot/xy_secboot.py flash \
  --port COM24 \
  --baud 115200 \
  --package "project/PLB -N32/Makefile/build/PLB_confirm_mailbox_counter6.sbp" \
  --payload 256 \
  --timeout-ms 1000 \
  --retries 10 \
  --recover-ms 5000 \
  --reset
```

2026-07-15 UART DATA 层故障探测记录：

| 项目 | 结果 |
|---|---|
| `duplicate-data` | 符合预期 |
| `bad-seq` | 符合预期 |
| `bad-offset` | 符合预期 |
| 探测后恢复 | 正常包重新刷写并启动 App，通过 |

### State/Rollback 诊断输出

为支持后续 state/rollback 页满和损坏恢复测试，SecBoot 的 `p` 命令在打印分区
布局外，额外打印当前 rollback counter 和 boot state 摘要：

```text
SecBoot-N32 diag rollback=6
SecBoot-N32 diag state=2 seq=... cnt=6 ver=1 attempts=...
```

字段含义：

| 字段 | 含义 |
|---|---|
| `rollback` | 当前持久化 anti-rollback counter |
| `state` | `1=PENDING`，`2=CONFIRMED` |
| `seq` | boot state append-only 记录序号 |
| `cnt` | boot state 对应 security counter |
| `ver` | boot state 对应 image version |
| `attempts` | pending boot attempts |

注意：`seq` 是逻辑序号，不是页内槽位号。state page 写满后擦除重写时，当前实现
会先读取旧页最大 `seq`，再写入 `seq + 1`，因此页满 rollover 后 `seq` 会继续递增，
不会回到 1。

SecBoot bootloader 已改为 `-Os` 构建，避免 V1 分区空间不足，同时保留调试符号。

2026-07-15 state page rollover 验证记录：

| 项目 | 结果 |
|---|---|
| rollover 前 | `rollback=7 state=2 seq=64 cnt=7 ver=1 attempts=1` |
| rollover 后 | `rollback=8 state=2 seq=67 cnt=8 ver=1 attempts=1` |
| 结论 | state page 满后擦除重写成功，逻辑 `seq` 继续递增，confirmed 状态保持有效 |

2026-07-15 rollback 递增连续性验证记录：

| 项目 | 结果 |
|---|---|
| counter9 正常包 | `PLB_confirm_mailbox_counter9.sbp` |
| counter9 后诊断 | `rollback=9 state=2 seq=73 cnt=9 ver=1 attempts=1` |
| 旧 counter8 包 | `PLB_confirm_mailbox_counter8.sbp` |
| 旧包拒绝 | `NACK seq=1 reason=ROLLBACK_REJECTED detail=0x8` |
| 结论 | rollback 从 8 到 9 递增正常，旧 counter 被 MANIFEST 阶段拒绝 |

### Flash 保护状态

WRP/RDP/option bytes 保护暂不在当前单板 V1 bring-up 中实测，避免把唯一开发板锁死
或进入不可恢复状态。当前仅记录为后续生产化/多板阶段任务：

| 区域 | 地址 | 后续策略 |
|---|---:|---|
| Bootloader | `0x08000000-0x08005FFF` | WRP，只允许量产/调试流程更新 |
| Boot state | `0x08006000` | App 不应可擦写，SecBoot/private system 控制 |
| Rollback | `0x08006800` | App 不应可擦写，生产优先 OTP/eFuse 或受保护 Flash |
| App manifest | `0x08007000` | SecBoot 写入，App 只读 |

当前开发板阶段不执行 WRP/RDP 测试；所有相关验证只停留在代码与文档策略层面。

2026-07-15 中断恢复验证记录：

| 项目 | 结果 |
|---|---|
| MANIFEST 后中断 | host 输出 `flash interrupted after manifest offset=0x0 seq=2` |
| MANIFEST 后恢复 | 正常包重新刷写后恢复 |
| DATA 中途停止 | 执行中途停止后重新刷正常包恢复 |
| 恢复后 App 状态 | UART4 输出 `PLB-N32 main loop start` 和周期 heartbeat |

恢复后日志示例：

```text
[I] PLB-N32 main loop start
[I] PLB-N32 UART4 heartbeat UART5 rx=0 tx=0 rb=0 drop=0 last=0
```

## PLB-N32 作为 App 的自动化流程

### V1 验证矩阵

| 类别 | 用例 | 状态 | 证据 |
|---|---|---|---|
| 正常链路 | SecBoot `HELLO/CAPS` | 通过 | host 输出 `CAPS payload=...` |
| 正常链路 | 正常包刷写 | 通过 | `END ACK ... detail=0x8007800` |
| 正常链路 | App 启动 | 通过 | `PLB-N32 main loop start` |
| 启动认证 | 每次复位验证后跳 App | 通过 | `SecBoot-N32 jump App entry=8007800` |
| Manifest | 坏 manifest CRC | 通过 | `BAD_MANIFEST` |
| Manifest | 错 product id | 通过 | `BAD_MANIFEST` |
| Manifest | 错 entry addr | 通过 | `BAD_MANIFEST` |
| Image/Auth | 坏 image data | 通过 | `IMAGE_HASH_OR_RANGE_FAILED` |
| Image/Auth | 坏 HMAC/signature | 通过 | `MANIFEST_MAC_FAILED` |
| Rollback | 旧 counter 拒绝 | 通过 | `ROLLBACK_REJECTED` |
| Rollback | counter 8 -> 9 递增 | 通过 | `rollback=9` |
| State | pending/no-confirm 最大尝试 | 通过 | `pending App attempts exceeded=3` |
| State | state page rollover | 通过 | `seq=64` 后 counter8 confirmed 有效 |
| Confirm | mailbox 受控 confirmed | 通过 | `App confirmed by mailbox counter=5` |
| Recovery | MANIFEST 后中断恢复 | 通过 | 中断后可重新刷正常包 |
| Recovery | DATA 中途断开恢复 | 通过 | 中断后可重新刷正常包 |
| UART DATA | duplicate DATA | 通过 | duplicate DATA 返回 ACK |
| UART DATA | bad seq | 通过 | 返回 `BAD_SEQ` |
| UART DATA | bad offset | 通过 | 返回 `BAD_OFFSET` |
| Host | bad package CRC | 通过 | `package crc32 mismatch` |
| Tooling | detail 解码 | 通过 | `MANIFEST_MAC_FAILED(...)` |
| Tooling | 版本日志 | 待验证 | `SecBoot-N32 V1.1-dev` / `PLB-N32 App V1.1-dev` |
| Production | WRP/RDP | 跳过 | 单板阶段不做，避免锁板 |

`project/PLB -N32` 已按 SecBoot App slot 链接：

| 项目 | 值 |
|---|---:|
| App Flash ORIGIN | `0x08007800` |
| App Flash LENGTH | `0x16800` |
| Vector table offset | `0x7800` |

本地构建 SecBoot、构建 PLB App、生成并检查 `.sbp`：

```bash
python tool/xy_secboot/plb_app_flow.py --clean
```

同时烧录 SecBoot bootloader：

```bash
python tool/xy_secboot/plb_app_flow.py --clean --flash-boot
```

通过 UART5 刷写 PLB App 包：

```bash
python tool/xy_secboot/plb_app_flow.py --flash-app-uart --port COM12
```

刷写后抓取 UART4 log：

```bash
python tool/xy_secboot/plb_app_flow.py --flash-app-uart --port COM12 --capture-log COM8
```

当前 MCU 端开发 HMAC 验证和 reset-time verify + jump app 已接入，自动化流程可以验证构建、打包、MANIFEST/DATA 传输、Flash 写入、END 认证链路和复位启动路径。

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
| `IMAGE_VERIFY_FAILED` | 未使用匹配 HMAC key、镜像 hash/HMAC 损坏或生产签名后端未接入 | 用 `--hmac-key tool/xy_secboot/dev_hmac_key.txt` 重新打包并重刷 |
| `ROLLBACK_REJECTED` | `security_counter` 低于已持久化 counter，或 rollback page 写入失败 | 提高 `--security-counter` 后重新打包；若写入失败检查 rollback page Flash |

## 开发顺序建议

| 阶段 | 目标 |
|---:|---|
| 1 | 固定 UART5 transport，保证 HELLO/CAPS、MANIFEST、DATA 可重复跑通 |
| 2 | 补 HMAC-SHA256 或 ECDSA-P256 验证后端 |
| 3 | 实现 App manifest boot 检查和 App jump |
| 4 | 将 App confirmed 从实验室直写替换为 private system 受控确认 |
| 5 | 加入 host fault injection 和自动化回归 |
| 6 | 替换为生产密钥/生产算法策略 |

## 安全注意事项

不要提交真实生产密钥。

HMAC 开发密钥只适合实验室验证。如果密钥存放在普通 Flash 且可被 App 读取，它不能作为生产 secure boot root of trust。

生产版本需要明确密钥保护位置、读保护策略、debug lock 策略和回滚计数不可逆策略。
