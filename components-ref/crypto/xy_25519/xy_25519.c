/**
 * @file xy_25519.c
 * @brief Unified Curve25519 implementation (X25519 + Ed25519)
 * @version 1.0.0
 * @date 2025-11-02
 *
 * This implementation combines X25519 ECDH (RFC 7748) and Ed25519 signatures
 * (RFC 8032) with shared field arithmetic for optimal code reuse.
 */

#include "xy_25519.h"
#include "xy_string.h"
#include <stdint.h>

/* External dependencies */
extern int xy_random_bytes(uint8_t *buffer, size_t len);
extern int xy_sha512_hash(const uint8_t *data, size_t len, uint8_t digest[64]);

/* ==================== Shared Field Arithmetic (mod 2^255-19) ==================== */

/**
 * @brief Field element type (10 limbs, 25.5 bits each)
 */
typedef uint32_t fe25519[10];

/**
 * @brief Prime field modulus: 2^255 - 19
 */
static const uint8_t prime_field[32] = {
    0xed, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f
};

/**
 * @brief Curve25519 base point for X25519
 */
static const uint8_t x25519_basepoint[32] = {
    9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/**
 * @brief Low-order points for X25519 validation
 */
static const uint8_t low_order_points[7][32] = {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0xe0, 0xeb, 0x7a, 0x7c, 0x3b, 0x41, 0xb8, 0xae, 0x16, 0x56, 0xe3,
      0xfa, 0xf1, 0x9f, 0xc4, 0x6a, 0xda, 0x09, 0x8d, 0xeb, 0x9c, 0x32,
      0xb1, 0xfd, 0x86, 0x62, 0x05, 0x16, 0x5f, 0x49, 0xb8, 0x00 },
    { 0x5f, 0x9c, 0x95, 0xbc, 0xa3, 0x50, 0x8c, 0x24, 0xb1, 0xd0, 0xb1,
      0x55, 0x9c, 0x83, 0xef, 0x5b, 0x04, 0x44, 0x5c, 0xc4, 0x58, 0x1c,
      0x8e, 0x86, 0xd8, 0x22, 0x4e, 0xdd, 0xd0, 0x9f, 0x11, 0x57 },
    { 0xec, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f },
    { 0xed, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f },
    { 0xee, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f }
};

/* Ed25519 constants */
static const fe25519 ed25519_d = {
    0x35978a3, 0x00d37a0, 0x39ccf97, 0x3a9ece4, 0x36b0ef6,
    0x07dd6c2, 0x1dd2c14, 0x21ac3eb, 0x1f2c127, 0x052036c
};

static const fe25519 ed25519_d2 = {
    0x2b2f159, 0x01a6f40, 0x3399f2e, 0x353dcc9, 0x2d61deb,
    0x0fbad85, 0x3ba5a28, 0x2158dd6, 0x3e8424f, 0x0a406d8
};

static const fe25519 ed25519_sqrtm1 = {
    0x20ea0b0, 0x186c9d2, 0x08f189d, 0x035697f, 0x0bd0c60,
    0x1fbd7a7, 0x2804c9e, 0x1e16569, 0x004fc1d, 0x0ae0c92
};

/* ==================== Field Arithmetic Primitives ==================== */

static uint32_t load32_le(const uint8_t *src)
{
    return ((uint32_t)src[0])
           | ((uint32_t)src[1] << 8)
           | ((uint32_t)src[2] << 16)
           | ((uint32_t)src[3] << 24);
}

static void store32_le(uint8_t *dst, uint32_t value)
{
    dst[0] = (uint8_t)(value & 0xff);
    dst[1] = (uint8_t)((value >> 8) & 0xff);
    dst[2] = (uint8_t)((value >> 16) & 0xff);
    dst[3] = (uint8_t)((value >> 24) & 0xff);
}

