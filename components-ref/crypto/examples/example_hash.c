/**
 * @file example_hash.c
 * @brief MD5 and SHA256 Hash Example
 */

#include <stdio.h>
#include <string.h>
#include "xy_tiny_crypto.h"

/* Print hash as hex string */
static void print_hash(const char *label, const uint8_t *hash, size_t len)
{
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) {
        printf("%02x", hash[i]);
    }
    printf("\n");
}

int main(void)
{
    printf("=== Hash Algorithm Example ===\n\n");

    /* Test data */
    const char *test_data = "Hello, World! This is a test message for hashing.";
    size_t data_len = strlen(test_data);

    printf("Input data: %s\n", test_data);
    printf("Data length: %zu bytes\n\n", data_len);

    /* ========== MD5 Hash ========== */
    printf("--- MD5 Hash ---\n");

    /* One-shot MD5 hash */
    uint8_t md5_digest[XY_MD5_DIGEST_SIZE];
    if (xy_md5_hash((const uint8_t *)test_data, data_len, md5_digest) == XY_CRYPTO_SUCCESS) {
        print_hash("MD5 (one-shot)", md5_digest, XY_MD5_DIGEST_SIZE);
    } else {
        printf("MD5 hash failed\n");
    }

    /* Incremental MD5 hash */
    xy_md5_ctx_t md5_ctx;
    xy_md5_init(&md5_ctx);

    /* Split into chunks to demonstrate incremental hashing */
    size_t chunk1_len = data_len / 3;
    size_t chunk2_len = data_len / 3;
    size_t chunk3_len = data_len - chunk1_len - chunk2_len;

    xy_md5_update(&md5_ctx, (const uint8_t *)test_data, chunk1_len);
    xy_md5_update(&md5_ctx, (const uint8_t *)test_data + chunk1_len, chunk2_len);
    xy_md5_update(&md5_ctx, (const uint8_t *)test_data + chunk1_len + chunk2_len, chunk3_len);

    uint8_t md5_digest_incremental[XY_MD5_DIGEST_SIZE];
    xy_md5_final(&md5_ctx, md5_digest_incremental);
    print_hash("MD5 (incremental)", md5_digest_incremental, XY_MD5_DIGEST_SIZE);

    /* Verify both methods produce same result */
    printf("MD5 results match: %s\n\n",
           memcmp(md5_digest, md5_digest_incremental, XY_MD5_DIGEST_SIZE) == 0 ? "YES" : "NO");

    /* ========== SHA-256 Hash ========== */
    printf("--- SHA-256 Hash ---\n");

    /* One-shot SHA-256 hash using xy_tiny_crypto.h wrapper */
    uint8_t sha256_digest[XY_SHA256_DIGEST_SIZE];
    if (xy_sha256_hash((const uint8_t *)test_data, data_len, sha256_digest) == XY_CRYPTO_SUCCESS) {
        print_hash("SHA256 (one-shot)", sha256_digest, XY_SHA256_DIGEST_SIZE);
    } else {
        printf("SHA256 hash failed\n");
    }

    /* Incremental SHA-256 hash using xy_sha256.h API */
    xy_sha256_ctx_t sha_ctx;
    xy_sha256_init(&sha_ctx);

    /* Process in chunks */
    xy_sha256_update(&sha_ctx, (const uint8_t *)test_data, chunk1_len);
    xy_sha256_update(&sha_ctx, (const uint8_t *)test_data + chunk1_len, chunk2_len);
    xy_sha256_update(&sha_ctx, (const uint8_t *)test_data + chunk1_len + chunk2_len, chunk3_len);

    uint8_t sha256_digest_incremental[XY_SHA256_DIGEST_SIZE];
    xy_sha256_finish(&sha_ctx, sha256_digest_incremental);
    print_hash("SHA256 (incremental)", sha256_digest_incremental, XY_SHA256_DIGEST_SIZE);

    /* Verify both methods produce same result */
    printf("SHA256 results match: %s\n\n",
           memcmp(sha256_digest, sha256_digest_incremental, XY_SHA256_DIGEST_SIZE) == 0 ? "YES" : "NO");

    /* ========== Empty string hash ========== */
    printf("--- Empty String Hash ---\n");
    uint8_t empty_md5[XY_MD5_DIGEST_SIZE];
    uint8_t empty_sha256[XY_SHA256_DIGEST_SIZE];

    xy_md5_hash((const uint8_t *)"", 0, empty_md5);
    xy_sha256_hash((const uint8_t *)"", 0, empty_sha256);

    print_hash("MD5 of empty string", empty_md5, XY_MD5_DIGEST_SIZE);
    print_hash("SHA256 of empty string", empty_sha256, XY_SHA256_DIGEST_SIZE);

    printf("\nHash example completed!\n");
    return 0;
}
