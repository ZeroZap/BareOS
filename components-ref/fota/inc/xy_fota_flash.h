/**
 * @file xy_fota_flash.h
 * @brief FOTA Flash Abstraction Layer
 * @version 2.0
 * @date 2026-04-02
 *
 * @attention
 * This header provides Flash abstraction for FOTA operations.
 * The xy_fota_flash_ops_t struct is defined in xy_fota.h.
 *
 * Platform support:
 *   - STM32 (HAL_FLASH)
 *   - WCH RISC-V (CH32V series)
 *   - HC32
 *
 * Flash layout:
 *   - Internal Flash: Bootloader + Dual Bank FOTA slots
 *   - External Flash: Optional NOR Flash for backup/images
 */

#ifndef XY_FOTA_FLASH_H
#define XY_FOTA_FLASH_H

#include <stdint.h>
#include <stdbool.h>
#include "xy_fota.h"

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * Platform Detection
 *============================================================================*/

/**
 * @brief MCU platform definitions
 */
#if defined(MCU_STM32)
    #define FOTA_PLATFORM_STM32    1
    #define FOTA_PLATFORM_WCH      0
    #define FOTA_PLATFORM_HC32     0
#elif defined(MCU_WCH)
    #define FOTA_PLATFORM_STM32    0
    #define FOTA_PLATFORM_WCH      1
    #define FOTA_PLATFORM_HC32     0
#elif defined(MCU_HC32)
    #define FOTA_PLATFORM_STM32    0
    #define FOTA_PLATFORM_WCH      0
    #define FOTA_PLATFORM_HC32     1
#else
    #define FOTA_PLATFORM_STM32    1  /* Default to STM32 */
    #define FOTA_PLATFORM_WCH      0
    #define FOTA_PLATFORM_HC32     0
#endif

/*==============================================================================
 * Flash Layout Definitions
 *============================================================================*/

/**
 * @brief STM32 Internal Flash Configuration
 */
#define FOTA_FLASH_BASE_ADDR       0x08000000  /**< STM32 Flash base */

/**
 * @brief Flash sector sizes
 */
#if defined(FLASH_SECTOR_SIZE_4K)
    #define FOTA_FLASH_SECTOR_SIZE  0x1000    /* 4KB sector */
#elif defined(FLASH_SECTOR_SIZE_16K)
    #define FOTA_FLASH_SECTOR_SIZE  0x4000    /* 16KB sector */
#elif defined(FLASH_SECTOR_SIZE_32K)
    #define FOTA_FLASH_SECTOR_SIZE  0x8000    /* 32KB sector */
#else
    #define FOTA_FLASH_SECTOR_SIZE  0x1000    /* Default 4KB */
#endif

/**
 * @brief FOTA dual-bank slot configuration
 *
 * Default layout for 512KB Flash:
 *   - 0x08000000 - 0x08003FFF: Bootloader (16KB)
 *   - 0x08004000 - 0x0805FFFF: App + FOTA (384KB)
 *   - 0x08060000 - 0x0807FFFF: Slot0 (128KB)
 *   - 0x08080000 - 0x0809FFFF: Slot1 (128KB)
 */
#ifndef FOTA_SLOT0_ADDR
    #define FOTA_SLOT0_ADDR         (FOTA_FLASH_BASE_ADDR + 0x60000)  /**< 384KB offset */
#endif

#ifndef FOTA_SLOT1_ADDR
    #define FOTA_SLOT1_ADDR         (FOTA_FLASH_BASE_ADDR + 0x80000)  /**< 512KB offset */
#endif

#ifndef FOTA_SLOT_SIZE
    #define FOTA_SLOT_SIZE          0x20000  /**< 128KB per slot */
#endif

/**
 * @brief Backup region (for single-slot mode)
 */
#ifndef FOTA_BACKUP_ADDR
    #define FOTA_BACKUP_ADDR        (FOTA_FLASH_BASE_ADDR + 0x40000)  /**< 256KB offset */
#endif

#ifndef FOTA_BACKUP_SIZE
    #define FOTA_BACKUP_SIZE        0x20000  /**< 128KB backup region */
#endif

/*==============================================================================
 * External NOR Flash Support (Optional)
 *============================================================================*/

/**
 * @brief External NOR Flash configuration
 */
typedef struct {
    uint32_t base_addr;           /**< NOR Flash base address */
    uint32_t capacity;            /**< Total capacity in bytes */
    uint32_t page_size;           /**< Program page size */
    uint32_t sector_size;          /**< Sector erase size */
    uint8_t  bus_width;           /**< Bus width (1/2/4) */
    bool     quad_enable;         /**< Quad mode enable */
} fota_norflash_config_t;

/**
 * @brief NOR Flash operation results
 */
typedef enum {
    FOTA_NOR_OK = 0,
    FOTA_NOR_ERROR = -1,
    FOTA_NOR_TIMEOUT = -2,
    FOTA_NOR_BUSY = -3,
    FOTA_NOR_NOT_FOUND = -4
} fota_nor_status_t;

/**
 * @brief External NOR Flash read function
 */
typedef int (*fota_nor_read_t)(uint32_t addr, uint8_t *data, uint32_t size);

