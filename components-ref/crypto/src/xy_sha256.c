/**
 * @file xy_sha256.c
 * @brief SHA256 Hash Implementation
 * @version 1.0.0
 * @date 2026-03-02
 */

#include "xy_sha256.h"
#include <string.h>

/**
 * @brief SHA256 常量
 */
static const uint32_t g_sha256_k[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

/**
 * @brief 循环右移
 */
#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))

/**
 * @brief SHA256 函数
 */
#define CH(x, y, z)  (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x)       (ROTR(x, 2)  ^ ROTR(x, 13) ^ ROTR(x, 22))
#define EP1(x)       (ROTR(x, 6)  ^ ROTR(x, 11) ^ ROTR(x, 25))
#define SIG0(x)      (ROTR(x, 7)  ^ ROTR(x, 18) ^ ((x) >> 3))
#define SIG1(x)      (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

/**
 * @brief 初始化 SHA256
 */
void xy_sha256_init(xy_sha256_ctx_t *ctx)
{
    ctx->h[0] = 0x6a09e667;
    ctx->h[1] = 0xbb67ae85;
    ctx->h[2] = 0x3c6ef372;
    ctx->h[3] = 0xa54ff53a;
    ctx->h[4] = 0x510e527f;
    ctx->h[5] = 0x9b05688c;
    ctx->h[6] = 0x1f83d9ab;
    ctx->h[7] = 0x5be0cd19;
    
    ctx->total = 0;
    ctx->buflen = 0;
}

/**
 * @brief SHA256 块处理
 */
static void sha256_process_block(xy_sha256_ctx_t *ctx, const uint8_t *data)
{
    uint32_t w[64];
    uint32_t a, b, c, d, e, f, g, h;
    int i;
    
    /* 准备消息调度 */
    for (i = 0; i < 16; i++) {
        w[i] = ((uint32_t)data[i * 4] << 24) |
               ((uint32_t)data[i * 4 + 1] << 16) |
               ((uint32_t)data[i * 4 + 2] << 8) |
               ((uint32_t)data[i * 4 + 3]);
    }
    
    for (i = 16; i < 64; i++) {
        w[i] = SIG1(w[i - 2]) + w[i - 7] + SIG0(w[i - 15]) + w[i - 16];
    }
    
    /* 初始化工作变量 */
    a = ctx->h[0];
    b = ctx->h[1];
    c = ctx->h[2];
    d = ctx->h[3];
    e = ctx->h[4];
    f = ctx->h[5];
    g = ctx->h[6];
    h = ctx->h[7];
    
    /* 主循环 */
    for (i = 0; i < 64; i++) {
        uint32_t t1 = h + EP1(e) + CH(e, f, g) + g_sha256_k[i] + w[i];
        uint32_t t2 = EP0(a) + MAJ(a, b, c);
        
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }
    
    /* 更新哈希值 */
    ctx->h[0] += a;
    ctx->h[1] += b;
    ctx->h[2] += c;
    ctx->h[3] += d;
    ctx->h[4] += e;
    ctx->h[5] += f;
    ctx->h[6] += g;
    ctx->h[7] += h;
}

/**
 * @brief SHA256 更新
 */
void xy_sha256_update(xy_sha256_ctx_t *ctx, const uint8_t *data, size_t len)
{
    size_t i;
    size_t index = ctx->buflen;
    
    ctx->total += len;
    
    /* 如果缓冲区有数据，先填充到 64 字节 */
    if (index > 0) {
        size_t left = 64 - index;
        if (len < left) {
            memcpy(ctx->buffer + index, data, len);
            ctx->buflen += len;
            return;
        }
        
        memcpy(ctx->buffer + index, data, left);
        sha256_process_block(ctx, ctx->buffer);
        data += left;
        len -= left;
    }
    
    /* 处理完整的 64 字节块 */
    while (len >= 64) {
        sha256_process_block(ctx, data);
        data += 64;
        len -= 64;
    }
    
    /* 剩余数据放入缓冲区 */
    if (len > 0) {
        memcpy(ctx->buffer, data, len);
        ctx->buflen = len;
    }
}

/**
 * @brief SHA256 完成
 */
void xy_sha256_finish(xy_sha256_ctx_t *ctx, uint8_t *hash)
{
    uint8_t zero = 0x00;
    uint8_t pad0 = 0x80;
    uint8_t pad1[64];
    uint32_t i;
    uint64_t bits;
    
    /* 添加填充位 */
    bits = ctx->total * 8;
    
    memset(pad1, 0, sizeof(pad1));
    pad1[56] = (bits >> 56) & 0xff;
    pad1[57] = (bits >> 48) & 0xff;
    pad1[58] = (bits >> 40) & 0xff;
    pad1[59] = (bits >> 32) & 0xff;
    pad1[60] = (bits >> 24) & 0xff;
    pad1[61] = (bits >> 16) & 0xff;
    pad1[62] = (bits >> 8) & 0xff;
    pad1[63] = bits & 0xff;
    
    /* 处理剩余数据 */
    if (ctx->buflen < 56) {
        xy_sha256_update(ctx, &pad0, 1);
        for (i = ctx->buflen; i < 56; i++) {
            xy_sha256_update(ctx, &zero, 1);
        }
        xy_sha256_update(ctx, pad1, 8);
    } else {
        xy_sha256_update(ctx, &pad0, 1);
        for (i = ctx->buflen; i < 64; i++) {
            xy_sha256_update(ctx, &zero, 1);
        }
        sha256_process_block(ctx, ctx->buffer);
        
        xy_sha256_update(ctx, pad1, 64);
    }
    
    /* 输出哈希值 (大端) */
    for (i = 0; i < 8; i++) {
        hash[i * 4] = (ctx->h[i] >> 24) & 0xff;
        hash[i * 4 + 1] = (ctx->h[i] >> 16) & 0xff;
        hash[i * 4 + 2] = (ctx->h[i] >> 8) & 0xff;
        hash[i * 4 + 3] = ctx->h[i] & 0xff;
    }
}

/**
 * @brief 计算 SHA256 哈希
 */
int xy_sha256(const uint8_t *data, size_t len, uint8_t *hash)
{
    xy_sha256_ctx_t ctx;
    
    if (!data || !hash) {
        return -1;
    }
    
    xy_sha256_init(&ctx);
    xy_sha256_update(&ctx, data, len);
    xy_sha256_finish(&ctx, hash);
    
    return 0;
}
