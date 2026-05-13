/**
 * @file test_tlv.c
 * @brief Test suite for XY TLV (Type-Length-Value) encode/decode operations
 *
 * @author XY Team
 * @date 2025
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "xy_tlv.h"

/* Test buffer size */
#define TEST_BUFFER_SIZE 512

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

/* ============ Test Cases ============ */

/**
 * @brief Test basic buffer initialization
 */
static bool test_buffer_init(void)
{
    print_test_header("Test: Buffer Initialization");

    uint8_t buffer[TEST_BUFFER_SIZE];
    xy_tlv_buffer_t tlv_buf;

    int ret = xy_tlv_buffer_init(&tlv_buf, buffer, TEST_BUFFER_SIZE);
    TEST_ASSERT(ret == XY_TLV_OK, "xy_tlv_buffer_init should return OK");

    TEST_ASSERT(tlv_buf.capacity == TEST_BUFFER_SIZE, "Capacity should match");
    TEST_ASSERT(tlv_buf.offset == 0, "Initial offset should be 0");
    TEST_ASSERT(tlv_buf.nesting == 0, "Initial nesting should be 0");
}

/**
 * @brief Test buffer reset
 */
static bool test_buffer_reset(void)
{
    print_test_header("Test: Buffer Reset");

    uint8_t buffer[TEST_BUFFER_SIZE];
    xy_tlv_buffer_t tlv_buf;

    xy_tlv_buffer_init(&tlv_buf, buffer, TEST_BUFFER_SIZE);

    /* Encode something to move offset */
    xy_tlv_encode_uint32(&tlv_buf, XY_TLV_TYPE_UINT32, 42);

    xy_tlv_buffer_reset(&tlv_buf);

    TEST_ASSERT(tlv_buf.offset == 0, "Offset should be 0 after reset");
    TEST_ASSERT(tlv_buf.nesting == 0, "Nesting should be 0 after reset");
}

/**
 * @brief Test buffer used/free space tracking
 */
static bool test_buffer_space(void)
{
    print_test_header("Test: Buffer Space Tracking");

    uint8_t buffer[TEST_BUFFER_SIZE];
    xy_tlv_buffer_t tlv_buf;

    xy_tlv_buffer_init(&tlv_buf, buffer, TEST_BUFFER_SIZE);

    uint16_t used = xy_tlv_buffer_get_used(&tlv_buf);
    uint16_t free = xy_tlv_buffer_get_free(&tlv_buf);

    TEST_ASSERT(used == 0, "Initial used should be 0");
    TEST_ASSERT(free == TEST_BUFFER_SIZE, "Initial free should be full capacity");

    /* Encode a uint32 (4 bytes header + 4 bytes value = 8 bytes) */
    xy_tlv_encode_uint32(&tlv_buf, XY_TLV_TYPE_UINT32, 42);

    used = xy_tlv_buffer_get_used(&tlv_buf);
    free = xy_tlv_buffer_get_free(&tlv_buf);

    TEST_ASSERT(used == 8, "Used should be 8 after encoding uint32");
    TEST_ASSERT(free == TEST_BUFFER_SIZE - 8, "Free should be reduced accordingly");
}

/**
 * @brief Test encoding uint8 values
 */
static bool test_encode_uint8(void)
{
    print_test_header("Test: Encode uint8");

    uint8_t buffer[TEST_BUFFER_SIZE];
    xy_tlv_buffer_t tlv_buf;

    xy_tlv_buffer_init(&tlv_buf, buffer, TEST_BUFFER_SIZE);

    int ret = xy_tlv_encode_uint8(&tlv_buf, XY_TLV_TYPE_UINT8, 0xAB);

    TEST_ASSERT(ret == XY_TLV_OK, "Encode uint8 should return OK");
    TEST_ASSERT(xy_tlv_buffer_get_used(&tlv_buf) == 3, "Encoded size should be 3 (header + 1 byte)");
}

/**
 * @brief Test encoding uint16 values
 */
