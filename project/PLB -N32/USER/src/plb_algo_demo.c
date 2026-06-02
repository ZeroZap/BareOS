/**
 * @file plb_algo_demo.c
 * @brief PLB-N32 algorithm validation over UART4 xy_log.
 */

#include "xy_log.h"
#include "xy_tiny_crypto.h"
#include <stdint.h>

static void bytes_to_hex(const uint8_t *bytes, uint32_t len, char *out)
{
    static const char hex[] = "0123456789abcdef";
    uint32_t i;

    for (i = 0; i < len; i++) {
        out[i * 2] = hex[(bytes[i] >> 4) & 0x0f];
        out[i * 2 + 1] = hex[bytes[i] & 0x0f];
    }
    out[len * 2] = '\0';
}

static int bytes_equal(const uint8_t *a, const uint8_t *b, uint32_t len)
{
    uint32_t i;

    for (i = 0; i < len; i++) {
        if (a[i] != b[i]) {
            return 0;
        }
    }
    return 1;
}

static void algo_demo_rng(void)
{
    uint32_t seed0 = 1;
    uint32_t seed1 = 2;
    uint32_t value;
    uint32_t i;

    xy_log_i("ALGO RNG pseudo seed=(%x,%x)", seed0, seed1);
    for (i = 0; i < 4; i++) {
        seed0 = seed0 * 1103515245u + 12345u + seed1;
        seed1 = seed1 * 1664525u + 1013904223u + seed0;
        value = seed0 ^ seed1;
        xy_log_i("ALGO RNG[%u]=%x", i, value);
    }
}

static void algo_demo_sha256(void)
{
    static const uint8_t msg[] = "Hello!";
    static const uint8_t expected[XY_SHA256_DIGEST_SIZE] = {
        0x33, 0x4d, 0x01, 0x6f, 0x75, 0x5c, 0xd6, 0xdc,
        0x58, 0xc5, 0x3a, 0x86, 0xe1, 0x83, 0x88, 0x2f,
        0x8e, 0xc1, 0x4b, 0x6a, 0x04, 0xa7, 0xc8, 0x4e,
        0x59, 0x6f, 0x09, 0xe2, 0x5c, 0x5f, 0x6a, 0x3a,
    };
    uint8_t digest[XY_SHA256_DIGEST_SIZE];
    char hex[XY_SHA256_DIGEST_SIZE * 2 + 1];

    if (xy_sha256_hash(msg, sizeof(msg) - 1u, digest) != XY_CRYPTO_SUCCESS) {
        xy_log_e("ALGO SHA256 failed");
        return;
    }

    bytes_to_hex(digest, sizeof(digest), hex);
    xy_log_i("ALGO SHA256(Hello!)=%s", hex);
    xy_log_i("ALGO SHA256 verify=%s", bytes_equal(digest, expected, sizeof(digest)) ? "OK" : "FAIL");
}

static void algo_demo_aes(void)
{
    static const uint8_t key[XY_AES_KEY_SIZE_128] = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
    };
    static const uint8_t plain[XY_AES_BLOCK_SIZE] = {
        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
        0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00,
    };
    xy_aes_ctx_t ctx;
    uint8_t cipher[XY_AES_BLOCK_SIZE];
    uint8_t plain_out[XY_AES_BLOCK_SIZE];
    char hex[XY_AES_BLOCK_SIZE * 2 + 1];

    if (xy_aes_init(&ctx, key, sizeof(key)) != XY_CRYPTO_SUCCESS) {
        xy_log_e("ALGO AES init failed");
        return;
    }
    if (xy_aes_encrypt_block(&ctx, plain, cipher) != XY_CRYPTO_SUCCESS) {
        xy_log_e("ALGO AES encrypt failed");
        return;
    }
    if (xy_aes_decrypt_block(&ctx, cipher, plain_out) != XY_CRYPTO_SUCCESS) {
        xy_log_e("ALGO AES decrypt failed");
        return;
    }

    bytes_to_hex(cipher, sizeof(cipher), hex);
    xy_log_i("ALGO AES-128 ECB cipher=%s", hex);
    xy_log_i("ALGO AES-128 ECB verify=%s", bytes_equal(plain, plain_out, sizeof(plain)) ? "OK" : "FAIL");
}

void plb_algo_demo_run(void)
{
    xy_log_i("ALGO demo start");
    algo_demo_rng();
    algo_demo_sha256();
    algo_demo_aes();
    xy_log_i("ALGO demo end");
}
