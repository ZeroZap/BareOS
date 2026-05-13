/**
 * @file xy_chacha20_poly1305.c
 * @brief ChaCha20-Poly1305 AEAD implementation
 * @version 1.0.0
 * @date 2025-11-02
 *
 * Implementation based on RFC 8439 with constant-time operations
 * for security-critical applications.
 */

#include "xy_chacha20_poly1305.h"
#include "xy_string.h"
#include <stdint.h>

/* ==================== ChaCha20 Implementation ==================== */

/**
 * @brief ChaCha20 quarter round operation
 *
 * Performs one quarter round on four 32-bit state words.
 *
 * @param a First state word
 * @param b Second state word
 * @param c Third state word
 * @param d Fourth state word
 */
static void prv_chacha20_quarter_round(uint32_t *a, uint32_t *b,
                                        uint32_t *c, uint32_t *d)
{
    *a += *b;
    *d ^= *a;
    *d = (*d << 16) | (*d >> 16);

    *c += *d;
    *b ^= *c;
    *b = (*b << 12) | (*b >> 20);

    *a += *b;
    *d ^= *a;
    *d = (*d << 8) | (*d >> 24);

    *c += *d;
    *b ^= *c;
    *b = (*b << 7) | (*b >> 25);
}

/**
 * @brief Load 32-bit little-endian value
 */
static uint32_t prv_load32_le(const uint8_t *src)
{
    return ((uint32_t)src[0])
           | ((uint32_t)src[1] << 8)
           | ((uint32_t)src[2] << 16)
           | ((uint32_t)src[3] << 24);
}

/**
 * @brief Store 32-bit value as little-endian
 */
static void prv_store32_le(uint8_t *dst, uint32_t value)
{
    dst[0] = (uint8_t)(value & 0xff);
    dst[1] = (uint8_t)((value >> 8) & 0xff);
    dst[2] = (uint8_t)((value >> 16) & 0xff);
    dst[3] = (uint8_t)((value >> 24) & 0xff);
}

/**
 * @brief ChaCha20 block function
 *
 * Generates one 64-byte keystream block.
 *
 * @param output Output buffer for keystream
 * @param state ChaCha20 state (16 words)
 */
static void prv_chacha20_block(uint8_t output[64], const uint32_t state[16])
{
    uint32_t working_state[16];
    int i;

    /* Copy state to working state */
    for (i = 0; i < 16; i++) {
        working_state[i] = state[i];
    }

    /* 20 rounds (10 double rounds) */
    for (i = 0; i < 10; i++) {
        /* Column rounds */
        prv_chacha20_quarter_round(&working_state[0], &working_state[4],
                                    &working_state[8], &working_state[12]);
        prv_chacha20_quarter_round(&working_state[1], &working_state[5],
                                    &working_state[9], &working_state[13]);
        prv_chacha20_quarter_round(&working_state[2], &working_state[6],
                                    &working_state[10], &working_state[14]);
        prv_chacha20_quarter_round(&working_state[3], &working_state[7],
                                    &working_state[11], &working_state[15]);

        /* Diagonal rounds */
        prv_chacha20_quarter_round(&working_state[0], &working_state[5],
                                    &working_state[10], &working_state[15]);
        prv_chacha20_quarter_round(&working_state[1], &working_state[6],
                                    &working_state[11], &working_state[12]);
        prv_chacha20_quarter_round(&working_state[2], &working_state[7],
                                    &working_state[8], &working_state[13]);
        prv_chacha20_quarter_round(&working_state[3], &working_state[4],
                                    &working_state[9], &working_state[14]);
    }

    /* Add original state to working state */
    for (i = 0; i < 16; i++) {
        working_state[i] += state[i];
    }

    /* Serialize output */
    for (i = 0; i < 16; i++) {
        prv_store32_le(&output[i * 4], working_state[i]);
    }
}

