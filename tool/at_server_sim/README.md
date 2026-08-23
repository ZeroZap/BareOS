# AT Server Simulator

用于通过串口模拟 4G/Wi-Fi 模组，验证 PLB-N32 的 BareOS AT client。工具支持普通命令响应、发送提示符、定长二进制数据、URC、响应分片以及超时/错误注入。

## 连接

PLB-N32 当前 AT 适配器使用 UART5。将板卡 UART5 TX/RX/GND 与电脑 USB 转串口交叉连接：

| PLB-N32 | USB 转串口 |
|---|---|
| UART5 TX | RX |
| UART5 RX | TX |
| GND | GND |

UART5 也可能由 SecBoot 开发传输占用。运行本工具前，应确保 APP 阶段只有 AT client 使用 UART5，并确认双方波特率一致。

## 安装与启动

```sh
python -m pip install -r tool/at_server_sim/requirements.txt
python tool/at_server_sim/at_server_sim.py --list
python tool/at_server_sim/at_server_sim.py --port COM8 --baud 115200 --profile ec2x
```

默认支持以下常用命令：

- `AT`、`ATE0`、`ATE1`
- `AT+CSQ`、`AT+CEREG?`、`AT+CREG?`
- `AT+CIMI`、`AT+CCID`、`AT+ICCID`
- `AT+CMEE=2`、`AT+CMGF=1`、`AT+CNMI=2,2,0,0,0`
- EC2X：`AT+QIOPEN=...`、`AT+QISEND=<link>,<len>`、`AT+QICLOSE=...`
- SIM76：`AT+CIPSEND=<link>,<len>`、`AT+CIPCLOSE=...`
- ESP-AT：初始化、连接和 `AT+CIPSEND=<link>,<len>`

未知命令返回 `ERROR`。`QISEND`/`CIPSEND` 先返回 `>`，随后严格接收声明长度的数据，再返回 `SEND OK`。

## 验证场景

### 基本命令与响应分片

每 3 字节发送一个响应分片，验证 AT client 能处理任意串口分包：

```sh
python tool/at_server_sim/at_server_sim.py --port COM8 --fragment-size 3 --fragment-delay-ms 10
```

### ERROR 与重试

对 `AT+CSQ` 始终返回 `ERROR`，并在 5 秒后校验板卡确实发送过该命令：

```sh
python tool/at_server_sim/at_server_sim.py --port COM8 --duration 5 --error-command "^AT\+CSQ$" --expect-command "^AT\+CSQ$"
```

日志中默认属性应出现总计 3 次 `AT+CSQ`：首次发送加 2 次重试。

### 超时与重试

丢弃 `AT+CSQ` 的所有响应：

```sh
python tool/at_server_sim/at_server_sim.py --port COM8 --duration 5 --drop-command "^AT\+CSQ$"
```

AT client 主循环必须持续调用 `plb_n32_at_process()`，系统毫秒 tick 也必须持续更新，否则超时和重试不会推进。

### 主动注入 URC

每秒发送一条 EC2X 接收 URC：

```sh
python tool/at_server_sim/at_server_sim.py --port COM8 --urc "+QIURC: \"recv\",0,5\nHELLO\r\n" --urc-interval 1
```

也可让板卡发送测试命令 `AT+SIMURC=RECV` 或 `AT+SIMURC=CLOSED`，模拟器在回复 `OK` 后立即注入对应 URC。

不同接收格式可用 `--profile ec2x`、`--profile sim76` 或 `--profile esp_at` 选择。

### 模拟慢响应

```sh
python tool/at_server_sim/at_server_sim.py --port COM8 --response-delay-ms 700
```

AT 组件普通命令默认超时为 500 ms。该配置可用于验证晚到响应和重试行为。

### QISEND 故障注入

不返回 `>`，验证 5 秒 prompt timeout：

```powershell
python "tool/at_server_sim/at_server_sim.py" --port COM24 --duration 12 --drop-prompt
```

接收完原始 payload 后返回 `ERROR`：

```powershell
python "tool/at_server_sim/at_server_sim.py" --port COM24 --duration 12 --send-result error
```

接收 payload 后不返回任何结果，验证 10 秒 data-result timeout：

```powershell
python "tool/at_server_sim/at_server_sim.py" --port COM24 --duration 18 --send-result drop
```

### URC 截断

只发送 URC header，缺少换行和 payload：

```powershell
python "tool/at_server_sim/at_server_sim.py" --port COM24 --duration 12 --urc-fault header
```

发送完整 header 但只发送部分 payload：

```powershell
python "tool/at_server_sim/at_server_sim.py" --port COM24 --duration 12 --urc-fault payload
```

带 URC deadline 的自检固件预期在 1.5 秒内输出：

```text
PLB-N32 AT selftest FAILED reason=URC_TIMEOUT
```

### 第三阶段板测矩阵

每条命令单独启动一次模拟器，模拟器开始监听后按一次板卡复位键。正常分片、prompt 和发送结果故障可直接复用已 confirmed 的 counter33 镜像。

| 用例 | 模拟器参数 | 板端预期 |
|---|---|---|
| 逐字节分片 | `--fragment-size 1 --fragment-delay-ms 5` | `AT selftest PASSED` |
| 丢失 prompt | `--drop-prompt` | `FAILED command=AT+QISEND`，约 5 秒 |
| payload 后 ERROR | `--send-result error` | `FAILED command=AT+QISEND` |
| payload 后无响应 | `--send-result drop` | `FAILED command=AT+QISEND`，约 10 秒 |
| URC header 截断 | `--urc-fault header` | `FAILED reason=URC_TIMEOUT` |
| URC payload 截断 | `--urc-fault payload` | `FAILED reason=URC_TIMEOUT` |

URC timeout 日志需要包含本节 deadline 修复的 counter34 或更新镜像。故障用例结束后应确认 `rb=0`、`drop=0`，且 UART5 RX window 最终关闭并重新允许 STOP2。

### 2026-08-03 counter36 验证记录

验证包：`PLB_at_selftest_counter36.sbp`，`image_version=2`，
`security_counter=36`。设备完成 mailbox confirmed 后，使用同一镜像反复复位执行
正常链路和故障注入，不重复升级相同 counter。

| 用例 | 结果 |
|---|---|
| 逐字节响应分片 | 通过，5 条命令全部命中 |
| QISEND prompt + 11 字节 payload | 通过，payload 为 `PLB-AT-DATA` |
| 二进制接收 URC | 通过，`id=0 len=5 payload=HELLO` |
| `AT+CSQ` ERROR + 两次重试 | 通过 |
| `AT+CSQ` timeout + 两次重试 | 通过 |
| QISEND prompt 丢失 | 通过，约 5 秒失败且未发送 payload |
| payload 后 ERROR | 通过，payload 仅发送一次 |
| payload 后无结果 | 通过，约 10 秒失败且未重发 payload |
| URC header 截断 | 通过，进入 `URC_TIMEOUT` |
| URC payload 截断 | 通过，进入 `URC_TIMEOUT` |
| UART/PM 收尾 | 通过，`rb=0 drop=0 locks=0/0/0/0`，RX window 关闭后恢复 STOP2 |

正常链路主机侧汇总为 `5 commands, 1 payloads`。板端最终输出：

```text
PLB-N32 AT selftest URC recv id=0 len=5 payload=HELLO
PLB-N32 AT selftest PASSED
PLB-N32 UART5 RX window closed, STOP2 allowed
```

本轮使用 `payload=256` 刷写时曾在 DATA 中途收到一次异常 `BAD_MANIFEST`；设备未提交
manifest，随后使用 `payload=128 --timeout-ms 2000 --retries 20` 完整恢复并得到
`END ACK detail=0x8007800`。该现象属于 SecBoot UART transport 稳定性跟踪项，不影响
AT 自检结论。

### UART TX ring 短写压力验证

`AT_SELFTEST_STRESS=y` 将 QISEND payload 扩展为 1024 字节，超过 N32 UART5 的
512 字节 TX ring。payload 使用确定性二进制模式：

```text
byte[i] = (i * 31 + 7) & 0xff
sha256 = 8d7e566766f6bd1bb4cac87cadfde681197f9243f4d2692a0fd12674092212a7
```

构建压力包：

```powershell
make -C "project/PLB -N32/Makefile" clean
make -C "project/PLB -N32/Makefile" package AT_SELFTEST_STRESS=y SECBOOT_IMAGE_VERSION=3 SECBOOT_SECURITY_COUNTER=37 SECBOOT_PACKAGE=build/PLB_at_tx_stress_counter37.sbp
```

模拟器自动校验 payload 长度和 SHA-256：

```powershell
python "tool/at_server_sim/at_server_sim.py" --port COM24 --baud 115200 --profile ec2x --duration 20 --expect-command "^AT$" --expect-command "^AT\+CSQ$" --expect-command "^AT\+CEREG\?$" --expect-command "^AT\+QISEND=0,1024$" --expect-command "^AT\+SIMURC=RECV$" --expect-payload-size 1024 --expect-payload-sha256 8d7e566766f6bd1bb4cac87cadfde681197f9243f4d2692a0fd12674092212a7
```

压力镜像要求板端至少观察到一次 adapter 短写，否则输出
`FAILED reason=NO_SHORT_WRITE`。通过时应输出：

```text
PLB-N32 AT selftest QISEND PASSED len=1024
PLB-N32 AT selftest TX calls=... short=... zero=...
PLB-N32 AT selftest PASSED
```

其中 `short` 必须大于 0；`zero` 允许为 0 或更大，取决于 TX ISR 与主循环的相对时序。

### 2026-08-03 counter37 验证记录

验证包：`PLB_at_tx_stress_counter37.sbp`，`image_version=3`，
`security_counter=37`。

| 项目 | 结果 |
|---|---|
| 1024 字节 payload 长度 | 通过 |
| payload SHA-256 | 通过，匹配 `8d7e5667...092212a7` |
| UART5 512 字节 TX ring 短写 | 通过，板端 `short > 0` |
| 短写续传完整性 | 通过，无丢失、重复或乱序 |
| payload 后 `SEND OK` | 通过，完整自检 `PASSED` |
| payload 后 `ERROR` | 通过，仅发送一次 payload 后正确失败 |
| payload 后无响应 | 通过，未重发 payload，约 10 秒后超时 |
| prompt 丢失 | 通过，未发送 payload，约 5 秒后超时 |
| UART/PM 收尾 | 通过，ring 清空、无 drop、无 PM lock 泄漏并恢复 STOP2 |