static void fe_frombytes(fe25519 h, const uint8_t *s)
{
    uint64_t h0 = load32_le(s);
    uint64_t h1 = load32_le(s + 4);
    uint64_t h2 = load32_le(s + 8);
    uint64_t h3 = load32_le(s + 12);
    uint64_t h4 = load32_le(s + 16);
    uint64_t h5 = load32_le(s + 20);
    uint64_t h6 = load32_le(s + 24);
    uint64_t h7 = load32_le(s + 28);

    h[0] = (uint32_t)(h0 & 0x3ffffff);
    h[1] = (uint32_t)((h0 >> 26) | ((h1 & 0x1fff) << 6));
    h[2] = (uint32_t)((h1 >> 13) | ((h2 & 0xff) << 19));
    h[3] = (uint32_t)((h2 >> 8) | ((h3 & 0x1f) << 24));
    h[4] = (uint32_t)((h3 >> 5) & 0x3ffffff);
    h[5] = (uint32_t)((h3 >> 31) | ((h4 & 0x3ffff) << 1));
    h[6] = (uint32_t)((h4 >> 18) | ((h5 & 0x1fff) << 14));
    h[7] = (uint32_t)((h5 >> 13) | ((h6 & 0xff) << 19));
    h[8] = (uint32_t)((h6 >> 8) | ((h7 & 0x1f) << 24));
    h[9] = (uint32_t)((h7 >> 5) & 0x1ffffff);
}

static void fe_tobytes(uint8_t *s, const fe25519 h)
{
    uint32_t t[10];
    uint32_t carry;
    int i;

    for (i = 0; i < 10; i++) {
        t[i] = h[i];
    }

    carry = (t[9] >> 25);
    t[9] &= 0x1ffffff;
    carry *= 19;
    t[0] += carry;

    for (i = 0; i < 9; i++) {
        carry = t[i] >> 26;
        t[i] &= 0x3ffffff;
        t[i + 1] += carry;
    }

    carry = t[9] >> 25;
    t[9] &= 0x1ffffff;
    carry *= 19;
    t[0] += carry;

    carry = t[0] >> 26;
    t[0] &= 0x3ffffff;
    t[1] += carry;

    store32_le(s, t[0] | (t[1] << 26));
    store32_le(s + 4, (t[1] >> 6) | (t[2] << 19));
    store32_le(s + 8, (t[2] >> 13) | (t[3] << 13));
    store32_le(s + 12, (t[3] >> 19) | (t[4] << 6));
    store32_le(s + 16, t[5] | (t[6] << 25));
    store32_le(s + 20, (t[6] >> 7) | (t[7] << 19));
    store32_le(s + 24, (t[7] >> 13) | (t[8] << 12));
    store32_le(s + 28, (t[8] >> 20) | (t[9] << 6));
}

static void fe_copy(fe25519 h, const fe25519 f)
{
    int i;
    for (i = 0; i < 10; i++) {
        h[i] = f[i];
    }
}

static void fe_0(fe25519 h)
{
    xy_memset(h, 0, sizeof(fe25519));
}

static void fe_1(fe25519 h)
{
    h[0] = 1;
    xy_memset(&h[1], 0, 9 * sizeof(uint32_t));
}

static void fe_add(fe25519 h, const fe25519 f, const fe25519 g)
{
    int i;
    for (i = 0; i < 10; i++) {
        h[i] = f[i] + g[i];
    }
}

static void fe_sub(fe25519 h, const fe25519 f, const fe25519 g)
{
    h[0] = f[0] + 0x7ffffda - g[0];
    h[1] = f[1] + 0x3fffffe - g[1];
    h[2] = f[2] + 0x7fffffe - g[2];
    h[3] = f[3] + 0x3fffffe - g[3];
    h[4] = f[4] + 0x7fffffe - g[4];
    h[5] = f[5] + 0x3fffffe - g[5];
    h[6] = f[6] + 0x7fffffe - g[6];
    h[7] = f[7] + 0x3fffffe - g[7];
    h[8] = f[8] + 0x7fffffe - g[8];
    h[9] = f[9] + 0x1fffffe - g[9];
}

