/**
 * @file test_nvm.c
 * @brief Test suite for XY NVM (Non-Volatile Memory) Key-Value operations
 *
 * @author XY Team
 * @date 2025
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

/* Simulated flash types - matching the NVM implementation */
typedef uint8_t sf_uint8_t;
typedef uint16_t sf_uint16_t;
typedef uint32_t sf_uint32_t;

/* Include the KV header (simulated for testing) */
#include "sf_kv.h"

/* Flash simulation */
#define SIM_FLASH_SIZE     2048
#define SIM_PAGE_SIZE      32

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

/* Simulated flash storage */
static uint8_t g_flash[SIM_FLASH_SIZE];
static uint32_t g_flash_erase_count = 0;
static uint32_t g_flash_write_count = 0;
static uint32_t g_flash_read_count = 0;

/* Simulated flash functions */
static void sim_flash_erase(void)
{
    memset(g_flash, 0xFF, sizeof(g_flash));
    g_flash_erase_count++;
}

static void sim_flash_write(uint32_t addr, const void *data, uint32_t len)
{
    if (addr + len <= SIM_FLASH_SIZE) {
        memcpy(&g_flash[addr], data, len);
        g_flash_write_count++;
    }
}

static void sim_flash_read(uint32_t addr, void *data, uint32_t len)
{
    if (addr + len <= SIM_FLASH_SIZE) {
        memcpy(data, &g_flash[addr], len);
        g_flash_read_count++;
    }
}

static void sim_flash_init(void)
{
    memset(g_flash, 0xFF, sizeof(g_flash));
    g_flash_erase_count = 0;
    g_flash_write_count = 0;
    g_flash_read_count = 0;
}

static void print_flash_stats(void)
{
    printf("  Flash stats - Erase: %u, Write: %u, Read: %u\n",
           g_flash_erase_count, g_flash_write_count, g_flash_read_count);
}

/* ============ Test Cases ============ */

/**
 * @brief Test basic KV set and get
 */
static bool test_kv_set_get(void)
{
    print_test_header("Test: KV Set and Get");

    sim_flash_init();

    /* Set a key-value pair */
    uint8_t test_data[] = "Test value";
    sf_uint8_t key_id = 1;

    void *result = sf_kv_set(key_id, test_data, sizeof(test_data) - 1);
    TEST_ASSERT(result != NULL, "sf_kv_set should return non-NULL");

    /* Get the value */
    void *got = sf_kv_get(key_id);
    TEST_ASSERT(got != NULL, "sf_kv_get should return non-NULL for existing key");

    /* Verify value matches */
    TEST_ASSERT(memcmp(got, test_data, sizeof(test_data) - 1) == 0,
                "Retrieved value should match set value");

    print_flash_stats();
    return true;
}

/**
 * @brief Test KV update existing key
 */
static bool test_kv_update(void)
{
    print_test_header("Test: KV Update Existing Key");

    sim_flash_init();

    sf_uint8_t key_id = 1;

    /* Set initial value */
    uint8_t data1[] = "First value";
    sf_kv_set(key_id, data1, sizeof(data1) - 1);

    /* Update with new value */
    uint8_t data2[] = "Second value that is longer";
    sf_kv_set(key_id, data2, sizeof(data2) - 1);

    /* Verify updated value */
    void *got = sf_kv_get(key_id);
    TEST_ASSERT(memcmp(got, data2, sizeof(data2) - 1) == 0,
                "Retrieved value should be the updated value");

    print_flash_stats();
    return true;
}

/**
 * @brief Test KV delete
 */
static bool test_kv_delete(void)
{
    print_test_header("Test: KV Delete");

    sim_flash_init();

    sf_uint8_t key_id = 5;
    uint8_t data[] = "Data to delete";

    /* Set */
    sf_kv_set(key_id, data, sizeof(data) - 1);

    /* Verify exists */
    void *before = sf_kv_get(key_id);
    TEST_ASSERT(before != NULL, "Key should exist before delete");

    /* Delete */
    void *del_result = sf_kv_del(key_id);
    (void)del_result; /* May be NULL depending on implementation */

    /* Verify deleted */
    void *after = sf_kv_get(key_id);
    TEST_ASSERT(after == NULL, "Key should not exist after delete");

    print_flash_stats();
    return true;
}

/**
 * @brief Test KV garbage collection check
 */
static bool test_kv_gc_check(void)
{
    print_test_header("Test: KV GC Check");

    sim_flash_init();

    /* This test just verifies the GC check function can be called */
    sf_kv_gc_check();
    printf("  [PASS] sf_kv_gc_check executed without error\n");

    return true;
}

/**
 * @brief Test KV with different key IDs
 */
