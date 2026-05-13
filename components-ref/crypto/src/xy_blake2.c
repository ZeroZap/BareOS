/**
 * @file xy_blake2.c
 * @brief BLAKE2b and BLAKE2s implementation
 * @version 1.0.0
 * @date 2025-11-02
 *
 * Implementation based on RFC 7693 with optimizations for embedded systems.
 * BLAKE2s is enabled by default for 32-bit platforms.
 * BLAKE2b is optional and disabled by default to save code space.
 */

#include "xy_blake2.h"
#include <stdint.h>
#include <string.h>

#if XY_BLAKE2_ENABLE_BLAKE2B || XY_BLAKE2_ENABLE_BLAKE2S

/* ==================== Helper Functions (shared) ==================== */

static uint64_t prv_load64_le(const uint8_t *src)
{
    return ((uint64_t)src[0])
           | ((uint64_t)src[1] << 8)
           | ((uint64_t)src[2] << 16)
           | ((uint64_t)src[3] << 24)
           | ((uint64_t)src[4] << 32)
           | ((uint64_t)src[5] << 40)
           | ((uint64_t)src[6] << 48)
           | ((uint64_t)src[7] << 56);
}

static void prv_store64_le(uint8_t *dst, uint64_t value)
{
    dst[0] = (uint8_t)(value & 0xff);
    dst[1] = (uint8_t)((value >> 8) & 0xff);
    dst[2] = (uint8_t)((value >> 16) & 0xff);
    dst[3] = (uint8_t)((value >> 24) & 0xff);
    dst[4] = (uint8_t)((value >> 32) & 0xff);
    dst[5] = (uint8_t)((value >> 40) & 0xff);
    dst[6] = (uint8_t)((value >> 48) & 0xff);
    dst[7] = (uint8_t)((value >> 56) & 0xff);
}

static uint32_t prv_load32_le(const uint8_t *src)
{
    return ((uint32_t)src[0])
           | ((uint32_t)src[1] << 8)
           | ((uint32_t)src[2] << 16)
           | ((uint32_t)src[3] << 24);
}

static void prv_store32_le(uint8_t *dst, uint32_t value)
{
    dst[0] = (uint8_t)(value & 0xff);
    dst[1] = (uint8_t)((value >> 8) & 0xff);
    dst[2] = (uint8_t)((value >> 16) & 0xff);
    dst[3] = (uint8_t)((value >> 24) & 0xff);
}

static uint64_t prv_rotr64(uint64_t w, unsigned c)
{
    return (w >> c) | (w << (64 - c));
}

static uint32_t prv_rotr32(uint32_t w, unsigned c)
{
    return (w >> c) | (w << (32 - c));
}

#endif /* XY_BLAKE2_ENABLE_BLAKE2B || XY_BLAKE2_ENABLE_BLAKE2S */

/* ==================== BLAKE2b Implementation ==================== */

#if XY_BLAKE2_ENABLE_BLAKE2B

static const uint64_t blake2b_iv[8] = {
    0x6a09e667f3bcc908ULL, 0xbb67ae8584caa73bULL,
    0x3c6ef372fe94f82bULL, 0xa54ff53a5f1d36f1ULL,
    0x510e527fade682d1ULL, 0x9b05688c2b3e6c1fULL,
    0x1f83d9abfb41bd6bULL, 0x5be0cd19137e2179ULL
};

static const uint8_t blake2b_sigma[12][16] = {
    {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 },
    { 14, 10,  4,  8,  9, 15, 13,  6,  1, 12,  0,  2, 11,  7,  5,  3 },
    { 11,  8, 12,  0,  5,  2, 15, 13, 10, 14,  3,  6,  7,  1,  9,  4 },
    {  7,  9,  3,  1, 13, 12, 11, 14,  2,  6,  5, 10,  4,  0, 15,  8 },
    {  9,  0,  5,  7,  2,  4, 10, 15, 14,  1, 11, 12,  6,  8,  3, 13 },
    {  2, 12,  6, 10,  0, 11,  8,  3,  4, 13,  7,  5, 15, 14,  1,  9 },
    { 12,  5,  1, 15, 14, 13,  4, 10,  0,  7,  6,  3,  9,  2,  8, 11 },
    { 13, 11,  7, 14, 12,  1,  3,  9,  5,  0, 15,  4,  8,  6,  2, 10 },
    {  6, 15, 14,  9, 11,  3,  0,  8, 12,  2, 13,  7,  1,  4, 10,  5 },
    { 10,  2,  8,  4,  7,  6,  1,  5, 15, 11,  9, 14,  3, 12, 13 , 0 },
    {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 },
    { 14, 10,  4,  8,  9, 15, 13,  6,  1, 12,  0,  2, 11,  7,  5,  3 }
};

