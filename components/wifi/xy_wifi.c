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
        struct { bool enable; }                                         dhcp;
        struct { const char *ip; const char *gateway;
                 const char *netmask; }                                static_ip;
        struct { const char *dns1; const char *dns2; }                  dns;
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
static xy_wifi_ap_t s_scan_results[XY_WIFI_SCAN_MAX];
static int s_scan_count;

static xy_wifi_mqtt_recv_cb_t s_mqtt_cb;

/* ── helpers ─────────────────────────────────────────────────────────── */

/* Use the shared AT_OP_OK / AT_OP_ERR from at_chat.h to avoid duplication. */
#define OP_OK(op, env)   AT_OP_OK((op), (env))
#define OP_ERR(op, env)  AT_OP_ERR((op), (env))

static void parse_ip(const char *buf, char *ip, int iplen);

static void close_all_sockets(void)
{
    int i;

    for (i = 0; i < XY_WIFI_SOCK_MAX; i++)
        g_socks[i].open = false;
}

static void set_error(xy_wifi_err_t err)
{
    g_wifi.last_error = err;
}

static void op_error(wifi_op_t *op, at_env_t *env, xy_wifi_err_t err)
{
    set_error(err);
    OP_ERR(op, env);
}

static void op_ok(wifi_op_t *op, at_env_t *env)
{
    set_error(WIFI_ERR_OK);
    OP_OK(op, env);
}

static bool valid_ipv4(const char *ip)
{
    int dots = 0;
    int digits = 0;
    int value = 0;

    if (ip == NULL || ip[0] == '\0') return false;
    while (*ip) {
        if (*ip >= '0' && *ip <= '9') {
            value = value * 10 + (*ip - '0');
            digits++;
            if (digits > 3 || value > 255) return false;
        } else if (*ip == '.') {
            if (digits == 0) return false;
            dots++;
            digits = 0;
            value = 0;
        } else {
            return false;
        }
        ip++;
    }
    return dots == 3 && digits != 0;
}

static void copy_token(char *dst, int dstlen, const char *src, int len)
{
    if (dst == NULL || dstlen <= 0) return;
    if (src == NULL || len <= 0) {
        dst[0] = '\0';
        return;
    }
    if (len >= dstlen) len = dstlen - 1;
    memcpy(dst, src, (size_t)len);
    dst[len] = '\0';
}

static bool parse_quoted_after(const char *buf, const char *key,
                               char *dst, int dstlen)
{
    const char *p = strstr(buf, key);
    const char *end;

    if (p == NULL) return false;
    p += strlen(key);
    end = strchr(p, '"');
    if (end == NULL || end < p) return false;
    copy_token(dst, dstlen, p, (int)(end - p));
    return true;
}

static void parse_ip_info(const char *buf)
{
    (void)parse_quoted_after(buf, "ip,\"", g_wifi.ip, (int)sizeof(g_wifi.ip));
    (void)parse_quoted_after(buf, "gateway,\"", g_wifi.gateway,
                             (int)sizeof(g_wifi.gateway));
    (void)parse_quoted_after(buf, "netmask,\"", g_wifi.netmask,
                             (int)sizeof(g_wifi.netmask));
    if (g_wifi.ip[0] == '\0')
        parse_ip(buf, g_wifi.ip, (int)sizeof(g_wifi.ip));
}

static xy_wifi_err_t parse_cwjap_error(const char *buf)
{
    const char *p = strstr(buf, "+CWJAP:");
    int code;

    if (p == NULL) return WIFI_ERR_CONNECT_FAIL;
    p += 7;
    code = (int)xy_strtol(p, NULL, 10);
    if (code == 1 || code == 2) return WIFI_ERR_TIMEOUT;
    if (code == 3) return WIFI_ERR_NO_AP;
    if (code == 4) return WIFI_ERR_AUTH;
    return WIFI_ERR_CONNECT_FAIL;
}

