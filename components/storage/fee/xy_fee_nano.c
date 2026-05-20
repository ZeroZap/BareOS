/**
 * @file xy_fee_nano.c
 * @brief Flash EEPROM Emulation - Nano 精简版实现
 * @version 2.0.0
 * @date 2026-03-15
 *
 * 支持裸机（Bare-metal）和 RTOS 环境
 * 不使用动态内存分配（malloc/free）
 */

#include "eflash.h"
#include "xy_string.h"

/*==============================================================================
 * 平台和后端配置
 *============================================================================*/

#ifndef XY_OS_BACKEND_BAREMETAL
    #define XY_OS_BACKEND_BAREMETAL  1
    #define XY_OS_BACKEND_FREERTOS   0
    #define XY_OS_BACKEND_RTTHREAD   0
#endif

#if !defined(XY_OS_BACKEND_FREERTOS)
    #define XY_OS_BACKEND_FREERTOS   0
#endif
#if !defined(XY_OS_BACKEND_RTTHREAD)
    #define XY_OS_BACKEND_RTTHREAD   0
#endif

/*==============================================================================
 * 临界区适配
 *============================================================================*/

#if XY_OS_BACKEND_BAREMETAL
    #define EFLASH_ENTER_CRITICAL()    do { } while (0)
    #define EFLASH_EXIT_CRITICAL()     do { } while (0)
#elif XY_OS_BACKEND_FREERTOS
    #define EFLASH_ENTER_CRITICAL()    taskENTER_CRITICAL()
    #define EFLASH_EXIT_CRITICAL()     taskEXIT_CRITICAL()
#elif XY_OS_BACKEND_RTTHREAD
    #define EFLASH_ENTER_CRITICAL()    rt_enter_critical()
    #define EFLASH_EXIT_CRITICAL()     rt_exit_critical()
#else
    #define EFLASH_ENTER_CRITICAL()    do { } while (0)
    #define EFLASH_EXIT_CRITICAL()     do { } while (0)
#endif

/*==============================================================================
 * 常量定义
 *============================================================================*/

#define EFLASH_ERASED_VALUE 0xFF /**< 擦除后的 Flash 值 */

/*==============================================================================
 * 内部辅助函数
 *============================================================================*/

/**
 * @brief 检查地址范围是否有效（内部调用）
 */
static bool is_address_valid_internal(eflash_handle_t *handle,
                                       uint32_t address, size_t size)
{
    if (handle == NULL || !handle->initialized) {
        return false;
    }

    if (address >= handle->config.total_size) {
        return false;
    }

    if (size == 0) {
        return true;
    }

    if (address + size > handle->config.total_size) {
        return false;
    }

    return true;
}

/**
 * @brief 获取地址对应的页索引（内部调用）
 */
static uint32_t get_page_index_internal(eflash_handle_t *handle,
                                         uint32_t address)
{
    if (handle == NULL || !handle->initialized) {
        return 0;
    }
    return address / handle->config.page_size;
}

/*==============================================================================
 * API 函数实现
 *============================================================================*/

/**
 * @brief 使用用户提供的 buffer 初始化 Flash
 *
 * @note 适用于裸机环境，不使用 malloc
 */
eflash_result_t eflash_init_with_buffer(eflash_handle_t *handle,
                                         const eflash_config_t *config,
                                         uint8_t *memory_buffer,
                                         bool *page_erased_buffer)
{
    if (handle == NULL || config == NULL) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    /* 验证配置 */
    if (config->page_size == 0 || config->page_size > EFLASH_MAX_PAGE_SIZE) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    if (config->page_count == 0 || config->page_count > EFLASH_MAX_PAGES) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    /* 计算总大小 */
    uint32_t total_size = config->page_size * config->page_count;
    if (config->total_size > 0 && config->total_size != total_size) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    /* 验证写入单元 */
    if (config->write_unit != EFLASH_WRITE_UNIT_32BIT
        && config->write_unit != EFLASH_WRITE_UNIT_64BIT
        && config->write_unit != EFLASH_WRITE_UNIT_128BIT) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    /* 验证用户提供 buffer */
    if (memory_buffer == NULL || page_erased_buffer == NULL) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    EFLASH_ENTER_CRITICAL();

    /* 初始化配置 */
    handle->config = *config;
    handle->config.total_size = total_size;

    /* 使用用户提供的 buffer */
    handle->memory = memory_buffer;
    handle->page_erased = page_erased_buffer;
    handle->user_provided_buffer = true;

    /* 初始化 Flash 内存为擦除态 */
    memset(handle->memory, EFLASH_ERASED_VALUE, total_size);

    /* 标记所有页为已擦除 */
    for (uint32_t i = 0; i < config->page_count; i++) {
        handle->page_erased[i] = true;
    }

    handle->initialized = true;

    EFLASH_EXIT_CRITICAL();

    return EFLASH_OK;
}

