/**
 * @file xy_blake2.h
 * @brief BLAKE2 cryptographic hash function (BLAKE2b and BLAKE2s)
 * @version 1.0.0
 * @date 2025-11-02
 *
 * This module implements BLAKE2b (64-bit) and BLAKE2s (32-bit) hash functions
 * as specified in RFC 7693. BLAKE2 is faster than MD5, SHA-1, SHA-2, and SHA-3,
 * yet is at least as secure as the latest standard SHA-3.
 */

#ifndef XY_BLAKE2_H
#define XY_BLAKE2_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Configuration ==================== */

/**
 * @brief Enable BLAKE2b (64-bit optimized) - Set to 1 to enable
 * Note: BLAKE2b requires more code space (~4.5 KB) and is optimized for
 * 64-bit platforms. For 32-bit embedded systems, use BLAKE2s instead.
 */
#ifndef XY_BLAKE2_ENABLE_BLAKE2B
#define XY_BLAKE2_ENABLE_BLAKE2B 0
#endif

/**
 * @brief Enable BLAKE2s (32-bit optimized) - Set to 1 to enable
 * Default enabled for embedded 32-bit platform optimization.
 * BLAKE2s uses ~4.2 KB code space and ~124 bytes RAM (optimal for M0/M3/M4).
 */
#ifndef XY_BLAKE2_ENABLE_BLAKE2S
#define XY_BLAKE2_ENABLE_BLAKE2S 1
#endif

/* ==================== Error Codes ==================== */

/**
 * @brief Operation successful
 */
#define XY_BLAKE2_SUCCESS 0

/**
 * @brief Invalid parameter (NULL pointer or invalid value)
 */
#define XY_BLAKE2_ERROR_INVALID_PARAM -1

/**
 * @brief General error
 */
#define XY_BLAKE2_ERROR -2

/* ==================== BLAKE2b (512-bit) ==================== */

#if XY_BLAKE2_ENABLE_BLAKE2B

/**
 * @brief BLAKE2b state size in bytes (64 bytes)
 */
#define XY_BLAKE2B_BLOCKBYTES 128

/**
 * @brief BLAKE2b hash output size in bytes (default 64, max 64)
 */
#define XY_BLAKE2B_OUTBYTES 64

/**
 * @brief BLAKE2b key size in bytes (max 64)
 */
#define XY_BLAKE2B_KEYBYTES 64

/**
 * @brief BLAKE2b personalization size in bytes
 */
#define XY_BLAKE2B_PERSONALBYTES 16

/**
 * @brief BLAKE2b salt size in bytes
 */
#define XY_BLAKE2B_SALTBYTES 16

/**
 * @brief BLAKE2b context structure
 */
typedef struct {
    uint64_t h[8];                      /**< Chained state */
    uint64_t t[2];                      /**< Total bytes processed */
    uint64_t f[2];                      /**< Finalization flags */
    uint8_t buf[XY_BLAKE2B_BLOCKBYTES]; /**< Input buffer */
    size_t buflen;                      /**< Bytes in buffer */
    size_t outlen;                      /**< Digest size */
} xy_blake2b_ctx_t;

/**
 * @brief BLAKE2b parameter block
 */
typedef struct {
    uint8_t digest_length;                       /**< Digest length (1-64) */
    uint8_t key_length;                          /**< Key length (0-64) */
    uint8_t fanout;                              /**< Fanout (0-255, 0=sequential) */
    uint8_t depth;                               /**< Depth (0-255, 0=sequential) */
    uint32_t leaf_length;                        /**< Leaf maximal byte length */
    uint32_t node_offset;                        /**< Node offset (low 32 bits) */
    uint32_t xof_length;                         /**< XOF digest length */
    uint8_t node_depth;                          /**< Node depth (0-255) */
    uint8_t inner_length;                        /**< Inner hash byte length */
    uint8_t reserved[14];                        /**< Reserved */
    uint8_t salt[XY_BLAKE2B_SALTBYTES];          /**< Salt */
    uint8_t personal[XY_BLAKE2B_PERSONALBYTES];  /**< Personalization */
} xy_blake2b_param_t;

/**
 * @brief Initialize BLAKE2b hash context
 *
 * @param ctx Context to initialize
 * @param outlen Desired output length in bytes (1-64)
 * @return XY_BLAKE2_SUCCESS on success, error code otherwise
 */
int xy_blake2b_init(xy_blake2b_ctx_t *ctx, size_t outlen);

/**
 * @brief Initialize BLAKE2b with key (for MAC)
 *
 * @param ctx Context to initialize
 * @param outlen Desired output length in bytes (1-64)
 * @param key Key data
 * @param keylen Key length in bytes (1-64)
 * @return XY_BLAKE2_SUCCESS on success, error code otherwise
 */
int xy_blake2b_init_key(xy_blake2b_ctx_t *ctx, size_t outlen,
                         const uint8_t *key, size_t keylen);

/**
 * @brief Initialize BLAKE2b with parameters
 *
 * @param ctx Context to initialize
 * @param param Parameter block
 * @return XY_BLAKE2_SUCCESS on success, error code otherwise
 */
int xy_blake2b_init_param(xy_blake2b_ctx_t *ctx, const xy_blake2b_param_t *param);

/**
 * @brief Update BLAKE2b hash with more data
 *
 * @param ctx BLAKE2b context
 * @param data Input data
 * @param datalen Length of input data
 * @return XY_BLAKE2_SUCCESS on success, error code otherwise
 */
int xy_blake2b_update(xy_blake2b_ctx_t *ctx, const uint8_t *data, size_t datalen);

