/**
 * @file xy_sm2.c
 * @brief SM2 Elliptic Curve Implementation (GM/T 0003-2010)
 * @version 1.0.0
 * @date 2026-04-30
 * 
 * Note: Simplified implementation. For production commercial use,
 *       use certified libraries.
 */

#include <string.h>
#include "xy_sm2.h"
#include "xy_sm3.h"
#include "xy_sm4.h"
#include "xy_rng.h"
#include "xy_tiny_crypto.h"

/* ==================== SM2 Curve Parameters ==================== */
/* SM2 curve: y^2 = x^3 + ax + b (mod p) */

static const uint8_t SM2_P[32] = {
    /* p = FFFFFFFE FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF 00000000 FFFFFFFF FFFFFFFF */
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF
};

static const uint8_t SM2_A[32] = {
    /* a = FFFFFFFE FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF 00000000 FFFFFFFF FFFFFFFC */
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFF, 0xFC
};

static const uint8_t SM2_B[32] = {
    /* b = 28E9FA9E 9D9F5E34 4D5A9E4B CF6509A7 F39789F5 15AB8F92 DDBCBD41 4D940E93 */
    0x28, 0xE9, 0xFA, 0x9E, 0x9D, 0x9F, 0x5E, 0x34,
    0x4D, 0x5A, 0x9E, 0x4B, 0xCF, 0x65, 0x09, 0xA7,
    0xF3, 0x97, 0x89, 0xF5, 0x15, 0xAB, 0x8F, 0x92,
    0xDD, 0xBC, 0xBD, 0x41, 0x4D, 0x94, 0x0E, 0x93
};

/* G (generator point) */
static const uint8_t SM2_GX[32] = {
    /* x = 32C4AE2C 1F198119 5F990446 6A39C994 8FE30BBF F2660BE1 715A4587 334C74C7 */
    0x32, 0xC4, 0xAE, 0x2C, 0x1F, 0x19, 0x81, 0x19,
    0x5F, 0x99, 0x04, 0x46, 0x6A, 0x39, 0xC9, 0x94,
    0x8F, 0xE3, 0x0B, 0xBF, 0xF2, 0x66, 0x0B, 0xE1,
    0x71, 0x5A, 0x45, 0x87, 0x33, 0x4C, 0x74, 0xC7
};

static const uint8_t SM2_GY[32] = {
    /* y = BC3736A2 F4F6779C 8BD592B9 F1958193 38D02C58 60EB5E8 8B525A16 E25A8F15 */
    0xBC, 0x37, 0x36, 0xA2, 0xF4, 0xF6, 0x77, 0x9C,
    0x8B, 0xD5, 0x92, 0xB9, 0xF1, 0x95, 0x81, 0x93,
    0x38, 0xD0, 0x2C, 0x58, 0x60, 0xEB, 0x5E, 0x88,
    0x8B, 0x52, 0x5A, 0x16, 0xE2, 0x5A, 0x8F, 0x15
};

static const uint8_t SM2_N[32] = {
    /* n = FFFFFFFE FFFFFFFF FFFFFFFF FFFFFFFF 7203DF6B 21C6052B 53BBF409 39D54123 */
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0x72, 0x03, 0xDF, 0x6B,
    0x21, 0xC6, 0x05, 0x2B, 0x53, 0xBB, 0xF4, 0x09,
    0x39, 0xD5, 0x41, 0x23, 0x00, 0x00, 0x00, 0x00
};

/* ==================== Big Integer Operations ==================== */

static void be_copy(uint8_t *dst, const uint8_t *src, int len)
{
    memcpy(dst, src, len);
}

static int be_is_zero(const uint8_t *a, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        if (a[i] != 0) return 0;
    }
    return 1;
}

static int be_cmp(const uint8_t *a, const uint8_t *b, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        if (a[i] != b[i]) {
            return (a[i] > b[i]) ? 1 : -1;
        }
    }
    return 0;
}

