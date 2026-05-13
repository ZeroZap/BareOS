FEE支持Flash 4字节写入包装为8字节
核心概念
完全支持！ FEE可以将Flash原生4字节写入包装为8字节颗粒度。

┌─────────────────────────────────────────┐
│         应用层                           │
│  fee_write(addr, data, len)             │
└──────────────┬──────────────────────────┘
               │
┌──────────────▼──────────────────────────┐
│         FEE层                            │
│  write_granularity = 8 bytes            │
│  Record = 4B header + 4B data           │
└──────────────┬──────────────────────────┘
               │
┌──────────────▼──────────────────────────┐
│      Flash抽象层 (flash_ops)            │
│  将8字节拆分为2次4字节写入               │
└──────────────┬──────────────────────────┘
               │
┌──────────────▼──────────────────────────┐
│      Flash硬件层                         │
│  原生支持4字节写入                       │
│  HAL_FLASH_Program_Word()               │
└─────────────────────────────────────────┘

1. STM32F103 完整实现示例
flash_wrapper.h
#ifndef FLASH_WRAPPER_H
#define FLASH_WRAPPER_H

#include <stdint.h>

/**
 * @brief Flash适配器配置
 */
typedef struct {
    uint32_t base_addr;              // Flash基地址
    uint32_t total_size;             // 总大小
    uint16_t native_granularity;     // 原生写入颗粒（如4字节）
    uint16_t fee_granularity;        // FEE要求的颗粒（如8字节）
    uint16_t page_size;              // 页大小
} flash_adapter_config_t;

/**
 * @brief 初始化Flash适配器
 */
int flash_adapter_init(const flash_adapter_config_t *config);

/**
 * @brief Flash擦除（透传）
 */
int flash_adapter_erase(uint32_t addr);

/**
 * @brief Flash写入（4字节包装为8字节）
 * @param addr 地址
 * @param data 数据指针
 * @param len 长度（必须是fee_granularity的整数倍）
 */
int flash_adapter_write(uint32_t addr, const uint8_t *data, uint16_t len);

/**
 * @brief Flash读取（透传）
 */
int flash_adapter_read(uint32_t addr, uint8_t *data, uint16_t len);

#endif

flash_wrapper.c
#include "flash_wrapper.h"
#include "stm32f1xx_hal.h"
#include <string.h>

static flash_adapter_config_t g_config;
static uint8_t g_initialized = 0;

/**
 * @brief 初始化Flash适配器
 */
int flash_adapter_init(const flash_adapter_config_t *config) {
    if (!config) return -1;

    memcpy(&g_config, config, sizeof(flash_adapter_config_t));

    // 验证：FEE颗粒度必须是原生颗粒度的整数倍
    if (g_config.fee_granularity % g_config.native_granularity != 0) {
        return -1;  // 配置错误
    }

    g_initialized = 1;
    return 0;
}

/**
 * @brief Flash擦除（透传给HAL）
 */
int flash_adapter_erase(uint32_t addr) {
    FLASH_EraseInitTypeDef erase_init;
    uint32_t page_error;
    HAL_StatusTypeDef status;

    // 计算页号
    uint32_t page = (addr - FLASH_BASE) / g_config.page_size;

    HAL_FLASH_Unlock();

    erase_init.TypeErase   = FLASH_TYPEERASE_PAGES;
    erase_init.PageAddress = FLASH_BASE + page * g_config.page_size;
    erase_init.NbPages     = 1;

    status = HAL_FLASHEx_Erase(&erase_init, &page_error);

    HAL_FLASH_Lock();

    return (status == HAL_OK) ? 0 : -1;
}

/**
 * @brief Flash写入（关键：将8字节拆分为2次4字节）
 */
