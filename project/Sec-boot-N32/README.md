# Sec-boot-N32

`Sec-boot-N32` is a copy of the PLB-N32 board project redefined as the first N32L406 secure boot development target.

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
| `CAPS` | Bootloader to host | Reports v1, 256-byte max payload, product, suite, app layout |
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
payload     N bytes   max 256 bytes
payload_crc 4 bytes   CRC32 over payload
```

## Security State

V1 can receive and program an App image, but it intentionally does not accept an image as bootable unless `xy_secboot_single_verify_active()` succeeds. The current N32 port supplies SHA-256 hashing and Flash/UART operations, while public-key verification/key provisioning still returns unsupported. This prevents accidentally shipping an unsigned image path.

## Next Steps

| Step | Task |
|---:|---|
| 1 | Add boot public key storage/provisioning |
| 2 | Add ECDSA-P256 verification backend or switch policy to HMAC-SHA256 for lab builds |
| 3 | Add app jump after verified manifest boot check |
| 4 | Persist boot state/rollback counter in reserved Flash pages |
| 5 | Add host `xy-secpack` and `xy-secflash` tools |
