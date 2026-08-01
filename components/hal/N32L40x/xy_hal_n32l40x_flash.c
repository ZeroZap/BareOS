#include "xy_hal_n32l40x_internal.h"

int xy_hal_flash_read(void *flash, uint32_t address, void *data, size_t len)
{
    (void)flash;
    if (!data && len) return XY_HAL_INVALID_PARAM;
    memcpy(data, (const void *)address, len);
    return XY_HAL_OK;
}

int xy_hal_flash_write(void *flash, uint32_t address, const void *data, size_t len)
{
    const uint8_t *p = (const uint8_t *)data;
    size_t i;
    (void)flash;
    if ((!data && len) || ((address | len) & 0x3u)) return XY_HAL_INVALID_PARAM;
    int result = XY_HAL_OK;
    if (FLASH_ClockInit() == FLASH_HSICLOCK_DISABLE) return XY_HAL_BUSY;
    if (xy_hal_power_acquire_lock(XY_HAL_PM_LOCK_STOP) != XY_HAL_OK) return XY_HAL_BUSY;
    FLASH_Unlock();
    for (i = 0u; i < len; i += 4u) {
        uint32_t word;
        memcpy(&word, &p[i], sizeof(word));
        if (FLASH_ProgramWord(address + (uint32_t)i, word) != FLASH_COMPL) {
            result = XY_HAL_ERROR;
            break;
        }
    }
    FLASH_Lock();
    (void)xy_hal_power_release_lock(XY_HAL_PM_LOCK_STOP);
    return result;
}

int xy_hal_flash_erase(void *flash, uint32_t address, size_t len)
{
    uint32_t end = address + (uint32_t)len;
    int result = XY_HAL_OK;
    (void)flash;
    if ((address & (XY_HAL_N32_FLASH_PAGE_SIZE - 1u)) != 0u) return XY_HAL_INVALID_PARAM;
    if (FLASH_ClockInit() == FLASH_HSICLOCK_DISABLE) return XY_HAL_BUSY;
    if (xy_hal_power_acquire_lock(XY_HAL_PM_LOCK_STOP) != XY_HAL_OK) return XY_HAL_BUSY;
    FLASH_Unlock();
    while (address < end) {
        if (FLASH_EraseOnePage(address) != FLASH_COMPL) {
            result = XY_HAL_ERROR;
            break;
        }
        address += XY_HAL_N32_FLASH_PAGE_SIZE;
    }
    FLASH_Lock();
    (void)xy_hal_power_release_lock(XY_HAL_PM_LOCK_STOP);
    return result;
}

int xy_hal_flash_get_info(void *flash, xy_hal_flash_info_t *info)
{
    (void)flash;
    if (!info) return XY_HAL_INVALID_PARAM;
    info->base_addr = XY_HAL_N32_FLASH_BASE;
    info->total_size = XY_HAL_N32_FLASH_SIZE;
    info->erase_size = XY_HAL_N32_FLASH_PAGE_SIZE;
    info->write_unit = 4u;
    info->erased_value = 0xffu;
    return XY_HAL_OK;
}