#define B2B_G(r,i,a,b,c,d)                      \
    do {                                        \
        a = a + b + m[blake2b_sigma[r][2*i+0]]; \
        d = prv_rotr64(d ^ a, 32);              \
        c = c + d;                              \
        b = prv_rotr64(b ^ c, 24);              \
        a = a + b + m[blake2b_sigma[r][2*i+1]]; \
        d = prv_rotr64(d ^ a, 16);              \
        c = c + d;                              \
        b = prv_rotr64(b ^ c, 63);              \
    } while(0)

static void prv_blake2b_compress(xy_blake2b_ctx_t *ctx, const uint8_t block[XY_BLAKE2B_BLOCKBYTES])
{
    uint64_t m[16], v[16];
    size_t i;

    for (i = 0; i < 16; i++) {
        m[i] = prv_load64_le(block + i * 8);
    }

    for (i = 0; i < 8; i++) {
        v[i] = ctx->h[i];
    }
    v[ 8] = blake2b_iv[0];
    v[ 9] = blake2b_iv[1];
    v[10] = blake2b_iv[2];
    v[11] = blake2b_iv[3];
    v[12] = blake2b_iv[4] ^ ctx->t[0];
    v[13] = blake2b_iv[5] ^ ctx->t[1];
    v[14] = blake2b_iv[6] ^ ctx->f[0];
    v[15] = blake2b_iv[7] ^ ctx->f[1];

    /* 12 rounds */
    B2B_G(0, 0, v[0], v[4], v[ 8], v[12]);
    B2B_G(0, 1, v[1], v[5], v[ 9], v[13]);
    B2B_G(0, 2, v[2], v[6], v[10], v[14]);
    B2B_G(0, 3, v[3], v[7], v[11], v[15]);
    B2B_G(0, 4, v[0], v[5], v[10], v[15]);
    B2B_G(0, 5, v[1], v[6], v[11], v[12]);
    B2B_G(0, 6, v[2], v[7], v[ 8], v[13]);
    B2B_G(0, 7, v[3], v[4], v[ 9], v[14]);

    B2B_G(1, 0, v[0], v[4], v[ 8], v[12]);
    B2B_G(1, 1, v[1], v[5], v[ 9], v[13]);
    B2B_G(1, 2, v[2], v[6], v[10], v[14]);
    B2B_G(1, 3, v[3], v[7], v[11], v[15]);
    B2B_G(1, 4, v[0], v[5], v[10], v[15]);
    B2B_G(1, 5, v[1], v[6], v[11], v[12]);
    B2B_G(1, 6, v[2], v[7], v[ 8], v[13]);
    B2B_G(1, 7, v[3], v[4], v[ 9], v[14]);

    B2B_G(2, 0, v[0], v[4], v[ 8], v[12]);
    B2B_G(2, 1, v[1], v[5], v[ 9], v[13]);
    B2B_G(2, 2, v[2], v[6], v[10], v[14]);
    B2B_G(2, 3, v[3], v[7], v[11], v[15]);
    B2B_G(2, 4, v[0], v[5], v[10], v[15]);
    B2B_G(2, 5, v[1], v[6], v[11], v[12]);
    B2B_G(2, 6, v[2], v[7], v[ 8], v[13]);
    B2B_G(2, 7, v[3], v[4], v[ 9], v[14]);

    B2B_G(3, 0, v[0], v[4], v[ 8], v[12]);
    B2B_G(3, 1, v[1], v[5], v[ 9], v[13]);
    B2B_G(3, 2, v[2], v[6], v[10], v[14]);
    B2B_G(3, 3, v[3], v[7], v[11], v[15]);
    B2B_G(3, 4, v[0], v[5], v[10], v[15]);
    B2B_G(3, 5, v[1], v[6], v[11], v[12]);
    B2B_G(3, 6, v[2], v[7], v[ 8], v[13]);
    B2B_G(3, 7, v[3], v[4], v[ 9], v[14]);

    B2B_G(4, 0, v[0], v[4], v[ 8], v[12]);
    B2B_G(4, 1, v[1], v[5], v[ 9], v[13]);
    B2B_G(4, 2, v[2], v[6], v[10], v[14]);
    B2B_G(4, 3, v[3], v[7], v[11], v[15]);
    B2B_G(4, 4, v[0], v[5], v[10], v[15]);
    B2B_G(4, 5, v[1], v[6], v[11], v[12]);
    B2B_G(4, 6, v[2], v[7], v[ 8], v[13]);
    B2B_G(4, 7, v[3], v[4], v[ 9], v[14]);

    B2B_G(5, 0, v[0], v[4], v[ 8], v[12]);
    B2B_G(5, 1, v[1], v[5], v[ 9], v[13]);
    B2B_G(5, 2, v[2], v[6], v[10], v[14]);
    B2B_G(5, 3, v[3], v[7], v[11], v[15]);
    B2B_G(5, 4, v[0], v[5], v[10], v[15]);
    B2B_G(5, 5, v[1], v[6], v[11], v[12]);
    B2B_G(5, 6, v[2], v[7], v[ 8], v[13]);
    B2B_G(5, 7, v[3], v[4], v[ 9], v[14]);

    B2B_G(6, 0, v[0], v[4], v[ 8], v[12]);
    B2B_G(6, 1, v[1], v[5], v[ 9], v[13]);
    B2B_G(6, 2, v[2], v[6], v[10], v[14]);
    B2B_G(6, 3, v[3], v[7], v[11], v[15]);
    B2B_G(6, 4, v[0], v[5], v[10], v[15]);
    B2B_G(6, 5, v[1], v[6], v[11], v[12]);
    B2B_G(6, 6, v[2], v[7], v[ 8], v[13]);
    B2B_G(6, 7, v[3], v[4], v[ 9], v[14]);

    B2B_G(7, 0, v[0], v[4], v[ 8], v[12]);
    B2B_G(7, 1, v[1], v[5], v[ 9], v[13]);
    B2B_G(7, 2, v[2], v[6], v[10], v[14]);
    B2B_G(7, 3, v[3], v[7], v[11], v[15]);
    B2B_G(7, 4, v[0], v[5], v[10], v[15]);
    B2B_G(7, 5, v[1], v[6], v[11], v[12]);
    B2B_G(7, 6, v[2], v[7], v[ 8], v[13]);
    B2B_G(7, 7, v[3], v[4], v[ 9], v[14]);

    B2B_G(8, 0, v[0], v[4], v[ 8], v[12]);
    B2B_G(8, 1, v[1], v[5], v[ 9], v[13]);
    B2B_G(8, 2, v[2], v[6], v[10], v[14]);
    B2B_G(8, 3, v[3], v[7], v[11], v[15]);
    B2B_G(8, 4, v[0], v[5], v[10], v[15]);
    B2B_G(8, 5, v[1], v[6], v[11], v[12]);
    B2B_G(8, 6, v[2], v[7], v[ 8], v[13]);
    B2B_G(8, 7, v[3], v[4], v[ 9], v[14]);

    B2B_G(9, 0, v[0], v[4], v[ 8], v[12]);
    B2B_G(9, 1, v[1], v[5], v[ 9], v[13]);
    B2B_G(9, 2, v[2], v[6], v[10], v[14]);
    B2B_G(9, 3, v[3], v[7], v[11], v[15]);
    B2B_G(9, 4, v[0], v[5], v[10], v[15]);
    B2B_G(9, 5, v[1], v[6], v[11], v[12]);
    B2B_G(9, 6, v[2], v[7], v[ 8], v[13]);
    B2B_G(9, 7, v[3], v[4], v[ 9], v[14]);

    B2B_G(10, 0, v[0], v[4], v[ 8], v[12]);
    B2B_G(10, 1, v[1], v[5], v[ 9], v[13]);
    B2B_G(10, 2, v[2], v[6], v[10], v[14]);
    B2B_G(10, 3, v[3], v[7], v[11], v[15]);
    B2B_G(10, 4, v[0], v[5], v[10], v[15]);
    B2B_G(10, 5, v[1], v[6], v[11], v[12]);
    B2B_G(10, 6, v[2], v[7], v[ 8], v[13]);
    B2B_G(10, 7, v[3], v[4], v[ 9], v[14]);

    B2B_G(11, 0, v[0], v[4], v[ 8], v[12]);
    B2B_G(11, 1, v[1], v[5], v[ 9], v[13]);
    B2B_G(11, 2, v[2], v[6], v[10], v[14]);
    B2B_G(11, 3, v[3], v[7], v[11], v[15]);
    B2B_G(11, 4, v[0], v[5], v[10], v[15]);
    B2B_G(11, 5, v[1], v[6], v[11], v[12]);
    B2B_G(11, 6, v[2], v[7], v[ 8], v[13]);
    B2B_G(11, 7, v[3], v[4], v[ 9], v[14]);

    for (i = 0; i < 8; i++) {
        ctx->h[i] ^= v[i] ^ v[i + 8];
    }
}

