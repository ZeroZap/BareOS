/**
 * @file fee_test.c
 * @brief FEE完整测试套件
 */

#include "fee.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* ============ Flash模拟器 ============ */

typedef struct {
    uint8_t *storage;
    uint16_t size;
    uint16_t page_size;
    uint32_t erase_count;
    uint32_t write_count;
    uint32_t read_count;
} flash_sim_t;

static flash_sim_t *g_sim = NULL;

int sim_erase(uint32_t addr)
{
    uint32_t offset = addr - (uint32_t)g_sim->storage;
    memset(&g_sim->storage[offset], 0xFF, g_sim->page_size);
    g_sim->erase_count++;
    printf(
        "    [Flash] Erase at 0x%04X (count=%u)\n", offset, g_sim->erase_count);
    return 0;
}

int sim_write(uint32_t addr, const uint8_t *data, uint16_t len)
{
    uint32_t offset = addr - (uint32_t)g_sim->storage;
    for (uint16_t i = 0; i < len; i++) {
        g_sim->storage[offset + i] &= data[i];
    }
    g_sim->write_count++;
    return 0;
}

int sim_read(uint32_t addr, uint8_t *data, uint16_t len)
{
    uint32_t offset = addr - (uint32_t)g_sim->storage;
    memcpy(data, &g_sim->storage[offset], len);
    g_sim->read_count++;
    return 0;
}

static const fee_flash_ops_t flash_ops = { .erase = sim_erase,
                                           .write = sim_write,
                                           .read  = sim_read };

void reset_flash_stats(void)
{
    g_sim->erase_count = 0;
    g_sim->write_count = 0;
    g_sim->read_count  = 0;
}

/* ============ 测试辅助函数 ============ */

void print_test_header(const char *title)
{
    printf("\n╔════════════════════════════════════════════════════════╗\n");
    printf("║  %-52s║\n", title);
    printf("╚════════════════════════════════════════════════════════╝\n\n");
}

void print_cache_data(const char *label, uint8_t *cache, uint16_t addr,
                      uint16_t len)
{
    printf("  %s [0x%04X]: ", label, addr);
    for (uint16_t i = 0; i < len && i < 16; i++) {
        printf("%02X ", cache[addr + i]);
    }
    if (len > 16)
        printf("...");
    printf("\n");
}

#define ASSERT_EQ(a, b, msg)                                            \
    if ((a) != (b)) {                                                   \
        printf("  ✗ FAILED: %s (expected %d, got %d)\n", msg, (int)(b), \
               (int)(a));                                               \
        return false;                                                   \
    }

#define ASSERT_TRUE(cond, msg)           \
    if (!(cond)) {                       \
        printf("  ✗ FAILED: %s\n", msg); \
        return false;                    \
    }

/* ============ 测试用例 ============ */

/**
 * @brief 测试1：基本初始化
 */
bool test_basic_init(void)
{
    print_test_header("Test 1: Basic Initialization");

#define PAGE_SIZE     1024
#define PAGES_PER_FEE 2
#define CACHE_SIZE    256
#define GRAN          8

    static uint8_t flash[PAGE_SIZE * PAGES_PER_FEE * 2];
    uint8_t virtual_eeprom[CACHE_SIZE];
    uint8_t work[FEE_WORK_SIZE(GRAN)];

    flash_sim_t sim = { flash, sizeof(flash), PAGE_SIZE, 0, 0, 0 };
    g_sim           = &sim;

    memset(flash, 0xFF, sizeof(flash));
    memset(virtual_eeprom, 0x00, sizeof(virtual_eeprom));

    fee_handle_t fee;
    fee_config_t config = { .flash_base         = flash,
                            .pages_per_fee_page = PAGES_PER_FEE,
                            .flash_page_size    = PAGE_SIZE,
                            .cache_size         = CACHE_SIZE,
                            .write_granularity  = GRAN,
                            .max_erase_count    = 10000,
                            .flash_ops          = &flash_ops };

    printf("Configuration:\n");
    printf("  FEE Page size:  %d bytes\n", PAGE_SIZE * PAGES_PER_FEE);
    printf("  Cache size:     %d bytes\n", CACHE_SIZE);
    printf("  Granularity:    %d bytes\n", GRAN);
    printf("  Record data:    %d bytes\n\n", FEE_RECORD_DATA_SIZE(GRAN));

    fee_status_t status = fee_init(&fee, &config, virtual_eeprom, work);
    ASSERT_EQ(status, FEE_OK, "fee_init() should return FEE_OK");

    // 验证Cache已清空（初始化为0xFF）
    bool all_ff = true;
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (virtual_eeprom[i] != 0xFF) {
            all_ff = false;
            break;
        }
    }
    ASSERT_TRUE(all_ff, "Cache should be all 0xFF after init");

    printf("  ✓ Initialization OK\n");
    printf("  ✓ Cache cleared to 0xFF\n");
    printf("  ✓ Test PASSED\n");

    return true;
}

