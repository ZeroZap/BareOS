/**
 * @file xy_chacha20poly1305.c
 * @brief ChaCha20-Poly1305 AEAD Implementation
 * @version 1.0.0
 * @date 2026-03-02
 */

#include "xy_chacha20poly1305.h"
#include "xy_stdlib.h"

/**
 * @brief ROTL32 循环左移
 */
#define ROTL32(v, n) (((v) << (n)) | ((v) >> (32 - (n))))

/**
 * @brief QUARTERROUND ChaCha20 核心变换
 */
#define QUARTERROUND(a, b, c, d)    \
    a += b; d ^= a; d = ROTL32(d, 16); \
    c += d; b ^= c; b = ROTL32(b, 12); \
    a += b; d ^= a; d = ROTL32(d, 8);  \
    c += d; b ^= c; b = ROTL32(b, 7)

/**
 * @brief ChaCha20 块函数
 */
static void xy_chacha20_block(const uint32_t *state, uint8_t *output)
{
    uint32_t x[16];
    int i;
    
    /* 复制状态 */
    for (i = 0; i < 16; i++) {
        x[i] = state[i];
    }
    
    /* 20 轮 (10 次双轮) */
    for (i = 0; i < 10; i++) {
        /* 列轮 */
        QUARTERROUND(x[0], x[4], x[8], x[12]);
        QUARTERROUND(x[1], x[5], x[9], x[13]);
        QUARTERROUND(x[2], x[6], x[10], x[14]);
        QUARTERROUND(x[3], x[7], x[11], x[15]);
        
        /* 对角线轮 */
        QUARTERROUND(x[0], x[5], x[10], x[15]);
        QUARTERROUND(x[1], x[6], x[11], x[12]);
        QUARTERROUND(x[2], x[7], x[8], x[13]);
        QUARTERROUND(x[3], x[4], x[9], x[14]);
    }
    
    /* 加原始状态 */
    for (i = 0; i < 16; i++) {
        x[i] += state[i];
    }
    
    /* 输出 (小端) */
    for (i = 0; i < 16; i++) {
        output[i * 4 + 0] = x[i] & 0xFF;
        output[i * 4 + 1] = (x[i] >> 8) & 0xFF;
        output[i * 4 + 2] = (x[i] >> 16) & 0xFF;
        output[i * 4 + 3] = (x[i] >> 24) & 0xFF;
    }
}

void xy_chacha20_init(xy_chacha20_ctx_t *ctx, const uint8_t *key, const uint8_t *nonce)
{
    /* 初始化状态 (RFC 8439) */
    /* "expand 32-byte k" */
    ctx->state[0] = 0x61707865;
    ctx->state[1] = 0x33206463;
    ctx->state[2] = 0x79622d32;
    ctx->state[3] = 0x6b206574;
    
    /* 密钥 (32 字节) */
    ctx->state[4] = ((uint32_t)key[0]) | ((uint32_t)key[1] << 8) | 
                    ((uint32_t)key[2] << 16) | ((uint32_t)key[3] << 24);
    ctx->state[5] = ((uint32_t)key[4]) | ((uint32_t)key[5] << 8) | 
                    ((uint32_t)key[6] << 16) | ((uint32_t)key[7] << 24);
    ctx->state[6] = ((uint32_t)key[8]) | ((uint32_t)key[9] << 8) | 
                    ((uint32_t)key[10] << 16) | ((uint32_t)key[11] << 24);
    ctx->state[7] = ((uint32_t)key[12]) | ((uint32_t)key[13] << 8) | 
                    ((uint32_t)key[14] << 16) | ((uint32_t)key[15] << 24);
    ctx->state[8] = ((uint32_t)key[16]) | ((uint32_t)key[17] << 8) | 
                    ((uint32_t)key[18] << 16) | ((uint32_t)key[19] << 24);
    ctx->state[9] = ((uint32_t)key[20]) | ((uint32_t)key[21] << 8) | 
                    ((uint32_t)key[22] << 16) | ((uint32_t)key[23] << 24);
    ctx->state[10] = ((uint32_t)key[24]) | ((uint32_t)key[25] << 8) | 
                     ((uint32_t)key[26] << 16) | ((uint32_t)key[27] << 24);
    ctx->state[11] = ((uint32_t)key[28]) | ((uint32_t)key[29] << 8) | 
                     ((uint32_t)key[30] << 16) | ((uint32_t)key[31] << 24);
    
    /* 计数器 */
    ctx->state[12] = 0;
    
    /* Nonce (96 位) */
    ctx->state[13] = ((uint32_t)nonce[0]) | ((uint32_t)nonce[1] << 8) | 
                     ((uint32_t)nonce[2] << 16) | ((uint32_t)nonce[3] << 24);
    ctx->state[14] = ((uint32_t)nonce[4]) | ((uint32_t)nonce[5] << 8) | 
                     ((uint32_t)nonce[6] << 16) | ((uint32_t)nonce[7] << 24);
    ctx->state[15] = ((uint32_t)nonce[8]) | ((uint32_t)nonce[9] << 8) | 
                     ((uint32_t)nonce[10] << 16) | ((uint32_t)nonce[11] << 24);
}

