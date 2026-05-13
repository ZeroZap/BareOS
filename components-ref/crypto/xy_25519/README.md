# XY_25519: Unified Curve25519 Library

## Overview

**xy_25519** is a unified cryptographic library that combines X25519 key exchange (RFC 7748) and Ed25519 digital signatures (RFC 8032) into a single, optimized module. It provides both ECDH key agreement and digital signature capabilities with shared field arithmetic operations for maximum code reuse.

## Features

### X25519 ECDH (Key Exchange)
- **RFC 7748 Compliant**: Standards-based implementation
- **Montgomery Ladder**: Constant-time scalar multiplication
- **Key Validation**: Low-order point rejection
- **Key Sizes**: 32-byte private/public keys
- **Security**: Prevents timing attacks through constant-time operations

### Ed25519 (Digital Signatures)
- **RFC 8032 Compliant**: Complete EdDSA implementation
- **Edwards Curve**: Twisted Edwards curve operations
- **Deterministic**: Same signature for same message and key
- **Security**: 128-bit security level
- **Key Sizes**: 32-byte keys, 64-byte signatures

### Shared Architecture
- **Unified Field Arithmetic**: Common modulo 2^255-19 operations
- **Code Reuse**: ~50% reduction vs separate implementations
- **Consistent Security**: Single optimization benefits both algorithms
- **Optimal Size**: ~8KB Flash for both algorithms

## Development History

### Phase 1: Initial Separate Implementations
**Status**: ✅ Completed

Created two independent implementations:
- **xy_x25519** (X25519 ECDH)
  - Montgomery ladder scalar multiplication
  - Constant-time field operations
  - Key pair generation and validation
  - Shared secret computation

- **xy_ed25519** (Ed25519 signatures)
  - Edwards curve point operations
  - Deterministic signing algorithm
  - Signature generation and verification
  - RFC 8032 test vectors

**Files Created**:
- `xy_x25519/xy_x25519.h` (129 lines)
- `xy_x25519/xy_x25519.c` (588 lines)
- `xy_ed25519/xy_ed25519.h` (166 lines)
- `xy_ed25519/xy_ed25519.c` (986 lines)

**Total**: 1,869 lines of code

### Phase 2: Code Consolidation
**Status**: ✅ Completed

Analyzed common operations between X25519 and Ed25519:
- Field arithmetic (mod 2^255-19)
- Data type definitions
- Utility functions
- Field element operations

**Shared Operations Identified**:
- `fe_frombytes()` / `fe_tobytes()` - byte conversions
- `fe_add()` / `fe_sub()` - field arithmetic
- `fe_mul()` / `fe_sq()` - multiplication/squaring
- `fe_invert()` - modular inversion
- `fe_cswap()` / `fe_cmov()` - constant-time operations

**Code Reduction**: ~450 lines of duplicated field arithmetic

### Phase 3: Unified Module Creation
**Status**: ✅ Completed

Created single `xy_25519` module combining both algorithms:

**Architecture**:
```
xy_25519.h
├── X25519 API (4 functions)
│   ├── xy_x25519_generate_keypair()
│   ├── xy_x25519_public_key()
│   ├── xy_x25519_shared_secret()
│   └── xy_x25519_validate_public_key()
│
└── Ed25519 API (5 functions)
    ├── xy_ed25519_generate_keypair()
    ├── xy_ed25519_public_key()
    ├── xy_ed25519_sign()
    ├── xy_ed25519_verify()
    └── xy_ed25519_sign_simple()

xy_25519.c
├── Shared Field Arithmetic (250 lines)
│   ├── Field element operations
│   ├── Modular arithmetic (mod 2^255-19)
│   └── Constant-time utilities
├── X25519 Implementation (150 lines)
│   └── Montgomery ladder + helpers
└── Ed25519 Implementation (250 lines)
    └── Edwards curve operations
```

**Files**:
- `xy_25519.h` (186 lines)
- `xy_25519.c` (684 lines)
- `Makefile` (40 lines)

**Total**: 910 lines (51% reduction from 1,869 lines)

### Phase 4: RULEBOOK Compliance
**Status**: ✅ Completed

Updated to follow XinYi framework standards:

**Include Pattern**:
```c
// Before (incorrect deep nesting)
#include "../../../xy_clib/xy_string.h"

// After (correct relative depth)
#include "xy_string.h"
```

