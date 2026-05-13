#include <stdint.h>
#include <string.h>
#include "xy_tiny_crypto.h"
#include <string.h>

// MD5 常量
static const uint32_t md5_k[64] = {
    0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a,
    0xa8304613, 0xfd469501, 0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
    0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821, 0xf61e2562, 0xc040b340,
    0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
    0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8,
    0x676f02d9, 0x8d2a4c8a, 0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
    0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70, 0x289b7ec6, 0xeaa127fa,
    0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
    0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92,
    0xffeff47d, 0x85845dd1, 0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
    0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
};

static const int md5_s[64] = { 7,  12, 17, 22, 7,  12, 17, 22, 7,  12, 17,
                               22, 7,  12, 17, 22, 5,  9,  14, 20, 5,  9,
                               14, 20, 5,  9,  14, 20, 5,  9,  14, 20, 4,
                               11, 16, 23, 4,  11, 16, 23, 4,  11, 16, 23,
                               4,  11, 16, 23, 6,  10, 15, 21, 6,  10, 15,
                               21, 6,  10, 15, 21, 6,  10, 15, 21 };

#define MD5_F(x, y, z) (((x) & (y)) | ((~(x)) & (z)))
#define MD5_G(x, y, z) (((x) & (z)) | ((y) & (~(z))))
#define MD5_H(x, y, z) ((x) ^ (y) ^ (z))
#define MD5_I(x, y, z) ((y) ^ ((x) | (~(z))))

#define MD5_ROTLEFT(a, b) (((a) << (b)) | ((a) >> (32 - (b))))

static void md5_transform(xy_md5_ctx_t *ctx, const uint8_t block[64])
{
    uint32_t a = ctx->state[0];
    uint32_t b = ctx->state[1];
    uint32_t c = ctx->state[2];
    uint32_t d = ctx->state[3];
    uint32_t m[16];
    int i;

    // 将字节转换为32位字
    for (i = 0; i < 16; i++) {
        m[i] = (uint32_t)block[i * 4] | (uint32_t)block[i * 4 + 1] << 8
               | (uint32_t)block[i * 4 + 2] << 16
               | (uint32_t)block[i * 4 + 3] << 24;
    }

    // 主循环
    for (i = 0; i < 64; i++) {
        uint32_t f, g;
        if (i < 16) {
            f = MD5_F(b, c, d);
            g = i;
        } else if (i < 32) {
            f = MD5_G(b, c, d);
            g = (5 * i + 1) % 16;
        } else if (i < 48) {
            f = MD5_H(b, c, d);
            g = (3 * i + 5) % 16;
        } else {
            f = MD5_I(b, c, d);
            g = (7 * i) % 16;
        }

        f = f + a + md5_k[i] + m[g];
        a = d;
        d = c;
        c = b;
        b = b + MD5_ROTLEFT(f, md5_s[i]);
    }

    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
}

int xy_md5_init(xy_md5_ctx_t *ctx)
{
    if (!ctx)
        return XY_CRYPTO_INVALID_PARAM;

    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xefcdab89;
    ctx->state[2] = 0x98badcfe;
    ctx->state[3] = 0x10325476;
    ctx->count    = 0;
    memset(ctx->buffer, 0, XY_MD5_BLOCK_SIZE);

    return XY_CRYPTO_SUCCESS;
}

int xy_md5_update(xy_md5_ctx_t *ctx, const uint8_t *data, size_t len)
{
    if (!ctx || !data)
        return XY_CRYPTO_INVALID_PARAM;

    size_t buffer_pos = (size_t)(ctx->count % XY_MD5_BLOCK_SIZE);
    ctx->count += len;

    while (len > 0) {
        size_t copy_len = XY_MD5_BLOCK_SIZE - buffer_pos;
        if (copy_len > len)
            copy_len = len;

        memcpy(ctx->buffer + buffer_pos, data, copy_len);
        data += copy_len;
        len -= copy_len;
        buffer_pos += copy_len;

        if (buffer_pos == XY_MD5_BLOCK_SIZE) {
            md5_transform(ctx, ctx->buffer);
            buffer_pos = 0;
        }
    }

    return XY_CRYPTO_SUCCESS;
}

int xy_md5_final(xy_md5_ctx_t *ctx, uint8_t digest[XY_MD5_DIGEST_SIZE])
{
    if (!ctx || !digest)
        return XY_CRYPTO_INVALID_PARAM;

    size_t buffer_pos  = (size_t)(ctx->count % XY_MD5_BLOCK_SIZE);
    uint64_t bit_count = ctx->count * 8;

    // 添加填充
    ctx->buffer[buffer_pos++] = 0x80;

    if (buffer_pos > 56) {
        memset(ctx->buffer + buffer_pos, 0, XY_MD5_BLOCK_SIZE - buffer_pos);
        md5_transform(ctx, ctx->buffer);
        buffer_pos = 0;
    }

    memset(ctx->buffer + buffer_pos, 0, 56 - buffer_pos);

    // 添加长度
    for (int i = 0; i < 8; i++) {
        ctx->buffer[56 + i] = (uint8_t)(bit_count >> (i * 8));
    }

    md5_transform(ctx, ctx->buffer);

    // 输出结果
    for (int i = 0; i < 4; i++) {
        digest[i * 4]     = (uint8_t)(ctx->state[i]);
        digest[i * 4 + 1] = (uint8_t)(ctx->state[i] >> 8);
        digest[i * 4 + 2] = (uint8_t)(ctx->state[i] >> 16);
        digest[i * 4 + 3] = (uint8_t)(ctx->state[i] >> 24);
    }

    return XY_CRYPTO_SUCCESS;
}

int xy_md5_hash(const uint8_t *data, size_t len,
                uint8_t digest[XY_MD5_DIGEST_SIZE])
{
    xy_md5_ctx_t ctx;
    int ret;

    if ((ret = xy_md5_init(&ctx)) != XY_CRYPTO_SUCCESS)
        return ret;
    if ((ret = xy_md5_update(&ctx, data, len)) != XY_CRYPTO_SUCCESS)
        return ret;
    if ((ret = xy_md5_final(&ctx, digest)) != XY_CRYPTO_SUCCESS)
        return ret;

    return XY_CRYPTO_SUCCESS;
}