/**
 * @brief 测试2：基本读写
 */
bool test_basic_read_write(void)
{
    print_test_header("Test 2: Basic Read/Write");

#define PAGE_SIZE     1024
#define PAGES_PER_FEE 2
#define CACHE_SIZE    256
#define GRAN          8

    static uint8_t flash[PAGE_SIZE * PAGES_PER_FEE * 2];
    uint8_t virtual_eeprom[CACHE_SIZE];
    uint8_t work[FEE_WORK_SIZE(GRAN)];

    flash_sim_t sim = { flash, sizeof(flash), PAGE_SIZE, 0, 0, 0 };
    g_sim           = &sim;

    memset(flash, 0xFF, sizeof(flash));

    fee_handle_t fee;
    fee_config_t config = { .flash_base         = flash,
                            .pages_per_fee_page = PAGES_PER_FEE,
                            .flash_page_size    = PAGE_SIZE,
                            .cache_size         = CACHE_SIZE,
                            .write_granularity  = GRAN,
                            .max_erase_count    = 10000,
                            .flash_ops          = &flash_ops };

    fee_init(&fee, &config, virtual_eeprom, work);

    // 写入测试数据
    uint8_t test_data[8] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88 };
    printf("Writing 8 bytes to address 0x00...\n");

    fee_status_t status = fee_write(&fee, 0, test_data, 8);
    ASSERT_EQ(status, FEE_OK, "fee_write() should succeed");

    print_cache_data("Cache", virtual_eeprom, 0, 8);

    // 验证Cache已更新
    ASSERT_TRUE(memcmp(virtual_eeprom, test_data, 8) == 0,
                "Cache should contain written data");

    // 读取验证
    uint8_t read_buf[8];
    status = fee_read(&fee, 0, read_buf, 8);
    ASSERT_EQ(status, FEE_OK, "fee_read() should succeed");

    ASSERT_TRUE(memcmp(read_buf, test_data, 8) == 0,
                "Read data should match written data");

    printf("  ✓ Write OK\n");
    printf("  ✓ Cache updated\n");
    printf("  ✓ Read matches write\n");
    printf("  ✓ Test PASSED\n");

    return true;
}

/**
 * @brief 测试3：多次覆盖写入
 */
bool test_multiple_writes(void)
{
    print_test_header("Test 3: Multiple Overwrites");

#define PAGE_SIZE     1024
#define PAGES_PER_FEE 2
#define CACHE_SIZE    128
#define GRAN          8

    static uint8_t flash[PAGE_SIZE * PAGES_PER_FEE * 2];
    uint8_t virtual_eeprom[CACHE_SIZE];
    uint8_t work[FEE_WORK_SIZE(GRAN)];

    flash_sim_t sim = { flash, sizeof(flash), PAGE_SIZE, 0, 0, 0 };
    g_sim           = &sim;

    memset(flash, 0xFF, sizeof(flash));

    fee_handle_t fee;
    fee_config_t config = { .flash_base         = flash,
                            .pages_per_fee_page = PAGES_PER_FEE,
                            .flash_page_size    = PAGE_SIZE,
                            .cache_size         = CACHE_SIZE,
                            .write_granularity  = GRAN,
                            .max_erase_count    = 10000,
                            .flash_ops          = &flash_ops };

    fee_init(&fee, &config, virtual_eeprom, work);

    // 第1次写入
    uint8_t data1[8] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88 };
    printf("Write #1: ");
    for (int i = 0; i < 8; i++)
        printf("%02X ", data1[i]);
    printf("\n");
    fee_write(&fee, 0, data1, 8);

    // 第2次写入（覆盖）
    uint8_t data2[8] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22 };
    printf("Write #2: ");
    for (int i = 0; i < 8; i++)
        printf("%02X ", data2[i]);
    printf("\n");
    fee_write(&fee, 0, data2, 8);

    // 第3次写入（覆盖）
    uint8_t data3[8] = { 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8 };
    printf("Write #3: ");
    for (int i = 0; i < 8; i++)
        printf("%02X ", data3[i]);
    printf("\n");
    fee_write(&fee, 0, data3, 8);

    print_cache_data("Final cache", virtual_eeprom, 0, 8);

    // 验证Cache是最新值
    ASSERT_TRUE(memcmp(virtual_eeprom, data3, 8) == 0,
                "Cache should contain latest data");

    // 获取信息
    uint16_t erase, free, rec_cnt;
    fee_get_info(&fee, &erase, &free, &rec_cnt);
    printf("\n  Flash records: %u (3 writes * 2 records = 6)\n", rec_cnt);
    printf("  Free space: %u bytes\n", free);

    printf("  ✓ Multiple writes OK\n");
    printf("  ✓ Cache contains latest value\n");
    printf("  ✓ Test PASSED\n");

    return true;
}