**Logging Compliance**:
```c
// Before (prohibited)
xy_printf("Error: %s\n", msg);

// After (required)
#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_INFO
#include "xy_log.h"
xy_log_e("Error: %s\n", msg);
```

**Changes Made**:
- Updated 28+ printf/xy_printf calls to xy_log_* macros
- Fixed include paths to use relative references
- Added LOCAL_LOG_LEVEL configuration
- Ensured C99 compliance
- Verified no standard library dependencies

### Phase 5: Header Integration
**Status**: ✅ Completed

Integrated with main crypto library:

**xy_tiny_crypto.h Updates**:
- Replaced separate X25519/Ed25519 sections
- Created unified `XY_CRYPTO_ENABLE_CURVE25519` macro
- Consolidated all API declarations
- Maintained backward compatibility

**xy_crypto_config.h Updates**:
- Added `XY_CRYPTO_ENABLE_CURVE25519` configuration option
- Provided legacy compatibility flags
- Documented configuration choices

## API Reference

### X25519 ECDH Functions

#### Generate Key Pair
```c
int xy_x25519_generate_keypair(
    uint8_t private_key[32],
    uint8_t public_key[32]);
```
**Returns**: `XY_X25519_SUCCESS` on success

#### Compute Public Key
```c
int xy_x25519_public_key(
    const uint8_t private_key[32],
    uint8_t public_key[32]);
```
**Returns**: `XY_X25519_SUCCESS` on success

#### Compute Shared Secret
```c
int xy_x25519_shared_secret(
    uint8_t shared_secret[32],
    const uint8_t our_private_key[32],
    const uint8_t their_public_key[32]);
```
**Returns**: `XY_X25519_SUCCESS` or `XY_X25519_ERROR_WEAK_KEY`

#### Validate Public Key
```c
int xy_x25519_validate_public_key(
    const uint8_t public_key[32]);
```
**Returns**: `XY_X25519_SUCCESS` or `XY_X25519_ERROR_WEAK_KEY`

### Ed25519 Signature Functions

#### Generate Key Pair
```c
int xy_ed25519_generate_keypair(
    uint8_t public_key[32],
    uint8_t private_key[32]);
```
**Returns**: `XY_ED25519_SUCCESS` on success

#### Derive Public Key
```c
int xy_ed25519_public_key(
    const uint8_t private_key[32],
    uint8_t public_key[32]);
```
**Returns**: `XY_ED25519_SUCCESS` on success

#### Sign Message
```c
int xy_ed25519_sign(
    uint8_t signature[64],
    const uint8_t *message,
    size_t message_len,
    const uint8_t public_key[32],
    const uint8_t private_key[32]);
```
**Returns**: `XY_ED25519_SUCCESS` on success

#### Verify Signature
```c
int xy_ed25519_verify(
    const uint8_t signature[64],
    const uint8_t *message,
    size_t message_len,
    const uint8_t public_key[32]);
```
**Returns**: `XY_ED25519_SUCCESS` if valid

#### Sign (Simple Interface)
```c
int xy_ed25519_sign_simple(
    uint8_t signature[64],
    const uint8_t *message,
    size_t message_len,
    const uint8_t private_key[32]);
```
**Returns**: `XY_ED25519_SUCCESS` on success

## Error Codes

### X25519 Errors
```c
#define XY_X25519_SUCCESS             0
#define XY_X25519_ERROR_INVALID_PARAM -1
#define XY_X25519_ERROR_WEAK_KEY      -2
#define XY_X25519_ERROR               -3
```

### Ed25519 Errors
```c
#define XY_ED25519_SUCCESS                0
#define XY_ED25519_ERROR_INVALID_PARAM   -1
#define XY_ED25519_ERROR_VERIFY_FAILED   -2
#define XY_ED25519_ERROR_INVALID_SIGNATURE -3
#define XY_ED25519_ERROR                 -4
```

## Usage Examples

### X25519 Key Exchange
```c
#include "xy_25519.h"

void secure_channel_setup(void) {
    uint8_t alice_priv[32], alice_pub[32];
    uint8_t bob_priv[32], bob_pub[32];
    uint8_t shared_a[32], shared_b[32];

    // Generate key pairs
    xy_x25519_generate_keypair(alice_priv, alice_pub);
    xy_x25519_generate_keypair(bob_priv, bob_pub);

    // Compute shared secrets
    xy_x25519_shared_secret(shared_a, alice_priv, bob_pub);
    xy_x25519_shared_secret(shared_b, bob_priv, alice_pub);

    // shared_a == shared_b (key agreement established)
}
```

