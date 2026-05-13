#include <stdint.h>
#include <string.h>
#include "xy_tiny_crypto.h"

#define HMAC_IPAD 0x36
#define HMAC_OPAD 0x5C

int xy_hmac_md5(const uint8_t *key, size_t key_len, const uint8_t *data,
                size_t data_len, uint8_t digest[XY_MD5_DIGEST_SIZE])
{
    if (!key || !data || !digest)
        return XY_CRYPTO_INVALID_PARAM;

    uint8_t k_ipad[XY_MD5_BLOCK_SIZE];
    uint8_t k_opad[XY_MD5_BLOCK_SIZE];
    uint8_t temp_key[XY_MD5_DIGEST_SIZE];
    const uint8_t *actual_key = key;
    size_t actual_key_len     = key_len;

    // 如果密钥长度大于块大小，先哈希密钥
    if (key_len > XY_MD5_BLOCK_SIZE) {
        xy_md5_hash(key, key_len, temp_key);
        actual_key     = temp_key;
        actual_key_len = XY_MD5_DIGEST_SIZE;
    }

    // 准备 ipad 和 opad
    memset(k_ipad, 0, XY_MD5_BLOCK_SIZE);
    memset(k_opad, 0, XY_MD5_BLOCK_SIZE);
    memcpy(k_ipad, actual_key, actual_key_len);
    memcpy(k_opad, actual_key, actual_key_len);

    for (int i = 0; i < XY_MD5_BLOCK_SIZE; i++) {
        k_ipad[i] ^= HMAC_IPAD;
        k_opad[i] ^= HMAC_OPAD;
    }

    // 内部哈希: H(K XOR ipad, text)
    xy_md5_ctx_t ctx;
    xy_md5_init(&ctx);
    xy_md5_update(&ctx, k_ipad, XY_MD5_BLOCK_SIZE);
    xy_md5_update(&ctx, data, data_len);
    xy_md5_final(&ctx, digest);

    // 外部哈希: H(K XOR opad, result)
    xy_md5_init(&ctx);
    xy_md5_update(&ctx, k_opad, XY_MD5_BLOCK_SIZE);
    xy_md5_update(&ctx, digest, XY_MD5_DIGEST_SIZE);
    xy_md5_final(&ctx, digest);

    return XY_CRYPTO_SUCCESS;
}

int xy_hmac_sha256(const uint8_t *key, size_t key_len, const uint8_t *data,
                   size_t data_len, uint8_t digest[XY_SHA256_DIGEST_SIZE])
{
    if (!key || !data || !digest)
        return XY_CRYPTO_INVALID_PARAM;

    uint8_t k_ipad[XY_SHA256_BLOCK_SIZE];
    uint8_t k_opad[XY_SHA256_BLOCK_SIZE];
    uint8_t temp_key[XY_SHA256_DIGEST_SIZE];
    const uint8_t *actual_key = key;
    size_t actual_key_len     = key_len;

    // 如果密钥长度大于块大小，先哈希密钥
    if (key_len > XY_SHA256_BLOCK_SIZE) {
        xy_sha256_hash(key, key_len, temp_key);
        actual_key     = temp_key;
        actual_key_len = XY_SHA256_DIGEST_SIZE;
    }

    // 准备 ipad 和 opad
    memset(k_ipad, 0, XY_SHA256_BLOCK_SIZE);
    memset(k_opad, 0, XY_SHA256_BLOCK_SIZE);
    memcpy(k_ipad, actual_key, actual_key_len);
    memcpy(k_opad, actual_key, actual_key_len);

    for (int i = 0; i < XY_SHA256_BLOCK_SIZE; i++) {
        k_ipad[i] ^= HMAC_IPAD;
        k_opad[i] ^= HMAC_OPAD;
    }

    // 内部哈希: H(K XOR ipad, text)
    xy_sha256_ctx_t ctx;
    xy_sha256_init(&ctx);
    xy_sha256_update(&ctx, k_ipad, XY_SHA256_BLOCK_SIZE);
    xy_sha256_update(&ctx, data, data_len);
    xy_sha256_final(&ctx, digest);

    // 外部哈希: H(K XOR opad, result)
    xy_sha256_init(&ctx);
    xy_sha256_update(&ctx, k_opad, XY_SHA256_BLOCK_SIZE);
    xy_sha256_update(&ctx, digest, XY_SHA256_DIGEST_SIZE);
    xy_sha256_final(&ctx, digest);

    return XY_CRYPTO_SUCCESS;
}