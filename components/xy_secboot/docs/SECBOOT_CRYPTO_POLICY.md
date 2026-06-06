# XY Secboot Crypto Policy

## Scope

`xy_secboot` defines the policy and abstraction layer for secure boot, protected model assets, encrypted upgrade transport, output authentication, rollback prevention, and low-cost side-channel hardening.

It does not own the cryptographic implementations. Backends can be selected from `components/crypto`, `components-ref/crypto`, a chip HAL, a certified commercial crypto library, or a secure element.

## Security Slots

| Slot | Purpose | Typical Algorithms |
|---|---|---|
| Firmware source authentication | Prove image origin | Ed25519, ECDSA-P256, SM2, keyed MAC for closed products |
| Firmware integrity | Bind manifest to image bytes | BLAKE2s-256, SHA-256, SM3 |
| Firmware tamper resistance | Reject modified manifest or image | Signature or MAC over manifest fields and image hash |
| Model encryption | Hide model weights or rules | ChaCha20-256, AES-CTR, SM4-CTR |
| Model integrity | Reject modified model assets | BLAKE2s keyed MAC, HMAC-SHA256, HMAC-SM3 |
| Transport protection | Protect OTA/update packets | ChaCha20-Poly1305, AES-GCM, SM4-GCM, SM4-CBC + HMAC-SM3 |
| Output authentication | Authenticate reported inference or distress output | BLAKE2s keyed MAC, HMAC-SHA256, HMAC-SM3 |
| Rollback prevention | Reject old valid images | OTP/eFuse monotonic counter or bootloader-private Flash |
| Basic side-channel hardening | Reduce low-cost timing/glitch attacks | constant-time compare, secure zero, SRAM canary, redundant checks |

## Built-In Suites

| Suite | Firmware Source | Firmware Hash | Firmware Tamper Protection | Model Encryption | Model Integrity | Transport | Output Auth | Rollback | Side-Channel Baseline | Main Use |
|---|---|---|---|---|---|---|---|---|---|---|
| `XY_SECBOOT_SUITE_MINIMAL` | BLAKE2s keyed MAC or HMAC-SHA256 | BLAKE2s-256 or SHA-256 | MAC over manifest and image hash | None | None | None | Optional MAC | Private Flash | constant-time compare, canary | Very small closed devices with protected symmetric key |
| `XY_SECBOOT_SUITE_MODERN` | Ed25519 | BLAKE2s-256 | Ed25519 over manifest and image hash | Optional ChaCha20 | Optional BLAKE2s keyed MAC | Optional ChaCha20-Poly1305 | Optional BLAKE2s keyed MAC | OTP/eFuse or private Flash | constant-time compare, canary | Small non-GM products without AES/ECC hardware |
| `XY_SECBOOT_SUITE_MARKET` | ECDSA-P256 | SHA-256 | ECDSA over manifest and image hash | AES-CTR/GCM optional | HMAC-SHA256 or GCM tag | TLS/DTLS or AES-GCM | HMAC-SHA256 | OTP/eFuse | constant-time compare, canary | Common industrial or consumer products with hardware crypto |
| `XY_SECBOOT_SUITE_GM` | SM2 | SM3 | SM2 over manifest and SM3 image hash | SM4-CTR/CBC | HMAC-SM3 | SM4-GCM or SM4-CBC + HMAC-SM3 | HMAC-SM3 | OTP/eFuse or private Flash | constant-time compare, canary, redundant checks | Products with mandatory Chinese commercial crypto requirements |
| `XY_SECBOOT_SUITE_PLB_GM` | SM2 | SM3 | SM2 over manifest and SM3 image hash | Disabled by default | HMAC-SM3 if model exists | SM4-GCM or SM4-CBC + HMAC-SM3 for OTA | HMAC-SM3 + counter | Private Flash, OTP preferred | constant-time compare, canary, redundant checks | PLB/vessel distress products requiring SM algorithms |
| `XY_SECBOOT_SUITE_MODEL_SECURE` | Ed25519 | BLAKE2s-256 | Ed25519 over manifest and image hash | ChaCha20-256 | BLAKE2s keyed MAC | ChaCha20-Poly1305 | BLAKE2s keyed MAC + counter | OTP/eFuse | constant-time compare, canary, optional double-run | AI/model asset protection on low-resource MCU |