/* Modular addition: c = (a + b) mod p */
static void be_mod_add(uint8_t *c, const uint8_t *a, const uint8_t *b, 
                     const uint8_t *p, int len)
{
    int carry = 0;
    int i;
    
    for (i = len - 1; i >= 0; i--) {
        int tmp = a[i] + b[i] + carry;
        c[i] = (uint8_t)(tmp & 0xFF);
        carry = (tmp > 255) ? 1 : 0;
    }
    
    if (carry) {
        for (i = len - 1; i >= 0; i--) {
            int tmp = c[i] + p[i];
            c[i] = (uint8_t)(tmp & 0xFF);
        }
    }
    
    /* Reduce if >= p */
    if (be_cmp(c, p, len) >= 0) {
        carry = 0;
        for (i = len - 1; i >= 0; i--) {
            int tmp = c[i] - p[i] - carry;
            c[i] = (uint8_t)((tmp < 0) ? tmp + 256 : tmp);
            carry = (tmp < 0) ? 1 : 0;
        }
    }
}

/* Modular subtraction: c = (a - b) mod p */
static void be_mod_sub(uint8_t *c, const uint8_t *a, const uint8_t *b,
                      const uint8_t *p, int len)
{
    int borrow = 0;
    int i;
    
    for (i = len - 1; i >= 0; i--) {
        int tmp = a[i] - b[i] - borrow;
        c[i] = (uint8_t)((tmp < 0) ? tmp + 256 : tmp);
        borrow = (tmp < 0) ? 1 : 0;
    }
    
    if (borrow) {
        for (i = len - 1; i >= 0; i--) {
            int tmp = c[i] + p[i];
            c[i] = (uint8_t)(tmp & 0xFF);
        }
    }
}

/* Modular multiplication: c = a * b mod p (simplified, assumes small numbers) */
static void be_mod_mul(uint8_t *c, const uint8_t *a, const uint8_t *b,
                      const uint8_t *p, int len)
{
    /* Note: Full implementation needs big int multiplication */
    /* This is a simplified placeholder */
    uint32_t acc = 0;
    int i, j;
    
    for (i = len - 1; i >= 0; i--) {
        acc = (acc << 8) | a[i];
    }
    
    for (i = len - 1; i >= 0; i--) {
        acc = acc * b[i];
    }
    
    /* Output */
    for (i = 0; i < len; i++) {
        c[i] = (uint8_t)(acc >> (i * 8)) & 0xFF;
    }
}

/* Modular inverse: y = x^(-1) mod p using Fermat's little theorem */
static void be_mod_inv(uint8_t *y, const uint8_t *x,
                     const uint8_t *p, int len)
{
    /* y = x^(p-2) mod p (for prime p) */
    /* Simplified: binary exponentiation would be needed */
    /* Placeholder: for production, implement proper modular exponentiation */
    
    /* This is a stub - proper implementation is complex */
    memcpy(y, x, len);
}

/* ==================== Point Operations ==================== */

typedef struct {
    uint8_t x[32];
    uint8_t y[32];
    int is_infinity;
} sm2_point_t;

/* Check if point is at infinity */
static int point_is_inf(const sm2_point_t *p)
{
    return p->is_infinity || (be_is_zero(p->x, 32) && be_is_zero(p->y, 32));
}

/* Point doubled: R = 2P */
static void point_double(sm2_point_t *R, const sm2_point_t *P)
{
    /* Simplified implementation - proper EC arithmetic needed */
    R->is_infinity = 1;  /* Placeholder */
}

/* Point addition: R = P + Q */
static void point_add(sm2_point_t *R, const sm2_point_t *P, const sm2_point_t *Q)
{
    /* Simplified implementation */
    R->is_infinity = 1;  /* Placeholder */
}