int xy_chacha20_init(xy_chacha20_ctx_t *ctx,
                      const uint8_t key[XY_CHACHA20_KEY_SIZE],
                      const uint8_t nonce[XY_CHACHA20_NONCE_SIZE],
                      uint32_t counter)
{
    const uint8_t *magic_constant = (const uint8_t *)"expand 32-byte k";

    /* Validate parameters */
    if (!ctx || !key || !nonce) {
        return XY_CHACHA20_POLY1305_ERROR_INVALID_PARAM;
    }

    /* Initialize state with constants "expand 32-byte k" */
    ctx->state[0] = prv_load32_le(&magic_constant[0]);
    ctx->state[1] = prv_load32_le(&magic_constant[4]);
    ctx->state[2] = prv_load32_le(&magic_constant[8]);
    ctx->state[3] = prv_load32_le(&magic_constant[12]);

    /* Load 256-bit key */
    ctx->state[4] = prv_load32_le(&key[0]);
    ctx->state[5] = prv_load32_le(&key[4]);
    ctx->state[6] = prv_load32_le(&key[8]);
    ctx->state[7] = prv_load32_le(&key[12]);
    ctx->state[8] = prv_load32_le(&key[16]);
    ctx->state[9] = prv_load32_le(&key[20]);
    ctx->state[10] = prv_load32_le(&key[24]);
    ctx->state[11] = prv_load32_le(&key[28]);

    /* Block counter */
    ctx->state[12] = counter;

    /* 96-bit nonce */
    ctx->state[13] = prv_load32_le(&nonce[0]);
    ctx->state[14] = prv_load32_le(&nonce[4]);
    ctx->state[15] = prv_load32_le(&nonce[8]);

    ctx->counter = counter;
    ctx->keystream_pos = 64; /* Force generation on first use */

    return XY_CHACHA20_POLY1305_SUCCESS;
}

int xy_chacha20_crypt(xy_chacha20_ctx_t *ctx,
                       uint8_t *output,
                       const uint8_t *input,
                       size_t length)
{
    size_t i;

    /* Validate parameters */
    if (!ctx || !output || !input) {
        return XY_CHACHA20_POLY1305_ERROR_INVALID_PARAM;
    }

    for (i = 0; i < length; i++) {
        /* Generate new keystream block if needed */
        if (ctx->keystream_pos >= 64) {
            prv_chacha20_block(ctx->keystream, ctx->state);
            ctx->state[12]++; /* Increment counter */
            ctx->keystream_pos = 0;
        }

        /* XOR input with keystream */
        output[i] = input[i] ^ ctx->keystream[ctx->keystream_pos++];
    }

    return XY_CHACHA20_POLY1305_SUCCESS;
}

/* ==================== Poly1305 Implementation ==================== */

/**
 * @brief Poly1305 modular multiplication
 *
 * Multiplies accumulator h by r, modulo 2^130-5.
 *
 * @param ctx Poly1305 context
 */
static void prv_poly1305_multiply(xy_poly1305_ctx_t *ctx)
{
    uint64_t d0, d1, d2, d3, d4;
    uint32_t h0, h1, h2, h3, h4;
    uint64_t c;

    h0 = ctx->h[0];
    h1 = ctx->h[1];
    h2 = ctx->h[2];
    h3 = ctx->h[3];
    h4 = ctx->h[4];

    /* h * r */
    d0 = ((uint64_t)h0 * ctx->r[0])
        + ((uint64_t)h1 * ctx->r[4])
        + ((uint64_t)h2 * ctx->r[3])
        + ((uint64_t)h3 * ctx->r[2])
        + ((uint64_t)h4 * ctx->r[1]);

    d1 = ((uint64_t)h0 * ctx->r[1])
        + ((uint64_t)h1 * ctx->r[0])
        + ((uint64_t)h2 * ctx->r[4])
        + ((uint64_t)h3 * ctx->r[3])
        + ((uint64_t)h4 * ctx->r[2]);

    d2 = ((uint64_t)h0 * ctx->r[2])
        + ((uint64_t)h1 * ctx->r[1])
        + ((uint64_t)h2 * ctx->r[0])
        + ((uint64_t)h3 * ctx->r[4])
        + ((uint64_t)h4 * ctx->r[3]);

    d3 = ((uint64_t)h0 * ctx->r[3])
        + ((uint64_t)h1 * ctx->r[2])
        + ((uint64_t)h2 * ctx->r[1])
        + ((uint64_t)h3 * ctx->r[0])
        + ((uint64_t)h4 * ctx->r[4]);

    d4 = ((uint64_t)h0 * ctx->r[4])
        + ((uint64_t)h1 * ctx->r[3])
        + ((uint64_t)h2 * ctx->r[2])
        + ((uint64_t)h3 * ctx->r[1])
        + ((uint64_t)h4 * ctx->r[0]);

    /* Partial reduction modulo 2^130-5 */
    c = (d0 >> 26);
    ctx->h[0] = (uint32_t)d0 & 0x3ffffff;
    d1 += c;

    c = (d1 >> 26);
    ctx->h[1] = (uint32_t)d1 & 0x3ffffff;
    d2 += c;

    c = (d2 >> 26);
    ctx->h[2] = (uint32_t)d2 & 0x3ffffff;
    d3 += c;

    c = (d3 >> 26);
    ctx->h[3] = (uint32_t)d3 & 0x3ffffff;
    d4 += c;

    c = (d4 >> 26);
    ctx->h[4] = (uint32_t)d4 & 0x3ffffff;
    ctx->h[0] += (uint32_t)(c * 5);

    c = (ctx->h[0] >> 26);
    ctx->h[0] &= 0x3ffffff;
    ctx->h[1] += (uint32_t)c;
}

