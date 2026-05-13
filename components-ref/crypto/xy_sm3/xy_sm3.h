/**
 * @file xy_sm3.h
 * @brief SM3 Hash Algorithm (GM/T 0004-2012)
 * @version 1.0.0
 * @date 2026-04-30
 */

#ifndef XY_SM3_H
#define XY_SM3_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== SM3 Constants ==================== */

#define XY_SM3_DIGEST_SIZE   32    /* 256 bits */
#define XY_SM3_BLOCK_SIZE   64    /* 512 bits */

/* ==================== SM3 Context ==================== */

typedef struct {
    uint32_t state[8];       /* 256-bit state (8 x 32-bit) */
    uint64_t count;         /* message length in bits */
    uint8_t buffer[64];    /* input buffer */
    int buffer_len;          /* bytes in buffer */
} xy_sm3_ctx_t;

/* ==================== SM3 API ==================== */

/**
 * @brief SM3 one-shot hash
 * @param data input data
 * @param len data length
 * @param digest output hash (32 bytes)
 * @return 0 on success
 */
int xy_sm3_hash(const uint8_t *data, size_t len, uint8_t digest[XY_SM3_DIGEST_SIZE]);

/**
 * @brief Initialize SM3 context
 * @param ctx SM3 context
 * @return 0 on success
 */
int xy_sm3_init(xy_sm3_ctx_t *ctx);

/**
 * @brief Update SM3 hash with more data
 * @param ctx SM3 context
 * @param data input data
 * @param len data length
 * @return 0 on success
 */
int xy_sm3_update(xy_sm3_ctx_t *ctx, const uint8_t *data, size_t len);

/**
 * @brief Finalize SM3 hash
 * @param ctx SM3 context
 * @param digest output hash (32 bytes)
 * @return 0 on success
 */
int xy_sm3_final(xy_sm3_ctx_t *ctx, uint8_t digest[XY_SM3_DIGEST_SIZE]);

/* ==================== HMAC-SM3 ==================== */

/**
 * @brief HMAC-SM3
 * @param key key data
 * @param key_len key length
 * @param data input data
 * @param data_len data length
 * @param digest output hash (32 bytes)
 * @return 0 on success
 */
int xy_hmac_sm3(const uint8_t *key, size_t key_len,
              const uint8_t *data, size_t data_len,
              uint8_t digest[XY_SM3_DIGEST_SIZE]);

#ifdef __cplusplus
}
#endif

#endif /* XY_SM3_H */