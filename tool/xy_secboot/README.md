# XY SecBoot Host Tool

Python host-side tool for SecBoot-N32 UART5 development.

It provides one shared protocol core with two frontends:

| Mode | Entry |
|---|---|
| CLI | `python tool/xy_secboot/xy_secboot.py ...` |
| GUI | `python tool/xy_secboot/xy_secboot.py gui` |

## Install

```bash
python -m pip install -r tool/xy_secboot/requirements.txt
```

`pyserial` is required for UART access. The GUI uses Python's standard `tkinter` module.

## CLI

List serial ports:

```bash
python tool/xy_secboot/xy_secboot.py ports
```

Build an `.sbp` package:

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

Inspect a package:

```bash
python tool/xy_secboot/xy_secboot.py inspect build/app.sbp
```

Build a mutated package for V1 fault-injection tests:

```bash
python tool/xy_secboot/xy_secboot.py fault-package \
  --package build/app.sbp \
  --output build/app_bad_signature.sbp \
  --fault bad-signature
```

Supported package faults:

| Fault | Expected target path |
|---|---|
| `bad-image` | `END` rejects with `IMAGE_VERIFY_FAILED` |
| `bad-hash` | `END` rejects with `IMAGE_VERIFY_FAILED` |
| `bad-signature` | `END` rejects with `IMAGE_VERIFY_FAILED` |
| `bad-manifest-crc` | `MANIFEST` rejects with `BAD_MANIFEST` |
| `bad-product` | `MANIFEST` rejects with `BAD_MANIFEST` |
| `bad-entry` | `MANIFEST` rejects with `BAD_MANIFEST` |
| `old-counter` | `END` rejects with `ROLLBACK_REJECTED` when below stored counter |
| `bad-package-crc` | Host `inspect` rejects the package before flashing |

Field-mutation faults that should preserve the HMAC, such as `bad-product`,
`bad-entry`, and `old-counter`, need `--hmac-key` when you want the bootloader
to reach the intended policy check instead of failing the manifest MAC first.

Flash over UART5 transport v1:

```bash
python tool/xy_secboot/xy_secboot.py flash \
  --port COM12 \
  --baud 115200 \
  --package build/app.sbp \
  --payload 256 \
  --timeout-ms 1000 \
  --retries 10
```

During bring-up, add `--reset` to reset the MCU after `END ACK`:

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

`END ACK` means the image was written, verified, and the App manifest was committed. SecBoot-N32 V1 does not immediately jump to the App after `END ACK`; on the next reset it opens a 1500 ms UART5 recovery window, then verifies the committed manifest and jumps to the App entry if no host input is present. Without `--reset`, continuing bootloader heartbeat logs after `END ACK` are expected until the next reset.

Interrupt a flash flow before `END` for recovery testing:

```bash
python tool/xy_secboot/xy_secboot.py flash \
  --port COM24 \
  --baud 115200 \
  --package build/app.sbp \
  --payload 256 \
  --timeout-ms 1000 \
  --retries 10 \
  --interrupt-after-manifest
```

```bash
python tool/xy_secboot/xy_secboot.py flash \
  --port COM24 \
  --baud 115200 \
  --package build/app.sbp \
  --payload 256 \
  --timeout-ms 1000 \
  --retries 10 \
  --interrupt-at-offset 0x4000
```

These options stop the host without sending `END`. The bootloader must not commit the
new manifest or jump the partially written image. A later normal flash should recover.

Probe UART DATA fault handling without sending `END`:

```bash
python tool/xy_secboot/xy_secboot.py probe-transport \
  --port COM24 \
  --baud 115200 \
  --package build/app.sbp \
  --payload 256 \
  --timeout-ms 1000 \
  --retries 10 \
  --recover-ms 5000 \
  --fault bad-seq
```

Supported transport probes:

| Fault | Expected result |
|---|---|
| `duplicate-data` | duplicate DATA returns ACK for the duplicate frame |
| `bad-seq` | DATA with skipped seq returns `BAD_SEQ` |
| `bad-offset` | DATA with wrong offset returns `BAD_OFFSET` |

Transport probes send MANIFEST and at least one DATA frame, so the App area may be
erased or partially written. Always flash a normal package after a probe.

If a valid App is already installed and the board jumps too quickly for manual flashing, use `--recover-ms` and reset the board while the host is sending recovery bytes:

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

`--recover-ms` is a host-side recovery preamble. The tool opens the serial port, sends `?` repeatedly for the configured duration, then clears received banner text before sending `HELLO`. Press the board reset key during this preamble so SecBoot receives UART5 input inside its 1500 ms recovery window and stays in bootloader mode.

