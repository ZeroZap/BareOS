/**
 * @file xy_chacha20_poly1305.h
 * @brief ChaCha20-Poly1305 AEAD cipher (RFC 8439)
 * @version 1.0.0
 * @date 2025-11-02
 *
 * This module implements ChaCha20 stream cipher, Poly1305 MAC, and their
 * combined AEAD (Authenticated Encryption with Associated Data) construction
 * as specified in RFC 8439.
 */

#ifndef XY_CHACHA20_POLY1305_H
#define XY_CHACHA20_POLY1305_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== ChaCha20 Constants ==================== */

/**
 * @brief ChaCha20 key size in bytes
 */
#define XY_CHACHA20_KEY_SIZE 32

/**
 * @brief ChaCha20 nonce size in bytes
 */
#define XY_CHACHA20_NONCE_SIZE 12

/**
 * @brief ChaCha20 block size in bytes
 */
#define XY_CHACHA20_BLOCK_SIZE 64

/* ==================== Poly1305 Constants ==================== */

/**
 * @brief Poly1305 key size in bytes
 */
#define XY_POLY1305_KEY_SIZE 32

/**
 * @brief Poly1305 tag/MAC size in bytes
 */
#define XY_POLY1305_TAG_SIZE 16

/* ==================== ChaCha20-Poly1305 AEAD Constants ==================== */

/**
 * @brief AEAD key size in bytes
 */
#define XY_CHACHA20_POLY1305_KEY_SIZE 32

/**
 * @brief AEAD nonce size in bytes
 */
#define XY_CHACHA20_POLY1305_NONCE_SIZE 12

/**
 * @brief AEAD authentication tag size in bytes
 */
#define XY_CHACHA20_POLY1305_TAG_SIZE 16

/* ==================== Error Codes ==================== */

/**
 * @brief Operation successful
 */
#define XY_CHACHA20_POLY1305_SUCCESS 0

/**
 * @brief Invalid parameter (NULL pointer or invalid value)
 */
#define XY_CHACHA20_POLY1305_ERROR_INVALID_PARAM -1

/**
 * @brief Authentication verification failed
 */
#define XY_CHACHA20_POLY1305_ERROR_AUTH_FAILED -2

/**
 * @brief General error
 */
#define XY_CHACHA20_POLY1305_ERROR -3

/* ==================== ChaCha20 Stream Cipher ==================== */

/**
 * @brief ChaCha20 context structure
 */
typedef struct {
    uint32_t state[16];      /**< Internal state (512 bits) */
    uint32_t counter;        /**< Block counter */
    uint8_t keystream[64];   /**< Current keystream block */
    size_t keystream_pos;    /**< Position in keystream */
} xy_chacha20_ctx_t;

/**
 * @brief Initialize ChaCha20 cipher
 *
 * @param ctx ChaCha20 context to initialize
 * @param key 32-byte encryption key
 * @param nonce 12-byte nonce
 * @param counter Initial block counter (usually 0 or 1)
 * @return XY_CHACHA20_POLY1305_SUCCESS on success, error code otherwise
 */
int xy_chacha20_init(xy_chacha20_ctx_t *ctx,
                      const uint8_t key[XY_CHACHA20_KEY_SIZE],
                      const uint8_t nonce[XY_CHACHA20_NONCE_SIZE],
                      uint32_t counter);

/**
 * @brief Encrypt/decrypt data with ChaCha20
 *
 * ChaCha20 is symmetric, so the same function is used for both operations.
 *
 * @param ctx ChaCha20 context (initialized)
 * @param output Output buffer (can be same as input for in-place operation)
 * @param input Input data
 * @param length Number of bytes to process
 * @return XY_CHACHA20_POLY1305_SUCCESS on success, error code otherwise
 */
int xy_chacha20_crypt(xy_chacha20_ctx_t *ctx,
                       uint8_t *output,
                       const uint8_t *input,
                       size_t length);

/* ==================== Poly1305 MAC ==================== */

/**
 * @brief Poly1305 context structure
 */
