# XY Secboot v1 Design

## Scope

Secboot v1 is the first executable design target for the `xy_secboot` component.

It is intentionally narrow:

```text
single internal Flash application slot
+ UART reliable transfer v1
+ minimal development crypto suite
+ manifest-based verification
+ anti-rollback hook
+ host-side pack and flash tools
```

v1 is not the final PLB GM production suite. It is the first stable base used to validate boot flow, UART retransmission, Flash write ordering, package generation, and host/MCU interoperability.

## Non-Goals

| Item | v1 Decision |
|---|---|
| Full internal A/B | Later |
| Internal A + external B | Later |
| Swap partition | Later |
| Cellular/satellite OTA staging | Later |
| Resume after power loss | Later |
| Sliding window | Later |
| SM2 production verification | Later, use certified or vendor backend |
| Encrypted transport | Later |
| Model protection | Later |

## v1 Security Suite

Use a development-friendly minimal suite first:

```text
SHA-256 image hash
+ HMAC-SHA256 firmware MAC over manifest signing scope
```

Reason:

| Reason | Detail |
|---|---|
| Current implementation exists | `components/crypto` already has SHA-256/HMAC-SHA256 paths |
| Small MCU friendly | No ECC big-number cost |
| Good for UART/Flash validation | Lets the transport and boot state mature first |
| Replaceable | The same `xy_secboot_crypto_ops_t` backend can later bind SM3/SM2 |

v1 production warning:

```text
HMAC-SHA256 requires a protected device key.
If the key is readable from normal Flash, this is not a production secure boot root of trust.
```

## v1 Flash Layout

```text
+----------------------+  Internal Flash base
| Bootloader           |  write-protected
+----------------------+
| Boot state           |  bootloader private, two records if possible
+----------------------+
| Rollback records     |  bootloader private or OTP
+----------------------+
| App manifest         |  written last
+----------------------+
| App image            |  single executable slot
+----------------------+
| Param/FEE            |  application data
+----------------------+
| Evtlog               |  application data
+----------------------+  Internal Flash end
```

The active application is considered valid only when:

```text
manifest valid
+ image hash valid
+ manifest MAC valid
+ rollback check valid
```

## Boot Flow

```text
reset
  |
  v
bootloader init
  |
  v
read App manifest
  |
  v
basic manifest checks
  |
  v
stream hash App image from Flash
  |
  v
verify manifest HMAC
  |
  v
check rollback counter
  |
  +-- OK -----> jump app
  |
  +-- fail ---> UART recovery
```

## UART Recovery Flow

UART recovery uses `UART_TRANSPORT_V1.md`.

```text
receive HELLO
send CAPS
receive MANIFEST
check manifest basics
mark state RECEIVING_IMAGE
erase App image area
receive DATA packets with ACK/NACK
receive END
hash image from Flash
verify manifest MAC
check rollback
write manifest last
reset
```

## Host Tools

v1 requires two host tools:

| Tool | Required | Purpose |
|---|---:|---|
| `xy-secpack` | Yes | Build `.sbp` package from `app.bin` |
| `xy-secflash` | Yes | Send `.sbp` to bootloader over UART v1 |
| `xy-secinspect` | Optional | Print package and manifest fields |

Use Python first:

```text
Python 3
+ pyserial
+ hashlib / hmac
+ argparse
```

## v1 Package Format

File extension:

```text
.sbp
```

Package layout:

```text
offset  size  field
0       4     magic = 'XSBP'
4       2     package_version = 1
6       2     header_len
8       4     suite_id
12      4     manifest_len
16      4     image_len
20      4     package_crc32
24      N     manifest
24+N    M     image
```

Package CRC is for host/file damage detection. It is not a security decision.

## Manifest v1

Manifest uses `xy_secboot_manifest_t` with little-endian encoding.

Required fields:

| Field | v1 Requirement |
|---|---|
| `magic` | `XY_SECBOOT_MANIFEST_MAGIC` |
| `header_version` | `XY_SECBOOT_MANIFEST_VERSION` |
| `header_len` | `sizeof(xy_secboot_manifest_t)` |
| `product_id` | Must match device |
| `image_type` | `XY_SECBOOT_IMAGE_APP` |
| `image_addr` | App slot image address |
| `image_size` | App binary size |
| `entry_addr` | App vector/entry address |
| `image_version` | Monotonic app version |
| `security_counter` | Anti-rollback counter |
| `image_hash` | SHA-256 digest in v1 |
| `signature` | HMAC-SHA256 tag in v1 |

v1 MAC input:

```text
manifest bytes from offset 0 to offsetof(xy_secboot_manifest_t, signature)
```

## Tool Commands

Pack:

```bash
python tools/xy_secboot/xy_secpack.py pack \
  --suite minimal-hmac-sha256 \
  --product-id 0x00010001 \
  --image-addr 0x08004000 \
  --entry-addr 0x08004000 \
  --version 1 \
  --security-counter 1 \
  --key-id dev-key-01 \
  --hmac-key keys/dev_hmac.key \
  --in build/app.bin \
  --out build/app.sbp
```

Flash:

```bash
python tools/xy_secboot/xy_secflash.py uart \
  --port COM12 \
  --baud 115200 \
  --package build/app.sbp \
  --payload 256 \
  --timeout-ms 1000 \
  --retries 10
```

Inspect:

```bash
python tools/xy_secboot/xy_secinspect.py build/app.sbp
```

## MCU v1 Milestones

| Step | Deliverable |
|---:|---|
| 1 | `xy_secboot_single_verify_active()` integrated with a real minimal crypto backend |
| 2 | Read manifest from Flash and enter recovery on failure |
| 3 | UART `HELLO` / `CAPS` |
| 4 | UART `MANIFEST` basic checks |
| 5 | UART `DATA` stop-and-wait write path |
| 6 | UART `END` full verify |
| 7 | Manifest written last |
| 8 | Reset and normal boot |

## Host v1 Milestones

| Step | Deliverable |
|---:|---|
| 1 | `.sbp` pack/unpack |
| 2 | HMAC-SHA256 manifest tag generation |
| 3 | UART frame encode/decode |
| 4 | Stop-and-wait transfer |
| 5 | ACK/NACK handling |
| 6 | Timeout retransmission |
| 7 | Fault injection options |

## Fault Injection Tests

v1 host should support these test modes:

| Test | Expected Bootloader Behavior |
|---|---|
| Corrupt DATA CRC | NACK, no Flash write |
| Drop ACK | Host retransmits, bootloader resends ACK |
| Duplicate DATA | Bootloader does not write twice |
| Wrong seq | NACK BAD_SEQ |
| Wrong offset | NACK BAD_OFFSET with next offset |
| Corrupt final image | END verify fails |
| Old security counter | Rollback reject |
| Reset during transfer | Re-enter recovery and require full restart |

## v1 Done Definition

v1 is complete when:

| Requirement | Status |
|---|---:|
| Host can generate `.sbp` | Required |
| Host can transmit over UART v1 | Required |
| Bootloader can receive and write image | Required |
| Bootloader verifies hash and HMAC from Flash | Required |
| Bootloader rejects corrupted image | Required |
| Bootloader rejects old security counter | Required |
| Lost ACK retransmission works | Required |
| Duplicate DATA is safe | Required |
| Manifest is written last | Required |
| On failure, bootloader stays recoverable | Required |

## Iteration Plan

| Version | Upgrade |
|---|---|
| v1.0 | Single-slot UART full retransmit, HMAC-SHA256 |
| v1.1 | Host fault injection and regression tests |
| v1.2 | External Flash staging hooks |
| v2.0 | Resume from offset and page bitmap |
| v3.0 | PLB GM backend: SM3/HMAC-SM3/SM2 verify adapter |