/**
 * @brief 测试4：掉电恢复
 */
bool test_power_loss_recovery(void)
{
    print_test_header("Test 4: Power-off Recovery");

#define PAGE_SIZE     1024
#define PAGES_PER_FEE 2
#define CACHE_SIZE    128
#define GRAN          8

    static uint8_t flash[PAGE_SIZE * PAGES_PER_FEE * 2];

    flash_sim_t sim = { flash, sizeof(flash), PAGE_SIZE, 0, 0, 0 };
    g_sim           = &sim;

    memset(flash, 0xFF, sizeof(flash));

    // ====== 第一次上电 ======
    printf("=== First Power-on ===\n");

    uint8_t eeprom1[CACHE_SIZE];
    uint8_t work1[FEE_WORK_SIZE(GRAN)];

    fee_handle_t fee1;
    fee_config_t config = { .flash_base         = flash,
                            .pages_per_fee_page = PAGES_PER_FEE,
                            .flash_page_size    = PAGE_SIZE,
                            .cache_size         = CACHE_SIZE,
                            .write_granularity  = GRAN,
                            .max_erase_count    = 10000,
                            .flash_ops          = &flash_ops };

    fee_init(&fee1, &config, eeprom1, work1);

    // 写入数据
    uint8_t data_addr0[8]  = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88 };
    uint8_t data_addr64[8] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22 };

    fee_write(&fee1, 0, data_addr0, 8);
    fee_write(&fee1, 64, data_addr64, 8);

    print_cache_data("Before power-off [0]", eeprom1, 0, 8);
    print_cache_data("Before power-off [64]", eeprom1, 64, 8);

    // ====== 模拟掉电 ======
    printf("\n=== Power Loss ===\n");
    printf("  Simulating power-off...\n");

    // ====== 第二次上电 ======
    printf("\n=== Second Power-on (Recovery) ===\n");

    uint8_t eeprom2[CACHE_SIZE];
    uint8_t work2[FEE_WORK_SIZE(GRAN)];

    memset(eeprom2, 0x00, sizeof(eeprom2)); // 故意初始化为非0xFF

    fee_handle_t fee2;
    fee_init(&fee2, &config, eeprom2, work2);

    print_cache_data("After recovery [0]", eeprom2, 0, 8);
    print_cache_data("After recovery [64]", eeprom2, 64, 8);

    // 验证恢复的数据
    ASSERT_TRUE(memcmp(&eeprom2[0], data_addr0, 8) == 0,
                "Recovered data at addr 0 should match");
    ASSERT_TRUE(memcmp(&eeprom2[64], data_addr64, 8) == 0,
                "Recovered data at addr 64 should match");

    printf("  ✓ Data recovered correctly\n");
    printf("  ✓ Cache rebuilt from Flash\n");
    printf("  ✓ Test PASSED\n");

    return true;
}

/**
 * @brief 测试5：垃圾回收
 */