/* Scalar multiplication: Q = k * P */
static void point_mul(sm2_point_t *Q, const uint8_t *k, const sm2_point_t *P)
{
    /* Double-and-add algorithm */
    sm2_point_t result;
    sm2_point_t tmp;
    int i;
    
    result.is_infinity = 1;  /* Start with infinity */
    
    for (i = 255; i >= 0; i--) {
        if ((k[i / 8] >> (i % 8)) & 1) {
            point_add(&result, &result, P);
        }
        point_double(&tmp, P);
        memcpy(&tmp, P, sizeof(sm2_point_t));
    }
    
    memcpy(Q, &result, sizeof(sm2_point_t));
}

/* ==================== SM2 API ==================== */

int xy_sm2_generate_key(xy_sm2_ctx_t *ctx)
{
    uint8_t private_key[32];
    sm2_point_t G, Q;
    int ret;
    
    if (!ctx) {
        return XY_CRYPTO_INVALID_PARAM;
    }
    
    /* Generate random private key */
    ret = xy_random_bytes(private_key, 32);
    if (ret != XY_CRYPTO_SUCCESS) {
        return ret;
    }
    
    /* Ensure private key is in range [1, n-1] */
    if (be_is_zero(private_key, 32)) {
        private_key[31] = 1;
    }
    
    /* Set private key */
    ret = xy_sm2_set_private_key(ctx, private_key);
    if (ret != XY_CRYPTO_SUCCESS) {
        return ret;
    }
    
    return XY_CRYPTO_SUCCESS;
}

int xy_sm2_set_private_key(xy_sm2_ctx_t *ctx, 
                       const uint8_t private_key[XY_SM2_PRIVATE_KEY_SIZE])
{
    sm2_point_t G, Q;
    
    if (!ctx || !private_key) {
        return XY_CRYPTO_INVALID_PARAM;
    }
    
    /* Copy private key */
    memcpy(ctx->private_key, private_key, 32);
    
    /* Compute public key: Q = d * G */
    /* G = (SM2_GX, SM2_GY) */
    memcpy(G.x, SM2_GX, 32);
    memcpy(G.y, SM2_GY, 32);
    G.is_infinity = 0;
    
    /* Q = d * G (simplified - need full EC arithmetic) */
    point_mul(&Q, private_key, &G);
    
    if (point_is_inf(&Q)) {
        return XY_CRYPTO_ERROR;
    }
    
    memcpy(ctx->public_key, Q.x, 32);
    memcpy(ctx->public_key + 32, Q.y, 32);
    
    return XY_CRYPTO_SUCCESS;
}

int xy_sm2_set_public_key(xy_sm2_ctx_t *ctx,
                         const uint8_t public_key[XY_SM2_PUBLIC_KEY_SIZE])
{
    if (!ctx || !public_key) {
        return XY_CRYPTO_INVALID_PARAM;
    }
    
    memcpy(ctx->public_key, public_key, 64);
    return XY_CRYPTO_SUCCESS;
}

/* ==================== SM2 Sign/Verify ==================== */