/**
 * @brief 初始化 Flash 设备（仅 PC 模拟器使用）
 *
 * @note 仅在 PC 模拟环境使用，嵌入式环境请用 eflash_init_with_buffer
 */
eflash_result_t eflash_init(eflash_handle_t *handle,
                            const eflash_config_t *config)
{
#if defined(_WIN32) || defined(__unix__) || defined(__APPLE__)
    /* PC 环境：使用动态分配模拟 */
    if (handle == NULL || config == NULL) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    /* 验证配置 */
    if (config->page_size == 0 || config->page_size > EFLASH_MAX_PAGE_SIZE) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    if (config->page_count == 0 || config->page_count > EFLASH_MAX_PAGES) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    /* 计算总大小 */
    uint32_t total_size = config->page_size * config->page_count;

    /* 分配内存 */
    handle->memory = (uint8_t *)malloc(total_size);
    if (handle->memory == NULL) {
        return EFLASH_ERROR_WRITE_FAIL;
    }

    /* 分配擦除状态数组 */
    handle->page_erased = (bool *)malloc(config->page_count * sizeof(bool));
    if (handle->page_erased == NULL) {
        free(handle->memory);
        handle->memory = NULL;
        return EFLASH_ERROR_WRITE_FAIL;
    }

    /* 初始化配置 */
    handle->config = *config;
    handle->config.total_size = total_size;
    handle->user_provided_buffer = false;

    /* 初始化 Flash 内存为擦除态 */
    memset(handle->memory, EFLASH_ERASED_VALUE, total_size);

    /* 标记所有页为已擦除 */
    for (uint32_t i = 0; i < config->page_count; i++) {
        handle->page_erased[i] = true;
    }

    handle->initialized = true;

    return EFLASH_OK;
#else
    /* 嵌入式环境：回退到用户 buffer 接口 */
    return EFLASH_ERROR_NOT_INIT;
#endif
}

/**
 * @brief 反初始化 Flash 设备
 */
eflash_result_t eflash_deinit(eflash_handle_t *handle)
{
    if (handle == NULL) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    EFLASH_ENTER_CRITICAL();

    if (!handle->initialized) {
        EFLASH_EXIT_CRITICAL();
        return EFLASH_ERROR_NOT_INIT;
    }

    /* 仅在非用户提供的 buffer 情况下释放内存（PC 模拟器） */
    if (!handle->user_provided_buffer) {
        if (handle->memory != NULL) {
            free(handle->memory);
            handle->memory = NULL;
        }

        if (handle->page_erased != NULL) {
            free(handle->page_erased);
            handle->page_erased = NULL;
        }
    }

    handle->initialized = false;

    EFLASH_EXIT_CRITICAL();

    return EFLASH_OK;
}

/**
 * @brief 读取 Flash 数据
 */
eflash_result_t eflash_read(eflash_handle_t *handle, uint32_t address,
                            uint8_t *data, size_t size)
{
    if (handle == NULL || data == NULL) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    if (!handle->initialized) {
        return EFLASH_ERROR_NOT_INIT;
    }

    if (size == 0) {
        return EFLASH_OK;
    }

    /* 检查地址范围 */
    if (!is_address_valid_internal(handle, address, size)) {
        return EFLASH_ERROR_OUT_OF_RANGE;
    }

    /* 读取数据 */
    memcpy(data, &handle->memory[address], size);

    return EFLASH_OK;
}

/**
 * @brief 写入 Flash 数据
 */
eflash_result_t eflash_write(eflash_handle_t *handle, uint32_t address,
                             const uint8_t *data, size_t size)
{
    if (handle == NULL || data == NULL) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    if (!handle->initialized) {
        return EFLASH_ERROR_NOT_INIT;
    }

    if (size == 0) {
        return EFLASH_OK;
    }

    /* 检查地址范围 */
    if (!is_address_valid_internal(handle, address, size)) {
        return EFLASH_ERROR_OUT_OF_RANGE;
    }

    /* 检查对齐 */
    uint32_t write_unit = (uint32_t)handle->config.write_unit;
    if (!EFLASH_IS_ALIGNED(address, write_unit)
        || !EFLASH_IS_ALIGNED(size, write_unit)) {
        return EFLASH_ERROR_ALIGNMENT;
    }

    EFLASH_ENTER_CRITICAL();

    /* 自动擦除页（如果启用） */
    if (handle->config.auto_erase) {
        uint32_t start_page = get_page_index_internal(handle, address);
        uint32_t end_page = get_page_index_internal(handle,
                                                     address + size - 1);

        for (uint32_t page = start_page; page <= end_page; page++) {
            if (page >= handle->config.page_count) {
                EFLASH_EXIT_CRITICAL();
                return EFLASH_ERROR_OUT_OF_RANGE;
            }
            if (!handle->page_erased[page]) {
                eflash_result_t result = eflash_erase_page(handle, page);
                if (result != EFLASH_OK) {
                    EFLASH_EXIT_CRITICAL();
                    return result;
                }
            }
        }
    }

    /* 模拟 Flash 写入操作（只能将 1 变为 0） */
    for (size_t i = 0; i < size; i++) {
        uint32_t addr = address + i;
        uint8_t current = handle->memory[addr];
        uint8_t new_val = data[i];

        /* Flash 写入只能清除位（1 -> 0），不能设置位（0 -> 1） */
        if ((current & new_val) != new_val) {
            /* 尝试写入 1 但当前位置是 0 - 需要先擦除 */
            if (!handle->config.auto_erase) {
                EFLASH_EXIT_CRITICAL();
                return EFLASH_ERROR_WRITE_FAIL;
            }
        }

        handle->memory[addr] = current & new_val;
    }

    /* 标记受影响的页为未擦除 */
    uint32_t start_page = get_page_index_internal(handle, address);
    uint32_t end_page = get_page_index_internal(handle, address + size - 1);
    for (uint32_t page = start_page; page <= end_page; page++) {
        handle->page_erased[page] = false;
    }

    EFLASH_EXIT_CRITICAL();

    return EFLASH_OK;
}