static bool test_encode_uint16(void)
{
    print_test_header("Test: Encode uint16");

    uint8_t buffer[TEST_BUFFER_SIZE];
    xy_tlv_buffer_t tlv_buf;

    xy_tlv_buffer_init(&tlv_buf, buffer, TEST_BUFFER_SIZE);

    int ret = xy_tlv_encode_uint16(&tlv_buf, XY_TLV_TYPE_UINT16, 0x1234);

    TEST_ASSERT(ret == XY_TLV_OK, "Encode uint16 should return OK");
    TEST_ASSERT(xy_tlv_buffer_get_used(&tlv_buf) == 4, "Encoded size should be 4 (header + 2 bytes)");
}

/**
 * @brief Test encoding uint32 values
 */
static bool test_encode_uint32(void)
{
    print_test_header("Test: Encode uint32");

    uint8_t buffer[TEST_BUFFER_SIZE];
    xy_tlv_buffer_t tlv_buf;

    xy_tlv_buffer_init(&tlv_buf, buffer, TEST_BUFFER_SIZE);

    int ret = xy_tlv_encode_uint32(&tlv_buf, XY_TLV_TYPE_UINT32, 0x12345678);

    TEST_ASSERT(ret == XY_TLV_OK, "Encode uint32 should return OK");
    TEST_ASSERT(xy_tlv_buffer_get_used(&tlv_buf) == 8, "Encoded size should be 8 (header + 4 bytes)");
}

/**
 * @brief Test encoding uint64 values
 */
static bool test_encode_uint64(void)
{
    print_test_header("Test: Encode uint64");

    uint8_t buffer[TEST_BUFFER_SIZE];
    xy_tlv_buffer_t tlv_buf;

    xy_tlv_buffer_init(&tlv_buf, buffer, TEST_BUFFER_SIZE);

    int ret = xy_tlv_encode_uint64(&tlv_buf, XY_TLV_TYPE_UINT64, 0x123456789ABCDEF0ULL);

    TEST_ASSERT(ret == XY_TLV_OK, "Encode uint64 should return OK");
    TEST_ASSERT(xy_tlv_buffer_get_used(&tlv_buf) == 12, "Encoded size should be 12 (header + 8 bytes)");
}

/**
 * @brief Test encoding string values
 */
static bool test_encode_string(void)
{
    print_test_header("Test: Encode String");

    uint8_t buffer[TEST_BUFFER_SIZE];
    xy_tlv_buffer_t tlv_buf;

    xy_tlv_buffer_init(&tlv_buf, buffer, TEST_BUFFER_SIZE);

    const char *test_str = "Hello, TLV!";
    int ret = xy_tlv_encode_string(&tlv_buf, XY_TLV_TYPE_STRING, test_str);

    TEST_ASSERT(ret == XY_TLV_OK, "Encode string should return OK");
    TEST_ASSERT(xy_tlv_buffer_get_used(&tlv_buf) == 4 + strlen(test_str),
                "Encoded size should include header + string length");
}

/**
 * @brief Test encoding boolean values
 */
static bool test_encode_bool(void)
{
    print_test_header("Test: Encode Boolean");

    uint8_t buffer[TEST_BUFFER_SIZE];
    xy_tlv_buffer_t tlv_buf;

    xy_tlv_buffer_init(&tlv_buf, buffer, TEST_BUFFER_SIZE);

    xy_tlv_encode_bool(&tlv_buf, XY_TLV_TYPE_BOOL, true);
    xy_tlv_encode_bool(&tlv_buf, XY_TLV_TYPE_BOOL, false);

    TEST_ASSERT(xy_tlv_buffer_get_used(&tlv_buf) == 6, "Two bools should take 6 bytes total");
}

/**
 * @brief Test decoding uint32 values
 */
