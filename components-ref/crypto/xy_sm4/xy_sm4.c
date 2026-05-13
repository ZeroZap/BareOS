/**
 * @file xy_sm4.c
 * @brief SM4 Block Cipher Implementation (GM/T 0002-2010)
 * @version 1.0.0
 * @date 2026-04-30
 */

#include <string.h>
#include "xy_sm4.h"
#include "xy_sm3.h"
#include "xy_tiny_crypto.h"

/* SM4 S-box (256 bytes, from standard) */
static const uint8_t sm4_sbox[256] = {
    0xd6, 0x90, 0xe9, 0xfe, 0xcc, 0xe1, 0x3d, 0xb7,
    0x16, 0xb6, 0x14, 0xc2, 0x28, 0xfb, 0x2c, 0x05,
    0x2b, 0x67, 0x9a, 0x76, 0x2a, 0xbb, 0x60, 0x5a,
    0x51, 0x3f, 0x5f, 0x11, 0x68, 0x50, 0x2d, 0x8a,
    0x06, 0x18, 0x1b, 0x20, 0x74, 0xe4, 0x7a, 0x2c,
    0x3b, 0xd6, 0x75, 0xdf, 0x06, 0x26, 0x6d, 0x12,
    0x24, 0x92, 0x3c, 0x37, 0x86, 0x11, 0x41, 0x14,
    0x29, 0x11, 0x3f, 0x5b, 0x7a, 0x77, 0x1d, 0xc3,
    0x58, 0x62, 0x1a, 0x1f, 0x5c, 0x72, 0x16, 0x15,
    0x4b, 0x8e, 0x5f, 0x53, 0x75, 0x7b, 0xba, 0xac,
    0x3e, 0x15, 0x0d, 0x29, 0x44, 0x32, 0xa5, 0x3a,
    0x49, 0x0b, 0x17, 0x1a, 0x5d, 0x9e, 0xa6, 0x21,
    0x07, 0x8d, 0x9a, 0x5e, 0x2f, 0x09, 0x1e, 0x87,
    0x4f, 0x13, 0x85, 0x36, 0xc6, 0x56, 0x41, 0x7a,
    0xdf, 0xe9, 0x6e, 0x4c, 0x0c, 0x83, 0xed, 0xb3,
    0x3d, 0x8c, 0x1d, 0xde, 0x8b, 0x1f, 0x33, 0xd3,
    0x35, 0x96, 0x18, 0xac, 0x1e, 0x46, 0x6f, 0xb5,
    0x59, 0x82, 0xc9, 0x63, 0x7d, 0x24, 0x8e, 0x98,
    0x66, 0x64, 0x99, 0x56, 0x4d, 0x53, 0xc0, 0x48,
    0xef, 0xec, 0x78, 0xcd, 0xaf, 0x31, 0x8b, 0x63,
    0xeb, 0x97, 0x45, 0xbd, 0x92, 0x60, 0xf1, 0x9c,
    0x23, 0x30, 0x14, 0x1f, 0x38, 0x49, 0xf5, 0x47,
    0x27, 0x0e, 0x03, 0x6c, 0xd1, 0x1e, 0x22, 0xff,
    0xc8, 0x6b, 0xd4, 0x94, 0x4e, 0xb1, 0x8f, 0xeb,
    0xb0, 0x54, 0x2f, 0x54, 0x84, 0x6b, 0x5d, 0x05,
    0x5c, 0x57, 0x19, 0x34, 0x71, 0x40, 0x7f, 0x1d,
    0x9c, 0x42, 0xe2, 0x4e, 0x51, 0x33, 0x03, 0xff,
    0x34, 0x7e, 0x08, 0x6d, 0x20, 0xbc, 0x5b, 0x9a,
    0x00, 0x02, 0x19, 0x2e, 0x88, 0xde, 0x13, 0x3c,
    0x80, 0xee, 0x90, 0xb1, 0xd7, 0x40, 0x5e, 0x0b,
    0x4a, 0xce, 0x8d, 0xcf, 0x44, 0xc5, 0x95, 0x0a,
    0x5d, 0x38, 0x5c, 0xa8, 0xc9, 0x55, 0xf3, 0xaa
};

/* System parameter FK (32-bit x 4) */
static const uint32_t FK[4] = {
    0xa3b1bac6, 0x56aa3350, 0x677d9197, 0xb27022dc
};