/**
 * @brief Process one 16-byte Poly1305 block
 *
 * @param ctx Poly1305 context
 * @param block 16-byte input block
 */
static void prv_poly1305_block(xy_poly1305_ctx_t *ctx, const uint8_t block[16])
{
    uint32_t t0, t1, t2, t3;

    /* Load block in little-endian */
    t0 = prv_load32_le(&block[0]);
    t1 = prv_load32_le(&block[4]);
    t2 = prv_load32_le(&block[8]);
    t3 = prv_load32_le(&block[12]);

    /* Add to accumulator */
    ctx->h[0] += t0 & 0x3ffffff;
    ctx->h[1] += ((t0 >> 26) | (t1 << 6)) & 0x3ffffff;
    ctx->h[2] += ((t1 >> 20) | (t2 << 12)) & 0x3ffffff;
    ctx->h[3] += ((t2 >> 14) | (t3 << 18)) & 0x3ffffff;
    ctx->h[4] += (t3 >> 8) | (1 << 24); /* Add 2^128 */

    /* Multiply by r */
    prv_poly1305_multiply(ctx);
}

int xy_poly1305_init(xy_poly1305_ctx_t *ctx,
                      const uint8_t key[XY_POLY1305_KEY_SIZE])
{
    /* Validate parameters */
    if (!ctx || !key) {
        return XY_CHACHA20_POLY1305_ERROR_INVALID_PARAM;
    }

    /* Clear accumulator */
    ctx->h[0] = 0;
    ctx->h[1] = 0;
    ctx->h[2] = 0;
    ctx->h[3] = 0;
    ctx->h[4] = 0;

    /* Load and clamp r */
    ctx->r[0] = (prv_load32_le(&key[0])) & 0x3ffffff;
    ctx->r[1] = (prv_load32_le(&key[3]) >> 2) & 0x3ffff03;
    ctx->r[2] = (prv_load32_le(&key[6]) >> 4) & 0x3ffc0ff;
    ctx->r[3] = (prv_load32_le(&key[9]) >> 6) & 0x3f03fff;
    ctx->r[4] = (prv_load32_le(&key[12]) >> 8) & 0x00fffff;

    /* Precompute 5*r for modular reduction */
    ctx->r[1] *= 5;
    ctx->r[2] *= 5;
    ctx->r[3] *= 5;
    ctx->r[4] *= 5;

    /* Load s */
    ctx->s[0] = prv_load32_le(&key[16]);
    ctx->s[1] = prv_load32_le(&key[20]);
    ctx->s[2] = prv_load32_le(&key[24]);
    ctx->s[3] = prv_load32_le(&key[28]);

    ctx->buffer_len = 0;

    return XY_CHACHA20_POLY1305_SUCCESS;
}