static bool test_decode_uint32(void)
{
    print_test_header("Test: Decode uint32");

    uint8_t buffer[TEST_BUFFER_SIZE];
    xy_tlv_buffer_t tlv_buf;

    xy_tlv_buffer_init(&tlv_buf, buffer, TEST_BUFFER_SIZE);

    /* Encode */
    uint32_t encode_val = 0x12345678;
    xy_tlv_encode_uint32(&tlv_buf, XY_TLV_TYPE_UINT32, encode_val);

    /* Decode */
    xy_tlv_iterator_t iter;
    xy_tlv_iterator_init(&iter, buffer, xy_tlv_buffer_get_used(&tlv_buf));

    xy_tlv_t tlv;
    int ret = xy_tlv_iterator_next(&iter, &tlv);

    TEST_ASSERT(ret == XY_TLV_OK, "Iterator next should return OK");
    TEST_ASSERT(tlv.type == XY_TLV_TYPE_UINT32, "Type should match");
    TEST_ASSERT(tlv.length == 4, "Length should be 4");

    uint32_t decode_val;
    ret = xy_tlv_decode_uint32(&tlv, &decode_val);

    TEST_ASSERT(ret == XY_TLV_OK, "Decode uint32 should return OK");
    TEST_ASSERT(decode_val == encode_val, "Decoded value should match encoded");
}

/**
 * @brief Test decoding string values
 */
static bool test_decode_string(void)
{
    print_test_header("Test: Decode String");

    uint8_t buffer[TEST_BUFFER_SIZE];
    xy_tlv_buffer_t tlv_buf;

    xy_tlv_buffer_init(&tlv_buf, buffer, TEST_BUFFER_SIZE);

    /* Encode */
    const char *encode_str = "Test String";
    xy_tlv_encode_string(&tlv_buf, XY_TLV_TYPE_STRING, encode_str);

    /* Decode */
    xy_tlv_iterator_t iter;
    xy_tlv_iterator_init(&iter, buffer, xy_tlv_buffer_get_used(&tlv_buf));

    xy_tlv_t tlv;
    xy_tlv_iterator_next(&iter, &tlv);

    char decode_str[64];
    int ret = xy_tlv_decode_string(&tlv, decode_str, sizeof(decode_str));

    TEST_ASSERT(ret == XY_TLV_OK, "Decode string should return OK");
    TEST_ASSERT(strcmp(decode_str, encode_str) == 0, "Decoded string should match encoded");
}

/**
 * @brief Test iterator has_next
 */
static bool test_iterator_has_next(void)
{
    print_test_header("Test: Iterator Has Next");

    uint8_t buffer[TEST_BUFFER_SIZE];
    xy_tlv_buffer_t tlv_buf;

    xy_tlv_buffer_init(&tlv_buf, buffer, TEST_BUFFER_SIZE);

    /* Encode two values */
    xy_tlv_encode_uint32(&tlv_buf, XY_TLV_TYPE_UINT32, 1);
    xy_tlv_encode_uint32(&tlv_buf, XY_TLV_TYPE_UINT32, 2);

    xy_tlv_iterator_t iter;
    xy_tlv_iterator_init(&iter, buffer, xy_tlv_buffer_get_used(&tlv_buf));

    TEST_ASSERT(xy_tlv_iterator_has_next(&iter) == true, "Should have next (first element)");

    xy_tlv_t tlv;
    xy_tlv_iterator_next(&iter, &tlv);
    TEST_ASSERT(xy_tlv_iterator_has_next(&iter) == true, "Should have next (second element)");

    xy_tlv_iterator_next(&iter, &tlv);
    TEST_ASSERT(xy_tlv_iterator_has_next(&iter) == false, "Should not have next (end)");
}

/**
 * @brief Test finding TLV by type
 */
static bool test_tlv_find(void)
{
    print_test_header("Test: Find TLV by Type");

    uint8_t buffer[TEST_BUFFER_SIZE];
    xy_tlv_buffer_t tlv_buf;

    xy_tlv_buffer_init(&tlv_buf, buffer, TEST_BUFFER_SIZE);

    /* Encode multiple values */
    xy_tlv_encode_uint32(&tlv_buf, XY_TLV_TYPE_UINT32, 1);
    xy_tlv_encode_uint16(&tlv_buf, XY_TLV_TYPE_UINT16, 2);
    xy_tlv_encode_uint8(&tlv_buf, XY_TLV_TYPE_UINT8, 3);

    xy_tlv_t tlv;
    int ret = xy_tlv_find(buffer, xy_tlv_buffer_get_used(&tlv_buf),
                          XY_TLV_TYPE_UINT16, &tlv);

    TEST_ASSERT(ret == XY_TLV_OK, "Find should return OK");
    TEST_ASSERT(tlv.type == XY_TLV_TYPE_UINT16, "Found type should match");

    uint16_t val;
    xy_tlv_decode_uint16(&tlv, &val);
    TEST_ASSERT(val == 2, "Decoded value should match");
}

