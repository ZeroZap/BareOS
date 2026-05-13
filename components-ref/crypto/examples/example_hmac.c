/**
 * @file example_hmac.c
 * @brief HMAC-SHA256 Usage Example
 */

#include <stdio.h>
#include <string.h>
#include "xy_tiny_crypto.h"

/* Print HMAC as hex string */
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
    printf("=== HMAC-SHA256 Example ===\n\n");

    /* Test key and message */
    const char *key = "my_secret_key_1234567890";
    const char *message = "Hello, HMAC! This is a test message.";

    printf("Key: %s\n", key);
    printf("Key length: %zu bytes\n", strlen(key));
    printf("Message: %s\n", message);
    printf("Message length: %zu bytes\n\n", strlen(message));

    /* ========== One-shot HMAC-SHA256 ========== */
    printf("--- HMAC-SHA256 (one-shot) ---\n");

    uint8_t hmac_result[XY_SHA256_DIGEST_SIZE];

    if (xy_hmac_sha256((const uint8_t *)key, strlen(key),
                       (const uint8_t *)message, strlen(message),
                       hmac_result) == XY_CRYPTO_SUCCESS) {
        print_hex("HMAC-SHA256", hmac_result, XY_SHA256_DIGEST_SIZE);
    } else {
        printf("HMAC-SHA256 failed\n");
        return -1;
    }

    /* ========== Different keys produce different HMACs ========== */
    printf("\n--- Different Keys ---\n");

    const char *key2 = "different_key";
    uint8_t hmac_result2[XY_SHA256_DIGEST_SIZE];

    xy_hmac_sha256((const uint8_t *)key2, strlen(key2),
                   (const uint8_t *)message, strlen(message),
                   hmac_result2);

    print_hex("HMAC (key1)", hmac_result, XY_SHA256_DIGEST_SIZE);
    print_hex("HMAC (key2)", hmac_result2, XY_SHA256_DIGEST_SIZE);
    printf("Same message, different keys: %s\n",
           memcmp(hmac_result, hmac_result2, XY_SHA256_DIGEST_SIZE) == 0 ? "SAME (unexpected)" : "DIFFERENT (expected)");

    /* ========== Different messages produce different HMACs ========== */
    printf("\n--- Different Messages ---\n");

    const char *message2 = "Different message";
    uint8_t hmac_result3[XY_SHA256_DIGEST_SIZE];

    xy_hmac_sha256((const uint8_t *)key, strlen(key),
                   (const uint8_t *)message2, strlen(message2),
                   hmac_result3);

    print_hex("HMAC (message1)", hmac_result, XY_SHA256_DIGEST_SIZE);
    print_hex("HMAC (message2)", hmac_result3, XY_SHA256_DIGEST_SIZE);
    printf("Same key, different messages: %s\n",
           memcmp(hmac_result, hmac_result3, XY_SHA256_DIGEST_SIZE) == 0 ? "SAME (unexpected)" : "DIFFERENT (expected)");

    /* ========== Empty message HMAC ========== */
    printf("\n--- Empty Message ---\n");

    uint8_t hmac_empty[XY_SHA256_DIGEST_SIZE];
    xy_hmac_sha256((const uint8_t *)key, strlen(key),
                   (const uint8_t *)"", 0,
                   hmac_empty);
    print_hex("HMAC of empty message", hmac_empty, XY_SHA256_DIGEST_SIZE);

    /* ========== Binary data example ========== */
    printf("\n--- Binary Data Example ---\n");

    uint8_t binary_key[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                            0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10};
    uint8_t binary_data[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                             0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
    uint8_t hmac_binary[XY_SHA256_DIGEST_SIZE];

    xy_hmac_sha256(binary_key, sizeof(binary_key),
                   binary_data, sizeof(binary_data),
                   hmac_binary);

    printf("Binary key length: %zu bytes\n", sizeof(binary_key));
    printf("Binary data length: %zu bytes\n", sizeof(binary_data));
    print_hex("HMAC (binary)", hmac_binary, XY_SHA256_DIGEST_SIZE);

    printf("\nHMAC-SHA256 example completed!\n");
    return 0;
}