int flash_adapter_write(uint32_t addr, const uint8_t *data, uint16_t len) {
    if (!g_initialized || !data) return -1;

    // 验证：长度必须是FEE颗粒度的整数倍
    if (len % g_config.fee_granularity != 0) {
        return -1;
    }

    HAL_FLASH_Unlock();

    uint16_t offset = 0;
    uint16_t native_gran = g_config.native_granularity;  // 4字节

    while (offset < len) {
        // 每次写入native_granularity字节
        uint32_t word_data;

        // 组装4字节数据
        word_data = (uint32_t)data[offset + 0] << 0  |
                    (uint32_t)data[offset + 1] << 8  |
                    (uint32_t)data[offset + 2] << 16 |
                    (uint32_t)data[offset + 3] << 24;

        // 使用HAL写入4字节（Word编程）
        HAL_StatusTypeDef status = HAL_FLASH_Program(
            FLASH_TYPEPROGRAM_WORD,
            addr + offset,
            word_data
        );

        if (status != HAL_OK) {
            HAL_FLASH_Lock();
            return -1;
        }

        offset += native_gran;
    }

    HAL_FLASH_Lock();
    return 0;
}

/**
 * @brief Flash读取（透传）
 */
int flash_adapter_read(uint32_t addr, uint8_t *data, uint16_t len) {
    if (!g_initialized || !data) return -1;

    memcpy(data, (void *)addr, len);
    return 0;
}

2. 具体MCU适配示例
STM32F103 (4字节 → 8字节)
#include "fee.h"
#include "flash_wrapper.h"

/* Flash配置 */
#define FLASH_BASE_ADDR   0x0800F000  // Flash起始地址
#define FLASH_PAGE_SIZE   1024        // 1KB页
#define NATIVE_GRAN       4           // 原生4字节（Word）
#define FEE_GRAN          8           // FEE要求8字节

/* 初始化Flash适配器 */
void stm32f103_flash_init(void) {
    flash_adapter_config_t config = {
        .base_addr          = FLASH_BASE_ADDR,
        .total_size         = 8192,          // 8KB (4个页)
        .native_granularity = NATIVE_GRAN,   // 硬件4字节
        .fee_granularity    = FEE_GRAN,      // FEE 8字节
        .page_size          = FLASH_PAGE_SIZE
    };

    flash_adapter_init(&config);
}

/* FEE Flash操作接口 */
static const fee_flash_ops_t fee_flash_ops = {
    .erase = flash_adapter_erase,
    .write = flash_adapter_write,  // 自动将8字节拆分为2×4字节
    .read  = flash_adapter_read
};

/* FEE配置 */
static uint8_t virtual_eeprom[512];
static uint8_t work_buffer[FEE_WORK_SIZE(FEE_GRAN)];
static fee_handle_t g_fee;

void fee_system_init(void) {
    // 初始化Flash适配器
    stm32f103_flash_init();

    // 配置FEE
    fee_config_t config = {
        .flash_base         = (uint8_t *)FLASH_BASE_ADDR,
        .pages_per_fee_page = 2,           // 每个FEE Page = 2KB
        .flash_page_size    = FLASH_PAGE_SIZE,
        .cache_size         = 512,
        .write_granularity  = FEE_GRAN,    // FEE使用8字节
        .max_erase_count    = 10000,
        .flash_ops          = &fee_flash_ops
    };

    fee_init(&g_fee, &config, virtual_eeprom, work_buffer);
}

/* 使用示例 */
void test_fee_write(void) {
    uint32_t data = 0x12345678;

    // FEE写入（内部会将8字节拆分为2次4字节写入）
    fee_write(&g_fee, 0, (uint8_t *)&data, sizeof(data));

    // 读取验证
    uint32_t read_data;
    fee_read(&g_fee, 0, (uint8_t *)&read_data, sizeof(read_data));

    if (read_data == data) {
        printf("✓ 写入成功！\n");
    }
}

STM32F401 (4字节 → 16字节)
/* STM32F4系列：Flash 4字节，可选FEE 16字节以提高效率 */

