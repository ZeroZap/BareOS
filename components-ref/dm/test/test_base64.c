/**
 * @file test_base64.c
 * @brief Test suite for XY Base64 encode/decode operations
 *
 * @author XY Team
 * @date 2025
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "xy_base64.h"

/* Test buffer sizes */
#define ENCODE_INPUT_SIZE 64
#define ENCODE_OUTPUT_SIZE 128
#define DECODE_INPUT_SIZE 128
#define DECODE_OUTPUT_SIZE 96

/* Helper macros */
#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            printf("  [FAIL] %s\n", msg); \
            return false; \
        } \
        printf("  [PASS] %s\n", msg); \
        return true; \
    } while(0)

#define RUN_TEST(test_func) \
    do { \
        printf("\n"); \
        if (test_func()) { \
            passed++; \
        } else { \
            failed++; \
        } \
        total++; \
    } while(0)

/* Print test header */
static void print_test_header(const char *test_name)
{
    printf("\n=== %s ===\n", test_name);
}

/* Test vectors */
static const struct {
    const char *input;
    const char *expected_output;
    uint32_t input_len;
} base64_test_vectors[] = {
    /* RFC 4648 test vectors */
    { "", "", 0 },
    { "f", "Zg==", 1 },
    { "fo", "Zm8=", 2 },
    { "foo", "Zm9v", 3 },
    { "foob", "Zm9vYg==", 4 },
    { "fooba", "Zm9vYmE=", 5 },
    { "foobar", "Zm9vYmFy", 6 },
    /* Additional test cases */
    { "Hello, World!", "SGVsbG8sIFdvcmxkIQ==", 13 },
    { "The quick brown fox jumps over the lazy dog", "VGhlIHF1aWNrIGJyb3duIGZveCBqdW1wcyBvdmVyIHRoZSBsYXp5IGRvZw==", 43 },
    { "ABC", "QUJD", 3 },
    { "AB", "QUI=", 2 },
    { "A", "QQ==", 1 },
};

/* ============ Test Cases ============ */

/**
 * @brief Test empty string encoding
 */
static bool test_encode_empty(void)
{
    print_test_header("Test: Encode Empty String");

    uint8_t input[] = "";
    uint8_t output[ENCODE_OUTPUT_SIZE];

    uint32_t encoded_len = xy_base64_encode(input, 0, output);

    TEST_ASSERT(encoded_len == 0, "Empty string should encode to 0 bytes");
}

/**
 * @brief Test single character encoding
 */
static bool test_encode_single_char(void)
{
    print_test_header("Test: Encode Single Character");

    uint8_t input[] = "A";
    uint8_t output[ENCODE_OUTPUT_SIZE];

    uint32_t encoded_len = xy_base64_encode(input, 1, output);

    TEST_ASSERT(encoded_len == 4, "Single char should encode to 4 bytes (QQ==)");
    TEST_ASSERT(memcmp(output, "QQ==", 4) == 0, "Output should be 'QQ=='");
}

/**
 * @brief Test two character encoding
 */
static bool test_encode_two_chars(void)
{
    print_test_header("Test: Encode Two Characters");

    uint8_t input[] = "AB";
    uint8_t output[ENCODE_OUTPUT_SIZE];

    uint32_t encoded_len = xy_base64_encode(input, 2, output);

    TEST_ASSERT(encoded_len == 4, "Two chars should encode to 4 bytes (QUI=)");
    TEST_ASSERT(memcmp(output, "QUI=", 4) == 0, "Output should be 'QUI='");
}

/**
 * @brief Test three character encoding
 */
static bool test_encode_three_chars(void)
{
    print_test_header("Test: Encode Three Characters");

    uint8_t input[] = "ABC";
    uint8_t output[ENCODE_OUTPUT_SIZE];

    uint32_t encoded_len = xy_base64_encode(input, 3, output);

    TEST_ASSERT(encoded_len == 4, "Three chars should encode to 4 bytes (QUJD)");
    TEST_ASSERT(memcmp(output, "QUJD", 4) == 0, "Output should be 'QUJD'");
}