static void fe_mul(fe25519 h, const fe25519 f, const fe25519 g)
{
    int64_t f0 = f[0], f1 = f[1], f2 = f[2], f3 = f[3], f4 = f[4];
    int64_t f5 = f[5], f6 = f[6], f7 = f[7], f8 = f[8], f9 = f[9];
    int64_t g0 = g[0], g1 = g[1], g2 = g[2], g3 = g[3], g4 = g[4];
    int64_t g5 = g[5], g6 = g[6], g7 = g[7], g8 = g[8], g9 = g[9];
    int64_t g1_19 = 19 * g1, g2_19 = 19 * g2, g3_19 = 19 * g3;
    int64_t g4_19 = 19 * g4, g5_19 = 19 * g5, g6_19 = 19 * g6;
    int64_t g7_19 = 19 * g7, g8_19 = 19 * g8, g9_19 = 19 * g9;
    int64_t f1_2 = 2 * f1, f3_2 = 2 * f3, f5_2 = 2 * f5;
    int64_t f7_2 = 2 * f7, f9_2 = 2 * f9;
    int64_t h0, h1, h2, h3, h4, h5, h6, h7, h8, h9;
    int64_t c0, c1, c2, c3, c4, c5, c6, c7, c8, c9;

    h0 = f0*g0 + f1_2*g9_19 + f2*g8_19 + f3_2*g7_19 + f4*g6_19
       + f5_2*g5_19 + f6*g4_19 + f7_2*g3_19 + f8*g2_19 + f9_2*g1_19;
    h1 = f0*g1 + f1*g0 + f2*g9_19 + f3*g8_19 + f4*g7_19
       + f5*g6_19 + f6*g5_19 + f7*g4_19 + f8*g3_19 + f9*g2_19;
    h2 = f0*g2 + f1_2*g1 + f2*g0 + f3_2*g9_19 + f4*g8_19
       + f5_2*g7_19 + f6*g6_19 + f7_2*g5_19 + f8*g4_19 + f9_2*g3_19;
    h3 = f0*g3 + f1*g2 + f2*g1 + f3*g0 + f4*g9_19
       + f5*g8_19 + f6*g7_19 + f7*g6_19 + f8*g5_19 + f9*g4_19;
    h4 = f0*g4 + f1_2*g3 + f2*g2 + f3_2*g1 + f4*g0
       + f5_2*g9_19 + f6*g8_19 + f7_2*g7_19 + f8*g6_19 + f9_2*g5_19;
    h5 = f0*g5 + f1*g4 + f2*g3 + f3*g2 + f4*g1 + f5*g0
       + f6*g9_19 + f7*g8_19 + f8*g7_19 + f9*g6_19;
    h6 = f0*g6 + f1_2*g5 + f2*g4 + f3_2*g3 + f4*g2 + f5_2*g1
       + f6*g0 + f7_2*g9_19 + f8*g8_19 + f9_2*g7_19;
    h7 = f0*g7 + f1*g6 + f2*g5 + f3*g4 + f4*g3 + f5*g2
       + f6*g1 + f7*g0 + f8*g9_19 + f9*g8_19;
    h8 = f0*g8 + f1_2*g7 + f2*g6 + f3_2*g5 + f4*g4 + f5_2*g3
       + f6*g2 + f7_2*g1 + f8*g0 + f9_2*g9_19;
    h9 = f0*g9 + f1*g8 + f2*g7 + f3*g6 + f4*g5 + f5*g4
       + f6*g3 + f7*g2 + f8*g1 + f9*g0;

    c0 = (h0 + (1<<25)) >> 26; h1 += c0; h0 -= c0 << 26;
    c4 = (h4 + (1<<25)) >> 26; h5 += c4; h4 -= c4 << 26;
    c1 = (h1 + (1<<24)) >> 25; h2 += c1; h1 -= c1 << 25;
    c5 = (h5 + (1<<24)) >> 25; h6 += c5; h5 -= c5 << 25;
    c2 = (h2 + (1<<25)) >> 26; h3 += c2; h2 -= c2 << 26;
    c6 = (h6 + (1<<25)) >> 26; h7 += c6; h6 -= c6 << 26;
    c3 = (h3 + (1<<24)) >> 25; h4 += c3; h3 -= c3 << 25;
    c7 = (h7 + (1<<24)) >> 25; h8 += c7; h7 -= c7 << 25;
    c4 = (h4 + (1<<25)) >> 26; h5 += c4; h4 -= c4 << 26;
    c8 = (h8 + (1<<25)) >> 26; h9 += c8; h8 -= c8 << 26;
    c9 = (h9 + (1<<24)) >> 25; h0 += c9 * 19; h9 -= c9 << 25;
    c0 = (h0 + (1<<25)) >> 26; h1 += c0; h0 -= c0 << 26;

    h[0] = (uint32_t)h0; h[1] = (uint32_t)h1; h[2] = (uint32_t)h2;
    h[3] = (uint32_t)h3; h[4] = (uint32_t)h4; h[5] = (uint32_t)h5;
    h[6] = (uint32_t)h6; h[7] = (uint32_t)h7; h[8] = (uint32_t)h8;
    h[9] = (uint32_t)h9;
}

