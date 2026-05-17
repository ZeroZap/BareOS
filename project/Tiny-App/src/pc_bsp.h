#ifndef PC_BSP_H
#define PC_BSP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Shared tick counter — incremented by pc_tick_update(). */
extern volatile unsigned int g_sys_tick_ms;

/* Call once at startup. */
void pc_bsp_init(void);

/* Call each main-loop iteration to advance g_sys_tick_ms. */
void pc_tick_update(void);

/* ── Simulated flash (for xy_flog tests) ─────────────────────────────
 * 8 × 4096-byte sectors = 32 KB of fake NOR flash.
 */
#define PC_FLASH_SECTOR_SIZE  4096u
#define PC_FLASH_NUM_SECTORS  8u
#define PC_FLASH_BASE         0x00000000u

int pc_flash_read (uint32_t addr, void *buf, uint32_t len);
int pc_flash_write(uint32_t addr, const void *buf, uint32_t len);
int pc_flash_erase(uint32_t addr, uint32_t len);

/* Poll simulated rtimer — call each main-loop iteration. */
void pc_rtimer_poll(void);

#ifdef __cplusplus
}
#endif

#endif /* PC_BSP_H */
