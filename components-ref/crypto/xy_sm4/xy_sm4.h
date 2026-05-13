/**
 * @file xy_sm4.h
 * @brief SM4 Block Cipher (GM/T 0002-2010)
 * @version 1.0.0
 * @date 2026-04-30
 */

#ifndef XY_SM4_H
#define XY_SM4_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== SM4 Constants ==================== */

#define XY_SM4_KEY_SIZE     16    /* 128 bits */
#define XY_SM4_BLOCK_SIZE  16    /* 128 bits */

/* ==================== SM4 Context ==================== */

typedef struct {
    uint32_t rk[32];    /* 32 round keys */
} xy_sm4_ctx_t;

/* ==================== SM4 API ==================== */

/**
 * @brief Initialize SM4 context with key
 * @param ctx SM4 context
 * @param key 128-bit key (16 bytes)
 * @return 0 on success
 */
int xy_sm4_init(xy_sm4_ctx_t *ctx, const uint8_t key[XY_SM4_KEY_SIZE]);

/**
 * @brief Encrypt one block (ECB mode)
 * @param ctx SM4 context
 * @param plaintext input (16 bytes)
 * @param ciphertext output (16 bytes)
 * @return 0 on success
 */
int xy_sm4_encrypt_block(xy_sm4_ctx_t *ctx, 
                       const uint8_t plaintext[XY_SM4_BLOCK_SIZE],
                       uint8_t ciphertext[XY_SM4_BLOCK_SIZE]);

/**
 * @brief Decrypt one block (ECB mode)
 * @param ctx SM4 context
 * @param ciphertext input (16 bytes)
 * @param plaintext output (16 bytes)
 * @return 0 on success
 */
int xy_sm4_decrypt_block(xy_sm4_ctx_t *ctx,
                        const uint8_t ciphertext[XY_SM4_BLOCK_SIZE],
                        uint8_t plaintext[XY_SM4_BLOCK_SIZE]);

/**
 * @brief CBC encrypt
 * @param ctx SM4 context
 * @param iv initialization vector (16 bytes, updated in place)
 * @param plaintext input data
 * @param len data length (multiple of 16)
 * @param ciphertext output
 * @return 0 on success
 */
int xy_sm4_cbc_encrypt(xy_sm4_ctx_t *ctx, uint8_t iv[XY_SM4_BLOCK_SIZE],
                      const uint8_t *plaintext, size_t len,
                      uint8_t *ciphertext);

/**
 * @brief CBC decrypt
 * @param ctx SM4 context
 * @param iv initialization vector (16 bytes, updated in place)
 * @param ciphertext input data
 * @param len data length (multiple of 16)
 * @param plaintext output
 * @return 0 on success
 */
int xy_sm4_cbc_decrypt(xy_sm4_ctx_t *ctx, uint8_t iv[XY_SM4_BLOCK_SIZE],
                       const uint8_t *ciphertext, size_t len,
                       uint8_t *plaintext);

/**
 * @brief SM4-GCM authenticated encryption (optional, basic version)
 * Note: Full GCM requires additional GMAC implementation
 */
int xy_sm4_gcm_encrypt(xy_sm4_ctx_t *ctx,
                       const uint8_t iv[12],
                       const uint8_t *aad, size_t aad_len,
                       const uint8_t *plaintext, size_t len,
                       uint8_t *ciphertext,
                       uint8_t tag[16]);

#ifdef __cplusplus
}
#endif

#endif /* XY_SM4_H */