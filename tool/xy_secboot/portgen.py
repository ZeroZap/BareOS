"""Generate a SecBoot V1 MCU porting skeleton from a JSON config."""

from __future__ import annotations

import json
from dataclasses import dataclass
from pathlib import Path
from typing import Any


DEFAULT_CONFIG: dict[str, Any] = {
    "project_name": "SecBoot-Example",
    "target_name": "EXAMPLE_MCU",
    "prefix": "secboot_example",
    "product_id": "0x00010001",
    "version": "V1.0.0-dev",
    "flash": {
        "base": "0x08000000",
        "total_size": "0x00020000",
        "page_size": "0x00000800",
    },
    "layout": {
        "boot_size": "0x00006000",
        "boot_config_size": "0x00000800",
        "boot_config_copies": 2,
        "state_size": "0x00000800",
        "rollback_size": "0x00000800",
        "manifest_size": "0x00000800",
        "reserved_tail_size": "0x00002000",
        "app_size": None,
        "mailbox_addr": "0x20003F00",
    },
    "transport": {
        "name": "UART",
        "log_uart": "UART4",
        "secboot_uart": "UART5",
        "baud": 115200,
        "max_payload": 512,
        "recommended_payload": 128,
        "recovery_timeout_ms": 1500,
    },
    "security": {
        "suite_id": 3,
        "dev_hmac_key": "tool/xy_secboot/dev_hmac_key.txt",
        "enable_wrp_api": True,
        "enable_rdp_api": True,
        "apply_wrp_rdp_at_boot": False,
    },
}


@dataclass(frozen=True)
class Layout:
    flash_base: int
    flash_total_size: int
    flash_page_size: int
    boot_base: int
    boot_size: int
    boot_config_a_base: int
    boot_config_b_base: int
    boot_config_size: int
    boot_config_copies: int
    state_base: int
    state_size: int
    rollback_base: int
    rollback_size: int
    manifest_base: int
    manifest_size: int
    app_base: int
    app_size: int
    mailbox_addr: int


def parse_int(value: Any, name: str) -> int:
    if isinstance(value, int):
        return value
    if isinstance(value, str):
        return int(value, 0)
    raise ValueError(f"{name} must be an integer or integer string")


def deep_merge(base: dict[str, Any], override: dict[str, Any]) -> dict[str, Any]:
    out = dict(base)
    for key, value in override.items():
        if isinstance(value, dict) and isinstance(out.get(key), dict):
            out[key] = deep_merge(out[key], value)
        else:
            out[key] = value
    return out


def c_ident(value: str, name: str) -> str:
    if not value:
        raise ValueError(f"{name} is empty")
    ok = value[0].isalpha() or value[0] == "_"
    ok = ok and all(ch.isalnum() or ch == "_" for ch in value)
    if not ok:
        raise ValueError(f"{name} must be a valid C identifier")
    return value


def macro_prefix(prefix: str) -> str:
    return prefix.upper()


def hex32(value: int) -> str:
    return f"0x{value:08X}u"


def hex_size(value: int) -> str:
    return f"0x{value:08X}u"


