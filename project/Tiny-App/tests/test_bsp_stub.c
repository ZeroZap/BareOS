/*
 * test_bsp_stub.c — minimal BSP symbols needed to link components into
 * the standalone bareos_tests binary. Provides only what the audit-fix
 * regression tests touch transitively.
 */

#include <stdint.h>
#include <stdio.h>

/* Required by components/at/src/at_port.c → at_get_ms() */
volatile unsigned int g_sys_tick_ms = 0;

/* Required by components/log/xy_log.c */
void xy_log_char(char ch) { (void)fputc(ch, stderr); }
