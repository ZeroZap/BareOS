# Sec-boot-N32

`Sec-boot-N32` is a copy of the PLB-N32 board project redefined as the first N32L406 secure boot development target.

See `SECBOOT_GUIDE.md` for the end-to-end build, package, flash, GUI, and troubleshooting guide.

## V1 Scope

```text
single internal Flash application slot
+ UART5 transport development
+ xy_secboot framework integration
+ minimal bootloader memory layout
```

## UART

UART5 is used as the secboot development transport.

| Signal | Pin |
|---|---|
| UART5 TX | PB8 |
| UART5 RX | PB9 |

UART4 remains the debug log UART.

## Flash Layout

| Region | Address | Size |
|---|---:|---:|
| Bootloader | `0x08000000` | `0x6000` |
| Boot state | `0x08006000` | `0x0800` |
| Rollback | `0x08006800` | `0x0800` |
| App manifest | `0x08007000` | `0x0800` |
| App image | `0x08007800` | `0x16800` |
| EEPROM reserved | `0x0801E000` | `0x1000` |
| FEE reserved | `0x0801F000` | `0x1000` |

## Current Commands

UART5 keeps two single-byte debug helpers:

| Command | Action |
|---|---|
| `?` | Send UART5 banner |
| `p` | Print partition layout on UART4 log |

UART5 secboot traffic uses `XY Secboot UART Transport v1` frames:

| Packet | Direction | Status |
|---|---|---|
| `HELLO` | Host to bootloader | Implemented, replies `CAPS` |
| `CAPS` | Bootloader to host | Reports v1, 512-byte max payload, product, suite, app layout |
| `MANIFEST` | Host to bootloader | Basic manifest validation, erases App image area |
| `DATA` | Host to bootloader | Strict seq/offset, CRC32 payload check, internal Flash write/readback |
| `END` | Host to bootloader | Runs `xy_secboot_single_verify_active`, writes manifest only after success |
| `ABORT` | Host to bootloader | Cancels active receive session |
| `RESET` | Host to bootloader | ACK then MCU reset |

The V1 frame format follows `components/xy_secboot/docs/UART_TRANSPORT_V1.md`:

```text
magic       2 bytes   'S' 'B'
version     1 byte    1
type        1 byte
flags       1 byte
reserved    1 byte
seq         2 bytes
session_id  4 bytes
offset      4 bytes
length      2 bytes
header_crc  2 bytes   CRC16/CCITT over header with this field zeroed
payload     N bytes   max 512 bytes
payload_crc 4 bytes   CRC32 over payload
```

## After Flash

`END ACK` means the App image was written, verified, and the manifest was committed. It does not jump to the App immediately. On the next reset, the bootloader opens a short UART5 recovery window, then verifies the committed manifest and jumps to the App if no host input is present.

Recommended modes:

| Phase | Mode | Reason |
|---|---|---|
| Bring-up/debug | Use host `flash --reset` after `END ACK` | Exercises the same reset-time verification path as a power cycle |
| Recovery/update | Send any UART5 byte during the recovery window | Keeps the bootloader in download mode before it jumps App |
| Production | Verify manifest at reset, then jump to App | Power-cycle/reset boots the verified App without a host |

Example debug command:

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

If `--reset` is omitted, repeated `SecBoot-N32 heartbeat` logs after `END ACK` are expected until the next reset. The current recovery window is 1500 ms.

For manual recovery after a valid App is already installed, run the host command with a longer recovery preamble and reset the board during that window:

```bash
python tool/xy_secboot/xy_secboot.py flash \
  --port COM12 \
  --baud 115200 \
  --package build/app.sbp \
  --payload 256 \
  --timeout-ms 1000 \
  --retries 10 \
  --recover-ms 5000
```

`--recover-ms` means the host opens the serial port and repeatedly sends `?` for the requested duration before sending `HELLO`. Press the board reset key while this preamble is running. SecBoot will see UART5 input during its 1500 ms recovery window, stay in bootloader mode, and then the host will continue with normal `HELLO -> CAPS -> MANIFEST -> DATA -> END` flashing.

Use `--recover-ms` only when the board already has a valid App and resets into the App too quickly for a normal `flash` command. If UART4 already shows SecBoot heartbeat, a normal `flash` command is enough.

## Security State

V1 can receive and program an App image, but it intentionally does not accept an image as bootable unless `xy_secboot_single_verify_active()` succeeds. The current N32 development build supplies SHA-256 hashing, Flash/UART operations, and a lab-only HMAC-SHA256 manifest MAC using `tool/xy_secboot/dev_hmac_key.txt`.

This HMAC path is only for bring-up. Production still needs a protected root key or public-key verification path before release.

## Next Steps

| Step | Task |
|---:|---|
| 1 | Add boot public key storage/provisioning |
| 2 | Replace lab HMAC path with production ECDSA-P256 or protected-key MAC backend |
| 3 | Harden reset-time App jump and recovery policy |
| 4 | Replace lab direct App confirm with private system confirm path |
| 5 | Add host `xy-secpack` and `xy-secflash` tools |