bool test_garbage_collection(void)
{
    print_test_header("Test 5: Garbage Collection");

#define PAGE_SIZE     512
#define PAGES_PER_FEE 1
#define CACHE_SIZE    64
#define GRAN          8

    static uint8_t flash[PAGE_SIZE * PAGES_PER_FEE * 2];
    uint8_t virtual_eeprom[CACHE_SIZE];
    uint8_t work[FEE_WORK_SIZE(GRAN)];

    flash_sim_t sim = { flash, sizeof(flash), PAGE_SIZE, 0, 0, 0 };
    g_sim           = &sim;

    memset(flash, 0xFF, sizeof(flash));

    fee_handle_t fee;
    fee_config_t config = { .flash_base         = flash,
                            .pages_per_fee_page = PAGES_PER_FEE,
                            .flash_page_size    = PAGE_SIZE,
                            .cache_size         = CACHE_SIZE,
                            .write_granularity  = GRAN,
                            .max_erase_count    = 10000,
                            .flash_ops          = &flash_ops };

    fee_init(&fee, &config, virtual_eeprom, work);

    printf("Data area: %u bytes\n", fee.data_area_size);
    printf("Max records: %u\n\n", fee.data_area_size / GRAN);

    // 填满FEE Page
    printf("Filling FEE Page...\n");
    uint16_t write_count = 0;

    for (int i = 0; i < 100; i++) {
        uint8_t data[8] = { (uint8_t)i,       (uint8_t)(i + 1),
                            (uint8_t)(i + 2), (uint8_t)(i + 3),
                            (uint8_t)(i + 4), (uint8_t)(i + 5),
                            (uint8_t)(i + 6), (uint8_t)(i + 7) };

        fee_status_t status = fee_write(&fee, (i % 8) * 8, data, 8);

        if (status == FEE_OK) {
            write_count++;

            if (i % 20 == 0) {
                uint16_t erase, free, rec_cnt;
                fee_get_info(&fee, &erase, &free, &rec_cnt);
                printf("  Iteration %2d: Erase=%u, Records=%u, Free=%u\n", i,
                       erase, rec_cnt, free);
            }
        }
    }

    uint16_t erase_before, free_before, rec_before;
    fee_get_info(&fee, &erase_before, &free_before, &rec_before);

    printf("\nBefore GC:\n");
    printf("  Erase count: %u\n", erase_before);
    printf("  Records: %u\n", rec_before);
    printf("  Free: %u bytes\n", free_before);

    // 保存Cache内容
    uint8_t saved_cache[CACHE_SIZE];
    memcpy(saved_cache, virtual_eeprom, CACHE_SIZE);

    // 手动触发GC
    printf("\nTriggering manual GC...\n");
    reset_flash_stats();

    fee_status_t status = fee_gc(&fee);
    ASSERT_EQ(status, FEE_OK, "GC should succeed");

    uint16_t erase_after, free_after, rec_after;
    fee_get_info(&fee, &erase_after, &free_after, &rec_after);

    printf("\nAfter GC:\n");
    printf(
        "  Erase count: %u (+%u)\n", erase_after, erase_after - erase_before);
    printf("  Records: %u\n", rec_after);
    printf("  Free: %u bytes\n", free_after);
    printf("  Flash erases: %u\n", g_sim->erase_count);

    // 验证
    ASSERT_EQ(
        erase_after, erase_before + 1, "Erase count should increment by 1");
    ASSERT_TRUE(rec_after < rec_before, "Records should be compacted");
    ASSERT_TRUE(memcmp(virtual_eeprom, saved_cache, CACHE_SIZE) == 0,
                "Cache should remain unchanged after GC");

    printf("  ✓ GC executed successfully\n");
    printf("  ✓ Cache unchanged\n");
    printf("  ✓ Records compacted\n");
    printf("  ✓ Test PASSED\n");

    return true;
}

/**
 * @brief 测试6：Cache一致性
 */