Use it when a normal `flash` command times out because the board has already jumped to the App. Do not use it when the board is already printing SecBoot heartbeat; in that case a normal `flash` command can talk to the bootloader directly.

Launch GUI:

```bash
python tool/xy_secboot/xy_secboot.py gui
```

## SecBoot V1 Port Generator

Generate a sample JSON configuration:

```bash
python tool/xy_secboot/xy_secboot.py portgen \
  --sample-config build/secboot_port_config.json
```

Generate an MCU porting skeleton and Chinese porting guide:

```bash
python tool/xy_secboot/xy_secboot.py portgen \
  --config build/secboot_port_config.json \
  --output build/secboot_port \
  --force
```

The generator emits:

| File | Purpose |
|---|---|
| `inc/*_layout.h` | Flash partition layout, boot config A/B, and mailbox address |
| `inc/*_port.h` | MCU port function declarations |
| `src/*_port.c` | Flash/UART/reset/jump/security stubs |
| `src/*_main.c` | Bootloader main-loop skeleton |
| `SECBOOT_V1_PORTING_GUIDE_CN.md` | Chinese porting and verification guide |

WRP/RDP APIs generated by this tool are real hardware security hooks once the MCU
port binds them to option bytes. The generated `apply()` stub returns failure by
default so development boards are not locked accidentally.

The generated layout includes two boot config pages, `BOOT_CFG_A` and
`BOOT_CFG_B`. They are redundant bootloader configuration storage areas, not App
A/B slots. A production port should read both copies, choose the newest valid
record by sequence and CRC, and always update the inactive copy first.

Run host-side boot config A/B recovery simulation tests:

```bash
python tool/xy_secboot/bootcfg_sim.py
```

## PLB App Flow

Build SecBoot, build PLB-N32 as the App slot image, package it, and inspect the package:

```bash
python tool/xy_secboot/plb_app_flow.py --clean
```

Also flash the SecBoot bootloader:

```bash
python tool/xy_secboot/plb_app_flow.py --clean --flash-boot
```

Flash the packaged PLB app through UART5:

```bash
python tool/xy_secboot/plb_app_flow.py --flash-app-uart --port COM12
```

Capture UART4 logs after the flow:

```bash
python tool/xy_secboot/plb_app_flow.py --flash-app-uart --port COM12 --capture-log COM8
```

## Package Notes

The packer emits the `.sbp` package described in `components/xy_secboot/docs/SECBOOT_V1_DESIGN.md`.

The MCU Flash write path requires DATA lengths to be 4-byte aligned. The packer pads the input image with `0xFF` to a 4-byte boundary before calculating the image hash and manifest fields.

The UART flasher validates that each ACK/NACK carries the sequence number of the frame currently in flight. Stale ACKs from delayed serial delivery are ignored and retried instead of advancing the DATA offset.

The flasher also decodes SecBoot internal verify detail values returned in ERROR
ACK payloads. For example, `detail=0xfffffffd` is printed as
`MANIFEST_MAC_FAILED(0xfffffffd)`, and `detail=0xfffffffe` is printed as
`IMAGE_HASH_OR_RANGE_FAILED(0xfffffffe)`.

The PLB-N32 demo App now requests `CONFIRMED` through a reset-retained SRAM mailbox at `0x20003F00`. The App writes the request and resets; SecBoot verifies the installed manifest and then writes the boot-state record itself. This is still a bring-up path, but the App no longer writes the SecBoot state Flash directly.

For pending-attempt recovery testing, build PLB with `NO_CONFIRM=y` so the App starts but does not write `CONFIRMED`:

```bash
make NO_CONFIRM=y package
```

`tool/xy_secboot/dev_hmac_key.txt` is the lab-only key wired into the SecBoot-N32 development build. Override it with `--hmac-key` for CLI packaging or `SECBOOT_HMAC_KEY=...` for the PLB Makefile target.

## Current Security State

The tool can build packages, send `HELLO`, transfer `MANIFEST` and `DATA`, and send `END`.

SecBoot-N32 currently has SHA-256 hashing and a lab-only HMAC-SHA256 verification path wired in. Packages built without the matching HMAC key still fail at `END` with `IMAGE_VERIFY_FAILED`; packages built with `tool/xy_secboot/dev_hmac_key.txt` can complete `END` and write the App manifest.

Do not commit real production keys. The checked-in development HMAC key is not a production root of trust.
