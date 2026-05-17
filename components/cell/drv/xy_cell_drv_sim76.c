#include "../xy_cell_drv.h"
#include "xy_string.h"
#include "xy_stdio.h"

/* ── socket receive URC  "+RECEIVE,<id>,<len>:"  (endmark ':') ───────── */

/* Header parser for "+RECEIVE,<id>,<len>:" */
static int parse_receive_hdr(const char *buf, int len,
                             int *id, int *bytes, int *hdr)
{
    (void)len;
    const char *p = buf;
    while (*p && *p != ',') p++;
    if (*p != ',') return -1;
    p++;
    *id    = (int)xy_strtol(p, (char **)&p, 10);
    if (*p != ',') return -1;
    p++;
    *bytes = (int)xy_strtol(p, (char **)&p, 10);
    *hdr   = (int)(p - buf) + 1;       /* include ':' */
    return 0;
}

static int urc_receive(at_urc_info_t *info)
{
    int id, plen, rc;
    const char *payload;

    rc = at_urc_recv_split(info, parse_receive_hdr, 0, &id, &payload, &plen);
    if (rc > 0) return rc;
    if (rc < 0) return 0;

    if (id >= 0 && id < XY_CELL_SOCK_MAX && g_cell_socks[id].open)
        xy_rb_put(&g_cell_socks[id].rx, (const uint8_t *)payload, (uint32_t)plen);
    return 0;
}

/* "+IPCLOSE: <id>,<cause>"  (endmark '\n') */
static int urc_ipclose(at_urc_info_t *info)
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

/* ── MQTT receive URCs (multi-line accumulation) ─────────────────────── */

static struct {
    int     cid;
    int     topiclen;
    int     payloadlen;
    char    topic[128];
    uint8_t payload[512];
} s_mqrx;

/* "+CMQTTRXSTART: <cid>,<topiclen>,<payloadlen>"  (endmark '\n') */
static int urc_mqttrxstart(at_urc_info_t *info)
{
    const char *p = strchr(info->urcbuf, ':');
    if (!p) return 0;
    p++;
    s_mqrx.cid        = (int)xy_strtol(p, (char **)&p, 10);
    if (*p == ',') p++;
    s_mqrx.topiclen   = (int)xy_strtol(p, (char **)&p, 10);
    if (*p == ',') p++;
    s_mqrx.payloadlen = (int)xy_strtol(p, NULL, 10);
    return 0;
}

/* "+CMQTTTOPIC: <cid>,<len>\n<topic text>"  (endmark '\n') */
static int urc_mqtttopic(at_urc_info_t *info)
{
    const char *p = info->urcbuf;
    int hdr_end = 0, i;
    for (i = 0; i < info->urclen; i++) {
        if (p[i] == '\n') { hdr_end = i + 1; break; }
    }
    if (!hdr_end) return 0;

    if (info->urclen == hdr_end) {
        int need = (s_mqrx.topiclen > 0) ? s_mqrx.topiclen : 1;
        return need + 2; /* topic bytes + CRLF */
    }

    int avail = info->urclen - hdr_end;
    int n = (avail < (int)sizeof(s_mqrx.topic) - 1) ? avail : (int)sizeof(s_mqrx.topic) - 1;
    while (n > 0 && (p[hdr_end + n - 1] == '\r' || p[hdr_end + n - 1] == '\n')) n--;
    strncpy(s_mqrx.topic, p + hdr_end, n);
    s_mqrx.topic[n] = '\0';
    return 0;
}

/* "+CMQTTPAYLOAD: <cid>,<len>\n<payload>"  (endmark '\n') */
static int urc_mqttpayload(at_urc_info_t *info)
{
    const char *p = info->urcbuf;
    int hdr_end = 0, i;
    for (i = 0; i < info->urclen; i++) {
        if (p[i] == '\n') { hdr_end = i + 1; break; }
    }
    if (!hdr_end) return 0;

    if (info->urclen == hdr_end) {
        int need = (s_mqrx.payloadlen > 0) ? s_mqrx.payloadlen : 1;
        return need + 2;
    }

    int avail = info->urclen - hdr_end;
    int n = (avail < (int)sizeof(s_mqrx.payload)) ? avail : (int)sizeof(s_mqrx.payload);
    memcpy(s_mqrx.payload, p + hdr_end, n);
    s_mqrx.payloadlen = n;
    return 0;
}