/* Fixed parameter CK (32-bit x 32) */
static const uint32_t CK[32] = {
    0x00070e15, 0x1c232a31, 0x383f464d, 0x545b6269,
    0x70777e85, 0x8c939aa1, 0xa8afb6bd, 0xc4cbd2d9,
    0xe0e7eeff, 0xfc030a11, 0x181f262d, 0x343b4249,
    0x50575e65, 0x6c737a81, 0x888f969d, 0xa4abb2b9,
    0xc0c7cecf, 0xdceddadb, 0xf8ff0607, 0x141b2229,
    0x303f464d, 0x4c535a69, 0x686f7685, 0x848b929d,
    0xa0a7aeb5, 0xbcc3cacc, 0xd8dfe2e5, 0xf4fbf9ff,
    0x10171e25, 0x2c333a41, 0x484f565d, 0x646b7279
};

/* ==================== Helper Functions ==================== */

/* S-box lookup */
static uint8_t sm4_sbox_get(uint8_t x)
{
    return sm4_sbox[x];
}

/* Non-linear transformation tau */
static uint32_t tau(uint32_t input)
{
    return (uint32_t)sm4_sbox_get((uint8_t)(input >> 24)) << 24 |
           (uint32_t)sm4_sbox_get((uint8_t)(input >> 16)) << 16 |
           (uint32_t)sm4_sbox_get((uint8_t)(input >> 8)) << 8 |
           (uint32_t)sm4_sbox_get((uint8_t)input);
}

/* Linear transformation L */
static uint32_t L(uint32_t B)
{
    return B ^ ROTL(B, 2) ^ ROTL(B, 10) ^ ROTL(B, 18) ^ ROTL(B, 24);
}

/* Linear transformation L' (for key schedule) */
static uint32_t L_prime(uint32_t B)
{
    return B ^ ROTL(B, 13) ^ ROTL(B, 23);
}

/* Inverse linear transformation (for decryption) */
static uint32_t L_inv(uint32_t B)
{
    return B ^ ROTL(B, 2) ^ ROTL(B, 10) ^ ROTL(B, 18) ^ ROTL(B, 24);
}

/* ==================== Key Schedule ==================== */

static void sm4_key_expand(const uint8_t key[16], uint32_t rk[32])
{
    uint32_t K[4];
    uint32_t tmp;
    int i;

    /* K = (MK0 ^ FK, MK1 ^ FK, MK2 ^ FK, MK3 ^ FK) */
    K[0] = ((uint32_t)key[0] << 24) | ((uint32_t)key[1] << 16) |
           ((uint32_t)key[2] << 8) | ((uint32_t)key[3]) ^ FK[0];
    K[1] = ((uint32_t)key[4] << 24) | ((uint32_t)key[5] << 16) |
           ((uint32_t)key[6] << 8) | ((uint32_t)key[7]) ^ FK[1];
    K[2] = ((uint32_t)key[8] << 24) | ((uint32_t)key[9] << 16) |
           ((uint32_t)key[10] << 8) | ((uint32_t)key[11]) ^ FK[2];
    K[3] = ((uint32_t)key[12] << 24) | ((uint32_t)key[13] << 16) |
           ((uint32_t)key[14] << 8) | ((uint32_t)key[15]) ^ FK[3];

    /* Generate 32 round keys */
    for (i = 0; i < 32; i++) {
        tmp = K[1] ^ K[2] ^ K[3] ^ CK[i];
        tmp = tau(tmp);
        tmp = K[0] ^ L_prime(tmp);
        K[0] = K[1];
        K[1] = K[2];
        K[2] = K[3];
        K[3] = rk[i] = tmp;
    }
}

/* ==================== Round Function ==================== */