结论：N32 UART5 真实非阻塞 adapter 在 TX ring 饱和时能够由 AT 核心正确续传，
响应计时不会早于 payload 完整提交，发送期间和物理 TX 完成前的 PM 阻止逻辑有效。

### 两段 payload 饱和验证

`AT_SELFTEST_SEGMENTED=y` 模拟 SMS 的两段写入模型：第一段为 1024 字节确定性
二进制正文，第二段为单字节 Ctrl-Z (`0x1a`)。AT 核心必须在 UART5 TX ring
饱和和短写期间保持两段顺序。

```text
combined size   = 1025
combined sha256 = 0fffc5e6c9aafc593ea65582451fbd1e91fa3b1a7b67b9e38c2f9e7306236266
suffix          = 1a
```

构建 counter38 包：

```powershell
make -C "project/PLB -N32/Makefile" clean
make -C "project/PLB -N32/Makefile" package AT_SELFTEST_SEGMENTED=y SECBOOT_IMAGE_VERSION=4 SECBOOT_SECURITY_COUNTER=38 SECBOOT_PACKAGE=build/PLB_at_segmented_counter38.sbp
```

模拟器联合校验长度、SHA-256 和 Ctrl-Z 尾字节：

```powershell
python "tool/at_server_sim/at_server_sim.py" --port COM24 --baud 115200 --profile ec2x --duration 20 --expect-command "^AT$" --expect-command "^AT\+CSQ$" --expect-command "^AT\+CEREG\?$" --expect-command "^AT\+QISEND=0,1025$" --expect-command "^AT\+SIMURC=RECV$" --expect-payload-size 1025 --expect-payload-sha256 0fffc5e6c9aafc593ea65582451fbd1e91fa3b1a7b67b9e38c2f9e7306236266 --expect-payload-suffix-hex 1a
```

板端通过日志必须包含：

```text
PLB-N32 AT selftest QISEND PASSED len=1025 segments=2
PLB-N32 AT selftest TX calls=... short=... zero=...
PLB-N32 AT selftest PASSED
```

### 2026-08-03 counter38 验证记录

验证包：`PLB_at_segmented_counter38.sbp`，`image_version=4`，
`security_counter=38`。

| 项目 | 结果 |
|---|---|
| 两段入队 | 通过，1024 字节正文加独立 Ctrl-Z |
| UART TX ring 饱和短写 | 通过，板端 `short > 0` |
| 组合长度 | 通过，精确 1025 字节 |
| 组合 SHA-256 | 通过，匹配 `0fffc5e6...06236266` |
| 第二段位置 | 通过，最后一个字节为 `0x1a` |
| 两段顺序与完整性 | 通过，无丢失、重复、提前或乱序 |
| `SEND OK` | 通过，完整自检 `PASSED` |
| payload 后 `ERROR` | 通过，仅发送一次组合 payload 后正确失败 |
| payload 后无响应 | 通过，未重发任一段，约 10 秒后超时 |
| prompt 丢失 | 通过，两段均未发送，约 5 秒后超时 |
| UART/PM 收尾 | 通过，ring 清空、无 drop、无锁泄漏并恢复 STOP2 |

结论：两段 TX queue 在真实 UART 短写和 ring 饱和条件下维持严格 FIFO，覆盖了
SMS 正文加 Ctrl-Z 的核心发送模型。

### 部分发送中止验证

`AT_SELFTEST_ABORT=y` 在 1024 字节 payload 首次真实短写后调用
`at_work_abort_all()`。AT 核心应丢弃尚未提交 UART ring 的尾部，但已经被 adapter
接受的前缀仍会由 UART 硬件发送完成。板端只有在 work queue 清空且 transport
`tx_idle` 后才输出通过。

构建 counter39 包：

```powershell
make -C "project/PLB -N32/Makefile" clean
make -C "project/PLB -N32/Makefile" package AT_SELFTEST_ABORT=y SECBOOT_IMAGE_VERSION=5 SECBOOT_SECURITY_COUNTER=39 SECBOOT_PACKAGE=build/PLB_at_abort_counter39.sbp
```

模拟器要求收到非空但不完整的确定性 payload 前缀，长度不超过 512 字节：

```powershell
python "tool/at_server_sim/at_server_sim.py" --port COM24 --baud 115200 --profile ec2x --duration 12 --expect-command "^AT$" --expect-command "^AT\+CSQ$" --expect-command "^AT\+CEREG\?$" --expect-command "^AT\+QISEND=0,1024$" --expect-partial-payload-max 512 --expect-partial-stress-pattern
```

板端通过日志：

```text
PLB-N32 AT selftest abort requested accepted=...
PLB-N32 AT selftest ABORT PASSED accepted=... calls=... short=... zero=...
```

`accepted` 必须大于 0 且小于 1024。模拟器不得收到完整 payload，也不得出现
`SEND OK`；heartbeat 最终应为 `rb=0 drop=0 locks=0/0/0/0` 并恢复 STOP2。

### 2026-08-03 counter39 验证记录

验证包：`PLB_at_abort_counter39.sbp`，`image_version=5`，
`security_counter=39`。

| 项目 | 结果 |
|---|---|
| 首次真实短写后 abort | 通过 |
| 已接收前缀发送完成 | 通过，模拟器收到非空确定性前缀 |
| 未提交 payload 尾部 | 通过，未继续泄漏且未形成完整 payload |
| payload 前缀上限 | 通过，不超过 UART5 512 字节 TX ring |
| work queue 清理 | 通过，abort 后未重新排入 QISEND |
| transport idle 判定 | 通过，物理 TX 完成后才输出 `ABORT PASSED` |
| UART/PM 收尾 | 通过，ring 清空、无 drop、无锁泄漏并恢复 STOP2 |

结论：运行中 abort 能清除 AT 内部待发送尾部，同时允许 adapter 已接收字节安全排空；
work busy 和 PM transport 状态最终恢复，不会在后续轮询泄漏旧 payload。

### 连续 URC burst 验证

`AT_SELFTEST_URC_BURST=y` 让板端发送 `AT+SIMURC=BURST`，模拟器随后连续注入
32 个 EC2X 二进制 URC，payload 按 `B000` 到 `B031` 严格递增。该构建关闭 UART5
逐字节 UART4 日志，避免诊断输出干扰 RX 压力测试。

构建 counter40 包：

```powershell
make -C "project/PLB -N32/Makefile" clean
make -C "project/PLB -N32/Makefile" package AT_SELFTEST_URC_BURST=y SECBOOT_IMAGE_VERSION=6 SECBOOT_SECURITY_COUNTER=40 SECBOOT_PACKAGE=build/PLB_at_urc_burst_counter40.sbp
```

运行 32 帧 burst：

```powershell
python "tool/at_server_sim/at_server_sim.py" --port COM24 --baud 115200 --profile ec2x --duration 15 --urc-burst-count 32 --expect-command "^AT$" --expect-command "^AT\+CSQ$" --expect-command "^AT\+CEREG\?$" --expect-command "^AT\+QISEND=0,11$" --expect-command "^AT\+SIMURC=BURST$"
```

板端通过日志：

```text
PLB-N32 AT selftest URC burst progress=8/32
PLB-N32 AT selftest URC burst progress=16/32
PLB-N32 AT selftest URC burst progress=24/32
PLB-N32 AT selftest URC burst progress=32/32
PLB-N32 AT selftest URC BURST PASSED count=32 drop=0
```

任何缺帧、重复、乱序、payload 格式错误或 UART ring drop 都必须判失败。测试后
heartbeat 应为 `rb=0 drop=0 locks=0/0/0/0` 并恢复 STOP2。

#### Counter40 现场问题与 counter41 修复

counter40 板测发现两个自测状态机问题，因此该包废弃，不作为 URC burst 通过依据：

- 1.5 秒 timeout 从命令响应开始固定计算，逐字节慢分片在第 32 帧到达前被误判
  `URC_TIMEOUT`；随后 parser 仍收到第 32 帧。
- 第 32 帧到达后立即宣布通过，没有等待 burst 静默期，导致 128 帧过量输入在
  `PASSED` 后继续增长到 `128/32`，未被判为过量。

counter41 改为：

- 每收到一个合法帧刷新 1500 ms inactivity timeout；
- 收齐 32 帧后等待 500 ms settle 静默窗口；
- settle 期间收到第 33 帧立即输出 `URC_BURST_EXCESS`；
- settle 结束时还要求 UART RX transport 为空且 AT 对象不 busy。

构建 counter41：

```powershell
make -C "project/PLB -N32/Makefile" clean
make -C "project/PLB -N32/Makefile" package AT_SELFTEST_URC_BURST=y SECBOOT_IMAGE_VERSION=7 SECBOOT_SECURITY_COUNTER=41 SECBOOT_PACKAGE=build/PLB_at_urc_burst_fix_counter41.sbp
```

刷写 counter41。刷写工具退出并等待 confirmed 复位完成后，才能让模拟器打开 COM24：

```powershell
python "tool/xy_secboot/xy_secboot.py" flash --port COM24 --baud 115200 --package "project/PLB -N32/Makefile/build/PLB_at_urc_burst_fix_counter41.sbp" --payload 256 --timeout-ms 1000 --retries 10 --recover-ms 5000 --reset
```

counter41 依次执行三组复测，每次测试前复位板端以重新启动自检。

正常 32 帧，预期 `URC BURST PASSED count=32 drop=0`：

```powershell
python "tool/at_server_sim/at_server_sim.py" --port COM24 --baud 115200 --profile ec2x --duration 15 --urc-burst-count 32 --expect-command "^AT$" --expect-command "^AT\+CSQ$" --expect-command "^AT\+CEREG\?$" --expect-command "^AT\+QISEND=0,11$" --expect-command "^AT\+SIMURC=BURST$"
```

慢分片 32 帧，整个 burst 超过 1.5 秒但每个有效帧都会刷新 inactivity timeout；
预期仍通过，不能出现 `URC_TIMEOUT`：

