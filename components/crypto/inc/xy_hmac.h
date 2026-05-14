/**
 * @file xy_hmac.h
 * @brief HMAC (Hash-based Message Authentication Code)
 */

#ifndef XY_HMAC_H
#define XY_HMAC_H

#include <stdint.h>

typedef struct {
    uint8_t key[64];
    uint32_t key_len;
} xy_hmac_ctx_t;

int xy_hmac_init(xy_hmac_ctx_t *ctx, const uint8_t *key, uint32_t key_len);
int xy_hmac_update(xy_hmac_ctx_t *ctx, const uint8_t *data, uint32_t len);
int xy_hmac_final(xy_hmac_ctx_t *ctx, uint8_t *output, uint32_t *out_len);

#endif /* XY_HMAC_H */
