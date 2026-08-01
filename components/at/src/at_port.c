/**
 * @file at_port.c
 * @brief AT-Command-V2 bare-metal platform adapter.
 *
 * Provides the three functions required by at_chat.c:
 *   at_malloc()   — memory allocation
 *   at_free()     — memory free
 *   at_get_ms()   — millisecond timestamp derived from the BareOS tick
 *
 * No RTOS is required.  The at_adapter_t.lock/unlock fields in your
 * adapter struct must be set to NULL (see 架构设计.md §5.2).
 *
 * HOW TO WIRE UP the system tick:
 *   Define XY_TICK_HZ to match the BSP tick rate.
 *   Call xy_tick_advance(1) from the tick ISR, or advance by the elapsed
 *   low-power timer ticks after tickless sleep.
 */

#include "../include/at_port.h"
#include "xy_mem.h"  /* xy_malloc / xy_free */
#include "xy_tick.h"

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
    return xy_tick_now_ms();
}