```powershell
python "tool/at_server_sim/at_server_sim.py" --port COM24 --baud 115200 --profile ec2x --duration 20 --urc-burst-count 32 --fragment-size 4 --fragment-delay-ms 10 --expect-command "^AT$" --expect-command "^AT\+CSQ$" --expect-command "^AT\+CEREG\?$" --expect-command "^AT\+QISEND=0,11$" --expect-command "^AT\+SIMURC=BURST$"
```

过量 33 帧，预期失败并输出
`FAILED reason=URC_BURST_EXCESS received=32`，不能先输出 `PASSED`：

```powershell
python "tool/at_server_sim/at_server_sim.py" --port COM24 --baud 115200 --profile ec2x --duration 15 --urc-burst-count 33 --expect-command "^AT$" --expect-command "^AT\+CSQ$" --expect-command "^AT\+CEREG\?$" --expect-command "^AT\+QISEND=0,11$" --expect-command "^AT\+SIMURC=BURST$"
```

三组测试结束后均检查 UART4 heartbeat。正常和慢分片用例应恢复
`rb=0 drop=0 locks=0/0/0/0` 与 STOP2；过量用例也应排空 transport、无 ring drop，
但自检结论必须保持失败。

### Counter42 URC timeout 恢复验证

counter42 验证截断二进制 URC 触发组件内部 500 ms timeout 后，parser 状态能够复位，
后续合法 URC 仍可被完整解析。构建并刷写：

```powershell
make -C "project/PLB -N32/Makefile" clean
make -C "project/PLB -N32/Makefile" package AT_SELFTEST_URC_RECOVERY=y SECBOOT_IMAGE_VERSION=8 SECBOOT_SECURITY_COUNTER=42 SECBOOT_PACKAGE=build/PLB_at_urc_recovery_counter42.sbp
python "tool/xy_secboot/xy_secboot.py" flash --port COM24 --baud 115200 --package "project/PLB -N32/Makefile/build/PLB_at_urc_recovery_counter42.sbp" --payload 256 --timeout-ms 1000 --retries 10 --recover-ms 5000 --reset
```

刷写工具退出且 App confirmed 复位完成后运行模拟器。它先发送只有 `HEL` 的截断帧，
等待 700 ms，再发送完整 `HELLO`：

```powershell
python "tool/at_server_sim/at_server_sim.py" --port COM24 --baud 115200 --profile ec2x --duration 15 --urc-recovery-delay-ms 700 --expect-command "^AT$" --expect-command "^AT\+CSQ$" --expect-command "^AT\+CEREG\?$" --expect-command "^AT\+QISEND=0,11$" --expect-command "^AT\+SIMURC=RECOVER$"
```

UART4 预期按顺序出现：

```text
PLB-N32 AT selftest URC timeout observed
PLB-N32 AT selftest URC recv id=0 len=5 payload=HELLO
PLB-N32 AT selftest URC RECOVERY PASSED timeout=1 drop=0
```

测试后确认 `rb=0 drop=0 locks=0/0/0/0`、UART5 RX window 关闭且恢复 STOP2。

### Counter43 URC buffer overflow 恢复验证

counter43 向板端注入声明长度 300 的 EC2X 二进制 URC，完整帧长度超过板端
`urc_bufsize=256`。框架应丢弃超长帧并重新同步，随后正确解析 `HELLO`，不能把
超长 payload 当作有效业务帧。构建并刷写：

```powershell
make -C "project/PLB -N32/Makefile" clean
make -C "project/PLB -N32/Makefile" package AT_SELFTEST_URC_OVERFLOW=y SECBOOT_IMAGE_VERSION=9 SECBOOT_SECURITY_COUNTER=43 SECBOOT_PACKAGE=build/PLB_at_urc_overflow_counter43.sbp
python "tool/xy_secboot/xy_secboot.py" flash --port COM24 --baud 115200 --package "project/PLB -N32/Makefile/build/PLB_at_urc_overflow_counter43.sbp" --payload 256 --timeout-ms 1000 --retries 10 --recover-ms 5000 --reset
```

刷写工具退出且 App confirmed 复位完成后运行模拟器：

```powershell
python "tool/at_server_sim/at_server_sim.py" --port COM24 --baud 115200 --profile ec2x --duration 15 --urc-overflow-payload-size 300 --expect-command "^AT$" --expect-command "^AT\+CSQ$" --expect-command "^AT\+CEREG\?$" --expect-command "^AT\+QISEND=0,11$" --expect-command "^AT\+SIMURC=OVERFLOW$"
```

UART4 预期：

```text
PLB-N32 AT selftest URC recv id=0 len=5 payload=HELLO
PLB-N32 AT selftest URC OVERFLOW RECOVERY PASSED drop=0
```

不得出现 `URC_OVERFLOW_ACCEPTED`、`URC_OVERFLOW_DROP` 或 `URC_TIMEOUT`。测试后确认
`rb=0 drop=0 locks=0/0/0/0`、UART5 RX window 关闭且恢复 STOP2。

### Counter44 命令响应与 URC 交错验证

counter44 在 `AT+CSQ` 的 `+CSQ: 18,0` 与最终 `OK` 之间插入一条二进制接收 URC。
板端必须先执行 URC 回调，随后仍以 `AT_RESP_OK` 完成 `AT+CSQ`，且响应中保留有效
CSQ 内容。构建并刷写：

```powershell
make -C "project/PLB -N32/Makefile" clean
make -C "project/PLB -N32/Makefile" package AT_SELFTEST_URC_INTERLEAVE=y SECBOOT_IMAGE_VERSION=10 SECBOOT_SECURITY_COUNTER=44 SECBOOT_PACKAGE=build/PLB_at_urc_interleave_counter44.sbp
python "tool/xy_secboot/xy_secboot.py" flash --port COM24 --baud 115200 --package "project/PLB -N32/Makefile/build/PLB_at_urc_interleave_counter44.sbp" --payload 256 --timeout-ms 1000 --retries 10 --recover-ms 5000 --reset
```

刷写工具退出且 App confirmed 复位完成后运行模拟器：

```powershell
python "tool/at_server_sim/at_server_sim.py" --port COM24 --baud 115200 --profile ec2x --duration 15 --interleave-urc-on-csq --expect-command "^AT$" --expect-command "^AT\+CSQ$" --expect-command "^AT\+CEREG\?$" --expect-command "^AT\+QISEND=0,11$" --expect-command "^AT\+SIMURC=INTERLEAVE$"
```

UART4 预期：

```text
PLB-N32 AT selftest URC recv id=0 len=5 payload=HELLO
PLB-N32 AT selftest command=AT+CSQ code=0 len=...
PLB-N32 AT selftest interleave CSQ and URC received
PLB-N32 AT selftest URC INTERLEAVE PASSED drop=0
```

不得出现 `INTERLEAVE_CSQ_MISSING`、`INTERLEAVE_URC_LATE`、`INTERLEAVE_INCOMPLETE`
或 `INTERLEAVE_DROP`。测试后确认 `rb=0 drop=0 locks=0/0/0/0` 并恢复 STOP2。

### Counter45 QISEND prompt 与 URC 交错验证

counter45 在收到 `AT+QISEND=0,11` 后先注入二进制 `HELLO` URC，再返回 `>`。
板端必须在 QISEND work 活跃期间执行 URC 回调，随后继续发送 11 字节 payload 并收到
`SEND OK`。最终握手不能用于补齐 prompt 阶段缺失的 URC。构建并刷写：

```powershell
make -C "project/PLB -N32/Makefile" clean
make -C "project/PLB -N32/Makefile" package AT_SELFTEST_URC_PROMPT=y SECBOOT_IMAGE_VERSION=11 SECBOOT_SECURITY_COUNTER=45 SECBOOT_PACKAGE=build/PLB_at_urc_prompt_counter45.sbp
python "tool/xy_secboot/xy_secboot.py" flash --port COM24 --baud 115200 --package "project/PLB -N32/Makefile/build/PLB_at_urc_prompt_counter45.sbp" --payload 256 --timeout-ms 1000 --retries 10 --recover-ms 5000 --reset
```

刷写工具退出且 App confirmed 复位完成后运行模拟器：

```powershell
python "tool/at_server_sim/at_server_sim.py" --port COM24 --baud 115200 --profile ec2x --duration 15 --interleave-urc-before-prompt --expect-command "^AT$" --expect-command "^AT\+CSQ$" --expect-command "^AT\+CEREG\?$" --expect-command "^AT\+QISEND=0,11$" --expect-command "^AT\+SIMURC=PROMPT$" --expect-payload-size 11
```

UART4 预期：

```text
PLB-N32 AT selftest URC received before prompt
PLB-N32 AT selftest URC recv id=0 len=5 payload=HELLO
PLB-N32 AT selftest QISEND PASSED len=11 segments=1
PLB-N32 AT selftest URC PROMPT PASSED drop=0
```

不得出现 `PROMPT_URC_MISSING`、`PROMPT_URC_INCOMPLETE`、`PROMPT_URC_DROP` 或
`FAILED command=AT+QISEND`。测试后确认 `rb=0 drop=0 locks=0/0/0/0` 并恢复 STOP2。

### Counter46 SEND OK 与 URC 交错验证

counter46 在接收完整 11 字节 payload 后先注入二进制 `HELLO` URC，再返回
`SEND OK`。板端必须在 QISEND state 2 中执行 URC 回调，保持 result timeout 状态并
最终正常完成发送。构建并刷写：

```powershell
make -C "project/PLB -N32/Makefile" clean
make -C "project/PLB -N32/Makefile" package AT_SELFTEST_URC_SEND_RESULT=y SECBOOT_IMAGE_VERSION=12 SECBOOT_SECURITY_COUNTER=46 SECBOOT_PACKAGE=build/PLB_at_urc_send_result_counter46.sbp
python "tool/xy_secboot/xy_secboot.py" flash --port COM24 --baud 115200 --package "project/PLB -N32/Makefile/build/PLB_at_urc_send_result_counter46.sbp" --payload 256 --timeout-ms 1000 --retries 10 --recover-ms 5000 --reset
```

刷写工具退出且 App confirmed 复位完成后运行模拟器：