def build_layout(config: dict[str, Any]) -> Layout:
    flash = config["flash"]
    layout = config["layout"]
    flash_base = parse_int(flash["base"], "flash.base")
    flash_total = parse_int(flash["total_size"], "flash.total_size")
    flash_page = parse_int(flash["page_size"], "flash.page_size")
    boot_size = parse_int(layout["boot_size"], "layout.boot_size")
    boot_config_size = parse_int(layout.get("boot_config_size", 0), "layout.boot_config_size")
    boot_config_copies = parse_int(layout.get("boot_config_copies", 2), "layout.boot_config_copies")
    state_size = parse_int(layout["state_size"], "layout.state_size")
    rollback_size = parse_int(layout["rollback_size"], "layout.rollback_size")
    manifest_size = parse_int(layout["manifest_size"], "layout.manifest_size")
    reserved_tail = parse_int(layout.get("reserved_tail_size", 0), "layout.reserved_tail_size")
    mailbox_addr = parse_int(layout["mailbox_addr"], "layout.mailbox_addr")

    boot_config_a_base = flash_base + boot_size
    boot_config_b_base = boot_config_a_base + boot_config_size
    state_base = boot_config_a_base + (boot_config_size * boot_config_copies)
    rollback_base = state_base + state_size
    manifest_base = rollback_base + rollback_size
    app_base = manifest_base + manifest_size
    flash_end = flash_base + flash_total
    app_end_limit = flash_end - reserved_tail
    if layout.get("app_size") is None:
        app_size = app_end_limit - app_base
    else:
        app_size = parse_int(layout["app_size"], "layout.app_size")

    if flash_page <= 0 or (flash_page & (flash_page - 1)) != 0:
        raise ValueError("flash.page_size must be a power of two")
    for name, value in [
        ("boot_size", boot_size),
        ("boot_config_size", boot_config_size),
        ("state_size", state_size),
        ("rollback_size", rollback_size),
        ("manifest_size", manifest_size),
        ("reserved_tail_size", reserved_tail),
        ("app_size", app_size),
    ]:
        if value < 0 or value % flash_page != 0:
            raise ValueError(f"layout.{name} must be page aligned")
    if boot_config_copies != 2:
        raise ValueError("layout.boot_config_copies must be 2 for A/B redundant config")
    if app_size <= 0:
        raise ValueError("computed app_size is not positive")
    if app_base + app_size > app_end_limit:
        raise ValueError("App area exceeds Flash layout")

    return Layout(
        flash_base=flash_base,
        flash_total_size=flash_total,
        flash_page_size=flash_page,
        boot_base=flash_base,
        boot_size=boot_size,
        boot_config_a_base=boot_config_a_base,
        boot_config_b_base=boot_config_b_base,
        boot_config_size=boot_config_size,
        boot_config_copies=boot_config_copies,
        state_base=state_base,
        state_size=state_size,
        rollback_base=rollback_base,
        rollback_size=rollback_size,
        manifest_base=manifest_base,
        manifest_size=manifest_size,
        app_base=app_base,
        app_size=app_size,
        mailbox_addr=mailbox_addr,
    )


def render_layout_h(prefix: str, layout: Layout) -> str:
    mp = macro_prefix(prefix)
    guard = f"{mp}_LAYOUT_H"
    return f"""#ifndef {guard}
#define {guard}

#define {mp}_FLASH_BASE_ADDR       {hex32(layout.flash_base)}
#define {mp}_FLASH_TOTAL_SIZE      {hex_size(layout.flash_total_size)}
#define {mp}_FLASH_PAGE_SIZE       {hex_size(layout.flash_page_size)}

#define {mp}_BOOT_BASE_ADDR        {hex32(layout.boot_base)}
#define {mp}_BOOT_TOTAL_SIZE       {hex_size(layout.boot_size)}
#define {mp}_BOOT_CFG_A_BASE_ADDR  {hex32(layout.boot_config_a_base)}
#define {mp}_BOOT_CFG_B_BASE_ADDR  {hex32(layout.boot_config_b_base)}
#define {mp}_BOOT_CFG_SIZE         {hex_size(layout.boot_config_size)}
#define {mp}_BOOT_CFG_COPIES       {layout.boot_config_copies}u
#define {mp}_STATE_BASE_ADDR       {hex32(layout.state_base)}
#define {mp}_ROLLBACK_BASE_ADDR    {hex32(layout.rollback_base)}
#define {mp}_APP_MANIFEST_ADDR     {hex32(layout.manifest_base)}
#define {mp}_APP_IMAGE_ADDR        {hex32(layout.app_base)}
#define {mp}_APP_IMAGE_SIZE        {hex_size(layout.app_size)}

#define {mp}_CONFIRM_MAILBOX_ADDR  {hex32(layout.mailbox_addr)}

#endif /* {guard} */
"""


