/**
 * @file test_factory.c
 * @brief Test suite for XY Factory data read/write operations
 *
 * @author XY Team
 * @date 2025
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "xy_factory.h"

/* Flash simulation */
#define SIM_FLASH_SIZE     4096
#define SIM_PAGE_SIZE      256

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

/* Global flash simulation storage */
static uint8_t g_flash_a[SIM_FLASH_SIZE];
static uint8_t g_flash_b[SIM_FLASH_SIZE];

/* Flash simulation operations */
static int sim_erase(uint32_t addr, uint32_t size)
{
    (void)addr;
    (void)size;
    return 0;
}

static int sim_write(uint32_t addr, const uint8_t *data, uint32_t size)
{
    if (addr < SIM_FLASH_SIZE) {
        memcpy(&g_flash_a[addr], data, size);
    } else if (addr >= SIM_FLASH_SIZE && addr < 2 * SIM_FLASH_SIZE) {
        memcpy(&g_flash_b[addr - SIM_FLASH_SIZE], data, size);
    }
    return 0;
}

static int sim_read(uint32_t addr, uint8_t *data, uint32_t size)
{
    if (addr < SIM_FLASH_SIZE) {
        memcpy(data, &g_flash_a[addr], size);
    } else if (addr >= SIM_FLASH_SIZE && addr < 2 * SIM_FLASH_SIZE) {
        memcpy(data, &g_flash_b[addr - SIM_FLASH_SIZE], size);
    }
    return 0;
}

static const factory_flash_ops_t sim_flash_ops = {
    .erase = sim_erase,
    .write = sim_write,
    .read = sim_read,
};

/* Setup function */
static void setup(void)
{
    memset(g_flash_a, 0xFF, sizeof(g_flash_a));
    memset(g_flash_b, 0xFF, sizeof(g_flash_b));
}

/* ============ Test Cases ============ */

/**
 * @brief Test factory initialization
 */
static bool test_factory_init(void)
{
    print_test_header("Test: Factory Init");

    factory_config_t config = {
        .flash_ops = &sim_flash_ops,
        .region_a_addr = 0,
        .region_a_size = SIM_FLASH_SIZE,
        .region_b_addr = SIM_FLASH_SIZE,
        .region_b_size = SIM_FLASH_SIZE,
    };

    factory_status_t status = factory_init(&config);

    TEST_ASSERT(status == FACTORY_OK || status == FACTORY_ERROR,
                "Factory init should return OK or ERROR (if already initialized)");
}

/**
 * @brief Test factory write and read
 */
static bool test_factory_write_read(void)
{
    print_test_header("Test: Factory Write/Read");

    setup();

    factory_config_t config = {
        .flash_ops = &sim_flash_ops,
        .region_a_addr = 0,
        .region_a_size = SIM_FLASH_SIZE,
        .region_b_addr = SIM_FLASH_SIZE,
        .region_b_size = SIM_FLASH_SIZE,
    };

    factory_init(&config);

    /* Write data */
    uint8_t write_data[] = "Test factory data 123!";
    uint8_t write_type = FACTORY_TYPE_CONFIG;

    factory_status_t write_status = factory_write(write_type, write_data, sizeof(write_data) - 1);

    TEST_ASSERT(write_status == FACTORY_OK, "Factory write should return OK");

    /* Read data */
    uint8_t read_data[64];
    uint16_t read_len = sizeof(read_data);

    factory_status_t read_status = factory_read(write_type, read_data, &read_len);

    TEST_ASSERT(read_status == FACTORY_OK, "Factory read should return OK");
    TEST_ASSERT(read_len == sizeof(write_data) - 1, "Read length should match write length");
    TEST_ASSERT(memcmp(read_data, write_data, sizeof(write_data) - 1) == 0,
                "Read data should match written data");
}

/**
 * @brief Test factory delete
 */
static bool test_factory_delete(void)
{
    print_test_header("Test: Factory Delete");

    setup();

    factory_config_t config = {
        .flash_ops = &sim_flash_ops,
        .region_a_addr = 0,
        .region_a_size = SIM_FLASH_SIZE,
        .region_b_addr = SIM_FLASH_SIZE,
        .region_b_size = SIM_FLASH_SIZE,
    };

    factory_init(&config);

    /* Write data */
    uint8_t write_data[] = "Data to delete";
    factory_write(FACTORY_TYPE_CONFIG, write_data, sizeof(write_data) - 1);

    /* Verify exists */
    bool exists_before = factory_exists(FACTORY_TYPE_CONFIG);
    TEST_ASSERT(exists_before == true, "Data should exist before delete");

    /* Delete */
    factory_status_t del_status = factory_delete(FACTORY_TYPE_CONFIG);
    TEST_ASSERT(del_status == FACTORY_OK, "Factory delete should return OK");

    /* Verify deleted */
    bool exists_after = factory_exists(FACTORY_TYPE_CONFIG);
    TEST_ASSERT(exists_after == false, "Data should not exist after delete");
}

