/**
 * @file app_sys.c
 * @brief System info module: logs boot cause / version, exposes shell commands.
 */

#include "app_sys.h"
#include "xy_sys.h"
#include "xy_log.h"
#include "tiny_cmd.h"

/* ── Shell commands ──────────────────────────────────────────────────── */

static int cmd_sysinfo(tiny_cmd_t *sh, int argc, char *argv[])
{
    (void)argc; (void)argv;
    uint8_t id[8];
    int n = xy_sys_get_chip_id(id, (int)sizeof(id));

    tiny_cmd_printf(sh, "sw=%s  hw=%s\r\n",
                    xy_sys_get_sw_ver(), xy_sys_get_hw_ver());
    tiny_cmd_printf(sh, "reset=%s\r\n",
                    xy_sys_reset_cause_str(xy_sys_get_reset_cause()));
    tiny_cmd_printf(sh, "chip=");
    for (int i = 0; i < n; i++)
        tiny_cmd_printf(sh, "%X%X", (unsigned)(id[i] >> 4), (unsigned)(id[i] & 0xFu));
    tiny_cmd_printf(sh, "\r\n");
    return 0;
}

static int cmd_reset(tiny_cmd_t *sh, int argc, char *argv[])
{
    (void)sh; (void)argc; (void)argv;
    xy_log_w("[sys] software reset requested");
    xy_sys_reset();   /* does not return */
    return 0;
}

static const tiny_cmd_entry_t s_sys_cmds[] = {
    { "sysinfo", "System version / reset cause / chip ID", cmd_sysinfo },
    { "reset",   "Trigger software reset",                 cmd_reset   },
};

/* ── Public API ──────────────────────────────────────────────────────── */

void app_sys_init(void)
{
    uint8_t id[8];
    int n = xy_sys_get_chip_id(id, (int)sizeof(id));
    xy_log_i("[sys] sw=%s  hw=%s  reset=%s  chip=",
             xy_sys_get_sw_ver(), xy_sys_get_hw_ver(),
             xy_sys_reset_cause_str(xy_sys_get_reset_cause()));
    /* chip ID bytes logged separately (xy_log_i doesn't handle %02X arrays) */
    for (int i = 0; i < n; i++) {
        char hex[3];
        hex[0] = "0123456789ABCDEF"[id[i] >> 4];
        hex[1] = "0123456789ABCDEF"[id[i] & 0xF];
        hex[2] = '\0';
        xy_log_raw(hex, 2);
    }
}

void app_sys_register_cmds(void)
{
    tiny_cmd_register(s_sys_cmds,
                      sizeof(s_sys_cmds) / sizeof(s_sys_cmds[0]));
}
