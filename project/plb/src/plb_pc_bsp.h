#ifndef PLB_PC_BSP_H
#define PLB_PC_BSP_H

#include <stdint.h>

void plb_pc_bsp_init(void);
void plb_pc_tick_advance(uint32_t delta_ms);

int plb_pc_flash_erase(uint32_t addr, uint32_t len);
int plb_pc_flash_write(uint32_t addr, const void *buf, uint32_t len);
int plb_pc_flash_read(uint32_t addr, void *buf, uint32_t len);

#define PLB_PC_FLASH_BASE       0u
#define PLB_PC_FLASH_PAGE_SIZE  256u
#define PLB_PC_FLASH_SIZE       1024u

#endif /* PLB_PC_BSP_H */