/**
 * @brief Test RFC 4648 test vectors
 */
static bool test_encode_rfc_vectors(void)
{
    print_test_header("Test: Encode RFC 4648 Vectors");

    uint8_t output[ENCODE_OUTPUT_SIZE];
    uint32_t i;

    for (i = 0; i < sizeof(base64_test_vectors) / sizeof(base64_test_vectors[0]); i++) {
        memset(output, 0, sizeof(output));
        uint32_t encoded_len = xy_base64_encode(
            (const uint8_t *)base64_test_vectors[i].input,
            base64_test_vectors[i].input_len,
            output
        );

        uint32_t expected_len = strlen(base64_test_vectors[i].expected_output);

        if (encoded_len != expected_len) {
            printf("  [FAIL] Vector %u: length mismatch (expected %u, got %u)\n",
                   i, expected_len, encoded_len);
            return false;
        }

        if (memcmp(output, base64_test_vectors[i].expected_output, expected_len) != 0) {
            printf("  [FAIL] Vector %u: output mismatch\n", i);
            printf("    Expected: %s\n", base64_test_vectors[i].expected_output);
            printf("    Got:      %s\n", output);
            return false;
        }
    }

    printf("  [PASS] All %u RFC 4648 vectors encoded correctly\n", i);
    return true;
}

/**
 * @brief Test decoding empty string
 */
static bool test_decode_empty(void)
{
    print_test_header("Test: Decode Empty String");

    uint8_t input[] = "";
    uint8_t output[DECODE_OUTPUT_SIZE];

    uint32_t decoded_len = xy_base64_decode(input, 0, output);

    TEST_ASSERT(decoded_len == 0, "Empty string should decode to 0 bytes");
}

/**
 * @brief Test decoding with padding
 */
static bool test_decode_with_padding(void)
{
    print_test_header("Test: Decode With Padding");

    /* Test "A" -> "QQ==" */
    uint8_t input1[] = "QQ==";
    uint8_t output1[DECODE_OUTPUT_SIZE];
    uint32_t len1 = xy_base64_decode(input1, 4, output1);

    TEST_ASSERT(len1 == 1, "QQ== should decode to 1 byte");
    TEST_ASSERT(output1[0] == 'A', "Decoded value should be 'A'");

    /* Test "AB" -> "QUI=" */
    uint8_t input2[] = "QUI=";
    uint8_t output2[DECODE_OUTPUT_SIZE];
    uint32_t len2 = xy_base64_decode(input2, 4, output2);

    TEST_ASSERT(len2 == 2, "QUI= should decode to 2 bytes");
    TEST_ASSERT(memcmp(output2, "AB", 2) == 0, "Decoded value should be 'AB'");

    /* Test "ABC" -> "QUJD" (no padding needed) */
    uint8_t input3[] = "QUJD";
    uint8_t output3[DECODE_OUTPUT_SIZE];
    uint32_t len3 = xy_base64_decode(input3, 4, output3);

    TEST_ASSERT(len3 == 3, "QUJD should decode to 3 bytes");
    TEST_ASSERT(memcmp(output3, "ABC", 3) == 0, "Decoded value should be 'ABC'");
}

/**
 * @brief Test decode known values
 */
static bool test_decode_known_values(void)
{
    print_test_header("Test: Decode Known Values");

    /* "Hello, World!" -> "SGVsbG8sIFdvcmxkIQ==" */
    uint8_t input1[] = "SGVsbG8sIFdvcmxkIQ==";
    uint8_t output1[DECODE_OUTPUT_SIZE];
    uint32_t len1 = xy_base64_decode(input1, strlen((char *)input1), output1);

    TEST_ASSERT(len1 == 13, "Decoded length should be 13");
    TEST_ASSERT(memcmp(output1, "Hello, World!", 13) == 0, "Decoded value should match");

    /* "foobar" -> "Zm9vYmFy" */
    uint8_t input2[] = "Zm9vYmFy";
    uint8_t output2[DECODE_OUTPUT_SIZE];
    uint32_t len2 = xy_base64_decode(input2, strlen((char *)input2), output2);

    TEST_ASSERT(len2 == 6, "Decoded length should be 6");
    TEST_ASSERT(memcmp(output2, "foobar", 6) == 0, "Decoded value should match");
}

