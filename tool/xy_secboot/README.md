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
  --security-counter 1
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

Launch GUI:

```bash
python tool/xy_secboot/xy_secboot.py gui
```

## Package Notes

The packer emits the `.sbp` package described in `components/xy_secboot/docs/SECBOOT_V1_DESIGN.md`.

The MCU Flash write path requires DATA lengths to be 4-byte aligned. The packer pads the input image with `0xFF` to a 4-byte boundary before calculating the image hash and manifest fields.

## Current Security State

The tool can build packages, send `HELLO`, transfer `MANIFEST` and `DATA`, and send `END`.

SecBoot-N32 currently has SHA-256 hashing wired in, but final public-key verification/key provisioning is still pending on the MCU side. Until that backend is added, `END` may return `IMAGE_VERIFY_FAILED`. That is expected and prevents accidentally accepting unsigned images.

Do not commit real production keys. Development HMAC key support is provided only for lab packages and future MCU backend work.