def render_port_h(prefix: str) -> str:
    mp = macro_prefix(prefix)
    guard = f"{mp}_PORT_H"
    return f"""#ifndef {guard}
#define {guard}

#include <stddef.h>
#include <stdint.h>

#include "xy_secboot_security.h"

#ifdef __cplusplus
extern "C" {{
#endif

int {prefix}_port_flash_read(uint32_t address, uint8_t *data, size_t len);
int {prefix}_port_flash_erase(uint32_t address, size_t len);
int {prefix}_port_flash_write(uint32_t address, const uint8_t *data, size_t len);

void {prefix}_port_uart_init(void);
void {prefix}_port_uart_poll(void);
int {prefix}_port_uart_read(uint8_t *data, size_t len, uint32_t timeout_ms);
int {prefix}_port_uart_write(const uint8_t *data, size_t len, uint32_t timeout_ms);
int {prefix}_port_uart_wait_tx_done(uint32_t timeout_ms);
uint32_t {prefix}_port_uart_pending(void);

void {prefix}_port_watchdog_kick(void);
void {prefix}_port_soft_reset(void);
int {prefix}_port_app_vector_check(uint32_t app_addr, uint32_t image_size);
void {prefix}_port_jump_app(uint32_t app_addr);

const xy_secboot_security_ops_t *{prefix}_port_security_ops(void);

#ifdef __cplusplus
}}
#endif

#endif /* {guard} */
"""


def render_port_c(prefix: str, target: str) -> str:
    return f"""#include "{prefix}_port.h"

#include "{prefix}_layout.h"

#include <string.h>

static int flash_range_ok(uint32_t address, size_t len)
{{
    uint32_t end = address + (uint32_t)len;

    if (end < address) {{
        return 0;
    }}
    if (address < {macro_prefix(prefix)}_FLASH_BASE_ADDR) {{
        return 0;
    }}
    if (end > ({macro_prefix(prefix)}_FLASH_BASE_ADDR + {macro_prefix(prefix)}_FLASH_TOTAL_SIZE)) {{
        return 0;
    }}
    return 1;
}}

int {prefix}_port_flash_read(uint32_t address, uint8_t *data, size_t len)
{{
    if ((data == NULL && len != 0u) || !flash_range_ok(address, len)) {{
        return -1;
    }}

    /* TODO({target}): replace with MCU internal Flash read if direct XIP read is not valid. */
    memcpy(data, (const void *)address, len);
    return 0;
}}

int {prefix}_port_flash_erase(uint32_t address, size_t len)
{{
    if ((address & ({macro_prefix(prefix)}_FLASH_PAGE_SIZE - 1u)) != 0u ||
        !flash_range_ok(address, len)) {{
        return -1;
    }}

    /* TODO({target}): unlock Flash, erase pages, kick watchdog, lock Flash. */
    (void)address;
    (void)len;
    return -1;
}}

int {prefix}_port_flash_write(uint32_t address, const uint8_t *data, size_t len)
{{
    if ((data == NULL && len != 0u) || !flash_range_ok(address, len) ||
        ((address | len) & 0x3u) != 0u) {{
        return -1;
    }}

    /* TODO({target}): unlock Flash, program words, readback verify, lock Flash. */
    (void)address;
    (void)data;
    (void)len;
    return -1;
}}

void {prefix}_port_uart_init(void)
{{
    /* TODO({target}): init SecBoot UART RX/TX and ISR/DMA ring buffer. */
}}

void {prefix}_port_uart_poll(void)
{{
    /* TODO({target}): move DMA/ISR data into the SecBoot RX ring if needed. */
}}

int {prefix}_port_uart_read(uint8_t *data, size_t len, uint32_t timeout_ms)
{{
    /* TODO({target}): pop up to len bytes from the SecBoot UART RX ring. */
    (void)data;
    (void)len;
    (void)timeout_ms;
    return 0;
}}

int {prefix}_port_uart_write(const uint8_t *data, size_t len, uint32_t timeout_ms)
{{
    /* TODO({target}): write len bytes to the SecBoot UART. */
    (void)data;
    (void)timeout_ms;
    return (int)len;
}}

int {prefix}_port_uart_wait_tx_done(uint32_t timeout_ms)
{{
    /* TODO({target}): wait until UART TX shift register is empty. */
    (void)timeout_ms;
    return 0;
}}

uint32_t {prefix}_port_uart_pending(void)
{{
    /* TODO({target}): return RX ring pending byte count. */
    return 0u;
}}

void {prefix}_port_watchdog_kick(void)
{{
    /* TODO({target}): reload watchdog if enabled. */
}}

void {prefix}_port_soft_reset(void)
{{
    /* TODO({target}): trigger MCU software reset. */
    while (1) {{
    }}
}}

int {prefix}_port_app_vector_check(uint32_t app_addr, uint32_t image_size)
{{
    uint32_t sp = *(const uint32_t *)app_addr;
    uint32_t reset = *(const uint32_t *)(app_addr + 4u);

    /* TODO({target}): adjust SRAM range for this MCU. */
    if (sp < 0x20000000u || (sp & 0x7u) != 0u) {{
        return -1;
    }}
    if (reset < app_addr || reset >= (app_addr + image_size) || (reset & 0x1u) == 0u) {{
        return -1;
    }}
    return 0;
}}

void {prefix}_port_jump_app(uint32_t app_addr)
{{
    /* TODO({target}): disable IRQs/peripherals, set VTOR/MSP, jump reset handler. */
    (void)app_addr;
    while (1) {{
    }}
}}

static int {prefix}_security_get_status(xy_secboot_security_status_t *status)
{{
    if (!status) {{
        return -1;
    }}

    /* TODO({target}): read real WRP/RDP option bytes here. */
    status->rdp_level = XY_SECBOOT_RDP_LEVEL_0;
    status->wrp_mask = 0u;
    return 0;
}}

static int {prefix}_security_apply(const xy_secboot_security_config_t *config)
{{
    /* TODO({target}): implement only in factory flow; real WRP/RDP can lock boards. */
    (void)config;
    return -1;
}}

static const xy_secboot_security_ops_t s_{prefix}_security_ops = {{
    {prefix}_security_get_status,
    {prefix}_security_apply,
}};

const xy_secboot_security_ops_t *{prefix}_port_security_ops(void)
{{
    return &s_{prefix}_security_ops;
}}
"""


