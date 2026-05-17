#include "../xy_cell_drv.h"
#include "xy_string.h"
#include "xy_stdio.h"

/* ML302 (CMCC Cat.1) driver.
 * No native MQTT — use TCP socket + a software MQTT stack.
 * Socket send: hex-encoded via AT+MIPSEND (max ~100 bytes per call). */

#define ML302_SEND_MAX  100  /* max raw bytes per MIPSEND */

/* ── hex encode helper ────────────────────────────────────────────────── */

static void to_hex(const uint8_t *src, int len, char *dst)
{
    static const char hx[] = "0123456789ABCDEF";
    int i;
    for (i = 0; i < len; i++) {
        dst[i * 2]     = hx[src[i] >> 4];
        dst[i * 2 + 1] = hx[src[i] & 0x0F];
    }
    dst[len * 2] = '\0';
}

/* hex decode helper — used for +MIPRTCP: receive */
static int from_hex(const char *src, uint8_t *dst, int maxlen)
{
    int n = 0;
    while (src[0] && src[1] && n < maxlen) {
        uint8_t hi = (src[0] >= 'A') ? (uint8_t)(src[0] - 'A' + 10)
                                      : (uint8_t)(src[0] - '0');
        uint8_t lo = (src[1] >= 'A') ? (uint8_t)(src[1] - 'A' + 10)
                                      : (uint8_t)(src[1] - '0');
        dst[n++] = (uint8_t)((hi << 4) | lo);
        src += 2;
    }
    return n;
}

/* ── socket receive URC
 *  "+MIPRTCP: <id>,<len>,<hexdata>"  (endmark '\n')
 * ─────────────────────────────────────────────────────────────────────── */

static int urc_miprtcp(at_urc_info_t *info)
{
    const char *p = strchr(info->urcbuf, ':');
    if (!p) return 0;
    p++;
    while (*p == ' ') p++;

    int id  = (int)xy_strtol(p, (char **)&p, 10);
    if (*p == ',') p++;
    int len = (int)xy_strtol(p, (char **)&p, 10);
    if (*p == ',') p++;

    if (id < 0 || id >= XY_CELL_SOCK_MAX || len <= 0) return 0;
    if (!g_cell_socks[id].open) return 0;

    uint8_t tmp[256];
    int n = from_hex(p, tmp, (int)sizeof(tmp));
    if (n > 0)
        xy_rb_put(&g_cell_socks[id].rx, tmp, (uint32_t)n);

    (void)len;
    return 0;
}

/* "+MIPCLOSE: <id>"  (endmark '\n') */
static int urc_mipclose(at_urc_info_t *info)
{
    const char *p = strchr(info->urcbuf, ':');
    if (!p) return 0;
    p++;
    while (*p == ' ') p++;
    int id = (int)xy_strtol(p, NULL, 10);
    if (id >= 0 && id < XY_CELL_SOCK_MAX)
        g_cell_socks[id].open = false;
    return 0;
}

static const urc_item_t s_urc_tbl[] = {
    { "+MIPRTCP:",  '\n', urc_miprtcp  },
    { "+MIPCLOSE:", '\n', urc_mipclose },
};

/* ── PDP ─────────────────────────────────────────────────────────────── */

static int work_pdp_activate(at_env_t *env)
{
    xy_cell_op_t *op = (xy_cell_op_t *)env->params;
    switch (env->state) {
    case 0:
        env->println(env, "AT+MIPCALL=1,\"%s\"", op->pdp.apn);
        env->reset_timer(env);
        env->state = 1;
        break;
    case 1:
        /* "+MIPCALL: 1,<ip>" or "OK" */
        if (env->contains(env, "+MIPCALL:")) {
            cell_parse_ip(env->recvbuf(env), g_cell.ip, (int)sizeof(g_cell.ip));
            g_cell.state = CELL_ST_ONLINE;
            OP_OK(op, env);
        } else if (env->contains(env, "OK")) {
            /* Get IP via query */
            env->recvclr(env);
            env->println(env, "AT+MIPCALL?");
            env->reset_timer(env);
            env->state = 2;
        } else if (env->contains(env, "ERROR") || env->is_timeout(env, 60000)) {
            OP_ERR(op, env);
        }
        break;
    case 2:
        if (env->contains(env, "+MIPCALL:")) {
            cell_parse_ip(env->recvbuf(env), g_cell.ip, (int)sizeof(g_cell.ip));
            g_cell.state = CELL_ST_ONLINE;
            OP_OK(op, env);
        } else if (env->contains(env, "ERROR") || env->is_timeout(env, 5000)) {
            g_cell.state = CELL_ST_ONLINE;
            OP_OK(op, env);
        }
        break;
    }
    return 0;
}

