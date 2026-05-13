/**
 * @file example_sm2.c
 * @brief SM2 Sign/Verify/Encrypt Example
 */

#include <stdio.h>
#include <string.h>
#include "xy_sm2/xy_sm2.h"

/* Print buffer as hex */
static void print_hex(const char *label, const uint8_t *data, size_t len)
{
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) {
        printf("%02x", data[i]);
    }
    printf("\n");
}

int main(void)
{
    printf("=== SM2 Sign/Verify/Encrypt Example ===\n\n");

    xy_sm2_ctx_t ctx;
    uint8_t signature[XY_SM2_SIGNATURE_SIZE];

    /* Default user ID as specified in GM/T 0003-2012 */
    const char *user_id = "1234567812345678";  /* 16 bytes default ID */

    /* Test message */
    const char *message = "Hello, SM2! This is a test message for digital signature.";
    printf("User ID: %s\n", user_id);
    printf("Message: %s\n", message);
    printf("Message length: %zu bytes\n\n", strlen(message));

    /* ========== Generate SM2 Key Pair ========== */
    printf("--- Generate SM2 Key Pair ---\n");

    if (xy_sm2_generate_key(&ctx) != 0) {
        printf("SM2 key generation failed\n");
        return -1;
    }

    printf("Key generation: SUCCESS\n");
    print_hex("Private Key", ctx.private_key, XY_SM2_PRIVATE_KEY_SIZE);
    print_hex("Public Key X", ctx.public_key, 32);
    print_hex("Public Key Y", ctx.public_key + 32, 32);
    printf("\n");

    /* ========== SM2 Sign ========== */
    printf("--- SM2 Sign ---\n");

    if (xy_sm2_sign(&ctx, (const uint8_t *)user_id, strlen(user_id),
                    (const uint8_t *)message, strlen(message),
                    signature) != 0) {
        printf("SM2 sign failed\n");
        return -1;
    }

    printf("Signing: SUCCESS\n");
    print_hex("Signature R", signature, 32);
    print_hex("Signature S", signature + 32, 32);
    printf("\n");

    /* ========== SM2 Verify ========== */
    printf("--- SM2 Verify ---\n");

    int verify_result = xy_sm2_verify(&ctx, (const uint8_t *)user_id, strlen(user_id),
                                      (const uint8_t *)message, strlen(message),
                                      signature);

    printf("Verification: %s\n", verify_result == 0 ? "SUCCESS" : "FAILED");

    /* Verify with tampered message (should fail) */
    const char *tampered_message = "Hello, SM2! This message has been tampered!";
    verify_result = xy_sm2_verify(&ctx, (const uint8_t *)user_id, strlen(user_id),
                                  (const uint8_t *)tampered_message, strlen(tampered_message),
                                  signature);
    printf("Verify tampered message: %s (expected: FAILED)\n",
           verify_result == 0 ? "SUCCESS (unexpected)" : "FAILED (expected)");
    printf("\n");

    /* ========== SM2 Encrypt ========== */
    printf("--- SM2 Encrypt ---\n");

    const char *plaintext = "Secret message for SM2 encryption!";
    uint8_t ciphertext[256] = {0};
    size_t cipher_len = 0;

    /* Encrypt using recipient's public key (self) */
    if (xy_sm2_encrypt(&ctx, ctx.public_key,
                       (const uint8_t *)plaintext, strlen(plaintext),
                       ciphertext, &cipher_len) != 0) {
        printf("SM2 encrypt failed\n");
        return -1;
    }

    printf("Plaintext: %s\n", plaintext);
    printf("Plaintext length: %zu bytes\n", strlen(plaintext));
    print_hex("Ciphertext", ciphertext, cipher_len);
    printf("Ciphertext length: %zu bytes\n\n", cipher_len);

    /* ========== SM2 Decrypt ========== */
    printf("--- SM2 Decrypt ---\n");

    uint8_t decrypted[256] = {0};
    size_t decrypted_len = 0;

    if (xy_sm2_decrypt(&ctx, ctx.public_key,
                       ciphertext, cipher_len,
                       decrypted, &decrypted_len) != 0) {
        printf("SM2 decrypt failed\n");
        return -1;
    }

    decrypted[decrypted_len] = '\0';
    printf("Decrypted: %s\n", decrypted);
    printf("Decrypted length: %zu bytes\n", decrypted_len);
    printf("Match: %s\n",
           (decrypted_len == strlen(plaintext)) &&
           (memcmp(plaintext, decrypted, decrypted_len) == 0) ? "YES" : "NO");
    printf("\n");

    /* ========== Using preset keys ========== */
    printf("--- Using Preset Keys ---\n");

    xy_sm2_ctx_t ctx2;

    /* Set predefined private key (for testing only) */
    uint8_t test_private_key[XY_SM2_PRIVATE_KEY_SIZE] = {
        0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0,
        0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0,
        0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0,
        0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0
    };
    uint8_t test_public_key[XY_SM2_PUBLIC_KEY_SIZE] = {
        0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89,
        0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89,
        0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89,
        0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89,
        0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89,
        0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89,
        0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89,
        0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89
    };

    xy_sm2_set_private_key(&ctx2, test_private_key);
    xy_sm2_set_public_key(&ctx2, test_public_key);

    printf("Using preset keys (test purposes only)\n");
    print_hex("Private Key", ctx2.private_key, XY_SM2_PRIVATE_KEY_SIZE);
    printf("Note: Public key derived from private key in full implementation\n\n");

    printf("SM2 example completed!\n");
    return 0;
}