static bool test_kv_different_keys(void)
{
    print_test_header("Test: KV Different Keys");

    sim_flash_init();

    /* Set multiple different keys */
    struct {
        sf_uint8_t key_id;
        const char *value;
    } test_cases[] = {
        { 1, "Value for key 1" },
        { 10, "Value for key 10" },
        { 50, "Value for key 50" },
        { 100, "Value for key 100" },
        { 200, "Value for key 200" },
    };

    for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
        sf_kv_set(test_cases[i].key_id, (void *)test_cases[i].value, strlen(test_cases[i].value));
    }

    /* Verify all values */
    for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
        void *got = sf_kv_get(test_cases[i].key_id);
        TEST_ASSERT(got != NULL, "Key should exist");
        TEST_ASSERT(strcmp((char *)got, test_cases[i].value) == 0,
                    "Value should match");
    }

    print_flash_stats();
    return true;
}

/**
 * @brief Test KV key ID boundaries
 */
static bool test_kv_key_boundaries(void)
{
    print_test_header("Test: KV Key Boundaries");

    sim_flash_init();

    uint8_t data[] = "Boundary test";

    /* Key ID 0 is reserved */
    /* Key ID 255 is reserved */

    /* Set valid boundary keys */
    sf_uint8_t key_min = 1;
    sf_uint8_t key_max = 254;

    sf_kv_set(key_min, data, sizeof(data) - 1);
    void *got_min = sf_kv_get(key_min);
    TEST_ASSERT(got_min != NULL, "Key ID 1 should work");

    sf_kv_set(key_max, data, sizeof(data) - 1);
    void *got_max = sf_kv_get(key_max);
    TEST_ASSERT(got_max != NULL, "Key ID 254 should work");

    print_flash_stats();
    return true;
}

/**
 * @brief Test empty value
 */
static bool test_kv_empty_value(void)
{
    print_test_header("Test: KV Empty Value");

    sim_flash_init();

    sf_uint8_t key_id = 1;
    uint8_t empty_data = 0;

    /* Set with zero length - depends on implementation handling */
    sf_kv_set(key_id, &empty_data, 0);

    /* Get should return something (implementation dependent) */
    void *got = sf_kv_get(key_id);
    printf("  Get result for zero-length: %p\n", got);

    print_flash_stats();
    return true;
}

/**
 * @brief Test multiple set/get cycles
 */
static bool test_kv_multiple_cycles(void)
{
    print_test_header("Test: KV Multiple Cycles");

    sim_flash_init();

    sf_uint8_t key_id = 1;

    /* Multiple cycles of set/get */
    for (int cycle = 0; cycle < 5; cycle++) {
        char data[32];
        sprintf(data, "Cycle %d data", cycle);

        sf_kv_set(key_id, data, strlen(data));

        void *got = sf_kv_get(key_id);
        TEST_ASSERT(got != NULL, "Key should exist after set");
        TEST_ASSERT(strcmp((char *)got, data) == 0, "Value should match");
    }

    print_flash_stats();
    return true;
}

/**
 * @brief Test data persistence across operations
 */
static bool test_kv_persistence(void)
{
    print_test_header("Test: KV Persistence");

    sim_flash_init();

    /* Set several keys */
    struct {
        sf_uint8_t key_id;
        const char *value;
    } entries[] = {
        { 10, "Ten" },
        { 20, "Twenty" },
        { 30, "Thirty" },
    };

    for (size_t i = 0; i < sizeof(entries) / sizeof(entries[0]); i++) {
        sf_kv_set(entries[i].key_id, (void *)entries[i].value, strlen(entries[i].value));
    }

    /* Perform GC (may compact storage) */
    sf_kv_gc_check();
    sf_kv_gc_env();

    /* Verify all still accessible */
    for (size_t i = 0; i < sizeof(entries) / sizeof(entries[0]); i++) {
        void *got = sf_kv_get(entries[i].key_id);
        TEST_ASSERT(got != NULL, "Key should still exist after GC");
        TEST_ASSERT(strcmp((char *)got, entries[i].value) == 0,
                    "Value should still be correct after GC");
    }

    print_flash_stats();
    return true;
}

/* ============ Main ============ */

int main(void)
{
    printf("\n");
    printf("========================================\n");
    printf("   XY NVM KV Test Suite\n");
    printf("========================================\n");

    int total = 0;
    int passed = 0;
    int failed = 0;

    /* Basic tests */
    RUN_TEST(test_kv_set_get);
    RUN_TEST(test_kv_update);
    RUN_TEST(test_kv_delete);

    /* GC tests */
    RUN_TEST(test_kv_gc_check);
    RUN_TEST(test_kv_persistence);

    /* Key tests */
    RUN_TEST(test_kv_different_keys);
    RUN_TEST(test_kv_key_boundaries);

    /* Edge cases */
    RUN_TEST(test_kv_empty_value);
    RUN_TEST(test_kv_multiple_cycles);

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