```powershell
python "tool/at_server_sim/at_server_sim.py" --port COM24 --baud 115200 --profile ec2x --duration 15 --interleave-urc-before-send-result --expect-command "^AT$" --expect-command "^AT\+CSQ$" --expect-command "^AT\+CEREG\?$" --expect-command "^AT\+QISEND=0,11$" --expect-command "^AT\+SIMURC=SENDRESULT$" --expect-payload-size 11
```

UART4 预期：

```text
PLB-N32 AT selftest URC received before SEND OK
PLB-N32 AT selftest URC recv id=0 len=5 payload=HELLO
PLB-N32 AT selftest QISEND PASSED len=11 segments=1
PLB-N32 AT selftest URC SEND RESULT PASSED drop=0
```

不得出现 `SEND_RESULT_URC_MISSING`、`SEND_RESULT_URC_INCOMPLETE`、
`SEND_RESULT_URC_DROP` 或 `FAILED command=AT+QISEND`。测试后确认
`rb=0 drop=0 locks=0/0/0/0` 并恢复 STOP2。

### Counter47 URC 伪 prompt 隔离验证

counter47 在真实 prompt 前发送 payload 为 `>` 的二进制 URC，并延迟真实 `>` 700 ms。
AT 组件必须从 command response buffer 中删除完整 URC 帧，禁止 URC payload 的控制
关键字驱动 QISEND 状态机。构建并刷写：

```powershell
make -C "project/PLB -N32/Makefile" clean
make -C "project/PLB -N32/Makefile" package AT_SELFTEST_URC_FAKE_PROMPT=y SECBOOT_IMAGE_VERSION=13 SECBOOT_SECURITY_COUNTER=47 SECBOOT_PACKAGE=build/PLB_at_urc_fake_prompt_counter47.sbp
python "tool/xy_secboot/xy_secboot.py" flash --port COM24 --baud 115200 --package "project/PLB -N32/Makefile/build/PLB_at_urc_fake_prompt_counter47.sbp" --payload 256 --timeout-ms 1000 --retries 10 --recover-ms 5000 --reset
```

刷写工具退出且 App confirmed 复位完成后运行模拟器：

```powershell
python "tool/at_server_sim/at_server_sim.py" --port COM24 --baud 115200 --profile ec2x --duration 15 --fake-prompt-urc-delay-ms 700 --expect-command "^AT$" --expect-command "^AT\+CSQ$" --expect-command "^AT\+CEREG\?$" --expect-command "^AT\+QISEND=0,11$" --expect-command "^AT\+SIMURC=FAKEPROMPT$" --expect-payload-size 11
```

UART4 预期：

```text
PLB-N32 AT selftest fake prompt URC received
PLB-N32 AT selftest QISEND PASSED len=11 segments=1
PLB-N32 AT selftest URC FAKE PROMPT PASSED drop=0
```

不得出现 `URC_FAKE_PROMPT_ACCEPTED`、`FAKE_PROMPT_URC_MISSING`、
`FAKE_PROMPT_INCOMPLETE`、`FAKE_PROMPT_DROP` 或 `FAILED command=AT+QISEND`。测试后
确认 `rb=0 drop=0 locks=0/0/0/0` 并恢复 STOP2。

### Counter49 URC 伪 ERROR 隔离验证

counter49 在 payload 接收完成后发送 payload 为 `ERROR` 的二进制 URC，并延迟真实
`SEND OK` 700 ms。AT 组件必须剔除 URC 帧，禁止其中的失败关键字提前终止 QISEND
state 2。构建并刷写：

```powershell
make -C "project/PLB -N32/Makefile" clean
make -C "project/PLB -N32/Makefile" package AT_SELFTEST_URC_FAKE_ERROR=y SECBOOT_IMAGE_VERSION=15 SECBOOT_SECURITY_COUNTER=49 SECBOOT_PACKAGE=build/PLB_at_urc_fake_error_counter49.sbp
python "tool/xy_secboot/xy_secboot.py" flash --port COM24 --baud 115200 --package "project/PLB -N32/Makefile/build/PLB_at_urc_fake_error_counter49.sbp" --payload 256 --timeout-ms 1000 --retries 10 --recover-ms 5000 --reset
```

刷写工具退出且 App confirmed 复位完成后运行模拟器：

```powershell
python "tool/at_server_sim/at_server_sim.py" --port COM24 --baud 115200 --profile ec2x --duration 15 --fake-error-urc-delay-ms 700 --expect-command "^AT$" --expect-command "^AT\+CSQ$" --expect-command "^AT\+CEREG\?$" --expect-command "^AT\+QISEND=0,11$" --expect-command "^AT\+SIMURC=FAKEERROR$" --expect-payload-size 11
```

UART4 预期：

```text
PLB-N32 AT selftest fake ERROR URC received
PLB-N32 AT selftest QISEND PASSED len=11 segments=1
PLB-N32 AT selftest URC FAKE ERROR PASSED drop=0
```

不得出现 `URC_FAKE_ERROR_ACCEPTED`、`FAKE_ERROR_URC_MISSING`、
`FAKE_ERROR_INCOMPLETE`、`FAKE_ERROR_DROP` 或 `FAILED command=AT+QISEND`。测试后确认
`rb=0 drop=0 locks=0/0/0/0` 并恢复 STOP2。

### Counter48 URC 伪 SEND OK 隔离验证

counter48 在 payload 接收完成后发送 payload 为 `SEND OK` 的二进制 URC，并延迟
真实 `SEND OK` 700 ms。AT 组件必须剔除 URC 帧，禁止其中的成功关键字提前完成
QISEND state 2。构建并刷写：

```powershell
make -C "project/PLB -N32/Makefile" clean
make -C "project/PLB -N32/Makefile" package AT_SELFTEST_URC_FAKE_SEND_OK=y SECBOOT_IMAGE_VERSION=14 SECBOOT_SECURITY_COUNTER=48 SECBOOT_PACKAGE=build/PLB_at_urc_fake_send_ok_counter48.sbp
python "tool/xy_secboot/xy_secboot.py" flash --port COM24 --baud 115200 --package "project/PLB -N32/Makefile/build/PLB_at_urc_fake_send_ok_counter48.sbp" --payload 256 --timeout-ms 1000 --retries 10 --recover-ms 5000 --reset
```

刷写工具退出且 App confirmed 复位完成后运行模拟器：

```powershell
python "tool/at_server_sim/at_server_sim.py" --port COM24 --baud 115200 --profile ec2x --duration 15 --fake-send-ok-urc-delay-ms 700 --expect-command "^AT$" --expect-command "^AT\+CSQ$" --expect-command "^AT\+CEREG\?$" --expect-command "^AT\+QISEND=0,11$" --expect-command "^AT\+SIMURC=FAKESENDOK$" --expect-payload-size 11
```

UART4 预期：

```text
PLB-N32 AT selftest fake SEND OK URC received
PLB-N32 AT selftest QISEND PASSED len=11 segments=1
PLB-N32 AT selftest URC FAKE SEND OK PASSED drop=0
```

不得出现 `URC_FAKE_SEND_OK_ACCEPTED`、`FAKE_SEND_OK_URC_MISSING`、
`FAKE_SEND_OK_INCOMPLETE`、`FAKE_SEND_OK_DROP` 或 `FAILED command=AT+QISEND`。测试后
确认 `rb=0 drop=0 locks=0/0/0/0` 并恢复 STOP2。

### Counter50 截断 URC 超时关键字隔离验证

counter50 在 QISEND payload 接收完成后注入 payload 为 `ERROR` 的二进制 URC，但
截去帧尾 `CRLF`。AT 组件必须暂停 QISEND work，禁止 partial URC 中的 `ERROR`
提前结束 state 2；500 ms URC timeout 窗口结束并清理该截断帧后，继续等待延迟到 700 ms 的
真实 `SEND OK`。构建并刷写：

```powershell
make -C "project/PLB -N32/Makefile" clean
make -C "project/PLB -N32/Makefile" package AT_SELFTEST_URC_PARTIAL_ERROR=y SECBOOT_IMAGE_VERSION=16 SECBOOT_SECURITY_COUNTER=50 SECBOOT_PACKAGE=build/PLB_at_urc_partial_error_counter50.sbp
python "tool/xy_secboot/xy_secboot.py" flash --port COM24 --baud 115200 --package "project/PLB -N32/Makefile/build/PLB_at_urc_partial_error_counter50.sbp" --payload 256 --timeout-ms 1000 --retries 10 --recover-ms 5000 --reset
```

刷写工具退出且 App confirmed 复位完成后运行模拟器：

```powershell
python "tool/at_server_sim/at_server_sim.py" --port COM24 --baud 115200 --profile ec2x --duration 15 --partial-error-urc-delay-ms 700 --expect-command "^AT$" --expect-command "^AT\+CSQ$" --expect-command "^AT\+CEREG\?$" --expect-command "^AT\+QISEND=0,11$" --expect-command "^AT\+SIMURC=PARTIALERROR$" --expect-payload-size 11
```

UART4 预期按顺序出现：

```text
PLB-N32 AT selftest partial ERROR URC timeout observed
PLB-N32 AT selftest QISEND PASSED len=11 segments=1
PLB-N32 AT selftest URC PARTIAL ERROR PASSED timeout=1 drop=0
```

不得出现 `PARTIAL_ERROR_TIMEOUT_MISSING`、`PARTIAL_ERROR_EARLY_RESULT`、
`PARTIAL_ERROR_INCOMPLETE`、`PARTIAL_ERROR_DROP` 或 `FAILED command=AT+QISEND`。
测试后确认 `rb=0 drop=0 locks=0/0/0/0` 并恢复 STOP2。

counter50 已完成实板验证，日志顺序、`timeout=1`、`drop=0`、heartbeat 和 STOP2
恢复均符合预期。

### Counter51 真实响应补齐截断 URC 帧尾验证

counter51 在 QISEND payload 完成后注入缺少尾部 `CRLF` 的 `ERROR` 二进制 URC，
但真实 `SEND OK` 在 300 ms 到达，早于 URC timeout。真实响应的前导 `CRLF` 会补齐
partial URC；组件必须先完成并剔除该 URC，再将同批数据中剩余的 `SEND OK` 交给
QISEND state 2。不得把 `ERROR` 当失败，也不得丢失真实结果或触发 URC timeout。

