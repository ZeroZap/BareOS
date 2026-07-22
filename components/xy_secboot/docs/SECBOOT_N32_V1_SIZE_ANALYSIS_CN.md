# SecBoot-N32 V1 大小与后续优化记录

本文记录当前 SecBoot-N32 V1 在不启用 Boot Config A/B 分区、不启用 App SLOT A/B 的情况下的固件大小、map 占用分析和后续可优化项。

## 1. 当前构建条件

构建目录：

```text
project/Sec-boot-N32/Makefile
```

构建命令：

```powershell
make
```

当前版本：

```text
SecBoot-N32 V1.1.2-dev
```

当前 Flash layout：

```text
Bootloader   0x08000000-0x08005FFF  24 KB
Boot State   0x08006000-0x080067FF   2 KB
Rollback     0x08006800-0x08006FFF   2 KB
Manifest     0x08007000-0x080077FF   2 KB
App Image    0x08007800-0x0801DFFF  90 KB
EEPROM       0x0801E000-0x0801EFFF   4 KB
FEE          0x0801F000-0x0801FFFF   4 KB
```

当前未启用：

```text
Boot Config A/B 分区
App SLOT A/B
WRP/RDP apply
Bootloader 自升级
```

说明：`xy_secboot_bootcfg` 通用组件源码已存在，但 SecBoot-N32 运行路径未引用，链接器 `--gc-sections` 会丢弃未使用函数，当前不增加最终固件大小。

## 2. 当前 size 结果

`arm-none-eabi-size build/SecBootN32.elf`：

```text
text = 13812 bytes
data = 1088 bytes
bss  = 3988 bytes
dec  = 18888 bytes
```

按 Flash/RAM 估算：

```text
Flash 占用约 = text + data = 14900 bytes ≈ 14.6 KB
RAM 占用约   = data + bss  = 5076 bytes ≈ 5.0 KB
```

Bootloader 分区：

```text
0x6000 = 24576 bytes = 24 KB
```

当前 bootloader Flash 余量：

```text
24576 - 14900 = 9676 bytes ≈ 9.4 KB
```

结论：当前 V1 大小健康，距离 24 KB bootloader 分区仍有约 9.4 KB 余量。

## 3. Section 占用

`arm-none-eabi-size -A build/SecBootN32.elf` 主要结果：

```text
.isr_vector            328
.text                11420
.rodata               2064
.init_array              8
.fini_array              4
.data                 1076
.bss                  2964
._user_heap_stack     1024
```

主要 Flash 来源：

```text
.text
.rodata
.isr_vector
.data load image
```

主要 RAM 来源：

```text
.data
.bss
._user_heap_stack
```

## 4. RAM 大项

| 符号 | 大小 | 来源 | 说明 |
|---|---:|---|---|
| `impure_data` | 1064 B | newlib | libc reentrancy 数据 |
| `g_print_buf` | 1024 B | `xy_stdio` | printf 格式化缓冲 |
| `s_uart5_rx_pool` | 1024 B | N32 UART5 | SecBoot UART RX ring buffer |
| `s_payload` | 512 B | `secboot_n32_v1.c` | UART DATA payload 缓冲 |
| `s_manifest` | 268 B | `secboot_n32_v1.c` | manifest RAM 缓冲 |

RAM 优化优先级：

```text
newlib impure_data
printf buffer
UART RX ring
payload buffer
manifest buffer
```

## 5. Flash 代码大项

| 符号 | 大小 | 说明 |
|---|---:|---|
| `secboot_n32_v1_poll` | 940 B | UART V1 协议主处理 |
| `xy_secboot_single_verify_active` | 568 B | manifest/image 校验主流程 |
| `secboot_n32_v1_try_boot_app` | 428 B | reset-time 验证和跳 App |
| `SystemInit` | 344 B | 系统初始化 |
| `GPIO_InitPeripheral` | 348 B | HAL GPIO 初始化 |
| `sha256_transform` | 320 B | SHA256 压缩函数 |
| `main` | 304 B | bootloader main |
| `secboot_hmac_sha256` | 294 B | dev HMAC-SHA256 |
| `xy_vsprintf` | 254 B | 日志格式化 |
| `RCC_GetClocksFreqValue` | 232 B | HAL 时钟计算 |
| `hash_image` | 184 B | App image hash |
| `xy_sha256_final` | 184 B | SHA256 final |
| `send_frame` | 172 B | UART frame 发送 |
| `secboot_state_read` | 160 B | state page 扫描 |
| `secboot_n32_v1_print_layout` | 164 B | layout 诊断打印 |