typedef struct {
    uint32_t r[5];          /**< Clamped key (r) */
    uint32_t h[5];          /**< Accumulator */
    uint32_t s[4];          /**< Secret (s) */
    uint8_t buffer[16];     /**< Message buffer */
    size_t buffer_len;      /**< Bytes in buffer */
} xy_poly1305_ctx_t;

/**
 * @brief Initialize Poly1305 MAC
 *
 * @param ctx Poly1305 context to initialize
 * @param key 32-byte Poly1305 key
 * @return XY_CHACHA20_POLY1305_SUCCESS on success, error code otherwise
 */
int xy_poly1305_init(xy_poly1305_ctx_t *ctx,
                      const uint8_t key[XY_POLY1305_KEY_SIZE]);

/**
 * @brief Update Poly1305 MAC with more data
 *
 * @param ctx Poly1305 context
 * @param data Data to authenticate
 * @param length Number of bytes
 * @return XY_CHACHA20_POLY1305_SUCCESS on success, error code otherwise
 */
int xy_poly1305_update(xy_poly1305_ctx_t *ctx,
                        const uint8_t *data,
                        size_t length);

/**
 * @brief Finalize Poly1305 MAC and output tag
 *
 * @param ctx Poly1305 context
 * @param tag Output buffer for 16-byte authentication tag
 * @return XY_CHACHA20_POLY1305_SUCCESS on success, error code otherwise
 */
int xy_poly1305_finish(xy_poly1305_ctx_t *ctx,
                        uint8_t tag[XY_POLY1305_TAG_SIZE]);

/* ==================== ChaCha20-Poly1305 AEAD ==================== */

/**
 * @brief Encrypt data with ChaCha20-Poly1305 AEAD
 *
 * Encrypts plaintext and generates an authentication tag over both the
 * ciphertext and associated data (AAD). This provides confidentiality
 * and authenticity.
 *
 * @param key 32-byte encryption key
 * @param nonce 12-byte nonce (must be unique for each encryption with same key)
 * @param aad Associated data (authenticated but not encrypted), can be NULL
 * @param aad_len Length of associated data
 * @param plaintext Data to encrypt
 * @param plaintext_len Length of plaintext
 * @param ciphertext Output buffer for encrypted data (same size as plaintext)
 * @param tag Output buffer for 16-byte authentication tag
 * @return XY_CHACHA20_POLY1305_SUCCESS on success, error code otherwise
 */
int xy_chacha20_poly1305_encrypt(
    const uint8_t key[XY_CHACHA20_POLY1305_KEY_SIZE],
    const uint8_t nonce[XY_CHACHA20_POLY1305_NONCE_SIZE],
    const uint8_t *aad,
    size_t aad_len,
    const uint8_t *plaintext,
    size_t plaintext_len,
    uint8_t *ciphertext,
    uint8_t tag[XY_CHACHA20_POLY1305_TAG_SIZE]);

/**
 * @brief Decrypt data with ChaCha20-Poly1305 AEAD
 *
 * Verifies the authentication tag, then decrypts the ciphertext if valid.
 * If verification fails, no plaintext is output.
 *
 * @param key 32-byte encryption key
 * @param nonce 12-byte nonce (same as used for encryption)
 * @param aad Associated data (same as used for encryption), can be NULL
 * @param aad_len Length of associated data
 * @param ciphertext Encrypted data
 * @param ciphertext_len Length of ciphertext
 * @param tag 16-byte authentication tag from encryption
 * @param plaintext Output buffer for decrypted data (same size as ciphertext)
 * @return XY_CHACHA20_POLY1305_SUCCESS if valid,
 *         XY_CHACHA20_POLY1305_ERROR_AUTH_FAILED if tag verification fails
 */
int xy_chacha20_poly1305_decrypt(
    const uint8_t key[XY_CHACHA20_POLY1305_KEY_SIZE],
    const uint8_t nonce[XY_CHACHA20_POLY1305_NONCE_SIZE],
    const uint8_t *aad,
    size_t aad_len,
    const uint8_t *ciphertext,
    size_t ciphertext_len,
    const uint8_t tag[XY_CHACHA20_POLY1305_TAG_SIZE],
    uint8_t *plaintext);

#ifdef __cplusplus
}
#endif

#endif /* XY_CHACHA20_POLY1305_H */