static const char *find_line(const char *buf, const char *prefix)
{
    const char *p = buf;
    size_t prefix_len = strlen(prefix);

    while ((p = strstr(p, prefix)) != NULL) {
        if (p == buf || p[-1] == '\n' || p[-1] == '\r') return p;
        p += prefix_len;
    }
    return NULL;
}

static bool parse_cwjap_info(const char *buf)
{
    const char *p = find_line(buf, "+CWJAP:");
    const char *end;

    if (p == NULL) return false;
    p = strchr(p, '"');
    if (p == NULL) return false;
    p++;
    end = strchr(p, '"');
    if (end == NULL) return false;
    copy_token(g_wifi.ssid, (int)sizeof(g_wifi.ssid), p, (int)(end - p));
    p = strchr(end + 1, '"');
    if (p != NULL) {
        p++;
        end = strchr(p, '"');
        if (end != NULL)
            copy_token(g_wifi.bssid, (int)sizeof(g_wifi.bssid), p, (int)(end - p));
    }
    p = strrchr(buf, ',');
    if (p != NULL) g_wifi.rssi_dbm = (int)xy_strtol(p + 1, NULL, 10);
    return true;
}

static bool parse_mac_info(const char *buf)
{
    return parse_quoted_after(buf, "+CIPSTAMAC:\"", g_wifi.mac,
                              (int)sizeof(g_wifi.mac));
}

static xy_wifi_auth_t map_auth(int ecn)
{
    switch (ecn) {
    case 0: return WIFI_AUTH_OPEN;
    case 1: return WIFI_AUTH_WEP;
    case 2: return WIFI_AUTH_WPA_PSK;
    case 3: return WIFI_AUTH_WPA2_PSK;
    case 4: return WIFI_AUTH_WPA_WPA2_PSK;
    case 5: return WIFI_AUTH_WPA2_ENTERPRISE;
    case 6: return WIFI_AUTH_WPA3_PSK;
    case 7: return WIFI_AUTH_WPA2_WPA3_PSK;
    default: return WIFI_AUTH_UNKNOWN;
    }
}

