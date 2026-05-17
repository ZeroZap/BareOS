/* Fibocom L610 (UNISOC Cat.1) driver.
 *
 * AT command reference:
 *   PDP        : AT+CGDCONT / AT+CGACT / AT+CGPADDR (standard 3GPP)
 *   TCP socket : AT+GTCPOPEN / AT+GTCPSEND / AT+GTCPCLOSE
 *   UDP socket : AT+GTUDPOPEN / AT+GTUDPSEND / AT+GTUDPCLOSE
 *   TLS socket : AT+GTSSLCFG / AT+GTSSLOPENEX / AT+GTSSLCLOSE
 *                (TLS send uses same command as TCP: AT+GTCPSEND)
 *   MQTT       : AT+GTMQTTOPEN / AT+GTMQTTSUB / AT+GTMQTTPUB /
 *                AT+GTMQTTCLOSE
 *   Receive    : +GTRECV: <id>,<len>\n<binary>
 *   MQTT recv  : +GTMQTTRECV: <cid>,"<topic>",<len>\n<payload>
 */

#include "../xy_cell_drv.h"
#include "xy_string.h"
#include "xy_stdio.h"

/* ── socket receive URC
 *  "+GTRECV: <id>,<len>\n<binary data>"  (endmark '\n')
 * ─────────────────────────────────────────────────────────────────────── */

static int urc_gtrecv(at_urc_info_t *info)
{
    const char *p = info->urcbuf;
    int hdr_end = 0, i;

    for (i = 0; i < info->urclen; i++) {
        if (p[i] == '\n') { hdr_end = i + 1; break; }
    }
    if (!hdr_end) return 0;

    if (info->urclen == hdr_end) {
        /* First call: parse "+GTRECV: <id>,<len>" */
        const char *q = strchr(p, ':');
        if (!q) return 0;
        q++;
        while (*q == ' ') q++;
        int id  = (int)xy_strtol(q, (char **)&q, 10);
        if (*q == ',') q++;
        int len = (int)xy_strtol(q, NULL, 10);
        if (id < 0 || id >= XY_CELL_SOCK_MAX || len <= 0) return 0;
        return len + 2; /* binary data + CRLF */
    }

    /* Second call: full buffer — re-parse id then copy data */
    {
        const char *q = strchr(p, ':');
        if (!q) return 0;
        q++;
        while (*q == ' ') q++;
        int id  = (int)xy_strtol(q, (char **)&q, 10);
        if (*q == ',') q++;
        int len = (int)xy_strtol(q, NULL, 10);
        if (id < 0 || id >= XY_CELL_SOCK_MAX || len <= 0) return 0;

        if (g_cell_socks[id].open) {
            int avail = info->urclen - hdr_end;
            int n = (avail < len) ? avail : len;
            xy_rb_put(&g_cell_socks[id].rx,
                      (const uint8_t *)(info->urcbuf + hdr_end), (uint32_t)n);
        }
    }
    return 0;
}

/* "+GTCLOSE: <id>"  (endmark '\n') */
static int urc_gtclose(at_urc_info_t *info)
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

/* ── MQTT receive URC
 *  "+GTMQTTRECV: <cid>,"<topic>",<len>\n<payload>"  (endmark '\n')
 * ─────────────────────────────────────────────────────────────────────── */

static struct {
    int  cid;
    char topic[128];
    int  payloadlen;
} s_mqrx;

static int urc_gtmqttrecv(at_urc_info_t *info)
{
    const char *p = info->urcbuf;
    int hdr_end = 0, i;

    for (i = 0; i < info->urclen; i++) {
        if (p[i] == '\n') { hdr_end = i + 1; break; }
    }
    if (!hdr_end) return 0;

    if (info->urclen == hdr_end) {
        /* First call: parse header */
        const char *q = strchr(p, ':');
        if (!q) return 0;
        q++;
        while (*q == ' ') q++;
        s_mqrx.cid = (int)xy_strtol(q, (char **)&q, 10);
        if (*q == ',') q++;

        /* topic in quotes */
        if (*q == '"') {
            q++;
            const char *ts = q;
            while (*q && *q != '"') q++;
            int n = (int)(q - ts);
            if (n >= (int)sizeof(s_mqrx.topic)) n = (int)sizeof(s_mqrx.topic) - 1;
            strncpy(s_mqrx.topic, ts, n);
            s_mqrx.topic[n] = '\0';
            if (*q == '"') q++;
        }
        if (*q == ',') q++;
        s_mqrx.payloadlen = (int)xy_strtol(q, NULL, 10);
        return (s_mqrx.payloadlen > 0 ? s_mqrx.payloadlen : 1) + 2;
    }

    /* Second call: deliver payload */
    {
        const char *payload = p + hdr_end;
        int paylen = info->urclen - hdr_end;
        while (paylen > 0 &&
               (payload[paylen - 1] == '\r' || payload[paylen - 1] == '\n'))
            paylen--;
        xy_cell_mqtt_deliver(s_mqrx.cid,
                             s_mqrx.topic, (uint16_t)strlen(s_mqrx.topic),
                             payload, (uint16_t)paylen);
    }
    return 0;
}