def render_main_c(prefix: str, config: dict[str, Any]) -> str:
    version = config["version"]
    project_name = config["project_name"]
    recovery = int(config["transport"].get("recovery_timeout_ms", 1500))
    return f"""#include "{prefix}_layout.h"
#include "{prefix}_port.h"
#include "xy_log.h"
#include "xy_secboot.h"

#include <stdint.h>

#define {macro_prefix(prefix)}_VERSION_STR "{project_name} {version}"

static void platform_init(void)
{{
    /* TODO: init clocks, GPIO, log UART, SysTick, and SecBoot UART. */
}}

int main(void)
{{
    platform_init();
    xy_log_init();
    xy_log_i("%s", {macro_prefix(prefix)}_VERSION_STR);
    {prefix}_port_uart_init();

    /* TODO: build the partition table and crypto ops like SecBoot-N32 V1. */
    /* TODO: call the V1 boot check before entering recovery. */
    /* (void)secboot_try_boot_app({recovery}u); */

    while (1) {{
        {prefix}_port_watchdog_kick();
        {prefix}_port_uart_poll();
        /* TODO: call the SecBoot V1 protocol poll function. */
    }}
}}
"""


def render_guide(config: dict[str, Any], layout: Layout) -> str:
    prefix = config["prefix"]
    mp = macro_prefix(prefix)
    transport = config["transport"]
    security = config["security"]
    product_id = parse_int(config["product_id"], "product_id")
    return f"""# {config['project_name']} SecBoot V1 移植指导书

本文由 `xy_secboot.py portgen` 根据配置自动生成，用于把当前 SecBoot V1 框架移植到 `{config['target_name']}`。

## 1. 生成内容

| 文件 | 作用 |
|---|---|
| `inc/{prefix}_layout.h` | Flash 分区、boot config A/B、App slot、mailbox 地址 |
| `inc/{prefix}_port.h` | MCU port 接口声明 |
| `src/{prefix}_port.c` | Flash/UART/reset/jump/security port 桩实现 |
| `src/{prefix}_main.c` | bootloader main loop 框架 |
| `secboot_port_config.json` | 本次生成使用的配置快照 |

## 2. 分区规划

| 区域 | 地址 | 大小 | 说明 |
|---|---:|---:|---|
| Bootloader | `{hex32(layout.boot_base)}` | `{hex_size(layout.boot_size)}` | 建议最终 WRP 保护 |
| Boot config A | `{hex32(layout.boot_config_a_base)}` | `{hex_size(layout.boot_config_size)}` | boot 配置冗余副本 A |
| Boot config B | `{hex32(layout.boot_config_b_base)}` | `{hex_size(layout.boot_config_size)}` | boot 配置冗余副本 B |
| Boot state | `{hex32(layout.state_base)}` | `{hex_size(layout.state_size)}` | pending/confirmed/attempts |
| Rollback | `{hex32(layout.rollback_base)}` | `{hex_size(layout.rollback_size)}` | anti-rollback counter |
| App manifest | `{hex32(layout.manifest_base)}` | `{hex_size(layout.manifest_size)}` | 只在 END 校验通过后提交 |
| App image | `{hex32(layout.app_base)}` | `{hex_size(layout.app_size)}` | 应用程序镜像 |
| Confirm mailbox | `{hex32(layout.mailbox_addr)}` | - | App 请求 confirmed 的 SRAM mailbox |

Flash 参数：

```text
FLASH_BASE       = {hex32(layout.flash_base)}
FLASH_TOTAL_SIZE = {hex_size(layout.flash_total_size)}
FLASH_PAGE_SIZE  = {hex_size(layout.flash_page_size)}
PRODUCT_ID       = 0x{product_id:08X}
```

## 3. Boot Config A/B 控制策略

Boot config A/B 是 bootloader 的配置冗余区，不是 App A/B 分区。它用于保存 product id、分区参数、UART 参数、安全策略、版本策略等 boot 配置。

建议记录格式：

```text
magic
format_version
seq
seq_inv
payload_len
payload_crc32
header_crc32
payload TLV/struct
```

启动读取策略：

```text
分别读取 A/B
校验 magic、inverse、CRC、版本
两份都有效时选择 seq 最新的一份
只有一份有效时使用有效副本
两份都无效时使用编译期默认配置或停留 recovery
```

更新写入策略：

```text
先选择非 active 副本
擦除非 active 副本所在页
写入完整新配置和 CRC
读回校验成功后，新副本成为 active
不要先擦 active 副本，避免掉电后两份都坏
```

这样比单区配置更安全：单区写配置时掉电可能导致配置丢失；A/B 冗余至少能保留上一份有效配置。

## 4. 必须补齐的 MCU port

在 `src/{prefix}_port.c` 中补齐以下函数：

```c
{prefix}_port_flash_read()
{prefix}_port_flash_erase()
{prefix}_port_flash_write()
{prefix}_port_uart_init()
{prefix}_port_uart_poll()
{prefix}_port_uart_read()
{prefix}_port_uart_write()
{prefix}_port_uart_wait_tx_done()
{prefix}_port_watchdog_kick()
{prefix}_port_soft_reset()
{prefix}_port_app_vector_check()
{prefix}_port_jump_app()
```

移植要求：

```text
Flash erase 地址必须按 {mp}_FLASH_PAGE_SIZE 对齐
Flash write 长度必须 4 字节对齐
UART RX 建议 ISR/DMA 写 ring buffer，主循环 poll 协议
jump App 前必须关闭无关中断、设置 VTOR/MSP、再跳 reset handler
```

## 4. 安全设定 WRP/RDP

本框架生成了 `xy_secboot_security_ops_t` 接口：

```c
const xy_secboot_security_ops_t *{prefix}_port_security_ops(void);
```

当前生成的 `apply()` 默认返回失败，防止误锁板。只有进入量产锁板流程时，才允许接入真实 option byte：

```text
WRP: 真实 Flash 写保护
RDP1/RDP2: 真实读保护
RDP2: 高风险/可能不可逆，开发阶段跳过
```

配置中当前安全策略：

```text
enable_wrp_api       = {security.get('enable_wrp_api')}
enable_rdp_api       = {security.get('enable_rdp_api')}
apply_wrp_rdp_at_boot = {security.get('apply_wrp_rdp_at_boot')}
```

## 5. 打包 App

App 链接地址必须等于：

```text
{hex32(layout.app_base)}
```

打包命令示例：

```powershell
python tool/xy_secboot/xy_secboot.py pack --input build/app.bin --output build/app.sbp --product-id 0x{product_id:08X} --image-addr {hex32(layout.app_base).replace('u', '')} --entry-addr {hex32(layout.app_base).replace('u', '')} --image-version 1 --security-counter 1 --hmac-key {security.get('dev_hmac_key')}
```

## 6. UART 烧录 App

推荐先用较稳的 payload：

```powershell
python tool/xy_secboot/xy_secboot.py flash --port COMx --baud {transport.get('baud', 115200)} --package build/app.sbp --payload {transport.get('recommended_payload', 128)} --timeout-ms 2000 --retries 20 --recover-ms 5000 --reset
```

## 7. 验证标准

必须看到以下闭环：

```text
bootloader version log
HELLO/CAPS 正常
MANIFEST ACK
DATA ACK
END ACK
reset 后 pending App boot attempt=1
jump App
App 写 mailbox 并软复位
bootloader App confirmed
再次 jump App
App main loop start
```

## 8. 常见问题

| 现象 | 可能原因 | 处理 |
|---|---|---|
| `timeout waiting for secboot frame` | App 已启动，未进入 recovery | 使用 `--recover-ms 5000` 并在窗口内复位 |
| `BAD_HEADER_CRC` / `BAD_PAYLOAD_CRC` | UART 丢字节或干扰 | 降低 payload 到 128，检查 GND/线材/串口占用 |
| `ROLLBACK_REJECTED` | security_counter 低于已接受版本 | 提高 `--security-counter` |
| 烧录后不跳 App | manifest 未提交或 App vector 不合法 | 检查 App 链接地址和 entry addr |

## 9. 生产前必须确认

```text
替换 dev HMAC key，确定生产签名/验签方案
确认 bootloader/state/rollback 的硬件隔离策略
确认 WRP/RDP factory-lock 流程，不在普通启动路径执行
完成掉电/中断/旧版本/坏签名回归测试
```
"""