static void parse_scan_results(const char *buf)
{
    const char *p = buf;

    s_scan_count = 0;
    while (s_scan_count < XY_WIFI_SCAN_MAX &&
           (p = strstr(p, "+CWLAP:(")) != NULL) {
        xy_wifi_ap_t *ap = &s_scan_results[s_scan_count];
        const char *q;
        const char *end;
        int ecn;

        memset(ap, 0, sizeof(*ap));
        p += 8;
        ecn = (int)xy_strtol(p, (char **)&p, 10);
        ap->auth = map_auth(ecn);
        q = strchr(p, '"');
        if (q == NULL) break;
        q++;
        end = strchr(q, '"');
        if (end == NULL) break;
        copy_token(ap->ssid, (int)sizeof(ap->ssid), q, (int)(end - q));
        p = end + 1;
        if (*p == ',') p++;
        ap->rssi_dbm = (int)xy_strtol(p, (char **)&p, 10);
        q = strchr(p, '"');
        if (q != NULL) {
            q++;
            end = strchr(q, '"');
            if (end != NULL) {
                copy_token(ap->bssid, (int)sizeof(ap->bssid), q, (int)(end - q));
                p = end + 1;
            }
        }
        if (*p == ',') p++;
        ap->channel = (int)xy_strtol(p, (char **)&p, 10);
        s_scan_count++;
    }
}

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
    const char *p = buf;
    const char *end = buf + len;
    unsigned int value;

    while (p < end && (*p == '\r' || *p == '\n')) p++;
    if (end - p < 8 || memcmp(p, "+IPD,", 5) != 0) return -1;
    p += 5;
    if (p >= end || *p < '0' || *p > '9') return -1;
    value = 0u;
    do {
        if (value > 214748364u) return -1;
        value = value * 10u + (unsigned int)(*p++ - '0');
    } while (p < end && *p >= '0' && *p <= '9');
    if (p >= end || *p != ',' || value > 2147483647u) return -1;
    *id = (int)value;
    p++;
    if (p >= end || *p < '0' || *p > '9') return -1;
    value = 0u;
    do {
        if (value > 214748364u) return -1;
        value = value * 10u + (unsigned int)(*p++ - '0');
    } while (p < end && *p >= '0' && *p <= '9');
    if (p >= end || *p != ':' || value == 0u || value > 2147483647u)
        return -1;
    *bytes = (int)value;
    p++;
    *hdr = (int)(p - buf);
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
    close_all_sockets();
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
            op_error(op, env, env->is_timeout(env, 3000) ? WIFI_ERR_TIMEOUT : WIFI_ERR_AT_ERROR);
        }
        break;
    case 3:
        if (env->contains(env, "OK")) {
            g_wifi.state = WIFI_ST_INIT;
            op_ok(op, env);
        } else if (env->contains(env, "ERROR") || env->is_timeout(env, 3000)) {
            op_error(op, env, env->is_timeout(env, 3000) ? WIFI_ERR_TIMEOUT : WIFI_ERR_AT_ERROR);
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
            op_error(op, env, env->contains(env, "+CWJAP:") ?
                     parse_cwjap_error(env->recvbuf(env)) :
                     (env->is_timeout(env, 20000) ? WIFI_ERR_TIMEOUT : WIFI_ERR_CONNECT_FAIL));
        }
        break;
    case 2:
        if (env->contains(env, "OK") || env->contains(env, "+CIPSTA:")) {
            parse_ip_info(env->recvbuf(env));
            /* Save SSID */
            strncpy(g_wifi.ssid, op->conn.ssid, sizeof(g_wifi.ssid) - 1);
            g_wifi.ssid[sizeof(g_wifi.ssid) - 1] = '\0';
            g_wifi.state = WIFI_ST_CONNECTED;
            op_ok(op, env);
        } else if (env->is_timeout(env, 5000)) {
            g_wifi.state = WIFI_ST_CONNECTED;
            op_ok(op, env);
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
            close_all_sockets();
            op_ok(op, env);
        }
        break;
    }
    return 0;
}

static int work_query_state(at_env_t *env)
{
    wifi_op_t *op = (wifi_op_t *)env->params;
    switch (env->state) {
    case 0:
        env->println(env, "AT+CWSTATE?");
        env->reset_timer(env);
        env->state = 1;
        break;
    case 1:
        if (env->contains(env, "OK")) {
            const char *p = strstr(env->recvbuf(env), "+CWSTATE:");
            int state = p ? (int)xy_strtol(p + 9, NULL, 10) : 0;

            if (state == 3) g_wifi.state = WIFI_ST_CONNECTED;
            else if (state == 0) g_wifi.state = WIFI_ST_INIT;
            (void)parse_quoted_after(env->recvbuf(env), ",\"",
                                     g_wifi.ssid, (int)sizeof(g_wifi.ssid));
            op_ok(op, env);
        } else if (env->contains(env, "ERROR") || env->is_timeout(env, 5000)) {
            op_error(op, env, env->is_timeout(env, 5000) ? WIFI_ERR_TIMEOUT : WIFI_ERR_AT_ERROR);
        }
        break;
    }
    return 0;
}

static int work_query_ip(at_env_t *env)
{
    wifi_op_t *op = (wifi_op_t *)env->params;
    switch (env->state) {
    case 0:
        env->println(env, "AT+CIPSTA?");
        env->reset_timer(env);
        env->state = 1;
        break;
    case 1:
        if (env->contains(env, "OK")) {
            parse_ip_info(env->recvbuf(env));
            if (g_wifi.ip[0] != '\0') g_wifi.state = WIFI_ST_CONNECTED;
            op_ok(op, env);
        } else if (env->contains(env, "ERROR") || env->is_timeout(env, 5000)) {
            op_error(op, env, env->is_timeout(env, 5000) ? WIFI_ERR_TIMEOUT : WIFI_ERR_AT_ERROR);
        }
        break;
    }
    return 0;
}