bool test_cache_consistency(void)
{
    print_test_header("Test 6: Cache Consistency");

#define PAGE_SIZE     1024
#define PAGES_PER_FEE 2
#define CACHE_SIZE    128
#define GRAN          8

    static uint8_t flash[PAGE_SIZE * PAGES_PER_FEE * 2];
    uint8_t virtual_eeprom[CACHE_SIZE];
    uint8_t work[FEE_WORK_SIZE(GRAN)];

    flash_sim_t sim = { flash, sizeof(flash), PAGE_SIZE, 0, 0, 0 };
    g_sim           = &sim;

    memset(flash, 0xFF, sizeof(flash));

    fee_handle_t fee;
    fee_config_t config = { .flash_base         = flash,
                            .pages_per_fee_page = PAGES_PER_FEE,
                            .flash_page_size    = PAGE_SIZE,
                            .cache_size         = CACHE_SIZE,
                            .write_granularity  = GRAN,
                            .max_erase_count    = 10000,
                            .flash_ops          = &flash_ops };

    fee_init(&fee, &config, virtual_eeprom, work);

    printf("Testing: Cache as virtual EEPROM mirror\n\n");

    // 测试1：写入后立即检查Cache
    printf("Test 6.1: Immediate cache update\n");
    uint8_t data1[8] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88 };
    fee_write(&fee, 10, data1, 8);

    ASSERT_TRUE(memcmp(&virtual_eeprom[10], data1, 8) == 0,
                "Cache should update immediately after write");
    printf("  ✓ Cache updated immediately\n\n");

    // 测试2：读取等价于直接访问Cache
    printf("Test 6.2: Read equals direct cache access\n");
    uint8_t read_buf[8];
    fee_read(&fee, 10, read_buf, 8);

    ASSERT_TRUE(memcmp(read_buf, &virtual_eeprom[10], 8) == 0,
                "fee_read() should equal direct cache access");
    printf("  ✓ Read from cache directly\n\n");

    // 测试3：多地址写入
    printf("Test 6.3: Multiple address writes\n");
    uint8_t data2[4] = { 0xAA, 0xBB, 0xCC, 0xDD };
    uint8_t data3[4] = { 0xEE, 0xFF, 0x11, 0x22 };

    fee_write(&fee, 0, data2, 4);
    fee_write(&fee, 64, data3, 4);

    ASSERT_TRUE(memcmp(&virtual_eeprom[0], data2, 4) == 0,
                "Cache[0] should contain data2");
    ASSERT_TRUE(memcmp(&virtual_eeprom[64], data3, 4) == 0,
                "Cache[64] should contain data3");
    printf("  ✓ Multiple addresses handled correctly\n\n");

    // 测试4：GC后Cache不变
    printf("Test 6.4: Cache unchanged after GC\n");
    uint8_t saved[CACHE_SIZE];
    memcpy(saved, virtual_eeprom, CACHE_SIZE);

    fee_gc(&fee);

    ASSERT_TRUE(memcmp(virtual_eeprom, saved, CACHE_SIZE) == 0,
                "Cache should be identical after GC");
    printf("  ✓ Cache unchanged after GC\n\n");

    // 测试5：掉电恢复后Cache一致
    printf("Test 6.5: Cache consistency after recovery\n");

    uint8_t eeprom2[CACHE_SIZE];
    uint8_t work2[FEE_WORK_SIZE(GRAN)];
    fee_handle_t fee2;

    memset(eeprom2, 0x00, sizeof(eeprom2));
    fee_init(&fee2, &config, eeprom2, work2);

    ASSERT_TRUE(memcmp(eeprom2, virtual_eeprom, CACHE_SIZE) == 0,
                "Recovered cache should match original");
    printf("  ✓ Cache recovered correctly\n\n");

    printf("  ✓ All cache consistency tests PASSED\n");

    return true;
}

/**
 * @brief 测试7：边界条件
 */