/**
 * @brief Test finding all TLVs by type
 */
static bool test_tlv_find_all(void)
{
    print_test_header("Test: Find All TLVs by Type");

    uint8_t buffer[TEST_BUFFER_SIZE];
    xy_tlv_buffer_t tlv_buf;

    xy_tlv_buffer_init(&tlv_buf, buffer, TEST_BUFFER_SIZE);

    /* Encode three uint32 values */
    xy_tlv_encode_uint32(&tlv_buf, XY_TLV_TYPE_UINT32, 10);
    xy_tlv_encode_uint16(&tlv_buf, XY_TLV_TYPE_UINT16, 20);
    xy_tlv_encode_uint32(&tlv_buf, XY_TLV_TYPE_UINT32, 30);
    xy_tlv_encode_uint32(&tlv_buf, XY_TLV_TYPE_UINT32, 40);

    xy_tlv_t found[4];
    uint16_t count = 4;

    int ret = xy_tlv_find_all(buffer, xy_tlv_buffer_get_used(&tlv_buf),
                              XY_TLV_TYPE_UINT32, found, &count);

    TEST_ASSERT(ret == 3, "Should find 3 uint32 values");
    TEST_ASSERT(count == 3, "Count should be 3");

    /* Verify values */
    uint32_t val;
    xy_tlv_decode_uint32(&found[0], &val);
    TEST_ASSERT(val == 10, "First value should be 10");

    xy_tlv_decode_uint32(&found[1], &val);
    TEST_ASSERT(val == 30, "Second value should be 30");

    xy_tlv_decode_uint32(&found[2], &val);
    TEST_ASSERT(val == 40, "Third value should be 40");
}

/**
 * @brief Test counting TLVs
 */
static bool test_tlv_count(void)
{
    print_test_header("Test: Count TLVs");

    uint8_t buffer[TEST_BUFFER_SIZE];
    xy_tlv_buffer_t tlv_buf;

    xy_tlv_buffer_init(&tlv_buf, buffer, TEST_BUFFER_SIZE);

    /* Encode five values */
    xy_tlv_encode_uint32(&tlv_buf, XY_TLV_TYPE_UINT32, 1);
    xy_tlv_encode_uint16(&tlv_buf, XY_TLV_TYPE_UINT16, 2);
    xy_tlv_encode_uint8(&tlv_buf, XY_TLV_TYPE_UINT8, 3);
    xy_tlv_encode_string(&tlv_buf, XY_TLV_TYPE_STRING, "A");
    xy_tlv_encode_bool(&tlv_buf, XY_TLV_TYPE_BOOL, true);

    int count = xy_tlv_count(buffer, xy_tlv_buffer_get_used(&tlv_buf));

    TEST_ASSERT(count == 5, "Should count 5 TLVs");
}

/**
 * @brief Test buffer overflow handling
 */
static bool test_buffer_overflow(void)
{
    print_test_header("Test: Buffer Overflow");

    uint8_t buffer[16]; /* Small buffer */
    xy_tlv_buffer_t tlv_buf;

    xy_tlv_buffer_init(&tlv_buf, buffer, sizeof(buffer));

    /* Try to encode a large value */
    int ret = xy_tlv_encode_bytes(&tlv_buf, XY_TLV_TYPE_BYTES, buffer, 100);

    TEST_ASSERT(ret == XY_TLV_BUFFER_OVERFLOW, "Should return buffer overflow error");
}

/**
 * @brief Test validate function
 */
