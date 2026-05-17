#include "xy_cell_drv.h"
#include "xy_string.h"
#include "xy_stdio.h"

/* ── globals ─────────────────────────────────────────────────────────── */

xy_cell_t      g_cell;
xy_cell_op_t   g_cell_op;
xy_cell_sock_t g_cell_socks[XY_CELL_SOCK_MAX];

static const xy_cell_drv_t      *s_drv;
static xy_cell_mqtt_recv_cb_t    s_mqtt_cb;
static xy_cell_sms_recv_cb_t     s_sms_cb;

#define MAX_URC  16
static urc_item_t s_urc_tbl[MAX_URC];
static int        s_urc_cnt;

/* ── SMS URC scratch buffers ─────────────────────────────────────────── */

#define SMS_TXT_MAX  162
static char s_sms_num[32];
static char s_sms_txt[SMS_TXT_MAX + 1];

/* ── parse helpers ────────────────────────────────────────────────────── */

/* Extract the first run of minlen..maxlen consecutive digits from buf. */
static void parse_digits(const char *buf, char *out, int minlen, int maxlen)
{
    const char *p = buf;
    while (*p) {
        if (*p >= '0' && *p <= '9') {
            int len = 0;
            const char *s = p;
            while (s[len] >= '0' && s[len] <= '9') len++;
            if (len >= minlen && len <= maxlen) {
                strncpy(out, s, len);
                out[len] = '\0';
                return;
            }
            p += len;
        } else {
            p++;
        }
    }
}

static void parse_csq(const char *buf, int *rssi_dbm)
{
    const char *p = strstr(buf, "+CSQ:");
    if (!p) return;
    p += 5;
    while (*p == ' ') p++;
    int rssi = (int)xy_strtol(p, NULL, 10);
    *rssi_dbm = (rssi == 99 || rssi == 0) ? 0 : (-113 + rssi * 2);
}

static int parse_reg_stat(const char *buf)
{
    const char *p = strstr(buf, "+CEREG:");
    if (!p) p = strstr(buf, "+CREG:");
    if (!p) return -1;
    p = strchr(p, ':');
    if (!p) return -1;
    p++;
    while (*p == ' ') p++;
    char *comma = strchr(p, ',');
    if (comma) p = comma + 1;
    while (*p == ' ') p++;
    return (int)xy_strtol(p, NULL, 10);
}