def write_file(path: Path, text: str, force: bool) -> None:
    if path.exists() and not force:
        raise FileExistsError(f"{path} already exists; use --force to overwrite")
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8", newline="\n")


def load_config(path: Path | None) -> dict[str, Any]:
    if path is None:
        return dict(DEFAULT_CONFIG)
    user = json.loads(path.read_text(encoding="utf-8-sig"))
    return deep_merge(DEFAULT_CONFIG, user)


def generate_port(config_path: Path | None, out_dir: Path, force: bool) -> list[Path]:
    config = load_config(config_path)
    prefix = c_ident(config["prefix"], "prefix")
    target = str(config["target_name"])
    layout = build_layout(config)
    files = {
        out_dir / "inc" / f"{prefix}_layout.h": render_layout_h(prefix, layout),
        out_dir / "inc" / f"{prefix}_port.h": render_port_h(prefix),
        out_dir / "src" / f"{prefix}_port.c": render_port_c(prefix, target),
        out_dir / "src" / f"{prefix}_main.c": render_main_c(prefix, config),
        out_dir / "SECBOOT_V1_PORTING_GUIDE_CN.md": render_guide(config, layout),
        out_dir / "secboot_port_config.json": json.dumps(config, indent=2, ensure_ascii=False) + "\n",
    }
    for path, text in files.items():
        write_file(path, text, force)
    return list(files.keys())


def write_sample_config(path: Path, force: bool) -> None:
    write_file(path, json.dumps(DEFAULT_CONFIG, indent=2, ensure_ascii=False) + "\n", force)
