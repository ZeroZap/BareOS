#include "../xy_cell_drv.h"
#include "xy_string.h"
#include "xy_stdio.h"

/* ── socket receive URC
 *  "+QIURC: \"recv\",<id>,<len>\n<data>"  (endmark '\n')
 * ─────────────────────────────────────────────────────────────────────── */

/* Header parser: "+QIURC: \"recv\",<id>,<len>\n" */
static int parse_qirecv_hdr(const char *buf, int len,
                            int *id, int *bytes, int *hdr)
{
    int i;
    for (i = 0; i < len; i++) {
        if (buf[i] == '\n') { *hdr = i + 1; break; }
    }
    if (i >= len) return -1;

    const char *q = strstr(buf, "\"recv\",");
    if (!q || (q - buf) > *hdr) return -1;
    q += 7;
    *id    = (int)xy_strtol(q, (char **)&q, 10);
    if (*q != ',') return -1;
    q++;
    *bytes = (int)xy_strtol(q, NULL, 10);
    return 0;
}

static int urc_qirecv(at_urc_info_t *info)
{
    int id, plen, rc;
    const char *payload;

    /* Quectel appends CRLF after binary payload — request 2 trail bytes. */
    rc = at_urc_recv_split(info, parse_qirecv_hdr, 2, &id, &payload, &plen);
    if (rc > 0) return rc;
    if (rc < 0) return 0;

    if (id >= 0 && id < XY_CELL_SOCK_MAX && g_cell_socks[id].open)
        xy_rb_put(&g_cell_socks[id].rx, (const uint8_t *)payload, (uint32_t)plen);
    return 0;
}

/* "+QIURC: \"closed\",<id>"  (endmark '\n') */
static int urc_qiclosed(at_urc_info_t *info)
{
    const char *q = strstr(info->urcbuf, "\"closed\",");
    if (!q) return 0;
    q += 9;
    int id = (int)xy_strtol(q, NULL, 10);
    if (id >= 0 && id < XY_CELL_SOCK_MAX)
        g_cell_socks[id].open = false;
    return 0;
}

/* ── MQTT receive URC
 *  "+QMTRECV: <cid>,<msgid>,\"<topic>\",\"<payload>\""  (endmark '\n')
 * ─────────────────────────────────────────────────────────────────────── */

static int urc_qmtrecv(at_urc_info_t *info)
{
    const char *p = strchr(info->urcbuf, ':');
    if (!p) return 0;
    p++;
    int cid = (int)xy_strtol(p, (char **)&p, 10); /* cid */
    if (*p == ',') p++;
    xy_strtol(p, (char **)&p, 10);                /* msgid (discard) */
    if (*p == ',') p++;

    /* topic: "<topic>" */
    if (*p != '"') return 0;
    p++;
    const char *topic = p;
    while (*p && *p != '"') p++;
    int topiclen = (int)(p - topic);
    if (*p == '"') p++;
    if (*p == ',') p++;

    /* payload: "<payload>" */
    if (*p != '"') return 0;
    p++;
    const char *payload = p;
    while (*p && *p != '"') p++;
    int payloadlen = (int)(p - payload);

    xy_cell_mqtt_deliver(cid, topic, (uint16_t)topiclen,
                         payload, (uint16_t)payloadlen);
    return 0;
}

static const urc_item_t s_urc_tbl[] = {
    { "+QIURC: \"recv\"",   '\n', urc_qirecv   },
    { "+QIURC: \"closed\"", '\n', urc_qiclosed },
    { "+QMTRECV:",          '\n', urc_qmtrecv  },
};

/* ── PDP ─────────────────────────────────────────────────────────────── */