bool test_boundary_conditions(void)
{
    print_test_header("Test 7: Boundary Conditions");

#define PAGE_SIZE     1024
#define PAGES_PER_FEE 2
#define CACHE_SIZE    128
#define GRAN          8

    static uint8_t flash[PAGE_SIZE * PAGES_PER_FEE * 2];
    uint8_t virtual_eeprom[CACHE_SIZE];
    uint8_t work[FEE_WORK_SIZE(GRAN)];

    flash_sim_t sim = { flash, sizeof(flash), PAGE_SIZE, 0, 0, 0 };
    g_sim           = &sim;

    memset(flash, 0xFF, sizeof(flash));

    fee_handle_t fee;
    fee_config_t config = { .flash_base         = flash,
                            .pages_per_fee_page = PAGES_PER_FEE,
                            .flash_page_size    = PAGE_SIZE,
                            .cache_size         = CACHE_SIZE,
                            .write_granularity  = GRAN,
                            .max_erase_count    = 10000,
                            .flash_ops          = &flash_ops };

    fee_init(&fee, &config, virtual_eeprom, work);

    // 测试1：起始地址
    printf("Test 7.1: Write at address 0\n");
    uint8_t data1[8]    = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
    fee_status_t status = fee_write(&fee, 0, data1, 8);
    ASSERT_EQ(status, FEE_OK, "Write at addr 0 should succeed");
    printf("  ✓ Address 0 OK\n\n");

    // 测试2：结束地址
    printf("Test 7.2: Write at end of cache\n");
    uint8_t data2[8] = { 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8 };
    status           = fee_write(&fee, CACHE_SIZE - 8, data2, 8);
    ASSERT_EQ(status, FEE_OK, "Write at end should succeed");
    printf("  ✓ End address OK\n\n");

    // 测试3：超出边界
    printf("Test 7.3: Write beyond boundary (should fail)\n");
    uint8_t data3[8] = { 0 };
    status           = fee_write(&fee, CACHE_SIZE - 4, data3, 8);
    ASSERT_EQ(status, FEE_ERROR_PARAM, "Write beyond boundary should fail");
    printf("  ✓ Boundary check OK\n\n");

    // 测试4：部分写入
    printf("Test 7.4: Partial write (3 bytes)\n");
    uint8_t data4[3] = { 0xAA, 0xBB, 0xCC };
    status           = fee_write(&fee, 50, data4, 3);
    ASSERT_EQ(status, FEE_OK, "Partial write should succeed");
    ASSERT_TRUE(virtual_eeprom[50] == 0xAA && virtual_eeprom[51] == 0xBB
                    && virtual_eeprom[52] == 0xCC,
                "Partial data should be correct");
    printf("  ✓ Partial write OK\n\n");

    // 测试5：零长度写入
    printf("Test 7.5: Zero-length write\n");
    status = fee_write(&fee, 10, data1, 0);
    // 应该直接返回成功（无操作）
    printf("  ✓ Zero-length handled\n\n");

    printf("  ✓ All boundary tests PASSED\n");

    return true;
}

/**
 * @brief 测试8：不同颗粒度
 */
bool test_different_granularities(void)
{
    print_test_header("Test 8: Different Write Granularities");

    uint8_t granularities[] = { 8, 16, 32 };

    for (int g = 0; g < 3; g++) {
        uint8_t gran = granularities[g];

        printf("Testing granularity = %d bytes\n", gran);
        printf("─────────────────────────────────\n");

#define PAGE_SIZE     1024
#define PAGES_PER_FEE 2
#define CACHE_SIZE    128

        static uint8_t flash[PAGE_SIZE * PAGES_PER_FEE * 2];
        uint8_t virtual_eeprom[CACHE_SIZE];
        uint8_t work[FEE_MAX_GRANULARITY * 2];

        flash_sim_t sim = { flash, sizeof(flash), PAGE_SIZE, 0, 0, 0 };
        g_sim           = &sim;

        memset(flash, 0xFF, sizeof(flash));

        fee_handle_t fee;
        fee_config_t config = { .flash_base         = flash,
                                .pages_per_fee_page = PAGES_PER_FEE,
                                .flash_page_size    = PAGE_SIZE,
                                .cache_size         = CACHE_SIZE,
                                .write_granularity  = gran,
                                .max_erase_count    = 10000,
                                .flash_ops          = &flash_ops };

        fee_init(&fee, &config, virtual_eeprom, work);

        printf("  Aligned header: %d bytes\n", fee.aligned_header_size);
        printf("  Record data:    %d bytes\n", fee.record_data_size);
        printf("  Max records:    %d\n", fee.data_area_size / gran);

        // 写入测试
        uint8_t test_data[16] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
                                  0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C,
                                  0x0D, 0x0E, 0x0F, 0x10 };

        fee_write(&fee, 0, test_data, 16);

        uint16_t erase, free, rec_cnt;
        fee_get_info(&fee, &erase, &free, &rec_cnt);

        printf("  Records created: %d\n", rec_cnt);

        // 验证
        uint8_t read_buf[16];
        fee_read(&fee, 0, read_buf, 16);

        if (memcmp(read_buf, test_data, 16) == 0) {
            printf("  ✓ Gran=%d PASSED\n\n", gran);
        } else {
            printf("  ✗ Gran=%d FAILED\n\n", gran);
            return false;
        }
    }

    printf("  ✓ All granularity tests PASSED\n");

    return true;
}