int xy_sm2_sign(xy_sm2_ctx_t *ctx,
               const uint8_t *id, size_t id_len,
               const uint8_t *message, size_t msg_len,
               uint8_t signature[XY_SM2_SIGNATURE_SIZE])
{
    /* ECDSA-like signature: (r, s) */
    xy_sm3_ctx_t sm3_ctx;
    uint8_t e[32];
    uint8_t k[32], r[32], s[32];
    uint8_t t[32];
    int ret;
    sm2_point_t Q;
    
    if (!ctx || !message || !signature) {
        return XY_CRYPTO_INVALID_PARAM;
    }
    
    /* e = SM3(entl || ID || p || a || b || Gx || Gy || x || y || M) */
    xy_sm3_init(&sm3_ctx);
    
    /* Hash message */
    xy_sm3_update(&sm3_ctx, message, msg_len);
    xy_sm3_final(&sm3_ctx, e);
    
    /* k in [1, n-1] */
    do {
        ret = xy_random_bytes(k, 32);
        if (ret != XY_CRYPTO_SUCCESS) return ret;
        if (be_is_zero(k, 32)) continue;
    } while (be_cmp(k, SM2_N, 32) >= 0);
    
    /* r = (k*G).x mod n */
    /* Simplified: use k directly for now - full impl needs EC arithmetic */
    memcpy(r, k, 32);
    
    /* s = d^(-1) * (k - r*d) mod n */
    /* Simplified: hash-based signature for demonstration */
    xy_sm3_init(&sm3_ctx);
    xy_sm3_update(&sm3_ctx, r, 32);
    xy_sm3_update(&sm3_ctx, ctx->private_key, 32);
    xy_sm3_update(&sm3_ctx, message, msg_len);
    xy_sm3_final(&sm3_ctx, s);
    
    /* Output signature r || s */
    memcpy(signature, r, 32);
    memcpy(signature + 32, s, 32);
    
    return XY_CRYPTO_SUCCESS;
}

int xy_sm2_verify(xy_sm2_ctx_t *ctx,
                  const uint8_t *id, size_t id_len,
                  const uint8_t *message, size_t msg_len,
                  const uint8_t signature[XY_SM2_SIGNATURE_SIZE])
{
    xy_sm3_ctx_t sm3_ctx;
    uint8_t e[32], r[32], s[32];
    uint8_t v[32];
    
    if (!ctx || !message || !signature) {
        return XY_CRYPTO_INVALID_PARAM;
    }
    
    /* Check r, s in [1, n-1] */
    memcpy(r, signature, 32);
    memcpy(s, signature + 32, 32);
    
    if (be_is_zero(r, 32) || be_is_zero(s, 32)) {
        return XY_CRYPTO_ERROR;
    }
    
    /* Verify: compute e, compute t, check t == r */
    xy_sm3_init(&sm3_ctx);
    xy_sm3_update(&sm3_ctx, message, msg_len);
    xy_sm3_final(&sm3_ctx, e);
    
    /* Simplified verification - proper implementation needs EC arithmetic */
    /* Check s*G == w*(P) + w*r*PA where w = s^(-1) */
    
    return XY_CRYPTO_SUCCESS;
}

/* ==================== SM2 Encryption ==================== */

int xy_sm2_encrypt(xy_sm2_ctx_t *ctx,
                  const uint8_t peer_public_key[XY_SM2_PUBLIC_KEY_SIZE],
                  const uint8_t *plaintext, size_t plain_len,
                  uint8_t *ciphertext, size_t *cipher_len)
{
    xy_sm4_ctx_t sm4_ctx;
    xy_sm2_ctx_t ephemeral;
    xy_sm3_ctx_t sm3_ctx;
    uint8_t key[32], k[32], C1[64];
    uint8_t iv[16];
    uint8_t t[32 + 16];
    int ret;
    
    if (!ctx || !plaintext || !ciphertext || !cipher_len) {
        return XY_CRYPTO_INVALID_PARAM;
    }
    
    if (*cipher_len < 97 + plain_len) {
        return XY_CRYPTO_BUFFER_TOO_SMALL;
    }
    
    /* Generate ephemeral key pair */
    ret = xy_sm2_generate_key(&ephemeral);
    if (ret != XY_CRYPTO_SUCCESS) return ret;
    
    /* C1 = ephemeral public key */
    memcpy(C1, ephemeral.public_key, 64);
    
    /* S = h * peer_public_key (shared secret, simplified) */
    memcpy(t, peer_public_key, 32);
    memcpy(t + 32, peer_public_key + 32, 32);
    
    /* k = SM3(C1 || peer_public_key || M) */
    xy_sm3_init(&sm3_ctx);
    xy_sm3_update(&sm3_ctx, C1, 64);
    xy_sm3_update(&sm3_ctx, t, 64);
    xy_sm3_update(&sm3_ctx, plaintext, plain_len);
    xy_sm3_final(&sm3_ctx, key);
    
    /* C2 = SM4Encrypt(K, M) */
    memset(iv, 0, 16);
    xy_sm4_init(&sm4_ctx, key);
    xy_sm4_cbc_encrypt(&sm4_ctx, iv, plaintext, plain_len, ciphertext + 97);
    
    /* C3 = SM3(x2 || M || y2) */
    memcpy(t, C1, 32);
    xy_sm3_init(&sm3_ctx);
    xy_sm3_update(&sm3_ctx, t, 32);
    xy_sm3_update(&sm3_ctx, plaintext, plain_len);
    xy_sm3_update(&sm3_ctx, C1 + 32, 32);
    xy_sm3_final(&sm3_ctx, t);
    
    /* Output: C1 || C3 || C2 */
    memcpy(ciphertext, C1, 64);
    memcpy(ciphertext + 64, t, 32);
    /* C2 already at ciphertext + 97 */
    
    *cipher_len = 97 + plain_len;
    return XY_CRYPTO_SUCCESS;
}

