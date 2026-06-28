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

`tool/xy_secboot/dev_hmac_key.txt` is the lab-only key wired into the SecBoot-N32 development build. Override it with `--hmac-key` for CLI packaging or `SECBOOT_HMAC_KEY=...` for the PLB Makefile target.

## Current Security State

The tool can build packages, send `HELLO`, transfer `MANIFEST` and `DATA`, and send `END`.

SecBoot-N32 currently has SHA-256 hashing and a lab-only HMAC-SHA256 verification path wired in. Packages built without the matching HMAC key still fail at `END` with `IMAGE_VERIFY_FAILED`; packages built with `tool/xy_secboot/dev_hmac_key.txt` can complete `END` and write the App manifest.

Do not commit real production keys. The checked-in development HMAC key is not a production root of trust.