int xy_blake2b_init_param(xy_blake2b_ctx_t *ctx, const xy_blake2b_param_t *param)
{
    const uint8_t *p;
    size_t i;

    if (!ctx || !param) {
        return XY_BLAKE2_ERROR_INVALID_PARAM;
    }

    memset(ctx, 0, sizeof(xy_blake2b_ctx_t));

    for (i = 0; i < 8; i++) {
        ctx->h[i] = blake2b_iv[i];
    }

    ctx->outlen = param->digest_length;

    p = (const uint8_t *)param;
    for (i = 0; i < 8; i++) {
        ctx->h[i] ^= prv_load64_le(p + i * 8);
    }

    return XY_BLAKE2_SUCCESS;
}

int xy_blake2b_init(xy_blake2b_ctx_t *ctx, size_t outlen)
{
    xy_blake2b_param_t param;

    if (!ctx || outlen == 0 || outlen > XY_BLAKE2B_OUTBYTES) {
        return XY_BLAKE2_ERROR_INVALID_PARAM;
    }

    memset(&param, 0, sizeof(param));
    param.digest_length = (uint8_t)outlen;
    param.fanout = 1;
    param.depth = 1;

    return xy_blake2b_init_param(ctx, &param);
}

int xy_blake2b_init_key(xy_blake2b_ctx_t *ctx, size_t outlen,
                         const uint8_t *key, size_t keylen)
{
    xy_blake2b_param_t param;
    int ret;

    if (!ctx || !key || keylen == 0 || keylen > XY_BLAKE2B_KEYBYTES) {
        return XY_BLAKE2_ERROR_INVALID_PARAM;
    }
    if (outlen == 0 || outlen > XY_BLAKE2B_OUTBYTES) {
        return XY_BLAKE2_ERROR_INVALID_PARAM;
    }

    memset(&param, 0, sizeof(param));
    param.digest_length = (uint8_t)outlen;
    param.key_length = (uint8_t)keylen;
    param.fanout = 1;
    param.depth = 1;

    ret = xy_blake2b_init_param(ctx, &param);
    if (ret != XY_BLAKE2_SUCCESS) {
        return ret;
    }

    {
        uint8_t block[XY_BLAKE2B_BLOCKBYTES];
        memset(block, 0, sizeof(block));
        memcpy(block, key, keylen);
        ret = xy_blake2b_update(ctx, block, sizeof(block));
        memset(block, 0, sizeof(block));
    }

    return ret;
}

