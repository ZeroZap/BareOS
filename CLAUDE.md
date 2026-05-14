# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

BareOS is a bare-metal C framework for a **vessel distress / PLB (Personal Locator Beacon) module**. It runs on ARM Cortex-M MCUs (STM32/WCH/HC32) with no RTOS — pure foreground/background architecture: ISRs feed ring buffers; the main loop drives everything else.

`components-ref/` is a read-only reference library. All production code lives in `components/`.

## Architecture

```
Application (main.c + BSP)
    ↓
Protothreads (pt/) + State machine (sm/)   ← cooperative task model
    ↓
AT client (at/)  |  AT server (ats/)       ← 4G/GNSS/Satellite / debug UART
    ↓
GNSS (gnss/)  |  Crypto (crypto/)  |  Sensor (sensor/)
    ↓
Storage: FEE (storage/fee/) → Param (storage/param/) + Evtlog (storage/evtlog/)
    ↓
clib (clib/)  |  Log (log/)  |  Modbus (modbus/)  |  Timer (timer/)
    ↓
BSP (provided by application, not in this repo)
```

**Main loop pattern:**
```c
while (1) {
    at_obj_process(g_at_4g);        // AT-Command-V2 work queue
    at_obj_process(g_at_gnss);
    at_obj_process(g_at_sat);
    PT_SCHEDULE(task_gnss(&s_pt_gnss));
    PT_SCHEDULE(task_comm(&s_pt_comm));
    PT_SCHEDULE(task_distress(&s_pt_distress));
    if (all_tasks_idle()) bsp_enter_stop_until_uart_or_timer();
}
```

**UART mapping:**
| UART | Module | Role |
|------|--------|------|
| UART1 | 4G modem | AT client (data/SMS/MQTT) |
| UART2 | GNSS | NMEA feed or AT client |
| UART3 | Satellite | AT client |
| UART4 (optional) | Debug/host | AT server (ats/) |

## Components (`components/`)

| Path | Purpose |
|------|---------|
| `at/` | AT-Command-V2: work queue, URC, no RTOS (`lock/unlock = NULL`) |
| `ats/` | AT server: bare-metal, `at_server_feed_byte()` called per RX byte |
| `pt/` | Contiki-NG-compatible: `pt.h` (protothreads), `etimer` (poll timer), `ctimer` (callback timer), `stimer` (seconds timer), `rtimer` (hardware real-time timer), `process` (process model) |
| `sm/` | State machine: entry/process/exit callbacks + timeout |
| `gnss/` | NMEA parser + AT response parser, coordinates as `int32_t × 1e7` |
| `timer/` | Software timer list, tick-based |
| `crypto/` | AES-128, SHA-256, HMAC-SHA256, CRC-16/32, CSPRNG — pure C |
| `clib/` | Custom C library: stdio (printf/scanf/sprintf), string, math, rb, assert |
| `log/` | Leveled logging over `xy_printf`; BSP provides `void xy_log_char(char ch)` |
| `modbus/` | Tiny Modbus RTU (mb_tiny) |
| `sensor/` | Sensor abstraction: bus/power/trigger/core |
| `storage/fee/` | Flash EEPROM Emulation Nano (no heap, no RTOS) |
| `storage/param/` | Device parameters (MMSI, serial, TX interval, flags, server, last pos) |
| `storage/evtlog/` | Fixed 16-byte ring buffer event log in Flash, CRC-16, power-off recovery |
| `storage/vlog/` | Variable-length typed log in RAM ring buffer (lost on reset); `xy_vlog_write()` / `xy_vlog_foreach()` |
| `norflash/` | NOR Flash abstraction (read/write/erase/power-down); BSP provides HAL via `xy_nor_port.c` |
| `mem/` | Multi-pool bare-metal memory allocator (`xy_malloc`/`xy_free`/`xy_malloc_variable`) |
| `tlv/` | TLV encoder/decoder — zero allocation, iterator pattern, nested containers |
| `json/` | coreJSON v3.3.0 — zero allocation, MISRA C:2012, no dynamic memory |
| `event/` | Bare-metal event bus — bounded ring queue, ISR-safe post, subscriber callbacks, Contiki-inspired |
| `sys/` | Reset cause, software reset, watchdog kick, chip ID, version strings; BSP provides weak hooks |

## Timing

All components share a single time source:
```c
extern volatile unsigned int g_sys_tick_ms;  /* incremented in SysTick_Handler */
```
`at_port.c` calls it via `at_get_ms()`. `etimer.c` calls it via `etimer_now_ms()`. The BSP must define and maintain `g_sys_tick_ms`.

## BSP Integration Points

The following must be provided by the application BSP (not in this repo):

| Symbol | Where used | Description |
|--------|-----------|-------------|
| `volatile unsigned int g_sys_tick_ms` | `at/src/at_port.c`, `pt/etimer.c` | SysTick ms counter |
| `void xy_log_char(char ch)` | `log/xy_log.c` | Single-byte UART output for logging |
| Flash erase/write/read callbacks | `storage/evtlog`, `storage/fee` | Passed at init time |

## Storage Layer

- **FEE Nano** (`storage/fee/`): key-value NVM over raw Flash. Default config: `XY_OS_BACKEND_BAREMETAL = 1`.
- **Param** (`storage/param/`): RAM shadow of 8 device parameters with dirty bitmask. Call `xy_param_save()` to flush to FEE.
- **Evtlog** (`storage/evtlog/`): Fixed 16-byte records, CRC-16/CCITT, ring buffer. `write_idx` recovered on boot by scanning for first empty (0xFF) slot.

## AT Client (at/)

Source: `AT-Command-V2` from `components-ref/net/at/`. Bare-metal init:
```c
static at_adapter_t adapt = {
    .lock   = NULL,   /* no RTOS mutex needed */
    .unlock = NULL,
    .write  = uart_write_fn,
    .read   = uart_read_fn,
};
at_obj_t *g_at_4g = at_obj_create(&adapt);
```
Call `at_obj_process(g_at_4g)` each main loop iteration.

## AT Server (ats/)

Bare-metal usage: call `at_server_feed_byte(&srv, ch)` per received byte (from ISR or DMA callback). Commands are dispatched when CR/LF is received. No threads, no semaphores.

## Clib vs Standard Library

`components/clib/` provides `xy_printf`, `xy_sprintf`, `xy_snprintf`, `xy_scanf`, `xy_sscanf`, and string/math utilities prefixed `xy_`. These are the only printf/scanf functions available — do not assume a hosted `stdio.h` libc is present.

## Reference Library (`components-ref/`)

Read-only reference. Do not add production code here. Notable sub-paths:
- `net/at/AT-Command-V2/` — source of `components/at/`
- `sys/xy_state_machine/` — source of `components/sm/`
- `dm/fee/` — source of `components/storage/fee/`
- `trace/xy_log/` — source of `components/log/`