```powershell
make -C "project/PLB -N32/Makefile" clean
make -C "project/PLB -N32/Makefile" package AT_SELFTEST_URC_PARTIAL_TAIL=y SECBOOT_IMAGE_VERSION=17 SECBOOT_SECURITY_COUNTER=51 SECBOOT_PACKAGE=build/PLB_at_urc_partial_tail_counter51.sbp
python "tool/xy_secboot/xy_secboot.py" flash --port COM24 --baud 115200 --package "project/PLB -N32/Makefile/build/PLB_at_urc_partial_tail_counter51.sbp" --payload 256 --timeout-ms 1000 --retries 10 --recover-ms 5000 --reset
```

刷写工具退出且 App confirmed 复位完成后运行：

```powershell
python "tool/at_server_sim/at_server_sim.py" --port COM24 --baud 115200 --profile ec2x --duration 15 --partial-error-tail-delay-ms 300 --expect-command "^AT$" --expect-command "^AT\+CSQ$" --expect-command "^AT\+CEREG\?$" --expect-command "^AT\+QISEND=0,11$" --expect-command "^AT\+SIMURC=PARTIALTAIL$" --expect-payload-size 11
```

UART4 预期按顺序出现：

```text
PLB-N32 AT selftest partial ERROR URC completed by response tail
PLB-N32 AT selftest QISEND PASSED len=11 segments=1
PLB-N32 AT selftest URC PARTIAL TAIL PASSED timeout=0 drop=0
```

不得出现 `partial tail URC timeout unexpected`、`PARTIAL_TAIL_INCOMPLETE`、
`PARTIAL_TAIL_EARLY_RESULT`、`PARTIAL_TAIL_DROP` 或 `FAILED command=AT+QISEND`。
测试后确认 `rb=0 drop=0 locks=0/0/0/0` 并恢复 STOP2。

counter51 已完成实板验证，partial URC 由真实响应前导 `CRLF` 正常补齐，随后
QISEND 正确接收同批数据中的 `SEND OK`；`timeout=0`、`drop=0`、heartbeat 和
STOP2 恢复均符合预期。下一测试包从 `security_counter=52` 开始。

### Counter52 截断伪 SEND OK 后真实 ERROR 验证

counter52 在 QISEND state 2 注入 payload 为 `SEND OK`、但缺少尾部 `CRLF` 的
二进制 URC。组件必须暂停 work，禁止伪成功；500 ms timeout 清理 partial URC 后，
接受 700 ms 到达的真实 `ERROR`。该失败是测试预期结果，随后板端还会执行
`AT+SIMURC=PARTIALSENDOK`，验证失败 work 已回收且队列可以继续推进。

```powershell
make -C "project/PLB -N32/Makefile" clean
make -C "project/PLB -N32/Makefile" package AT_SELFTEST_URC_PARTIAL_SEND_OK=y SECBOOT_IMAGE_VERSION=18 SECBOOT_SECURITY_COUNTER=52 SECBOOT_PACKAGE=build/PLB_at_urc_partial_send_ok_counter52.sbp
python "tool/xy_secboot/xy_secboot.py" flash --port COM24 --baud 115200 --package "project/PLB -N32/Makefile/build/PLB_at_urc_partial_send_ok_counter52.sbp" --payload 256 --timeout-ms 1000 --retries 10 --recover-ms 5000 --reset
```

刷写工具退出且 App confirmed 复位完成后运行：

```powershell
python "tool/at_server_sim/at_server_sim.py" --port COM24 --baud 115200 --profile ec2x --duration 15 --partial-send-ok-urc-delay-ms 700 --send-result error --expect-command "^AT$" --expect-command "^AT\+CSQ$" --expect-command "^AT\+CEREG\?$" --expect-command "^AT\+QISEND=0,11$" --expect-command "^AT\+SIMURC=PARTIALSENDOK$" --expect-payload-size 11
```

UART4 预期按顺序出现：

```text
PLB-N32 AT selftest partial SEND OK URC timeout observed
PLB-N32 AT selftest QISEND expected ERROR received
PLB-N32 AT selftest command=AT+SIMURC=PARTIALSENDOK code=0 len=...
PLB-N32 AT selftest URC PARTIAL SEND OK PASSED timeout=1 error=1 drop=0
```

不得出现 `PARTIAL_SEND_OK_ACCEPTED`、`PARTIAL_SEND_OK_TIMEOUT_MISSING`、
`PARTIAL_SEND_OK_EARLY_ERROR`、`PARTIAL_SEND_OK_INCOMPLETE`、
`PARTIAL_SEND_OK_DROP` 或 `FAILED command=AT+QISEND`。测试后确认
`rb=0 drop=0 locks=0/0/0/0` 并恢复 STOP2。

counter52 已完成实板验证：截断伪 `SEND OK` 未提前完成 QISEND，partial URC timeout
后真实 `ERROR` 被正确接受，后续标记命令正常执行；`timeout=1`、`error=1`、
`drop=0`、heartbeat 和 STOP2 恢复均符合预期。下一测试包从
`security_counter=53` 开始。

### Counter53 真实 ERROR 补齐截断 SEND OK URC 帧尾验证

counter53 注入 payload 为 `SEND OK`、但缺少尾部 `CRLF` 的二进制 URC，300 ms
后发送真实 `ERROR`。真实响应的前导 `CRLF` 必须完成 partial URC；组件随后剔除
伪成功帧，并将同批剩余的 `ERROR` 交给 QISEND state 2。该 `ERROR` 是预期结果，
之后执行标记命令验证失败 work 已回收。整个过程不得触发 URC timeout。

```powershell
make -C "project/PLB -N32/Makefile" clean
make -C "project/PLB -N32/Makefile" package AT_SELFTEST_URC_PARTIAL_SEND_TAIL=y SECBOOT_IMAGE_VERSION=19 SECBOOT_SECURITY_COUNTER=53 SECBOOT_PACKAGE=build/PLB_at_urc_partial_send_tail_counter53.sbp
python "tool/xy_secboot/xy_secboot.py" flash --port COM24 --baud 115200 --package "project/PLB -N32/Makefile/build/PLB_at_urc_partial_send_tail_counter53.sbp" --payload 256 --timeout-ms 1000 --retries 10 --recover-ms 5000 --reset
```

刷写工具退出且 App confirmed 复位完成后运行：

```powershell
python "tool/at_server_sim/at_server_sim.py" --port COM24 --baud 115200 --profile ec2x --duration 15 --partial-send-ok-tail-delay-ms 300 --send-result error --expect-command "^AT$" --expect-command "^AT\+CSQ$" --expect-command "^AT\+CEREG\?$" --expect-command "^AT\+QISEND=0,11$" --expect-command "^AT\+SIMURC=PARTIALSENDTAIL$" --expect-payload-size 11
```

UART4 预期按顺序出现：

```text
PLB-N32 AT selftest partial SEND OK URC completed by ERROR tail
PLB-N32 AT selftest QISEND expected ERROR received after URC tail
PLB-N32 AT selftest command=AT+SIMURC=PARTIALSENDTAIL code=0 len=...
PLB-N32 AT selftest URC PARTIAL SEND TAIL PASSED timeout=0 error=1 drop=0
```

不得出现 `partial SEND tail URC timeout unexpected`、`PARTIAL_SEND_TAIL_ACCEPTED`、
`PARTIAL_SEND_TAIL_EARLY_ERROR`、`PARTIAL_SEND_TAIL_INCOMPLETE`、
`PARTIAL_SEND_TAIL_DROP` 或 `FAILED command=AT+QISEND`。测试后确认
`rb=0 drop=0 locks=0/0/0/0` 并恢复 STOP2。

counter53 已完成实板验证：真实 `ERROR` 前导 `CRLF` 正常补齐截断伪 `SEND OK`
URC，伪成功帧被剔除，同批 `ERROR` 被 QISEND 正确接受，后续标记命令正常执行；
`timeout=0`、`error=1`、`drop=0`、heartbeat 和 STOP2 恢复均符合预期。
下一测试包从 `security_counter=54` 开始。

### Counter54 迟到 retry 响应跨 work 隔离验证

counter54 让 `AT+SIMLATEA` 第一次响应延迟 700 ms，超过板端 500 ms timeout，迫使
同一 work 发送第二次请求。第一次迟到响应按同一 operation 的成功被接受；模拟器
随后把第二次请求的响应再延迟 300 ms，使其在下一条 `AT+SIMLATEB` 活跃期间到达。
下一 work 必须忽略迟到的 `+SIMLATEA` 响应，只能由自己的 `+SIMLATEB` 前缀完成。

相同命令的 retry 没有协议 transaction ID，因此无法区分 attempt1/attempt2；本用例
验证可保证的 operation 语义、单次回调，以及跨 work 的响应前缀隔离。

```powershell
make -C "project/PLB -N32/Makefile" clean
make -C "project/PLB -N32/Makefile" package AT_SELFTEST_LATE_RESPONSE=y SECBOOT_IMAGE_VERSION=20 SECBOOT_SECURITY_COUNTER=54 SECBOOT_PACKAGE=build/PLB_at_late_response_counter54.sbp
python "tool/xy_secboot/xy_secboot.py" flash --port COM24 --baud 115200 --package "project/PLB -N32/Makefile/build/PLB_at_late_response_counter54.sbp" --payload 256 --timeout-ms 1000 --retries 10 --recover-ms 5000 --reset
```

刷写工具退出且 App confirmed 复位完成后运行：

```powershell
python "tool/at_server_sim/at_server_sim.py" --port COM24 --baud 115200 --profile ec2x --duration 15 --late-response-isolation --expect-late-a-attempts 2 --expect-command "^AT$" --expect-command "^AT\+CSQ$" --expect-command "^AT\+CEREG\?$" --expect-command "^AT\+QISEND=0,11$" --expect-command "^AT\+SIMLATEA$" --expect-command "^AT\+SIMLATEB$" --expect-payload-size 11
```

UART4 预期：

```text
PLB-N32 AT selftest command=AT+SIMLATEA code=0 len=...
PLB-N32 AT selftest command=AT+SIMLATEB code=0 len=...
PLB-N32 AT selftest LATE RESPONSE PASSED attempts=2 callbacks=2 drop=0
```

