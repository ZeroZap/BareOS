#ifndef PLB_N32_FLASH_FEE_H
#define PLB_N32_FLASH_FEE_H

#include <stdint.h>
#include "xy_fee_nano.h"
#include "xy_eeprom.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PLB_N32_FLASH_BASE_ADDR       0x08000000u
#define PLB_N32_FLASH_TOTAL_SIZE      0x00020000u
#define PLB_N32_FLASH_PAGE_SIZE       0x00000800u
#define PLB_N32_FLASH_PAGE_COUNT      64u

#define PLB_N32_FEE_PAGE_COUNT        2u
#define PLB_N32_FEE_TOTAL_SIZE        (PLB_N32_FLASH_PAGE_SIZE * PLB_N32_FEE_PAGE_COUNT)
#define PLB_N32_FEE_BASE_ADDR         \
    (PLB_N32_FLASH_BASE_ADDR + PLB_N32_FLASH_TOTAL_SIZE - PLB_N32_FEE_TOTAL_SIZE)

#define PLB_N32_EEPROM_PAGE_COUNT     2u
#define PLB_N32_EEPROM_TOTAL_SIZE     \
    (PLB_N32_FLASH_PAGE_SIZE * PLB_N32_EEPROM_PAGE_COUNT)
#define PLB_N32_EEPROM_BASE_ADDR      \
    (PLB_N32_FEE_BASE_ADDR - PLB_N32_EEPROM_TOTAL_SIZE)

typedef struct {
    uint8_t ip[4];
    uint16_t port;
    uint16_t reserved;
} plb_n32_server_endpoint_t;

eflash_result_t plb_n32_fee_init(void);
eflash_t *plb_n32_fee(void);

xy_eeprom_result_t plb_n32_eeprom_init(void);
xy_eeprom_t *plb_n32_eeprom(void);

eflash_result_t plb_n32_boot_count_update(uint32_t *boot_count);
uint32_t plb_n32_boot_count_get(void);

eflash_result_t plb_n32_server_endpoint_load(plb_n32_server_endpoint_t *endpoint);
eflash_result_t plb_n32_server_endpoint_save(const plb_n32_server_endpoint_t *endpoint);

eflash_result_t plb_n32_flash_read(uint32_t offset, uint8_t *data, size_t size);
eflash_result_t plb_n32_flash_write(uint32_t offset, const uint8_t *data,
                                    size_t size);
eflash_result_t plb_n32_flash_erase_page(uint32_t page_index);

#ifdef __cplusplus
}
#endif

#endif /* PLB_N32_FLASH_FEE_H */
