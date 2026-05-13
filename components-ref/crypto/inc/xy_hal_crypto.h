/**
 * @file xy_hal_crypto.h
 * @brief HAL interface for hardware-accelerated cryptographic operations
 *
 * This file defines the Hardware Abstraction Layer (HAL) interface for
 * cryptographic operations. Implement these functions to use platform-specific
 * hardware acceleration.
 */

#ifndef XY_HAL_CRYPTO_H
#define XY_HAL_CRYPTO_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Return Codes ==================== */
#define XY_HAL_CRYPTO_OK    0
#define XY_HAL_CRYPTO_ERROR -1
#define XY_HAL_CRYPTO_NOT_IMPL \
    -2 /* Hardware not available, fallback to software */

/* ==================== AES HAL Interface ==================== */

/**
 * @brief Initialize AES hardware
 * @return XY_HAL_CRYPTO_OK on success, error code otherwise
 */
int xy_hal_aes_init(void);

/**
 * @brief Deinitialize AES hardware
 * @return XY_HAL_CRYPTO_OK on success, error code otherwise
 */
int xy_hal_aes_deinit(void);

/**
 * @brief Set AES encryption key
 * @param key Pointer to key data
 * @param key_bits Key size in bits (128, 192, or 256)
 * @return XY_HAL_CRYPTO_OK on success, error code otherwise
 */
int xy_hal_aes_setkey_enc(const uint8_t *key, uint32_t key_bits);

/**
 * @brief Set AES decryption key
 * @param key Pointer to key data
 * @param key_bits Key size in bits (128, 192, or 256)
 * @return XY_HAL_CRYPTO_OK on success, error code otherwise
 */
int xy_hal_aes_setkey_dec(const uint8_t *key, uint32_t key_bits);

/**
 * @brief AES ECB mode encryption (single block)
 * @param input Input plaintext block (16 bytes)
 * @param output Output ciphertext block (16 bytes)
 * @return XY_HAL_CRYPTO_OK on success, error code otherwise
 */
int xy_hal_aes_encrypt_ecb(const uint8_t input[16], uint8_t output[16]);

/**
 * @brief AES ECB mode decryption (single block)
 * @param input Input ciphertext block (16 bytes)
 * @param output Output plaintext block (16 bytes)
 * @return XY_HAL_CRYPTO_OK on success, error code otherwise
 */
int xy_hal_aes_decrypt_ecb(const uint8_t input[16], uint8_t output[16]);

/**
 * @brief AES CBC mode encryption
 * @param iv Initialization vector (16 bytes)
 * @param input Input plaintext
 * @param output Output ciphertext
 * @param length Data length (must be multiple of 16)
 * @return XY_HAL_CRYPTO_OK on success, error code otherwise
 */
int xy_hal_aes_encrypt_cbc(uint8_t iv[16], const uint8_t *input,
                           uint8_t *output, size_t length);

/**
 * @brief AES CBC mode decryption
 * @param iv Initialization vector (16 bytes)
 * @param input Input ciphertext
 * @param output Output plaintext
 * @param length Data length (must be multiple of 16)
 * @return XY_HAL_CRYPTO_OK on success, error code otherwise
 */
int xy_hal_aes_decrypt_cbc(uint8_t iv[16], const uint8_t *input,
                           uint8_t *output, size_t length);

/* ==================== SHA HAL Interface ==================== */

/**
 * @brief Initialize SHA hardware
 * @return XY_HAL_CRYPTO_OK on success, error code otherwise
 */
int xy_hal_sha_init(void);

/**
 * @brief Deinitialize SHA hardware
 * @return XY_HAL_CRYPTO_OK on success, error code otherwise
 */
int xy_hal_sha_deinit(void);

/**
 * @brief Compute SHA-256 hash
 * @param input Input data
 * @param length Input data length
 * @param output Output hash (32 bytes)
 * @return XY_HAL_CRYPTO_OK on success, error code otherwise
 */
int xy_hal_sha256_compute(const uint8_t *input, size_t length,
                          uint8_t output[32]);

/**
 * @brief Start SHA-256 streaming operation
 * @return XY_HAL_CRYPTO_OK on success, error code otherwise
 */
int xy_hal_sha256_start(void);

/**
 * @brief Update SHA-256 with data
 * @param input Input data
 * @param length Input data length
 * @return XY_HAL_CRYPTO_OK on success, error code otherwise
 */
int xy_hal_sha256_update(const uint8_t *input, size_t length);

/**
 * @brief Finalize SHA-256 and get result
 * @param output Output hash (32 bytes)
 * @return XY_HAL_CRYPTO_OK on success, error code otherwise
 */
int xy_hal_sha256_finish(uint8_t output[32]);

/* ==================== CRC HAL Interface ==================== */

/**
 * @brief Initialize CRC hardware
 * @return XY_HAL_CRYPTO_OK on success, error code otherwise
 */
int xy_hal_crc_init(void);

/**
 * @brief Compute CRC32
 * @param input Input data
 * @param length Input data length
 * @return CRC32 value
 */
uint32_t xy_hal_crc32_compute(const uint8_t *input, size_t length);

/**
 * @brief Accumulate CRC32
 * @param crc Initial CRC value
 * @param input Input data
 * @param length Input data length
 * @return Updated CRC32 value
 */
uint32_t xy_hal_crc32_accumulate(uint32_t crc, const uint8_t *input,
                                 size_t length);

/* ==================== RNG HAL Interface ==================== */

/**
 * @brief Initialize hardware random number generator
 * @return XY_HAL_CRYPTO_OK on success, error code otherwise
 */
int xy_hal_rng_init(void);

/**
 * @brief Deinitialize hardware random number generator
 * @return XY_HAL_CRYPTO_OK on success, error code otherwise
 */
int xy_hal_rng_deinit(void);

/**
 * @brief Generate random bytes
 * @param output Output buffer for random data
 * @param length Number of random bytes to generate
 * @return XY_HAL_CRYPTO_OK on success, error code otherwise
 */
int xy_hal_rng_generate(uint8_t *output, size_t length);

/**
 * @brief Generate a random 32-bit value
 * @param output Pointer to store random value
 * @return XY_HAL_CRYPTO_OK on success, error code otherwise
 */
int xy_hal_rng_get_uint32(uint32_t *output);

/* ==================== Capability Query ==================== */

/**
 * @brief Check if hardware AES is available
 * @return 1 if available, 0 otherwise
 */
int xy_hal_crypto_has_aes(void);

/**
 * @brief Check if hardware SHA is available
 * @return 1 if available, 0 otherwise
 */
int xy_hal_crypto_has_sha(void);

/**
 * @brief Check if hardware CRC is available
 * @return 1 if available, 0 otherwise
 */
int xy_hal_crypto_has_crc(void);

/**
 * @brief Check if hardware RNG is available
 * @return 1 if available, 0 otherwise
 */
int xy_hal_crypto_has_rng(void);

#ifdef __cplusplus
}
#endif

#endif /* XY_HAL_CRYPTO_H */
