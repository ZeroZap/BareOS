/**
 * @file xy_sha256.h
 * @brief SHA256 Hash Interface
 * @version 2.0.0
 */

#ifndef XY_SHA256_H
#define XY_SHA256_H

#include <stdint.h>
#include "xy_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define XY_SHA256_HASH_SIZE    32
#define XY_SHA256_DIGEST_SIZE  32
#define XY_SHA256_BLOCK_SIZE   64

typedef struct {
    uint32_t state[8];
    uint64_t count;
    uint8_t  buffer[XY_SHA256_BLOCK_SIZE];
} xy_sha256_ctx_t;

int xy_sha256_init(xy_sha256_ctx_t *ctx);
int xy_sha256_update(xy_sha256_ctx_t *ctx, const uint8_t *data, size_t len);
int xy_sha256_final(xy_sha256_ctx_t *ctx, uint8_t digest[XY_SHA256_DIGEST_SIZE]);
int xy_sha256_hash(const uint8_t *data, size_t len, uint8_t digest[XY_SHA256_DIGEST_SIZE]);

/* Legacy name kept for internal callers */
static inline void xy_sha256_finish(xy_sha256_ctx_t *ctx, uint8_t *digest)
{
    xy_sha256_final(ctx, digest);
}

#ifdef __cplusplus
}
#endif

#endif /* XY_SHA256_H */