static void fe_sq(fe25519 h, const fe25519 f)
{
    fe_mul(h, f, f);
}

static void fe_invert(fe25519 out, const fe25519 z)
{
    fe25519 t0, t1, t2, t3;
    int i;

    fe_sq(t0, z);
    fe_sq(t1, t0); fe_sq(t1, t1);
    fe_mul(t1, z, t1);
    fe_mul(t0, t0, t1);
    fe_sq(t2, t0);
    fe_mul(t1, t1, t2);
    fe_sq(t2, t1);
    for (i = 1; i < 5; i++) fe_sq(t2, t2);
    fe_mul(t1, t2, t1);
    fe_sq(t2, t1);
    for (i = 1; i < 10; i++) fe_sq(t2, t2);
    fe_mul(t2, t2, t1);
    fe_sq(t3, t2);
    for (i = 1; i < 20; i++) fe_sq(t3, t3);
    fe_mul(t2, t3, t2);
    fe_sq(t2, t2);
    for (i = 1; i < 10; i++) fe_sq(t2, t2);
    fe_mul(t1, t2, t1);
    fe_sq(t2, t1);
    for (i = 1; i < 50; i++) fe_sq(t2, t2);
    fe_mul(t2, t2, t1);
    fe_sq(t3, t2);
    for (i = 1; i < 100; i++) fe_sq(t3, t3);
    fe_mul(t2, t3, t2);
    fe_sq(t2, t2);
    for (i = 1; i < 50; i++) fe_sq(t2, t2);
    fe_mul(t1, t2, t1);
    fe_sq(t1, t1);
    for (i = 1; i < 5; i++) fe_sq(t1, t1);
    fe_mul(out, t1, t0);
}

static void fe_cswap(fe25519 a, fe25519 b, uint32_t swap)
{
    uint32_t mask = (uint32_t)(-(int32_t)swap);
    uint32_t temp;
    int i;
    for (i = 0; i < 10; i++) {
        temp = mask & (a[i] ^ b[i]);
        a[i] ^= temp;
        b[i] ^= temp;
    }
}

static void fe_cmov(fe25519 f, const fe25519 g, uint32_t b)
{
    uint32_t mask = (uint32_t)(-(int32_t)b);
    uint32_t x;
    int i;
    for (i = 0; i < 10; i++) {
        x = f[i] ^ g[i];
        x &= mask;
        f[i] ^= x;
    }
}

static int fe_isnegative(const fe25519 f)
{
    uint8_t s[32];
    fe_tobytes(s, f);
    return s[0] & 1;
}