不得出现 `LATE_A_RESPONSE_MISSING`、`LATE_RESPONSE_ISOLATION`、
`LATE_RESPONSE_DROP` 或任何通用 `FAILED`。最终确认
`rb=0 drop=0 locks=0/0/0/0` 并恢复 STOP2。

counter54 已完成实板验证：模拟器收到两次 `AT+SIMLATEA` 和一次
`AT+SIMLATEB`；A 仅回调一次，B 的 53 字节 response 同时保留迟到 A2 和自身
响应，但只由 `+SIMLATEB:` 前缀完成。UART5 RX 曾瞬时为 `rb=2`，最终 heartbeat
恢复 `rb=0 drop=0 locks=0/0/0/0` 并进入 STOP2。启动时出现的单字节噪声由模拟器
正确重同步到 `AT`，未影响结果。下一测试包从 `security_counter=55` 开始。

### Counter55 command response buffer 边界验证

counter55 验证 UART5 AT 对象 `recv_bufsize=256` 的精确边界。组件最多保存 255
字节响应和尾部 NUL：255 字节响应必须完整成功；256 和 257 字节响应必须明确返回
`AT_RESP_ERROR`，保留前 255 字节，禁止清零回卷后被尾部 `OK` 假成功。每次 overflow
后立即执行 `AT+SIMBUF=RECOVER`，确认 parser、work pool 和后续命令恢复。

```powershell
make -C "project/PLB -N32/Makefile" clean
make -C "project/PLB -N32/Makefile" package AT_SELFTEST_RESP_BOUNDARY=y SECBOOT_IMAGE_VERSION=21 SECBOOT_SECURITY_COUNTER=55 SECBOOT_PACKAGE=build/PLB_at_resp_boundary_counter55.sbp
python "tool/xy_secboot/xy_secboot.py" flash --port COM24 --baud 115200 --package "project/PLB -N32/Makefile/build/PLB_at_resp_boundary_counter55.sbp" --payload 256 --timeout-ms 1000 --retries 10 --recover-ms 5000 --reset
```

刷写工具退出且 App confirmed 复位完成后运行：

```powershell
python "tool/at_server_sim/at_server_sim.py" --port COM24 --baud 115200 --profile ec2x --duration 15 --expect-command "^AT$" --expect-command "^AT\+CSQ$" --expect-command "^AT\+CEREG\?$" --expect-command "^AT\+QISEND=0,11$" --expect-command "^AT\+SIMBUF=255$" --expect-command "^AT\+SIMBUF=256$" --expect-command "^AT\+SIMBUF=257$" --expect-command "^AT\+SIMBUF=RECOVER$" --expect-payload-size 11
```

UART4 预期：

```text
PLB-N32 AT selftest command=AT+SIMBUF=255 code=0 len=255
PLB-N32 AT selftest command=AT+SIMBUF=256 code=1 len=255
PLB-N32 AT selftest command=AT+SIMBUF=RECOVER code=0 len=25
PLB-N32 AT selftest command=AT+SIMBUF=257 code=1 len=255
PLB-N32 AT selftest command=AT+SIMBUF=RECOVER code=0 len=25
PLB-N32 AT selftest RESPONSE BOUNDARY PASSED ok=255 overflow=256/257 recover=2 drop=0
```

不得出现 `RESP_BOUNDARY`、`RESP_BOUNDARY_CONTENT`、`RESP_BOUNDARY_DROP` 或其他
通用 `FAILED`。最终确认 `rb=0 drop=0 locks=0/0/0/0` 并恢复 STOP2。

counter55 已完成实板验证：255 字节响应完整成功，256/257 字节响应均明确返回
`AT_RESP_ERROR` 且保留 `len=255`；257 字节响应尾部排空后，恢复命令保持
`code=0 len=25`，无残留污染。测试期间 UART RX ring 峰值至少达到 `rb=10`，最终
`rb=0 drop=0` 并恢复 STOP2。下一测试包从 `security_counter=56` 开始。

### Counter56 TX 连续零写与恢复验证

counter56 在板端 AT adapter 注入两次确定性 TX stall：QISEND 命令行首次写入时连续
700 ms 返回 0，收到 prompt 后 payload 首次写入时再连续 700 ms 返回 0。stall
恢复后必须只发送一条完整 `AT+QISEND=0,11` 和一份 11 字节 payload；命令响应计时
必须从命令行完整提交后开始，QISEND result timeout 必须从 payload 完整提交后开始。
两段 stall 期间 `at_obj_pm_can_sleep()` 必须保持 false。

```powershell
make -C "project/PLB -N32/Makefile" clean
make -C "project/PLB -N32/Makefile" package AT_SELFTEST_TX_ZERO_STALL=y SECBOOT_IMAGE_VERSION=22 SECBOOT_SECURITY_COUNTER=56 SECBOOT_PACKAGE=build/PLB_at_tx_zero_stall_counter56.sbp
python "tool/xy_secboot/xy_secboot.py" flash --port COM24 --baud 115200 --package "project/PLB -N32/Makefile/build/PLB_at_tx_zero_stall_counter56.sbp" --payload 256 --timeout-ms 1000 --retries 10 --recover-ms 5000 --reset
```

刷写工具退出且 App confirmed 复位完成后运行：

```powershell
python "tool/at_server_sim/at_server_sim.py" --port COM24 --baud 115200 --profile ec2x --duration 15 --expect-command "^AT$" --expect-command "^AT\+CSQ$" --expect-command "^AT\+CEREG\?$" --expect-command "^AT\+QISEND=0,11$" --expect-command "^AT\+SIMURC=RECV$" --expect-payload-size 11 --expect-payload-sha256 79511600d60ecac6bae80a0970ff663f29496a02c2960f7a5723f5d6fbf51675
```

UART4 预期：

```text
PLB-N32 AT selftest TX command zero stall start
PLB-N32 AT selftest TX command zero stall recovered elapsed=700
PLB-N32 AT selftest TX payload zero stall start
PLB-N32 AT selftest TX payload zero stall recovered elapsed=700
PLB-N32 AT selftest QISEND PASSED len=11 segments=1
PLB-N32 AT selftest TX ZERO STALL PASSED command=700 payload=700 zero=... drop=0
```

不得出现 `TX_ZERO_STALL_INCOMPLETE`、`TX_ZERO_STALL_PM`、`TX_ZERO_STALL_DROP`、
prompt/result timeout 或其他通用 `FAILED`。最终确认
`rb=0 drop=0 locks=0/0/0/0` 并恢复 STOP2。

counter56 已完成实板验证：命令行和 payload 两段 700 ms 连续零写均正常恢复，
QISEND 命令与 11 字节 payload 无丢失或重复，stall 期间 PM 保持禁止睡眠；最终
`drop=0`、heartbeat 和 STOP2 恢复均符合预期。下一测试包从
`security_counter=57` 开始。

### Counter57 多 work 队列、优先级和静态池回收验证

counter57 是纯板端确定性测试，不需要启动 `at_server_sim.py`。测试一次性填满 8 项
全局静态 work pool，验证第 9 项被拒绝；队列中 low 先入队、high 后入队，实际执行
必须为 high FIFO 4 项后接 low FIFO 4 项。随后再次填满队列并调用
`at_work_abort_all()`，8 个 context 必须立即进入 `AT_WORK_STAT_ABORT/AT_RESP_ABORT`，
且不经过额外 process 就能重新填满 8 项。重新执行完毕后再提交 1 个探针，确认静态池
已完整回收。

```powershell
make -C "project/PLB -N32/Makefile" clean
make -C "project/PLB -N32/Makefile" package AT_SELFTEST_WORK_QUEUE=y SECBOOT_IMAGE_VERSION=23 SECBOOT_SECURITY_COUNTER=57 SECBOOT_PACKAGE=build/PLB_at_work_queue_counter57.sbp
python "tool/xy_secboot/xy_secboot.py" flash --port COM24 --baud 115200 --package "project/PLB -N32/Makefile/build/PLB_at_work_queue_counter57.sbp" --payload 256 --timeout-ms 1000 --retries 10 --recover-ms 5000 --reset
```

UART4 预期按顺序出现：

```text
PLB-N32 AT selftest work queue full rejection passed
PLB-N32 AT selftest abort all and immediate requeue passed
PLB-N32 AT selftest WORK QUEUE PASSED order=H4/L4 full=8 reject=1 abort=8 requeue=8 recycle=1 drop=0
```

不得出现 `WORK_QUEUE_FILL`、`WORK_QUEUE_PRIORITY`、`WORK_QUEUE_ABORT_REQUEUE`、
`WORK_QUEUE_REQUEUE_ORDER`、`WORK_POOL_RECYCLE`、`WORK_QUEUE_FINAL` 或其他通用
`FAILED`。最终确认 heartbeat 为 `rb=0 drop=0 locks=0/0/0/0` 且恢复 STOP2。

counter57 已完成实板验证：high/low FIFO 顺序、8 项满队列、第 9 项拒绝、abort all
context 状态、立即重新入队和最终静态池探针均符合预期；最终 `drop=0`、heartbeat
和 STOP2 正常恢复。下一测试包从 `security_counter=58` 开始。

### Counter58 URC 精确边界和畸形长度恢复验证

counter58 首次实板测试未通过，不作为本轮通过依据。日志显示最大帧和越界帧分别被
业务层报告为 `len=232/233`，最终 `RECOVER` 被报告为 `len=9`。根因是
`at_urc_recv_split()` 将尾部 CRLF 计入 `out_payload_len`；同时首次测试将串口完整帧
开销误用于 parser 内部边界计算。counter59 修复 payload 长度契约，并按 parser 已
丢弃前导 CRLF 后的真实内部边界 232/233 重新测试。原计划的多对象共享池测试顺延。

counter58 验证 UART5 AT 对象 `urc_bufsize=256` 的精确边界及畸形长度恢复。框架会先
丢弃 EC2X URC 的前导 CRLF，因此 parser 内部帧固定开销为 23 字节。payload 232
字节时内部帧为 255 字节，必须完整进入业务回调；payload 233 字节时内部帧为 256
字节，必须在接收阶段拒绝。随后同批
依次注入声明长度 `0`、`-1`、`2147483647`、声明 5 但只有 3 字节的短 payload，
以及声明 3 但发送 4 字节的多 payload。短 payload 必须触发一次 500 ms timeout；
其余异常帧不得进入业务成功路径。最后的 `RECOVER` 合法帧必须正常接收，证明 parser
已重新同步。