/* "+CMQTTRXEND: <cid>"  (endmark '\n') */
static int urc_mqttrxend(at_urc_info_t *info)
{
    (void)info;
    xy_cell_mqtt_deliver(s_mqrx.cid,
                         s_mqrx.topic,   (uint16_t)strlen(s_mqrx.topic),
                         s_mqrx.payload, (uint16_t)s_mqrx.payloadlen);
    return 0;
}

static const urc_item_t s_urc_tbl[] = {
    { "+RECEIVE,",      ':',  urc_receive      },
    { "+IPCLOSE:",      '\n', urc_ipclose      },
    { "+CMQTTRXSTART:", '\n', urc_mqttrxstart  },
    { "+CMQTTTOPIC:",   '\n', urc_mqtttopic    },
    { "+CMQTTPAYLOAD:", '\n', urc_mqttpayload  },
    { "+CMQTTRXEND:",   '\n', urc_mqttrxend    },
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
            env->println(env, "AT+IPADDR");
            env->reset_timer(env);
            env->state = 3;
        } else if (env->contains(env, "ERROR") || env->is_timeout(env, 30000)) {
            OP_ERR(op, env);
        }
        break;
    case 3:
        if (env->contains(env, "OK")) {
            cell_parse_ip(env->recvbuf(env), g_cell.ip, (int)sizeof(g_cell.ip));
            g_cell.state = CELL_ST_ONLINE;
            OP_OK(op, env);
        } else if (env->contains(env, "ERROR") || env->is_timeout(env, 5000)) {
            /* Still online even if we can't retrieve IP */
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
            env->println(env, "AT+CSSLCFG=\"sslversion\",%d,3",
                         op->sock_open.id);
            env->reset_timer(env);
            env->state = 1;
        } else {
            env->state = 2; /* skip TLS config */
        }
        break;
    case 1:
        if (env->contains(env, "OK") || env->contains(env, "ERROR") ||
            env->is_timeout(env, 3000)) {
            env->recvclr(env);
            env->state = 2;
        }
        break;
    case 2: {
        const char *proto = op->sock_open.tls ? "SSL" : op->sock_open.proto;
        env->println(env, "AT+CIPOPEN=%d,\"%s\",\"%s\",%d",
                     op->sock_open.id, proto,
                     op->sock_open.host, (int)op->sock_open.port);
        env->reset_timer(env);
        env->state = 3;
        break;
    }
    case 3:
        if (env->contains(env, "OK") || env->contains(env, "+CIPOPEN:")) {
            g_cell_socks[op->sock_open.id].open = true;
            g_cell_socks[op->sock_open.id].tls  = op->sock_open.tls;
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
    if (env->state == 0) {
        env->println(env, "AT+CIPSEND=%d,%d",
                     op->sock_send.id, (int)op->sock_send.len);
        env->reset_timer(env);
        env->state = 1;
        return 0;
    }
    int rc = at_prompt_send_step(env,
                                 op->sock_send.data, op->sock_send.len,
                                 "DATA ACCEPT", "OK", NULL,
                                 5000, 10000);
    if      (rc > 0) OP_OK(op, env);
    else if (rc < 0) OP_ERR(op, env);
    return 0;
}