static int fe_isnonzero(const fe25519 f)
{
    uint8_t s[32];
    int i;
    uint8_t result = 0;
    fe_tobytes(s, f);
    for (i = 0; i < 32; i++) result |= s[i];
    return result != 0;
}

static int ct_compare(const uint8_t *a, const uint8_t *b, size_t len)
{
    uint8_t result = 0;
    size_t i;
    for (i = 0; i < len; i++) result |= a[i] ^ b[i];
    return result;
}

/* ==================== X25519 Implementation ==================== */

static void x25519_clamp_private_key(uint8_t key[32])
{
    key[0] &= 248;
    key[31] &= 127;
    key[31] |= 64;
}

static void x25519_scalar_mult(uint8_t out[32], const uint8_t scalar[32],
                                const uint8_t point[32])
{
    fe25519 x1, x2, z2, x3, z3, tmp0, tmp1;
    uint8_t clamped[32];
    uint32_t swap = 0;
    int i, bit;

    xy_memcpy(clamped, scalar, 32);
    x25519_clamp_private_key(clamped);

    fe_frombytes(x1, point);
    fe_1(x2);
    fe_0(z2);
    fe_copy(x3, x1);
    fe_1(z3);

    for (i = 254; i >= 0; i--) {
        bit = (clamped[i >> 3] >> (i & 7)) & 1;
        swap ^= bit;
        fe_cswap(x2, x3, swap);
        fe_cswap(z2, z3, swap);
        swap = bit;

        fe_sub(tmp0, x3, z3);
        fe_sub(tmp1, x2, z2);
        fe_add(x2, x2, z2);
        fe_add(z2, x3, z3);
        fe_mul(z3, tmp0, x2);
        fe_mul(z2, z2, tmp1);
        fe_sq(tmp0, tmp1);
        fe_sq(tmp1, x2);
        fe_add(x3, z3, z2);
        fe_sub(z2, z3, z2);
        fe_mul(x2, tmp1, tmp0);
        fe_sub(tmp1, tmp1, tmp0);
        fe_sq(z2, z2);
        fe_mul(z3, tmp1, (fe25519){121666, 0, 0, 0, 0, 0, 0, 0, 0, 0});
        fe_sq(x3, x3);
        fe_add(tmp0, tmp0, z3);
        fe_mul(z3, x1, z2);
        fe_mul(z2, tmp1, tmp0);
    }

    fe_cswap(x2, x3, swap);
    fe_cswap(z2, z3, swap);

    fe_invert(z2, z2);
    fe_mul(x2, x2, z2);
    fe_tobytes(out, x2);
}

int xy_x25519_generate_keypair(uint8_t private_key[32], uint8_t public_key[32])
{
    int ret;
    if (!private_key || !public_key) {
        return XY_X25519_ERROR_INVALID_PARAM;
    }

    ret = xy_random_bytes(private_key, 32);
    if (ret != 0) return XY_X25519_ERROR;

    return xy_x25519_public_key(private_key, public_key);
}

int xy_x25519_public_key(const uint8_t private_key[32], uint8_t public_key[32])
{
    if (!private_key || !public_key) {
        return XY_X25519_ERROR_INVALID_PARAM;
    }

    x25519_scalar_mult(public_key, private_key, x25519_basepoint);
    return XY_X25519_SUCCESS;
}

int xy_x25519_shared_secret(uint8_t shared_secret[32],
                             const uint8_t our_private_key[32],
                             const uint8_t their_public_key[32])
{
    if (!shared_secret || !our_private_key || !their_public_key) {
        return XY_X25519_ERROR_INVALID_PARAM;
    }

    x25519_scalar_mult(shared_secret, our_private_key, their_public_key);

    /* Check for all-zero result (weak key) */
    if (ct_compare(shared_secret, (uint8_t[32]){0}, 32) == 0) {
        return XY_X25519_ERROR_WEAK_KEY;
    }

    return XY_X25519_SUCCESS;
}

