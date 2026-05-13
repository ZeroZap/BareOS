/**
 * @file test_xy_sm2.c
 * @brief SM2 algorithm tests
 * @version 1.0.0
 * @date 2026-04-30
 */

#include <stdio.h>
#include <string.h>
#include "xy_sm2.h"
#include "xy_sm3.h"
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

void test_sm2_keygen(void)
{
    xy_sm2_ctx_t ctx;
    int ret;

    printf("\n=== SM2 Key Generation Test ===\n");

    ret = xy_sm2_generate_key(&ctx);
    if (ret == XY_CRYPTO_SUCCESS) {
        printf("SM2 key pair generated successfully\n");
        print_hex(ctx.public_key, 32, "Public Key X");
        print_hex(ctx.public_key + 32, 32, "Public Key Y");
        printf("Private Key: [32 bytes hidden]\n");
    } else {
        printf("SM2 key generation failed: %d\n", ret);
    }
}

void test_sm2_sign_verify(void)
{
    xy_sm2_ctx_t ctx;
    const uint8_t *id = (const uint8_t *)"test_user@example.com";
    size_t id_len = strlen((const char *)id);
    const char *message = "The quick brown fox jumps over the lazy dog";
    uint8_t signature[XY_SM2_SIGNATURE_SIZE];
    int ret;

    printf("\n=== SM2 Sign/Verify Test ===\n");

    /* Generate key pair */
    ret = xy_sm2_generate_key(&ctx);
    if (ret != XY_CRYPTO_SUCCESS) {
        printf("Key generation failed\n");
        return;
    }

    /* Sign */
    ret = xy_sm2_sign(&ctx, id, id_len, (const uint8_t *)message, strlen(message), signature);
    if (ret == XY_CRYPTO_SUCCESS) {
        printf("SM2 signature generated successfully\n");
        print_hex(signature, 32, "Signature R");
        print_hex(signature + 32, 32, "Signature S");
    } else {
        printf("SM2 sign failed: %d\n", ret);
        return;
    }

    /* Verify */
    ret = xy_sm2_verify(&ctx, id, id_len, (const uint8_t *)message, strlen(message), signature);
    if (ret == XY_CRYPTO_SUCCESS) {
        printf("SM2 signature verified successfully\n");
    } else {
        printf("SM2 verify failed: %d\n", ret);
    }
}

void test_sm2_encrypt_decrypt(void)
{
    xy_sm2_ctx_t ctx;
    const char *plaintext = "Hello, SM2!";
    uint8_t ciphertext[128];
    uint8_t decrypted[128];
    size_t cipher_len = sizeof(ciphertext);
    size_t plain_len = sizeof(decrypted);
    int ret;

    printf("\n=== SM2 Encrypt/Decrypt Test ===\n");

    /* Generate key pair */
    ret = xy_sm2_generate_key(&ctx);
    if (ret != XY_CRYPTO_SUCCESS) {
        printf("Key generation failed\n");
        return;
    }

    /* Encrypt */
    ret = xy_sm2_ecdh_encrypt(&ctx, (const uint8_t *)plaintext, strlen(plaintext), ciphertext);
    if (ret == XY_CRYPTO_SUCCESS) {
        printf("SM2 encryption successful, ciphertext len: %zu\n", 97 + strlen(plaintext));
        print_hex(ciphertext, 64, "C1 (ephemeral pubkey)");
    } else {
        printf("SM2 encrypt failed: %d\n", ret);
        return;
    }

    /* Decrypt */
    ret = xy_sm2_ecdh_decrypt(&ctx, ciphertext, 97 + strlen(plaintext), decrypted, &plain_len);
    if (ret == XY_CRYPTO_SUCCESS) {
        decrypted[plain_len] = '\0';
        printf("SM2 decryption successful\n");
        printf("Decrypted: %s\n", decrypted);

        if (strcmp(plaintext, (char *)decrypted) == 0) {
            printf("SM2 encrypt/decrypt test PASSED\n");
        } else {
            printf("SM2 encrypt/decrypt test FAILED\n");
        }
    } else {
        printf("SM2 decrypt failed: %d\n", ret);
    }
}

int test_sm2_main(void)
{
    printf("XY Tiny Crypto - SM2 Tests\n");
    printf("===========================\n");

    test_sm2_keygen();
    test_sm2_sign_verify();
    test_sm2_encrypt_decrypt();

    printf("\nSM2 Tests Complete!\n");
    return 0;
}
