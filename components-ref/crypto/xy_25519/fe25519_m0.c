/**
 * @file fe25519_m0.c
 * @brief Cortex-M0 optimized field arithmetic implementation
 * @version 1.0.0
 * @date 2025-11-02
 *
 * Based on curve25519-cortexm0 by Haase & Schwabe with optimizations:
 * - 8x32-bit packed representation
 * - On-the-fly reduction (add/sub process MSW first)
 * - Assembly multiply/square/reduce for critical path
 * - Constant-time operations for side-channel resistance
 */

#include "fe25519_m0.h"
#include <stdint.h>

/* ==================== Basic Operations ==================== */

void fe25519_setzero_m0(fe25519_m0 *out) {
    for (int i = 0; i < 8; i++) {
        out->limbs[i] = 0;
    }
}

void fe25519_setone_m0(fe25519_m0 *out) {
    out->limbs[0] = 1;
    for (int i = 1; i < 8; i++) {
        out->limbs[i] = 0;
    }
}

void fe25519_copy_m0(fe25519_m0 *dest, const fe25519_m0 *src) {
    for (int i = 0; i < 8; i++) {
        dest->limbs[i] = src->limbs[i];
    }
}

/* ==================== Pack/Unpack ==================== */

void fe25519_unpack_m0(fe25519_m0 *out, const uint8_t in[32]) {
    // Load little-endian 32-bit words
    for (int i = 0; i < 8; i++) {
        out->limbs[i] = ((uint32_t)in[i*4 + 0])
                      | ((uint32_t)in[i*4 + 1] << 8)
                      | ((uint32_t)in[i*4 + 2] << 16)
                      | ((uint32_t)in[i*4 + 3] << 24);
    }
    // Clear bit 255 to ensure value < 2^255
    out->limbs[7] &= 0x7FFFFFFF;
}

void fe25519_pack_m0(uint8_t out[32], const fe25519_m0 *in) {
    fe25519_m0 temp;

    // Make a copy and reduce completely
    fe25519_copy_m0(&temp, in);
    fe25519_reduce_completely_m0(&temp);

    // Store little-endian 32-bit words
    for (int i = 0; i < 8; i++) {
        out[i*4 + 0] = (uint8_t)(temp.limbs[i]);
        out[i*4 + 1] = (uint8_t)(temp.limbs[i] >> 8);
        out[i*4 + 2] = (uint8_t)(temp.limbs[i] >> 16);
        out[i*4 + 3] = (uint8_t)(temp.limbs[i] >> 24);
    }
}

/* ==================== Addition with On-the-Fly Reduction ==================== */

void fe25519_add_m0(fe25519_m0 *out, const fe25519_m0 *a, const fe25519_m0 *b) {
    uint64_t accu = 0;

    // Process MSW first for on-the-fly reduction
    // This allows immediate reduction of any overflow
    accu = (uint64_t)a->limbs[7] + b->limbs[7];
    out->limbs[7] = ((uint32_t)accu) & 0x7FFFFFFF;  // Keep only 31 bits

    // Propagate carry with 19x multiplier (from 2^255-19 modulus)
    // If bit 31 was set, we add 19 to compensate
    accu = ((uint32_t)(accu >> 31)) * 19;

    // Add remaining limbs with carry propagation
    for (int i = 0; i < 7; i++) {
        accu += (uint64_t)a->limbs[i] + b->limbs[i];
        out->limbs[i] = (uint32_t)accu;
        accu >>= 32;
    }

    // Final carry back to MSW
    accu += out->limbs[7];
    out->limbs[7] = (uint32_t)accu;
}

/* ==================== Subtraction with On-the-Fly Reduction ==================== */