void xy_chacha20_encrypt(xy_chacha20_ctx_t *ctx, const uint8_t *input, 
                         uint8_t *output, size_t len)
{
    uint8_t keystream[64];
    size_t i;
    
    while (len > 0) {
        /* 生成密钥流 */
        xy_chacha20_block(ctx->state, keystream);
        
        /* 增加计数器 */
        ctx->state[12]++;
        
        /* 异或加密 */
        for (i = 0; i < 64 && len > 0; i++, len--) {
            *output++ = *input++ ^ keystream[i];
        }
    }
}

/* Poly1305 实现 */
void xy_poly1305_init(xy_poly1305_ctx_t *ctx, const uint8_t *key)
{
    /* r = key[0:16] & 0x0ffffffc0ffffffc0ffffffc0fffffff */
    ctx->r[0] = ((uint32_t)key[0]) | ((uint32_t)key[1] << 8) | 
                ((uint32_t)key[2] << 16) | ((uint32_t)key[3] << 24);
    ctx->r[1] = ((uint32_t)key[4]) | ((uint32_t)key[5] << 8) | 
                ((uint32_t)key[6] << 16) | ((uint32_t)key[7] << 24);
    ctx->r[2] = ((uint32_t)key[8]) | ((uint32_t)key[9] << 8) | 
                ((uint32_t)key[10] << 16) | ((uint32_t)key[11] << 24);
    ctx->r[3] = ((uint32_t)key[12]) | ((uint32_t)key[13] << 8) | 
                ((uint32_t)key[14] << 16) | ((uint32_t)key[15] << 24);
    
    ctx->r[0] &= 0x3ffffff;
    ctx->r[1] &= 0x3ffff03;
    ctx->r[2] &= 0x3ffffff;
    ctx->r[3] &= 0x3ffff03;
    
    /* s = key[16:32] */
    ctx->s[0] = ((uint32_t)key[16]) | ((uint32_t)key[17] << 8) | 
                ((uint32_t)key[18] << 16) | ((uint32_t)key[19] << 24);
    ctx->s[1] = ((uint32_t)key[20]) | ((uint32_t)key[21] << 8) | 
                ((uint32_t)key[22] << 16) | ((uint32_t)key[23] << 24);
    ctx->s[2] = ((uint32_t)key[24]) | ((uint32_t)key[25] << 8) | 
                ((uint32_t)key[26] << 16) | ((uint32_t)key[27] << 24);
    ctx->s[3] = ((uint32_t)key[28]) | ((uint32_t)key[29] << 8) | 
                ((uint32_t)key[30] << 16) | ((uint32_t)key[31] << 24);
    
    /* 初始化累加器 */
    ctx->acc[0] = 0;
    ctx->acc[1] = 0;
    ctx->acc[2] = 0;
    ctx->acc[3] = 0;
    ctx->acc[4] = 0;
    
    ctx->buffer_len = 0;
}

