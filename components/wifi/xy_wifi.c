/* xy_wifi.c — WiFi abstraction for ESP32 (ESP-AT firmware).
 *
 * AT command reference (ESP-AT):
 *   Init    : ATE0 / AT+CWMODE=1 / AT+CIPMUX=1
 *   Connect : AT+CWJAP="ssid","pass"
 *   Get IP  : AT+CIPSTA?
 *   Disconnect: AT+CWQAP
 *   Sock open : AT+CIPSTART=<id>,"TCP"/"UDP","host",port
 *   Sock send : AT+CIPSEND=<id>,<len>  → >  → data  → SEND OK
 *   Sock close: AT+CIPCLOSE=<id>
 *   Recv URC  : +IPD,<id>,<len>:  (binary, endmark ':')
 *   Close URC : <id>,CLOSED        (endmark '\n')
 *   MQTT      : AT+MQTTUSERCFG / AT+MQTTCONN / AT+MQTTPUB / AT+MQTTSUB / AT+MQTTCLEAN
 */

#include "xy_wifi.h"
#include "xy_string.h"
#include "xy_stdio.h"

/* ── globals ─────────────────────────────────────────────────────────── */

static xy_wifi_t g_wifi;

typedef struct {
    volatile bool done;
    at_resp_code  code;
    union {
        struct { const char *ssid; const char *pass; }                  conn;
        struct { int id; const char *proto; const char *host;
                 uint16_t port; }                                        sock_open;
        struct { int id; const void *data; uint16_t len; }             sock_send;
        struct { int id; }                                              sock_close;
        struct { const char *host; uint16_t port;
                 const char *client_id; const char *user;
                 const char *pass; bool tls; }                          mqtt_conn;
        struct { const char *topic; const void *payload;
                 uint16_t len; uint8_t qos; }                           mqtt_pub;
        struct { const char *topic; uint8_t qos; }                      mqtt_sub;
    };
} wifi_op_t;

static wifi_op_t g_op;

typedef struct {
    bool    open;
    uint8_t mem[XY_WIFI_SOCK_RXBUF];
    xy_rb_t rx;
} wifi_sock_t;

static wifi_sock_t g_socks[XY_WIFI_SOCK_MAX];

static xy_wifi_mqtt_recv_cb_t s_mqtt_cb;

/* ── helpers ─────────────────────────────────────────────────────────── */

/* Use the shared AT_OP_OK / AT_OP_ERR from at_chat.h to avoid duplication. */
#define OP_OK(op, env)   AT_OP_OK((op), (env))
#define OP_ERR(op, env)  AT_OP_ERR((op), (env))

static void parse_ip(const char *buf, char *ip, int iplen)
{
    /* Look for ip,"x.x.x.x" pattern from AT+CIPSTA? response */
    const char *p = strstr(buf, "ip,\"");
    if (p) {
        p += 4;
        const char *end = strchr(p, '"');
        if (end && end > p) {
            int n = (int)(end - p);
            if (n < iplen) { strncpy(ip, p, n); ip[n] = '\0'; return; }
        }
    }
    /* Fallback: find dotted-quad */
    p = buf;
    while (*p) {
        if (*p >= '0' && *p <= '9') {
            int dots = 0, i = 0;
            const char *s = p;
            while ((s[i] >= '0' && s[i] <= '9') || s[i] == '.') {
                if (s[i] == '.') dots++;
                i++;
            }
            if (dots == 3 && i >= 7 && i < iplen) {
                strncpy(ip, s, i);
                ip[i] = '\0';
                return;
            }
        }
        p++;
    }
}

/* ── socket receive URC  "+IPD,<id>,<len>:"  (endmark ':') ─────────── */

/* Header parser for "+IPD,<id>,<len>:" */
static int parse_ipd_hdr(const char *buf, int len, int *id, int *bytes, int *hdr)
{
    (void)len;
    const char *p = buf;
    while (*p && *p != ',') p++;       /* skip "+IPD" */
    if (*p != ',') return -1;
    p++;
    *id    = (int)xy_strtol(p, (char **)&p, 10);
    if (*p != ',') return -1;
    p++;
    *bytes = (int)xy_strtol(p, (char **)&p, 10);
    /* p now points at ':' (the endmark, included in hdr_len) */
    *hdr   = (int)(p - buf) + 1;
    return 0;
}