Flash 大项主要集中在：

```text
UART V1 协议
manifest/image verify
SHA256/HMAC
HAL 初始化/Flash/UART/RCC
log/printf
诊断打印
```

## 6. rodata 大项

| 符号 | 大小 | 说明 |
|---|---:|---|
| `sha256_k` | 256 B | SHA256 常量表 |
| `banner.0` | 191 B | UART recovery banner |
| `s_secboot_n32_parts` | 100 B | 分区表 |
| `s_secboot_crypto_ops` | 56 B | crypto ops |
| `s_secboot_dev_hmac_key` | 31 B | dev HMAC key |

另外 `.rodata` 中包含大量日志字符串。若进入量产压缩，可按日志等级裁剪。

## 7. newlib 来源分析

当前 newlib 主要来源有两类。

### 7.1 默认启动链路

map 中显示：

```text
crt0.o -> __libc_init_array
crt0.o -> exit
exit.o -> _global_impure_ptr
exit.o -> __call_exitprocs
atexit.o -> __register_exitproc
libnosys.a(_exit.o)
```

这会引入：

```text
impure_data 1064 B RAM
exit/atexit 相关 Flash 代码
```

该来源不是业务代码直接调用，而是默认 toolchain startup/newlib runtime 链路引入。

### 7.2 libc memcpy/memset

明确拉入 libc `memcpy` 的源文件：

```text
project/Sec-boot-N32/USER/src/secboot_n32_port.c
```

原因：

```c
#include <string.h>
memcpy(...)
```

另有：

```text
components/clib/xy_stdio.c 里有 memcpy 调用痕迹
components/clib/xy_string.c 的循环可能被编译器识别为 builtin memset/memcpy
```

## 8. 当前 V1 可优化项

### 8.1 低风险优化

1. 把 `secboot_n32_port.c` 的 `<string.h>` 改成 `xy_string.h`，显式使用 `xy_memcpy`。
2. 检查 `xy_stdio.c` 的 `memcpy` 调用，改成 `xy_memcpy` 或确保宏替换生效。
3. 增加编译选项：

```make
-fno-builtin-memcpy -fno-builtin-memset -fno-builtin-memcmp
```

4. 裁剪非必要诊断日志字符串，例如 layout 频繁打印、长 banner。
5. 检查 `g_print_buf` 是否必须 1024 B，可按日志最大行长度缩小。
6. 默认 UART flash 推荐 `payload=128`，但协议 max 可保留 512。

### 8.2 中风险优化

1. 裁剪 `xy_vsprintf` 支持的格式，只保留 `%s/%u/%d/%x/%c` 等必要格式。
2. 精简 HAL RCC/GPIO/UART 初始化，减少通用 HAL 分支。
3. 将 `secboot_n32_v1_print_layout()` 置于调试宏下，量产关闭。
4. 根据实际链路质量调整 `s_uart5_rx_pool`，在稳定后评估是否小于 1024。

### 8.3 高风险优化

1. 去掉默认 `crt0.o`/newlib init/exit 链路，改用 `-nostartfiles` 和 MCU startup 直进 `main`。
2. 移除 newlib `impure_data` 依赖。
3. 使用硬件 SHA/HMAC 或裁剪 crypto 实现。
4. 用极简寄存器操作替代 HAL。

高风险优化需要单独分支验证，避免破坏启动和中断初始化。

## 9. 暂不优化项

当前暂不做：

```text
Boot Config A/B 分区接入
App SLOT A/B
Bootloader 自升级
真实 WRP/RDP apply
去 newlib 启动链路
HAL 大规模替换
```

理由：当前 V1 仍有足够 Flash 余量，优先保证主链路稳定和可验证。

## 10. 当前结论

当前不带 A/B config 分区的 SecBoot-N32 V1：

```text
Flash 占用约 14.6 KB / 24 KB
RAM 占用约 5.0 KB
剩余 Flash 约 9.4 KB
```

当前最大的优化空间：

```text
newlib runtime / impure_data
printf/log buffer
UART RX 和 payload buffer
日志字符串
HAL 初始化代码
SHA256/HMAC
```

建议后续优化顺序：

1. 先做 `memcpy/memset` 去 newlib 小优化。
2. 再裁剪日志和 printf buffer。
3. 再优化 UART buffer 和 payload 策略。
4. 最后再考虑去 `crt0.o`/newlib runtime。

当前 V1 可以继续保持现状，后续再按本记录逐项优化。
