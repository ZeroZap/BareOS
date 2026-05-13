/**
 * @file xy_chacha20poly1305.h
 * @brief ChaCha20-Poly1305 AEAD Implementation
 * @version 1.0.0
 * @date 2026-03-02
 */

#ifndef XY_CHACHA20POLY1305_H
#define XY_CHACHA20POLY1305_H

#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ChaCha20 上下文
 */
typedef struct {
    uint32_t state[16];
} xy_chacha20_ctx_t;

/**
 * @brief Poly1305 上下文
 */
typedef struct {
    uint32_t r[4];
    uint32_t s[4];
    uint32_t acc[5];
    uint8_t buffer[16];
    size_t buffer_len;
} xy_poly1305_ctx_t;

/**
 * @brief ChaCha20 初始化
 * @param ctx ChaCha20 上下文
 * @param key 256 位密钥
 * @param nonce 96 位 Nonce
 */
void xy_chacha20_init(xy_chacha20_ctx_t *ctx, const uint8_t *key, const uint8_t *nonce);

/**
 * @brief ChaCha20 加密/解密
 * @param ctx ChaCha20 上下文
 * @param input 输入数据
 * @param output 输出数据
 * @param len 数据长度
 */
void xy_chacha20_encrypt(xy_chacha20_ctx_t *ctx, const uint8_t *input, 
                         uint8_t *output, size_t len);

#define xy_chacha20_decrypt xy_chacha20_encrypt

/**
 * @brief Poly1305 初始化
 * @param ctx Poly1305 上下文
 * @param key 256 位密钥
 */
void xy_poly1305_init(xy_poly1305_ctx_t *ctx, const uint8_t *key);

/**
 * @brief Poly1305 更新
 * @param ctx Poly1305 上下文
 * @param data 数据
 * @param len 数据长度
 */
void xy_poly1305_update(xy_poly1305_ctx_t *ctx, const uint8_t *data, size_t len);

/**
 * @brief Poly1305 完成
 * @param ctx Poly1305 上下文
 * @param tag 输出 128 位 Tag
 */
void xy_poly1305_finish(xy_poly1305_ctx_t *ctx, uint8_t *tag);

/**
 * @brief ChaCha20-Poly1305 AEAD 加密
 * @param key 256 位密钥
 * @param nonce 96 位 Nonce
 * @param aad 附加认证数据
 * @param aad_len AAD 长度
 * @param plaintext 明文
 * @param pt_len 明文长度
 * @param ciphertext 密文输出 (包含 Tag)
 * @param ct_len 密文长度指针
 * @return 0 成功，-1 失败
 */
int xy_chacha20poly1305_encrypt(const uint8_t *key, const uint8_t *nonce,
                                const uint8_t *aad, size_t aad_len,
                                const uint8_t *plaintext, size_t pt_len,
                                uint8_t *ciphertext, size_t *ct_len);

/**
 * @brief ChaCha20-Poly1305 AEAD 解密
 * @param key 256 位密钥
 * @param nonce 96 位 Nonce
 * @param aad 附加认证数据
 * @param aad_len AAD 长度
 * @param ciphertext 密文 (包含 Tag)
 * @param ct_len 密文长度
 * @param plaintext 明文输出
 * @param pt_len 明文长度指针
 * @return 0 成功，-1 失败 (验证失败)
 */
int xy_chacha20poly1305_decrypt(const uint8_t *key, const uint8_t *nonce,
                                const uint8_t *aad, size_t aad_len,
                                const uint8_t *ciphertext, size_t ct_len,
                                uint8_t *plaintext, size_t *pt_len);

#ifdef __cplusplus
}
#endif

#endif