int xy_blake2b_update(xy_blake2b_ctx_t *ctx, const uint8_t *data, size_t datalen)
{
    size_t left, fill;

    if (!ctx || (!data && datalen > 0)) {
        return XY_BLAKE2_ERROR_INVALID_PARAM;
    }

    left = ctx->buflen;
    fill = XY_BLAKE2B_BLOCKBYTES - left;

    if (datalen > fill) {
        ctx->buflen = 0;
        memcpy(ctx->buf + left, data, fill);
        ctx->t[0] += XY_BLAKE2B_BLOCKBYTES;
        if (ctx->t[0] < XY_BLAKE2B_BLOCKBYTES) {
            ctx->t[1]++;
        }
        prv_blake2b_compress(ctx, ctx->buf);
        data += fill;
        datalen -= fill;

        while (datalen > XY_BLAKE2B_BLOCKBYTES) {
            ctx->t[0] += XY_BLAKE2B_BLOCKBYTES;
            if (ctx->t[0] < XY_BLAKE2B_BLOCKBYTES) {
                ctx->t[1]++;
            }
            prv_blake2b_compress(ctx, data);
            data += XY_BLAKE2B_BLOCKBYTES;
            datalen -= XY_BLAKE2B_BLOCKBYTES;
        }
    }

    memcpy(ctx->buf + ctx->buflen, data, datalen);
    ctx->buflen += datalen;

    return XY_BLAKE2_SUCCESS;
}

