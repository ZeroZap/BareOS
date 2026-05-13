/**
 * @file xy_sm2.h
 * @brief SM2 Elliptic Curve Digital Signature (GM/T 0003-2010)
 * @version 1.0.0
 * @date 2026-04-30
 * 
 * Note: This is a simplified implementation. For production use,
 *       consider using certified libraries for commercial crypto.
 */

#ifndef XY_SM2_H
#define XY_SM2_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== SM2 Curve Constants ==================== */
/* SM2 recommended curve: y^2 = x^3 + ax + b (mod p) */

#define XY_SM2_PUBLIC_KEY_SIZE   64    /* 512 bits (2 x 32) */
#define XY_SM2_PRIVATE_KEY_SIZE  32    /* 256 bits */
#define XY_SM2_SIGNATURE_SIZE    64    /* 512 bits (2 x 32) */
#define XY_SM2_ID_SIZE_MAX       255    /* Max user ID length */

/* ==================== SM2 Context ==================== */

typedef struct {
    uint8_t private_key[32];    /* d: private key */
    uint8_t public_key[64];     /* (x, y): public key */
} xy_sm2_ctx_t;

/* ==================== SM2 API: Key Generation ==================== */

/**
 * @brief Generate SM2 key pair
 * @param ctx SM2 context (output)
 * @return 0 on success
 */
int xy_sm2_generate_key(xy_sm2_ctx_t *ctx);

/**
 * @brief Set private key (derive public key)
 * @param ctx SM2 context
 * @param private_key 32-byte private key
 * @return 0 on success
 */
int xy_sm2_set_private_key(xy_sm2_ctx_t *ctx, 
                        const uint8_t private_key[XY_SM2_PRIVATE_KEY_SIZE]);

/**
 * @brief Set public key
 * @param ctx SM2 context
 * @param public_key 64-byte public key (x || y)
 * @return 0 on success
 */
int xy_sm2_set_public_key(xy_sm2_ctx_t *ctx,
                         const uint8_t public_key[XY_SM2_PUBLIC_KEY_SIZE]);

/* ==================== SM2 API: Sign/Verify ==================== */

/**
 * @brief SM2 sign (ECDSA-like)
 * @param ctx SM2 context with private key
 * @param id user ID (optional, can be NULL)
 * @param id_len user ID length
 * @param message message to sign
 * @param msg_len message length
 * @param signature output (64 bytes: r || s)
 * @return 0 on success
 */
int xy_sm2_sign(xy_sm2_ctx_t *ctx,
               const uint8_t *id, size_t id_len,
               const uint8_t *message, size_t msg_len,
               uint8_t signature[XY_SM2_SIGNATURE_SIZE]);

/**
 * @brief SM2 verify
 * @param ctx SM2 context with public key
 * @param id user ID (optional, can be NULL)
 * @param id_len user ID length
 * @param message message
 * @param msg_len message length
 * @param signature signature to verify (64 bytes)
 * @return 0 on success, negative on error
 */
int xy_sm2_verify(xy_sm2_ctx_t *ctx,
                  const uint8_t *id, size_t id_len,
                  const uint8_t *message, size_t msg_len,
                  const uint8_t signature[XY_SM2_SIGNATURE_SIZE]);

/* ==================== SM2 API: Key Exchange ==================== */

/**
 * @brief SM2 key exchange (ECIES-like, simplified)
 * @param ctx SM2 context
 * @param peer_public_key peer's public key (64 bytes)
 * @param plaintext input data
 * @param plain_len data length
 * @param ciphertext output
 * @param cipher_len output length (on input: max, on output: actual)
 * @return 0 on success
 */
int xy_sm2_encrypt(xy_sm2_ctx_t *ctx,
                   const uint8_t peer_public_key[XY_SM2_PUBLIC_KEY_SIZE],
                   const uint8_t *plaintext, size_t plain_len,
                   uint8_t *ciphertext, size_t *cipher_len);

/**
 * @brief SM2 key exchange decrypt
 * @param ctx SM2 context (must have private key)
 * @param peer_public_key peer's public key (64 bytes)
 * @param ciphertext encrypted data
 * @param cipher_len ciphertext length
 * @param plaintext output
 * @param plain_len output length (on input: max, on output: actual)
 * @return 0 on success
 */
int xy_sm2_decrypt(xy_sm2_ctx_t *ctx,
                    const uint8_t peer_public_key[XY_SM2_PUBLIC_KEY_SIZE],
                    const uint8_t *ciphertext, size_t cipher_len,
                    uint8_t *plaintext, size_t *plain_len);

/* ==================== SM2 Encryption API ==================== */

/**
 * @brief SM2 encrypt (public key encryption)
 * @param ctx SM2 context (with public key)
 * @param plaintext input
 * @param plain_len length
 * @param ciphertext output (97 bytes + plaintext)
 * @return 0 on success
 */
int xy_sm2_ecdh_encrypt(xy_sm2_ctx_t *ctx,
                        const uint8_t *plaintext, size_t plain_len,
                        uint8_t *ciphertext);

/**
 * @brief SM2 decrypt (private key decryption)
 * @param ctx SM2 context (with private key)
 * @param ciphertext encrypted data
 * @param cipher_len length
 * @param plaintext output
 * @param plain_len output length
 * @return 0 on success
 */
int xy_sm2_ecdh_decrypt(xy_sm2_ctx_t *ctx,
                        const uint8_t *ciphertext, size_t cipher_len,
                        uint8_t *plaintext, size_t *plain_len);

#ifdef __cplusplus
}
#endif

#endif /* XY_SM2_H */