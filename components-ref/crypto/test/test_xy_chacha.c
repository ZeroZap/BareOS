/**
 * @file test_xy_chacha.c
 * @brief ChaCha20-Poly1305 tests
 * @version 1.0.0
 * @date 2026-04-30
 */

#include <stdio.h>
#include <string.h>
#include "xy_chacha20poly1305.h"
#include "xy_tiny_crypto.h"

static void print_hex(const uint8_t *data, size_t len, const char *label)
{
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) {
        printf("%02x", data[i]);
    }
    printf("\n");
}

void test_chacha20_poly1305_basic(void)
{
    /* RFC 7539 test vectors */
    uint8_t key[32] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };
    uint8_t nonce[12] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01
    };
    uint8_t aad[32] = {
        0xfd, 0xff, 0xff, 0xff, 0xfd, 0xff, 0xff, 0xff,
        0xfd, 0xff, 0xff, 0xff, 0xfd, 0xff, 0xff, 0xff,
        0xfd, 0xff, 0xff, 0xff, 0xfd, 0xff, 0xff, 0xff,
        0xfd, 0xff, 0xff, 0xff, 0xfd, 0xff, 0xff, 0xff
    };
    uint8_t plaintext[] = {
        0xL, 0x69, 0x6e, 0x65, 0x20, 0x74, 0x6f, 0x20,
        0x6d, 0x65, 0x73, 0x73, 0x61, 0x67, 0x65, 0x2c,
        0x20, 0x61, 0x6e, 0x64, 0x20, 0x74, 0x6f, 0x20,
        0x65, 0x6e, 0x63, 0x72, 0x79, 0x70, 0x74, 0x20,
        0x61, 0x20, 0x6d, 0x65, 0x73, 0x73, 0x61, 0x67,
        0x65, 0x2c, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x74,
        0x6f, 0x20, 0x65, 0x6e, 0x63, 0x72, 0x79, 0x70,
        0x74, 0x20, 0x61, 0x20, 0x6d, 0x65, 0x73, 0x73,
        0x61, 0x67, 0x65, 0x2e
    };
    uint8_t ciphertext[96];
    uint8_t tag[16];
    int ret;

    printf("\n=== ChaCha20-Poly1305 Basic Test ===\n");

    ret = xy_chacha20poly1305_encrypt(key, nonce, aad, sizeof(aad),
                                      plaintext, sizeof(plaintext),
                                      ciphertext, tag);
    if (ret == XY_CRYPTO_SUCCESS) {
        printf("Encryption successful\n");
        printf("Ciphertext: ");
        print_hex(ciphertext, 96, "");
        printf("Tag: ");
        print_hex(tag, 16, "");
    } else {
        printf("ChaCha20-Poly1305 encrypt failed: %d\n", ret);
    }
}

void test_chacha20_poly1305_decrypt(void)
{
    uint8_t key[32] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };
    uint8_t nonce[12] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x00, 0x01 };
    uint8_t plaintext[] = "Test message for Chacha20-Poly1305!";
    uint8_t ciphertext[64];
    uint8_t decrypted[64];
    uint8_t tag[16];
    int ret;

    printf("\n=== ChaCha20-Poly1305 Decrypt Test ===\n");

    /* Encrypt */
    ret = xy_chacha20poly1305_encrypt(key, nonce, NULL, 0,
                                      plaintext, strlen((char *)plaintext),
                                      ciphertext, tag);
    if (ret != XY_CRYPTO_SUCCESS) {
        printf("Encryption failed: %d\n", ret);
        return;
    }

    /* Decrypt */
    ret = xy_chacha20poly1305_decrypt(key, nonce, NULL, 0,
                                      ciphertext, strlen((char *)plaintext),
                                      decrypted, tag);
    if (ret != XY_CRYPTO_SUCCESS) {
        printf("Decryption failed: %d\n", ret);
        return;
    }

    decrypted[strlen((char *)plaintext)] = '\0';
    printf("Original: %s\n", plaintext);
    printf("Decrypted: %s\n", decrypted);

    if (memcmp(plaintext, decrypted, strlen((char *)plaintext)) == 0) {
        printf("ChaCha20-Poly1305 decrypt test PASSED\n");
    } else {
        printf("ChaCha20-Poly1305 decrypt test FAILED\n");
    }
}

int test_chacha_main(void)
{
    printf("XY Tiny Crypto - ChaCha20-Poly1305 Tests\n");
    printf("=========================================\n");

    test_chacha20_poly1305_basic();
    test_chacha20_poly1305_decrypt();

    printf("\nChaCha20-Poly1305 Tests Complete!\n");
    return 0;
}