void fe25519_sub_m0(fe25519_m0 *out, const fe25519_m0 *a, const fe25519_m0 *b) {
    int64_t accu = 0;

    // Process MSW first
    accu = (int64_t)a->limbs[7] - b->limbs[7];

    // Always set bit 31, compensate by subtracting 1 from reduction value
    // This ensures result is always positive
    out->limbs[7] = ((uint32_t)accu) | 0x80000000;

    // Propagate borrow with 19x multiplier
    // -1 compensates for the OR 0x80000000 above
    accu = 19 * ((int32_t)(accu >> 31) - 1);

    // Subtract remaining limbs with borrow propagation
    for (int i = 0; i < 7; i++) {
        accu += (int64_t)a->limbs[i] - b->limbs[i];
        out->limbs[i] = (uint32_t)accu;
        accu >>= 32;
    }

    // Final borrow back to MSW
    accu += out->limbs[7];
    out->limbs[7] = (uint32_t)accu;
}

/* ==================== Multiplication (using assembly) ==================== */

void fe25519_mul_m0(fe25519_m0 *out, const fe25519_m0 *a, const fe25519_m0 *b) {
    uint32_t tmp[16];

    // Call assembly-optimized 256x256 multiply (~400 cycles)
    multiply256x256_asm(tmp, a->limbs, b->limbs);

    // Call assembly-optimized modular reduction (~180 cycles)
    fe25519_reduceTo256Bits_asm(out->limbs, tmp);
}

/* ==================== Squaring (using assembly) ==================== */

void fe25519_square_m0(fe25519_m0 *out, const fe25519_m0 *a) {
    uint32_t tmp[16];

    // Call assembly-optimized squaring (~300 cycles)
    square256_asm(tmp, a->limbs);

    // Call assembly-optimized modular reduction (~180 cycles)
    fe25519_reduceTo256Bits_asm(out->limbs, tmp);
}

/* ==================== Complete Reduction ==================== */

void fe25519_reduce_completely_m0(fe25519_m0 *inout) {
    uint32_t num_reductions;
    uint32_t initial_guess = inout->limbs[7] >> 31;
    uint64_t accu;

    // Initial guess: how many times do we need to subtract prime?
    // Based on bit 31 of MSW
    // Add one extra 19 to ensure correct number of reductions
    accu = (uint64_t)initial_guess * 19 + 19;

    // Propagate through all limbs (without writing back)
    for (int i = 0; i < 7; i++) {
        accu += inout->limbs[i];
        accu >>= 32;
    }
    accu += inout->limbs[7];

    // Actual number of reductions needed
    num_reductions = (uint32_t)(accu >> 31);

    // Perform the reduction
    accu = (uint64_t)num_reductions * 19;

    for (int i = 0; i < 7; i++) {
        accu += inout->limbs[i];
        inout->limbs[i] = (uint32_t)accu;
        accu >>= 32;
    }
    accu += inout->limbs[7];
    inout->limbs[7] = (uint32_t)accu & 0x7FFFFFFF;
}

/* ==================== Constant-Time Conditional Swap ==================== */

void fe25519_cswap_m0(fe25519_m0 *a, fe25519_m0 *b, int condition) {
    // Create constant-time mask from condition
    // If condition != 0, mask = 0xFFFFFFFF
    // If condition == 0, mask = 0x00000000
    int32_t mask = (int32_t)condition;
    mask = -mask;  // Replicate sign bit to all bits

    for (int i = 0; i < 8; i++) {
        uint32_t val1 = a->limbs[i];
        uint32_t val2 = b->limbs[i];
        uint32_t temp = val1;

        // Constant-time swap using XOR trick
        val1 ^= (uint32_t)mask & (val2 ^ val1);
        val2 ^= (uint32_t)mask & (val2 ^ temp);

        a->limbs[i] = val1;
        b->limbs[i] = val2;
    }
}

/* ==================== Inversion using Fermat's Little Theorem ==================== */

