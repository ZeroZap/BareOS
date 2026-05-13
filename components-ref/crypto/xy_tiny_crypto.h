/**
 * @file xy_tiny_crypto.h
 * @brief XY Tiny Crypto Library - Main Header File
 * @version 1.0.0
 * @date 2026-02-28
 */

#ifndef XY_TINY_CRYPTO_H
#define XY_TINY_CRYPTO_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Error Codes ==================== */

#define XY_CRYPTO_SUCCESS           0
#define XY_CRYPTO_ERROR             (-1)
#define XY_CRYPTO_INVALID_PARAM     (-2)
#define XY_CRYPTO_BUFFER_TOO_SMALL  (-3)

/* ==================== MD5 ==================== */

#define XY_MD5_DIGEST_SIZE      16
#define XY_MD5_BLOCK_SIZE       64

typedef struct {
    uint32_t state[4];
    uint64_t count;
    uint8_t buffer[XY_MD5_BLOCK_SIZE];
} xy_md5_ctx_t;

int xy_md5_hash(const uint8_t *data, size_t len, uint8_t digest[XY_MD5_DIGEST_SIZE]);
int xy_md5_init(xy_md5_ctx_t *ctx);
int xy_md5_update(xy_md5_ctx_t *ctx, const uint8_t *data, size_t len);
int xy_md5_final(xy_md5_ctx_t *ctx, uint8_t digest[XY_MD5_DIGEST_SIZE]);

/* ==================== SHA-256 ==================== */

#define XY_SHA256_DIGEST_SIZE   32
#define XY_SHA256_BLOCK_SIZE    64

typedef struct {
    uint32_t state[8];
    uint64_t count;
    uint8_t buffer[XY_SHA256_BLOCK_SIZE];
} xy_sha256_ctx_t;

int xy_sha256_hash(const uint8_t *data, size_t len, uint8_t digest[XY_SHA256_DIGEST_SIZE]);
int xy_sha256_init(xy_sha256_ctx_t *ctx);
int xy_sha256_update(xy_sha256_ctx_t *ctx, const uint8_t *data, size_t len);
int xy_sha256_final(xy_sha256_ctx_t *ctx, uint8_t digest[XY_SHA256_DIGEST_SIZE]);

/* ==================== AES ==================== */

#define XY_AES_KEY_SIZE_128     16
#define XY_AES_KEY_SIZE_192     24
#define XY_AES_KEY_SIZE_256     32
#define XY_AES_BLOCK_SIZE       16

typedef struct {
    uint32_t round_keys[60];
    int rounds;
} xy_aes_ctx_t;

int xy_aes_init(xy_aes_ctx_t *ctx, const uint8_t *key, int key_size);
int xy_aes_encrypt_block(xy_aes_ctx_t *ctx, const uint8_t *plaintext, uint8_t *ciphertext);
int xy_aes_decrypt_block(xy_aes_ctx_t *ctx, const uint8_t *ciphertext, uint8_t *plaintext);
int xy_aes_cbc_encrypt(xy_aes_ctx_t *ctx, const uint8_t *iv,
                       const uint8_t *plaintext, size_t len, uint8_t *ciphertext);
int xy_aes_cbc_decrypt(xy_aes_ctx_t *ctx, const uint8_t *iv,
                       const uint8_t *ciphertext, size_t len, uint8_t *plaintext);

/* ==================== Base64 ==================== */

size_t xy_base64_encode_len(size_t input_len);
size_t xy_base64_decode_len(size_t input_len);
int xy_base64_encode(const uint8_t *input, size_t input_len, char *output, size_t output_len);
int xy_base64_decode(const char *input, size_t input_len, uint8_t *output, size_t output_len);

/* ==================== Hex Encoding ==================== */

size_t xy_hex_encode_len(size_t input_len);
size_t xy_hex_decode_len(size_t input_len);
int xy_hex_encode(const uint8_t *input, size_t input_len, char *output, size_t output_len);
int xy_hex_decode(const char *input, size_t input_len, uint8_t *output, size_t output_len);

/* ==================== CRC32 ==================== */

uint32_t xy_crc32(const uint8_t *data, size_t len);
uint32_t xy_crc32_update(uint32_t crc, const uint8_t *data, size_t len);

/* ==================== HMAC ==================== */

int xy_hmac_md5(const uint8_t *key, size_t key_len,
                const uint8_t *data, size_t data_len,
                uint8_t digest[XY_MD5_DIGEST_SIZE]);
int xy_hmac_sha256(const uint8_t *key, size_t key_len,
                   const uint8_t *data, size_t data_len,
                   uint8_t digest[XY_SHA256_DIGEST_SIZE]);

/* ==================== Random Number Generation ==================== */

int xy_random_bytes(uint8_t *buffer, size_t len);
uint32_t xy_random_uint32(void);

/* ==================== Ascon Lightweight Cryptography ==================== */
/* See xy_ascon/xy_ascon.h for full API */
#include "xy_ascon/xy_ascon.h"

/* ==================== TinyJambu Lightweight Cryptography ==================== */
/* See xy_tinyjambu/xy_tinyjambu.h for full API */
#include "xy_tinyjambu/xy_tinyjambu.h"

/* ==================== Photon Beetle Lightweight Cryptography ==================== */
/* See xy_photon_beetle/xy_photon_beetle.h for full API */
#include "xy_photon_beetle/xy_photon_beetle.h"

/* ==================== SM3 Hash (GM/T 0004-2012) ==================== */
/* See xy_sm3/xy_sm3.h for full API */
#include "xy_sm3/xy_sm3.h"

/* ==================== SM4 Block Cipher (GM/T 0002-2010) ==================== */
/* See xy_sm4/xy_sm4.h for full API */
#include "xy_sm4/xy_sm4.h"

/* ==================== SM2 Elliptic Curve (GM/T 0003-2010) ==================== */
/* See xy_sm2/xy_sm2.h for full API */
#include "xy_sm2/xy_sm2.h"

#ifdef __cplusplus
}
#endif

#endif /* XY_TINY_CRYPTO_H */