int xy_poly1305_update(xy_poly1305_ctx_t *ctx,
                        const uint8_t *data,
                        size_t length)
{
    size_t i;

    /* Validate parameters */
    if (!ctx || (!data && length > 0)) {
        return XY_CHACHA20_POLY1305_ERROR_INVALID_PARAM;
    }

    i = 0;

    /* Process buffered data first */
    if (ctx->buffer_len > 0) {
        size_t to_copy = 16 - ctx->buffer_len;
        if (to_copy > length) {
            to_copy = length;
        }

        xy_memcpy(&ctx->buffer[ctx->buffer_len], &data[i], to_copy);
        ctx->buffer_len += to_copy;
        i += to_copy;

        if (ctx->buffer_len == 16) {
            prv_poly1305_block(ctx, ctx->buffer);
            ctx->buffer_len = 0;
        }
    }

    /* Process complete blocks */
    while (i + 16 <= length) {
        prv_poly1305_block(ctx, &data[i]);
        i += 16;
    }

    /* Buffer remaining bytes */
    if (i < length) {
        xy_memcpy(ctx->buffer, &data[i], length - i);
        ctx->buffer_len = length - i;
    }

    return XY_CHACHA20_POLY1305_SUCCESS;
}

int xy_poly1305_finish(xy_poly1305_ctx_t *ctx,
                        uint8_t tag[XY_POLY1305_TAG_SIZE])
{
    uint64_t f0, f1, f2, f3;
    uint32_t g0, g1, g2, g3, g4;
    uint32_t mask;

    /* Validate parameters */
    if (!ctx || !tag) {
        return XY_CHACHA20_POLY1305_ERROR_INVALID_PARAM;
    }

    /* Process final partial block if any */
    if (ctx->buffer_len > 0) {
        size_t i;
        /* Pad with zeros */
        for (i = ctx->buffer_len; i < 16; i++) {
            ctx->buffer[i] = 0;
        }

        /* Process with padding bit at position buffer_len */
        {
            uint32_t t0, t1, t2, t3;
            t0 = prv_load32_le(&ctx->buffer[0]);
            t1 = prv_load32_le(&ctx->buffer[4]);
            t2 = prv_load32_le(&ctx->buffer[8]);
            t3 = prv_load32_le(&ctx->buffer[12]);

            ctx->h[0] += t0 & 0x3ffffff;
            ctx->h[1] += ((t0 >> 26) | (t1 << 6)) & 0x3ffffff;
            ctx->h[2] += ((t1 >> 20) | (t2 << 12)) & 0x3ffffff;
            ctx->h[3] += ((t2 >> 14) | (t3 << 18)) & 0x3ffffff;
            ctx->h[4] += (t3 >> 8) | (1 << (ctx->buffer_len * 8 - 104));

            prv_poly1305_multiply(ctx);
        }
    }

    /* Final reduction modulo 2^130-5 */
    g0 = ctx->h[0] + 5;
    g1 = ctx->h[1] + (g0 >> 26);
    g0 &= 0x3ffffff;
    g2 = ctx->h[2] + (g1 >> 26);
    g1 &= 0x3ffffff;
    g3 = ctx->h[3] + (g2 >> 26);
    g2 &= 0x3ffffff;
    g4 = ctx->h[4] + (g3 >> 26) - (1 << 26);
    g3 &= 0x3ffffff;

    /* Select h if h < p, else select g = h - p */
    mask = (g4 >> 31) - 1; /* All 1s if g4 negative, all 0s otherwise */
    ctx->h[0] = (ctx->h[0] & ~mask) | (g0 & mask);
    ctx->h[1] = (ctx->h[1] & ~mask) | (g1 & mask);
    ctx->h[2] = (ctx->h[2] & ~mask) | (g2 & mask);
    ctx->h[3] = (ctx->h[3] & ~mask) | (g3 & mask);
    ctx->h[4] = (ctx->h[4] & ~mask) | (g4 & mask);

    /* Combine into 128-bit value */
    f0 = ((ctx->h[0]) | ((uint64_t)ctx->h[1] << 26)) + ctx->s[0];
    f1 = ((ctx->h[1] >> 6) | ((uint64_t)ctx->h[2] << 20)) + ctx->s[1];
    f2 = ((ctx->h[2] >> 12) | ((uint64_t)ctx->h[3] << 14)) + ctx->s[2];
    f3 = ((ctx->h[3] >> 18) | ((uint64_t)ctx->h[4] << 8)) + ctx->s[3];

    /* Handle carries */
    f1 += (f0 >> 32);
    f2 += (f1 >> 32);
    f3 += (f2 >> 32);

    /* Output tag */
    prv_store32_le(&tag[0], (uint32_t)f0);
    prv_store32_le(&tag[4], (uint32_t)f1);
    prv_store32_le(&tag[8], (uint32_t)f2);
    prv_store32_le(&tag[12], (uint32_t)f3);

    return XY_CHACHA20_POLY1305_SUCCESS;
}