int xy_blake2b_final(xy_blake2b_ctx_t *ctx, uint8_t *digest, size_t outlen)
{
    uint8_t buffer[XY_BLAKE2B_OUTBYTES];
    size_t i;

    if (!ctx || !digest) {
        return XY_BLAKE2_ERROR_INVALID_PARAM;
    }
    if (outlen != ctx->outlen) {
        return XY_BLAKE2_ERROR_INVALID_PARAM;
    }

    ctx->t[0] += ctx->buflen;
    if (ctx->t[0] < ctx->buflen) {
        ctx->t[1]++;
    }

    ctx->f[0] = (uint64_t)-1;

    memset(ctx->buf + ctx->buflen, 0, XY_BLAKE2B_BLOCKBYTES - ctx->buflen);
    prv_blake2b_compress(ctx, ctx->buf);

    for (i = 0; i < 8; i++) {
        prv_store64_le(buffer + i * 8, ctx->h[i]);
    }

    memcpy(digest, buffer, outlen);
    memset(buffer, 0, sizeof(buffer));

    return XY_BLAKE2_SUCCESS;
}

int xy_blake2b(uint8_t *digest, size_t outlen,
                const uint8_t *data, size_t datalen,
                const uint8_t *key, size_t keylen)
{
    xy_blake2b_ctx_t ctx;
    int ret;

    if (key && keylen > 0) {
        ret = xy_blake2b_init_key(&ctx, outlen, key, keylen);
    } else {
        ret = xy_blake2b_init(&ctx, outlen);
    }

    if (ret != XY_BLAKE2_SUCCESS) {
        return ret;
    }

    if (data && datalen > 0) {
        ret = xy_blake2b_update(&ctx, data, datalen);
        if (ret != XY_BLAKE2_SUCCESS) {
            return ret;
        }
    }

    ret = xy_blake2b_final(&ctx, digest, outlen);

    memset(&ctx, 0, sizeof(ctx));

    return ret;
}

#endif /* XY_BLAKE2_ENABLE_BLAKE2B */

/* ==================== BLAKE2s Implementation ==================== */

#if XY_BLAKE2_ENABLE_BLAKE2S

static const uint32_t blake2s_iv[8] = {
    0x6A09E667UL, 0xBB67AE85UL, 0x3C6EF372UL, 0xA54FF53AUL,
    0x510E527FUL, 0x9B05688CUL, 0x1F83D9ABUL, 0x5BE0CD19UL
};

static const uint8_t blake2s_sigma[10][16] = {
    {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 },
    { 14, 10,  4,  8,  9, 15, 13,  6,  1, 12,  0,  2, 11,  7,  5,  3 },
    { 11,  8, 12,  0,  5,  2, 15, 13, 10, 14,  3,  6,  7,  1,  9,  4 },
    {  7,  9,  3,  1, 13, 12, 11, 14,  2,  6,  5, 10,  4,  0, 15,  8 },
    {  9,  0,  5,  7,  2,  4, 10, 15, 14,  1, 11, 12,  6,  8,  3, 13 },
    {  2, 12,  6, 10,  0, 11,  8,  3,  4, 13,  7,  5, 15, 14,  1,  9 },
    { 12,  5,  1, 15, 14, 13,  4, 10,  0,  7,  6,  3,  9,  2,  8, 11 },
    { 13, 11,  7, 14, 12,  1,  3,  9,  5,  0, 15,  4,  8,  6,  2, 10 },
    {  6, 15, 14,  9, 11,  3,  0,  8, 12,  2, 13,  7,  1,  4, 10,  5 },
    { 10,  2,  8,  4,  7,  6,  1,  5, 15, 11,  9, 14,  3, 12, 13 , 0 }
};

