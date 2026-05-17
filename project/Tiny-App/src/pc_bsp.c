#include "pc_bsp.h"
#include "xy_mem.h"
#include "rtimer.h"
#include "xy_sys.h"
#include "xy_io.h"
#include "xy_log.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

/* ── Tick ────────────────────────────────────────────────────────────── */

volatile unsigned int g_sys_tick_ms = 0;

static clock_t s_clk_start;

/* ── Default heap pool (32 KB) ───────────────────────────────────────── */

#define PC_HEAP_SIZE (32u * 1024u)

static unsigned char s_heap_buf[PC_HEAP_SIZE];
static xy_mem_pool_t s_heap_pool;

void pc_bsp_init(void)
{
    s_clk_start = clock();
    xy_mem_pool_init(&s_heap_pool, "pc_heap", s_heap_buf, sizeof(s_heap_buf));
    rtimer_init();
}

void pc_tick_update(void)
{
    clock_t now = clock();
    g_sys_tick_ms = (unsigned int)(
        (unsigned long long)(now - s_clk_start) * 1000ULL / (unsigned long long)CLOCKS_PER_SEC);
}

/* ── BSP hook: log output ────────────────────────────────────────────── */

void xy_log_char(char ch)
{
    putchar((unsigned char)ch);
    fflush(stdout);
}

/* ── Simulated NOR flash ─────────────────────────────────────────────── */

static uint8_t s_flash[PC_FLASH_NUM_SECTORS * PC_FLASH_SECTOR_SIZE];

int pc_flash_read(uint32_t addr, void *buf, uint32_t len)
{
    if (addr + len > sizeof(s_flash)) return -1;
    memcpy(buf, s_flash + addr, len);
    return 0;
}

int pc_flash_write(uint32_t addr, const void *buf, uint32_t len)
{
    if (addr + len > sizeof(s_flash)) return -1;
    /* NOR flash: can only clear bits (AND with new data). */
    const uint8_t *src = (const uint8_t *)buf;
    for (uint32_t i = 0; i < len; i++)
        s_flash[addr + i] &= src[i];
    return 0;
}

int pc_flash_erase(uint32_t addr, uint32_t len)
{
    if (addr + len > sizeof(s_flash)) return -1;
    memset(s_flash + addr, 0xFF, len);
    return 0;
}

/* ── rtimer PC arch ──────────────────────────────────────────────────── */

static rtimer_clock_t s_rtimer_when;
static int            s_rtimer_armed;

void rtimer_arch_init(void)    { s_rtimer_armed = 0; }

rtimer_clock_t rtimer_arch_now(void)
{
    /* Scale ms tick → rtimer ticks (RTIMER_SECOND / 1000 per ms). */
    return (rtimer_clock_t)((unsigned long long)g_sys_tick_ms *
                             (RTIMER_SECOND / 1000UL));
}

void rtimer_arch_schedule(rtimer_clock_t when)
{
    s_rtimer_when = when;
    s_rtimer_armed = 1;
}

void pc_rtimer_poll(void)
{
    if (s_rtimer_armed && rtimer_arch_now() >= s_rtimer_when) {
        s_rtimer_armed = 0;
        rtimer_run();
    }
}

/* ── sys PC BSP (override weak stubs in xy_sys.c) ───────────────────── */

void xy_sys_hw_reset(void)
{
    exit(0);
}

xy_reset_cause_t xy_sys_get_reset_cause_hw(void)
{
    return XY_RESET_CAUSE_POWER_ON;
}

int xy_sys_get_chip_id_hw(uint8_t *buf, int maxlen)
{
    /* Fake 8-byte chip ID: "PC-BOS\x01\x00" */
    static const uint8_t s_id[8] = {0x50,0x43,0x2D,0x42,0x4F,0x53,0x01,0x00};
    int n = (maxlen < 8) ? maxlen : 8;
    memcpy(buf, s_id, (size_t)n);
    return n;
}

/* ── io PC BSP (override weak stubs in xy_io.c) ─────────────────────── */

static const char * const s_led_names[XY_LED_COUNT] = {
    "STATUS", "ALARM", "GPS"
};

void xy_io_led_hw_set(xy_led_id_t id, bool on)
{
    if (id < XY_LED_COUNT)
        xy_log_d("[LED] %s=%s", s_led_names[id], on ? "ON " : "OFF");
}