static const urc_item_t s_urc_tbl[] = {
    { "+GTRECV:",     '\n', urc_gtrecv     },
    { "+GTCLOSE:",    '\n', urc_gtclose    },
    { "+GTMQTTRECV:", '\n', urc_gtmqttrecv },
};

/* ── PDP ─────────────────────────────────────────────────────────────── */

static int work_pdp_activate(at_env_t *env)
{
    xy_cell_op_t *op = (xy_cell_op_t *)env->params;
    switch (env->state) {
    case 0:
        env->println(env, "AT+CGDCONT=1,\"IP\",\"%s\"", op->pdp.apn);
        env->reset_timer(env);
        env->state = 1;
        break;
    case 1:
        if (env->contains(env, "OK")) {
            env->recvclr(env);
            env->println(env, "AT+CGACT=1,1");
            env->reset_timer(env);
            env->state = 2;
        } else if (env->contains(env, "ERROR") || env->is_timeout(env, 5000)) {
            OP_ERR(op, env);
        }
        break;
    case 2:
        if (env->contains(env, "OK")) {
            env->recvclr(env);
            env->println(env, "AT+CGPADDR=1");
            env->reset_timer(env);
            env->state = 3;
        } else if (env->contains(env, "ERROR") || env->is_timeout(env, 30000)) {
            OP_ERR(op, env);
        }
        break;
    case 3:
        if (env->contains(env, "+CGPADDR:")) {
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
        env->println(env, "AT+CGACT=0,1");
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
    case 0:
        if (op->sock_open.tls) {
            /* Configure SSL version: 5 = TLSv1.2 */
            env->println(env, "AT+GTSSLCFG=%d,5", op->sock_open.id);
            env->reset_timer(env);
            env->state = 1;
        } else if (strcmp(op->sock_open.proto, "UDP") == 0) {
            env->println(env, "AT+GTUDPOPEN=%d,\"%s\",%d",
                         op->sock_open.id,
                         op->sock_open.host, (int)op->sock_open.port);
            env->reset_timer(env);
            env->state = 3;
        } else {
            env->println(env, "AT+GTCPOPEN=%d,\"%s\",%d",
                         op->sock_open.id,
                         op->sock_open.host, (int)op->sock_open.port);
            env->reset_timer(env);
            env->state = 3;
        }
        break;
    case 1:
        /* Wait for GTSSLCFG OK */
        if (env->contains(env, "OK") || env->contains(env, "ERROR") ||
            env->is_timeout(env, 3000)) {
            env->recvclr(env);
            /* ssl_id = id, tcp_id = id */
            env->println(env, "AT+GTSSLOPENEX=%d,%d,\"%s\",%d",
                         op->sock_open.id, op->sock_open.id,
                         op->sock_open.host, (int)op->sock_open.port);
            env->reset_timer(env);
            env->state = 2;
        }
        break;
    case 2:
        /* "+GTSSLOPEN: <ssl_id>,<tcp_id>,0" — last field 0=success */
        if (env->contains(env, "+GTSSLOPEN:")) {
            const char *r = env->contains(env, ",0");
            if (r) {
                g_cell_socks[op->sock_open.id].open = true;
                g_cell_socks[op->sock_open.id].tls  = true;
                OP_OK(op, env);
            } else {
                OP_ERR(op, env);
            }
        } else if (env->contains(env, "ERROR") || env->is_timeout(env, 30000)) {
            OP_ERR(op, env);
        }
        break;
    case 3:
        /* "+GTCPOPEN: <id>,0" or "+GTUDPOPEN: <id>,0" — 0=success */
        if (env->contains(env, "+GTCPOPEN:") ||
            env->contains(env, "+GTUDPOPEN:")) {
            const char *r = env->contains(env, ",0");
            if (r) {
                g_cell_socks[op->sock_open.id].open = true;
                g_cell_socks[op->sock_open.id].tls  = false;
                OP_OK(op, env);
            } else {
                OP_ERR(op, env);
            }
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
    switch (env->state) {
    case 0:
        /* TLS and TCP both use AT+GTCPSEND at the AT layer */
        env->println(env, "AT+GTCPSEND=%d,%d",
                     op->sock_send.id, (int)op->sock_send.len);
        env->reset_timer(env);
        env->state = 1;
        break;
    case 1:
        if (env->contains(env, ">")) {
            env->obj->adap->write(op->sock_send.data,
                                  (unsigned int)op->sock_send.len);
            env->recvclr(env);
            env->reset_timer(env);
            env->state = 2;
        } else if (env->is_timeout(env, 5000)) {
            OP_ERR(op, env);
        }
        break;
    case 2:
        /* "+GTCPSEND: <id>,OK" or plain "OK" */
        if (env->contains(env, ",OK") || env->contains(env, "OK")) {
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
    case 0: {
        int id = op->sock_close.id;
        if (g_cell_socks[id].tls) {
            env->println(env, "AT+GTSSLCLOSE=%d", id);
        } else {
            env->println(env, "AT+GTCPCLOSE=%d", id);
        }
        env->reset_timer(env);
        env->state = 1;
        break;
    }
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

/* ── Module MQTT ─────────────────────────────────────────────────────── */

static int work_mqtt_connect(at_env_t *env)
{
    xy_cell_op_t *op = (xy_cell_op_t *)env->params;
    switch (env->state) {
    case 0:
        /* AT+GTMQTTOPEN=<cid>,<keepalive>,"<host>",<port>,
         *               "<client_id>","<user>","<pass>",<tls>  */
        env->println(env,
            "AT+GTMQTTOPEN=%d,60,\"%s\",%d,\"%s\",\"%s\",\"%s\",%d",
            op->mqtt_conn.cid,
            op->mqtt_conn.host, (int)op->mqtt_conn.port,
            op->mqtt_conn.client_id,
            op->mqtt_conn.user ? op->mqtt_conn.user : "",
            op->mqtt_conn.pass ? op->mqtt_conn.pass : "",
            op->mqtt_conn.tls ? 1 : 0);
        env->reset_timer(env);
        env->state = 1;
        break;
    case 1:
        if (env->contains(env, "+GTMQTTOPEN:")) {
            /* "+GTMQTTOPEN: <cid>,0" → 0 = success */
            const char *r = env->contains(env, ",0");
            if (r) { OP_OK(op, env); }
            else   { OP_ERR(op, env); }
        } else if (env->contains(env, "ERROR") || env->is_timeout(env, 30000)) {
            OP_ERR(op, env);
        }
        break;
    }
    return 0;
}

static int work_mqtt_publish(at_env_t *env)
{
    xy_cell_op_t *op = (xy_cell_op_t *)env->params;
    switch (env->state) {
    case 0:
        /* AT+GTMQTTPUB=<cid>,"<topic>",<qos>,<retain>,<len> */
        env->println(env, "AT+GTMQTTPUB=%d,\"%s\",%d,0,%d",
                     op->mqtt_pub.cid,
                     op->mqtt_pub.topic,
                     (int)op->mqtt_pub.qos,
                     (int)op->mqtt_pub.len);
        env->reset_timer(env);
        env->state = 1;
        break;
    case 1:
        if (env->contains(env, ">")) {
            env->obj->adap->write(op->mqtt_pub.payload,
                                  (unsigned int)op->mqtt_pub.len);
            env->recvclr(env);
            env->reset_timer(env);
            env->state = 2;
        } else if (env->is_timeout(env, 5000)) {
            OP_ERR(op, env);
        }
        break;
    case 2:
        if (env->contains(env, "OK")) {
            OP_OK(op, env);
        } else if (env->contains(env, "ERROR") || env->is_timeout(env, 30000)) {
            OP_ERR(op, env);
        }
        break;
    }
    return 0;
}

static int work_mqtt_subscribe(at_env_t *env)
{
    xy_cell_op_t *op = (xy_cell_op_t *)env->params;
    switch (env->state) {
    case 0:
        env->println(env, "AT+GTMQTTSUB=%d,\"%s\",%d",
                     op->mqtt_sub.cid,
                     op->mqtt_sub.topic,
                     (int)op->mqtt_sub.qos);
        env->reset_timer(env);
        env->state = 1;
        break;
    case 1:
        if (env->contains(env, "+GTMQTTSUB:")) {
            const char *r = env->contains(env, ",0");
            if (r) { OP_OK(op, env); }
            else   { OP_ERR(op, env); }
        } else if (env->contains(env, "ERROR") || env->is_timeout(env, 30000)) {
            OP_ERR(op, env);
        }
        break;
    }
    return 0;
}

static int work_mqtt_disconnect(at_env_t *env)
{
    xy_cell_op_t *op = (xy_cell_op_t *)env->params;
    switch (env->state) {
    case 0:
        env->println(env, "AT+GTMQTTCLOSE=%d", op->mqtt_disc.cid);
        env->reset_timer(env);
        env->state = 1;
        break;
    case 1:
        if (env->contains(env, "+GTMQTTCLOSE:") || env->contains(env, "OK") ||
            env->contains(env, "ERROR") || env->is_timeout(env, 10000)) {
            OP_OK(op, env);
        }
        break;
    }
    return 0;
}

/* ── vtable ───────────────────────────────────────────────────────────── */

const xy_cell_drv_t xy_cell_drv_fibocom = {
    .name             = "L610",
    .pdp_activate     = work_pdp_activate,
    .pdp_deactivate   = work_pdp_deactivate,
    .sock_open        = work_sock_open,
    .sock_send        = work_sock_send,
    .sock_close       = work_sock_close,
    .mqtt_connect     = work_mqtt_connect,
    .mqtt_publish     = work_mqtt_publish,
    .mqtt_subscribe   = work_mqtt_subscribe,
    .mqtt_disconnect  = work_mqtt_disconnect,
    .urc_tbl          = s_urc_tbl,
    .urc_cnt          = (int)(sizeof(s_urc_tbl) / sizeof(s_urc_tbl[0])),
};