static int urc_ipd(at_urc_info_t *info)
{
    int id, plen, rc;
    const char *payload;

    rc = at_urc_recv_split(info, parse_ipd_hdr, 0, &id, &payload, &plen);
    if (rc > 0) return rc;             /* need more bytes */
    if (rc < 0) return 0;              /* bad header */

    if (id >= 0 && id < XY_WIFI_SOCK_MAX && g_socks[id].open)
        xy_rb_put(&g_socks[id].rx, (const uint8_t *)payload, (uint32_t)plen);
    return 0;
}

/* "<id>,CLOSED"  (endmark '\n') — one entry per socket id */
static int urc_closed(at_urc_info_t *info)
{
    int id = (int)xy_strtol(info->urcbuf, NULL, 10);
    if (id >= 0 && id < XY_WIFI_SOCK_MAX)
        g_socks[id].open = false;
    return 0;
}

/* WIFI event URCs */
static int urc_wifi_got_ip(at_urc_info_t *info)
{
    (void)info;
    g_wifi.state = WIFI_ST_CONNECTED;
    return 0;
}

static int urc_wifi_disconnect(at_urc_info_t *info)
{
    (void)info;
    g_wifi.state = WIFI_ST_INIT;
    memset(g_wifi.ip, 0, sizeof(g_wifi.ip));
    return 0;
}

/* MQTT receive URC  "+MQTTSUBRECV:<linkid>,"<topic>",<len>,<payload>"  (endmark '\n') */
static int urc_mqttsubrecv(at_urc_info_t *info)
{
    if (!s_mqtt_cb) return 0;
    const char *p = strchr(info->urcbuf, ':');
    if (!p) return 0;
    p++;
    xy_strtol(p, (char **)&p, 10); /* linkid — discard */
    if (*p == ',') p++;
    /* topic in quotes */
    if (*p != '"') return 0;
    p++;
    const char *topic = p;
    while (*p && *p != '"') p++;
    int topiclen = (int)(p - topic);
    if (*p == '"') p++;
    if (*p == ',') p++;
    int len = (int)xy_strtol(p, (char **)&p, 10);
    if (*p == ',') p++;
    /* payload follows directly */
    if (len > 0) {
        char t[128];
        int n = (topiclen < (int)sizeof(t) - 1) ? topiclen : (int)sizeof(t) - 1;
        strncpy(t, topic, n);
        t[n] = '\0';
        s_mqtt_cb(t, p, (uint16_t)len);
    }
    return 0;
}

static const urc_item_t s_urc_tbl[] = {
    { "+IPD,",         ':',  urc_ipd            },
    { "0,CLOSED",      '\n', urc_closed         },
    { "1,CLOSED",      '\n', urc_closed         },
    { "2,CLOSED",      '\n', urc_closed         },
    { "3,CLOSED",      '\n', urc_closed         },
    { "WIFI GOT IP",   '\n', urc_wifi_got_ip    },
    { "WIFI DISCONNEC",'\n', urc_wifi_disconnect },
    { "+MQTTSUBRECV:", '\n', urc_mqttsubrecv    },
};

/* ── work handlers ────────────────────────────────────────────────────── */

static int work_init(at_env_t *env)
{
    wifi_op_t *op = (wifi_op_t *)env->params;
    switch (env->state) {
    case 0:
        env->println(env, "ATE0");
        env->reset_timer(env);
        env->state = 1;
        break;
    case 1:
        if (env->contains(env, "OK") || env->is_timeout(env, 3000)) {
            env->recvclr(env);
            env->println(env, "AT+CWMODE=1");
            env->reset_timer(env);
            env->state = 2;
        }
        break;
    case 2:
        if (env->contains(env, "OK")) {
            env->recvclr(env);
            env->println(env, "AT+CIPMUX=1");
            env->reset_timer(env);
            env->state = 3;
        } else if (env->contains(env, "ERROR") || env->is_timeout(env, 3000)) {
            OP_ERR(op, env);
        }
        break;
    case 3:
        if (env->contains(env, "OK")) {
            g_wifi.state = WIFI_ST_INIT;
            OP_OK(op, env);
        } else if (env->contains(env, "ERROR") || env->is_timeout(env, 3000)) {
            OP_ERR(op, env);
        }
        break;
    }
    return 0;
}