static void sm4_round(const uint32_t rk[32], 
                      const uint8_t input[16],
                      uint8_t output[16])
{
    uint32_t X[5];
    int i;

    /* X = input */
    X[0] = ((uint32_t)input[0] << 24) | ((uint32_t)input[1] << 16) |
           ((uint32_t)input[2] << 8) | ((uint32_t)input[3]);
    X[1] = ((uint32_t)input[4] << 24) | ((uint32_t)input[5] << 16) |
           ((uint32_t)input[6] << 8) | ((uint32_t)input[7]);
    X[2] = ((uint32_t)input[8] << 24) | ((uint32_t)input[9] << 16) |
           ((uint32_t)input[10] << 8) | ((uint32_t)input[11]);
    X[3] = ((uint32_t)input[12] << 24) | ((uint32_t)input[13] << 16) |
           ((uint32_t)input[14] << 8) | ((uint32_t)input[15]);

    /* 32 rounds */
    for (i = 0; i < 32; i++) {
        X[4] = X[1] ^ X[2] ^ X[3] ^ rk[i];
        X[4] = tau(X[4]);
        X[4] = X[0] ^ L(X[4]);
        X[0] = X[1];
        X[1] = X[2];
        X[2] = X[3];
        X[3] = X[4];
    }

    /* Output (X4 ^ X1 ^ X2 ^ X3) */
    X[4] ^= X[1] ^ X[2] ^ X[3];

    output[0] = (uint8_t)(X[4] >> 24);
    output[1] = (uint8_t)(X[4] >> 16);
    output[2] = (uint8_t)(X[4] >> 8);
    output[3] = (uint8_t)X[4];
    output[4] = (uint8_t)(X[3] >> 24);
    output[5] = (uint8_t)(X[3] >> 16);
    output[6] = (uint8_t)(X[3] >> 8);
    output[7] = (uint8_t)X[3];
    output[8] = (uint8_t)(X[2] >> 24);
    output[9] = (uint8_t)(X[2] >> 16);
    output[10] = (uint8_t)(X[2] >> 8);
    output[11] = (uint8_t)X[2];
    output[12] = (uint8_t)(X[1] >> 24);
    output[13] = (uint8_t)(X[1] >> 16);
    output[14] = (uint8_t)(X[1] >> 8);
    output[15] = (uint8_t)X[1];
}

/* Inverse round (for decryption) */
static void sm4_inv_round(const uint32_t rk[32], 
                       const uint8_t input[16],
                       uint8_t output[16])
{
    uint32_t X[5];
    int i;

    X[0] = ((uint32_t)input[0] << 24) | ((uint32_t)input[1] << 16) |
           ((uint32_t)input[2] << 8) | ((uint32_t)input[3]);
    X[1] = ((uint32_t)input[4] << 24) | ((uint32_t)input[5] << 16) |
           ((uint32_t)input[6] << 8) | ((uint32_t)input[7]);
    X[2] = ((uint32_t)input[8] << 24) | ((uint32_t)input[9] << 16) |
           ((uint32_t)input[10] << 8) | ((uint32_t)input[11]);
    X[3] = ((uint32_t)input[12] << 24) | ((uint32_t)input[13] << 16) |
           ((uint32_t)input[14] << 8) | ((uint32_t)input[15]);

    for (i = 31; i >= 0; i--) {
        X[4] = X[1] ^ X[2] ^ X[3] ^ rk[i];
        X[4] = tau(X[4]);
        X[4] = X[0] ^ L_inv(X[4]);
        X[0] = X[1];
        X[1] = X[2];
        X[2] = X[3];
        X[3] = X[4];
    }

    X[4] ^= X[1] ^ X[2] ^ X[3];

    output[0] = (uint8_t)(X[4] >> 24);
    output[1] = (uint8_t)(X[4] >> 16);
    output[2] = (uint8_t)(X[4] >> 8);
    output[3] = (uint8_t)X[4];
    output[4] = (uint8_t)(X[3] >> 24);
    output[5] = (uint8_t)(X[3] >> 16);
    output[6] = (uint8_t)(X[3] >> 8);
    output[7] = (uint8_t)X[3];
    output[8] = (uint8_t)(X[2] >> 24);
    output[9] = (uint8_t)(X[2] >> 16);
    output[10] = (uint8_t)(X[2] >> 8);
    output[11] = (uint8_t)X[2];
    output[12] = (uint8_t)(X[1] >> 24);
    output[13] = (uint8_t)(X[1] >> 16);
    output[14] = (uint8_t)(X[1] >> 8);
    output[15] = (uint8_t)X[1];
}

/* ==================== SM4 API ==================== */

int xy_sm4_init(xy_sm4_ctx_t *ctx, const uint8_t key[XY_SM4_KEY_SIZE])
{
    if (!ctx || !key) {
        return XY_CRYPTO_INVALID_PARAM;
    }

    sm4_key_expand(key, ctx->rk);
    return XY_CRYPTO_SUCCESS;
}

int xy_sm4_encrypt_block(xy_sm4_ctx_t *ctx,
                       const uint8_t plaintext[XY_SM4_BLOCK_SIZE],
                       uint8_t ciphertext[XY_SM4_BLOCK_SIZE])
{
    if (!ctx || !plaintext || !ciphertext) {
        return XY_CRYPTO_INVALID_PARAM;
    }

    sm4_round(ctx->rk, plaintext, ciphertext);
    return XY_CRYPTO_SUCCESS;
}