/**
 * @brief Finalize BLAKE2b hash and output digest
 *
 * @param ctx BLAKE2b context
 * @param digest Output buffer for hash
 * @param outlen Output length (must match initialization)
 * @return XY_BLAKE2_SUCCESS on success, error code otherwise
 */
int xy_blake2b_final(xy_blake2b_ctx_t *ctx, uint8_t *digest, size_t outlen);

/**
 * @brief Compute BLAKE2b hash in one shot
 *
 * @param digest Output buffer for hash (64 bytes)
 * @param outlen Desired output length (1-64)
 * @param data Input data
 * @param datalen Length of input data
 * @param key Key data (optional, can be NULL)
 * @param keylen Key length (0 if no key)
 * @return XY_BLAKE2_SUCCESS on success, error code otherwise
 */
int xy_blake2b(uint8_t *digest, size_t outlen,
                const uint8_t *data, size_t datalen,
                const uint8_t *key, size_t keylen);

#endif /* XY_BLAKE2_ENABLE_BLAKE2B */

/* ==================== BLAKE2s (256-bit) ==================== */

#if XY_BLAKE2_ENABLE_BLAKE2S

/**
 * @brief BLAKE2s state size in bytes (32 bytes)
 */
#define XY_BLAKE2S_BLOCKBYTES 64

/**
 * @brief BLAKE2s hash output size in bytes (default 32, max 32)
 */
#define XY_BLAKE2S_OUTBYTES 32

/**
 * @brief BLAKE2s key size in bytes (max 32)
 */
#define XY_BLAKE2S_KEYBYTES 32

/**
 * @brief BLAKE2s personalization size in bytes
 */
#define XY_BLAKE2S_PERSONALBYTES 8

/**
 * @brief BLAKE2s salt size in bytes
 */
#define XY_BLAKE2S_SALTBYTES 8

/**
 * @brief BLAKE2s context structure
 */
typedef struct {
    uint32_t h[8];                      /**< Chained state */
    uint32_t t[2];                      /**< Total bytes processed */
    uint32_t f[2];                      /**< Finalization flags */
    uint8_t buf[XY_BLAKE2S_BLOCKBYTES]; /**< Input buffer */
    size_t buflen;                      /**< Bytes in buffer */
    size_t outlen;                      /**< Digest size */
} xy_blake2s_ctx_t;

/**
 * @brief BLAKE2s parameter block
 */
typedef struct {
    uint8_t digest_length;                       /**< Digest length (1-32) */
    uint8_t key_length;                          /**< Key length (0-32) */
    uint8_t fanout;                              /**< Fanout (0-255, 0=sequential) */
    uint8_t depth;                               /**< Depth (0-255, 0=sequential) */
    uint32_t leaf_length;                        /**< Leaf maximal byte length */
    uint32_t node_offset;                        /**< Node offset */
    uint16_t xof_length;                         /**< XOF digest length */
    uint8_t node_depth;                          /**< Node depth (0-255) */
    uint8_t inner_length;                        /**< Inner hash byte length */
    uint8_t salt[XY_BLAKE2S_SALTBYTES];          /**< Salt */
    uint8_t personal[XY_BLAKE2S_PERSONALBYTES];  /**< Personalization */
} xy_blake2s_param_t;

/**
 * @brief Initialize BLAKE2s hash context
 *
 * @param ctx Context to initialize
 * @param outlen Desired output length in bytes (1-32)
 * @return XY_BLAKE2_SUCCESS on success, error code otherwise
 */
int xy_blake2s_init(xy_blake2s_ctx_t *ctx, size_t outlen);

/**
 * @brief Initialize BLAKE2s with key (for MAC)
 *
 * @param ctx Context to initialize
 * @param outlen Desired output length in bytes (1-32)
 * @param key Key data
 * @param keylen Key length in bytes (1-32)
 * @return XY_BLAKE2_SUCCESS on success, error code otherwise
 */
int xy_blake2s_init_key(xy_blake2s_ctx_t *ctx, size_t outlen,
                         const uint8_t *key, size_t keylen);

/**
 * @brief Initialize BLAKE2s with parameters
 *
 * @param ctx Context to initialize
 * @param param Parameter block
 * @return XY_BLAKE2_SUCCESS on success, error code otherwise
 */
int xy_blake2s_init_param(xy_blake2s_ctx_t *ctx, const xy_blake2s_param_t *param);

/**
 * @brief Update BLAKE2s hash with more data
 *
 * @param ctx BLAKE2s context
 * @param data Input data
 * @param datalen Length of input data
 * @return XY_BLAKE2_SUCCESS on success, error code otherwise
 */
int xy_blake2s_update(xy_blake2s_ctx_t *ctx, const uint8_t *data, size_t datalen);

/**
 * @brief Finalize BLAKE2s hash and output digest
 *
 * @param ctx BLAKE2s context
 * @param digest Output buffer for hash
 * @param outlen Output length (must match initialization)
 * @return XY_BLAKE2_SUCCESS on success, error code otherwise
 */
int xy_blake2s_final(xy_blake2s_ctx_t *ctx, uint8_t *digest, size_t outlen);

/**
 * @brief Compute BLAKE2s hash in one shot
 *
 * @param digest Output buffer for hash (32 bytes)
 * @param outlen Desired output length (1-32)
 * @param data Input data
 * @param datalen Length of input data
 * @param key Key data (optional, can be NULL)
 * @param keylen Key length (0 if no key)
 * @return XY_BLAKE2_SUCCESS on success, error code otherwise
 */
int xy_blake2s(uint8_t *digest, size_t outlen,
                const uint8_t *data, size_t datalen,
                const uint8_t *key, size_t keylen);

#endif /* XY_BLAKE2_ENABLE_BLAKE2S */

#ifdef __cplusplus
}
#endif

#endif /* XY_BLAKE2_H */