## PLB Default

For PLB products that require SM algorithms, use:

```c
#define XY_SECBOOT_SUITE                    XY_SECBOOT_SUITE_PLB_GM
#define XY_SECBOOT_ENABLE_MODEL_PROTECT     0u
#define XY_SECBOOT_ENABLE_TRANSPORT_AEAD    1u
#define XY_SECBOOT_ENABLE_OUTPUT_AUTH       1u
#define XY_SECBOOT_ENABLE_CANARY            1u
#define XY_SECBOOT_ENABLE_DOUBLE_RUN        0u
#define XY_SECBOOT_READ_BUFFER_SIZE         512u
```

The default PLB chain is:

```text
SM2 verify(manifest signature)
+ SM3 streaming image hash
+ HMAC-SM3 for output authentication and optional model integrity
+ anti-rollback counter
+ constant-time compare and SRAM canary
```

If a PLB SKU has no OTA update channel, disable transport AEAD:

```c
#define XY_SECBOOT_ENABLE_TRANSPORT_AEAD 0u
```

## Manifest Signing Scope

The signature or firmware MAC must cover all security-sensitive manifest fields, not only the image hash.

Required covered fields:

| Field | Reason |
|---|---|
| `magic` | Prevent format confusion |
| `header_version` | Prevent downgrade to an old manifest format |
| `product_id` | Prevent cross-product image reuse |
| `image_type` | Bind App, model, parameter, or boot patch type |
| `image_addr` | Prevent relocation to unsafe address |
| `image_size` | Prevent truncation or extension |
| `entry_addr` | Prevent jump target tampering |
| `image_version` | Support rollback prevention |
| `min_boot_version` | Prevent a new image from running on an old bootloader |
| `security_counter` | Bind monotonic security version |
| `key_id` | Support key rotation |
| `nonce` | Bind encrypted payload nonce |
| `image_hash` | Bind image bytes |

## Backend Notes

The `components-ref/crypto` SM2 header marks the implementation as simplified. For compliance or production PLB builds, keep the `xy_secboot` interface stable but bind it to a certified SM2/SM3/SM4 library, chip vendor security library, hardware secure engine, or secure element.

The current `components/crypto` ECDSA implementation is not suitable for production secure boot because it does not perform full signature verification. Do not use it for `XY_SECBOOT_SUITE_MARKET` until it is replaced.

## Resource Rules

Use streaming hash over the image. Do not copy the whole image into RAM.

Recommended read buffer sizes:

| MCU Class | Buffer |
|---|---:|
| Tiny M0/M0+ | 256 B |
| Low-end M3/M4 | 512 B |
| Larger M4/M33 | 1024 B to 2048 B |

Disable algorithms that are not used by the selected suite. A verify-only bootloader usually does not need CSPRNG, Base64, Hex, MD5, SHA1, RSA, or software AES.

## Output Authentication

For PLB distress messages or remote status reports, authenticate output with:

```text
tag = HMAC-SM3(output_key,
               device_id || message_type || boot_counter ||
               output_counter || timestamp || payload)
```

The receiver must reject stale `output_counter` values to prevent replay.

## Side-Channel Baseline

Always enable:

| Control | Cost | Purpose |
|---|---:|---|
| constant-time compare | Very low | Avoid tag/signature timing leaks |
| secure zero | Very low | Clear keys, tags, temporary buffers |
| SRAM canary | Low | Detect stack/SRAM corruption and simple glitch effects |
| inverse fields | Low | Detect bit-flips in boot state and rollback records |

Enable only for higher security SKUs:

| Control | Cost | Purpose |
|---|---:|---|
| double-run verification | Medium | Detect fault injection around critical decisions |
| blinded model LUT | High | Reduce model side-channel leakage |
| randomized dummy operations | Medium | Make timing/power traces less stable |
