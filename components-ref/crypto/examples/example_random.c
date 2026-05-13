/**
 * @file example_random.c
 * @brief Random Number Generation Example
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

int main(void)
{
    printf("=== Random Number Generation Example ===\n\n");

    /* ========== Generate Random Bytes ========== */
    printf("--- Random Bytes ---\n");

    uint8_t random_bytes[32] = {0};

    if (xy_random_bytes(random_bytes, sizeof(random_bytes)) == XY_CRYPTO_SUCCESS) {
        printf("Generated %zu random bytes: ", sizeof(random_bytes));
        print_hex("", random_bytes, sizeof(random_bytes));
    } else {
        printf("Failed to generate random bytes\n");
        return -1;
    }
    printf("\n");

    /* ========== Generate Random uint32 ========== */
    printf("--- Random uint32 Values ---\n");

    printf("Generated 10 random uint32 values:\n  ");
    for (int i = 0; i < 10; i++) {
        printf("%10u ", xy_random_uint32());
        if ((i + 1) % 5 == 0) printf("\n  ");
    }
    printf("\n\n");

    /* ========== Generate Multiple Times (showing uniqueness) ========== */
    printf("--- Uniqueness Test ---\n");

    uint8_t prev_bytes[16] = {0};
    uint8_t curr_bytes[16] = {0};
    int unique_count = 0;

    printf("Generating 5 random 16-byte sequences:\n");
    for (int i = 0; i < 5; i++) {
        xy_random_bytes(curr_bytes, sizeof(curr_bytes));
        printf("  #%d: ", i + 1);
        print_hex("", curr_bytes, sizeof(curr_bytes));

        if (i > 0 && memcmp(prev_bytes, curr_bytes, sizeof(curr_bytes)) != 0) {
            unique_count++;
        }
        memcpy(prev_bytes, curr_bytes, sizeof(prev_bytes));
    }
    printf("At least %d out of 4 were unique (expected: all unique)\n\n", unique_count);

    /* ========== Random for Cryptographic Use Cases ========== */
    printf("--- Cryptographic Use Cases ---\n");

    /* Generate AES key */
    uint8_t aes_key[XY_AES_KEY_SIZE_128] = {0};
    xy_random_bytes(aes_key, sizeof(aes_key));
    printf("Random AES-128 key: ");
    print_hex("", aes_key, sizeof(aes_key));

    /* Generate IV/Nonce */
    uint8_t iv[XY_AES_BLOCK_SIZE] = {0};
    xy_random_bytes(iv, sizeof(iv));
    printf("Random IV: ");
    print_hex("", iv, sizeof(iv));

    /* Generate salt */
    uint8_t salt[16] = {0};
    xy_random_bytes(salt, sizeof(salt));
    printf("Random salt: ");
    print_hex("", salt, sizeof(salt));

    /* Generate nonce for HMAC */
    uint8_t nonce[8] = {0};
    xy_random_bytes(nonce, sizeof(nonce));
    printf("Random nonce: ");
    print_hex("", nonce, sizeof(nonce));
    printf("\n");

    /* ========== Random in Range ========== */
    printf("--- Random in Range (derived) ---\n");

    printf("Random values in range [0, 99]:\n  ");
    for (int i = 0; i < 10; i++) {
        uint32_t r = xy_random_uint32();
        printf("%2u ", r % 100);
        if ((i + 1) % 10 == 0) printf("\n  ");
    }
    printf("\n\n");

    /* ========== Fill Buffer ========== */
    printf("--- Fill Buffer ---\n");

    uint8_t buffer[16];
    xy_random_bytes(buffer, sizeof(buffer));
    printf("Buffer filled with random data: ");
    print_hex("", buffer, sizeof(buffer));

    /* Zero buffer then fill */
    memset(buffer, 0, sizeof(buffer));
    xy_random_bytes(buffer, sizeof(buffer));
    printf("After zero and fill: ");
    print_hex("", buffer, sizeof(buffer));

    printf("\nRandom number generation example completed!\n");
    return 0;
}
