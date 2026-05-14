#ifndef XY_TINY_CRYPTO_H
#define XY_TINY_CRYPTO_H

#include <stdint.h>
#include <stddef.h>
#include "xy_crypto_config.h"

#ifdef __cplusplus
extern "C" {
#endif

// ==================== Platform Management ====================

/**
 * @brief Initialize cryptographic platform
 *
 * Call this function before using any crypto operations.
 * It initializes hardware acceleration if available.
 *
 * @return XY_CRYPTO_SUCCESS on success, error code otherwise
 */
int xy_crypto_platform_init(void);

/**
 * @brief Deinitialize cryptographic platform
 *
 * Call this function when done with crypto operations.
 *
 * @return XY_CRYPTO_SUCCESS on success, error code otherwise
 */
int xy_crypto_platform_deinit(void);

/**
 * @brief Get platform type string
 * @return Platform description string ("Software", "HAL", or "SDK")
 */
const char *xy_crypto_platform_name(void);

/**
 * @brief Check if hardware acceleration is available for algorithm
 * @param algorithm Algorithm identifier (0=AES, 1=SHA, 2=CRC, 3=RNG)
 * @return 1 if hardware accelerated, 0 if software only
 */
int xy_crypto_is_hw_accelerated(int algorithm);

// ==================== 通用定义 ====================
#define XY_CRYPTO_SUCCESS          0
#define XY_CRYPTO_ERROR            -1
#define XY_CRYPTO_INVALID_PARAM    -2
#define XY_CRYPTO_BUFFER_TOO_SMALL -3

// ==================== MD5 算法 ====================
#if XY_CRYPTO_ENABLE_MD5
#define XY_MD5_DIGEST_SIZE 16
#define XY_MD5_BLOCK_SIZE  64

typedef struct {
    uint32_t state[4];
    uint64_t count;
    uint8_t buffer[XY_MD5_BLOCK_SIZE];
} xy_md5_ctx_t;

int xy_md5_init(xy_md5_ctx_t *ctx);
int xy_md5_update(xy_md5_ctx_t *ctx, const uint8_t *data, size_t len);
int xy_md5_final(xy_md5_ctx_t *ctx, uint8_t digest[XY_MD5_DIGEST_SIZE]);
int xy_md5_hash(const uint8_t *data, size_t len,
                uint8_t digest[XY_MD5_DIGEST_SIZE]);

#endif /* XY_CRYPTO_ENABLE_MD5 */

// ==================== SHA1 算法 ====================
#if XY_CRYPTO_ENABLE_SHA1
#define XY_SHA1_DIGEST_SIZE 20
#define XY_SHA1_BLOCK_SIZE  64

typedef struct {
    uint32_t state[5];
    uint64_t count;
    uint8_t buffer[XY_SHA1_BLOCK_SIZE];
} xy_sha1_ctx_t;

int xy_sha1_init(xy_sha1_ctx_t *ctx);
int xy_sha1_update(xy_sha1_ctx_t *ctx, const uint8_t *data, size_t len);
int xy_sha1_final(xy_sha1_ctx_t *ctx, uint8_t digest[XY_SHA1_DIGEST_SIZE]);
int xy_sha1_hash(const uint8_t *data, size_t len,
                 uint8_t digest[XY_SHA1_DIGEST_SIZE]);

#endif /* XY_CRYPTO_ENABLE_SHA1 */

// ==================== SHA256 算法 ====================
#if XY_CRYPTO_ENABLE_SHA256
#define XY_SHA256_DIGEST_SIZE 32
#define XY_SHA256_BLOCK_SIZE  64

typedef struct {
    uint32_t state[8];
    uint64_t count;
    uint8_t buffer[XY_SHA256_BLOCK_SIZE];
} xy_sha256_ctx_t;

int xy_sha256_init(xy_sha256_ctx_t *ctx);
int xy_sha256_update(xy_sha256_ctx_t *ctx, const uint8_t *data, size_t len);
int xy_sha256_final(xy_sha256_ctx_t *ctx,
                    uint8_t digest[XY_SHA256_DIGEST_SIZE]);
int xy_sha256_hash(const uint8_t *data, size_t len,
                   uint8_t digest[XY_SHA256_DIGEST_SIZE]);

#endif /* XY_CRYPTO_ENABLE_SHA256 */

// ==================== AES 算法 ====================
#if XY_CRYPTO_ENABLE_AES
#define XY_AES_BLOCK_SIZE   16
#define XY_AES_KEY_SIZE_128 16
#define XY_AES_KEY_SIZE_192 24
#define XY_AES_KEY_SIZE_256 32

typedef struct {
    uint32_t round_keys[60]; // 最大支持 AES-256 (14轮+1)
    int rounds;
} xy_aes_ctx_t;

int xy_aes_init(xy_aes_ctx_t *ctx, const uint8_t *key, int key_size);
int xy_aes_encrypt_block(xy_aes_ctx_t *ctx, const uint8_t *plaintext,
                         uint8_t *ciphertext);
int xy_aes_decrypt_block(xy_aes_ctx_t *ctx, const uint8_t *ciphertext,
                         uint8_t *plaintext);

// AES CBC 模式
int xy_aes_cbc_encrypt(xy_aes_ctx_t *ctx, const uint8_t *iv,
                       const uint8_t *plaintext, size_t len,
                       uint8_t *ciphertext);
int xy_aes_cbc_decrypt(xy_aes_ctx_t *ctx, const uint8_t *iv,
                       const uint8_t *ciphertext, size_t len,
                       uint8_t *plaintext);

#endif /* XY_CRYPTO_ENABLE_AES */

// ==================== Base64 编码 ====================
size_t xy_base64_encode_len(size_t input_len);
size_t xy_base64_decode_len(size_t input_len);
int xy_base64_encode(const uint8_t *input, size_t input_len, char *output,
                     size_t output_len);
int xy_base64_decode(const char *input, size_t input_len, uint8_t *output,
                     size_t output_len);

// ==================== 十六进制编码 ====================
size_t xy_hex_encode_len(size_t input_len);
size_t xy_hex_decode_len(size_t input_len);
int xy_hex_encode(const uint8_t *input, size_t input_len, char *output,
                  size_t output_len);
int xy_hex_decode(const char *input, size_t input_len, uint8_t *output,
                  size_t output_len);

// ==================== 随机数生成 ====================
int xy_random_bytes(uint8_t *buffer, size_t len);
uint32_t xy_random_uint32(void);

// ==================== CSPRNG (ChaCha20-based) ====================
#if XY_CRYPTO_ENABLE_CSPRNG

/**
 * @brief Initialize CSPRNG with seed material
 *
 * @param seed Seed data (minimum 32 bytes recommended)
 * @param seed_len Length of seed data
 * @return 0 on success, negative on error
 */
int xy_csprng_init(const uint8_t *seed, size_t seed_len);

/**
 * @brief Reseed CSPRNG with additional entropy
 *
 * @param entropy Additional entropy data
 * @param entropy_len Length of entropy data
 * @return 0 on success, negative on error
 */
int xy_csprng_reseed(const uint8_t *entropy, size_t entropy_len);

/**
 * @brief Generate cryptographically secure random bytes
 *
 * @param output Output buffer
 * @param output_len Number of bytes to generate
 * @return 0 on success, negative on error
 */
int xy_csprng_generate(uint8_t *output, size_t output_len);

/**
 * @brief Generate a random 32-bit unsigned integer
 *
 * @return Random uint32_t value
 */
uint32_t xy_csprng_uint32(void);

/**
 * @brief Generate a random 64-bit unsigned integer
 *
 * @return Random uint64_t value
 */
uint64_t xy_csprng_uint64(void);

/**
 * @brief Generate uniform random number in range [0, upper_bound)
 *
 * Uses rejection sampling to avoid modulo bias.
 *
 * @param upper_bound Exclusive upper bound (must be > 0)
 * @return Random value in [0, upper_bound)
 */
uint32_t xy_csprng_uniform(uint32_t upper_bound);

/**
 * @brief Securely cleanup CSPRNG state
 *
 * Zeros all internal state. Must call xy_csprng_init() again before use.
 */
void xy_csprng_cleanup(void);

#endif /* XY_CRYPTO_ENABLE_CSPRNG */

// ==================== HMAC ====================
#if XY_CRYPTO_ENABLE_HMAC
int xy_hmac_md5(const uint8_t *key, size_t key_len, const uint8_t *data,
                size_t data_len, uint8_t digest[XY_MD5_DIGEST_SIZE]);

int xy_hmac_sha1(const uint8_t *key, size_t key_len, const uint8_t *data,
                 size_t data_len, uint8_t digest[XY_SHA1_DIGEST_SIZE]);

int xy_hmac_sha256(const uint8_t *key, size_t key_len, const uint8_t *data,
                   size_t data_len, uint8_t digest[XY_SHA256_DIGEST_SIZE]);

#endif /* XY_CRYPTO_ENABLE_HMAC */

// ==================== CRC32 ====================
uint32_t xy_crc32(const uint8_t *data, size_t len);
uint32_t xy_crc32_update(uint32_t crc, const uint8_t *data, size_t len);

// ==================== RSA 算法 ====================
#if XY_CRYPTO_ENABLE_RSA
#define XY_RSA_MAX_KEY_SIZE     4096
#define XY_RSA_MAX_BYTES        (XY_RSA_MAX_KEY_SIZE / 8)
#define XY_RSA_DEFAULT_EXPONENT 65537

// 大整数结构，用于RSA计算
typedef struct {
    uint32_t *data; // 数据存储
    int size;       // 当前使用的数组大小
    int capacity;   // 数组容量
} xy_bigint_t;

// RSA密钥对结构
typedef struct {
    xy_bigint_t n; // 模数
    xy_bigint_t e; // 公钥指数
    xy_bigint_t d; // 私钥指数
    xy_bigint_t p; // 素数p
    xy_bigint_t q; // 素数q
    int key_size;  // 密钥长度（bits）
} xy_rsa_key_t;

// RSA公钥结构
typedef struct {
    xy_bigint_t n; // 模数
    xy_bigint_t e; // 公钥指数
    int key_size;  // 密钥长度（bits）
} xy_rsa_public_key_t;

// RSA初始化和清理
int xy_rsa_init(void);
void xy_rsa_cleanup(void);

// 大整数操作
int xy_bigint_init(xy_bigint_t *a, int capacity);
void xy_bigint_free(xy_bigint_t *a);
int xy_bigint_copy(xy_bigint_t *dst, const xy_bigint_t *src);
int xy_bigint_from_bytes(xy_bigint_t *a, const uint8_t *bytes, size_t len);
int xy_bigint_to_bytes(const xy_bigint_t *a, uint8_t *bytes, size_t len);

// RSA密钥操作
int xy_rsa_generate_key_pair(xy_rsa_key_t *key, int key_size);
int xy_rsa_import_public_key(xy_rsa_public_key_t *pub_key,
                             const uint8_t *n_bytes, size_t n_len,
                             const uint8_t *e_bytes, size_t e_len);
int xy_rsa_export_public_key(const xy_rsa_key_t *key, uint8_t *n_bytes,
                             size_t *n_len, uint8_t *e_bytes, size_t *e_len);
void xy_rsa_free_key(xy_rsa_key_t *key);
void xy_rsa_free_public_key(xy_rsa_public_key_t *pub_key);

// RSA加密解密
int xy_rsa_public_encrypt(const xy_rsa_public_key_t *pub_key,
                          const uint8_t *plaintext, size_t plaintext_len,
                          uint8_t *ciphertext, size_t *ciphertext_len);
int xy_rsa_private_decrypt(const xy_rsa_key_t *key, const uint8_t *ciphertext,
                           size_t ciphertext_len, uint8_t *plaintext,
                           size_t *plaintext_len);
int xy_rsa_private_encrypt(const xy_rsa_key_t *key, const uint8_t *plaintext,
                           size_t plaintext_len, uint8_t *ciphertext,
                           size_t *ciphertext_len);
int xy_rsa_public_decrypt(const xy_rsa_public_key_t *pub_key,
                          const uint8_t *ciphertext, size_t ciphertext_len,
                          uint8_t *plaintext, size_t *plaintext_len);

// RSA签名验证
int xy_rsa_sign(const xy_rsa_key_t *key, const uint8_t *hash, size_t hash_len,
                uint8_t *signature, size_t *sig_len);
int xy_rsa_verify(const xy_rsa_public_key_t *pub_key, const uint8_t *hash,
                  size_t hash_len, const uint8_t *signature, size_t sig_len);

#endif /* XY_CRYPTO_ENABLE_RSA */

// ==================== Curve25519 (X25519 + Ed25519) ====================
#if XY_CRYPTO_ENABLE_CURVE25519

/* X25519 ECDH Constants */
#define XY_X25519_PUBLIC_KEY_SIZE 32
#define XY_X25519_PRIVATE_KEY_SIZE 32
#define XY_X25519_SHARED_SECRET_SIZE 32

#define XY_X25519_SUCCESS 0
#define XY_X25519_ERROR_INVALID_PARAM -1
#define XY_X25519_ERROR_WEAK_KEY -2
#define XY_X25519_ERROR -3

/* Ed25519 Signature Constants */
#define XY_ED25519_PUBLIC_KEY_SIZE 32
#define XY_ED25519_PRIVATE_KEY_SIZE 32
#define XY_ED25519_SIGNATURE_SIZE 64
#define XY_ED25519_SEED_SIZE 32

#define XY_ED25519_SUCCESS 0
#define XY_ED25519_ERROR_INVALID_PARAM -1
#define XY_ED25519_ERROR_VERIFY_FAILED -2
#define XY_ED25519_ERROR_INVALID_SIGNATURE -3
#define XY_ED25519_ERROR -4

/* X25519 ECDH API */
int xy_x25519_generate_keypair(uint8_t private_key[XY_X25519_PRIVATE_KEY_SIZE],
                                uint8_t public_key[XY_X25519_PUBLIC_KEY_SIZE]);
int xy_x25519_public_key(const uint8_t private_key[XY_X25519_PRIVATE_KEY_SIZE],
                          uint8_t public_key[XY_X25519_PUBLIC_KEY_SIZE]);
int xy_x25519_shared_secret(
    uint8_t shared_secret[XY_X25519_SHARED_SECRET_SIZE],
    const uint8_t our_private_key[XY_X25519_PRIVATE_KEY_SIZE],
    const uint8_t their_public_key[XY_X25519_PUBLIC_KEY_SIZE]);
int xy_x25519_validate_public_key(
    const uint8_t public_key[XY_X25519_PUBLIC_KEY_SIZE]);

/* Ed25519 Signature API */
int xy_ed25519_generate_keypair(
    uint8_t public_key[XY_ED25519_PUBLIC_KEY_SIZE],
    uint8_t private_key[XY_ED25519_PRIVATE_KEY_SIZE]);
int xy_ed25519_public_key(
    const uint8_t private_key[XY_ED25519_PRIVATE_KEY_SIZE],
    uint8_t public_key[XY_ED25519_PUBLIC_KEY_SIZE]);
int xy_ed25519_sign(uint8_t signature[XY_ED25519_SIGNATURE_SIZE],
                     const uint8_t *message,
                     size_t message_len,
                     const uint8_t public_key[XY_ED25519_PUBLIC_KEY_SIZE],
                     const uint8_t private_key[XY_ED25519_PRIVATE_KEY_SIZE]);
int xy_ed25519_verify(const uint8_t signature[XY_ED25519_SIGNATURE_SIZE],
                       const uint8_t *message,
                       size_t message_len,
                       const uint8_t public_key[XY_ED25519_PUBLIC_KEY_SIZE]);
int xy_ed25519_sign_simple(
    uint8_t signature[XY_ED25519_SIGNATURE_SIZE],
    const uint8_t *message,
    size_t message_len,
    const uint8_t private_key[XY_ED25519_PRIVATE_KEY_SIZE]);

#endif /* XY_CRYPTO_ENABLE_CURVE25519 */

#ifdef __cplusplus
}
#endif

#endif // XY_TINY_CRYPTO_H