static int work_pdp_activate(at_env_t *env)
{
    xy_cell_op_t *op = (xy_cell_op_t *)env->params;
    switch (env->state) {
    case 0:
        env->println(env, "AT+QICSGP=1,1,\"%s\",\"\",\"\",1", op->pdp.apn);
        env->reset_timer(env);
        env->state = 1;
        break;
    case 1:
        if (env->contains(env, "OK")) {
            env->recvclr(env);
            env->println(env, "AT+QIACT=1");
            env->reset_timer(env);
            env->state = 2;
        } else if (env->contains(env, "ERROR") || env->is_timeout(env, 5000)) {
            OP_ERR(op, env);
        }
        break;
    case 2:
        if (env->contains(env, "OK")) {
            env->recvclr(env);
            env->println(env, "AT+QILOCIP");
            env->reset_timer(env);
            env->state = 3;
        } else if (env->contains(env, "ERROR") || env->is_timeout(env, 150000)) {
            OP_ERR(op, env);
        }
        break;
    case 3:
        if (env->contains(env, "OK") || env->contains(env, "\r\n")) {
            cell_parse_ip(env->recvbuf(env), g_cell.ip, (int)sizeof(g_cell.ip));
            g_cell.state = CELL_ST_ONLINE;
            OP_OK(op, env);
        } else if (env->is_timeout(env, 5000)) {
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
        env->println(env, "AT+QIDEACT=1");
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
            /* Configure TLS version on matching ssl_ctx (use id as ssl_id) */
            env->println(env, "AT+QSSLCFG=\"sslversion\",%d,4",
                         op->sock_open.id);
            env->reset_timer(env);
            env->state = 1;
        } else {
            env->state = 2;
        }
        break;
    case 1:
        if (env->contains(env, "OK") || env->contains(env, "ERROR") ||
            env->is_timeout(env, 3000)) {
            env->recvclr(env);
            env->state = 2;
        }
        break;
    case 2:
        if (op->sock_open.tls) {
            env->println(env,
                "AT+QSSLOPEN=0,%d,%d,\"%s\",%d,2",
                op->sock_open.id, op->sock_open.id,
                op->sock_open.host, (int)op->sock_open.port);
        } else {
            env->println(env,
                "AT+QIOPEN=0,%d,\"%s\",\"%s\",%d,0,1",
                op->sock_open.id, op->sock_open.proto,
                op->sock_open.host, (int)op->sock_open.port);
        }
        env->reset_timer(env);
        env->state = 3;
        break;
    case 3:
        if (env->contains(env, "+QIOPEN:") || env->contains(env, "+QSSLOPEN:")) {
            /* "+QIOPEN: <id>,0" or "+QSSLOPEN: <ssl_id>,<id>,0" → 0 = success */
            const char *r = env->contains(env, ",0");
            if (r) {
                g_cell_socks[op->sock_open.id].open = true;
                g_cell_socks[op->sock_open.id].tls  = op->sock_open.tls;
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
    if (env->state == 0) {
        const char *cmd = (op->sock_send.id < XY_CELL_SOCK_MAX &&
                           g_cell_socks[op->sock_send.id].tls)
                          ? "AT+QSSLSEND" : "AT+QISEND";
        env->println(env, "%s=%d,%d", cmd,
                     op->sock_send.id, (int)op->sock_send.len);
        env->reset_timer(env);
        env->state = 1;
        return 0;
    }
    int rc = at_prompt_send_step(env,
                                 op->sock_send.data, op->sock_send.len,
                                 "SEND OK", "OK", NULL,
                                 5000, 10000);
    if      (rc > 0) OP_OK(op, env);
    else if (rc < 0) OP_ERR(op, env);
    return 0;
}

static int work_sock_close(at_env_t *env)
{
    xy_cell_op_t *op = (xy_cell_op_t *)env->params;
    const char   *cmd = g_cell_socks[op->sock_close.id].tls
                        ? "AT+QSSLCLOSE" : "AT+QICLOSE";
    switch (env->state) {
    case 0:
        env->println(env, "%s=%d", cmd, op->sock_close.id);
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
        if (op->mqtt_conn.tls) {
            env->println(env, "AT+QMTSSLCFG=%d,%d",
                         op->mqtt_conn.cid, op->mqtt_conn.cid);
            env->reset_timer(env);
            env->state = 1;
        } else {
            env->state = 2;
        }
        break;
    case 1:
        if (env->contains(env, "OK") || env->contains(env, "ERROR") ||
            env->is_timeout(env, 3000)) {
            env->recvclr(env);
            env->state = 2;
        }
        break;
    case 2:
        env->println(env, "AT+QMTOPEN=%d,\"%s\",%d",
                     op->mqtt_conn.cid, op->mqtt_conn.host,
                     (int)op->mqtt_conn.port);
        env->reset_timer(env);
        env->state = 3;
        break;
    case 3:
        if (env->contains(env, "+QMTOPEN:")) {
            const char *r = env->contains(env, ",0");
            if (!r) { OP_ERR(op, env); break; }
            env->recvclr(env);
            env->println(env, "AT+QMTCONN=%d,\"%s\",\"%s\",\"%s\"",
                         op->mqtt_conn.cid,
                         op->mqtt_conn.client_id,
                         op->mqtt_conn.user ? op->mqtt_conn.user : "",
                         op->mqtt_conn.pass ? op->mqtt_conn.pass : "");
            env->reset_timer(env);
            env->state = 4;
        } else if (env->contains(env, "ERROR") || env->is_timeout(env, 30000)) {
            OP_ERR(op, env);
        }
        break;
    case 4:
        if (env->contains(env, "+QMTCONN:")) {
            const char *r = env->contains(env, ",0,0");
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
        env->println(env, "AT+QMTPUB=%d,0,%d,0,\"%s\",%d",
                     op->mqtt_pub.cid, (int)op->mqtt_pub.qos,
                     op->mqtt_pub.topic, (int)op->mqtt_pub.len);
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
        if (env->contains(env, "SEND OK") || env->contains(env, "+QMTPUB:")) {
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
        env->println(env, "AT+QMTSUB=%d,1,\"%s\",%d",
                     op->mqtt_sub.cid, op->mqtt_sub.topic,
                     (int)op->mqtt_sub.qos);
        env->reset_timer(env);
        env->state = 1;
        break;
    case 1:
        if (env->contains(env, "+QMTSUB:")) {
            const char *r = env->contains(env, ",0,0");
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
        env->println(env, "AT+QMTDISC=%d", op->mqtt_disc.cid);
        env->reset_timer(env);
        env->state = 1;
        break;
    case 1:
        if (env->contains(env, "OK") || env->contains(env, "+QMTDISC:") ||
            env->contains(env, "ERROR") || env->is_timeout(env, 10000)) {
            OP_OK(op, env);
        }
        break;
    }
    return 0;
}

/* ── vtable ───────────────────────────────────────────────────────────── */

const xy_cell_drv_t xy_cell_drv_ec2x = {
    .name             = "EC2X",
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
