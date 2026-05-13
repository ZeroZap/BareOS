/**
 * @file example_sm4.c
 * @brief SM4 Encryption Example (ECB and CBC modes)
 */

#include <stdio.h>
#include <string.h>
#include "xy_sm4/xy_sm4.h"

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
static size_t pkcs7_pad(const uint8_t *input, size_t input_len, uint8_t *output, size_t block_size)
{
    size_t padded_len = ((input_len / block_size) + 1) * block_size;
    size_t padding = padded_len - input_len;

    memcpy(output, input, input_len);
    for (size_t i = input_len; i < padded_len; i++) {
        output[i] = (uint8_t)padding;
    }
    return padded_len;
}

/* Remove PKCS#7 padding */
static int pkcs7_unpad(const uint8_t *input, size_t input_len, uint8_t *output, size_t *output_len)
{
    if (input_len == 0) return -1;

    uint8_t padding = input[input_len - 1];
    if (padding == 0 || padding > 16 || padding > input_len) return -1;

    for (size_t i = 0; i < padding; i++) {
        if (input[input_len - 1 - i] != padding) return -1;
    }

    *output_len = input_len - padding;
    memcpy(output, input, *output_len);
    return 0;
}

int main(void)
{
    printf("=== SM4 Encryption Example ===\n\n");

    /* SM4 uses 128-bit (16 byte) key */
    uint8_t key[XY_SM4_KEY_SIZE] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10
    };

    printf("Key: ");
    print_hex("", key, XY_SM4_KEY_SIZE);
    printf("\n");

    /* ========== SM4 ECB Mode ========== */
    printf("--- SM4 ECB Mode ---\n");

    const char *plaintext = "Hello, SM4 ECB!";
    uint8_t padded[64] = {0};
    size_t padded_len = pkcs7_pad((const uint8_t *)plaintext, strlen(plaintext), padded, XY_SM4_BLOCK_SIZE);

    xy_sm4_ctx_t ctx;
    xy_sm4_init(&ctx, key, XY_SM4_KEY_SIZE);

    /* Encrypt block by block */
    uint8_t ciphertext[64] = {0};
    for (size_t i = 0; i < padded_len; i += XY_SM4_BLOCK_SIZE) {
        xy_sm4_encrypt_block(&ctx, padded + i, ciphertext + i);
    }

    printf("Plaintext: %s\n", plaintext);
    print_hex("Ciphertext", ciphertext, padded_len);

    /* Decrypt block by block */
    uint8_t decrypted[64] = {0};
    for (size_t i = 0; i < padded_len; i += XY_SM4_BLOCK_SIZE) {
        xy_sm4_decrypt_block(&ctx, ciphertext + i, decrypted + i);
    }

    size_t out_len = 0;
    uint8_t result[64] = {0};
    pkcs7_unpad(decrypted, padded_len, result, &out_len);
    result[out_len] = '\0';

    printf("Decrypted: %s\n", result);
    printf("Match: %s\n\n", strcmp(plaintext, (char *)result) == 0 ? "YES" : "NO");

    /* ========== SM4 CBC Mode ========== */
    printf("--- SM4 CBC Mode ---\n");

    uint8_t iv[XY_SM4_BLOCK_SIZE] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    };

    const char *plaintext2 = "Hello, SM4 CBC mode encryption!";
    size_t padded_len2 = pkcs7_pad((const uint8_t *)plaintext2, strlen(plaintext2), padded, XY_SM4_BLOCK_SIZE);

    xy_sm4_init(&ctx, key, XY_SM4_KEY_SIZE);

    /* Encrypt */
    uint8_t ciphertext2[64] = {0};
    xy_sm4_cbc_encrypt(&ctx, iv, padded, padded_len2, ciphertext2);

    printf("Plaintext: %s\n", plaintext2);
    print_hex("IV", iv, XY_SM4_BLOCK_SIZE);
    print_hex("Ciphertext", ciphertext2, padded_len2);

    /* Decrypt */
    uint8_t decrypted2[64] = {0};
    xy_sm4_init(&ctx, key, XY_SM4_KEY_SIZE);  /* Re-init for decryption */
    xy_sm4_cbc_decrypt(&ctx, iv, ciphertext2, padded_len2, decrypted2);

    size_t out_len2 = 0;
    uint8_t result2[64] = {0};
    pkcs7_unpad(decrypted2, padded_len2, result2, &out_len2);
    result2[out_len2] = '\0';

    printf("Decrypted: %s\n", result2);
    printf("Match: %s\n\n", strcmp(plaintext2, (char *)result2) == 0 ? "YES" : "NO");

    /* ========== Different Key Sizes (if supported) ========== */
    printf("--- Different Key Test ---\n");

    uint8_t key2[XY_SM4_KEY_SIZE] = {
        0xff, 0xee, 0xdd, 0xcc, 0xbb, 0xaa, 0x99, 0x88,
        0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00
    };

    xy_sm4_init(&ctx, key2, XY_SM4_KEY_SIZE);
    uint8_t short_plaintext[] = "SM4!";
    uint8_t enc_result[16] = {0};
    uint8_t dec_result[16] = {0};

    /* Pad manually for short input */
    size_t short_padded_len = pkcs7_pad(short_plaintext, strlen((char *)short_plaintext), padded, XY_SM4_BLOCK_SIZE);
    xy_sm4_encrypt_block(&ctx, padded, enc_result);

    printf("Short plaintext: %s\n", short_plaintext);
    print_hex("Encrypted", enc_result, XY_SM4_BLOCK_SIZE);

    xy_sm4_decrypt_block(&ctx, enc_result, dec_result);
    size_t out_short = 0;
    pkcs7_unpad(dec_result, XY_SM4_BLOCK_SIZE, result, &out_short);
    result[out_short] = '\0';
    printf("Decrypted: %s\n", result);
    printf("Match: %s\n\n", strcmp((char *)short_plaintext, (char *)result) == 0 ? "YES" : "NO");

    printf("SM4 example completed!\n");
    return 0;
}