```powershell
make -C "project/PLB -N32/Makefile" clean
make -C "project/PLB -N32/Makefile" package AT_SELFTEST_URC_BOUNDARY=y SECBOOT_IMAGE_VERSION=25 SECBOOT_SECURITY_COUNTER=59 SECBOOT_PACKAGE=build/PLB_at_urc_boundary_fix_counter59.sbp GCC_PATH="D:/Program Files (x86)/GNU Arm Embedded Toolchain/10 2021.10/bin/"
python "tool/xy_secboot/xy_secboot.py" flash --port COM24 --baud 115200 --package "project/PLB -N32/Makefile/build/PLB_at_urc_boundary_fix_counter59.sbp" --payload 256 --timeout-ms 1000 --retries 10 --recover-ms 5000 --reset
```

刷写工具退出且 App confirmed 复位完成后运行模拟器：

```powershell
python "tool/at_server_sim/at_server_sim.py" --port COM24 --baud 115200 --profile ec2x --duration 20 --urc-recovery-delay-ms 700 --expect-command "^AT$" --expect-command "^AT\+CSQ$" --expect-command "^AT\+CEREG\?$" --expect-command "^AT\+QISEND=0,11$" --expect-command "^AT\+SIMURC=BOUNDARY$" --expect-payload-size 11 --expect-payload-sha256 79511600d60ecac6bae80a0970ff663f29496a02c2960f7a5723f5d6fbf51675
```

UART4 预期按顺序出现：

```text
PLB-N32 AT selftest URC boundary max legal received len=232
PLB-N32 AT selftest URC boundary short payload timeout observed
PLB-N32 AT selftest URC boundary recovery received len=7
PLB-N32 AT selftest URC BOUNDARY PASSED max=232 over=233 malformed=0/-1/huge short=1 long=1 recover=1 drop=0
```

不得出现 `unexpected boundary URC`、`URC_BOUNDARY_COUNTS`、`URC_BOUNDARY_DROP`、
通用 `URC_TIMEOUT` 或其他 `FAILED`。最终确认 heartbeat 为
`rb=0 drop=0 locks=0/0/0/0`，UART5 RX window 关闭且恢复 STOP2。

counter59 修复复测已完成实板验证：最大合法 payload 232 字节被完整接收，233 字节
及 `0/-1/2147483647`、短 payload、多 payload 均未进入业务成功路径；短帧只触发
一次预期 timeout，最终 `RECOVER len=7` 正常接收。最终 `drop=0`、heartbeat 和
STOP2 恢复均符合预期。下一测试包从 `security_counter=60` 开始。

### Counter60 多 AT 对象共享静态池验证

counter60 首次实板测试未进入自检，连续两次启动均输出
`PLB-N32 AT client init failed`。SecBoot counter60 安装、pending boot、mailbox confirm
和 App 跳转均正常。根因不是 6 KB allocator 物理池不足，而是 AT 组件默认
`AT_MEM_LIMIT_SIZE=3 KB` 软件上限使第三个对象创建失败。counter61 仅在多对象测试
构建将该上限提升到 6 KB，并输出三个对象创建后的当前/峰值 AT 内存统计。

counter60 创建三个独立 AT 对象，模拟产品最终的 4G、GNSS 和卫星链路。三个对象
共享组件全局 8 项静态 work pool，按 A=3、B=3、C=2 填满后，第 9 项必须被拒绝。
随后只对 B 调用 `at_work_abort_all()`：B 的三个 context 必须变为
`AT_WORK_STAT_ABORT/AT_RESP_ABORT`，A/C 的 context 和队列不能改变；B 必须能立即
重新入队三项。最终执行计数必须为 A=3、B=3、C=2，三个对象全部 idle 后统一 PM
检查才允许 STOP2。

该测试全部使用无 I/O custom work，不需要运行 `at_server_sim.py`，避免多个测试对象
竞争 UART5 read adapter。

```powershell
make -C "project/PLB -N32/Makefile" clean
make -C "project/PLB -N32/Makefile" package AT_SELFTEST_MULTI_OBJ=y SECBOOT_IMAGE_VERSION=27 SECBOOT_SECURITY_COUNTER=61 SECBOOT_PACKAGE=build/PLB_at_multi_obj_fix_counter61.sbp GCC_PATH="D:/Program Files (x86)/GNU Arm Embedded Toolchain/10 2021.10/bin/"
python "tool/xy_secboot/xy_secboot.py" flash --port COM24 --baud 115200 --package "project/PLB -N32/Makefile/build/PLB_at_multi_obj_fix_counter61.sbp" --payload 256 --timeout-ms 1000 --retries 10 --recover-ms 5000 --reset
```

UART4 预期按顺序出现：

```text
PLB-N32 AT multi object create passed count=3 mem=.../...
PLB-N32 AT selftest multi object pool competition passed total=8 reject=1 abort=B3 requeue=B3
PLB-N32 AT selftest MULTI OBJECT PASSED pool=8 A=3 B=3 C=2 isolated=1 idle=3 drop=0
```

不得出现 `MULTI_OBJ_POOL_FILL`、`MULTI_OBJ_ABORT_CONTEXT`、`MULTI_OBJ_REQUEUE`、
`MULTI_OBJ_ISOLATION`、`MULTI_OBJ_FINAL` 或其他 `FAILED`。最终确认 heartbeat 为
`rb=0 drop=0 locks=0/0/0/0` 且恢复 STOP2。

counter61 修复复测已完成实板验证：三个 AT 对象创建成功，全局 8 项静态池竞争、
第 9 项拒绝、B 对象独立 abort 和立即重新入队、A/C 隔离及最终 A=3/B=3/C=2
执行计数均符合预期。三个对象全部 idle 后 PM 正常放行，`drop=0`、heartbeat 和
STOP2 恢复正常。下一测试包从 `security_counter=62` 开始。

### Counter62 adapter error 回调完整性验证

counter62 修复并验证 adapter `.error()` 回调。旧实现会在填充 `at_response_t` 之前
调用 error hook，导致 hook 读取未初始化的 `obj/params/code/recvbuf/recvcnt`。修复后
error hook 和普通 work callback 使用同一份已初始化 response，并保证 error hook
先调用。

板端依次执行显式 `ERROR`、无响应 timeout、256 字节 response overflow 和正常恢复
四条命令。前三条必须各调用一次 error hook，普通 callback 必须调用四次；双方看到
的对象、参数、状态、buffer 指针和长度必须一致。恢复命令不得调用 error hook。

```powershell
make -C "project/PLB -N32/Makefile" clean
make -C "project/PLB -N32/Makefile" package AT_SELFTEST_ERROR_CALLBACK=y SECBOOT_IMAGE_VERSION=28 SECBOOT_SECURITY_COUNTER=62 SECBOOT_PACKAGE=build/PLB_at_error_callback_counter62.sbp GCC_PATH="D:/Program Files (x86)/GNU Arm Embedded Toolchain/10 2021.10/bin/"
python "tool/xy_secboot/xy_secboot.py" flash --port COM24 --baud 115200 --package "project/PLB -N32/Makefile/build/PLB_at_error_callback_counter62.sbp" --payload 256 --timeout-ms 1000 --retries 10 --recover-ms 5000 --reset
```

刷写工具退出且 App confirmed 复位完成后运行模拟器：

```powershell
python "tool/at_server_sim/at_server_sim.py" --port COM24 --baud 115200 --profile ec2x --duration 15 --expect-command "^AT\+SIMERR=ERROR$" --expect-command "^AT\+SIMERR=TIMEOUT$" --expect-command "^AT\+SIMBUF=256$" --expect-command "^AT\+SIMBUF=RECOVER$"
```

UART4 预期：

```text
PLB-N32 AT selftest error callback step=0 code=1 len=9 hook=1
PLB-N32 AT selftest error callback step=1 code=2 len=0 hook=2
PLB-N32 AT selftest error callback step=2 code=1 len=255 hook=3
PLB-N32 AT selftest error callback step=3 code=0 len=25 hook=3
PLB-N32 AT selftest ERROR CALLBACK PASSED error=1 timeout=1 overflow=1 recover=1 hook=3 callback=4 drop=0
```

不得出现 `ERROR_CALLBACK_FIELDS`、`ERROR_CALLBACK_FINAL` 或其他 `FAILED`。最终确认
heartbeat 为 `rb=0 drop=0 locks=0/0/0/0` 且恢复 STOP2。

counter62 已完成实板验证：显式 ERROR、timeout、response overflow 三类失败均先向
adapter error hook 提交完整 response，再调用普通 callback；正常恢复命令未误调用
error hook。最终 `hook=3 callback=4 drop=0`，UART ring 排空、RX window 关闭且
STOP2 正常恢复。下一测试包从 `security_counter=63` 开始。

### Counter63 AT 对象销毁、context abort 与重建验证

counter63 修复并验证 `at_obj_destroy()` 对 active 和 queued work 的生命周期处理。
旧实现会释放 work block，但关联 `at_context_t` 仍停留在 `RUN/READY`，调用方无法
知道对象已销毁。修复后 destroy 会先将所有 context 统一设置为
`AT_WORK_STAT_ABORT/AT_RESP_ABORT`，清除 pending TX，再释放 work、recv/URC buffer
和对象内存。

板端先填满 8 项静态 pool，推进第一项到 RUN 后直接销毁对象，验证 1 个 active 和
7 个 queued context 全部 abort，AT memory watcher current usage 回到 0。随后立即
重建对象，内存使用必须恢复到相同单对象基线，再填满并执行 8 项，确认全局静态 pool
和对象 allocator 均完整回收。

本轮是纯板端测试，不需要运行 `at_server_sim.py`。

```powershell
make -C "project/PLB -N32/Makefile" clean
make -C "project/PLB -N32/Makefile" package AT_SELFTEST_DESTROY=y SECBOOT_IMAGE_VERSION=29 SECBOOT_SECURITY_COUNTER=63 SECBOOT_PACKAGE=build/PLB_at_destroy_recreate_counter63.sbp GCC_PATH="D:/Program Files (x86)/GNU Arm Embedded Toolchain/10 2021.10/bin/"
python "tool/xy_secboot/xy_secboot.py" flash --port COM24 --baud 115200 --package "project/PLB -N32/Makefile/build/PLB_at_destroy_recreate_counter63.sbp" --payload 256 --timeout-ms 1000 --retries 10 --recover-ms 5000 --reset
```