/* ==================== ChaCha20-Poly1305 AEAD ==================== */

/**
 * @brief Pad length to 16-byte boundary
 *
 * @param poly Poly1305 context
 * @param len Length value to pad
 */
static void prv_poly1305_pad16(xy_poly1305_ctx_t *poly, size_t len)
{
    static const uint8_t zeros[16] = {0};
    size_t padding = (16 - (len % 16)) % 16;

    if (padding > 0) {
        xy_poly1305_update(poly, zeros, padding);
    }
}

int xy_chacha20_poly1305_encrypt(
    const uint8_t key[XY_CHACHA20_POLY1305_KEY_SIZE],
    const uint8_t nonce[XY_CHACHA20_POLY1305_NONCE_SIZE],
    const uint8_t *aad,
    size_t aad_len,
    const uint8_t *plaintext,
    size_t plaintext_len,
    uint8_t *ciphertext,
    uint8_t tag[XY_CHACHA20_POLY1305_TAG_SIZE])
{
    xy_chacha20_ctx_t chacha_ctx;
    xy_poly1305_ctx_t poly_ctx;
    uint8_t poly_key[32];
    uint8_t length_block[16];
    int ret;

    /* Validate parameters */
    if (!key || !nonce || !ciphertext || !tag) {
        return XY_CHACHA20_POLY1305_ERROR_INVALID_PARAM;
    }
    if ((!plaintext && plaintext_len > 0) || (!aad && aad_len > 0)) {
        return XY_CHACHA20_POLY1305_ERROR_INVALID_PARAM;
    }

    /* Generate Poly1305 key using ChaCha20 with counter=0 */
    ret = xy_chacha20_init(&chacha_ctx, key, nonce, 0);
    if (ret != XY_CHACHA20_POLY1305_SUCCESS) {
        return ret;
    }

    xy_memset(poly_key, 0, sizeof(poly_key));
    ret = xy_chacha20_crypt(&chacha_ctx, poly_key, poly_key, 32);
    if (ret != XY_CHACHA20_POLY1305_SUCCESS) {
        return ret;
    }

    /* Initialize Poly1305 */
    ret = xy_poly1305_init(&poly_ctx, poly_key);
    if (ret != XY_CHACHA20_POLY1305_SUCCESS) {
        return ret;
    }

    /* Encrypt plaintext with counter=1 */
    ret = xy_chacha20_init(&chacha_ctx, key, nonce, 1);
    if (ret != XY_CHACHA20_POLY1305_SUCCESS) {
        return ret;
    }

    if (plaintext_len > 0) {
        ret = xy_chacha20_crypt(&chacha_ctx, ciphertext, plaintext,
                                plaintext_len);
        if (ret != XY_CHACHA20_POLY1305_SUCCESS) {
            return ret;
        }
    }

    /* Construct Poly1305 input: AAD || pad || ciphertext || pad || lengths */
    if (aad_len > 0) {
        xy_poly1305_update(&poly_ctx, aad, aad_len);
        prv_poly1305_pad16(&poly_ctx, aad_len);
    }

    if (plaintext_len > 0) {
        xy_poly1305_update(&poly_ctx, ciphertext, plaintext_len);
        prv_poly1305_pad16(&poly_ctx, plaintext_len);
    }

    /* Add lengths (64-bit little-endian) */
    prv_store32_le(&length_block[0], (uint32_t)aad_len);
    prv_store32_le(&length_block[4], (uint32_t)(aad_len >> 32));
    prv_store32_le(&length_block[8], (uint32_t)plaintext_len);
    prv_store32_le(&length_block[12], (uint32_t)(plaintext_len >> 32));
    xy_poly1305_update(&poly_ctx, length_block, 16);

    /* Generate tag */
    ret = xy_poly1305_finish(&poly_ctx, tag);

    /* Clear sensitive data */
    xy_memset(poly_key, 0, sizeof(poly_key));
    xy_memset(&chacha_ctx, 0, sizeof(chacha_ctx));
    xy_memset(&poly_ctx, 0, sizeof(poly_ctx));

    return ret;
}

