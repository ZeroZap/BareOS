/**
 * @file example_sm3.c
 * @brief SM3 Hash Example
 */

#include <stdio.h>
#include <string.h>
#include "xy_sm3/xy_sm3.h"

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
    printf("=== SM3 Hash Example ===\n\n");

    /* Test data */
    const char *test_data = "Hello, SM3! This is a test message for SM3 hash.";
    size_t data_len = strlen(test_data);

    printf("Input data: %s\n", test_data);
    printf("Data length: %zu bytes\n\n", data_len);

    /* ========== One-shot SM3 Hash ========== */
    printf("--- SM3 Hash (one-shot) ---\n");

    uint8_t sm3_digest[XY_SM3_DIGEST_SIZE];

    if (xy_sm3_hash((const uint8_t *)test_data, data_len, sm3_digest) != XY_CRYPTO_SUCCESS) {
        printf("SM3 hash failed\n");
        return -1;
    }

    print_hex("SM3 Hash", sm3_digest, XY_SM3_DIGEST_SIZE);
    printf("\n");

    /* ========== Incremental SM3 Hash ========== */
    printf("--- SM3 Hash (incremental) ---\n");

    xy_sm3_ctx_t ctx;
    xy_sm3_init(&ctx);

    /* Process in chunks to demonstrate incremental hashing */
    size_t chunk_size = data_len / 3;
    size_t offset = 0;

    printf("Processing message in 3 chunks:\n");

    xy_sm3_update(&ctx, (const uint8_t *)test_data, chunk_size);
    printf("  Chunk 1: %zu bytes\n", chunk_size);
    offset += chunk_size;

    xy_sm3_update(&ctx, (const uint8_t *)test_data + offset, chunk_size);
    printf("  Chunk 2: %zu bytes\n", chunk_size);
    offset += chunk_size;

    xy_sm3_update(&ctx, (const uint8_t *)test_data + offset, data_len - offset);
    printf("  Chunk 3: %zu bytes\n", data_len - offset);

    uint8_t sm3_digest_incremental[XY_SM3_DIGEST_SIZE];
    xy_sm3_final(&ctx, sm3_digest_incremental);
    print_hex("SM3 Hash (incremental)", sm3_digest_incremental, XY_SM3_DIGEST_SIZE);

    /* Verify both methods produce same result */
    printf("Results match: %s\n\n",
           memcmp(sm3_digest, sm3_digest_incremental, XY_SM3_DIGEST_SIZE) == 0 ? "YES" : "NO");

    /* ========== Empty String Hash ========== */
    printf("--- Empty String Hash ---\n");

    uint8_t empty_digest[XY_SM3_DIGEST_SIZE];
    xy_sm3_hash((const uint8_t *)"", 0, empty_digest);
    print_hex("SM3 of empty string", empty_digest, XY_SM3_DIGEST_SIZE);
    printf("\n");

    /* ========== Known Test Vectors ========== */
    printf("--- Known Test Vectors ---\n");

    /* Test vector: "abc" */
    const char *abc = "abc";
    uint8_t abc_digest[XY_SM3_DIGEST_SIZE];
    xy_sm3_hash((const uint8_t *)abc, strlen(abc), abc_digest);
    printf("SM3(\"abc\") = ");
    print_hex("", abc_digest, XY_SM3_DIGEST_SIZE);

    /* Test vector: "abcabcdabcabcdabcabcdabcabcdabcabcdabcabcdabcabcdabcabcdabcabcdabcabcdabcabcdabcabcd" */
    const char *long_msg = "abcabcdabcabcdabcabcdabcabcdabcabcdabcabcdabcabcdabcabcdabcabcdabcabcdabcabcdabcabcd";
    uint8_t long_digest[XY_SM3_DIGEST_SIZE];
    xy_sm3_hash((const uint8_t *)long_msg, strlen(long_msg), long_digest);
    printf("SM3(512-bit repeated \"abc\") = ");
    print_hex("", long_digest, XY_SM3_DIGEST_SIZE);
    printf("\n");

    /* ========== Binary Data Hash ========== */
    printf("--- Binary Data Hash ---\n");

    uint8_t binary_data[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                             0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
    uint8_t binary_digest[XY_SM3_DIGEST_SIZE];

    printf("Binary data: ");
    print_hex("", binary_data, sizeof(binary_data));

    xy_sm3_hash(binary_data, sizeof(binary_data), binary_digest);
    printf("SM3 hash: ");
    print_hex("", binary_digest, XY_SM3_DIGEST_SIZE);

    printf("\nSM3 example completed!\n");
    return 0;
}
