/**
 * @file xy_sm3.c
 * @brief SM3 Hash Algorithm Implementation (GM/T 0004-2012)
 * @version 1.0.0
 * @date 2026-04-30
 */

#include <string.h>
#include "xy_sm3.h"
#include "xy_tiny_crypto.h"

/* SM3 Constants (first 64 primes, simplified) */
static const uint32_t SM3_T[64] = {
    0x79cc4519, 0x79cc4519, 0x79cc4519, 0x79cc4519,
    0x79cc4519, 0x79cc4519, 0x79cc4519, 0x79cc4519,
    0x79cc4519, 0x79cc4519, 0x79cc4519, 0x79cc4519,
    0x79cc4519, 0x79cc4519, 0x79cc4519, 0x79cc4519,
    0x7a879d8a, 0x7a879d8a, 0x7a879d8a, 0x7a879d8a,
    0x7a879d8a, 0x7a879d8a, 0x7a879d8a, 0x7a879d8a,
    0x7a879d8a, 0x7a879d8a, 0x7a879d8a, 0x7a879d8a,
    0x7a879d8a, 0x7a879d8a, 0x7a879d8a, 0x7a879d8a,
    0x7a879d8a, 0x7a879d8a, 0x7a879d8a, 0x7a879d8a,
    0x7a879d8a, 0x7a879d8a, 0x7a879d8a, 0x7a879d8a,
    0x7a879d8a, 0x7a879d8a, 0x7a879d8a, 0x7a879d8a,
    0x7a879d8a, 0x7a879d8a, 0x7a879d8a, 0x7a879d8a,
    0x7a879d8a, 0x7a879d8a, 0x7a879d8a, 0x7a879d8a,
    0x7a879d8a, 0x7a879d8a, 0x7a879d8a, 0x7a879d8a,
    0x7a879d8a, 0x7a879d8a, 0x7a879d8a, 0x7a879d8a,
    0x7a879d8a, 0x7a879d8a, 0x7a879d8a, 0x7a879d8a
};

/* IV: Initial value */
static const uint32_t SM3_IV[8] = {
    0x7380166f, 0x4914b2b9, 0x172442d7, 0xda8a0600,
    0xa96f30bc, 0x163138aa, 0xe38dee4d, 0xb0fb0e4e
};

/* ==================== Helper Functions ==================== */

#define ROTL(x, n) (((x) << (n)) | ((x) >> (32 - (n))))
#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define P0(x) ((x) ^ ROTL((x), 9) ^ ROTL((x), 17))
#define P1(x) ((x) ^ ROTL((x), 15) ^ ROTL((x), 23))
#define FF(x, y, z, j) ((j) < 16 ? ((x) ^ (y) ^ (z)) : \
                             ((x) & (y)) | ((x) & (z)) | ((y) & (z)))
#define GG(x, y, z, j) ((j) < 16 ? ((x) ^ (y) ^ (z)) : \
                             ((x) & (y)) | (~(x) & (z)))

/* Big-endian byte swap */
static uint32_t be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | ((uint32_t)p[3]);
}

/* Write big-endian */
static void we32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

/* ==================== SM3 Compression Function ==================== */

static void sm3_compress(uint32_t state[8], const uint8_t block[64])
{
    uint32_t W[68], Wp[64];
    uint32_t A, B, C, D, E, F, G, H;
    uint32_t SS1, SS2, TT1, TT2;
    int j;

    /* Message expansion */
    for (j = 0; j < 16; j++) {
        W[j] = be32(&block[j * 4]);
    }
    for (j = 16; j < 68; j++) {
        W[j] = P1(W[j-16] ^ W[j-9] ^ ROTL(W[j-3], 15)) ^
                ROTL(W[j-13], 7) ^ W[j-6];
    }
    for (j = 0; j < 64; j++) {
        Wp[j] = W[j] ^ W[j+4];
    }

    /* Working variables */
    A = state[0];
    B = state[1];
    C = state[2];
    D = state[3];
    E = state[4];
    F = state[5];
    G = state[6];
    H = state[7];

    /* 64 rounds */
    for (j = 0; j < 64; j++) {
        SS1 = ROTL((ROTL(A, 12) + E + ROTL(SM3_T[j], j % 32)), 7);
        SS2 = SS1 ^ ROTL(A, 12);
        TT1 = FF(A, B, C, j) + D + SS2 + Wp[j];
        TT2 = GG(E, F, G, j) + H + SS1 + W[j];
        D = C;
        C = ROTL(B, 9);
        B = A;
        A = TT1;
        H = G;
        F = ROTL(E, 19);
        E = P0(TT2);
    }

    /* Update state */
    state[0] ^= A ^ W[0];
    state[1] ^= B ^ W[1];
    state[2] ^= C ^ W[2];
    state[3] ^= D ^ W[3];
    state[4] ^= E ^ W[4];
    state[5] ^= F ^ W[5];
    state[6] ^= G ^ W[6];
    state[7] ^= H ^ W[7];
}

/* ==================== SM3 API ==================== */