int xy_chacha20_poly1305_decrypt(
    const uint8_t key[XY_CHACHA20_POLY1305_KEY_SIZE],
    const uint8_t nonce[XY_CHACHA20_POLY1305_NONCE_SIZE],
    const uint8_t *aad,
    size_t aad_len,
    const uint8_t *ciphertext,
    size_t ciphertext_len,
    const uint8_t tag[XY_CHACHA20_POLY1305_TAG_SIZE],
    uint8_t *plaintext)
{
    xy_chacha20_ctx_t chacha_ctx;
    xy_poly1305_ctx_t poly_ctx;
    uint8_t poly_key[32];
    uint8_t computed_tag[16];
    uint8_t length_block[16];
    int ret;
    size_t i;
    uint8_t diff;

    /* Validate parameters */
    if (!key || !nonce || !tag || !plaintext) {
        return XY_CHACHA20_POLY1305_ERROR_INVALID_PARAM;
    }
    if ((!ciphertext && ciphertext_len > 0) || (!aad && aad_len > 0)) {
        return XY_CHACHA20_POLY1305_ERROR_INVALID_PARAM;
    }

    /* Generate Poly1305 key */
    ret = xy_chacha20_init(&chacha_ctx, key, nonce, 0);
    if (ret != XY_CHACHA20_POLY1305_SUCCESS) {
        return ret;
    }

    xy_memset(poly_key, 0, sizeof(poly_key));
    ret = xy_chacha20_crypt(&chacha_ctx, poly_key, poly_key, 32);
    if (ret != XY_CHACHA20_POLY1305_SUCCESS) {
        return ret;
    }

    /* Initialize Poly1305 and compute tag */
    ret = xy_poly1305_init(&poly_ctx, poly_key);
    if (ret != XY_CHACHA20_POLY1305_SUCCESS) {
        return ret;
    }

    if (aad_len > 0) {
        xy_poly1305_update(&poly_ctx, aad, aad_len);
        prv_poly1305_pad16(&poly_ctx, aad_len);
    }

    if (ciphertext_len > 0) {
        xy_poly1305_update(&poly_ctx, ciphertext, ciphertext_len);
        prv_poly1305_pad16(&poly_ctx, ciphertext_len);
    }

    prv_store32_le(&length_block[0], (uint32_t)aad_len);
    prv_store32_le(&length_block[4], (uint32_t)(aad_len >> 32));
    prv_store32_le(&length_block[8], (uint32_t)ciphertext_len);
    prv_store32_le(&length_block[12], (uint32_t)(ciphertext_len >> 32));
    xy_poly1305_update(&poly_ctx, length_block, 16);

    ret = xy_poly1305_finish(&poly_ctx, computed_tag);
    if (ret != XY_CHACHA20_POLY1305_SUCCESS) {
        goto cleanup;
    }

    /* Constant-time tag comparison */
    diff = 0;
    for (i = 0; i < 16; i++) {
        diff |= tag[i] ^ computed_tag[i];
    }

    if (diff != 0) {
        ret = XY_CHACHA20_POLY1305_ERROR_AUTH_FAILED;
        goto cleanup;
    }

    /* Tag verified - decrypt ciphertext */
    ret = xy_chacha20_init(&chacha_ctx, key, nonce, 1);
    if (ret != XY_CHACHA20_POLY1305_SUCCESS) {
        goto cleanup;
    }

    if (ciphertext_len > 0) {
        ret = xy_chacha20_crypt(&chacha_ctx, plaintext, ciphertext,
                                ciphertext_len);
    }

cleanup:
    /* Clear sensitive data */
    xy_memset(poly_key, 0, sizeof(poly_key));
    xy_memset(computed_tag, 0, sizeof(computed_tag));
    xy_memset(&chacha_ctx, 0, sizeof(chacha_ctx));
    xy_memset(&poly_ctx, 0, sizeof(poly_ctx));

    return ret;
}
