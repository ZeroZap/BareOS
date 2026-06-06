# XY Secboot

`xy_secboot` is the secure boot policy and abstraction component for BareOS.

It defines stable interfaces for selecting cryptographic suites, describing signed manifests, binding crypto backends, authenticating outputs, and planning secure Flash partitions.

It does not implement cryptographic algorithms directly. Backends may come from `components/crypto`, `components-ref/crypto`, chip HAL, a certified commercial crypto library, or a secure element.

## Files

| Path | Purpose |
|---|---|
| `inc/xy_secboot_config.h` | Suite selection and resource limits |
| `inc/xy_secboot_crypto.h` | Algorithm IDs, manifest format, crypto backend ops |
| `inc/xy_secboot_partition.h` | Partition descriptors and range-check interface |
| `docs/SECBOOT_CRYPTO_POLICY.md` | Algorithm suite policy and PLB GM defaults |
| `docs/PARTITION_POLICY.md` | Single-slot, A/B, rollback, and PLB partition guidance |

## Default Policy

The default suite is `XY_SECBOOT_SUITE_PLB_GM`:

```text
SM2 signature verification
+ SM3 streaming image hash
+ HMAC-SM3 output/model authentication where enabled
+ anti-rollback counter
+ constant-time compare and SRAM canary
```

## Integration Notes

Production secure boot must bind these interfaces to complete and reviewed crypto implementations.

The current `components/crypto` ECDSA implementation is not suitable for production secure boot. The `components-ref/crypto` SM2 implementation is marked simplified and should be replaced by a certified or vendor-backed implementation for compliance builds.
