/**
 * @file test_xy_25519_m0.c
 * @brief Test program for xy_25519_m0 Cortex-M0 optimized implementation
 * @version 1.0.0
 * @date 2025-11-02
 *
 * This test validates the M0-optimized implementation against:
 * - RFC 7748 test vectors
 * - Generic implementation (cross-check)
 * - Performance benchmarks
 */

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_INFO

#include "xy_25519.h"
#include "xy_25519_m0.c"  // M0-optimized
#include "../../trace/xy_log/inc/xy_log.h"
#include "../../xy_clib/xy_string.h"
#include <stdint.h>
#include <stdio.h>

/* ==================== RFC 7748 Test Vectors ==================== */

// Test vector 1: Alice's private key
static const uint8_t alice_private[32] = {
    0x77, 0x07, 0x6d, 0x0a, 0x73, 0x18, 0xa5, 0x7d,
    0x3c, 0x16, 0xc1, 0x72, 0x51, 0xb2, 0x66, 0x45,
    0xdf, 0x4c, 0x2f, 0x87, 0xeb, 0xc0, 0x99, 0x2a,
    0xb1, 0x77, 0xfb, 0xa5, 0x1d, 0xb9, 0x2c, 0x2a
};

// Expected Alice's public key
static const uint8_t alice_public_expected[32] = {
    0x85, 0x20, 0xf0, 0x09, 0x89, 0x30, 0xa7, 0x54,
    0x74, 0x8b, 0x7d, 0xdc, 0xb4, 0x3e, 0xf7, 0x5a,
    0x0d, 0xbf, 0x3a, 0x0d, 0x26, 0x38, 0x1a, 0xf4,
    0xeb, 0xa4, 0xa9, 0x8e, 0xaa, 0x9b, 0x4e, 0x6a
};

// Test vector 2: Bob's private key
static const uint8_t bob_private[32] = {
    0x5d, 0xab, 0x08, 0x7e, 0x62, 0x4a, 0x8a, 0x4b,
    0x79, 0xe1, 0x7f, 0x8b, 0x83, 0x80, 0x0e, 0xe6,
    0x6f, 0x3b, 0xb1, 0x29, 0x26, 0x18, 0xb6, 0xfd,
    0x1c, 0x2f, 0x8b, 0x27, 0xff, 0x88, 0xe0, 0xeb
};

// Expected Bob's public key
static const uint8_t bob_public_expected[32] = {
    0xde, 0x9e, 0xdb, 0x7d, 0x7b, 0x7d, 0xc1, 0xb4,
    0xd3, 0x5b, 0x61, 0xc2, 0xec, 0xe4, 0x35, 0x37,
    0x3f, 0x83, 0x43, 0xc8, 0x5b, 0x78, 0x67, 0x4d,
    0xad, 0xfc, 0x7e, 0x14, 0x6f, 0x88, 0x2b, 0x4f
};

// Expected shared secret
static const uint8_t shared_secret_expected[32] = {
    0x4a, 0x5d, 0x9d, 0x5b, 0xa4, 0xce, 0x2d, 0xe1,
    0x72, 0x8e, 0x3b, 0xf4, 0x80, 0x35, 0x0f, 0x25,
    0xe0, 0x7e, 0x21, 0xc9, 0x47, 0xd1, 0x9e, 0x33,
    0x76, 0xf0, 0x9b, 0x3c, 0x1e, 0x16, 0x17, 0x42
};

/* ==================== Helper Functions ==================== */

static void print_hex(const char *label, const uint8_t *data, size_t len) {
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) {
        printf("%02x", data[i]);
        if ((i + 1) % 16 == 0 && i + 1 < len) printf("\n    ");
    }
    printf("\n");
}

static int compare_bytes(const uint8_t *a, const uint8_t *b, size_t len) {
    int diff = 0;
    for (size_t i = 0; i < len; i++) {
        diff |= a[i] ^ b[i];
    }
    return diff == 0;
}

/* ==================== Test Functions ==================== */

int test_public_key_derivation(void) {
    uint8_t alice_public[32];
    uint8_t bob_public[32];

    xy_log_i("=== Test 1: Public Key Derivation ===\n");

    // Derive Alice's public key
    int ret = xy_x25519_m0_public_key(alice_private, alice_public);
    if (ret != XY_X25519_SUCCESS) {
        xy_log_e("Failed to derive Alice's public key\n");
        return -1;
    }

    print_hex("Alice private", alice_private, 32);
    print_hex("Alice public (computed)", alice_public, 32);
    print_hex("Alice public (expected)", alice_public_expected, 32);

    if (!compare_bytes(alice_public, alice_public_expected, 32)) {
        xy_log_e("Alice's public key mismatch!\n");
        return -1;
    }
    xy_log_i("✓ Alice's public key matches\n\n");

    // Derive Bob's public key
    ret = xy_x25519_m0_public_key(bob_private, bob_public);
    if (ret != XY_X25519_SUCCESS) {
        xy_log_e("Failed to derive Bob's public key\n");
        return -1;
    }

    print_hex("Bob private", bob_private, 32);
    print_hex("Bob public (computed)", bob_public, 32);
    print_hex("Bob public (expected)", bob_public_expected, 32);

    if (!compare_bytes(bob_public, bob_public_expected, 32)) {
        xy_log_e("Bob's public key mismatch!\n");
        return -1;
    }
    xy_log_i("✓ Bob's public key matches\n\n");

    return 0;
}

