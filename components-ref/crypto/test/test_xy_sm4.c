/**
 * @file test_xy_sm4.c
 * @brief SM4 block cipher tests
 * @version 1.0.0
 * @date 2026-04-30
 */

#include <stdio.h>
#include <string.h>
#include "xy_sm4.h"
#include "xy_tiny_crypto.h"

static void print_hex(const uint8_t *data, size_t len, const char *label)
{
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) {
        printf("%02x", data[i]);
    }
    printf("\n");
}

void test_sm4_ecb(void)
{
    xy_sm4_ctx_t ctx;
    uint8_t key[XY_SM4_KEY_SIZE] = { 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
                                     0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10 };
    uint8_t plaintext[XY_SM4_BLOCK_SIZE] = { 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
                                            0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10 };
    uint8_t ciphertext[XY_SM4_BLOCK_SIZE];
    uint8_t decrypted[XY_SM4_BLOCK_SIZE];
    int ret;

    printf("\n=== SM4 ECB Mode Test ===\n");

    ret = xy_sm4_init(&ctx, key);
    if (ret != XY_CRYPTO_SUCCESS) {
        printf("SM4 init failed: %d\n", ret);
        return;
    }

    ret = xy_sm4_encrypt_block(&ctx, plaintext, ciphertext);
    if (ret != XY_CRYPTO_SUCCESS) {
        printf("SM4 encrypt failed: %d\n", ret);
        return;
    }

    printf("Plaintext:  ");
    print_hex(plaintext, XY_SM4_BLOCK_SIZE, "");
    printf("Ciphertext: ");
    print_hex(ciphertext, XY_SM4_BLOCK_SIZE, "");

    ret = xy_sm4_decrypt_block(&ctx, ciphertext, decrypted);
    if (ret != XY_CRYPTO_SUCCESS) {
        printf("SM4 decrypt failed: %d\n", ret);
        return;
    }

    printf("Decrypted:  ");
    print_hex(decrypted, XY_SM4_BLOCK_SIZE, "");

    if (memcmp(plaintext, decrypted, XY_SM4_BLOCK_SIZE) == 0) {
        printf("SM4 ECB test PASSED\n");
    } else {
        printf("SM4 ECB test FAILED\n");
    }
}

void test_sm4_cbc(void)
{
    xy_sm4_ctx_t ctx;
    uint8_t key[XY_SM4_KEY_SIZE] = { 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
                                     0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10 };
    uint8_t iv[XY_SM4_BLOCK_SIZE] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                                    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };
    uint8_t plaintext[32] = "The quick brown fox jumps";
    uint8_t ciphertext[32];
    uint8_t decrypted[32];
    int ret;

    printf("\n=== SM4 CBC Mode Test ===\n");

    ret = xy_sm4_init(&ctx, key);
    if (ret != XY_CRYPTO_SUCCESS) {
        printf("SM4 init failed: %d\n", ret);
        return;
    }

    ret = xy_sm4_cbc_encrypt(&ctx, iv, plaintext, 32, ciphertext);
    if (ret != XY_CRYPTO_SUCCESS) {
        printf("SM4 CBC encrypt failed: %d\n", ret);
        return;
    }

    printf("Plaintext: %s\n", plaintext);
    printf("Ciphertext: ");
    print_hex(ciphertext, 32, "");

    /* Reset IV for decryption */
    memset(iv, 0, XY_SM4_BLOCK_SIZE);
    iv[0] = 0x00; iv[1] = 0x01; iv[2] = 0x02; iv[3] = 0x03;
    iv[4] = 0x04; iv[5] = 0x05; iv[6] = 0x06; iv[7] = 0x07;
    iv[8] = 0x08; iv[9] = 0x09; iv[10] = 0x0a; iv[11] = 0x0b;
    iv[12] = 0x0c; iv[13] = 0x0d; iv[14] = 0x0e; iv[15] = 0x0f;

    ret = xy_sm4_cbc_decrypt(&ctx, iv, ciphertext, 32, decrypted);
    if (ret != XY_CRYPTO_SUCCESS) {
        printf("SM4 CBC decrypt failed: %d\n", ret);
        return;
    }

    decrypted[31] = '\0';
    printf("Decrypted: %s\n", decrypted);

    if (memcmp(plaintext, decrypted, 32) == 0) {
        printf("SM4 CBC test PASSED\n");
    } else {
        printf("SM4 CBC test FAILED\n");
    }
}

int test_sm4_main(void)
{
    printf("XY Tiny Crypto - SM4 Tests\n");
    printf("==========================\n");

    test_sm4_ecb();
    test_sm4_cbc();

    printf("\nSM4 Tests Complete!\n");
    return 0;
}