/**
 * @brief Test encode/decode round trip
 */
static bool test_roundtrip(void)
{
    print_test_header("Test: Roundtrip Encode/Decode");

    const char *test_strings[] = {
        "",
        "A",
        "AB",
        "ABC",
        "ABCD",
        "Hello",
        "Hello, World!",
        "The quick brown fox jumps over the lazy dog",
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",
    };

    uint8_t encoded[ENCODE_OUTPUT_SIZE];
    uint8_t decoded[DECODE_OUTPUT_SIZE];
    uint32_t i;

    for (i = 0; i < sizeof(test_strings) / sizeof(test_strings[0]); i++) {
        const char *original = test_strings[i];
        uint32_t original_len = strlen(original);

        /* Encode */
        memset(encoded, 0, sizeof(encoded));
        uint32_t encoded_len = xy_base64_encode((const uint8_t *)original, original_len, encoded);

        /* Decode */
        memset(decoded, 0, sizeof(decoded));
        uint32_t decoded_len = xy_base64_decode(encoded, encoded_len, decoded);

        /* Compare */
        if (decoded_len != original_len) {
            printf("  [FAIL] String '%s': length mismatch (%u vs %u)\n",
                   original, original_len, decoded_len);
            return false;
        }

        if (memcmp(original, decoded, original_len) != 0) {
            printf("  [FAIL] String '%s': content mismatch\n", original);
            return false;
        }
    }

    printf("  [PASS] All %u roundtrip tests passed\n", i);
    return true;
}

/**
 * @brief Test encode output length calculation
 */
static bool test_encode_length_calculation(void)
{
    print_test_header("Test: Encode Length Calculation");

    /* Empty */
    TEST_ASSERT(xy_base64_encode((const uint8_t *)"", 0, NULL) == 0, "Empty input -> 0 output");

    /* 1 byte -> 4 with padding */
    TEST_ASSERT(xy_base64_encode((const uint8_t *)"A", 1, NULL) == 4, "1 byte -> 4 output");

    /* 2 bytes -> 4 with padding */
    TEST_ASSERT(xy_base64_encode((const uint8_t *)"AB", 2, NULL) == 4, "2 bytes -> 4 output");

    /* 3 bytes -> 4 no padding */
    TEST_ASSERT(xy_base64_encode((const uint8_t *)"ABC", 3, NULL) == 4, "3 bytes -> 4 output");

    /* 4 bytes -> 8 with padding */
    TEST_ASSERT(xy_base64_encode((const uint8_t *)"ABCD", 4, NULL) == 8, "4 bytes -> 8 output");

    /* 6 bytes -> 8 no padding */
    TEST_ASSERT(xy_base64_encode((const uint8_t *)"ABCDEF", 6, NULL) == 8, "6 bytes -> 8 output");
}

/* ============ Main ============ */

int main(void)
{
    printf("\n");
    printf("========================================\n");
    printf("   XY Base64 Test Suite\n");
    printf("========================================\n");

    int total = 0;
    int passed = 0;
    int failed = 0;

    /* Encode tests */
    RUN_TEST(test_encode_empty);
    RUN_TEST(test_encode_single_char);
    RUN_TEST(test_encode_two_chars);
    RUN_TEST(test_encode_three_chars);
    RUN_TEST(test_encode_rfc_vectors);
    RUN_TEST(test_encode_length_calculation);

    /* Decode tests */
    RUN_TEST(test_decode_empty);
    RUN_TEST(test_decode_with_padding);
    RUN_TEST(test_decode_known_values);

    /* Roundtrip test */
    RUN_TEST(test_roundtrip);

    printf("\n");
    printf("========================================\n");
    printf("   Test Results: %d/%d passed\n", passed, total);
    printf("========================================\n");

    if (failed > 0) {
        printf("   %d tests FAILED\n", failed);
        return 1;
    }

    printf("   All tests PASSED!\n");
    printf("========================================\n\n");

    return 0;
}