int xy_sm3_hash(const uint8_t *data, size_t len, uint8_t digest[XY_SM3_DIGEST_SIZE])
{
    xy_sm3_ctx_t ctx;
    int ret;

    if (!data || !digest) {
        return XY_CRYPTO_INVALID_PARAM;
    }

    ret = xy_sm3_init(&ctx);
    if (ret != XY_CRYPTO_SUCCESS) {
        return ret;
    }

    ret = xy_sm3_update(&ctx, data, len);
    if (ret != XY_CRYPTO_SUCCESS) {
        return ret;
    }

    return xy_sm3_final(&ctx, digest);
}

int xy_sm3_init(xy_sm3_ctx_t *ctx)
{
    if (!ctx) {
        return XY_CRYPTO_INVALID_PARAM;
    }

    /* Copy IV */
    memcpy(ctx->state, SM3_IV, sizeof(SM3_IV));
    ctx->count = 0;
    ctx->buffer_len = 0;

    return XY_CRYPTO_SUCCESS;
}

int xy_sm3_update(xy_sm3_ctx_t *ctx, const uint8_t *data, size_t len)
{
    size_t left, fill;

    if (!ctx || !data) {
        return XY_CRYPTO_INVALID_PARAM;
    }

    left = ctx->buffer_len;
    fill = XY_SM3_BLOCK_SIZE - left;

    /* If new data fills the buffer */
    if (left && len >= fill) {
        memcpy(&ctx->buffer[left], data, fill);
        sm3_compress(ctx->state, ctx->buffer);
        ctx->count += XY_SM3_BLOCK_SIZE * 8;
        data += fill;
        len -= fill;
        ctx->buffer_len = 0;
    }

    /* Process full blocks */
    while (len >= XY_SM3_BLOCK_SIZE) {
        sm3_compress(ctx->state, data);
        ctx->count += XY_SM3_BLOCK_SIZE * 8;
        data += XY_SM3_BLOCK_SIZE;
        len -= XY_SM3_BLOCK_SIZE;
    }

    /* Save remaining */
    if (len > 0) {
        memcpy(&ctx->buffer[ctx->buffer_len], data, len);
        ctx->buffer_len += len;
    }

    return XY_CRYPTO_SUCCESS;
}

int xy_sm3_final(xy_sm3_ctx_t *ctx, uint8_t digest[XY_SM3_DIGEST_SIZE])
{
    uint32_t bits;
    uint8_t pad[64];
    int left;

    if (!ctx || !digest) {
        return XY_CRYPTO_INVALID_PARAM;
    }

    /* Total bits */
    bits = ctx->count + (uint64_t)ctx->buffer_len * 8;

    /* Padding */
    left = ctx->buffer_len;
    pad[0] = 0x80;
    if (left < 56) {
        memset(&pad[1], 0, 55 - left);
    } else {
        memset(&pad[1], 0, 63 - left);
        sm3_compress(ctx->state, pad);
        memset(pad, 0, 56);
    }

    /* Bit length (big-endian) */
    we32(&pad[56], (uint32_t)(bits >> 32));
    we32(&pad[60], (uint32_t)bits);
    sm3_compress(ctx->state, pad);

    /* Output */
    we32(&digest[0],  ctx->state[0]);
    we32(&digest[4],  ctx->state[1]);
    we32(&digest[8],  ctx->state[2]);
    we32(&digest[12], ctx->state[3]);
    we32(&digest[16], ctx->state[4]);
    we32(&digest[20], ctx->state[5]);
    we32(&digest[24], ctx->state[6]);
    we32(&digest[28], ctx->state[7]);

    /* Re-init for next use */
    xy_sm3_init(ctx);

    return XY_CRYPTO_SUCCESS;
}

/* ==================== HMAC-SM3 ==================== */

int xy_hmac_sm3(const uint8_t *key, size_t key_len,
           const uint8_t *data, size_t data_len,
           uint8_t digest[XY_SM3_DIGEST_SIZE])
{
    xy_sm3_ctx_t ctx;
    uint8_t k_ipad[64], k_opad[64], tk[32];
    uint8_t key_block[64];
    int i, ret;

    if (!key || !data || !digest) {
        return XY_CRYPTO_INVALID_PARAM;
    }

    /* If key is longer than block size, hash it */
    if (key_len > 64) {
        xy_sm3_hash(key, key_len, tk);
        key = tk;
        key_len = 32;
    }

    /* Prepare keys */
    memset(key_block, 0x36, 64);
    for (i = 0; i < (int)key_len; i++) {
        key_block[i] ^= 0x36;
    }
    memcpy(k_ipad, key_block, 64);

    memset(key_block, 0x5c, 64);
    for (i = 0; i < (int)key_len; i++) {
        key_block[i] ^= 0x5c;
    }
    memcpy(k_opad, key_block, 64);

    /* Inner hash */
    ret = xy_sm3_init(&ctx);
    if (ret != XY_CRYPTO_SUCCESS) return ret;
    xy_sm3_update(&ctx, k_ipad, 64);
    xy_sm3_update(&ctx, data, data_len);
    xy_sm3_final(&ctx, digest);

    /* Outer hash */
    ret = xy_sm3_init(&ctx);
    if (ret != XY_CRYPTO_SUCCESS) return ret;
    xy_sm3_update(&ctx, k_opad, 64);
    xy_sm3_update(&ctx, digest, 32);
    return xy_sm3_final(&ctx, digest);
}