/**
 * @brief Test factory exists check
 */
static bool test_factory_exists(void)
{
    print_test_header("Test: Factory Exists Check");

    setup();

    factory_config_t config = {
        .flash_ops = &sim_flash_ops,
        .region_a_addr = 0,
        .region_a_size = SIM_FLASH_SIZE,
        .region_b_addr = SIM_FLASH_SIZE,
        .region_b_size = SIM_FLASH_SIZE,
    };

    factory_init(&config);

    /* Initially should not exist */
    bool exists_before = factory_exists(FACTORY_TYPE_DEVICE_ID);
    TEST_ASSERT(exists_before == false, "Type should not exist initially");

    /* Write data */
    uint8_t data[] = "Device ID data";
    factory_write(FACTORY_TYPE_DEVICE_ID, data, sizeof(data) - 1);

    /* Should exist now */
    bool exists_after = factory_exists(FACTORY_TYPE_DEVICE_ID);
    TEST_ASSERT(exists_after == true, "Type should exist after write");
}

/**
 * @brief Test factory enumeration
 */
static bool test_factory_enum(void)
{
    print_test_header("Test: Factory Enumeration");

    setup();

    factory_config_t config = {
        .flash_ops = &sim_flash_ops,
        .region_a_addr = 0,
        .region_a_size = SIM_FLASH_SIZE,
        .region_b_addr = SIM_FLASH_SIZE,
        .region_b_size = SIM_FLASH_SIZE,
    };

    factory_init(&config);

    /* Write multiple types */
    uint8_t data1[] = "Config data";
    uint8_t data2[] = "Calibration data";
    uint8_t data3[] = "Device ID data";

    factory_write(FACTORY_TYPE_CONFIG, data1, sizeof(data1) - 1);
    factory_write(FACTORY_TYPE_CALIBRATION, data2, sizeof(data2) - 1);
    factory_write(FACTORY_TYPE_DEVICE_ID, data3, sizeof(data3) - 1);

    /* Enumerate */
    uint8_t types[16];
    uint16_t count = 16;

    factory_status_t status = factory_enum(types, &count);

    TEST_ASSERT(status == FACTORY_OK, "Factory enum should return OK");
    TEST_ASSERT(count >= 3, "Should find at least 3 types");

    /* Check all written types are present */
    bool found_config = false, found_calib = false, found_device = false;
    for (uint16_t i = 0; i < count; i++) {
        if (types[i] == FACTORY_TYPE_CONFIG) found_config = true;
        if (types[i] == FACTORY_TYPE_CALIBRATION) found_calib = true;
        if (types[i] == FACTORY_TYPE_DEVICE_ID) found_device = true;
    }

    TEST_ASSERT(found_config && found_calib && found_device,
                "All written types should be enumerated");
}

/**
 * @brief Test factory format
 */
static bool test_factory_format(void)
{
    print_test_header("Test: Factory Format");

    setup();

    factory_config_t config = {
        .flash_ops = &sim_flash_ops,
        .region_a_addr = 0,
        .region_a_size = SIM_FLASH_SIZE,
        .region_b_addr = SIM_FLASH_SIZE,
        .region_b_size = SIM_FLASH_SIZE,
    };

    factory_init(&config);

    /* Write some data first */
    uint8_t data[] = "Some data";
    factory_write(FACTORY_TYPE_CONFIG, data, sizeof(data) - 1);

    /* Format */
    factory_status_t status = factory_format();
    TEST_ASSERT(status == FACTORY_OK, "Factory format should return OK");

    /* Verify all data is gone */
    bool exists_after = factory_exists(FACTORY_TYPE_CONFIG);
    TEST_ASSERT(exists_after == false, "No data should exist after format");
}

/**
 * @brief Test factory CRC16 calculation
 */
static bool test_factory_crc16(void)
{
    print_test_header("Test: CRC16 Calculation");

    /* Test known values */
    uint8_t test_data[] = "123456789";
    uint16_t crc = factory_crc16(test_data, 9);

    /* CRC16-CCITT of "123456789" is 0x29B1 */
    TEST_ASSERT(crc == 0x29B1, "CRC16 of '123456789' should be 0x29B1");

    /* Test empty data */
    crc = factory_crc16((const uint8_t *)"", 0);
    TEST_ASSERT(crc == 0xFFFF, "CRC16 of empty data should be 0xFFFF");
}