/* Extract the first dotted-quad IPv4 address found in buf. */
void cell_parse_ip(const char *buf, char *ip, int iplen)
{
    const char *p = buf;
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

/* ── SMS URC handler (standard 3GPP, registered for all drivers) ─────── */

int xy_cell_urc_cmt(at_urc_info_t *info)
{
    const char *p = info->urcbuf;
    int hdr_end = 0;
    int i;

    for (i = 0; i < info->urclen; i++) {
        if (p[i] == '\n') { hdr_end = i + 1; break; }
    }
    if (!hdr_end) return 0;

    if (info->urclen == hdr_end) {
        /* First call: request SMS text body (max 160 chars + CRLF) */
        return SMS_TXT_MAX;
    }

    /* Second call: parse phone number from header "+CMT: "+num","",..." */
    {
        const char *q = strchr(p, '"');
        if (q) {
            const char *eq = strchr(q + 1, '"');
            if (eq && eq > q + 1) {
                int n = (int)(eq - q - 1);
                if (n >= (int)sizeof(s_sms_num)) n = (int)sizeof(s_sms_num) - 1;
                strncpy(s_sms_num, q + 1, n);
                s_sms_num[n] = '\0';
            }
        }
    }

    /* Extract text (after header newline, stop at CR/LF) */
    {
        const char *txt = p + hdr_end;
        int avail = info->urclen - hdr_end;
        int tlen = 0;
        while (tlen < avail && tlen < SMS_TXT_MAX &&
               txt[tlen] != '\r' && txt[tlen] != '\n') {
            tlen++;
        }
        strncpy(s_sms_txt, txt, tlen);
        s_sms_txt[tlen] = '\0';
    }

    if (s_sms_cb) s_sms_cb(s_sms_num, s_sms_txt);
    return 0;
}

/* ── MQTT deliver (called by driver URC handlers) ────────────────────── */

void xy_cell_mqtt_deliver(int cid, const char *topic, uint16_t topiclen,
                          const void *payload, uint16_t payloadlen)
{
    if (!s_mqtt_cb) return;
    char t[128];
    int n = (topiclen < (int)sizeof(t) - 1) ? topiclen : (int)sizeof(t) - 1;
    strncpy(t, topic, n);
    t[n] = '\0';
    s_mqtt_cb(cid, t, payload, payloadlen);
}

/* ── core work handlers (3GPP standard AT commands) ──────────────────── */

static int work_modem_init(at_env_t *env)
{
    xy_cell_op_t *op = (xy_cell_op_t *)env->params;
    switch (env->state) {
    case 0:
        env->println(env, "ATE0");
        env->reset_timer(env);
        env->state = 1;
        break;
    case 1:
        if (env->contains(env, "OK")) {
            env->recvclr(env);
            env->println(env, "AT+CMEE=2");
            env->reset_timer(env);
            env->state = 2;
        } else if (env->is_timeout(env, 3000)) {
            OP_ERR(op, env);
        }
        break;
    case 2:
        if (env->contains(env, "OK")) {
            env->recvclr(env);
            env->println(env, "AT+CMGF=1");
            env->reset_timer(env);
            env->state = 3;
        } else if (env->contains(env, "ERROR") || env->is_timeout(env, 3000)) {
            OP_ERR(op, env);
        }
        break;
    case 3:
        if (env->contains(env, "OK")) {
            env->recvclr(env);
            env->println(env, "AT+CNMI=2,2,0,0,0");
            env->reset_timer(env);
            env->state = 4;
        } else if (env->contains(env, "ERROR") || env->is_timeout(env, 3000)) {
            /* CMGF/CNMI may not apply on all modems — continue anyway */
            g_cell.state = CELL_ST_INIT;
            OP_OK(op, env);
        }
        break;
    case 4:
        if (env->contains(env, "OK") || env->contains(env, "ERROR")) {
            g_cell.state = CELL_ST_INIT;
            OP_OK(op, env);
        } else if (env->is_timeout(env, 3000)) {
            g_cell.state = CELL_ST_INIT;
            OP_OK(op, env);
        }
        break;
    }
    return 0;
}

static int work_check_sim(at_env_t *env)
{
    xy_cell_op_t *op = (xy_cell_op_t *)env->params;
    switch (env->state) {
    case 0:
        env->println(env, "AT+CIMI");
        env->reset_timer(env);
        env->state = 1;
        break;
    case 1:
        if (env->contains(env, "OK")) {
            parse_digits(env->recvbuf(env), g_cell.imsi, 10, 15);
            env->recvclr(env);
            env->println(env, "AT+CCID");
            env->reset_timer(env);
            env->state = 2;
        } else if (env->contains(env, "ERROR") || env->is_timeout(env, 5000)) {
            OP_ERR(op, env);
        }
        break;
    case 2:
        if (env->contains(env, "OK")) {
            parse_digits(env->recvbuf(env), g_cell.iccid, 18, 20);
            g_cell.state = CELL_ST_SIM_OK;
            OP_OK(op, env);
        } else if (env->contains(env, "ERROR") || env->is_timeout(env, 5000)) {
            /* Try alternate ICCID command */
            env->recvclr(env);
            env->println(env, "AT+ICCID");
            env->reset_timer(env);
            env->state = 3;
        }
        break;
    case 3:
        if (env->contains(env, "OK")) {
            parse_digits(env->recvbuf(env), g_cell.iccid, 18, 20);
        }
        /* Succeed regardless — ICCID is optional */
        g_cell.state = CELL_ST_SIM_OK;
        OP_OK(op, env);
        break;
    }
    return 0;
}

static int work_update_signal(at_env_t *env)
{
    xy_cell_op_t *op = (xy_cell_op_t *)env->params;
    switch (env->state) {
    case 0:
        env->println(env, "AT+CSQ");
        env->reset_timer(env);
        env->state = 1;
        break;
    case 1:
        if (env->contains(env, "+CSQ:")) {
            parse_csq(env->recvbuf(env), &g_cell.rssi_dbm);
            OP_OK(op, env);
        } else if (env->contains(env, "ERROR") || env->is_timeout(env, 3000)) {
            OP_ERR(op, env);
        }
        break;
    }
    return 0;
}

static int work_check_reg(at_env_t *env)
{
    xy_cell_op_t *op = (xy_cell_op_t *)env->params;
    switch (env->state) {
    case 0:
        env->println(env, "AT+CEREG?");
        env->reset_timer(env);
        env->state = 1;
        break;
    case 1:
        if (env->contains(env, "+CEREG:")) {
            int stat = parse_reg_stat(env->recvbuf(env));
            if (stat == 1 || stat == 5) g_cell.state = CELL_ST_REGISTERED;
            OP_OK(op, env);
        } else if (env->contains(env, "ERROR") || env->is_timeout(env, 5000)) {
            env->recvclr(env);
            env->println(env, "AT+CREG?");
            env->reset_timer(env);
            env->state = 2;
        }
        break;
    case 2:
        if (env->contains(env, "+CREG:")) {
            int stat = parse_reg_stat(env->recvbuf(env));
            if (stat == 1 || stat == 5) g_cell.state = CELL_ST_REGISTERED;
            OP_OK(op, env);
        } else if (env->contains(env, "ERROR") || env->is_timeout(env, 5000)) {
            OP_ERR(op, env);
        }
        break;
    }
    return 0;
}

static int work_sms_send(at_env_t *env)
{
    xy_cell_op_t *op = (xy_cell_op_t *)env->params;
    static const char ctrl_z = '\x1A';
    switch (env->state) {
    case 0:
        env->println(env, "AT+CMGS=\"%s\"", op->sms.number);
        env->reset_timer(env);
        env->state = 1;
        break;
    case 1:
        if (env->contains(env, ">")) {
            unsigned int tlen = (unsigned int)strlen(op->sms.text);
            env->obj->adap->write(op->sms.text, tlen);
            env->obj->adap->write(&ctrl_z, 1);
            env->recvclr(env);
            env->reset_timer(env);
            env->state = 2;
        } else if (env->is_timeout(env, 5000)) {
            OP_ERR(op, env);
        }
        break;
    case 2:
        if (env->contains(env, "+CMGS:") || env->contains(env, "OK")) {
            OP_OK(op, env);
        } else if (env->contains(env, "ERROR") || env->is_timeout(env, 60000)) {
            OP_ERR(op, env);
        }
        break;
    }
    return 0;
}

/* ── URC table construction ───────────────────────────────────────────── */

static void build_urc_table(void)
{
    static const urc_item_t k_cmt_urc = { "+CMT:", '\n', xy_cell_urc_cmt };
    int i;
    s_urc_cnt = 0;
    for (i = 0; i < s_drv->urc_cnt && s_urc_cnt < MAX_URC; i++)
        memcpy(&s_urc_tbl[s_urc_cnt++], &s_drv->urc_tbl[i], sizeof(urc_item_t));
    if (s_urc_cnt < MAX_URC)
        memcpy(&s_urc_tbl[s_urc_cnt++], &k_cmt_urc, sizeof(urc_item_t));
    at_obj_set_urc(g_cell.at, s_urc_tbl, s_urc_cnt);
}

/* ── init ─────────────────────────────────────────────────────────────── */

void xy_cell_init(at_obj_t *at, xy_cell_mdm_t mdm)
{
    int i;
    memset(&g_cell,      0, sizeof(g_cell));
    memset(&g_cell_op,   0, sizeof(g_cell_op));
    memset(g_cell_socks, 0, sizeof(g_cell_socks));

    g_cell.at = at;

    switch (mdm) {
    case CELL_MDM_SIM76XX: s_drv = &xy_cell_drv_sim76;   break;
    case CELL_MDM_EC2X:    s_drv = &xy_cell_drv_ec2x;    break;
    case CELL_MDM_ML302:   s_drv = &xy_cell_drv_ml302;   break;
    case CELL_MDM_FIBOCOM: s_drv = &xy_cell_drv_fibocom; break;
    default:               s_drv = &xy_cell_drv_sim76;   break;
    }

    for (i = 0; i < XY_CELL_SOCK_MAX; i++)
        xy_rb_init(&g_cell_socks[i].rx, g_cell_socks[i].mem, XY_CELL_SOCK_RXBUF);

    build_urc_table();
}

/* ── public API ───────────────────────────────────────────────────────── */

xy_cell_state_t  xy_cell_get_state(void) { return g_cell.state; }
const xy_cell_t *xy_cell_get_info(void)  { return &g_cell; }

bool xy_cell_op_done(void) { return (bool)g_cell_op.done; }
bool xy_cell_op_ok(void)   { return g_cell_op.done && (g_cell_op.code == AT_RESP_OK); }

static bool start_op(at_work_t work)
{
    if (!work) return false;
    g_cell_op.done = false;
    g_cell_op.code = AT_RESP_ERROR;
    return at_do_work(g_cell.at, &g_cell_op, work);
}

bool xy_cell_start_modem_init(void)    { return start_op(work_modem_init); }
bool xy_cell_start_check_sim(void)     { return start_op(work_check_sim); }
bool xy_cell_start_update_signal(void) { return start_op(work_update_signal); }
bool xy_cell_start_check_reg(void)     { return start_op(work_check_reg); }

bool xy_cell_start_pdp_activate(const char *apn)
{
    g_cell_op.pdp.apn = apn;
    return start_op(s_drv->pdp_activate);
}

bool xy_cell_start_pdp_deactivate(void)
{
    return start_op(s_drv->pdp_deactivate);
}

bool xy_cell_start_sock_open(int id, const char *proto,
                              const char *host, uint16_t port)
{
    if (id < 0 || id >= XY_CELL_SOCK_MAX) return false;
    g_cell_op.sock_open.id    = id;
    g_cell_op.sock_open.proto = proto;
    g_cell_op.sock_open.host  = host;
    g_cell_op.sock_open.port  = port;
    g_cell_op.sock_open.tls   = (strcmp(proto, "TLS") == 0 ||
                                  strcmp(proto, "SSL") == 0);
    return start_op(s_drv->sock_open);
}

bool xy_cell_start_sock_send(int id, const void *data, uint16_t len)
{
    if (id < 0 || id >= XY_CELL_SOCK_MAX) return false;
    g_cell_op.sock_send.id   = id;
    g_cell_op.sock_send.data = data;
    g_cell_op.sock_send.len  = len;
    return start_op(s_drv->sock_send);
}

int xy_cell_sock_recv(int id, void *buf, uint16_t maxlen)
{
    if (id < 0 || id >= XY_CELL_SOCK_MAX) return -1;
    return (int)xy_rb_get(&g_cell_socks[id].rx, (uint8_t *)buf, maxlen);
}

void xy_cell_start_sock_close(int id)
{
    if (id < 0 || id >= XY_CELL_SOCK_MAX) return;
    g_cell_op.sock_close.id = id;
    start_op(s_drv->sock_close);
}

bool xy_cell_sock_is_open(int id)
{
    if (id < 0 || id >= XY_CELL_SOCK_MAX) return false;
    return g_cell_socks[id].open;
}

bool xy_cell_start_mqtt_connect(int cid, const char *host, uint16_t port,
                                 const char *client_id,
                                 const char *user, const char *pass, bool tls)
{
    if (!s_drv->mqtt_connect) return false;
    g_cell_op.mqtt_conn.cid       = cid;
    g_cell_op.mqtt_conn.host      = host;
    g_cell_op.mqtt_conn.port      = port;
    g_cell_op.mqtt_conn.client_id = client_id;
    g_cell_op.mqtt_conn.user      = user;
    g_cell_op.mqtt_conn.pass      = pass;
    g_cell_op.mqtt_conn.tls       = tls;
    return start_op(s_drv->mqtt_connect);
}

bool xy_cell_start_mqtt_disconnect(int cid)
{
    if (!s_drv->mqtt_disconnect) return false;
    g_cell_op.mqtt_disc.cid = cid;
    return start_op(s_drv->mqtt_disconnect);
}

bool xy_cell_start_mqtt_publish(int cid, const char *topic,
                                 const void *payload, uint16_t len, uint8_t qos)
{
    if (!s_drv->mqtt_publish) return false;
    g_cell_op.mqtt_pub.cid     = cid;
    g_cell_op.mqtt_pub.topic   = topic;
    g_cell_op.mqtt_pub.payload = payload;
    g_cell_op.mqtt_pub.len     = len;
    g_cell_op.mqtt_pub.qos     = qos;
    return start_op(s_drv->mqtt_publish);
}

bool xy_cell_start_mqtt_subscribe(int cid, const char *topic, uint8_t qos)
{
    if (!s_drv->mqtt_subscribe) return false;
    g_cell_op.mqtt_sub.cid   = cid;
    g_cell_op.mqtt_sub.topic = topic;
    g_cell_op.mqtt_sub.qos   = qos;
    return start_op(s_drv->mqtt_subscribe);
}

void xy_cell_mqtt_set_recv_cb(xy_cell_mqtt_recv_cb_t cb) { s_mqtt_cb = cb; }

bool xy_cell_start_sms_send(const char *number, const char *text)
{
    g_cell_op.sms.number = number;
    g_cell_op.sms.text   = text;
    return start_op(work_sms_send);
}

void xy_cell_sms_set_recv_cb(xy_cell_sms_recv_cb_t cb) { s_sms_cb = cb; }