static int work_pdp_deactivate(at_env_t *env)
{
    xy_cell_op_t *op = (xy_cell_op_t *)env->params;
    switch (env->state) {
    case 0:
        env->println(env, "AT+MIPCALL=0");
        env->reset_timer(env);
        env->state = 1;
        break;
    case 1:
        if (env->contains(env, "OK") || env->contains(env, "ERROR") ||
            env->is_timeout(env, 10000)) {
            g_cell.state = CELL_ST_REGISTERED;
            memset(g_cell.ip, 0, sizeof(g_cell.ip));
            OP_OK(op, env);
        }
        break;
    }
    return 0;
}

/* ── Socket ───────────────────────────────────────────────────────────── */

static int work_sock_open(at_env_t *env)
{
    xy_cell_op_t *op = (xy_cell_op_t *)env->params;
    switch (env->state) {
    case 0: {
        /* mode: 1=TCP, 0=UDP */
        int mode = (strcmp(op->sock_open.proto, "UDP") == 0) ? 0 : 1;
        env->println(env, "AT+MIPOPEN=%d,%d,\"%s\",%d",
                     op->sock_open.id, mode,
                     op->sock_open.host, (int)op->sock_open.port);
        env->reset_timer(env);
        env->state = 1;
        break;
    }
    case 1:
        if (env->contains(env, "OK") || env->contains(env, "+MIPOPEN:")) {
            g_cell_socks[op->sock_open.id].open = true;
            g_cell_socks[op->sock_open.id].tls  = false;
            OP_OK(op, env);
        } else if (env->contains(env, "ERROR") || env->is_timeout(env, 30000)) {
            OP_ERR(op, env);
        }
        break;
    }
    return 0;
}

static int work_sock_send(at_env_t *env)
{
    xy_cell_op_t *op = (xy_cell_op_t *)env->params;
    /* Hex-encode up to ML302_SEND_MAX bytes per call */
    static char s_hexbuf[ML302_SEND_MAX * 2 + 1];
    switch (env->state) {
    case 0: {
        int n = (op->sock_send.len > ML302_SEND_MAX)
                ? ML302_SEND_MAX : (int)op->sock_send.len;
        to_hex((const uint8_t *)op->sock_send.data, n, s_hexbuf);
        env->println(env, "AT+MIPSEND=%d,\"%s\"",
                     op->sock_send.id, s_hexbuf);
        env->reset_timer(env);
        env->state = 1;
        break;
    }
    case 1:
        if (env->contains(env, "OK") || env->contains(env, "+MIPSEND:")) {
            OP_OK(op, env);
        } else if (env->contains(env, "ERROR") || env->is_timeout(env, 10000)) {
            OP_ERR(op, env);
        }
        break;
    }
    return 0;
}

static int work_sock_close(at_env_t *env)
{
    xy_cell_op_t *op = (xy_cell_op_t *)env->params;
    switch (env->state) {
    case 0:
        env->println(env, "AT+MIPCLOSE=%d", op->sock_close.id);
        env->reset_timer(env);
        env->state = 1;
        break;
    case 1:
        if (env->contains(env, "OK") || env->contains(env, "ERROR") ||
            env->is_timeout(env, 10000)) {
            g_cell_socks[op->sock_close.id].open = false;
            OP_OK(op, env);
        }
        break;
    }
    return 0;
}

/* ── vtable (no native MQTT — mqtt_* entries are NULL) ───────────────── */

const xy_cell_drv_t xy_cell_drv_ml302 = {
    .name             = "ML302",
    .pdp_activate     = work_pdp_activate,
    .pdp_deactivate   = work_pdp_deactivate,
    .sock_open        = work_sock_open,
    .sock_send        = work_sock_send,
    .sock_close       = work_sock_close,
    .mqtt_connect     = NULL,
    .mqtt_publish     = NULL,
    .mqtt_subscribe   = NULL,
    .mqtt_disconnect  = NULL,
    .urc_tbl          = s_urc_tbl,
    .urc_cnt          = (int)(sizeof(s_urc_tbl) / sizeof(s_urc_tbl[0])),
};