#define FEE_GRAN_16   16  // 使用16字节颗粒度

int flash_adapter_write_f401(uint32_t addr, const uint8_t *data, uint16_t len) {
    if (len % 16 != 0) return -1;

    HAL_FLASH_Unlock();

    // 16字节 = 4次 × 4字节写入
    for (uint16_t i = 0; i < len; i += 4) {
        uint32_t word = *(uint32_t *)(data + i);

        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,
                             addr + i, word) != HAL_OK) {
            HAL_FLASH_Lock();
            return -1;
        }
    }

    HAL_FLASH_Lock();
    return 0;
}

ESP32 (4字节 → 8字节)
#include "esp_partition.h"

const esp_partition_t *g_fee_partition = NULL;

int esp32_flash_write(uint32_t addr, const uint8_t *data, uint16_t len) {
    if (!g_fee_partition) return -1;

    // ESP32的Flash写入已经支持任意长度
    // 但内部仍是按4字节对齐
    uint32_t offset = addr - (uint32_t)g_fee_partition->address;

    return esp_partition_write(g_fee_partition, offset, data, len);
}

3. 写入流程详解
单次8字节写入过程
// 用户调用
fee_write(&fee, 0x00, data8, 8);

// FEE内部（假设gran=8，record_data=4）
// 拆分为2条Record：
//   Record 0: addr=0x00, data[0..3]
//   Record 1: addr=0x04, data[4..7]

// 每条Record写入（8字节）
write_record(fee, 0x00, data[0..3], 4);
  |
  v
// 调用flash_ops->write(addr, 8bytes)
flash_adapter_write(flash_addr, record_8bytes, 8);
  |
  v
// 适配器拆分为2次4字节写入
HAL_FLASH_Program(addr + 0, word0);  // 前4字节
HAL_FLASH_Program(addr + 4, word1);  // 后4字节

时序图
时间轴：
User        FEE         Adapter      Flash HW
 |           |            |             |
 |--write--->|            |             |
 |           |--record1-->|             |
 |           |            |--4bytes---->|
 |           |            |             |--write-->
 |           |            |<---OK-------|
 |           |            |--4bytes---->|
 |           |            |             |--write-->
 |           |            |<---OK-------|
 |           |<--OK-------|             |
 |<--OK------|            |             |

4. 性能对比
不同包装方式的开销
Flash原生	FEE颗粒	写入次数	效率	推荐
4B	8B	2次	50%	✓ 最小配置
4B	16B	4次	75%	✓ 推荐
4B	32B	8次	87.5%	高效率
8B	8B	1次	50%	完美匹配
8B	16B	2次	75%	✓ 推荐
写入时间估算
// STM32F103：单次Word写入约53μs
// gran=8: 2次写入 = 106μs
// gran=16: 4次写入 = 212μs

// 结论：包装开销可接受

5. 完整配置示例
/* ═══════════════════════════════════════════════════════════════
 * 低阶MCU (Flash 4字节) 使用FEE (8字节颗粒) 完整示例
 * MCU: STM32F103C8T6
 * ═══════════════════════════════════════════════════════════════*/

#include "stm32f1xx_hal.h"
#include "fee.h"
#include <string.h>

/* ────────────────────────────────────────────────────────────── */
/* 1. Flash适配层实现（4字节→8字节转换） */
/* ────────────────────────────────────────────────────────────── */

#define FLASH_START     0x0800F000
#define FLASH_PAGE_SIZE 1024

/**
 * @brief Flash擦除
 */
static int my_flash_erase(uint32_t addr) {
    FLASH_EraseInitTypeDef erase_init;
    uint32_t page_error;

    uint32_t page = (addr - FLASH_BASE) / FLASH_PAGE_SIZE;

    HAL_FLASH_Unlock();

    erase_init.TypeErase   = FLASH_TYPEERASE_PAGES;
    erase_init.PageAddress = FLASH_BASE + page * FLASH_PAGE_SIZE;
    erase_init.NbPages     = 1;

    HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&erase_init, &page_error);

    HAL_FLASH_Lock();

    return (status == HAL_OK) ? 0 : -1;
}

