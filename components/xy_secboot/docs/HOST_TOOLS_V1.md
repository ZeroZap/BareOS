# XY Secboot Host Tools v1

## Overview

Secboot v1 needs host tools from the start. The MCU bootloader and the host tools form one upgrade system.

v1 tools:

| Tool | Purpose |
|---|---|
| `xy-secpack` | Build signed or MACed `.sbp` packages |
| `xy-secflash` | Transfer `.sbp` over UART transport v1 |
| `xy-secinspect` | Inspect package and manifest fields |

## Directory Plan

```text
tools/xy_secboot/
  README.md
  requirements.txt
  xy_secpack.py
  xy_secflash.py
  xy_secinspect.py
  secboot_manifest.py
  secboot_package.py
  secboot_uart.py
  tests/
    test_manifest.py
    test_package.py
    test_uart_frame.py
```

## Python Dependencies

v1 minimum:

```text
pyserial
```

Use Python standard library for:

```text
argparse
hashlib
hmac
struct
zlib
```

## Package Builder

`xy-secpack` responsibilities:

| Function | Required |
|---|---:|
| Read raw `app.bin` | Yes |
| Calculate SHA-256 image hash | Yes |
| Build little-endian manifest | Yes |
| Calculate HMAC-SHA256 manifest tag | Yes |
| Emit `.sbp` package | Yes |
| Inspect generated package | Yes |

## UART Flasher

`xy-secflash` responsibilities:

| Function | Required |
|---|---:|
| Open serial port | Yes |
| Send `HELLO` | Yes |
| Parse `CAPS` | Yes |
| Send `MANIFEST` | Yes |
| Send `DATA` stop-and-wait | Yes |
| Handle ACK/NACK | Yes |
| Timeout retransmit | Yes |
| Send `END` | Yes |
| Print progress | Yes |
| Fault injection flags | Yes |

## Fault Injection CLI Flags

Planned options:

```text
--corrupt-packet N
--drop-ack N
--duplicate-packet N
--skip-seq N
--wrong-offset N
--reset-at-offset N
--delay-ms N
--random-timeout
```

These options are used to prove the bootloader is robust before enabling field updates.

## Security Notes

HMAC keys used by v1 tools are development keys unless they are provisioned into protected device storage.

Do not commit real keys. Tool examples should use clearly marked test keys only.

For PLB GM production, the toolchain must later support SM3 and SM2 signing through a certified or vendor-approved implementation.