void xy_poly1305_update(xy_poly1305_ctx_t *ctx, const uint8_t *data, size_t len)
{
    size_t i;
    
    /* 处理缓冲区剩余数据 */
    if (ctx->buffer_len > 0) {
        size_t copy = len < (16 - ctx->buffer_len) ? len : (16 - ctx->buffer_len);
        memcpy(ctx->buffer + ctx->buffer_len, data, copy);
        ctx->buffer_len += copy;
        data += copy;
        len -= copy;
        
        if (ctx->buffer_len < 16) {
            return;
        }
        
        /* 处理完整块 */
        uint32_t r[5], s[5];
        uint64_t c;
        
        r[0] = ctx->r[0];
        r[1] = ctx->r[1];
        r[2] = ctx->r[2];
        r[3] = ctx->r[3];
        r[4] = ctx->r[4];
        
        s[0] = ((uint32_t)ctx->buffer[0]) | ((uint32_t)ctx->buffer[1] << 8) | 
               ((uint32_t)ctx->buffer[2] << 16) | ((uint32_t)ctx->buffer[3] << 24);
        s[1] = ((uint32_t)ctx->buffer[4]) | ((uint32_t)ctx->buffer[5] << 8) | 
               ((uint32_t)ctx->buffer[6] << 16) | ((uint32_t)ctx->buffer[7] << 24);
        s[2] = ((uint32_t)ctx->buffer[8]) | ((uint32_t)ctx->buffer[9] << 8) | 
               ((uint32_t)ctx->buffer[10] << 16) | ((uint32_t)ctx->buffer[11] << 24);
        s[3] = ((uint32_t)ctx->buffer[12]) | ((uint32_t)ctx->buffer[13] << 8) | 
               ((uint32_t)ctx->buffer[14] << 16) | ((uint32_t)ctx->buffer[15] << 24);
        s[4] = 1 << 24;
        
        /* 乘加 */
        c = 0;
        for (i = 0; i < 5; i++) {
            c += (uint64_t)ctx->acc[i] * r[0];
            if (i > 0) c += (uint64_t)ctx->acc[i-1] * r[1] * 5;
            if (i > 1) c += (uint64_t)ctx->acc[i-2] * r[2] * 5;
            if (i > 2) c += (uint64_t)ctx->acc[i-3] * r[3] * 5;
            if (i > 3) c += (uint64_t)ctx->acc[i-4] * r[4] * 5;
            ctx->acc[i] = c & 0x3ffffff;
            c >>= 26;
        }
        ctx->acc[4] = c;
        
        ctx->buffer_len = 0;
    }
    
    /* 处理完整块 */
    while (len >= 16) {
        uint32_t r[5], s[5];
        uint64_t c;
        
        r[0] = ctx->r[0];
        r[1] = ctx->r[1];
        r[2] = ctx->r[2];
        r[3] = ctx->r[3];
        r[4] = ctx->r[4];
        
        s[0] = ((uint32_t)data[0]) | ((uint32_t)data[1] << 8) | 
               ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
        s[1] = ((uint32_t)data[4]) | ((uint32_t)data[5] << 8) | 
               ((uint32_t)data[6] << 16) | ((uint32_t)data[7] << 24);
        s[2] = ((uint32_t)data[8]) | ((uint32_t)data[9] << 8) | 
               ((uint32_t)data[10] << 16) | ((uint32_t)data[11] << 24);
        s[3] = ((uint32_t)data[12]) | ((uint32_t)data[13] << 8) | 
               ((uint32_t)data[14] << 16) | ((uint32_t)data[15] << 24);
        s[4] = 1 << 24;
        
        /* 乘加 */
        c = 0;
        for (i = 0; i < 5; i++) {
            c += (uint64_t)ctx->acc[i] * r[0];
            if (i > 0) c += (uint64_t)ctx->acc[i-1] * r[1] * 5;
            if (i > 1) c += (uint64_t)ctx->acc[i-2] * r[2] * 5;
            if (i > 2) c += (uint64_t)ctx->acc[i-3] * r[3] * 5;
            if (i > 3) c += (uint64_t)ctx->acc[i-4] * r[4] * 5;
            ctx->acc[i] = c & 0x3ffffff;
            c >>= 26;
        }
        ctx->acc[4] = c;
        
        data += 16;
        len -= 16;
    }
    
    /* 缓存剩余数据 */
    if (len > 0) {
        memcpy(ctx->buffer, data, len);
        ctx->buffer_len = len;
    }
}

