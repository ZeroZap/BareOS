/**
 * @file app_io.c
 * @brief IO demo: blinks LED_STATUS via ctimer (500 ms), exposes led command.
 */

#include "app_io.h"
#include "xy_io.h"
#include "ctimer.h"
#include "xy_log.h"
#include "tiny_cmd.h"
#include <string.h>

/* ── LED blink ctimer ────────────────────────────────────────────────── */

static struct ctimer s_blink_ct;

static void on_blink(void *p)
{
    (void)p;
    xy_io_led_toggle(XY_LED_STATUS);
    ctimer_reset(&s_blink_ct);
}

/* ── io_process ──────────────────────────────────────────────────────── */

PROCESS(io_process, "IO Demo");

PROCESS_THREAD(io_process, ev, data)
{
    (void)ev; (void)data;
    PROCESS_BEGIN();

    xy_io_init();
    ctimer_set(&s_blink_ct, 500u, on_blink, NULL);
    xy_log_i("[io] LED_STATUS blink started (500ms)");

    PROCESS_END();
}

/* ── Shell command: led ──────────────────────────────────────────────── */

static int cmd_led(tiny_cmd_t *sh, int argc, char *argv[])
{
    if (argc < 3) {
        tiny_cmd_printf(sh, "Usage: led <0|1|2> <on|off|toggle>\r\n");
        tiny_cmd_printf(sh, "  0=STATUS  1=ALARM  2=GPS\r\n");
        return -1;
    }

    int id = argv[1][0] - '0';
    if (id < 0 || id >= (int)XY_LED_COUNT) {
        tiny_cmd_printf(sh, "bad id (0-%d)\r\n", (int)XY_LED_COUNT - 1);
        return -1;
    }

    if (strcmp(argv[2], "on") == 0) {
        xy_io_led_set((xy_led_id_t)id, true);
    } else if (strcmp(argv[2], "off") == 0) {
        xy_io_led_set((xy_led_id_t)id, false);
    } else if (strcmp(argv[2], "toggle") == 0) {
        xy_io_led_toggle((xy_led_id_t)id);
    } else {
        tiny_cmd_printf(sh, "unknown action '%s'\r\n", argv[2]);
        return -1;
    }

    tiny_cmd_printf(sh, "LED[%d] = %s\r\n", id,
                    xy_io_led_get((xy_led_id_t)id) ? "ON" : "OFF");
    return 0;
}

static int cmd_btn(tiny_cmd_t *sh, int argc, char *argv[])
{
    (void)argc; (void)argv;
    tiny_cmd_printf(sh, "BTN_ACTIVATE=%d  BTN_TEST=%d\r\n",
                    (int)xy_io_btn_pressed(XY_BTN_ACTIVATE),
                    (int)xy_io_btn_pressed(XY_BTN_TEST));
    return 0;
}

static const tiny_cmd_entry_t s_io_cmds[] = {
    { "led", "led <0-2> <on|off|toggle>",  cmd_led },
    { "btn", "Read button states",          cmd_btn },
};

void app_io_register_cmds(void)
{
    tiny_cmd_register(s_io_cmds,
                      sizeof(s_io_cmds) / sizeof(s_io_cmds[0]));
}