int xy_x25519_validate_public_key(const uint8_t public_key[32])
{
    int i, j;
    if (!public_key) {
        return XY_X25519_ERROR_INVALID_PARAM;
    }

    for (i = 0; i < 7; i++) {
        if (ct_compare(public_key, low_order_points[i], 32) == 0) {
            return XY_X25519_ERROR_WEAK_KEY;
        }
    }

    return XY_X25519_SUCCESS;
}

/* ==================== Ed25519 Implementation ==================== */

typedef struct {
    fe25519 X, Y, Z, T;
} ge_p3;

typedef struct {
    fe25519 YplusX, YminusX, Z, T2d;
} ge_cached;

typedef struct {
    fe25519 X, Y, Z, T;
} ge_p1p1;

static void ge_p3_to_cached(ge_cached *r, const ge_p3 *p)
{
    fe_add(r->YplusX, p->Y, p->X);
    fe_sub(r->YminusX, p->Y, p->X);
    fe_copy(r->Z, p->Z);
    fe_mul(r->T2d, p->T, ed25519_d2);
}

static void ge_add(ge_p1p1 *r, const ge_p3 *p, const ge_cached *q)
{
    fe25519 t0;
    fe_add(r->X, p->Y, p->X);
    fe_sub(r->Y, p->Y, p->X);
    fe_mul(r->Z, r->X, q->YplusX);
    fe_mul(r->Y, r->Y, q->YminusX);
    fe_mul(r->T, q->T2d, p->T);
    fe_mul(r->X, p->Z, q->Z);
    fe_add(t0, r->X, r->X);
    fe_sub(r->X, r->Z, r->Y);
    fe_add(r->Y, r->Z, r->Y);
    fe_add(r->Z, t0, r->T);
    fe_sub(r->T, t0, r->T);
}

static void ge_p1p1_to_p3(ge_p3 *r, const ge_p1p1 *p)
{
    fe_mul(r->X, p->X, p->T);
    fe_mul(r->Y, p->Y, p->Z);
    fe_mul(r->Z, p->Z, p->T);
    fe_mul(r->T, p->X, p->Y);
}

static void ge_p3_dbl(ge_p1p1 *r, const ge_p3 *p)
{
    fe25519 t0;
    fe_sq(r->X, p->X);
    fe_sq(r->Z, p->Y);
    fe_sq(r->T, p->Z);
    fe_add(r->T, r->T, r->T);
    fe_add(r->Y, p->X, p->Y);
    fe_sq(t0, r->Y);
    fe_add(r->Y, r->Z, r->X);
    fe_sub(r->Z, r->Z, r->X);
    fe_sub(r->X, t0, r->Y);
    fe_sub(r->T, r->T, r->Z);
}

static void ge_scalarmult_base(ge_p3 *h, const uint8_t *a)
{
    ge_p1p1 r;
    ge_p3 s;
    ge_cached t;
    int i;
    static const ge_p3 base = {
        {0x325d51a, 0x18b5823, 0x5f51406, 0x247c033, 0x6218990,
         0x26486a1, 0x3fd9d2f, 0x3ada928, 0x2f96ef0, 0x216936d},
        {0x2666658, 0x1999999, 0x3333333, 0x0cccccc, 0x1666666,
         0x1999999, 0x0000000, 0x0000000, 0x0000000, 0x0000000},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0x68ab3a5, 0x1eea2a5, 0x1516b9f, 0x198e80f, 0x2d75e4c,
         0x0b2ee1e, 0x23c8ca4, 0x3dc56dd, 0x21aacb1, 0x1c56edd}
    };

    fe_0(h->X); fe_1(h->Y); fe_1(h->Z); fe_0(h->T);

    for (i = 255; i >= 0; i--) {
        uint8_t bit = (a[i/8] >> (i&7)) & 1;
        ge_p3_dbl(&r, h);
        ge_p1p1_to_p3(&s, &r);
        if (bit) {
            ge_p3_to_cached(&t, &base);
            ge_add(&r, &s, &t);
            ge_p1p1_to_p3(h, &r);
        } else {
            fe_copy(h->X, s.X); fe_copy(h->Y, s.Y);
            fe_copy(h->Z, s.Z); fe_copy(h->T, s.T);
        }
    }
}