#define B2S_G(r,i,a,b,c,d)                      \
    do {                                        \
        a = a + b + m[blake2s_sigma[r][2*i+0]]; \
        d = prv_rotr32(d ^ a, 16);              \
        c = c + d;                              \
        b = prv_rotr32(b ^ c, 12);              \
        a = a + b + m[blake2s_sigma[r][2*i+1]]; \
        d = prv_rotr32(d ^ a, 8);               \
        c = c + d;                              \
        b = prv_rotr32(b ^ c, 7);               \
    } while(0)

static void prv_blake2s_compress(xy_blake2s_ctx_t *ctx, const uint8_t block[XY_BLAKE2S_BLOCKBYTES])
{
    uint32_t m[16], v[16];
    size_t i;

    for (i = 0; i < 16; i++) {
        m[i] = prv_load32_le(block + i * 4);
    }

    for (i = 0; i < 8; i++) {
        v[i] = ctx->h[i];
    }
    v[ 8] = blake2s_iv[0];
    v[ 9] = blake2s_iv[1];
    v[10] = blake2s_iv[2];
    v[11] = blake2s_iv[3];
    v[12] = blake2s_iv[4] ^ ctx->t[0];
    v[13] = blake2s_iv[5] ^ ctx->t[1];
    v[14] = blake2s_iv[6] ^ ctx->f[0];
    v[15] = blake2s_iv[7] ^ ctx->f[1];

    /* 10 rounds */
    for (int round = 0; round < 10; round++) {
        B2S_G(round, 0, v[0], v[4], v[ 8], v[12]);
        B2S_G(round, 1, v[1], v[5], v[ 9], v[13]);
        B2S_G(round, 2, v[2], v[6], v[10], v[14]);
        B2S_G(round, 3, v[3], v[7], v[11], v[15]);
        B2S_G(round, 4, v[0], v[5], v[10], v[15]);
        B2S_G(round, 5, v[1], v[6], v[11], v[12]);
        B2S_G(round, 6, v[2], v[7], v[ 8], v[13]);
        B2S_G(round, 7, v[3], v[4], v[ 9], v[14]);
    }

    for (i = 0; i < 8; i++) {
        ctx->h[i] ^= v[i] ^ v[i + 8];
    }
}

int xy_blake2s_init_param(xy_blake2s_ctx_t *ctx, const xy_blake2s_param_t *param)
{
    const uint8_t *p;
    size_t i;

    if (!ctx || !param) {
        return XY_BLAKE2_ERROR_INVALID_PARAM;
    }

    memset(ctx, 0, sizeof(xy_blake2s_ctx_t));

    for (i = 0; i < 8; i++) {
        ctx->h[i] = blake2s_iv[i];
    }

    ctx->outlen = param->digest_length;

    p = (const uint8_t *)param;
    for (i = 0; i < 8; i++) {
        ctx->h[i] ^= prv_load32_le(p + i * 4);
    }

    return XY_BLAKE2_SUCCESS;
}

int xy_blake2s_init(xy_blake2s_ctx_t *ctx, size_t outlen)
{
    xy_blake2s_param_t param;

    if (!ctx || outlen == 0 || outlen > XY_BLAKE2S_OUTBYTES) {
        return XY_BLAKE2_ERROR_INVALID_PARAM;
    }

    memset(&param, 0, sizeof(param));
    param.digest_length = (uint8_t)outlen;
    param.fanout = 1;
    param.depth = 1;

    return xy_blake2s_init_param(ctx, &param);
}