/**
 * @brief 擦除 Flash 页
 */
eflash_result_t eflash_erase_page(eflash_handle_t *handle, uint32_t page_index)
{
    if (handle == NULL) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    if (!handle->initialized) {
        return EFLASH_ERROR_NOT_INIT;
    }

    EFLASH_ENTER_CRITICAL();

    if (page_index >= handle->config.page_count) {
        EFLASH_EXIT_CRITICAL();
        return EFLASH_ERROR_OUT_OF_RANGE;
    }

    /* 擦除页（设置所有字节为 0xFF） */
    uint32_t page_offset = page_index * handle->config.page_size;
    memset(&handle->memory[page_offset], EFLASH_ERASED_VALUE,
           handle->config.page_size);

    /* 标记页为已擦除 */
    handle->page_erased[page_index] = true;

    EFLASH_EXIT_CRITICAL();

    return EFLASH_OK;
}

/**
 * @brief 擦除 Flash 扇区（基于地址）
 */
eflash_result_t eflash_erase_sector(eflash_handle_t *handle, uint32_t address)
{
    if (handle == NULL) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    if (!handle->initialized) {
        return EFLASH_ERROR_NOT_INIT;
    }

    /* 获取地址对应的页索引 */
    uint32_t page_index = get_page_index_internal(handle, address);

    return eflash_erase_page(handle, page_index);
}

/**
 * @brief 擦除整个 Flash
 */
eflash_result_t eflash_erase_all(eflash_handle_t *handle)
{
    if (handle == NULL) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    if (!handle->initialized) {
        return EFLASH_ERROR_NOT_INIT;
    }

    EFLASH_ENTER_CRITICAL();

    /* 擦除所有 Flash 内存 */
    memset(handle->memory, EFLASH_ERASED_VALUE, handle->config.total_size);

    /* 标记所有页为已擦除 */
    for (uint32_t i = 0; i < handle->config.page_count; i++) {
        handle->page_erased[i] = true;
    }

    EFLASH_EXIT_CRITICAL();

    return EFLASH_OK;
}

/**
 * @brief 获取 Flash 信息
 */
eflash_result_t eflash_get_info(eflash_handle_t *handle,
                                eflash_config_t *config)
{
    if (handle == NULL || config == NULL) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    if (!handle->initialized) {
        return EFLASH_ERROR_NOT_INIT;
    }

    EFLASH_ENTER_CRITICAL();
    *config = handle->config;
    EFLASH_EXIT_CRITICAL();

    return EFLASH_OK;
}

/**
 * @brief 检查地址范围是否有效
 */
bool eflash_is_address_valid(eflash_handle_t *handle, uint32_t address,
                             size_t size)
{
    EFLASH_ENTER_CRITICAL();
    bool result = is_address_valid_internal(handle, address, size);
    EFLASH_EXIT_CRITICAL();
    return result;
}

/**
 * @brief 获取地址对应的页索引
 */
uint32_t eflash_get_page_index(eflash_handle_t *handle, uint32_t address)
{
    EFLASH_ENTER_CRITICAL();
    uint32_t result = get_page_index_internal(handle, address);
    EFLASH_EXIT_CRITICAL();
    return result;
}

/**
 * @brief 检查页是否已擦除
 */
bool eflash_is_page_erased(eflash_handle_t *handle, uint32_t page_index)
{
    if (handle == NULL || !handle->initialized) {
        return false;
    }

    EFLASH_ENTER_CRITICAL();

    if (page_index >= handle->config.page_count) {
        EFLASH_EXIT_CRITICAL();
        return false;
    }

    bool result = handle->page_erased[page_index];

    EFLASH_EXIT_CRITICAL();

    return result;
}