/**
 * @brief External NOR Flash write function
 */
typedef int (*fota_nor_write_t)(uint32_t addr, const uint8_t *data, uint32_t size);

/**
 * @brief External NOR Flash erase function
 */
typedef int (*fota_nor_erase_t)(uint32_t addr, uint32_t size);

/**
 * @brief External NOR Flash operations structure
 */
typedef struct {
    fota_nor_read_t   read;
    fota_nor_write_t  write;
    fota_nor_erase_t  erase;
    uint32_t base_addr;           /**< Base address of NOR Flash */
} fota_norflash_ops_t;

/*==============================================================================
 * Flash Verification and Utilities
 *============================================================================*/

/**
 * @brief Flash verification modes
 */
typedef enum {
    FOTA_VERIFY_CRC32 = 0,       /**< CRC32 verification */
    FOTA_VERIFY_HASH,            /**< SHA256 hash verification */
    FOTA_VERIFY_SIGNATURE        /**< ECDSA signature verification */
} fota_verify_mode_t;

/**
 * @brief Flash operation error codes
 */
#define FOTA_FLASH_OK             0
#define FOTA_FLASH_ERROR         (-1)
#define FOTA_FLASH_TIMEOUT       (-2)
#define FOTA_FLASH_ALIGN_ERROR   (-3)
#define FOTA_FLASH_VERIFY_ERROR  (-4)

/*==============================================================================
 * Function Declarations
 *============================================================================*/

/**
 * @brief Get internal Flash operations interface
 * @return Pointer to xy_fota_flash_ops_t structure
 *
 * @note This function returns operations for internal MCU Flash
 *       (STM32/WCH/HC32 internal Flash)
 */
const xy_fota_flash_ops_t* xy_fota_get_flash_ops(void);

/**
 * @brief Initialize FOTA Flash (wrapper)
 * @return 0 on success, negative on error
 */
int xy_fota_flash_init(void);

/**
 * @brief De-initialize FOTA Flash (wrapper)
 * @return 0 on success, negative on error
 */
int xy_fota_flash_deinit(void);

/**
 * @brief Read from FOTA Flash
 * @param addr Flash address
 * @param data Data buffer
 * @param size Number of bytes to read
 * @return 0 on success, negative on error
 */
int xy_fota_flash_read(uint32_t addr, uint8_t *data, uint32_t size);

/**
 * @brief Write to FOTA Flash
 * @param addr Flash address (must be aligned)
 * @param data Data to write
 * @param size Number of bytes to write
 * @return 0 on success, negative on error
 */
int xy_fota_flash_write(uint32_t addr, const uint8_t *data, uint32_t size);

/**
 * @brief Erase FOTA Flash sectors
 * @param addr Start address (should be sector-aligned)
 * @param size Number of bytes to erase
 * @return 0 on success, negative on error
 */
int xy_fota_flash_erase(uint32_t addr, uint32_t size);

/**
 * @brief Verify Flash region
 * @param addr Start address
 * @param size Size to verify
 * @param expected_crc Expected CRC32 value
 * @return true if verification passes
 */
bool xy_fota_flash_verify(uint32_t addr, uint32_t size, uint32_t expected_crc);

/**
 * @brief Get Flash slot information
 * @param slot Slot number (0 or 1)
 * @param addr Output: slot start address
 * @param size Output: slot size
 * @return 0 on success, negative on error
 */
int xy_fota_get_slot_info(uint8_t slot, uint32_t *addr, uint32_t *size);

/**
 * @brief Check if slot is valid
 * @param slot Slot number
 * @return true if slot contains valid firmware
 */
bool xy_fota_is_slot_valid(uint8_t slot);

/**
 * @brief Initialize external NOR Flash operations
 * @param config NOR Flash configuration
 * @param ops Flash operations structure
 * @return 0 on success, negative on error
 */
int xy_fota_norflash_init(const fota_norflash_config_t *config,
                          fota_norflash_ops_t *ops);

/**
 * @brief Get external NOR Flash operations
 * @return Pointer to fota_norflash_ops_t structure, or NULL if not available
 */
const fota_norflash_ops_t* xy_fota_get_norflash_ops(void);

/*==============================================================================
 * Platform-Specific Flash Operations (Internal)
 *============================================================================*/

/**
 * @brief Unlock Flash for writing
 */
void fota_flash_unlock(void);

/**
 * @brief Lock Flash after writing
 */
void fota_flash_lock(void);

/**
 * @brief Get Flash sector number for address
 * @param addr Flash address
 * @return Sector number
 */
uint32_t fota_flash_get_sector(uint32_t addr);

/**
 * @brief Wait for Flash operation to complete
 * @param timeout_ms Timeout in milliseconds
 * @return 0 on success, negative on timeout
 */
int fota_flash_wait_ready(uint32_t timeout_ms);

/*==============================================================================
 * Compile-time Assertions
 *============================================================================*/

/**
 * @brief Verify alignment requirements
 */
_Static_assert(sizeof(xy_fota_flash_ops_t) >= 5 * sizeof(void*),
                "xy_fota_flash_ops_t must have 5 function pointers");

#ifdef __cplusplus
}
#endif

#endif /* XY_FOTA_FLASH_H */