static int work_query_mac(at_env_t *env)
{
    wifi_op_t *op = (wifi_op_t *)env->params;
    switch (env->state) {
    case 0:
        env->println(env, "AT+CIPSTAMAC?");
        env->reset_timer(env);
        env->state = 1;
        break;
    case 1:
        if (env->contains(env, "OK")) {
            (void)parse_mac_info(env->recvbuf(env));
            op_ok(op, env);
        } else if (env->contains(env, "ERROR") || env->is_timeout(env, 5000)) {
            op_error(op, env, env->is_timeout(env, 5000) ? WIFI_ERR_TIMEOUT : WIFI_ERR_AT_ERROR);
        }
        break;
    }
    return 0;
}

static int work_query_rssi(at_env_t *env)
{
    wifi_op_t *op = (wifi_op_t *)env->params;
    switch (env->state) {
    case 0:
        env->println(env, "AT+CWJAP?");
        env->reset_timer(env);
        env->state = 1;
        break;
    case 1:
        if (env->contains(env, "OK")) {
            if (parse_cwjap_info(env->recvbuf(env))) g_wifi.state = WIFI_ST_CONNECTED;
            op_ok(op, env);
        } else if (env->contains(env, "No AP") || env->contains(env, "ERROR") ||
                   env->is_timeout(env, 5000)) {
            op_error(op, env, env->contains(env, "No AP") ? WIFI_ERR_NO_AP :
                     (env->is_timeout(env, 5000) ? WIFI_ERR_TIMEOUT : WIFI_ERR_AT_ERROR));
        }
        break;
    }
    return 0;
}

static int work_scan(at_env_t *env)
{
    wifi_op_t *op = (wifi_op_t *)env->params;
    switch (env->state) {
    case 0:
        s_scan_count = 0;
        env->println(env, "AT+CWLAP");
        env->reset_timer(env);
        env->state = 1;
        break;
    case 1:
        if (env->contains(env, "OK")) {
            parse_scan_results(env->recvbuf(env));
            op_ok(op, env);
        } else if (env->contains(env, "ERROR") || env->is_timeout(env, 15000)) {
            op_error(op, env, env->is_timeout(env, 15000) ? WIFI_ERR_TIMEOUT : WIFI_ERR_AT_ERROR);
        }
        break;
    }
    return 0;
}

static int work_set_dhcp(at_env_t *env)
{
    wifi_op_t *op = (wifi_op_t *)env->params;
    switch (env->state) {
    case 0:
        env->println(env, "AT+CWDHCP=1,%d", op->dhcp.enable ? 1 : 0);
        env->reset_timer(env);
        env->state = 1;
        break;
    case 1:
        if (env->contains(env, "OK")) {
            op_ok(op, env);
        } else if (env->contains(env, "ERROR") || env->is_timeout(env, 5000)) {
            op_error(op, env, env->is_timeout(env, 5000) ? WIFI_ERR_TIMEOUT : WIFI_ERR_AT_ERROR);
        }
        break;
    }
    return 0;
}

