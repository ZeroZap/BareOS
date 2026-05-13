/**
 * @file example_aes.c
 * @brief AES Encryption/Decryption Example (ECB and CBC modes)
 */

#include <stdio.h>
#include <string.h>
#include "xy_tiny_crypto.h"

/* Print buffer as hex */
static void print_hex(const char *label, const uint8_t *data, size_t len)
{
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) {
        printf("%02x", data[i]);
    }
    printf("\n");
}

/* PKCS#7 padding */
static void pkcs7_pad(const uint8_t *input, size_t input_len, uint8_t *output, size_t block_size)
{
    size_t padded_len = ((input_len / block_size) + 1) * block_size;
    size_t padding = padded_len - input_len;

    memcpy(output, input, input_len);
    for (size_t i = input_len; i < padded_len; i++) {
        output[i] = (uint8_t)padding;
    }
}

/* Remove PKCS#7 padding */
static int pkcs7_unpad(const uint8_t *input, size_t input_len, uint8_t *output, size_t *output_len)
{
    if (input_len == 0) return -1;

    uint8_t padding = input[input_len - 1];
    if (padding == 0 || padding > 16 || padding > input_len) return -1;

    for (uint8_t i = 0; i < padding; i++) {
        if (input[input_len - 1 - i] != padding) return -1;
    }

    *output_len = input_len - padding;
    memcpy(output, input, *output_len);
    return 0;
}

/* AES ECB mode encryption */
static int example_aes_ecb(void)
{
    printf("\n--- AES ECB Mode ---\n");

    /* 128-bit key */
    uint8_t key[XY_AES_KEY_SIZE_128] = {
        0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
        0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
    };

    const char *plaintext = "Hello, AES ECB!";
    uint8_t padded[64] = {0};
    size_t padded_len = ((strlen(plaintext) / XY_AES_BLOCK_SIZE) + 1) * XY_AES_BLOCK_SIZE;

    pkcs7_pad((const uint8_t *)plaintext, strlen(plaintext), padded, XY_AES_BLOCK_SIZE);

    xy_aes_ctx_t ctx;
    if (xy_aes_init(&ctx, key, XY_AES_KEY_SIZE_128) != XY_CRYPTO_SUCCESS) {
        printf("AES init failed\n");
        return -1;
    }

    /* Encrypt block by block */
    uint8_t ciphertext[64] = {0};
    for (size_t i = 0; i < padded_len; i += XY_AES_BLOCK_SIZE) {
        if (xy_aes_encrypt_block(&ctx, padded + i, ciphertext + i) != XY_CRYPTO_SUCCESS) {
            printf("AES encrypt block failed\n");
            return -1;
        }
    }

    print_hex("Plaintext", (const uint8_t *)plaintext, strlen(plaintext));
    print_hex("Ciphertext", ciphertext, padded_len);

    /* Decrypt */
    uint8_t decrypted[64] = {0};
    for (size_t i = 0; i < padded_len; i += XY_AES_BLOCK_SIZE) {
        if (xy_aes_decrypt_block(&ctx, ciphertext + i, decrypted + i) != XY_CRYPTO_SUCCESS) {
            printf("AES decrypt block failed\n");
            return -1;
        }
    }

    size_t out_len = 0;
    uint8_t result[64] = {0};
    pkcs7_unpad(decrypted, padded_len, result, &out_len);
    result[out_len] = '\0';

    printf("Decrypted: %s\n", result);
    printf("Match: %s\n", strcmp(plaintext, (char *)result) == 0 ? "YES" : "NO");

    return 0;
}

/* AES CBC mode encryption */
static int example_aes_cbc(void)
{
    printf("\n--- AES CBC Mode ---\n");

    /* 128-bit key and IV */
    uint8_t key[XY_AES_KEY_SIZE_128] = {
        0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
        0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
    };

    uint8_t iv[XY_AES_BLOCK_SIZE] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    };

    const char *plaintext = "Hello, AES CBC mode encryption!";
    uint8_t padded[64] = {0};
    size_t padded_len = ((strlen(plaintext) / XY_AES_BLOCK_SIZE) + 1) * XY_AES_BLOCK_SIZE;

    pkcs7_pad((const uint8_t *)plaintext, strlen(plaintext), padded, XY_AES_BLOCK_SIZE);

    xy_aes_ctx_t ctx;
    if (xy_aes_init(&ctx, key, XY_AES_KEY_SIZE_128) != XY_CRYPTO_SUCCESS) {
        printf("AES init failed\n");
        return -1;
    }

    /* Encrypt */
    uint8_t ciphertext[64] = {0};
    if (xy_aes_cbc_encrypt(&ctx, iv, padded, padded_len, ciphertext) != XY_CRYPTO_SUCCESS) {
        printf("AES CBC encrypt failed\n");
        return -1;
    }

    print_hex("Plaintext", (const uint8_t *)plaintext, strlen(plaintext));
    print_hex("Ciphertext", ciphertext, padded_len);

    /* Decrypt */
    uint8_t decrypted[64] = {0};
    if (xy_aes_cbc_decrypt(&ctx, iv, ciphertext, padded_len, decrypted) != XY_CRYPTO_SUCCESS) {
        printf("AES CBC decrypt failed\n");
        return -1;
    }

    size_t out_len = 0;
    uint8_t result[64] = {0};
    pkcs7_unpad(decrypted, padded_len, result, &out_len);
    result[out_len] = '\0';

    printf("Decrypted: %s\n", result);
    printf("Match: %s\n", strcmp(plaintext, (char *)result) == 0 ? "YES" : "NO");

    return 0;
}

int main(void)
{
    printf("=== AES Encryption Example ===\n");

    if (example_aes_ecb() != 0) {
        printf("ECB mode failed\n");
        return -1;
    }

    if (example_aes_cbc() != 0) {
        printf("CBC mode failed\n");
        return -1;
    }

    printf("\nAES example completed successfully!\n");
    return 0;
}
