/**
 * @file xy_fota_flash.c
 * @brief FOTA Flash Interface Implementation (STM32 Internal Flash)
 * @version 1.0.0
 * @date 2026-03-01 自主学习
 */

#include "xy_fota.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* STM32 Internal Flash 基地址 */
#define FLASH_BASE_ADDR         0x08000000

/* Flash 扇区大小 (根据具体型号调整) */
#define FLASH_SECTOR_SIZE       0x1000  /* 4KB */

/* FOTA 存储区配置 */
#define FOTA_SLOT0_ADDR         (FLASH_BASE_ADDR + 0x60000)  /* 384KB 处 */
#define FOTA_SLOT1_ADDR         (FLASH_BASE_ADDR + 0x80000)  /* 512KB 处 */
#define FOTA_SLOT_SIZE          0x20000  /* 128KB per slot */

/**
 * @brief 解锁 Flash
 */
static void flash_unlock(void)
{
#ifdef MCU_STM32
    /* STM32 HAL Flash Unlock */
    HAL_FLASH_Unlock();
#elif defined(MCU_WCH)
    /* WCH Flash Unlock */
    FLASH_Unlock();
#endif
}

/**
 * @brief 锁定 Flash
 */
static void flash_lock(void)
{
#ifdef MCU_STM32
    /* STM32 HAL Flash Lock */
    HAL_FLASH_Lock();
#elif defined(MCU_WCH)
    /* WCH Flash Lock */
    FLASH_Lock();
#endif
}

/**
 * @brief 擦除 Flash 扇区
 */
static int flash_erase_sector(uint32_t addr, uint32_t size)
{
    uint32_t current_addr = addr;
    uint32_t remaining = size;
    
    while (remaining > 0) {
#ifdef MCU_STM32
        /* 计算扇区号 */
        uint32_t sector = FLASH_GetSector(current_addr);
        
        /* STM32 Flash Erase */
        FLASH_EraseInitTypeDef erase;
        uint32_t sector_error;
        
        erase.TypeErase = FLASH_TYPEERASE_SECTORS;
        erase.Sector = sector;
        erase.NbSectors = 1;
        erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
        
        if (HAL_FLASHEx_Erase(&erase, &sector_error) != HAL_OK) {
            return -1;
        }
#elif defined(MCU_WCH)
        /* WCH Flash Erase */
        FLASH_ErasePage(current_addr);
#endif
        
        current_addr += FLASH_SECTOR_SIZE;
        remaining -= FLASH_SECTOR_SIZE;
    }
    
    return 0;
}

/**
 * @brief 写入 Flash
 */
static int flash_program(uint32_t addr, const uint8_t *data, uint32_t size)
{
    uint32_t i;
    
    for (i = 0; i < size; i += 4) {
        uint32_t word;
        
        /* 组合 32 位数据 */
        word = ((uint32_t)data[i]) |
               ((uint32_t)data[i + 1] << 8) |
               ((uint32_t)data[i + 2] << 16) |
               ((uint32_t)data[i + 3] << 24);
        
#ifdef MCU_STM32
        /* STM32 Flash Program */
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr + i, word) != HAL_OK) {
            return -1;
        }
#elif defined(MCU_WCH)
        /* WCH Flash Program */
        FLASH_Program_Word(addr + i, word);
#endif
    }
    
    return 0;
}

/**
 * @brief FOTA Flash 初始化
 */
static int xy_fota_flash_init(void)
{
    flash_unlock();
    xy_log_d("FOTA Flash initialized\n");
    return 0;
}

/**
 * @brief FOTA Flash 去初始化
 */
static int xy_fota_flash_deinit(void)
{
    flash_lock();
    xy_log_d("FOTA Flash deinitialized\n");
    return 0;
}

/**
 * @brief FOTA Flash 读取
 */
static int xy_fota_flash_read(uint32_t addr, uint8_t *data, uint32_t size)
{
    if (!data || size == 0) {
        return -1;
    }
    
    /* 直接内存拷贝 (Flash 映射到内存空间) */
    memcpy(data, (const void *)addr, size);
    
    return 0;
}

/**
 * @brief FOTA Flash 写入
 */
static int xy_fota_flash_write(uint32_t addr, const uint8_t *data, uint32_t size)
{
    if (!data || size == 0) {
        return -1;
    }
    
    /* 检查地址对齐 */
    if (addr & 0x03) {
        xy_log_e("Flash write address not aligned\n");
        return -1;
    }
    
    return flash_program(addr, data, size);
}

/**
 * @brief FOTA Flash 擦除
 */
static int xy_fota_flash_erase(uint32_t addr, uint32_t size)
{
    /* 检查扇区对齐 */
    if (addr & (FLASH_SECTOR_SIZE - 1)) {
        xy_log_w("Flash erase address not sector aligned\n");
    }
    
    return flash_erase_sector(addr, size);
}

/**
 * @brief FOTA Flash 操作接口
 */
const xy_fota_flash_ops_t g_fota_flash_ops = {
    .init = xy_fota_flash_init,
    .deinit = xy_fota_flash_deinit,
    .read = xy_fota_flash_read,
    .write = xy_fota_flash_write,
    .erase = xy_fota_flash_erase,
};

/**
 * @brief 获取 Flash 操作接口
 */
const xy_fota_flash_ops_t* xy_fota_get_flash_ops(void)
{
    return &g_fota_flash_ops;
}
