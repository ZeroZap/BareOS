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