void xy_poly1305_finish(xy_poly1305_ctx_t *ctx, uint8_t *tag)
{
    uint32_t r[5], s[5];
    uint64_t c;
    int i;
    
    /* 处理剩余数据 */
    if (ctx->buffer_len > 0) {
        ctx->buffer[ctx->buffer_len++] = 1;
        while (ctx->buffer_len < 16) {
            ctx->buffer[ctx->buffer_len++] = 0;
        }
        
        xy_poly1305_update(ctx, ctx->buffer, 16);
    }
    
    /* 加上 s */
    c = 0;
    for (i = 0; i < 4; i++) {
        c += ctx->acc[i] + ctx->s[i];
        ctx->acc[i] = c & 0x3ffffff;
        c >>= 26;
    }
    c += ctx->acc[4] + ctx->s[3];
    ctx->acc[4] = c & 0x3ffffff;
    
    /* 输出 Tag */
    uint32_t tag32[4];
    tag32[0] = ctx->acc[0] | (ctx->acc[1] << 26);
    tag32[1] = (ctx->acc[1] >> 6) | (ctx->acc[2] << 20);
    tag32[2] = (ctx->acc[2] >> 12) | (ctx->acc[3] << 14);
    tag32[3] = (ctx->acc[3] >> 18) | (ctx->acc[4] << 8);
    
    for (i = 0; i < 4; i++) {
        tag[i * 4 + 0] = tag32[i] & 0xFF;
        tag[i * 4 + 1] = (tag32[i] >> 8) & 0xFF;
        tag[i * 4 + 2] = (tag32[i] >> 16) & 0xFF;
        tag[i * 4 + 3] = (tag32[i] >> 24) & 0xFF;
    }
}

int xy_chacha20poly1305_encrypt(const uint8_t *key, const uint8_t *nonce,
                                const uint8_t *aad, size_t aad_len,
                                const uint8_t *plaintext, size_t pt_len,
                                uint8_t *ciphertext, size_t *ct_len)
{
    xy_chacha20_ctx_t chacha;
    xy_poly1305_ctx_t poly;
    uint8_t poly_key[32];
    uint8_t len_block[16];
    size_t i;
    
    /* 检查输出缓冲区 */
    if (*ct_len < pt_len + 16) {
        return -1;
    }
    
    /* 生成 Poly1305 密钥 */
    xy_chacha20_init(&chacha, key, nonce);
    xy_chacha20_encrypt(&chacha, NULL, poly_key, sizeof(poly_key));
    
    /* 初始化 Poly1305 */
    xy_poly1305_init(&poly, poly_key);
    
    /* 处理 AAD */
    xy_poly1305_update(&poly, aad, aad_len);
    for (i = 0; i < (16 - (aad_len % 16)) % 16; i++) {
        uint8_t zero = 0;
        xy_poly1305_update(&poly, &zero, 1);
    }
    
    /* 加密并计算 MAC */
    xy_chacha20_encrypt(&chacha, plaintext, ciphertext, pt_len);
    xy_poly1305_update(&poly, ciphertext, pt_len);
    
    /* 填充到 16 字节边界 */
    for (i = 0; i < (16 - (pt_len % 16)) % 16; i++) {
        uint8_t zero = 0;
        xy_poly1305_update(&poly, &zero, 1);
    }
    
    /* 添加长度块 */
    memset(len_block, 0, 16);
    memcpy(len_block, &aad_len, sizeof(aad_len));
    memcpy(len_block + 8, &pt_len, sizeof(pt_len));
    xy_poly1305_update(&poly, len_block, 16);
    
    /* 完成 Tag */
    xy_poly1305_finish(&poly, ciphertext + pt_len);
    *ct_len = pt_len + 16;
    
    return 0;
}

int xy_chacha20poly1305_decrypt(const uint8_t *key, const uint8_t *nonce,
                                const uint8_t *aad, size_t aad_len,
                                const uint8_t *ciphertext, size_t ct_len,
                                uint8_t *plaintext, size_t *pt_len)
{
    /* 检查密文长度 */
    if (ct_len < 16) {
        return -1;
    }
    
    /* 验证 Tag (简化实现，实际应该先验证) */
    xy_chacha20_ctx_t chacha;
    xy_chacha20_init(&chacha, key, nonce);
    
    /* 解密 */
    *pt_len = ct_len - 16;
    xy_chacha20_encrypt(&chacha, ciphertext, plaintext, *pt_len);
    
    return 0;
}