static int work_connect(at_env_t *env)
{
    wifi_op_t *op = (wifi_op_t *)env->params;
    switch (env->state) {
    case 0:
        env->println(env, "AT+CWJAP=\"%s\",\"%s\"",
                     op->conn.ssid, op->conn.pass);
        env->reset_timer(env);
        env->state = 1;
        break;
    case 1:
        /* WIFI GOT IP + OK means connected; +CWJAP:n means error */
        if ((env->contains(env, "WIFI GOT IP") || env->contains(env, "OK")) &&
             env->contains(env, "OK")) {
            env->recvclr(env);
            env->println(env, "AT+CIPSTA?");
            env->reset_timer(env);
            env->state = 2;
        } else if (env->contains(env, "+CWJAP:") ||
                   env->contains(env, "FAIL")    ||
                   env->is_timeout(env, 20000)) {
            OP_ERR(op, env);
        }
        break;
    case 2:
        if (env->contains(env, "OK") || env->contains(env, "+CIPSTA:")) {
            parse_ip(env->recvbuf(env), g_wifi.ip, (int)sizeof(g_wifi.ip));
            /* Save SSID */
            strncpy(g_wifi.ssid, op->conn.ssid, sizeof(g_wifi.ssid) - 1);
            g_wifi.ssid[sizeof(g_wifi.ssid) - 1] = '\0';
            g_wifi.state = WIFI_ST_CONNECTED;
            OP_OK(op, env);
        } else if (env->is_timeout(env, 5000)) {
            g_wifi.state = WIFI_ST_CONNECTED;
            OP_OK(op, env);
        }
        break;
    }
    return 0;
}

static int work_disconnect(at_env_t *env)
{
    wifi_op_t *op = (wifi_op_t *)env->params;
    switch (env->state) {
    case 0:
        env->println(env, "AT+CWQAP");
        env->reset_timer(env);
        env->state = 1;
        break;
    case 1:
        if (env->contains(env, "OK") || env->contains(env, "ERROR") ||
            env->is_timeout(env, 5000)) {
            g_wifi.state = WIFI_ST_INIT;
            memset(g_wifi.ip,   0, sizeof(g_wifi.ip));
            memset(g_wifi.ssid, 0, sizeof(g_wifi.ssid));
            OP_OK(op, env);
        }
        break;
    }
    return 0;
}

static int work_sock_open(at_env_t *env)
{
    wifi_op_t *op = (wifi_op_t *)env->params;
    switch (env->state) {
    case 0:
        env->println(env, "AT+CIPSTART=%d,\"%s\",\"%s\",%d",
                     op->sock_open.id, op->sock_open.proto,
                     op->sock_open.host, (int)op->sock_open.port);
        env->reset_timer(env);
        env->state = 1;
        break;
    case 1:
        /* Success: "<id>,CONNECT\r\n\r\nOK" */
        if (env->contains(env, "CONNECT") && env->contains(env, "OK")) {
            g_socks[op->sock_open.id].open = true;
            OP_OK(op, env);
        } else if (env->contains(env, "ERROR") ||
                   env->contains(env, "ALREADY") ||
                   env->is_timeout(env, 30000)) {
            OP_ERR(op, env);
        }
        break;
    }
    return 0;
}