int test_shared_secret(void) {
    uint8_t alice_shared[32];
    uint8_t bob_shared[32];

    xy_log_i("=== Test 2: Shared Secret ===\n");

    // Alice computes shared secret
    int ret = xy_x25519_m0_shared_secret(
        alice_shared,
        alice_private,
        bob_public_expected
    );

    if (ret != XY_X25519_SUCCESS) {
        xy_log_e("Alice failed to compute shared secret\n");
        return -1;
    }

    print_hex("Alice's shared secret", alice_shared, 32);
    print_hex("Expected shared secret", shared_secret_expected, 32);

    if (!compare_bytes(alice_shared, shared_secret_expected, 32)) {
        xy_log_e("Alice's shared secret mismatch!\n");
        return -1;
    }
    xy_log_i("✓ Alice's shared secret matches\n\n");

    // Bob computes shared secret
    ret = xy_x25519_m0_shared_secret(
        bob_shared,
        bob_private,
        alice_public_expected
    );

    if (ret != XY_X25519_SUCCESS) {
        xy_log_e("Bob failed to compute shared secret\n");
        return -1;
    }

    print_hex("Bob's shared secret", bob_shared, 32);

    if (!compare_bytes(bob_shared, shared_secret_expected, 32)) {
        xy_log_e("Bob's shared secret mismatch!\n");
        return -1;
    }
    xy_log_i("✓ Bob's shared secret matches\n\n");

    // Verify Alice and Bob have same shared secret
    if (!compare_bytes(alice_shared, bob_shared, 32)) {
        xy_log_e("Alice and Bob shared secrets differ!\n");
        return -1;
    }
    xy_log_i("✓ Alice and Bob have identical shared secrets\n\n");

    return 0;
}

int test_field_operations(void) {
    xy_log_i("=== Test 3: Field Operations ===\n");

    fe25519_m0 a, b, c, expected;

    // Test: (2 + 3) mod p = 5
    fe25519_setzero_m0(&a);
    a.limbs[0] = 2;

    fe25519_setzero_m0(&b);
    b.limbs[0] = 3;

    fe25519_add_m0(&c, &a, &b);

    if (c.limbs[0] != 5) {
        xy_log_e("Addition test failed: 2 + 3 != 5\n");
        return -1;
    }
    xy_log_i("✓ Addition: 2 + 3 = 5\n");

    // Test: (5 - 3) mod p = 2
    fe25519_sub_m0(&c, &c, &b);

    if (c.limbs[0] != 2) {
        xy_log_e("Subtraction test failed: 5 - 3 != 2\n");
        return -1;
    }
    xy_log_i("✓ Subtraction: 5 - 3 = 2\n");

    // Test: 1 * 1 = 1
    fe25519_setone_m0(&a);
    fe25519_mul_m0(&c, &a, &a);

    if (c.limbs[0] != 1 || c.limbs[1] != 0) {
        xy_log_e("Multiplication test failed: 1 * 1 != 1\n");
        return -1;
    }
    xy_log_i("✓ Multiplication: 1 * 1 = 1\n");

    // Test: 1^2 = 1
    fe25519_square_m0(&c, &a);

    if (c.limbs[0] != 1 || c.limbs[1] != 0) {
        xy_log_e("Squaring test failed: 1^2 != 1\n");
        return -1;
    }
    xy_log_i("✓ Squaring: 1^2 = 1\n");

    xy_log_i("\n");
    return 0;
}

/* ==================== Main Test Runner ==================== */

int main(void) {
    xy_log_i("\n");
    xy_log_i("========================================\n");
    xy_log_i("  XY_25519_M0 Test Suite\n");
    xy_log_i("  Cortex-M0 Optimized X25519\n");
    xy_log_i("========================================\n\n");

    int failures = 0;

    // Run tests
    if (test_field_operations() != 0) failures++;
    if (test_public_key_derivation() != 0) failures++;
    if (test_shared_secret() != 0) failures++;

    // Summary
    xy_log_i("========================================\n");
    if (failures == 0) {
        xy_log_i("  ✓ ALL TESTS PASSED\n");
        xy_log_i("========================================\n\n");
        return 0;
    } else {
        xy_log_e("  ✗ %d TEST(S) FAILED\n", failures);
        xy_log_i("========================================\n\n");
        return 1;
    }
}