static int work_set_static_ip(at_env_t *env)
{
    wifi_op_t *op = (wifi_op_t *)env->params;
    switch (env->state) {
    case 0:
        env->println(env, "AT+CIPSTA=\"%s\",\"%s\",\"%s\"",
                     op->static_ip.ip, op->static_ip.gateway,
                     op->static_ip.netmask);
        env->reset_timer(env);
        env->state = 1;
        break;
    case 1:
        if (env->contains(env, "OK")) {
            copy_token(g_wifi.ip, (int)sizeof(g_wifi.ip), op->static_ip.ip,
                       (int)strlen(op->static_ip.ip));
            copy_token(g_wifi.gateway, (int)sizeof(g_wifi.gateway),
                       op->static_ip.gateway, (int)strlen(op->static_ip.gateway));
            copy_token(g_wifi.netmask, (int)sizeof(g_wifi.netmask),
                       op->static_ip.netmask, (int)strlen(op->static_ip.netmask));
            op_ok(op, env);
        } else if (env->contains(env, "ERROR") || env->is_timeout(env, 5000)) {
            op_error(op, env, env->is_timeout(env, 5000) ? WIFI_ERR_TIMEOUT : WIFI_ERR_AT_ERROR);
        }
        break;
    }
    return 0;
}

static int work_set_dns(at_env_t *env)
{
    wifi_op_t *op = (wifi_op_t *)env->params;
    switch (env->state) {
    case 0:
        if (op->dns.dns2 != NULL && op->dns.dns2[0] != '\0')
            env->println(env, "AT+CIPDNS=1,\"%s\",\"%s\"",
                         op->dns.dns1, op->dns.dns2);
        else
            env->println(env, "AT+CIPDNS=1,\"%s\"", op->dns.dns1);
        env->reset_timer(env);
        env->state = 1;
        break;
    case 1:
        if (env->contains(env, "OK")) {
            copy_token(g_wifi.dns1, (int)sizeof(g_wifi.dns1), op->dns.dns1,
                       (int)strlen(op->dns.dns1));
            if (op->dns.dns2 != NULL)
                copy_token(g_wifi.dns2, (int)sizeof(g_wifi.dns2), op->dns.dns2,
                           (int)strlen(op->dns.dns2));
            op_ok(op, env);
        } else if (env->contains(env, "ERROR") || env->is_timeout(env, 5000)) {
            op_error(op, env, env->is_timeout(env, 5000) ? WIFI_ERR_TIMEOUT : WIFI_ERR_AT_ERROR);
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
            op_ok(op, env);
        } else if (env->contains(env, "ERROR") ||
                   env->contains(env, "ALREADY") ||
                   env->is_timeout(env, 30000)) {
            op_error(op, env, env->is_timeout(env, 30000) ? WIFI_ERR_TIMEOUT : WIFI_ERR_AT_ERROR);
        }
        break;
    }
    return 0;
}

