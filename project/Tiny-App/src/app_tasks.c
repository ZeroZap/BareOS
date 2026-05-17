/**
 * @file app_tasks.c
 * @brief PC application tasks — heartbeat + command shell.
 *
 * heartbeat_process: fires every 1 s and logs the current tick.
 * cmd_process:       wraps tiny_cmd; stdin bytes arrive as APP_EVT_UART_BYTE
 *                    events from main (pc_uart_poll → process_post).
 */

#include "app_tasks.h"
#include "app_timer.h"
#include "app_sys.h"
#include "app_io.h"
#include "pc_uart.h"
#include "xy_log.h"
#include "etimer.h"

extern volatile unsigned int g_sys_tick_ms;

/* ── Shared shell instance ───────────────────────────────────────────── */

tiny_cmd_t g_shell;

/* ── Built-in test commands ──────────────────────────────────────────── */

static int cmd_info(tiny_cmd_t *sh, int argc, char *argv[])
{
    (void)argc; (void)argv;
    tiny_cmd_printf(sh, "BareOS PC build — tick=%u ms\r\n", g_sys_tick_ms);
    return 0;
}

static int cmd_log_level(tiny_cmd_t *sh, int argc, char *argv[])
{
    if (argc < 2) {
        tiny_cmd_printf(sh, "Usage: loglevel <0-3>\r\n");
        return -1;
    }
    uint8_t lv = (uint8_t)(argv[1][0] - '0');
    extern void xy_log_set_dynamic_level(uint8_t);
    xy_log_set_dynamic_level(lv);
    tiny_cmd_printf(sh, "log level = %u\r\n", lv);
    return 0;
}

static const tiny_cmd_entry_t g_cmds[] = {
    { "info",     "Print system info",        cmd_info      },
    { "loglevel", "loglevel <0-3>",            cmd_log_level },
};

/* ── heartbeat_process ────────────────────────────────────────────────── */

PROCESS(heartbeat_process, "Heartbeat");

PROCESS_THREAD(heartbeat_process, ev, data)
{
    static struct etimer t;
    PROCESS_BEGIN();

    xy_log_i("heartbeat started");

    while (1) {
        etimer_set(&t, 1000u);
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&t));
        xy_log_i("[%u ms] alive", g_sys_tick_ms);
    }

    PROCESS_END();
}

/* ── cmd_process ─────────────────────────────────────────────────────── */

PROCESS(cmd_process, "CMD Shell");

PROCESS_THREAD(cmd_process, ev, data)
{
    PROCESS_BEGIN();

    tiny_cmd_init(&g_shell, "cmd> ", true, pc_uart_write_str);
    tiny_cmd_register(g_cmds, sizeof(g_cmds) / sizeof(g_cmds[0]));
    app_timer_register_cmds();
    app_sys_register_cmds();
    app_io_register_cmds();
    xy_log_i("shell ready — type 'help'");

    while (1) {
        /* PROCESS_YIELD always suspends; the process is resumed for each
         * event posted to it (APP_EVT_UART_BYTE from on_uart_rx).
         * PROCESS_WAIT_EVENT_UNTIL must NOT be used here: when ev already
         * matches the condition it never suspends, causing an infinite loop
         * that blocks process_run() entirely. */
        PROCESS_YIELD();
        tiny_cmd_process(&g_shell);
    }

    PROCESS_END();
}
