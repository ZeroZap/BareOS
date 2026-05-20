/**
 * @file xy_ats.c
 * @brief AT Command Server — bare-metal, main-loop driven.
 */

#include "xy_ats.h"
#include "xy_stdio.h"
#include "xy_string.h"

/* ── Init ───────────────────────────────────────────────────────────── */

int at_server_init(at_server_t *server, const char *name)
{
    if (!server) return -1;
    memset(server, 0, sizeof(at_server_t));
    server->name      = name;
    server->status    = ATS_SERVER_STATUS_INITIALIZED;
    server->echo_mode = ATS_SERVER_ECHO_MODE;
    ats_hash_init(&server->cmd_table);
    return 0;
}

at_server_t *at_server_create(const char *name)
{
    static at_server_t s_inst;
    at_server_init(&s_inst, name);
    return &s_inst;
}

void at_server_delete(at_server_t *server)
{
    if (server) at_server_stop(server);
}

/* ── HAL / lifecycle ────────────────────────────────────────────────── */

int at_server_set_hal(at_server_t *server,
                      int (*get_char)(char *ch, uint32_t timeout),
                      size_t (*send)(const char *data, size_t len))
{
    if (!server) return -1;
    server->get_char = get_char;
    server->send     = send;
    return 0;
}

int at_server_start(at_server_t *server)
{
    if (!server) return -1;
    server->status         = ATS_SERVER_STATUS_RUNNING;
    server->parser_running = true;
    ATS_DBG("started: %s\n", server->name);
    return 0;
}

int at_server_stop(at_server_t *server)
{
    if (!server) return -1;
    server->parser_running = false;
    server->status         = ATS_SERVER_STATUS_INITIALIZED;
    ATS_DBG("stopped: %s\n", server->name);
    return 0;
}

/* ── Bare-metal feed API ────────────────────────────────────────────── */

void at_server_feed_byte(at_server_t *server, char ch)
{
    if (!server || server->status != ATS_SERVER_STATUS_RUNNING) return;

    if (server->echo_mode && server->send) {
        server->send(&ch, 1);
    }

    if (ch == '\r' || ch == '\n') {
        if (server->recv_len > 0) {
            server->recv_buf[server->recv_len] = '\0';
            at_server_process_command(server, server->recv_buf);
            server->recv_len = 0;
        }
        return;
    }

    if (server->recv_len < ATS_SERVER_RECV_BUF_SIZE - 1) {
        server->recv_buf[server->recv_len++] = ch;
    }
}

/* ── Command registration ───────────────────────────────────────────── */

int at_server_register_cmd(at_server_t *server, const at_cmd_t *cmd)
{
    if (!server || !cmd) return -1;
    if (!ats_hash_insert(&server->cmd_table, cmd->name, (struct at_cmd *)cmd))
        return -1;
    ATS_DBG("registered: %s\n", cmd->name);
    return 0;
}

int at_server_unregister_cmd(at_server_t *server, const char *name)
{
    if (!server || !name) return -1;
    return ats_hash_remove(&server->cmd_table, name) ? 0 : -1;
}

at_cmd_t *at_server_find_cmd(at_server_t *server, const char *name)
{
    if (!server || !name) return NULL;
    return (at_cmd_t *)ats_hash_find(&server->cmd_table, name);
}

/* ── Command dispatch ───────────────────────────────────────────────── */

int at_server_process_command(at_server_t *server, const char *cmd_line)
{
    if (!server || !cmd_line) return -1;

    /* Skip "AT" prefix */
    const char *p = cmd_line;
    if (strncmp(p, "AT", 2) == 0) p += 2;
    if (*p == '+') p++;

    /* Build lookup name (strip =? / ? / =args) */
    char lookup[ATS_CMD_NAME_MAX_LEN];
    strncpy(lookup, p, sizeof(lookup) - 1);
    lookup[sizeof(lookup) - 1] = '\0';

    char *eq  = strchr(lookup, '=');
    char *qm  = strchr(lookup, '?');
    if (eq) *eq = '\0';
    else if (qm) *qm = '\0';

    at_cmd_t *cmd = at_server_find_cmd(server, lookup);
    if (!cmd) {
        if (server->send) server->send("ERROR\r\n", 7);
        server->cmd_error++;
        return -1;
    }
    server->cmd_processed++;

    /* Determine mode */
    at_cmd_mode_t mode = ATS_CMD_MODE_EXEC;
    const char *args   = NULL;

    if (strstr(p, "=?")) {
        mode = ATS_CMD_MODE_TEST;
    } else if (qm && !eq) {
        mode = ATS_CMD_MODE_QUERY;
    } else if (eq) {
        mode = ATS_CMD_MODE_SETUP;
        args = strchr(p, '=') + 1;
    }

    at_result_t result = ATS_RESULT_FAIL;
    switch (mode) {
    case ATS_CMD_MODE_TEST:  if (cmd->test)         result = cmd->test();         break;
    case ATS_CMD_MODE_QUERY: if (cmd->query)        result = cmd->query();        break;
    case ATS_CMD_MODE_SETUP: if (cmd->setup)        result = cmd->setup(args);    break;
    case ATS_CMD_MODE_EXEC:  if (cmd->exec)         result = cmd->exec();         break;
    }

    at_server_print_result(server, result);
    return 0;
}