static int work_sock_send(at_env_t *env)
{
    wifi_op_t *op = (wifi_op_t *)env->params;
    if (env->state == 0) {
        env->println(env, "AT+CIPSEND=%d,%d",
                     op->sock_send.id, (int)op->sock_send.len);
        env->reset_timer(env);
        env->state = 1;
        return 0;
    }
    int rc = at_prompt_send_step(env,
                                 op->sock_send.data, op->sock_send.len,
                                 "SEND OK", NULL, "SEND FAIL",
                                 5000, 10000);
    if      (rc > 0) op_ok(op, env);
    else if (rc < 0) op_error(op, env, WIFI_ERR_AT_ERROR);
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
            op_ok(op, env);
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
            op_error(op, env, env->is_timeout(env, 5000) ? WIFI_ERR_TIMEOUT : WIFI_ERR_AT_ERROR);
        }
        break;
    case 2:
        if (env->contains(env, "+MQTTCONNECTED:") || env->contains(env, "OK")) {
            op_ok(op, env);
        } else if (env->contains(env, "ERROR") ||
                   env->contains(env, "+MQTTDISCONNECTED:") ||
                   env->is_timeout(env, 30000)) {
            op_error(op, env, env->is_timeout(env, 30000) ? WIFI_ERR_TIMEOUT : WIFI_ERR_AT_ERROR);
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
            op_ok(op, env);
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
            if (env->write == NULL ||
                !env->write(env, op->mqtt_pub.payload,
                            (unsigned int)op->mqtt_pub.len)) {
                op_error(op, env, WIFI_ERR_AT_ERROR);
                break;
            }
            env->recvclr(env);
            env->reset_timer(env);
            env->state = 2;
        } else if (env->is_timeout(env, 5000)) {
            op_error(op, env, WIFI_ERR_TIMEOUT);
        }
        break;
    case 2:
        if (env->contains(env, "OK")) {
            op_ok(op, env);
        } else if (env->contains(env, "ERROR") || env->is_timeout(env, 30000)) {
            op_error(op, env, env->is_timeout(env, 30000) ? WIFI_ERR_TIMEOUT : WIFI_ERR_AT_ERROR);
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
            op_ok(op, env);
        } else if (env->contains(env, "ERROR") || env->is_timeout(env, 10000)) {
            op_error(op, env, env->is_timeout(env, 10000) ? WIFI_ERR_TIMEOUT : WIFI_ERR_AT_ERROR);
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
    g_wifi.last_error = WIFI_ERR_OK;
    g_op.done = true;

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
bool xy_wifi_busy(void)    { return !g_op.done; }
xy_wifi_err_t xy_wifi_last_error(void) { return g_wifi.last_error; }

const char *xy_wifi_last_error_str(void)
{
    switch (g_wifi.last_error) {
    case WIFI_ERR_OK:           return "OK";
    case WIFI_ERR_BUSY:         return "BUSY";
    case WIFI_ERR_PARAM:        return "PARAM";
    case WIFI_ERR_TIMEOUT:      return "TIMEOUT";
    case WIFI_ERR_AT_ERROR:     return "AT_ERROR";
    case WIFI_ERR_AUTH:         return "AUTH";
    case WIFI_ERR_NO_AP:        return "NO_AP";
    case WIFI_ERR_CONNECT_FAIL: return "CONNECT_FAIL";
    case WIFI_ERR_DNS:          return "DNS";
    case WIFI_ERR_CLOSED:       return "CLOSED";
    case WIFI_ERR_UNSUPPORTED:  return "UNSUPPORTED";
    default:                    return "UNKNOWN";
    }
}

bool xy_wifi_is_connected(void) { return g_wifi.state == WIFI_ST_CONNECTED; }
bool xy_wifi_has_ip(void)       { return g_wifi.ip[0] != '\0'; }

static bool start_op(at_work_t work)
{
    if (g_wifi.at == NULL || work == NULL) {
        set_error(WIFI_ERR_PARAM);
        return false;
    }
    if (!g_op.done) {
        set_error(WIFI_ERR_BUSY);
        return false;
    }
    g_op.done = false;
    g_op.code = AT_RESP_ERROR;
    if (!at_do_work(g_wifi.at, &g_op, work)) {
        g_op.done = true;
        return false;
    }
    return true;
}

bool xy_wifi_start_init(void) { return start_op(work_init); }

bool xy_wifi_start_connect(const char *ssid, const char *pass)
{
    if (!g_op.done) {
        set_error(WIFI_ERR_BUSY);
        return false;
    }
    if (ssid == NULL || pass == NULL || ssid[0] == '\0') {
        set_error(WIFI_ERR_PARAM);
        return false;
    }
    g_op.conn.ssid = ssid;
    g_op.conn.pass = pass;
    return start_op(work_connect);
}

bool xy_wifi_start_disconnect(void) { return start_op(work_disconnect); }

bool xy_wifi_start_query_state(void) { return start_op(work_query_state); }
bool xy_wifi_start_query_ip(void)    { return start_op(work_query_ip); }
bool xy_wifi_start_query_mac(void)   { return start_op(work_query_mac); }
bool xy_wifi_start_query_rssi(void)  { return start_op(work_query_rssi); }

bool xy_wifi_start_scan(void) { return start_op(work_scan); }

int xy_wifi_scan_count(void) { return s_scan_count; }

const xy_wifi_ap_t *xy_wifi_scan_result(int index)
{
    if (index < 0 || index >= s_scan_count) return NULL;
    return &s_scan_results[index];
}

bool xy_wifi_start_set_dhcp(bool enable)
{
    if (!g_op.done) {
        set_error(WIFI_ERR_BUSY);
        return false;
    }
    g_op.dhcp.enable = enable;
    return start_op(work_set_dhcp);
}

bool xy_wifi_start_set_static_ip(const char *ip,
                                 const char *gateway,
                                 const char *netmask)
{
    if (!g_op.done) {
        set_error(WIFI_ERR_BUSY);
        return false;
    }
    if (!valid_ipv4(ip) || !valid_ipv4(gateway) || !valid_ipv4(netmask)) {
        set_error(WIFI_ERR_PARAM);
        return false;
    }
    g_op.static_ip.ip = ip;
    g_op.static_ip.gateway = gateway;
    g_op.static_ip.netmask = netmask;
    return start_op(work_set_static_ip);
}

bool xy_wifi_start_set_dns(const char *dns1, const char *dns2)
{
    if (!g_op.done) {
        set_error(WIFI_ERR_BUSY);
        return false;
    }
    if (!valid_ipv4(dns1) || (dns2 != NULL && dns2[0] != '\0' && !valid_ipv4(dns2))) {
        set_error(WIFI_ERR_PARAM);
        return false;
    }
    g_op.dns.dns1 = dns1;
    g_op.dns.dns2 = dns2;
    return start_op(work_set_dns);
}

bool xy_wifi_start_sock_open(int id, const char *proto,
                              const char *host, uint16_t port)
{
    if (!g_op.done || id < 0 || id >= XY_WIFI_SOCK_MAX || proto == NULL ||
        host == NULL || proto[0] == '\0' || host[0] == '\0' || port == 0)
        return false;
    g_op.sock_open.id    = id;
    g_op.sock_open.proto = proto;
    g_op.sock_open.host  = host;
    g_op.sock_open.port  = port;
    return start_op(work_sock_open);
}

bool xy_wifi_start_sock_send(int id, const void *data, uint16_t len)
{
    if (!g_op.done || id < 0 || id >= XY_WIFI_SOCK_MAX || data == NULL || len == 0 ||
        !g_socks[id].open)
        return false;
    g_op.sock_send.id   = id;
    g_op.sock_send.data = data;
    g_op.sock_send.len  = len;
    return start_op(work_sock_send);
}

int xy_wifi_sock_recv(int id, void *buf, uint16_t maxlen)
{
    if (id < 0 || id >= XY_WIFI_SOCK_MAX || buf == NULL || maxlen == 0)
        return -1;
    return (int)xy_rb_get(&g_socks[id].rx, (uint8_t *)buf, maxlen);
}

void xy_wifi_start_sock_close(int id)
{
    if (!g_op.done || id < 0 || id >= XY_WIFI_SOCK_MAX) return;
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
    if (!g_op.done || host == NULL || client_id == NULL || host[0] == '\0' ||
        client_id[0] == '\0' || port == 0)
        return false;
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
    if (!g_op.done || topic == NULL || payload == NULL || topic[0] == '\0' ||
        len == 0 || qos > 2)
        return false;
    g_op.mqtt_pub.topic   = topic;
    g_op.mqtt_pub.payload = payload;
    g_op.mqtt_pub.len     = len;
    g_op.mqtt_pub.qos     = qos;
    return start_op(work_mqtt_publish);
}

bool xy_wifi_start_mqtt_subscribe(const char *topic, uint8_t qos)
{
    if (!g_op.done || topic == NULL || topic[0] == '\0' || qos > 2)
        return false;
    g_op.mqtt_sub.topic = topic;
    g_op.mqtt_sub.qos   = qos;
    return start_op(work_mqtt_subscribe);
}

void xy_wifi_mqtt_set_recv_cb(xy_wifi_mqtt_recv_cb_t cb) { s_mqtt_cb = cb; }