static bool test_tlv_validate(void)
{
    print_test_header("Test: Validate TLV");

    uint8_t buffer[TEST_BUFFER_SIZE];
    xy_tlv_buffer_t tlv_buf;

    xy_tlv_buffer_init(&tlv_buf, buffer, TEST_BUFFER_SIZE);

    xy_tlv_encode_uint32(&tlv_buf, XY_TLV_TYPE_UINT32, 42);

    int ret = xy_tlv_validate(buffer, xy_tlv_buffer_get_used(&tlv_buf));

    TEST_ASSERT(ret == XY_TLV_OK, "Valid TLV buffer should pass validation");
}

/**
 * @brief Test checksum calculation
 */
static bool test_checksum(void)
{
    print_test_header("Test: Checksum Calculation");

    uint8_t buffer[TEST_BUFFER_SIZE];
    xy_tlv_buffer_t tlv_buf;

    xy_tlv_buffer_init(&tlv_buf, buffer, TEST_BUFFER_SIZE);

    xy_tlv_encode_uint32(&tlv_buf, XY_TLV_TYPE_UINT32, 0x12345678);

    uint16_t checksum = xy_tlv_checksum(buffer, xy_tlv_buffer_get_used(&tlv_buf));

    TEST_ASSERT(checksum != 0, "Checksum should not be zero for valid data");
}

/**
 * @brief Test error string function
 */
static bool test_error_strings(void)
{
    print_test_header("Test: Error Strings");

    const char *msg = xy_tlv_get_error_string(XY_TLV_OK);
    TEST_ASSERT(msg != NULL, "Error string for OK should not be NULL");
    TEST_ASSERT(strcmp(msg, "OK") == 0, "Error string for OK should be 'OK'");

    msg = xy_tlv_get_error_string(XY_TLV_ERROR);
    TEST_ASSERT(msg != NULL, "Error string for ERROR should not be NULL");

    msg = xy_tlv_get_error_string(XY_TLV_BUFFER_OVERFLOW);
    TEST_ASSERT(msg != NULL, "Error string for BUFFER_OVERFLOW should not be NULL");
}

/**
 * @brief Test type name function
 */
static bool test_type_names(void)
{
    print_test_header("Test: Type Names");

    const char *name = xy_tlv_get_type_name(XY_TLV_TYPE_UINT32);
    TEST_ASSERT(name != NULL, "Type name should not be NULL");

    name = xy_tlv_get_type_name(XY_TLV_TYPE_STRING);
    TEST_ASSERT(name != NULL, "Type name for STRING should not be NULL");

    name = xy_tlv_get_type_name(0x9999);
    TEST_ASSERT(name != NULL, "Custom type name should not be NULL");
}

/**
 * @brief Test multiple encode/decode cycle
 */
