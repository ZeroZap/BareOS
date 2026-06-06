# XY Secboot UART Transport v1

## Scope

UART transport v1 is the first reliable transfer protocol for single-slot sec-boot recovery.

It is intentionally simple:

```text
stop-and-wait
+ fixed sequence number
+ offset check
+ packet CRC
+ ACK/NACK
+ timeout retransmission
+ final image hash and signature/MAC verification
```

UART transport v1 does not provide final security by itself. Packet CRC only detects transmission damage. The final security decision is still made by the sec-boot manifest, image hash, signature or MAC, and rollback check.

## Design Goals

| Goal | Decision |
|---|---|
| Small RAM | One packet buffer only |
| Simple recovery | Stop-and-wait, no sliding window |
| Deterministic Flash writes | Strict `seq` and `offset` ordering |
| Lost ACK handling | Duplicate DATA returns duplicate ACK |
| Corruption detection | CRC per packet |
| Security | Final manifest verification, not packet CRC |
| First implementation | No resume, no compression, no encryption |

## Protocol Flow

```text
Host                              Bootloader
----                              ----------
HELLO --------------------------> capability response
MANIFEST -----------------------> basic manifest check, ACK/NACK
DATA seq=0 offset=0 ------------> CRC, seq, offset, write, ACK
DATA seq=1 offset=N ------------> CRC, seq, offset, write, ACK
...
END ----------------------------> hash from Flash, verify, ACK/ERROR
RESET --------------------------> reset to normal boot path
```

## Packet Types

| Type | Direction | Purpose |
|---|---|---|
| `HELLO` | Host to bootloader | Start session and request capability |
| `CAPS` | Bootloader to host | Report protocol version, payload size, product, suite |
| `MANIFEST` | Host to bootloader | Send image metadata before erase/write |
| `DATA` | Host to bootloader | Send image chunk |
| `END` | Host to bootloader | Finish transfer and trigger full verification |
| `ACK` | Bootloader to host | Accept packet and report next offset |
| `NACK` | Bootloader to host | Reject packet with reason |
| `ERROR` | Bootloader to host | Abort current session |
| `RESET` | Host to bootloader | Request reboot after complete |
| `ABORT` | Host to bootloader | Cancel current session |

## Frame Format

Use little-endian fields.

```text
magic       2 bytes   'S' 'B'
version     1 byte    protocol version, v1 = 1
type        1 byte    packet type
flags       1 byte
reserved    1 byte
seq         2 bytes
session_id  4 bytes
offset      4 bytes
length      2 bytes   payload length
header_crc  2 bytes   CRC16 over fixed header with this field zeroed
payload     N bytes
payload_crc 4 bytes   CRC32 over payload
```

Fixed header size is 20 bytes. Maximum payload is reported by `CAPS`.

For very small MCUs, `session_id` can be fixed to zero in v1, but the field remains reserved for later resume and anti-replay improvements.

## ACK Format

`ACK`, `NACK`, and `ERROR` reuse the same frame header and carry this payload:

```text
ack_seq       2 bytes
reason        2 bytes
next_offset   4 bytes
detail        4 bytes
```

`next_offset` is the bootloader's expected next write offset. Host should use it to recover from lost ACKs or offset mismatch.

## NACK Reasons

| Reason | Meaning | Host Action |
|---|---|---|
| `BAD_HEADER_CRC` | Header damaged | Resend current packet |
| `BAD_PAYLOAD_CRC` | Payload damaged | Resend current packet |
| `BAD_SEQ` | Sequence mismatch | Resend expected packet |
| `BAD_OFFSET` | Offset mismatch | Seek to `next_offset` |
| `BAD_LENGTH` | Length is invalid | Abort or resend corrected packet |
| `BAD_MANIFEST` | Manifest failed basic checks | Abort |
| `FLASH_ERASE_FAILED` | Flash erase failed | Abort |
| `FLASH_WRITE_FAILED` | Flash write failed | Abort |
| `FLASH_VERIFY_FAILED` | Readback mismatch | Resend or abort |
| `IMAGE_VERIFY_FAILED` | Final hash/signature failed | Abort |
| `ROLLBACK_REJECTED` | Version/security counter too old | Abort |
| `BUSY` | Bootloader is erasing or verifying | Retry after delay |

## Stop-And-Wait Rules

Host rules:

| Rule | Requirement |
|---|---|
| Send one packet at a time | Wait for ACK before next DATA |
| Timeout | Resend current packet |
| Retry limit | Abort after repeated failures |
| NACK with next_offset | Re-seek and resend from that offset |
| ERROR | Abort session |

Bootloader rules:

| Rule | Requirement |
|---|---|
| CRC failure | Do not write payload |
| Expected seq | Accept and write |
| Duplicate seq | Do not write again, resend prior ACK |
| Future seq | NACK with expected seq/offset |
| Offset mismatch | NACK with `next_offset` |
| Flash write | Optional readback verify before ACK |

Duplicate DATA handling is mandatory because host may retransmit when the bootloader's ACK was lost.

## Recommended Parameters

| UART Speed | Payload | ACK Timeout | Retries |
|---|---:|---:|---:|
| 9600 / 19200 | 128 B | 2000 ms | 10 |
| 115200 | 256 B | 1000 ms | 10 |
| 460800 / 921600 | 512 B | 500 ms | 10 |
| Factory fixture | 512 B to 1024 B | 300 ms to 500 ms | 5 |

For the first implementation, default to:

```text
payload = 256 B
timeout = 1000 ms
retries = 10
```

## Flash Write Order

Single-slot direct update uses this order:

```text
1. Receive MANIFEST into RAM and check basic fields.
2. Mark boot state as receiving image.
3. Erase App image area.
4. Receive DATA packets in strict offset order.
5. Write each chunk to App image area.
6. Optionally read back each chunk before ACK.
7. Receive END.
8. Hash App image from Flash.
9. Verify manifest signature or firmware MAC.
10. Check anti-rollback counter.
11. Write App manifest last.
12. Reset and boot normal path.
```

Manifest is written last so an interrupted transfer cannot look like a valid image.

## Power-Loss Policy

v1 does not require resume.

After reset, if boot state says transfer was in progress, the bootloader enters UART recovery and asks the host to restart from offset zero.

Future versions may add resume by storing:

```text
session_id
expected_seq
expected_offset
page bitmap
page hash or page crc
```

## Security Boundary

Packet CRC is not security.

Final acceptance requires:

| Check | Required |
|---|---:|
| Product ID | Yes |
| Image address and size | Yes |
| Entry address | Yes |
| Image hash | Yes |
| Manifest signature or firmware MAC | Yes |
| Anti-rollback counter | Yes |
| Canary check before jump | Recommended |

## Version Roadmap

| Version | Planned Feature |
|---|---|
| v1 | Stop-and-wait, CRC, ACK/NACK, full retransmit |
| v2 | Resume from offset, page bitmap, stronger session state |
| v3 | External Flash staging for cellular/satellite download |
| v4 | Optional encrypted transport or AEAD packets |
| v5 | Sliding window for high-speed UART/USB |