### Ed25519 Digital Signatures
```c
#include "xy_25519.h"

void message_authentication(void) {
    uint8_t private_key[32], public_key[32];
    uint8_t signature[64];
    uint8_t message[] = "Hello, World!";
    int result;

    // Generate key pair
    xy_ed25519_generate_keypair(public_key, private_key);

    // Sign message
    xy_ed25519_sign(signature, message, sizeof(message) - 1,
                    public_key, private_key);

    // Verify signature
    result = xy_ed25519_verify(signature, message,
                               sizeof(message) - 1, public_key);

    if (result == XY_ED25519_SUCCESS) {
        // Signature is valid
    }
}
```

## Performance Characteristics

### Memory Usage
- **Stack**: < 1KB per operation (no dynamic allocation)
- **ROM**: ~8KB for both algorithms
- **RAM**: 32-byte arrays for keys

### Execution Time (Estimated)

| Platform | X25519 | Ed25519 Sign | Ed25519 Verify |
|----------|--------|--------------|----------------|
| PC (x64, 3.2GHz) | <1ms | <2ms | <3ms |
| Cortex-M4 (168MHz) | ~15ms | ~25ms | ~35ms |
| Cortex-M0+ (48MHz) | ~60ms | ~100ms | ~140ms |

## Platform Support

### Supported Architectures
- ✅ **x86/x64** (PC)
- ✅ **ARM Cortex-M0/M0+**
- ✅ **ARM Cortex-M3/M4/M7**
- ✅ **Other 32-bit architectures**

### Compiler Support
- ✅ **GCC** (Linux/Windows/RISC-V)
- ✅ **ARM GCC** (Embedded)
- ✅ **Clang**

### Requirements
- **C Standard**: C99 or later
- **Build Flags**: `-std=c99 -O2`
- **Dependencies**: xy_clib (memcpy, memset, memcmp)
- **Optional**: xy_log (for logging), xy_random (for key generation)

## Building

### Compilation
```bash
cd xy_25519
make library    # Create libxy_25519.a
make clean      # Clean generated files
```

### Integration
Include in your project:
```c
#include "xy_25519.h"

// Enable/disable in xy_crypto_config.h
#define XY_CRYPTO_ENABLE_CURVE25519 1
```

## Security Properties

### Constant-Time Operations
- Field arithmetic avoids secret-dependent branching
- Montgomery ladder prevents branch prediction attacks
- Conditional swaps use time-invariant patterns

### Key Management
- Private keys never copied unnecessarily
- Output arrays passed by reference (no leakage)
- No key material in debug output

### Cryptographic Standards
- **X25519**: RFC 7748 (Montgomery ECDH)
- **Ed25519**: RFC 8032 (EdDSA on Curve25519)
- **Field**: Prime field modulo 2^255-19
- **Security**: 128-bit security strength

## Testing

### Recommended Test Cases
1. **Key Generation**: Verify random key pair generation
2. **ECDH Agreement**: Confirm shared secret consistency
3. **Signature Verification**: Valid signature acceptance
4. **Invalid Signatures**: Rejection of forged signatures
5. **Edge Cases**: Low-order point handling

## Future Enhancements

### Potential Optimizations
- [ ] Hardware acceleration hooks (AES-NI, NEON)
- [ ] Precomputed point tables for Ed25519
- [ ] Batch signature verification
- [ ] Fixed-point arithmetic for M0+ platforms

### Additional Features
- [ ] X448 (larger variant of X25519)
- [ ] Ed448 (larger variant of Ed25519)
- [ ] Hash-based signatures (SPHINCS)

## License

Part of the XinYi embedded framework. Follow project licensing guidelines.

## References

- **RFC 7748**: Elliptic Curves for Security
  <https://tools.ietf.org/html/rfc7748>

- **RFC 8032**: Edwards-Curve Digital Signature Algorithm (EdDSA)
  <https://tools.ietf.org/html/rfc8032>

- **Bernstein et al.**: Twisted Edwards Curves
  <https://hyperelliptic.org/EFD/g1p/auto-twisted-extended.html>

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0.0 | 2025-11-02 | Initial release - Unified X25519 + Ed25519 implementation |

---

**Status**: Production Ready
**Maintainers**: XinYi Crypto Team
**Last Updated**: 2025-11-02
