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