static bool test_encode_decode_cycle(void)
{
    print_test_header("Test: Encode/Decode Cycle");

    uint8_t buffer[TEST_BUFFER_SIZE];
    xy_tlv_buffer_t tlv_buf;

    xy_tlv_buffer_init(&tlv_buf, buffer, TEST_BUFFER_SIZE);

    /* Encode various types */
    xy_tlv_encode_uint8(&tlv_buf, 0x0001, 0x12);
    xy_tlv_encode_uint16(&tlv_buf, 0x0002, 0x3456);
    xy_tlv_encode_uint32(&tlv_buf, 0x0003, 0x789ABCDE);
    xy_tlv_encode_uint64(&tlv_buf, 0x0004, 0x123456789ABCDEF0ULL);
    xy_tlv_encode_string(&tlv_buf, 0x0101, "Test");
    xy_tlv_encode_bool(&tlv_buf, 0x000B, true);

    /* Decode and verify */
    xy_tlv_iterator_t iter;
    xy_tlv_iterator_init(&iter, buffer, xy_tlv_buffer_get_used(&tlv_buf));

    xy_tlv_t tlv;
    uint8_t u8;
    uint16_t u16;
    uint32_t u32;
    uint64_t u64;
    char str[32];
    bool b;

    TEST_ASSERT(xy_tlv_iterator_next(&iter, &tlv) == XY_TLV_OK, "Read uint8");
    xy_tlv_decode_uint8(&tlv, &u8);
    TEST_ASSERT(u8 == 0x12, "uint8 value matches");

    TEST_ASSERT(xy_tlv_iterator_next(&iter, &tlv) == XY_TLV_OK, "Read uint16");
    xy_tlv_decode_uint16(&tlv, &u16);
    TEST_ASSERT(u16 == 0x3456, "uint16 value matches");

    TEST_ASSERT(xy_tlv_iterator_next(&iter, &tlv) == XY_TLV_OK, "Read uint32");
    xy_tlv_decode_uint32(&tlv, &u32);
    TEST_ASSERT(u32 == 0x789ABCDE, "uint32 value matches");

    TEST_ASSERT(xy_tlv_iterator_next(&iter, &tlv) == XY_TLV_OK, "Read uint64");
    xy_tlv_decode_uint64(&tlv, &u64);
    TEST_ASSERT(u64 == 0x123456789ABCDEF0ULL, "uint64 value matches");

    TEST_ASSERT(xy_tlv_iterator_next(&iter, &tlv) == XY_TLV_OK, "Read string");
    xy_tlv_decode_string(&tlv, str, sizeof(str));
    TEST_ASSERT(strcmp(str, "Test") == 0, "string value matches");

    TEST_ASSERT(xy_tlv_iterator_next(&iter, &tlv) == XY_TLV_OK, "Read bool");
    xy_tlv_decode_bool(&tlv, &b);
    TEST_ASSERT(b == true, "bool value matches");

    TEST_ASSERT(xy_tlv_iterator_has_next(&iter) == false, "No more elements");
}

/**
 * @brief Test get_type_name function
 */
static bool test_get_stats(void)
{
    print_test_header("Test: Statistics");

    xy_tlv_reset_stats();

    uint8_t buffer[TEST_BUFFER_SIZE];
    xy_tlv_buffer_t tlv_buf;

    xy_tlv_buffer_init(&tlv_buf, buffer, TEST_BUFFER_SIZE);

    /* Encode some values */
    xy_tlv_encode_uint32(&tlv_buf, XY_TLV_TYPE_UINT32, 1);
    xy_tlv_encode_uint32(&tlv_buf, XY_TLV_TYPE_UINT32, 2);

    xy_tlv_stats_t stats;
    xy_tlv_get_stats(&stats);

    TEST_ASSERT(stats.total_encoded >= 2, "Should have encoded at least 2 TLVs");
    TEST_ASSERT(stats.bytes_encoded > 0, "Should have encoded some bytes");
}

/* ============ Main ============ */

int main(void)
{
    printf("\n");
    printf("========================================\n");
    printf("   XY TLV Test Suite\n");
    printf("========================================\n");

    int total = 0;
    int passed = 0;
    int failed = 0;

    /* Buffer tests */
    RUN_TEST(test_buffer_init);
    RUN_TEST(test_buffer_reset);
    RUN_TEST(test_buffer_space);

    /* Encode tests */
    RUN_TEST(test_encode_uint8);
    RUN_TEST(test_encode_uint16);
    RUN_TEST(test_encode_uint32);
    RUN_TEST(test_encode_uint64);
    RUN_TEST(test_encode_string);
    RUN_TEST(test_encode_bool);

    /* Decode tests */
    RUN_TEST(test_decode_uint32);
    RUN_TEST(test_decode_string);

    /* Iterator tests */
    RUN_TEST(test_iterator_has_next);

    /* Search tests */
    RUN_TEST(test_tlv_find);
    RUN_TEST(test_tlv_find_all);
    RUN_TEST(test_tlv_count);

    /* Error handling */
    RUN_TEST(test_buffer_overflow);
    RUN_TEST(test_tlv_validate);

    /* Utility tests */
    RUN_TEST(test_checksum);
    RUN_TEST(test_error_strings);
    RUN_TEST(test_type_names);
    RUN_TEST(test_get_stats);

    /* Integration test */
    RUN_TEST(test_encode_decode_cycle);

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
