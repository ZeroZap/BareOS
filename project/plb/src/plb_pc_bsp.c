#include "plb_pc_bsp.h"

#include "xy_mem.h"

#include <stdio.h>
#include <string.h>

volatile unsigned int g_sys_tick_ms;

static uint8_t s_flash[PLB_PC_FLASH_SIZE];
static uint8_t s_heap[128u * 1024u];
static xy_mem_pool_t s_heap_pool;
static int s_heap_inited;

void plb_pc_bsp_init(void)
{
    g_sys_tick_ms = 0;
    memset(s_flash, 0xFF, sizeof(s_flash));
    if (!s_heap_inited) {
        xy_mem_pool_init(&s_heap_pool, "plb_pc_heap", s_heap, sizeof(s_heap));
        s_heap_inited = 1;
    } else {
        xy_mem_pool_reset(&s_heap_pool);
    }
    xy_mem_set_default_pool(&s_heap_pool);
}

void plb_pc_tick_advance(uint32_t delta_ms)
{
    g_sys_tick_ms += delta_ms;
}

void xy_log_char(char ch)
{
    putchar((unsigned char)ch);
    fflush(stdout);
}

int plb_pc_flash_erase(uint32_t addr, uint32_t len)
{
    if (addr + len > sizeof(s_flash)) return -1;
    memset(&s_flash[addr], 0xFF, len);
    return 0;
}

int plb_pc_flash_write(uint32_t addr, const void *buf, uint32_t len)
{
    const uint8_t *src = (const uint8_t *)buf;
    if (addr + len > sizeof(s_flash)) return -1;
    for (uint32_t i = 0; i < len; i++) {
        s_flash[addr + i] &= src[i];
    }
    return 0;
}

int plb_pc_flash_read(uint32_t addr, void *buf, uint32_t len)
{
    if (addr + len > sizeof(s_flash)) return -1;
    memcpy(buf, &s_flash[addr], len);
    return 0;
}