UART4 预期：

```text
PLB-N32 AT selftest destroy active=1 queued=7 contexts=8 memory=0 recreate=1 pool=8
PLB-N32 AT selftest DESTROY RECREATE PASSED abort=8 recreate=1 refill=8 runs=8 drop=0
```

不得出现 `DESTROY_FILL`、`DESTROY_ACTIVE_MISSING`、`DESTROY_CONTEXT`、
`DESTROY_MEMORY`、`DESTROY_RECREATE`、`DESTROY_RECREATE_ORDER`、`DESTROY_FINAL` 或
其他 `FAILED`。最终确认 heartbeat 为 `rb=0 drop=0 locks=0/0/0/0` 且恢复 STOP2。

counter63 已完成实板验证：1 个 active 和 7 个 queued work 在 destroy 时全部转为
abort，8 个 context 状态正确，对象动态内存回到 0；对象立即重建后全局静态 pool
重新填满并完整执行 8 项，最终 `drop=0`。SecBoot counter63 pending/confirm 流程正常。
下一测试包从 `security_counter=64` 开始。

### Counter64 AT uint32 tick 回卷验证

counter64 在每个阶段开始前将 BareOS tick 设置到 `0xffffff00`，实际跨越
`0xffffffff -> 0`，验证所有 AT 时间差使用无符号回卷语义：

- custom work 的 `next_wait(300 ms)` 必须只运行一次；
- command `timeout=300 ms, retry=1` 必须发送两次并只回调一次 timeout；
- 截断二进制 URC 必须跨回卷触发一次 500 ms timeout，随后合法 `RECOVER` 仍被接收；
- 完成后恢复正常 tick 区间，AT 对象和 transport idle，PM 可重新进入 STOP2。

```powershell
make -C "project/PLB -N32/Makefile" clean
make -C "project/PLB -N32/Makefile" package AT_SELFTEST_TICK_WRAP=y SECBOOT_IMAGE_VERSION=30 SECBOOT_SECURITY_COUNTER=64 SECBOOT_PACKAGE=build/PLB_at_tick_wrap_counter64.sbp GCC_PATH="D:/Program Files (x86)/GNU Arm Embedded Toolchain/10 2021.10/bin/"
python "tool/xy_secboot/xy_secboot.py" flash --port COM24 --baud 115200 --package "project/PLB -N32/Makefile/build/PLB_at_tick_wrap_counter64.sbp" --payload 256 --timeout-ms 1000 --retries 10 --recover-ms 5000 --reset
```

刷写工具退出且 App confirmed 复位完成后运行模拟器：

```powershell
python "tool/at_server_sim/at_server_sim.py" --port COM24 --baud 115200 --profile ec2x --duration 15 --urc-recovery-delay-ms 700 --expect-wrap-attempts 2 --expect-command "^AT\+SIMWRAP=TIMEOUT$" --expect-command "^AT\+SIMWRAP=URC$"
```

UART4 预期：

```text
PLB-N32 AT selftest tick wrap next_wait passed now=...
PLB-N32 AT selftest tick wrap timeout retry passed callbacks=1 now=...
PLB-N32 AT selftest TICK WRAP PASSED next_wait=1 retry=2 timeout=1 urc_timeout=1 recover=1 drop=0
```

前两条日志中的 `now` 必须是低值，证明操作确实跨过 uint32 回卷。不得出现
`TICK_WRAP` 或 `TICK_WRAP_FINAL` 的 `FAILED`。最终确认 heartbeat 为
`rb=0 drop=0 locks=0/0/0/0` 且恢复 STOP2。

counter64 已完成实板验证：`next_wait(300 ms)`、command timeout/retry 和二进制
URC timeout/recovery 均实际跨越 `0xffffffff -> 0` 后按预期触发；命令发送两次且
只回调一次 timeout，URC 后续恢复成功，最终 `drop=0`、heartbeat 和 STOP2 正常。
下一测试包从 `security_counter=65` 开始。

### Counter65 URC 重叠前缀、表顺序和 payload 隔离验证

counter65 修复 URC table 的重叠前缀选择。旧实现使用表中首个 `strstr()` 命中项，
短前缀排在长前缀之前时会抢占长帧，且 payload 中出现其他前缀也可能造成歧义。
修复后只从去除前导 CRLF 后的 URC 起始位置匹配，并在所有命中项中选择最长前缀；
相同长度才保持表顺序。

测试表故意将短项 `+SIM:` 放在长项 `+SIM: LONG` 之前。模拟器依次发送长帧、短帧、
payload 内再次包含 `+SIM:` 的长帧、未知 `+SIMX:` 畸形帧和恢复长帧。最终必须为
long=3、short=1、embedded=1、malformed=0、recovery=1。

```powershell
make -C "project/PLB -N32/Makefile" clean
make -C "project/PLB -N32/Makefile" package AT_SELFTEST_URC_PREFIX=y SECBOOT_IMAGE_VERSION=31 SECBOOT_SECURITY_COUNTER=65 SECBOOT_PACKAGE=build/PLB_at_urc_prefix_counter65.sbp GCC_PATH="D:/Program Files (x86)/GNU Arm Embedded Toolchain/10 2021.10/bin/"
python "tool/xy_secboot/xy_secboot.py" flash --port COM24 --baud 115200 --package "project/PLB -N32/Makefile/build/PLB_at_urc_prefix_counter65.sbp" --payload 256 --timeout-ms 1000 --retries 10 --recover-ms 5000 --reset
```

刷写工具退出且 App confirmed 复位完成后运行模拟器：

```powershell
python "tool/at_server_sim/at_server_sim.py" --port COM24 --baud 115200 --profile ec2x --duration 15 --expect-command "^AT\+SIMURC=PREFIX$"
```

UART4 预期：

```text
PLB-N32 AT selftest URC PREFIX PASSED long=3 short=1 embedded=1 malformed=0 recovery=1 drop=0
```

不得出现 `URC_PREFIX_DISPATCH`、`URC_PREFIX_FINAL` 或其他 `FAILED`。最终确认 heartbeat
为 `rb=0 drop=0 locks=0/0/0/0` 且恢复 STOP2。

## 建议的板端验证顺序

1. 显式调用 `plb_n32_at_init()`，并在主循环持续调用 `plb_n32_at_process()`。
2. 依次提交 `AT`、`AT+CSQ`、`AT+CEREG?`，检查完成回调中的 `AT_RESP_OK` 和响应内容。
3. 通过 cell EC2X backend 提交发送，确认 `QISEND -> > -> 原始数据 -> SEND OK` 完整交互。
4. 使用 `--error-command` 验证 `AT_RESP_ERROR`，使用 `--drop-command` 验证 `AT_RESP_TIMEOUT`。
5. 使用 `--fragment-size 1` 验证逐字节响应，最后注入接收/关闭 URC 验证异步路径。

不要在 `at_exec_cmd()` 后阻塞等待结果；BareOS AT client 需要前台轮询推进工作队列。

## SecBoot-N32 板卡

SecBoot bootloader 和 PLB App 在不同启动阶段复用 UART5/COM24：

- SecBoot recovery 阶段：COM24 由 `tool/xy_secboot` 独占，用于刷写 `.sbp`。
- App 启动后：Bootloader 已停止运行，COM24 可由本 AT Server 模拟器独占。
- 两个上位机程序不能同时打开 COM24。

设备当前已安装 `security_counter=31` 时，新测试包必须使用 32 或更高 counter。构建带一次性 AT 自检的 counter32 包：

```powershell
make -C "project/PLB -N32/Makefile" clean
make -C "project/PLB -N32/Makefile" AT_SELFTEST=y SECBOOT_SECURITY_COUNTER=32 SECBOOT_IMAGE_VERSION=2 SECBOOT_PACKAGE=build/PLB_at_selftest_counter32.sbp inspect-package
```

先启动 SecBoot 刷写，命令执行后在 recovery 时间内人工复位：

```powershell
python "tool/xy_secboot/xy_secboot.py" flash --port COM24 --baud 115200 --package "project/PLB -N32/Makefile/build/PLB_at_selftest_counter32.sbp" --payload 256 --timeout-ms 1000 --retries 10 --recover-ms 5000 --reset
```

看到 `END ACK` 后等待设备完成 App confirmed 复位。关闭刷写进程，确认 UART4/COM16 出现 `PLB-N32 main loop start`，再启动模拟器：

```powershell
python "tool/at_server_sim/at_server_sim.py" --port COM24 --baud 115200 --profile ec2x --duration 15 --expect-command "^AT$" --expect-command "^AT\+CSQ$" --expect-command "^AT\+CEREG\?$"
```

由于 App 自检可能在模拟器打开前已经开始并进入重试，建议在执行模拟器命令后再按一次复位键。UART4/COM16 的最终预期日志为：

```text
PLB-N32 AT selftest start
PLB-N32 AT selftest command=AT code=0 len=...
PLB-N32 AT selftest command=AT+CSQ code=0 len=...
PLB-N32 AT selftest command=AT+CEREG? code=0 len=...
PLB-N32 AT selftest PASSED
```

若 counter31 只是文件名而不是设备当前 rollback 值，应先在 SecBoot recovery 中发送 `p`，通过 UART4 的 `SecBoot-N32 diag rollback=...` 确认实际值。新包 counter 必须严格高于该值。

## 自动检查与退出码

`--expect-command` 可重复使用。配合 `--duration` 可用于硬件在环脚本：

```sh
python tool/at_server_sim/at_server_sim.py --port COM8 --duration 10 \
  --expect-command "^ATE0$" --expect-command "^AT\+CSQ$"
```

- `0`：运行完成，所有预期命令均已收到。
- `1`：缺少预期命令，或 `--list` 未发现串口。
- `2`：参数、正则表达式或依赖错误。

## 本机单元测试

单元测试不需要串口和 `pyserial`：

```sh
python -m unittest discover -s tool/at_server_sim -p "test_*.py" -v
```
