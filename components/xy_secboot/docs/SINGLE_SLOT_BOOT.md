# XY Secboot Single-Slot Boot

## Scope

Single-slot sec-boot is the first implementation target for the smallest devices.

It assumes one executable application slot in internal MCU Flash. If the active image is invalid or the user forces recovery, the bootloader enters a UART download mode and streams a new signed image into the same application area.

## Memory Layout

```text
+----------------------+  Internal Flash base
| Bootloader           |  write-protected
+----------------------+
| Boot state           |  bootloader private, two records if possible
+----------------------+
| Rollback records     |  bootloader private or OTP
+----------------------+
| App manifest         |  signed or MACed
+----------------------+
| App image            |  single executable slot
+----------------------+
| Param/FEE            |  application data
+----------------------+
| Evtlog               |  application data
+----------------------+  Internal Flash end
```

## Boot Flow

```text
reset
  |
  v
init clock, watchdog, minimal UART
  |
  v
load manifest
  |
  v
check range, product_id, image_type, version
  |
  v
stream hash active image
  |
  v
verify signature or MAC
  |
  v
check anti-rollback counter
  |
  +-- valid --> jump app
  |
  +-- invalid or forced recovery --> UART recovery
```

## UART Recovery Flow

```text
enter recovery
  |
  v
receive manifest header
  |
  v
validate address, size, version, product_id
  |
  v
erase app image area
  |
  v
receive image chunks and write directly to app area
  |
  v
write manifest last
  |
  v
verify full image again from Flash
  |
  v
update rollback counter if policy allows
  |
  v
reset and boot normal path
```

## Packet Strategy

Use small packets to keep RAM bounded.

| MCU Class | Packet Payload | Read Buffer |
|---|---:|---:|
| Tiny M0/M0+ | 128 B to 256 B | 256 B |
| Low-end M3/M4 | 256 B to 512 B | 512 B |
| Larger M4/M33 | 512 B to 1024 B | 1024 B |

Each packet should include:

```text
type
sequence
offset
payload_len
payload
crc16 or crc32 for transport damage detection
```

The final security decision must use the manifest signature or MAC, not the packet CRC.

## Power-Loss Behavior

Single-slot cannot guarantee an always-bootable application during update. It must guarantee an always-available bootloader recovery path.

Rules:

| Rule | Reason |
|---|---|
| Manifest is written last | Prevent half-written images from looking valid |
| Boot state marks `receive_image` | Detect interrupted update |
| Existing app is considered invalid after erase starts | Avoid jumping into partial image |
| Recovery UART is always available | Device can be repaired after failed update |
| Rollback counter updates only after verification | Avoid burning counter for bad image |

## State Record

Use two boot state records when one Flash page is available. Select the valid record with the largest sequence.

```c
typedef struct {
    uint32_t magic;
    uint32_t seq;
    uint32_t state;
    uint32_t state_inv;
    uint32_t image_version;
    uint32_t image_version_inv;
    uint32_t received_size;
    uint32_t crc32;
} xy_secboot_single_record_t;
```

`state_inv` and `image_version_inv` must be bitwise inverses.

## Security Checks

Mandatory checks before jumping to the app:

| Check | Required |
|---|---:|
| Manifest magic and header length | Yes |
| Product ID | Yes |
| Image address and size range | Yes |
| Entry address range | Yes |
| Image hash | Yes |
| Signature or firmware MAC | Yes |
| Anti-rollback counter | Yes |
| Constant-time digest/tag compare | Yes |
| Canary check before jump | Recommended |

## First Implementation Boundary

The first implementation should include only:

| Item | Status |
|---|---|
| Single internal app slot | In scope |
| UART recovery/download | In scope |
| Streaming hash from Flash | In scope |
| Manifest verification | In scope |
| Anti-rollback read/check/write hooks | In scope |
| Cellular/satellite download | Out of scope, later maps to staging storage |
| External Flash B slot | Out of scope, documented in partition policy |
| Swap partition | Out of scope, documented in partition policy |
