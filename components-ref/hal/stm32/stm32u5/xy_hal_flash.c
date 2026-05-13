/**
 * @file xy_hal_flash.c
 * @brief Flash HAL STM32U5 Implementation
 * @version 2.0
 * @date 2026-02-28
 */

#include "../../inc/xy_hal_flash.h"

#if defined(STM32U5) || defined(STM32U5xx)

#include "stm32u5xx_hal.h"
#include <string.h>

/* Flash context structure */
typedef struct {
    void *flash;
    uint8_t initialized;
    uint8_t locked;
} flash_ctx_t;

static flash_ctx_t g_flash_ctx = { 0 };

/* Flash constants for STM32U5 */
#define FLASH_BASE_ADDR         0x08000000U
#define FLASH_PAGE_SIZE         0x2000U     /* 8KB per page */
#define FLASH_SECTOR_SIZE       0x8000U     /* 32KB per sector */
#define FLASH_WRITE_ALIGNMENT   16U         /* 128-bit write alignment */

/* Flash sector information */
static const xy_hal_flash_sector_info_t g_sector_info[] = {
    { .start_addr = 0x08000000U, .size = FLASH_SECTOR_SIZE, .is_protected = 0 },
    { .start_addr = 0x08008000U, .size = FLASH_SECTOR_SIZE, .is_protected = 0 },
    { .start_addr = 0x08010000U, .size = FLASH_SECTOR_SIZE, .is_protected = 0 },
    { .start_addr = 0x08018000U, .size = FLASH_SECTOR_SIZE, .is_protected = 0 },
    { .start_addr = 0x08020000U, .size = FLASH_SECTOR_SIZE, .is_protected = 0 },
    { .start_addr = 0x08028000U, .size = FLASH_SECTOR_SIZE, .is_protected = 0 },
    { .start_addr = 0x08030000U, .size = FLASH_SECTOR_SIZE, .is_protected = 0 },
    { .start_addr = 0x08038000U, .size = FLASH_SECTOR_SIZE, .is_protected = 0 },
};

#define FLASH_SECTOR_COUNT    (sizeof(g_sector_info) / sizeof(g_sector_info[0]))