static int work_sock_close(at_env_t *env)
{
    xy_cell_op_t *op = (xy_cell_op_t *)env->params;
    switch (env->state) {
    case 0:
        env->println(env, "AT+CIPCLOSE=%d", op->sock_close.id);
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

/* ── Module MQTT ─────────────────────────────────────────────────────── */

static int work_mqtt_connect(at_env_t *env)
{
    xy_cell_op_t *op = (xy_cell_op_t *)env->params;
    switch (env->state) {
    case 0:
        env->println(env, "AT+CMQTTACCQ=%d,\"%s\"",
                     op->mqtt_conn.cid, op->mqtt_conn.client_id);
        env->reset_timer(env);
        env->state = 1;
        break;
    case 1:
        if (env->contains(env, "OK")) {
            env->recvclr(env);
            if (op->mqtt_conn.tls) {
                env->println(env,
                    "AT+CMQTTCONNECT=%d,\"ssl://%s:%d\",60,1,\"%s\",\"%s\"",
                    op->mqtt_conn.cid, op->mqtt_conn.host,
                    (int)op->mqtt_conn.port,
                    op->mqtt_conn.user ? op->mqtt_conn.user : "",
                    op->mqtt_conn.pass ? op->mqtt_conn.pass : "");
            } else {
                env->println(env,
                    "AT+CMQTTCONNECT=%d,\"tcp://%s:%d\",60,1,\"%s\",\"%s\"",
                    op->mqtt_conn.cid, op->mqtt_conn.host,
                    (int)op->mqtt_conn.port,
                    op->mqtt_conn.user ? op->mqtt_conn.user : "",
                    op->mqtt_conn.pass ? op->mqtt_conn.pass : "");
            }
            env->reset_timer(env);
            env->state = 2;
        } else if (env->contains(env, "ERROR") || env->is_timeout(env, 5000)) {
            OP_ERR(op, env);
        }
        break;
    case 2:
        if (env->contains(env, "+CMQTTCONNECT:")) {
            /* "+CMQTTCONNECT: <cid>,0" means success */
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
    int           tlen = (int)strlen(op->mqtt_pub.topic);
    switch (env->state) {
    case 0:
        env->println(env, "AT+CMQTTTOPIC=%d,%d", op->mqtt_pub.cid, tlen);
        env->reset_timer(env);
        env->state = 1;
        break;
    case 1:
        if (env->contains(env, ">")) {
            env->obj->adap->write(op->mqtt_pub.topic, (unsigned int)tlen);
            env->recvclr(env);
            env->reset_timer(env);
            env->state = 2;
        } else if (env->is_timeout(env, 5000)) {
            OP_ERR(op, env);
        }
        break;
    case 2:
        if (env->contains(env, "OK")) {
            env->recvclr(env);
            env->println(env, "AT+CMQTTPAYLOAD=%d,%d",
                         op->mqtt_pub.cid, (int)op->mqtt_pub.len);
            env->reset_timer(env);
            env->state = 3;
        } else if (env->contains(env, "ERROR") || env->is_timeout(env, 5000)) {
            OP_ERR(op, env);
        }
        break;
    case 3:
        if (env->contains(env, ">")) {
            env->obj->adap->write(op->mqtt_pub.payload,
                                  (unsigned int)op->mqtt_pub.len);
            env->recvclr(env);
            env->reset_timer(env);
            env->state = 4;
        } else if (env->is_timeout(env, 5000)) {
            OP_ERR(op, env);
        }
        break;
    case 4:
        if (env->contains(env, "OK")) {
            env->recvclr(env);
            env->println(env, "AT+CMQTTPUB=%d,%d,60",
                         op->mqtt_pub.cid, (int)op->mqtt_pub.qos);
            env->reset_timer(env);
            env->state = 5;
        } else if (env->contains(env, "ERROR") || env->is_timeout(env, 5000)) {
            OP_ERR(op, env);
        }
        break;
    case 5:
        if (env->contains(env, "+CMQTTPUB:")) {
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

static int work_mqtt_subscribe(at_env_t *env)
{
    xy_cell_op_t *op = (xy_cell_op_t *)env->params;
    int           tlen = (int)strlen(op->mqtt_sub.topic);
    switch (env->state) {
    case 0:
        env->println(env, "AT+CMQTTSUBTOPIC=%d,%d,%d",
                     op->mqtt_sub.cid, tlen, (int)op->mqtt_sub.qos);
        env->reset_timer(env);
        env->state = 1;
        break;
    case 1:
        if (env->contains(env, ">")) {
            env->obj->adap->write(op->mqtt_sub.topic, (unsigned int)tlen);
            env->recvclr(env);
            env->reset_timer(env);
            env->state = 2;
        } else if (env->is_timeout(env, 5000)) {
            OP_ERR(op, env);
        }
        break;
    case 2:
        if (env->contains(env, "OK")) {
            env->recvclr(env);
            env->println(env, "AT+CMQTTSUB=%d", op->mqtt_sub.cid);
            env->reset_timer(env);
            env->state = 3;
        } else if (env->contains(env, "ERROR") || env->is_timeout(env, 5000)) {
            OP_ERR(op, env);
        }
        break;
    case 3:
        if (env->contains(env, "+CMQTTSUB:")) {
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
        env->println(env, "AT+CMQTTDISC=%d,120", op->mqtt_disc.cid);
        env->reset_timer(env);
        env->state = 1;
        break;
    case 1:
        if (env->contains(env, "OK") || env->contains(env, "+CMQTTDISC:") ||
            env->contains(env, "ERROR") || env->is_timeout(env, 10000)) {
            OP_OK(op, env);
        }
        break;
    }
    return 0;
}

/* ── vtable ───────────────────────────────────────────────────────────── */

const xy_cell_drv_t xy_cell_drv_sim76 = {
    .name             = "SIM76XX",
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
