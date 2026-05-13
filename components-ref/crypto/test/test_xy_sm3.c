/**
 * @file test_xy_sm3.c
 * @brief SM3 hash algorithm tests
 * @version 1.0.0
 * @date 2026-04-30
 */

#include <stdio.h>
#include <string.h>
#include "xy_sm3.h"
#include "xy_tiny_crypto.h"

static void print_hex(const uint8_t *data, size_t len, const char *label)
{
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) {
        printf("%02x", data[i]);
    }
    printf("\n");
}

void test_sm3_one_shot(void)
{
    const char *test_data = "abc";
    uint8_t digest[XY_SM3_DIGEST_SIZE];
    int ret;

    printf("\n=== SM3 One-Shot Hash Test ===\n");

    ret = xy_sm3_hash((const uint8_t *)test_data, strlen(test_data), digest);
    if (ret == XY_CRYPTO_SUCCESS) {
        printf("Input: %s\n", test_data);
        print_hex(digest, XY_SM3_DIGEST_SIZE, "SM3 Hash");
        printf("Expected: 66c7f0f4 62eeedd9 d1fd2e8 70d9cb20 e731f3e6 54d99022 46da4a7a 291f76c0\n");
    } else {
        printf("SM3 hash failed: %d\n", ret);
    }
}

void test_sm3_incremental(void)
{
    xy_sm3_ctx_t ctx;
    uint8_t digest[XY_SM3_DIGEST_SIZE];
    const uint8_t *part1 = (const uint8_t *)"abc";
    const uint8_t *part2 = (const uint8_t *)"def";
    const uint8_t *part3 = (const uint8_t *)"ghi";

    printf("\n=== SM3 Incremental Hash Test ===\n");

    xy_sm3_init(&ctx);
    xy_sm3_update(&ctx, part1, 3);
    xy_sm3_update(&ctx, part2, 3);
    xy_sm3_update(&ctx, part3, 3);
    xy_sm3_final(&ctx, digest);

    printf("Input: abcdefghi (incremental)\n");
    print_hex(digest, XY_SM3_DIGEST_SIZE, "SM3 Hash");

    /* Verify it matches one-shot */
    uint8_t one_shot[XY_SM3_DIGEST_SIZE];
    xy_sm3_hash((const uint8_t *)"abcdefghi", 9, one_shot);

    if (memcmp(digest, one_shot, XY_SM3_DIGEST_SIZE) == 0) {
        printf("Incremental hash matches one-shot: PASSED\n");
    } else {
        printf("Incremental hash mismatch: FAILED\n");
    }
}

void test_sm3_hmac(void)
{
    const uint8_t key[] = { 0x6b, 0x65, 0x79 }; /* "key" */
    const uint8_t data[] = { 0x64, 0x61, 0x74, 0x61 }; /* "data" */
    uint8_t digest[XY_SM3_DIGEST_SIZE];
    int ret;

    printf("\n=== HMAC-SM3 Test ===\n");

    ret = xy_hmac_sm3(key, sizeof(key), data, sizeof(data), digest);
    if (ret == XY_CRYPTO_SUCCESS) {
        printf("Key: key, Data: data\n");
        print_hex(digest, XY_SM3_DIGEST_SIZE, "HMAC-SM3");
    } else {
        printf("HMAC-SM3 failed: %d\n", ret);
    }
}

int test_sm3_main(void)
{
    printf("XY Tiny Crypto - SM3 Tests\n");
    printf("==========================\n");

    test_sm3_one_shot();
    test_sm3_incremental();
    test_sm3_hmac();

    printf("\nSM3 Tests Complete!\n");
    return 0;
}