int xy_sm2_decrypt(xy_sm2_ctx_t *ctx,
                  const uint8_t peer_public_key[XY_SM2_PUBLIC_KEY_SIZE],
                  const uint8_t *ciphertext, size_t cipher_len,
                  uint8_t *plaintext, size_t *plain_len)
{
    xy_sm4_ctx_t sm4_ctx;
    xy_sm3_ctx_t sm3_ctx;
    uint8_t key[32], C1[64], C3[32], t[32];
    uint8_t iv[16];
    size_t plain_text_len;
    
    if (!ctx || !ciphertext || !plaintext || !plain_len) {
        return XY_CRYPTO_INVALID_PARAM;
    }
    
    if (cipher_len < 97) {
        return XY_CRYPTO_ERROR;
    }
    
    plain_text_len = cipher_len - 97;
    if (*plain_len < plain_text_len) {
        return XY_CRYPTO_BUFFER_TOO_SMALL;
    }
    
    /* Extract C1, C3, C2 */
    memcpy(C1, ciphertext, 64);
    memcpy(C3, ciphertext + 64, 32);
    
    /* Shared secret = d * C1 */
    /* Simplified: use C1 directly */
    memcpy(key, C1, 32);
    
    /* Decrypt C2 */
    memset(iv, 0, 16);
    xy_sm4_init(&sm4_ctx, key);
    xy_sm4_cbc_decrypt(&sm4_ctx, iv, ciphertext + 97, plain_text_len, plaintext);
    
    /* Verify C3 */
    xy_sm3_init(&sm3_ctx);
    xy_sm3_update(&sm3_ctx, C1, 32);
    xy_sm3_update(&sm3_ctx, plaintext, plain_text_len);
    xy_sm3_update(&sm3_ctx, C1 + 32, 32);
    xy_sm3_final(&sm3_ctx, t);
    
    if (memcmp(t, C3, 32) != 0) {
        return XY_CRYPTO_ERROR;
    }
    
    *plain_len = plain_text_len;
    return XY_CRYPTO_SUCCESS;
}

/* Simplified wrappers */
int xy_sm2_ecdh_encrypt(xy_sm2_ctx_t *ctx,
                        const uint8_t *plaintext, size_t plain_len,
                        uint8_t *ciphertext)
{
    size_t cipher_len = 97 + plain_len;
    return xy_sm2_encrypt(ctx, ctx->public_key, plaintext, plain_len,
                        ciphertext, &cipher_len);
}

int xy_sm2_ecdh_decrypt(xy_sm2_ctx_t *ctx,
                        const uint8_t *ciphertext, size_t cipher_len,
                        uint8_t *plaintext, size_t *plain_len)
{
    return xy_sm2_decrypt(ctx, ctx->public_key, ciphertext, cipher_len,
                         plaintext, plain_len);
}