/* ── Response helpers ───────────────────────────────────────────────── */

int at_server_printf(at_server_t *server, const char *fmt, ...)
{
    if (!server || !fmt || !server->send) return -1;
    char buf[ATS_SERVER_SEND_BUF_SIZE];
    va_list ap;
    va_start(ap, fmt);
    int n = (int)xy_vsprintf(buf, fmt, ap);
    va_end(ap);
    if (n > 0) server->send(buf, (size_t)n);
    return n;
}

int at_server_printfln(at_server_t *server, const char *fmt, ...)
{
    if (!server || !fmt || !server->send) return -1;
    char buf[ATS_SERVER_SEND_BUF_SIZE];
    va_list ap;
    va_start(ap, fmt);
    int n = (int)xy_vsprintf(buf, fmt, ap);
    va_end(ap);
    if (n > 0 && n < (int)(sizeof(buf) - 2)) {
        buf[n++] = '\r';
        buf[n++] = '\n';
        buf[n]   = '\0';
    }
    if (n > 0) server->send(buf, (size_t)n);
    return n;
}

int at_server_print_result(at_server_t *server, at_result_t result)
{
    if (!server || !server->send) return -1;
    if (result == ATS_RESULT_OK) {
        server->send("OK\r\n", 4);
        server->cmd_ok++;
    } else if (result != ATS_RESULT_NULL) {
        server->send("ERROR\r\n", 7);
        server->cmd_error++;
    }
    return 0;
}

size_t at_server_send(at_server_t *server, const char *data, size_t len)
{
    if (!server || !server->send) return 0;
    return server->send(data, len);
}

/* ── Echo ───────────────────────────────────────────────────────────── */

void at_server_set_echo(at_server_t *server, bool enable)
{
    if (server) server->echo_mode = enable;
}

bool at_server_get_echo(at_server_t *server)
{
    return server ? server->echo_mode : false;
}

/* ── Stats ──────────────────────────────────────────────────────────── */

void at_server_get_stats(at_server_t *server, uint32_t *cmd_processed,
                         uint32_t *cmd_ok, uint32_t *cmd_error)
{
    if (!server) return;
    if (cmd_processed) *cmd_processed = server->cmd_processed;
    if (cmd_ok)        *cmd_ok        = server->cmd_ok;
    if (cmd_error)     *cmd_error     = server->cmd_error;
}

void at_server_reset_stats(at_server_t *server)
{
    if (!server) return;
    server->cmd_processed = 0;
    server->cmd_ok        = 0;
    server->cmd_error     = 0;
}

at_server_t *at_server_get_by_name(const char *name)
{
    static at_server_t s_inst;
    if (name && s_inst.name && strcmp(name, s_inst.name) == 0)
        return &s_inst;
    return NULL;
}

/* ── Parameter parsing ──────────────────────────────────────────────── */

int at_parse_int(const char *args, int *value)
{
    if (!args || !value) return -1;
    char *end;
    *value = (int)xy_strtol(args, &end, 10);
    return (end != args) ? 0 : -1;
}

int at_parse_string(const char *args, char *value, size_t max_len)
{
    if (!args || !value || max_len == 0) return -1;
    /* Strip surrounding quotes if present */
    if (*args == '"') args++;
    size_t i = 0;
    while (*args && *args != '"' && *args != ',' && i < max_len - 1)
        value[i++] = *args++;
    value[i] = '\0';
    return 0;
}

int at_parse_hex(const char *args, uint32_t *value)
{
    if (!args || !value) return -1;
    if (args[0] == '0' && (args[1] == 'x' || args[1] == 'X')) args += 2;
    char *end;
    *value = (uint32_t)xy_strtoul(args, &end, 16);
    return (end != args) ? 0 : -1;
}

int at_parse_args(const char *args, const char *format, ...)
{
    if (!args || !format) return 0;
    va_list ap;
    va_start(ap, format);
    int n = (int)xy_vsscanf(args, format, ap);
    va_end(ap);
    return n;
}
