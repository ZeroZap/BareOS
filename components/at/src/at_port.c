/**
 * @file at_port.c
 * @brief AT-Command-V2 bare-metal platform adapter.
 *
 * Provides the three functions required by at_chat.c:
 *   at_malloc()   — memory allocation
 *   at_free()     — memory free
 *   at_get_ms()   — millisecond timestamp
 *
 * No RTOS is required.  The at_adapter_t.lock/unlock fields in your
 * adapter struct must be set to NULL (see 架构设计.md §5.2).
 *
 * HOW TO WIRE UP sys_tick_ms:
 *   Declare  volatile uint32_t g_sys_tick_ms;  in bsp/bsp_tick.c
 *   Increment it inside SysTick_Handler():
 *       void SysTick_Handler(void) { g_sys_tick_ms++; }
 *   Expose it via  extern volatile uint32_t g_sys_tick_ms;  in bsp/bsp_tick.h
 */

#include "../include/at_port.h"
#include "xy_mem.h"  /* xy_malloc / xy_free */

/* ── millisecond counter provided by the BSP SysTick handler ─────── */
extern volatile unsigned int g_sys_tick_ms;

/* ── Memory ──────────────────────────────────────────────────────── */

void *at_malloc(unsigned int nbytes)
{
    return xy_malloc(nbytes);
}

void at_free(void *ptr)
{
    xy_free(ptr);
}

/* ── Time ────────────────────────────────────────────────────────── */

unsigned int at_get_ms(void)
{
    return g_sys_tick_ms;
}
