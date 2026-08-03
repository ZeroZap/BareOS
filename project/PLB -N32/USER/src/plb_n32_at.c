#include "plb_n32_at.h"

#include "n32l40x_cfg.h"
#include "xy_log.h"
#include "xy_mem.h"
#include "xy_stdio.h"
#include "xy_string.h"
#include "xy_tick.h"

#ifndef PLB_N32_ENABLE_AT_SELFTEST
#define PLB_N32_ENABLE_AT_SELFTEST 0
#endif

#ifndef PLB_N32_AT_SELFTEST_PAYLOAD_SIZE
#define PLB_N32_AT_SELFTEST_PAYLOAD_SIZE 11u
#endif

XY_MEM_POOL_DECLARE(s_at_mem, 2048u);

static at_obj_t *s_at;

#if PLB_N32_ENABLE_AT_SELFTEST
static const char *const s_selftest_commands[] = {
    "AT",
    "AT+CSQ",
    "AT+CEREG?",
};
static unsigned char s_selftest_payload[PLB_N32_AT_SELFTEST_PAYLOAD_SIZE];
static unsigned int s_selftest_step;
static unsigned int s_selftest_tx_calls;
static unsigned int s_selftest_tx_short_writes;
static unsigned int s_selftest_tx_zero_writes;
static bool s_selftest_measure_tx;
static bool s_selftest_active;
static bool s_selftest_waiting;
static bool s_selftest_urc_received;
static uint32_t s_selftest_urc_deadline_ms;

static int plb_n32_at_parse_qirecv(const char *buf, int len,
                                   int *id, int *bytes, int *hdr)
{
    const char *recv;
    const char *cursor;
    int i;

    for (i = 0; i < len; i++) {
        if (buf[i] == '\n') {
            *hdr = i + 1;
            break;
        }
    }
    if (i >= len) {
        return -1;
    }
    recv = strstr(buf, "\"recv\",");
    if (recv == NULL || recv - buf >= *hdr) {
        return -1;
    }
    cursor = recv + 7;
    *id = (int)xy_strtol(cursor, (char **)&cursor, 10);
    if (*cursor++ != ',') {
        return -1;
    }
    *bytes = (int)xy_strtol(cursor, NULL, 10);
    return 0;
}

static int plb_n32_at_selftest_urc(at_urc_info_t *info)
{
    const char *payload;
    int id;
    int payload_len;
    int rc = at_urc_recv_split(info, plb_n32_at_parse_qirecv, 2,
                               &id, &payload, &payload_len);

    if (rc != 0) {
        return rc > 0 ? rc : 0;
    }
    if (id == 0 && payload_len == 5 && memcmp(payload, "HELLO", 5) == 0) {
        s_selftest_urc_received = true;
        xy_log_i("PLB-N32 AT selftest URC recv id=%d len=%d payload=HELLO",
                 id, payload_len);
    } else {
        xy_log_w("PLB-N32 AT selftest unexpected URC id=%d len=%d",
                 id, payload_len);
    }
    return 0;
}

static const urc_item_t s_selftest_urc_table[] = {
    { "+QIURC: \"recv\"", '\n', plb_n32_at_selftest_urc },
};

static int plb_n32_at_selftest_send_work(at_env_t *env)
{
    int rc;

    if (env->state == 0) {
        env->println(env, "AT+QISEND=0,%u",
                     (unsigned int)sizeof(s_selftest_payload));
        env->reset_timer(env);
        env->state = 1;
        return 0;
    }
    rc = at_prompt_send_step(env, s_selftest_payload,
                             sizeof(s_selftest_payload),
                             "SEND OK", "OK", NULL, 5000u, 10000u);
    if (rc > 0) {
        if (sizeof(s_selftest_payload) > 512u &&
            s_selftest_tx_short_writes == 0u) {
            s_selftest_active = false;
            s_selftest_measure_tx = false;
            xy_log_w("PLB-N32 AT selftest FAILED reason=NO_SHORT_WRITE");
            env->finish(env, AT_RESP_ERROR);
            return 0;
        }
        xy_log_i("PLB-N32 AT selftest QISEND PASSED len=%u",
                  (unsigned int)sizeof(s_selftest_payload));
        xy_log_i("PLB-N32 AT selftest TX calls=%u short=%u zero=%u",
                 s_selftest_tx_calls,
                 s_selftest_tx_short_writes,
                 s_selftest_tx_zero_writes);
        s_selftest_step++;
        s_selftest_waiting = false;
        env->finish(env, AT_RESP_OK);
    } else if (rc < 0) {
        s_selftest_active = false;
        s_selftest_measure_tx = false;
        xy_log_w("PLB-N32 AT selftest FAILED command=AT+QISEND");
        env->finish(env, AT_RESP_ERROR);
    }
    return 0;
}