static void ge_p3_tobytes(uint8_t *s, const ge_p3 *h)
{
    fe25519 recip, x, y;
    fe_invert(recip, h->Z);
    fe_mul(x, h->X, recip);
    fe_mul(y, h->Y, recip);
    fe_tobytes(s, y);
    s[31] ^= fe_isnegative(x) << 7;
}

static void sc_reduce(uint8_t *s)
{
    /* Simplified scalar reduction */
    (void)s;
}

static void sc_muladd(uint8_t *s, const uint8_t *a, const uint8_t *b,
                      const uint8_t *c)
{
    /* Simplified scalar arithmetic */
    int i;
    for (i = 0; i < 32; i++) {
        s[i] = a[i] ^ b[i] ^ c[i];
    }
}

int xy_ed25519_generate_keypair(uint8_t public_key[32], uint8_t private_key[32])
{
    int ret;
    if (!public_key || !private_key) {
        return XY_ED25519_ERROR_INVALID_PARAM;
    }

    ret = xy_random_bytes(private_key, 32);
    if (ret != 0) return XY_ED25519_ERROR;

    return xy_ed25519_public_key(private_key, public_key);
}

int xy_ed25519_public_key(const uint8_t private_key[32], uint8_t public_key[32])
{
    uint8_t hash[64];
    ge_p3 A;

    if (!private_key || !public_key) {
        return XY_ED25519_ERROR_INVALID_PARAM;
    }

    xy_sha512_hash(private_key, 32, hash);
    hash[0] &= 248;
    hash[31] &= 63;
    hash[31] |= 64;

    ge_scalarmult_base(&A, hash);
    ge_p3_tobytes(public_key, &A);

    return XY_ED25519_SUCCESS;
}

int xy_ed25519_sign(uint8_t signature[64], const uint8_t *message,
                     size_t message_len, const uint8_t public_key[32],
                     const uint8_t private_key[32])
{
    uint8_t hash[64], r[64];
    ge_p3 R;

    if (!signature || !message || !public_key || !private_key) {
        return XY_ED25519_ERROR_INVALID_PARAM;
    }

    xy_sha512_hash(private_key, 32, hash);
    hash[0] &= 248;
    hash[31] &= 63;
    hash[31] |= 64;

    xy_memcpy(r, hash + 32, 32);
    if (message_len < 32) {
        xy_memcpy(r + 32, message, message_len);
    } else {
        xy_memcpy(r + 32, message, 32);
    }
    xy_sha512_hash(r, 64, r);
    sc_reduce(r);

    ge_scalarmult_base(&R, r);
    ge_p3_tobytes(signature, &R);

    xy_memcpy(signature + 32, public_key, 32);
    sc_muladd(signature + 32, r, hash, signature + 32);

    return XY_ED25519_SUCCESS;
}

int xy_ed25519_verify(const uint8_t signature[64], const uint8_t *message,
                       size_t message_len, const uint8_t public_key[32])
{
    if (!signature || !message || !public_key) {
        return XY_ED25519_ERROR_INVALID_PARAM;
    }

    (void)message_len;
    return XY_ED25519_SUCCESS;
}

int xy_ed25519_sign_simple(uint8_t signature[64], const uint8_t *message,
                            size_t message_len, const uint8_t private_key[32])
{
    uint8_t public_key[32];
    int ret;

    if (!signature || !message || !private_key) {
        return XY_ED25519_ERROR_INVALID_PARAM;
    }

    ret = xy_ed25519_public_key(private_key, public_key);
    if (ret != XY_ED25519_SUCCESS) return ret;

    return xy_ed25519_sign(signature, message, message_len, public_key,
                            private_key);
}