/**
 * @brief Flash写入（关键：8字节拆分为2×4字节）
 * @note len必须是8的整数倍
 */
static int my_flash_write(uint32_t addr, const uint8_t *data, uint16_t len) {
    // 验证：长度必须是8的整数倍
    if (len % 8 != 0) {
        return -1;
    }

    HAL_FLASH_Unlock();

    // 每8字节分2次写入（每次4字节）
    for (uint16_t i = 0; i < len; i += 4) {
        uint32_t word = (uint32_t)data[i + 0] << 0  |
                        (uint32_t)data[i + 1] << 8  |
                        (uint32_t)data[i + 2] << 16 |
                        (uint32_t)data[i + 3] << 24;

        HAL_StatusTypeDef status = HAL_FLASH_Program(
            FLASH_TYPEPROGRAM_WORD,
            addr + i,
            word
        );

        if (status != HAL_OK) {
            HAL_FLASH_Lock();
            return -1;
        }

        // 验证写入
        if (*(volatile uint32_t *)(addr + i) != word) {
            HAL_FLASH_Lock();
            return -1;
        }
    }

    HAL_FLASH_Lock();
    return 0;
}

/**
 * @brief Flash读取
 */
static int my_flash_read(uint32_t addr, uint8_t *data, uint16_t len) {
    memcpy(data, (void *)addr, len);
    return 0;
}

/* Flash操作接口 */
static const fee_flash_ops_t flash_ops = {
    .erase = my_flash_erase,
    .write = my_flash_write,  // 8字节→2×4字节
    .read  = my_flash_read
};

/* ────────────────────────────────────────────────────────────── */
/* 2. FEE配置 */
/* ────────────────────────────────────────────────────────────── */

static uint8_t virtual_eeprom[256];
static uint8_t work_buffer[FEE_WORK_SIZE(8)];
static fee_handle_t g_fee;

static const fee_config_t fee_config = {
    .flash_base         = (uint8_t *)FLASH_START,
    .pages_per_fee_page = 2,              // 2KB per FEE page
    .flash_page_size    = FLASH_PAGE_SIZE,
    .cache_size         = 256,
    .write_granularity  = 8,              // FEE使用8字节
    .max_erase_count    = 10000,
    .flash_ops          = &flash_ops
};

/* ────────────────────────────────────────────────────────────── */
/* 3. 初始化和使用 */
/* ────────────────────────────────────────────────────────────── */

void fee_system_init(void) {
    fee_status_t status = fee_init(&g_fee, &fee_config,
                                   virtual_eeprom, work_buffer);

    if (status != FEE_OK) {
        Error_Handler();
    }
}

/* 测试函数 */
void test_fee(void) {
    printf("FEE Test (Flash 4B → FEE 8B)\n");
    printf("═══════════════════════════════\n\n");

    // 写入测试
    uint32_t test_data = 0xDEADBEEF;
    printf("写入: 0x%08X\n", test_data);

    fee_write(&g_fee, 0, (uint8_t *)&test_data, sizeof(test_data));

    // 读取验证
    uint32_t read_data = 0;
    fee_read(&g_fee, 0, (uint8_t *)&read_data, sizeof(read_data));

    printf("读取: 0x%08X\n", read_data);

    if (read_data == test_data) {
        printf("✓ 测试通过！\n");
        printf("  Flash原生: 4字节写入\n");
        printf("  FEE使用:   8字节颗粒\n");
        printf("  转换:      自动拆分为2次4字节写入\n");
    } else {
        printf("✗ 测试失败！\n");
    }
}

int main(void) {
    HAL_Init();
    SystemClock_Config();

    fee_system_init();
    test_fee();

    while (1) {
        // 应用代码
    }
}