static void plb_n32_at_selftest_response(at_response_t *response)
{
    unsigned int command_count =
        sizeof(s_selftest_commands) / sizeof(s_selftest_commands[0]);
    const char *command = s_selftest_step < command_count
                          ? s_selftest_commands[s_selftest_step]
                          : "AT+SIMURC=RECV";

    s_selftest_waiting = false;
    xy_log_i("PLB-N32 AT selftest command=%s code=%d len=%u",
             command,
             (int)response->code,
             (unsigned int)response->recvcnt);
    if (response->code != AT_RESP_OK) {
        s_selftest_active = false;
        xy_log_w("PLB-N32 AT selftest FAILED command=%s", command);
        return;
    }

    s_selftest_step++;
    if (s_selftest_step == command_count + 2u) {
        s_selftest_urc_deadline_ms = xy_tick_now_ms() + 1500u;
    }
}
#endif

static unsigned int plb_n32_at_write(const void *buf, unsigned int len)
{
    unsigned int written = n32_uart5_write_nonblock(buf, len);

#if PLB_N32_ENABLE_AT_SELFTEST
    if (s_selftest_measure_tx) {
        s_selftest_tx_calls++;
        if (written < len) {
            s_selftest_tx_short_writes++;
        }
        if (written == 0u && len != 0u) {
            s_selftest_tx_zero_writes++;
        }
    }
#endif
    return written;
}

static unsigned int plb_n32_at_read(void *buf, unsigned int len)
{
    int read_len = n32_uart5_secboot_read((uint8_t *)buf, (size_t)len, 0u);

    return read_len > 0 ? (unsigned int)read_len : 0u;
}

static const at_adapter_t s_adapter = {
    .lock = NULL,
    .unlock = NULL,
    .write = plb_n32_at_write,
    .read = plb_n32_at_read,
    .error = NULL,
    .debug = NULL,
#if AT_URC_WARCH_EN
    .urc_bufsize = 256u,
#endif
    .recv_bufsize = 256u,
    .rx_pending = n32_uart5_rx_pending,
    .tx_idle = n32_uart5_tx_idle,
};

bool plb_n32_at_init(void)
{
    if (s_at != NULL) {
        return true;
    }

    XY_MEM_POOL_INIT(s_at_mem);
    s_at = at_obj_create(&s_adapter);
#if PLB_N32_ENABLE_AT_SELFTEST
    if (s_at != NULL) {
        at_obj_set_urc(s_at, s_selftest_urc_table,
                       sizeof(s_selftest_urc_table) /
                       sizeof(s_selftest_urc_table[0]));
    }
#endif
    return s_at != NULL;
}

bool plb_n32_at_selftest_start(void)
{
#if PLB_N32_ENABLE_AT_SELFTEST
    unsigned int i;

    if (s_at == NULL || s_selftest_active) {
        return false;
    }
    s_selftest_step = 0u;
    s_selftest_waiting = false;
    s_selftest_urc_received = false;
    s_selftest_urc_deadline_ms = 0u;
    s_selftest_tx_calls = 0u;
    s_selftest_tx_short_writes = 0u;
    s_selftest_tx_zero_writes = 0u;
    s_selftest_measure_tx = false;
    if (sizeof(s_selftest_payload) == 11u) {
        memcpy(s_selftest_payload, "PLB-AT-DATA", 11u);
    } else {
        for (i = 0u; i < sizeof(s_selftest_payload); i++) {
            s_selftest_payload[i] = (unsigned char)((i * 31u + 7u) & 0xffu);
        }
    }
    s_selftest_active = true;
    xy_log_i("PLB-N32 AT selftest start");
    return true;
#else
    return false;
#endif
}

void plb_n32_at_process(void)
{
    if (s_at != NULL) {
        at_obj_process(s_at);
#if PLB_N32_ENABLE_AT_SELFTEST
        if (s_selftest_active && !s_selftest_waiting) {
            at_attr_t attr;
            unsigned int command_count =
                sizeof(s_selftest_commands) / sizeof(s_selftest_commands[0]);

            if (s_selftest_step < command_count) {
                at_attr_deinit(&attr);
                attr.cb = plb_n32_at_selftest_response;
                attr.timeout = 1000u;
                attr.retry = 2u;
                if (at_exec_cmd(s_at, &attr, "%s",
                                s_selftest_commands[s_selftest_step])) {
                    s_selftest_waiting = true;
                }
            } else if (s_selftest_step == command_count) {
                s_selftest_measure_tx = true;
                if (at_do_work(s_at, NULL, plb_n32_at_selftest_send_work)) {
                    s_selftest_waiting = true;
                } else {
                    s_selftest_measure_tx = false;
                }
            } else if (s_selftest_step == command_count + 1u) {
                s_selftest_measure_tx = false;
                at_attr_deinit(&attr);
                attr.cb = plb_n32_at_selftest_response;
                attr.timeout = 1000u;
                attr.retry = 2u;
                if (at_exec_cmd(s_at, &attr, "AT+SIMURC=RECV")) {
                    s_selftest_waiting = true;
                }
            } else if (s_selftest_urc_received) {
                s_selftest_active = false;
                xy_log_i("PLB-N32 AT selftest PASSED");
            } else if ((int32_t)(xy_tick_now_ms() -
                                 s_selftest_urc_deadline_ms) >= 0) {
                s_selftest_active = false;
                xy_log_w("PLB-N32 AT selftest FAILED reason=URC_TIMEOUT");
            }
        }
#endif
    }
}

at_obj_t *plb_n32_at_obj(void)
{
    return s_at;
}
