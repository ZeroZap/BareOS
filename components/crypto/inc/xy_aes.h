/**
 * @file xy_aes.h
 * @brief AES Encryption Placeholder
 */

#ifndef XY_AES_H
#define XY_AES_H

#include <stdint.h>

typedef struct {
    uint8_t key[32];
    uint8_t round_keys[15][16];
    int rounds;
} xy_aes_ctx_t;

int xy_aes_init(xy_aes_ctx_t *ctx, const uint8_t *key, int key_bits);
int xy_aes_encrypt(xy_aes_ctx_t *ctx, const uint8_t *in, uint8_t *out);
int xy_aes_decrypt(xy_aes_ctx_t *ctx, const uint8_t *in, uint8_t *out);

#endif /* XY_AES_H */