xy_hal_error_t xy_hal_flash_init(void *flash)
{
    XY_UNUSED(flash);

    if (g_flash_ctx.initialized) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }

    g_flash_ctx.flash      = flash;
    g_flash_ctx.initialized = 1;
    g_flash_ctx.locked      = 1; /* Flash is locked after reset */

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_flash_deinit(void *flash)
{
    XY_UNUSED(flash);

    if (!g_flash_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    /* Ensure flash is locked before deinit */
    if (!g_flash_ctx.locked) {
        HAL_FLASH_Lock();
        g_flash_ctx.locked = 1;
    }

    g_flash_ctx.initialized = 0;
    g_flash_ctx.flash       = NULL;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_flash_read(void *flash, uint32_t addr, uint8_t *data,
                                 uint32_t size)
{
    XY_UNUSED(flash);

    if (!data || size == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (!xy_hal_flash_is_valid_address(NULL, addr)) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    /* Flash read is simple memory access */
    memcpy(data, (const uint8_t *)addr, size);

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_flash_write(void *flash, uint32_t addr,
                                  const uint8_t *data, uint32_t size)
{
    XY_UNUSED(flash);

    if (!data || size == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (!xy_hal_flash_is_valid_address(NULL, addr)) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    /* Check alignment */
    if (addr % FLASH_WRITE_ALIGNMENT != 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (g_flash_ctx.locked) {
        return XY_HAL_ERROR_NOT_INIT; /* Flash must be unlocked first */
    }

    /* Write data in 128-bit (16-byte) chunks */
    uint32_t offset = 0;
    while (offset < size) {
        uint64_t write_data[2] = { 0, 0 };

        /* Copy data to write buffer */
        size_t remaining = size - offset;
        if (remaining >= 16) {
            memcpy(write_data, data + offset, 16);
        } else {
            memcpy(write_data, data + offset, remaining);
        }

        /* Program 128-bit data */
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, addr + offset,
                              (uint64_t)write_data[0]) != HAL_OK) {
            return XY_HAL_ERROR_FAIL;
        }

        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, addr + offset + 8,
                              (uint64_t)write_data[1]) != HAL_OK) {
            return XY_HAL_ERROR_FAIL;
        }

        offset += 16;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_flash_erase(void *flash, uint32_t addr, uint32_t size)
{
    XY_UNUSED(flash);
    XY_UNUSED(size);

    if (!xy_hal_flash_is_valid_address(NULL, addr)) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (g_flash_ctx.locked) {
        return XY_HAL_ERROR_NOT_INIT; /* Flash must be unlocked first */
    }

    /* Calculate page number */
    uint32_t page_num = (addr - FLASH_BASE_ADDR) / FLASH_PAGE_SIZE;

    FLASH_EraseInitTypeDef erase_init = { 0 };
    erase_init.TypeErase    = FLASH_TYPEERASE_PAGES;
    erase_init.Page         = page_num;
    erase_init.NbPages      = 1;

    uint32_t error = 0;
    if (HAL_FLASHEx_Erase(&erase_init, &error) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_flash_mass_erase(void *flash)
{
    XY_UNUSED(flash);

    if (g_flash_ctx.locked) {
        return XY_HAL_ERROR_NOT_INIT; /* Flash must be unlocked first */
    }

    FLASH_EraseInitTypeDef erase_init = { 0 };
    erase_init.TypeErase = FLASH_TYPEERASE_MASSERASE;

    uint32_t error = 0;
    if (HAL_FLASHEx_Erase(&erase_init, &error) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_flash_lock(void *flash)
{
    XY_UNUSED(flash);

    if (!g_flash_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    HAL_FLASH_Lock();
    g_flash_ctx.locked = 1;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_flash_unlock(void *flash)
{
    XY_UNUSED(flash);

    if (!g_flash_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    if (HAL_FLASH_Unlock() != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    g_flash_ctx.locked = 0;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_flash_set_read_protect(void *flash,
                                             xy_hal_flash_rdp_level_t level)
{
    XY_UNUSED(flash);
    XY_UNUSED(level);

    /* Read protection requires special handling */
    /* This is a simplified implementation */
    return XY_HAL_ERROR_NOT_SUPPORT;
}

xy_hal_error_t xy_hal_flash_get_read_protect(void *flash,
                                             xy_hal_flash_rdp_level_t *level)
{
    if (!level) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    XY_UNUSED(flash);

    /* Get current RDP level */
    FLASH_OBProgramInitTypeDef ob_init = { 0 };
    HAL_FLASHEx_OBGetConfig(&ob_init);

    switch (ob_init.RDPLevel) {
    case OB_RDP_LEVEL_0:
        *level = XY_HAL_FLASH_RDP_LEVEL_0;
        break;
    case OB_RDP_LEVEL_1:
        *level = XY_HAL_FLASH_RDP_LEVEL_1;
        break;
    case OB_RDP_LEVEL_2:
        *level = XY_HAL_FLASH_RDP_LEVEL_2;
        break;
    default:
        *level = XY_HAL_FLASH_RDP_LEVEL_0;
        break;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_flash_get_info(void *flash, xy_hal_flash_info_t *info)
{
    XY_UNUSED(flash);

    if (!info) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    info->flash_size      = FLASH_SECTOR_COUNT * FLASH_SECTOR_SIZE;
    info->sector_count    = FLASH_SECTOR_COUNT;
    info->page_size       = FLASH_PAGE_SIZE;
    info->write_alignment = FLASH_WRITE_ALIGNMENT;
    info->sectors         = g_sector_info;

    return XY_HAL_OK;
}

int xy_hal_flash_is_valid_address(void *flash, uint32_t addr)
{
    XY_UNUSED(flash);

    uint32_t flash_size = FLASH_SECTOR_COUNT * FLASH_SECTOR_SIZE;
    return (addr >= FLASH_BASE_ADDR && addr < (FLASH_BASE_ADDR + flash_size));
}

xy_hal_error_t xy_hal_flash_get_sector_info(void *flash, uint32_t sector_num,
                                            xy_hal_flash_sector_info_t *info)
{
    XY_UNUSED(flash);

    if (!info) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (sector_num >= FLASH_SECTOR_COUNT) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    *info = g_sector_info[sector_num];

    return XY_HAL_OK;
}

#endif /* STM32U5 || STM32U5xx */