void fe25519_invert_m0(fe25519_m0 *out, const fe25519_m0 *in) {
    // Compute out = in^(p-2) mod p where p = 2^255-19
    // Uses addition chain optimized for 2^255-19
    // Based on curve25519-donna implementation

    fe25519_m0 z2, z9, z11, z2_5_0, z2_10_0, z2_20_0, z2_50_0, z2_100_0;
    fe25519_m0 t0, t1;

    // z2 = in^2
    fe25519_square_m0(&z2, in);

    // z9 = z2^(2^2) * in = in^9
    fe25519_square_m0(&t0, &z2);
    fe25519_square_m0(&t0, &t0);
    fe25519_mul_m0(&z9, &t0, in);

    // z11 = z9 * z2 = in^11
    fe25519_mul_m0(&z11, &z9, &z2);

    // z2_5_0 = z11^(2^5) * z11 = in^(2^5-1)
    fe25519_square_m0(&t0, &z11);
    fe25519_mul_m0(&z2_5_0, &t0, &z9);  // Note: original uses z2_10_0 temp

    // z2_10_0 = z2_5_0^(2^5) * z2_5_0 = in^(2^10-1)
    fe25519_square_m0(&t0, &z2_5_0);
    for (int i = 1; i < 5; i++) {
        fe25519_square_m0(&t0, &t0);
    }
    fe25519_mul_m0(&z2_10_0, &t0, &z2_5_0);

    // z2_20_0 = z2_10_0^(2^10) * z2_10_0 = in^(2^20-1)
    fe25519_square_m0(&t0, &z2_10_0);
    for (int i = 1; i < 10; i++) {
        fe25519_square_m0(&t0, &t0);
    }
    fe25519_mul_m0(&z2_20_0, &t0, &z2_10_0);

    // z2_50_0 = z2_20_0^(2^20) * z2_20_0^(2^10) * z2_10_0 = in^(2^50-1)
    fe25519_square_m0(&t0, &z2_20_0);
    for (int i = 1; i < 20; i++) {
        fe25519_square_m0(&t0, &t0);
    }
    fe25519_mul_m0(&t0, &t0, &z2_20_0);
    fe25519_square_m0(&t0, &t0);
    for (int i = 1; i < 10; i++) {
        fe25519_square_m0(&t0, &t0);
    }
    fe25519_mul_m0(&z2_50_0, &t0, &z2_10_0);

    // z2_100_0 = z2_50_0^(2^50) * z2_50_0 = in^(2^100-1)
    fe25519_square_m0(&t0, &z2_50_0);
    for (int i = 1; i < 50; i++) {
        fe25519_square_m0(&t0, &t0);
    }
    fe25519_mul_m0(&z2_100_0, &t0, &z2_50_0);

    // t0 = z2_100_0^(2^100) * z2_100_0 = in^(2^200-1)
    fe25519_square_m0(&t0, &z2_100_0);
    for (int i = 1; i < 100; i++) {
        fe25519_square_m0(&t0, &t0);
    }
    fe25519_mul_m0(&t0, &t0, &z2_100_0);

    // t0 = t0^(2^50) * z2_50_0 = in^(2^250-1)
    for (int i = 0; i < 50; i++) {
        fe25519_square_m0(&t0, &t0);
    }
    fe25519_mul_m0(&t0, &t0, &z2_50_0);

    // out = t0^(2^5) * z11 = in^(2^255-21) = in^(p-2)
    for (int i = 0; i < 5; i++) {
        fe25519_square_m0(&t0, &t0);
    }
    fe25519_mul_m0(out, &t0, &z11);
}

/* ==================== Equality Check ==================== */

int fe25519_iseq_m0(const fe25519_m0 *a, const fe25519_m0 *b) {
    fe25519_m0 temp_a, temp_b;
    uint32_t diff = 0;

    // Reduce both completely
    fe25519_copy_m0(&temp_a, a);
    fe25519_copy_m0(&temp_b, b);
    fe25519_reduce_completely_m0(&temp_a);
    fe25519_reduce_completely_m0(&temp_b);

    // Constant-time comparison
    for (int i = 0; i < 8; i++) {
        diff |= temp_a.limbs[i] ^ temp_b.limbs[i];
    }

    // Return 1 if equal, 0 if different
    return (diff == 0) ? 1 : 0;
}

int fe25519_iszero_m0(const fe25519_m0 *a) {
    fe25519_m0 zero;
    fe25519_setzero_m0(&zero);
    return fe25519_iseq_m0(a, &zero);
}