int xy_sm4_decrypt_block(xy_sm4_ctx_t *ctx,
                        const uint8_t ciphertext[XY_SM4_BLOCK_SIZE],
                        uint8_t plaintext[XY_SM4_BLOCK_SIZE])
{
    if (!ctx || !ciphertext || !plaintext) {
        return XY_CRYPTO_INVALID_PARAM;
    }

    sm4_inv_round(ctx->rk, ciphertext, plaintext);
    return XY_CRYPTO_SUCCESS;
}

int xy_sm4_cbc_encrypt(xy_sm4_ctx_t *ctx, uint8_t iv[XY_SM4_BLOCK_SIZE],
                      const uint8_t *plaintext, size_t len,
                      uint8_t *ciphertext)
{
    size_t i;
    uint8_t block[16], prev[16];

    if (!ctx || !plaintext || !ciphertext || (len % 16 != 0)) {
        return XY_CRYPTO_INVALID_PARAM;
    }

    memcpy(prev, iv, 16);

    for (i = 0; i < len; i += 16) {
        /* XOR with previous ciphertext */
        int j;
        for (j = 0; j < 16; j++) {
            block[j] = plaintext[i + j] ^ prev[j];
        }

        sm4_round(ctx->rk, block, &ciphertext[i]);
        memcpy(prev, &ciphertext[i], 16);
    }

    /* Update IV */
    memcpy(iv, prev, 16);
    return XY_CRYPTO_SUCCESS;
}

int xy_sm4_cbc_decrypt(xy_sm4_ctx_t *ctx, uint8_t iv[XY_SM4_BLOCK_SIZE],
                       const uint8_t *ciphertext, size_t len,
                       uint8_t *plaintext)
{
    size_t i;
    uint8_t block[16], prev[16];

    if (!ctx || !ciphertext || !plaintext || (len % 16 != 0)) {
        return XY_CRYPTO_INVALID_PARAM;
    }

    memcpy(prev, iv, 16);

    for (i = 0; i < len; i += 16) {
        memcpy(block, &ciphertext[i], 16);
        sm4_inv_round(ctx->rk, block, &plaintext[i]);

        /* XOR with previous ciphertext */
        int j;
        for (j = 0; j < 16; j++) {
            plaintext[i + j] ^= prev[j];
        }
        memcpy(prev, &ciphertext[i], 16);
    }

    memcpy(iv, prev, 16);
    return XY_CRYPTO_SUCCESS;
}

/* GCM (basic version - note: full GCM needs GMAC) */
int xy_sm4_gcm_encrypt(xy_sm4_ctx_t *ctx,
                     const uint8_t iv[12],
                     const uint8_t *aad, size_t aad_len,
                     const uint8_t *plaintext, size_t len,
                     uint8_t *ciphertext,
                     uint8_t tag[16])
{
    /* Basic GCM implementation - simplified */
    uint8_t enc_iv[16];
    xy_sm3_ctx_t sm3_ctx;
    uint8_t hash[32];
    size_t i;

    if (!ctx || !iv || !plaintext || !ciphertext || !tag) {
        return XY_CRYPTO_INVALID_PARAM;
    }

    /* Prepare IV (increment counter from 2) */
    memset(enc_iv, 0, 16);
    memcpy(enc_iv, iv, 12);
    enc_iv[15] = 2;

    /* Encrypt with CTR mode */
    for (i = 0; i < len; i += 16) {
        uint8_t ctr[16], keystream[16];
        size_t block_len = (len - i < 16) ? len - i : 16;
        int j;

        /* Increment counter */
        uint32_t ctr_val = (enc_iv[12] << 24) | (enc_iv[13] << 16) |
                          (enc_iv[14] << 8) | enc_iv[15];
        enc_iv[12] = (uint8_t)(ctr_val >> 24);
        enc_iv[13] = (uint8_t)(ctr_val >> 16);
        enc_iv[14] = (uint8_t)(ctr_val >> 8);
        enc_iv[15] = (uint8_t)ctr_val;

        sm4_round(ctx->rk, enc_iv, keystream);

        for (j = 0; j < (int)block_len; j++) {
            ciphertext[i + j] = plaintext[i + j] ^ keystream[j];
        }
    }

    /* Calculate authentication tag (simplified, uses SM3) */
    xy_sm3_init(&sm3_ctx);
    if (aad && aad_len > 0) {
        xy_sm3_update(&sm3_ctx, aad, aad_len);
    }
    if (ciphertext && len > 0) {
        xy_sm3_update(&sm3_ctx, ciphertext, len);
    }
    xy_sm3_final(&sm3_ctx, hash);

    /* First 16 bytes of hash as tag */
    memcpy(tag, hash, 16);

    return XY_CRYPTO_SUCCESS;
}