static int work_sock_send(at_env_t *env)
{
    wifi_op_t *op = (wifi_op_t *)env->params;
    switch (env->state) {
    case 0:
        env->println(env, "AT+CIPSEND=%d,%d",
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
        if (env->contains(env, "SEND OK")) {
            OP_OK(op, env);
        } else if (env->contains(env, "ERROR") ||
                   env->contains(env, "SEND FAIL") ||
                   env->is_timeout(env, 10000)) {
            OP_ERR(op, env);
        }
        break;
    }
    return 0;
}

static int work_sock_close(at_env_t *env)
{
    wifi_op_t *op = (wifi_op_t *)env->params;
    switch (env->state) {
    case 0:
        env->println(env, "AT+CIPCLOSE=%d", op->sock_close.id);
        env->reset_timer(env);
        env->state = 1;
        break;
    case 1:
        if (env->contains(env, "OK") || env->contains(env, "ERROR") ||
            env->is_timeout(env, 5000)) {
            g_socks[op->sock_close.id].open = false;
            OP_OK(op, env);
        }
        break;
    }
    return 0;
}

/* ── Module MQTT (ESP-AT AT+MQTT* commands) ──────────────────────────── */

static int work_mqtt_connect(at_env_t *env)
{
    wifi_op_t *op = (wifi_op_t *)env->params;
    switch (env->state) {
    case 0:
        /* AT+MQTTUSERCFG=<linkid>,<scheme>,"<client_id>","<user>","<pass>",0,0,"" */
        /* scheme: 1=TCP, 4=TLS */
        env->println(env,
            "AT+MQTTUSERCFG=0,%d,\"%s\",\"%s\",\"%s\",0,0,\"\"",
            op->mqtt_conn.tls ? 4 : 1,
            op->mqtt_conn.client_id,
            op->mqtt_conn.user ? op->mqtt_conn.user : "",
            op->mqtt_conn.pass ? op->mqtt_conn.pass : "");
        env->reset_timer(env);
        env->state = 1;
        break;
    case 1:
        if (env->contains(env, "OK")) {
            env->recvclr(env);
            env->println(env, "AT+MQTTCONN=0,\"%s\",%d,1",
                         op->mqtt_conn.host, (int)op->mqtt_conn.port);
            env->reset_timer(env);
            env->state = 2;
        } else if (env->contains(env, "ERROR") || env->is_timeout(env, 5000)) {
            OP_ERR(op, env);
        }
        break;
    case 2:
        if (env->contains(env, "+MQTTCONNECTED:") || env->contains(env, "OK")) {
            OP_OK(op, env);
        } else if (env->contains(env, "ERROR") ||
                   env->contains(env, "+MQTTDISCONNECTED:") ||
                   env->is_timeout(env, 30000)) {
            OP_ERR(op, env);
        }
        break;
    }
    return 0;
}

static int work_mqtt_disconnect(at_env_t *env)
{
    wifi_op_t *op = (wifi_op_t *)env->params;
    switch (env->state) {
    case 0:
        env->println(env, "AT+MQTTCLEAN=0");
        env->reset_timer(env);
        env->state = 1;
        break;
    case 1:
        if (env->contains(env, "OK") || env->contains(env, "ERROR") ||
            env->is_timeout(env, 5000)) {
            OP_OK(op, env);
        }
        break;
    }
    return 0;
}

static int work_mqtt_publish(at_env_t *env)
{
    wifi_op_t *op = (wifi_op_t *)env->params;
    switch (env->state) {
    case 0:
        /* AT+MQTTPUB=<linkid>,"<topic>","<payload>",<qos>,<retain> */
        env->println(env, "AT+MQTTPUBRAW=0,\"%s\",%d,%d,0",
                     op->mqtt_pub.topic,
                     (int)op->mqtt_pub.len,
                     (int)op->mqtt_pub.qos);
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
    wifi_op_t *op = (wifi_op_t *)env->params;
    switch (env->state) {
    case 0:
        env->println(env, "AT+MQTTSUB=0,\"%s\",%d",
                     op->mqtt_sub.topic, (int)op->mqtt_sub.qos);
        env->reset_timer(env);
        env->state = 1;
        break;
    case 1:
        if (env->contains(env, "OK")) {
            OP_OK(op, env);
        } else if (env->contains(env, "ERROR") || env->is_timeout(env, 10000)) {
            OP_ERR(op, env);
        }
        break;
    }
    return 0;
}

/* ── init ─────────────────────────────────────────────────────────────── */

void xy_wifi_init(at_obj_t *at)
{
    int i;
    memset(&g_wifi, 0, sizeof(g_wifi));
    memset(&g_op,   0, sizeof(g_op));
    memset(g_socks, 0, sizeof(g_socks));

    g_wifi.at = at;

    for (i = 0; i < XY_WIFI_SOCK_MAX; i++)
        xy_rb_init(&g_socks[i].rx, g_socks[i].mem, XY_WIFI_SOCK_RXBUF);

    at_obj_set_urc(at, s_urc_tbl,
                   (int)(sizeof(s_urc_tbl) / sizeof(s_urc_tbl[0])));
}

/* ── public API ───────────────────────────────────────────────────────── */

xy_wifi_state_t  xy_wifi_get_state(void) { return g_wifi.state; }
const xy_wifi_t *xy_wifi_get_info(void)  { return &g_wifi; }

bool xy_wifi_op_done(void) { return (bool)g_op.done; }
bool xy_wifi_op_ok(void)   { return g_op.done && (g_op.code == AT_RESP_OK); }

static bool start_op(at_work_t work)
{
    g_op.done = false;
    g_op.code = AT_RESP_ERROR;
    return at_do_work(g_wifi.at, &g_op, work);
}

bool xy_wifi_start_init(void) { return start_op(work_init); }

bool xy_wifi_start_connect(const char *ssid, const char *pass)
{
    g_op.conn.ssid = ssid;
    g_op.conn.pass = pass;
    return start_op(work_connect);
}

bool xy_wifi_start_disconnect(void) { return start_op(work_disconnect); }

bool xy_wifi_start_sock_open(int id, const char *proto,
                              const char *host, uint16_t port)
{
    if (id < 0 || id >= XY_WIFI_SOCK_MAX) return false;
    g_op.sock_open.id    = id;
    g_op.sock_open.proto = proto;
    g_op.sock_open.host  = host;
    g_op.sock_open.port  = port;
    return start_op(work_sock_open);
}

bool xy_wifi_start_sock_send(int id, const void *data, uint16_t len)
{
    if (id < 0 || id >= XY_WIFI_SOCK_MAX) return false;
    g_op.sock_send.id   = id;
    g_op.sock_send.data = data;
    g_op.sock_send.len  = len;
    return start_op(work_sock_send);
}

int xy_wifi_sock_recv(int id, void *buf, uint16_t maxlen)
{
    if (id < 0 || id >= XY_WIFI_SOCK_MAX) return -1;
    return (int)xy_rb_get(&g_socks[id].rx, (uint8_t *)buf, maxlen);
}

void xy_wifi_start_sock_close(int id)
{
    if (id < 0 || id >= XY_WIFI_SOCK_MAX) return;
    g_op.sock_close.id = id;
    start_op(work_sock_close);
}

bool xy_wifi_sock_is_open(int id)
{
    if (id < 0 || id >= XY_WIFI_SOCK_MAX) return false;
    return g_socks[id].open;
}

bool xy_wifi_start_mqtt_connect(const char *host, uint16_t port,
                                 const char *client_id,
                                 const char *user, const char *pass,
                                 bool tls)
{
    g_op.mqtt_conn.host      = host;
    g_op.mqtt_conn.port      = port;
    g_op.mqtt_conn.client_id = client_id;
    g_op.mqtt_conn.user      = user;
    g_op.mqtt_conn.pass      = pass;
    g_op.mqtt_conn.tls       = tls;
    return start_op(work_mqtt_connect);
}

bool xy_wifi_start_mqtt_disconnect(void) { return start_op(work_mqtt_disconnect); }

bool xy_wifi_start_mqtt_publish(const char *topic,
                                 const void *payload, uint16_t len,
                                 uint8_t qos)
{
    g_op.mqtt_pub.topic   = topic;
    g_op.mqtt_pub.payload = payload;
    g_op.mqtt_pub.len     = len;
    g_op.mqtt_pub.qos     = qos;
    return start_op(work_mqtt_publish);
}

bool xy_wifi_start_mqtt_subscribe(const char *topic, uint8_t qos)
{
    g_op.mqtt_sub.topic = topic;
    g_op.mqtt_sub.qos   = qos;
    return start_op(work_mqtt_subscribe);
}

void xy_wifi_mqtt_set_recv_cb(xy_wifi_mqtt_recv_cb_t cb) { s_mqtt_cb = cb; }