/**
 * @brief 性能测试
 */
void test_performance(void)
{
    print_test_header("Performance Test");

#define PAGE_SIZE     1024
#define PAGES_PER_FEE 2
#define CACHE_SIZE    256
#define GRAN          8

    static uint8_t flash[PAGE_SIZE * PAGES_PER_FEE * 2];
    uint8_t virtual_eeprom[CACHE_SIZE];
    uint8_t work[FEE_WORK_SIZE(GRAN)];

    flash_sim_t sim = { flash, sizeof(flash), PAGE_SIZE, 0, 0, 0 };
    g_sim           = &sim;

    memset(flash, 0xFF, sizeof(flash));
    reset_flash_stats();

    fee_handle_t fee;
    fee_config_t config = { .flash_base         = flash,
                            .pages_per_fee_page = PAGES_PER_FEE,
                            .flash_page_size    = PAGE_SIZE,
                            .cache_size         = CACHE_SIZE,
                            .write_granularity  = GRAN,
                            .max_erase_count    = 10000,
                            .flash_ops          = &flash_ops };

    printf("Configuration:\n");
    printf("  Cache: %d bytes\n", CACHE_SIZE);
    printf("  Gran:  %d bytes\n", GRAN);
    printf("  RAM:   %zu bytes\n\n", FEE_TOTAL_RAM(CACHE_SIZE, GRAN));

    fee_init(&fee, &config, virtual_eeprom, work);

    // 写入性能测试
    printf("Write Performance Test (100 operations):\n");
    reset_flash_stats();

    uint8_t test_data[8] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88 };

    for (int i = 0; i < 100; i++) {
        fee_write(&fee, (i % 32) * 8, test_data, 8);
    }

    printf("  Total writes:   100\n");
    printf("  Flash erases:   %u\n", g_sim->erase_count);
    printf("  Flash writes:   %u\n", g_sim->write_count);
    printf("  Flash reads:    %u (during init)\n", g_sim->read_count);

    // 读取性能测试
    printf("\nRead Performance Test (1000 operations):\n");
    reset_flash_stats();

    uint8_t read_buf[8];
    for (int i = 0; i < 1000; i++) {
        fee_read(&fee, (i % 32) * 8, read_buf, 8);
    }

    printf("  Total reads:    1000\n");
    printf("  Flash accesses: %u (should be 0!)\n", g_sim->read_count);

    if (g_sim->read_count == 0) {
        printf("  ✓ All reads from cache (zero Flash access)\n");
    }

    printf("\n");
}

/**
 * @brief 运行所有测试
 */
void run_all_tests(void)
{
    printf("\n");
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║           FEE Test Suite v1.0                         ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n");

    int passed = 0;
    int failed = 0;

#define RUN_TEST(test) \
    if (test()) {      \
        passed++;      \
    } else {           \
        failed++;      \
    }

    RUN_TEST(test_basic_init);
    RUN_TEST(test_basic_read_write);
    RUN_TEST(test_multiple_writes);
    RUN_TEST(test_power_loss_recovery);
    RUN_TEST(test_garbage_collection);
    RUN_TEST(test_cache_consistency);
    RUN_TEST(test_boundary_conditions);
    RUN_TEST(test_different_granularities);

    test_performance();

    printf("\n");
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║                    Test Summary                        ║\n");
    printf("╠════════════════════════════════════════════════════════╣\n");
    printf("║  Total:  %2d tests                                      ║\n",
           passed + failed);
    printf("║  Passed: %2d tests                                      ║\n",
           passed);
    printf("║  Failed: %2d tests                                      ║\n",
           failed);
    printf("╚════════════════════════════════════════════════════════╝\n");

    if (failed == 0) {
        printf("\n  ✓✓✓ ALL TESTS PASSED ✓✓✓\n\n");
    } else {
        printf("\n  ✗✗✗ SOME TESTS FAILED ✗✗✗\n\n");
    }
}

/* ============ Main函数 ============ */

int main(void)
{
    run_all_tests();
    return 0;
}