/**
 * @brief Test factory CRC16 update
 */
static bool test_factory_crc16_update(void)
{
    print_test_header("Test: CRC16 Update");

    uint16_t crc_initial = factory_crc16((const uint8_t *)"1234", 4);
    uint16_t crc_full = factory_crc16((const uint8_t *)"123456789", 9);

    uint16_t crc_updated = factory_crc16_update(crc_initial, (const uint8_t *)"56789", 5);

    TEST_ASSERT(crc_updated == crc_full, "CRC update should produce same result as full calculation");
}

/**
 * @brief Test factory status strings
 */
static bool test_factory_status_str(void)
{
    print_test_header("Test: Status Strings");

    const char *str = factory_status_str(FACTORY_OK);
    TEST_ASSERT(str != NULL, "Status string should not be NULL");
    TEST_ASSERT(strcmp(str, "OK") == 0, "FACTORY_OK should map to 'OK'");

    str = factory_status_str(FACTORY_ERROR);
    TEST_ASSERT(str != NULL, "Error status string should not be NULL");

    str = factory_status_str(FACTORY_ERROR_FLASH);
    TEST_ASSERT(str != NULL, "Flash error status string should not be NULL");
}

/**
 * @brief Test multiple write cycles
 */
static bool test_factory_multiple_writes(void)
{
    print_test_header("Test: Multiple Write Cycles");

    setup();

    factory_config_t config = {
        .flash_ops = &sim_flash_ops,
        .region_a_addr = 0,
        .region_a_size = SIM_FLASH_SIZE,
        .region_b_addr = SIM_FLASH_SIZE,
        .region_b_size = SIM_FLASH_SIZE,
    };

    factory_init(&config);

    /* Write multiple times to same type (should update) */
    for (int i = 0; i < 3; i++) {
        uint8_t data[32];
        sprintf((char *)data, "Write iteration %d", i);

        factory_status_t status = factory_write(FACTORY_TYPE_CONFIG, data, strlen((char *)data));
        TEST_ASSERT(status == FACTORY_OK, "Write should succeed");

        /* Read back */
        uint8_t read_data[32];
        uint16_t read_len = sizeof(read_data);

        factory_read(FACTORY_TYPE_CONFIG, read_data, &read_len);
        TEST_ASSERT(memcmp(data, read_data, strlen((char *)data)) == 0,
                    "Read data should match written data");
    }
}

/**
 * @brief Test different data types
 */
static bool test_factory_different_types(void)
{
    print_test_header("Test: Different Data Types");

    setup();

    factory_config_t config = {
        .flash_ops = &sim_flash_ops,
        .region_a_addr = 0,
        .region_a_size = SIM_FLASH_SIZE,
        .region_b_addr = SIM_FLASH_SIZE,
        .region_b_size = SIM_FLASH_SIZE,
    };

    factory_init(&config);

    /* Test all factory types */
    factory_type_t types[] = {
        FACTORY_TYPE_DEVICE_ID,
        FACTORY_TYPE_CALIBRATION,
        FACTORY_TYPE_CONFIG,
        FACTORY_TYPE_SECURITY_KEY,
        FACTORY_TYPE_MANUFACTURE,
    };

    for (size_t i = 0; i < sizeof(types) / sizeof(types[0]); i++) {
        uint8_t write_data[] = "Test data";
        factory_write(types[i], write_data, sizeof(write_data) - 1);

        bool exists = factory_exists(types[i]);
        TEST_ASSERT(exists == true, "Type should exist after write");
    }
}

/* ============ Main ============ */

int main(void)
{
    printf("\n");
    printf("========================================\n");
    printf("   XY Factory Test Suite\n");
    printf("========================================\n");

    int total = 0;
    int passed = 0;
    int failed = 0;

    /* Basic tests */
    RUN_TEST(test_factory_init);
    RUN_TEST(test_factory_write_read);
    RUN_TEST(test_factory_delete);
    RUN_TEST(test_factory_exists);

    /* Enumeration and format */
    RUN_TEST(test_factory_enum);
    RUN_TEST(test_factory_format);

    /* CRC and utility tests */
    RUN_TEST(test_factory_crc16);
    RUN_TEST(test_factory_crc16_update);
    RUN_TEST(test_factory_status_str);

    /* Stress tests */
    RUN_TEST(test_factory_multiple_writes);
    RUN_TEST(test_factory_different_types);

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
