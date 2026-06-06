# XY Secboot Partition Policy

## Goals

The partition layout must protect the boot chain, preserve recovery paths, support rollback prevention, and avoid bricking the device on interrupted update.

Security algorithms cannot replace partition isolation. The bootloader must be write-protected, and the application must not be able to erase rollback or boot state storage.

## Partition Types

| Partition | Required | Access | Purpose |
|---|---:|---|---|
| Bootloader | Yes | Read-only after provisioning | Root of trust, public keys, boot policy |
| Boot state | Recommended | Bootloader write, App restricted | Pending/confirmed slot state, boot attempt counter |
| Rollback counter | Yes | Bootloader write only | Monotonic accepted security version |
| App A manifest | Yes | Bootloader read/write during update | Signed metadata for slot A |
| App A image | Yes | Bootloader read/write during update | Primary or confirmed application |
| App B manifest | Recommended | Bootloader read/write during update | Signed metadata for slot B |
| App B image | Recommended | Bootloader read/write during update | Update and recovery application |
| Model manifest | Optional | Bootloader/App controlled | Model metadata, MAC, nonce, version |
| Model image | Optional | Encrypted or authenticated | Model weights/rules |
| Param/FEE | Product-specific | App write | Device parameters, not trusted for rollback |
| Evtlog | Product-specific | App write | Event log, not trusted for boot decisions |
| Factory/NV keys | Optional | Locked or secure read | Calibration, injected keys, device identity |

## Minimal Single-Slot Layout

Use this when Flash is too small for A/B images.

```text
+----------------------+  Flash base
| Bootloader           |  write-protected
+----------------------+
| Boot state           |  bootloader private
+----------------------+
| Rollback records     |  bootloader private, two copies
+----------------------+
| App manifest         |  signed/MACed
+----------------------+
| App image            |  verified before jump
+----------------------+
| Param/FEE            |  application data
+----------------------+
| Evtlog               |  application data
+----------------------+  Flash end
```

Pros:

| Item | Result |
|---|---|
| Flash cost | Lowest |
| Boot complexity | Low |
| Recovery from interrupted update | Weak unless update is carefully staged |
| Brick risk | Higher than A/B |

Single-slot rules:

| Rule | Requirement |
|---|---|
| Write new image | Use a temporary staging area if possible |
| Update rollback counter | Only after image verification and activation decision |
| Power loss | Must leave at least one bootable image or enter recovery mode |
| Recovery | UART/USB/4G recovery command should be available |

## Recommended A/B Layout

Use this for PLB or field devices where maintenance is difficult.

```text
+----------------------+  Flash base
| Bootloader           |  write-protected
+----------------------+
| Boot state           |  bootloader private
+----------------------+
| Rollback records     |  bootloader private, two copies
+----------------------+
| App A manifest       |
+----------------------+
| App A image          |
+----------------------+
| App B manifest       |
+----------------------+
| App B image          |
+----------------------+
| Model manifest       |  optional
+----------------------+
| Model image          |  optional encrypted/authenticated
+----------------------+
| Param/FEE            |
+----------------------+
| Evtlog               |
+----------------------+
| Factory/NV           |  optional locked page
+----------------------+  Flash end
```

Pros:

| Item | Result |
|---|---|
| Flash cost | About 2x app image slots |
| Boot complexity | Medium |
| Recovery from interrupted update | Strong |
| Brick risk | Low |

A/B state machine:

| State | Meaning | Bootloader Action |
|---|---|---|
| `empty` | Slot has no valid image | Ignore |
| `pending` | New slot written but not confirmed | Verify and boot with attempt counter |
| `confirmed` | Slot is known good | Prefer when no pending slot exists |
| `bad` | Slot failed verification or boot attempts | Do not boot |

Update sequence:

| Step | Action |
|---:|---|
| 1 | Select inactive slot |
| 2 | Erase inactive manifest and image area |
| 3 | Write image chunks |
| 4 | Write manifest last |
| 5 | Mark slot `pending` in boot state |
| 6 | Reboot |
| 7 | Bootloader verifies pending slot |
| 8 | App marks itself `confirmed` after stable startup |
| 9 | Bootloader updates rollback counter only after policy allows it |

## Boot State Record

Keep boot state in bootloader-private Flash or option-byte-protected storage.

```c
typedef struct {
    uint32_t magic;
    uint32_t seq;
    uint32_t active_slot;
    uint32_t pending_slot;
    uint32_t confirmed_slot;
    uint32_t boot_attempts;
    uint32_t last_reset_cause;
    uint32_t state_inv;
    uint32_t crc32;
} xy_secboot_state_record_t;
```

Store two copies and choose the valid record with the largest `seq`.

## Rollback Record

The rollback counter is not normal application configuration. Do not store it in a writable parameter area unless the application cannot erase it.

```c
typedef struct {
    uint32_t magic;
    uint32_t seq;
    uint32_t counter;
    uint32_t counter_inv;
    uint32_t crc32;
} xy_secboot_rollback_record_t;
```

Rules:

| Rule | Requirement |
|---|---|
| Accept image | `manifest.security_counter >= stored_counter` |
| Reject rollback | If manifest counter is lower |
| Update counter | Only after signature/MAC verification and activation policy |
| Fault detection | Require `counter == ~counter_inv` and CRC valid |

## PLB Partition Recommendation

For PLB/vessel distress products, prefer A/B even if it reduces maximum application size. Field recovery is costly and the distress function must remain available.

Recommended PLB layout:

| Partition | Size Rule | Notes |
|---|---|---|
| Bootloader | Fixed, write-protected | Includes SM2 public key, SM3, policy, recovery command |
| Boot state | 1 to 2 erase pages | Two-copy records |
| Rollback | 1 to 2 erase pages or OTP | OTP/eFuse preferred |
| App A manifest | 1 erase page | Written after image |
| App A image | Product-sized | Confirmed factory image initially |
| App B manifest | 1 erase page | Update slot |
| App B image | Same as A if possible | Recovery from failed update |
| Param/FEE | Existing product size | Do not trust for secure decisions |
| Evtlog | Existing product size | Useful for reset/update diagnostics |
| Factory/NV | 1 to 2 pages or secure storage | Device ID, injected keys, calibration |

If Flash is too small for equal A/B slots, use one large factory slot and one smaller recovery slot only if the recovery firmware can perform communications and update.

## External NOR Flash

If application or model assets live in external NOR Flash:

| Requirement | Reason |
|---|---|
| Verify before execute | External Flash is easier to modify |
| Authenticate model chunks | Avoid loading tampered weights |
| Store keys internally | External Flash must not contain plaintext keys |
| Bind manifest address and size | Prevent relocation or truncation attacks |
| Use anti-rollback internally | External Flash cannot be trusted for monotonic state |

For XIP from external Flash, verify the full image before enabling execution. If full verification time is unacceptable, keep a small internal recovery app that can re-verify and repair the external image.

## Alignment

All manifest, state, rollback, and image partitions should be aligned to the Flash erase size. Use `XY_SECBOOT_PARTITION_ALIGNMENT` as the minimum policy alignment.

## Open Decisions

Before finalizing a product layout, decide:

| Decision | Options |
|---|---|
| Slot mode | Single-slot, A/B, A + recovery |
| Rollback storage | OTP/eFuse, option bytes, private Flash |
| Update channel | Factory only, UART/USB, 4G, satellite |
| Transport security | None, SM4 AEAD, existing secure channel |
| Model storage | None, internal Flash, external NOR |
| Output authentication | Disabled, HMAC-SM3, BLAKE2s MAC, HMAC-SHA256 |
