/**
 * @file main.c
 * @brief BareOS PC minimal system entry point.
 *
 * Main loop pattern mirrors the target firmware:
 *
 *   while (1) {
 *       pc_tick_update();      // advance g_sys_tick_ms from host clock
 *       pc_uart_poll();        // drain stdin → rx callback → process_post
 *       ctimer_run();          // fire expired callback timers
 *       process_run();         // run etimer_run + dispatch events to tasks
 *   }
 *
 * UART rx bytes are posted as APP_EVT_UART_BYTE to cmd_process, which feeds
 * them into the tiny_cmd shell.  The shell writes output via pc_uart_write_str
 * (→ stdout).
 *
 * heartbeat_process logs a 1-second alive message using etimer.
 */

#include "pc_bsp.h"
#include "pc_uart.h"
#include "app_tasks.h"
#include "app_sys.h"
#include "xy_log.h"
#include "ctimer.h"
#include "process.h"

/* ── UART rx callback ─────────────────────────────────────────────────
 * Called from pc_uart_poll() — ONE byte per main-loop iteration.
 * Because pc_uart_poll() reads exactly one byte per call and the main
 * loop runs process_run() after each byte, the command is always
 * dispatched (line_ready cleared) before the next line's first byte
 * arrives, so tiny_cmd_feed_byte() never drops input.
 */
static void on_uart_rx(uint8_t byte, void *arg)
{
    (void)arg;
    tiny_cmd_feed_byte(&g_shell, (char)byte);
    process_post(&cmd_process, APP_EVT_UART_BYTE, NULL);
}

/* ── Entry point ─────────────────────────────────────────────────────── */

int main(void)
{
    pc_bsp_init();
    xy_log_init();         /* routes xy_printf → xy_log_char → stdout */

    xy_log_i("=== BareOS PC minimal system ===");
    xy_log_i("Components: clib, crypto, pt, sys, io, btn, gnss, ais, dsc, epirb, uwb, flog, cmd");
    app_sys_init();        /* log reset cause + version + chip ID */

    /* Wire stdin → tiny_cmd. */
    pc_uart_init(on_uart_rx, NULL);

    /* Start application tasks. */
    process_start(&heartbeat_process, NULL);
    process_start(&cmd_process,       NULL);
    process_start(&timer_process,     NULL);
    process_start(&io_process,        NULL);

    /* ── Main loop ─────────────────────────────────────────────────── */
    while (1) {
        pc_tick_update();   /* keep g_sys_tick_ms current                */
        pc_uart_poll();     /* read one stdin byte → on_uart_rx          */
        pc_rtimer_poll();   /* fire rtimer callbacks when deadline hits  */
        ctimer_run();       /* advance callback timers                   */
        process_run();      /* etimer expiry + event dispatch to tasks   */
    }

    return 0;
}