int xy_blake2s_init_key(xy_blake2s_ctx_t *ctx, size_t outlen,
                         const uint8_t *key, size_t keylen)
{
    xy_blake2s_param_t param;
    int ret;

    if (!ctx || !key || keylen == 0 || keylen > XY_BLAKE2S_KEYBYTES) {
        return XY_BLAKE2_ERROR_INVALID_PARAM;
    }
    if (outlen == 0 || outlen > XY_BLAKE2S_OUTBYTES) {
        return XY_BLAKE2_ERROR_INVALID_PARAM;
    }

    memset(&param, 0, sizeof(param));
    param.digest_length = (uint8_t)outlen;
    param.key_length = (uint8_t)keylen;
    param.fanout = 1;
    param.depth = 1;

    ret = xy_blake2s_init_param(ctx, &param);
    if (ret != XY_BLAKE2_SUCCESS) {
        return ret;
    }

    {
        uint8_t block[XY_BLAKE2S_BLOCKBYTES];
        memset(block, 0, sizeof(block));
        memcpy(block, key, keylen);
        ret = xy_blake2s_update(ctx, block, sizeof(block));
        memset(block, 0, sizeof(block));
    }

    return ret;
}

int xy_blake2s_update(xy_blake2s_ctx_t *ctx, const uint8_t *data, size_t datalen)
{
    size_t left, fill;

    if (!ctx || (!data && datalen > 0)) {
        return XY_BLAKE2_ERROR_INVALID_PARAM;
    }

    left = ctx->buflen;
    fill = XY_BLAKE2S_BLOCKBYTES - left;

    if (datalen > fill) {
        ctx->buflen = 0;
        memcpy(ctx->buf + left, data, fill);
        ctx->t[0] += XY_BLAKE2S_BLOCKBYTES;
        if (ctx->t[0] < XY_BLAKE2S_BLOCKBYTES) {
            ctx->t[1]++;
        }
        prv_blake2s_compress(ctx, ctx->buf);
        data += fill;
        datalen -= fill;

        while (datalen > XY_BLAKE2S_BLOCKBYTES) {
            ctx->t[0] += XY_BLAKE2S_BLOCKBYTES;
            if (ctx->t[0] < XY_BLAKE2S_BLOCKBYTES) {
                ctx->t[1]++;
            }
            prv_blake2s_compress(ctx, data);
            data += XY_BLAKE2S_BLOCKBYTES;
            datalen -= XY_BLAKE2S_BLOCKBYTES;
        }
    }

    memcpy(ctx->buf + ctx->buflen, data, datalen);
    ctx->buflen += datalen;

    return XY_BLAKE2_SUCCESS;
}

int xy_blake2s_final(xy_blake2s_ctx_t *ctx, uint8_t *digest, size_t outlen)
{
    uint8_t buffer[XY_BLAKE2S_OUTBYTES];
    size_t i;

    if (!ctx || !digest) {
        return XY_BLAKE2_ERROR_INVALID_PARAM;
    }
    if (outlen != ctx->outlen) {
        return XY_BLAKE2_ERROR_INVALID_PARAM;
    }

    ctx->t[0] += ctx->buflen;
    if (ctx->t[0] < ctx->buflen) {
        ctx->t[1]++;
    }

    ctx->f[0] = (uint32_t)-1;

    memset(ctx->buf + ctx->buflen, 0, XY_BLAKE2S_BLOCKBYTES - ctx->buflen);
    prv_blake2s_compress(ctx, ctx->buf);

    for (i = 0; i < 8; i++) {
        prv_store32_le(buffer + i * 4, ctx->h[i]);
    }

    memcpy(digest, buffer, outlen);
    memset(buffer, 0, sizeof(buffer));

    return XY_BLAKE2_SUCCESS;
}

int xy_blake2s(uint8_t *digest, size_t outlen,
                const uint8_t *data, size_t datalen,
                const uint8_t *key, size_t keylen)
{
    xy_blake2s_ctx_t ctx;
    int ret;

    if (key && keylen > 0) {
        ret = xy_blake2s_init_key(&ctx, outlen, key, keylen);
    } else {
        ret = xy_blake2s_init(&ctx, outlen);
    }

    if (ret != XY_BLAKE2_SUCCESS) {
        return ret;
    }

    if (data && datalen > 0) {
        ret = xy_blake2s_update(&ctx, data, datalen);
        if (ret != XY_BLAKE2_SUCCESS) {
            return ret;
        }
    }

    ret = xy_blake2s_final(&ctx, digest, outlen);

    memset(&ctx, 0, sizeof(ctx));

    return ret;
}

#endif /* XY_BLAKE2_ENABLE_BLAKE2S */
