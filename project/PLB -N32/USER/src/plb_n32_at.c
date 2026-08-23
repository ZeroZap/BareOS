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

#ifndef PLB_N32_AT_SELFTEST_SEGMENTED
#define PLB_N32_AT_SELFTEST_SEGMENTED 0
#endif

#ifndef PLB_N32_AT_SELFTEST_ABORT
#define PLB_N32_AT_SELFTEST_ABORT 0
#endif

#ifndef PLB_N32_AT_SELFTEST_URC_BURST
#define PLB_N32_AT_SELFTEST_URC_BURST 0
#endif

#ifndef PLB_N32_AT_SELFTEST_URC_RECOVERY
#define PLB_N32_AT_SELFTEST_URC_RECOVERY 0
#endif

#ifndef PLB_N32_AT_SELFTEST_URC_OVERFLOW
#define PLB_N32_AT_SELFTEST_URC_OVERFLOW 0
#endif

#ifndef PLB_N32_AT_SELFTEST_URC_INTERLEAVE
#define PLB_N32_AT_SELFTEST_URC_INTERLEAVE 0
#endif

#ifndef PLB_N32_AT_SELFTEST_URC_PROMPT
#define PLB_N32_AT_SELFTEST_URC_PROMPT 0
#endif

#ifndef PLB_N32_AT_SELFTEST_URC_SEND_RESULT
#define PLB_N32_AT_SELFTEST_URC_SEND_RESULT 0
#endif

#ifndef PLB_N32_AT_SELFTEST_URC_FAKE_PROMPT
#define PLB_N32_AT_SELFTEST_URC_FAKE_PROMPT 0
#endif

#ifndef PLB_N32_AT_SELFTEST_URC_FAKE_SEND_OK
#define PLB_N32_AT_SELFTEST_URC_FAKE_SEND_OK 0
#endif

#ifndef PLB_N32_AT_SELFTEST_URC_FAKE_ERROR
#define PLB_N32_AT_SELFTEST_URC_FAKE_ERROR 0
#endif

#ifndef PLB_N32_AT_SELFTEST_URC_PARTIAL_ERROR
#define PLB_N32_AT_SELFTEST_URC_PARTIAL_ERROR 0
#endif

#ifndef PLB_N32_AT_SELFTEST_URC_PARTIAL_TAIL
#define PLB_N32_AT_SELFTEST_URC_PARTIAL_TAIL 0
#endif

#ifndef PLB_N32_AT_SELFTEST_URC_PARTIAL_SEND_OK
#define PLB_N32_AT_SELFTEST_URC_PARTIAL_SEND_OK 0
#endif

#ifndef PLB_N32_AT_SELFTEST_URC_PARTIAL_SEND_TAIL
#define PLB_N32_AT_SELFTEST_URC_PARTIAL_SEND_TAIL 0
#endif

#ifndef PLB_N32_AT_SELFTEST_LATE_RESPONSE
#define PLB_N32_AT_SELFTEST_LATE_RESPONSE 0
#endif

#ifndef PLB_N32_AT_SELFTEST_RESP_BOUNDARY
#define PLB_N32_AT_SELFTEST_RESP_BOUNDARY 0
#endif

#ifndef PLB_N32_AT_SELFTEST_TX_ZERO_STALL
#define PLB_N32_AT_SELFTEST_TX_ZERO_STALL 0
#endif

#ifndef PLB_N32_AT_SELFTEST_WORK_QUEUE
#define PLB_N32_AT_SELFTEST_WORK_QUEUE 0
#endif

#ifndef PLB_N32_AT_SELFTEST_URC_BOUNDARY
#define PLB_N32_AT_SELFTEST_URC_BOUNDARY 0
#endif

#ifndef PLB_N32_AT_SELFTEST_MULTI_OBJ
#define PLB_N32_AT_SELFTEST_MULTI_OBJ 0
#endif

#ifndef PLB_N32_AT_SELFTEST_ERROR_CALLBACK
#define PLB_N32_AT_SELFTEST_ERROR_CALLBACK 0
#endif

#ifndef PLB_N32_AT_SELFTEST_DESTROY
#define PLB_N32_AT_SELFTEST_DESTROY 0
#endif

#ifndef PLB_N32_AT_SELFTEST_TICK_WRAP
#define PLB_N32_AT_SELFTEST_TICK_WRAP 0
#endif

#ifndef PLB_N32_AT_SELFTEST_URC_PREFIX
#define PLB_N32_AT_SELFTEST_URC_PREFIX 0
#endif

#define PLB_N32_AT_URC_BURST_COUNT 32u
#define PLB_N32_AT_URC_INACTIVITY_MS 1500u
#define PLB_N32_AT_URC_SETTLE_MS 500u
#define PLB_N32_AT_TX_ZERO_STALL_MS 700u
#define PLB_N32_AT_WORK_QUEUE_COUNT 8u

#if PLB_N32_AT_SELFTEST_MULTI_OBJ
XY_MEM_POOL_DECLARE(s_at_mem, 6144u);
#else
XY_MEM_POOL_DECLARE(s_at_mem, 2048u);
#endif

static at_obj_t *s_at;
static const at_adapter_t s_adapter;
#if PLB_N32_AT_SELFTEST_MULTI_OBJ
static at_obj_t *s_at_aux[2];
#endif

#if PLB_N32_ENABLE_AT_SELFTEST
static const char *const s_selftest_commands[] = {
    "AT",
    "AT+CSQ",
    "AT+CEREG?",
};
#if PLB_N32_AT_SELFTEST_RESP_BOUNDARY
static const char *const s_selftest_boundary_commands[] = {
    "AT+SIMBUF=255",
    "AT+SIMBUF=256",
    "AT+SIMBUF=RECOVER",
    "AT+SIMBUF=257",
    "AT+SIMBUF=RECOVER",
};
#endif
static unsigned char s_selftest_payload[PLB_N32_AT_SELFTEST_PAYLOAD_SIZE];
static const unsigned char s_selftest_ctrl_z = 0x1au;
static unsigned int s_selftest_step;
static unsigned int s_selftest_tx_calls;
static unsigned int s_selftest_tx_short_writes;
static unsigned int s_selftest_tx_zero_writes;
static bool s_selftest_measure_tx;
#if PLB_N32_AT_SELFTEST_TX_ZERO_STALL
static bool s_selftest_cmd_stall_started;
static bool s_selftest_cmd_stall_done;
static bool s_selftest_payload_stall_started;
static bool s_selftest_payload_stall_done;
static bool s_selftest_stall_pm_blocked;
static uint32_t s_selftest_cmd_stall_ms;
static uint32_t s_selftest_payload_stall_ms;
#endif
#if PLB_N32_AT_SELFTEST_WORK_QUEUE
typedef struct {
    unsigned char id;
} plb_n32_at_queue_item_t;

static plb_n32_at_queue_item_t s_selftest_queue_items[17];
static plb_n32_at_queue_item_t s_selftest_abort_items[PLB_N32_AT_WORK_QUEUE_COUNT];
static at_context_t s_selftest_abort_contexts[PLB_N32_AT_WORK_QUEUE_COUNT];
static unsigned char s_selftest_queue_order[17];
static unsigned int s_selftest_queue_phase;
static unsigned int s_selftest_queue_runs;
static bool s_selftest_queue_failed;
#endif
#if PLB_N32_AT_SELFTEST_URC_BOUNDARY
static unsigned int s_selftest_boundary_max_received;
static unsigned int s_selftest_boundary_recovery_received;
static unsigned int s_selftest_boundary_timeout_received;
static unsigned int s_selftest_boundary_unexpected;
#endif
#if PLB_N32_AT_SELFTEST_MULTI_OBJ
typedef struct {
    at_obj_t *owner;
    unsigned char object;
    unsigned char sequence;
} plb_n32_at_multi_item_t;

static plb_n32_at_multi_item_t s_selftest_multi_items[11];
static at_context_t s_selftest_multi_contexts[3];
static unsigned char s_selftest_multi_count[3];
static unsigned char s_selftest_multi_next[3];
static unsigned int s_selftest_multi_phase;
static bool s_selftest_multi_failed;
#endif
#if PLB_N32_AT_SELFTEST_ERROR_CALLBACK
static unsigned int s_selftest_error_step;
static unsigned int s_selftest_error_hook_count;
static unsigned int s_selftest_error_callback_count;
static at_response_t s_selftest_error_hook_response;
static bool s_selftest_error_failed;
#endif
#if PLB_N32_AT_SELFTEST_DESTROY
static at_context_t s_selftest_destroy_contexts[8];
static unsigned char s_selftest_destroy_ids[8];
static unsigned int s_selftest_destroy_phase;
static unsigned int s_selftest_destroy_runs;
static unsigned int s_selftest_destroy_object_mem;
static bool s_selftest_destroy_failed;
#endif
#if PLB_N32_AT_SELFTEST_TICK_WRAP
static unsigned int s_selftest_wrap_phase;
static unsigned int s_selftest_wrap_wait_runs;
static unsigned int s_selftest_wrap_timeout_callbacks;
static unsigned int s_selftest_wrap_urc_timeouts;
static unsigned int s_selftest_wrap_recoveries;
static uint32_t s_selftest_wrap_original_tick;
static bool s_selftest_wrap_failed;
#endif
#if PLB_N32_AT_SELFTEST_URC_PREFIX
static unsigned int s_selftest_prefix_short;
static unsigned int s_selftest_prefix_long;
static unsigned int s_selftest_prefix_embedded;
static unsigned int s_selftest_prefix_recovery;
static unsigned int s_selftest_prefix_unexpected;
static bool s_selftest_prefix_command_sent;
static bool s_selftest_prefix_marker_done;
#endif
#if PLB_N32_AT_SELFTEST_ABORT
static bool s_selftest_abort_armed;
static bool s_selftest_abort_requested;
static bool s_selftest_abort_wait_idle;
static unsigned int s_selftest_abort_accepted;
#endif
static bool s_selftest_active;
static bool s_selftest_waiting;
static bool s_selftest_urc_received;
static uint32_t s_selftest_urc_deadline_ms;
#if PLB_N32_AT_SELFTEST_LATE_RESPONSE
static bool s_selftest_late_a_received;
#endif
#if PLB_N32_AT_SELFTEST_URC_RECOVERY
static bool s_selftest_urc_timeout_received;
#endif
#if PLB_N32_AT_SELFTEST_URC_OVERFLOW
static bool s_selftest_urc_overflow_unexpected;
#endif
#if PLB_N32_AT_SELFTEST_URC_INTERLEAVE
static bool s_selftest_interleave_csq_valid;
#endif
#if PLB_N32_AT_SELFTEST_URC_PROMPT
static bool s_selftest_prompt_urc_received;
#endif
#if PLB_N32_AT_SELFTEST_URC_SEND_RESULT || \
    PLB_N32_AT_SELFTEST_URC_FAKE_SEND_OK || \
    PLB_N32_AT_SELFTEST_URC_FAKE_ERROR || \
    PLB_N32_AT_SELFTEST_URC_PARTIAL_ERROR || \
    PLB_N32_AT_SELFTEST_URC_PARTIAL_TAIL || \
    PLB_N32_AT_SELFTEST_URC_PARTIAL_SEND_OK || \
    PLB_N32_AT_SELFTEST_URC_PARTIAL_SEND_TAIL
static bool s_selftest_payload_queued;
#endif
#if PLB_N32_AT_SELFTEST_URC_SEND_RESULT
static bool s_selftest_send_result_urc_received;
#endif
#if PLB_N32_AT_SELFTEST_URC_FAKE_PROMPT
static bool s_selftest_fake_prompt_urc_received;
static uint32_t s_selftest_fake_prompt_urc_ms;
#endif
#if PLB_N32_AT_SELFTEST_URC_FAKE_SEND_OK
static bool s_selftest_fake_send_ok_urc_received;
static uint32_t s_selftest_fake_send_ok_urc_ms;
#endif
#if PLB_N32_AT_SELFTEST_URC_FAKE_ERROR
static bool s_selftest_fake_error_urc_received;
static uint32_t s_selftest_fake_error_urc_ms;
#endif
#if PLB_N32_AT_SELFTEST_URC_PARTIAL_ERROR
static bool s_selftest_partial_error_timeout_received;
static uint32_t s_selftest_partial_error_timeout_ms;
#endif
#if PLB_N32_AT_SELFTEST_URC_PARTIAL_TAIL
static bool s_selftest_partial_tail_received;
static bool s_selftest_partial_tail_timeout;
static uint32_t s_selftest_partial_tail_start_ms;
#endif
#if PLB_N32_AT_SELFTEST_URC_PARTIAL_SEND_OK
static bool s_selftest_partial_send_ok_timeout_received;
static bool s_selftest_partial_send_ok_real_error_received;
static uint32_t s_selftest_partial_send_ok_timeout_ms;
#endif
#if PLB_N32_AT_SELFTEST_URC_PARTIAL_SEND_TAIL
static bool s_selftest_partial_send_tail_received;
static bool s_selftest_partial_send_tail_timeout;
static bool s_selftest_partial_send_tail_real_error;
static uint32_t s_selftest_partial_send_tail_start_ms;
#endif
#if PLB_N32_AT_SELFTEST_URC_BURST
static unsigned int s_selftest_urc_burst_received;
static bool s_selftest_urc_burst_failed;
static bool s_selftest_urc_burst_excess;
static uint32_t s_selftest_urc_settle_deadline_ms;
static uint32_t s_selftest_urc_settle_rx_count;
#endif

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
    int rc;

#if PLB_N32_AT_SELFTEST_URC_BOUNDARY
    if (info->status == URC_RECV_TIMEOUT) {
        s_selftest_boundary_timeout_received++;
        xy_log_i("PLB-N32 AT selftest URC boundary short payload timeout observed");
        return 0;
    }
#endif

#if PLB_N32_AT_SELFTEST_URC_PARTIAL_ERROR
    if (info->status == URC_RECV_TIMEOUT) {
        s_selftest_urc_received = true;
        s_selftest_partial_error_timeout_received = true;
        s_selftest_partial_error_timeout_ms = xy_tick_now_ms();
        xy_log_i("PLB-N32 AT selftest partial ERROR URC timeout observed");
        return 0;
    }
#endif
#if PLB_N32_AT_SELFTEST_URC_PARTIAL_TAIL
    if (info->status == URC_RECV_TIMEOUT) {
        s_selftest_partial_tail_timeout = true;
        xy_log_w("PLB-N32 AT selftest partial tail URC timeout unexpected");
        return 0;
    }
#endif
#if PLB_N32_AT_SELFTEST_URC_PARTIAL_SEND_OK
    if (info->status == URC_RECV_TIMEOUT) {
        s_selftest_urc_received = true;
        s_selftest_partial_send_ok_timeout_received = true;
        s_selftest_partial_send_ok_timeout_ms = xy_tick_now_ms();
        xy_log_i("PLB-N32 AT selftest partial SEND OK URC timeout observed");
        return 0;
    }
#endif
#if PLB_N32_AT_SELFTEST_URC_PARTIAL_SEND_TAIL
    if (info->status == URC_RECV_TIMEOUT) {
        s_selftest_partial_send_tail_timeout = true;
        xy_log_w("PLB-N32 AT selftest partial SEND tail URC timeout unexpected");
        return 0;
    }
#endif
#if PLB_N32_AT_SELFTEST_URC_RECOVERY
    if (info->status == URC_RECV_TIMEOUT) {
        s_selftest_urc_timeout_received = true;
        xy_log_i("PLB-N32 AT selftest URC timeout observed");
        return 0;
    }
#endif
    rc = at_urc_recv_split(info, plb_n32_at_parse_qirecv, 2,
                               &id, &payload, &payload_len);

    if (rc != 0) {
        return rc > 0 ? rc : 0;
    }
#if PLB_N32_AT_SELFTEST_URC_BOUNDARY
    if (id == 0 && payload_len == 232) {
        int i;

        for (i = 0; i < payload_len; i++) {
            if (payload[i] != 'M') {
                s_selftest_boundary_unexpected++;
                return 0;
            }
        }
        s_selftest_boundary_max_received++;
        xy_log_i("PLB-N32 AT selftest URC boundary max legal received len=232");
    } else if (id == 0 && payload_len == 7 &&
               memcmp(payload, "RECOVER", 7) == 0) {
        s_selftest_boundary_recovery_received++;
        s_selftest_urc_received = true;
        xy_log_i("PLB-N32 AT selftest URC boundary recovery received len=7");
    } else {
        s_selftest_boundary_unexpected++;
        xy_log_w("PLB-N32 AT selftest unexpected boundary URC id=%d len=%d",
                 id, payload_len);
    }
    return 0;
#endif
#if PLB_N32_AT_SELFTEST_URC_BURST
    if (id == 0 && payload_len == 4 && payload[0] == 'B') {
        unsigned int sequence = 0u;

        if (payload[1] < '0' || payload[1] > '9' ||
            payload[2] < '0' || payload[2] > '9' ||
            payload[3] < '0' || payload[3] > '9') {
            s_selftest_urc_burst_failed = true;
            xy_log_w("PLB-N32 AT selftest URC burst format error");
        } else {
            sequence = (unsigned int)(payload[1] - '0') * 100u +
                       (unsigned int)(payload[2] - '0') * 10u +
                       (unsigned int)(payload[3] - '0');
            if (s_selftest_urc_burst_received >= PLB_N32_AT_URC_BURST_COUNT) {
                s_selftest_urc_burst_failed = true;
                s_selftest_urc_burst_excess = true;
                xy_log_w("PLB-N32 AT selftest URC burst excess sequence=%u", sequence);
            } else if (sequence != s_selftest_urc_burst_received) {
                s_selftest_urc_burst_failed = true;
                xy_log_w("PLB-N32 AT selftest URC burst order expected=%u got=%u",
                         s_selftest_urc_burst_received, sequence);
            } else {
                s_selftest_urc_burst_received++;
                s_selftest_urc_deadline_ms = xy_tick_now_ms() +
                                             PLB_N32_AT_URC_INACTIVITY_MS;
                if (s_selftest_urc_burst_received == PLB_N32_AT_URC_BURST_COUNT) {
                    s_selftest_urc_settle_deadline_ms = xy_tick_now_ms() +
                                                       PLB_N32_AT_URC_SETTLE_MS;
                    s_selftest_urc_settle_rx_count = g_n32_uart5_rx_count;
                }
                if ((s_selftest_urc_burst_received % 8u) == 0u) {
                    xy_log_i("PLB-N32 AT selftest URC burst progress=%u/%u",
                             s_selftest_urc_burst_received,
                             PLB_N32_AT_URC_BURST_COUNT);
                }
            }
        }
    } else {
        s_selftest_urc_burst_failed = true;
        xy_log_w("PLB-N32 AT selftest unexpected burst URC id=%d len=%d",
                 id, payload_len);
    }
#else
#if PLB_N32_AT_SELFTEST_URC_FAKE_PROMPT
    if (id == 0 && payload_len == 1 && payload[0] == '>') {
        s_selftest_urc_received = true;
        if (s_selftest_measure_tx) {
            s_selftest_fake_prompt_urc_received = true;
            s_selftest_fake_prompt_urc_ms = xy_tick_now_ms();
            xy_log_i("PLB-N32 AT selftest fake prompt URC received");
        }
        return 0;
    }
#endif
#if PLB_N32_AT_SELFTEST_URC_FAKE_SEND_OK
    if (id == 0 && payload_len == 7 && memcmp(payload, "SEND OK", 7) == 0) {
        s_selftest_urc_received = true;
        if (s_selftest_measure_tx && s_selftest_payload_queued) {
            s_selftest_fake_send_ok_urc_received = true;
            s_selftest_fake_send_ok_urc_ms = xy_tick_now_ms();
            xy_log_i("PLB-N32 AT selftest fake SEND OK URC received");
        }
        return 0;
    }
#endif
#if PLB_N32_AT_SELFTEST_URC_PARTIAL_SEND_TAIL
    if (id == 0 && payload_len == 7 && memcmp(payload, "SEND OK", 7) == 0) {
        s_selftest_urc_received = true;
        if (s_selftest_measure_tx && s_selftest_payload_queued) {
            s_selftest_partial_send_tail_received = true;
            xy_log_i("PLB-N32 AT selftest partial SEND OK URC completed by ERROR tail");
        }
        return 0;
    }
#endif
#if PLB_N32_AT_SELFTEST_URC_FAKE_ERROR
    if (id == 0 && payload_len == 5 && memcmp(payload, "ERROR", 5) == 0) {
        s_selftest_urc_received = true;
        if (s_selftest_measure_tx && s_selftest_payload_queued) {
            s_selftest_fake_error_urc_received = true;
            s_selftest_fake_error_urc_ms = xy_tick_now_ms();
            xy_log_i("PLB-N32 AT selftest fake ERROR URC received");
        }
        return 0;
    }
#endif
#if PLB_N32_AT_SELFTEST_URC_PARTIAL_TAIL
    if (id == 0 && payload_len == 5 && memcmp(payload, "ERROR", 5) == 0) {
        s_selftest_urc_received = true;
        if (s_selftest_measure_tx && s_selftest_payload_queued) {
            s_selftest_partial_tail_received = true;
            xy_log_i("PLB-N32 AT selftest partial ERROR URC completed by response tail");
        }
        return 0;
    }
#endif
#if PLB_N32_AT_SELFTEST_URC_OVERFLOW
    if (payload_len != 5 || memcmp(payload, "HELLO", 5) != 0) {
        s_selftest_urc_overflow_unexpected = true;
        xy_log_w("PLB-N32 AT selftest unexpected overflow URC id=%d len=%d",
                 id, payload_len);
        return 0;
    }
#endif
    if (id == 0 && payload_len == 5 && memcmp(payload, "HELLO", 5) == 0) {
#if PLB_N32_AT_SELFTEST_URC_RECOVERY
        if (!s_selftest_urc_timeout_received) {
            xy_log_w("PLB-N32 AT selftest URC recovery missing timeout");
            return 0;
        }
#endif
        s_selftest_urc_received = true;
#if PLB_N32_AT_SELFTEST_URC_PROMPT
        if (s_selftest_measure_tx) {
            s_selftest_prompt_urc_received = true;
            xy_log_i("PLB-N32 AT selftest URC received before prompt");
        }
#endif
#if PLB_N32_AT_SELFTEST_URC_SEND_RESULT
        if (s_selftest_measure_tx && s_selftest_payload_queued) {
            s_selftest_send_result_urc_received = true;
            xy_log_i("PLB-N32 AT selftest URC received before SEND OK");
        }
#endif
        xy_log_i("PLB-N32 AT selftest URC recv id=%d len=%d payload=HELLO",
                 id, payload_len);
    } else {
        xy_log_w("PLB-N32 AT selftest unexpected URC id=%d len=%d",
                  id, payload_len);
    }
#endif
    return 0;
}

static const urc_item_t s_selftest_urc_table[] = {
    { "+QIURC: \"recv\"", '\n', plb_n32_at_selftest_urc },
};

#if PLB_N32_AT_SELFTEST_WORK_QUEUE
static void plb_n32_at_selftest_queue_work(at_env_t *env)
{
    plb_n32_at_queue_item_t *item =
        (plb_n32_at_queue_item_t *)env->params;

    if (item == NULL || s_selftest_queue_runs >= sizeof(s_selftest_queue_order)) {
        s_selftest_queue_failed = true;
    } else {
        s_selftest_queue_order[s_selftest_queue_runs++] = item->id;
    }
    env->finish(env, AT_RESP_OK);
}

static bool plb_n32_at_selftest_queue_batch(unsigned int item_offset)
{
    at_attr_t attr;
    unsigned int i;

    for (i = 0u; i < PLB_N32_AT_WORK_QUEUE_COUNT; i++) {
        at_attr_deinit(&attr);
        attr.params = &s_selftest_queue_items[item_offset + i];
        attr.priority = i < 4u ? AT_PRIORITY_LOW : AT_PRIORITY_HIGH;
        if (!at_custom_cmd(s_at, &attr, plb_n32_at_selftest_queue_work)) {
            return false;
        }
    }
    at_attr_deinit(&attr);
    attr.params = &s_selftest_queue_items[16];
    return !at_custom_cmd(s_at, &attr, plb_n32_at_selftest_queue_work);
}

static bool plb_n32_at_selftest_queue_order_valid(unsigned int offset)
{
    static const unsigned char expected[] = { 4u, 5u, 6u, 7u, 0u, 1u, 2u, 3u };
    unsigned int i;

    for (i = 0u; i < sizeof(expected); i++) {
        if (s_selftest_queue_order[offset + i] != expected[i] + offset) {
            return false;
        }
    }
    return true;
}

static bool plb_n32_at_selftest_abort_batch(void)
{
    at_attr_t attr;
    unsigned int i;

    for (i = 0u; i < PLB_N32_AT_WORK_QUEUE_COUNT; i++) {
        at_attr_deinit(&attr);
        at_context_init(&s_selftest_abort_contexts[i], NULL, 0u);
        at_context_attach(&attr, &s_selftest_abort_contexts[i]);
        attr.params = &s_selftest_abort_items[i];
        if (!at_custom_cmd(s_at, &attr, plb_n32_at_selftest_queue_work)) {
            return false;
        }
    }
    at_attr_deinit(&attr);
    if (at_custom_cmd(s_at, &attr, plb_n32_at_selftest_queue_work)) {
        return false;
    }
    at_work_abort_all(s_at);
    for (i = 0u; i < PLB_N32_AT_WORK_QUEUE_COUNT; i++) {
        if (at_work_get_state(&s_selftest_abort_contexts[i]) != AT_WORK_STAT_ABORT ||
            at_work_get_result(&s_selftest_abort_contexts[i]) != AT_RESP_ABORT) {
            return false;
        }
    }
    return true;
}

static void plb_n32_at_selftest_queue_process(void)
{
    at_attr_t attr;

    if (!s_selftest_active) {
        return;
    }
    if (s_selftest_queue_failed) {
        s_selftest_active = false;
        xy_log_w("PLB-N32 AT selftest FAILED reason=WORK_QUEUE_RECORD");
        return;
    }
    if (s_selftest_queue_phase == 0u) {
        if (!plb_n32_at_selftest_queue_batch(0u)) {
            s_selftest_active = false;
            xy_log_w("PLB-N32 AT selftest FAILED reason=WORK_QUEUE_FILL");
            return;
        }
        s_selftest_queue_phase = 1u;
        xy_log_i("PLB-N32 AT selftest work queue full rejection passed");
    } else if (s_selftest_queue_phase == 1u && !at_obj_busy(s_at)) {
        if (s_selftest_queue_runs != PLB_N32_AT_WORK_QUEUE_COUNT ||
            !plb_n32_at_selftest_queue_order_valid(0u)) {
            s_selftest_active = false;
            xy_log_w("PLB-N32 AT selftest FAILED reason=WORK_QUEUE_PRIORITY runs=%u",
                     s_selftest_queue_runs);
            return;
        }
        if (!plb_n32_at_selftest_abort_batch() ||
            !plb_n32_at_selftest_queue_batch(8u)) {
            s_selftest_active = false;
            xy_log_w("PLB-N32 AT selftest FAILED reason=WORK_QUEUE_ABORT_REQUEUE");
            return;
        }
        s_selftest_queue_phase = 2u;
        xy_log_i("PLB-N32 AT selftest abort all and immediate requeue passed");
    } else if (s_selftest_queue_phase == 2u && !at_obj_busy(s_at)) {
        if (s_selftest_queue_runs != 2u * PLB_N32_AT_WORK_QUEUE_COUNT ||
            !plb_n32_at_selftest_queue_order_valid(PLB_N32_AT_WORK_QUEUE_COUNT)) {
            s_selftest_active = false;
            xy_log_w("PLB-N32 AT selftest FAILED reason=WORK_QUEUE_REQUEUE_ORDER runs=%u",
                     s_selftest_queue_runs);
            return;
        }
        at_attr_deinit(&attr);
        attr.params = &s_selftest_queue_items[16];
        if (!at_custom_cmd(s_at, &attr, plb_n32_at_selftest_queue_work)) {
            s_selftest_active = false;
            xy_log_w("PLB-N32 AT selftest FAILED reason=WORK_POOL_RECYCLE");
            return;
        }
        s_selftest_queue_phase = 3u;
    } else if (s_selftest_queue_phase == 3u && !at_obj_busy(s_at)) {
        s_selftest_active = false;
        if (s_selftest_queue_runs != 17u || s_selftest_queue_order[16] != 16u ||
            !at_obj_pm_can_sleep(s_at) || g_n32_uart5_rx_drop_count != 0u) {
            xy_log_w("PLB-N32 AT selftest FAILED reason=WORK_QUEUE_FINAL runs=%u drop=%u",
                     s_selftest_queue_runs,
                     (unsigned int)g_n32_uart5_rx_drop_count);
        } else {
            xy_log_i("PLB-N32 AT selftest WORK QUEUE PASSED order=H4/L4 full=8 reject=1 abort=8 requeue=8 recycle=1 drop=0");
        }
    }
}
#endif

#if PLB_N32_AT_SELFTEST_MULTI_OBJ
static void plb_n32_at_selftest_multi_work(at_env_t *env)
{
    plb_n32_at_multi_item_t *item =
        (plb_n32_at_multi_item_t *)env->params;

    if (item == NULL || item->object >= 3u || env->obj != item->owner ||
        item->sequence != s_selftest_multi_next[item->object]) {
        s_selftest_multi_failed = true;
    } else {
        s_selftest_multi_next[item->object]++;
        s_selftest_multi_count[item->object]++;
    }
    env->finish(env, AT_RESP_OK);
}

static bool plb_n32_at_selftest_multi_enqueue(at_obj_t *at,
                                               unsigned int item_index,
                                               unsigned int object,
                                               unsigned int sequence,
                                               at_context_t *context)
{
    at_attr_t attr;
    plb_n32_at_multi_item_t *item = &s_selftest_multi_items[item_index];

    item->owner = at;
    item->object = (unsigned char)object;
    item->sequence = (unsigned char)sequence;
    at_attr_deinit(&attr);
    attr.params = item;
    if (context != NULL) {
        at_context_init(context, NULL, 0u);
        at_context_attach(&attr, context);
    }
    return at_custom_cmd(at, &attr, plb_n32_at_selftest_multi_work);
}

static void plb_n32_at_selftest_multi_process(void)
{
    unsigned int i;

    if (!s_selftest_active) {
        return;
    }
    if (s_selftest_multi_failed) {
        s_selftest_active = false;
        xy_log_w("PLB-N32 AT selftest FAILED reason=MULTI_OBJ_ISOLATION");
        return;
    }
    if (s_selftest_multi_phase == 0u) {
        bool queued = true;

        for (i = 0u; i < 3u; i++) {
            queued = queued && plb_n32_at_selftest_multi_enqueue(
                s_at, i, 0u, i, NULL);
            queued = queued && plb_n32_at_selftest_multi_enqueue(
                s_at_aux[0], 3u + i, 1u, i, &s_selftest_multi_contexts[i]);
        }
        for (i = 0u; i < 2u; i++) {
            queued = queued && plb_n32_at_selftest_multi_enqueue(
                s_at_aux[1], 6u + i, 2u, i, NULL);
        }
        if (!queued || plb_n32_at_selftest_multi_enqueue(
                           s_at, 8u, 0u, 3u, NULL) ||
            plb_n32_at_pm_can_sleep(NULL)) {
            s_selftest_active = false;
            xy_log_w("PLB-N32 AT selftest FAILED reason=MULTI_OBJ_POOL_FILL");
            return;
        }
        at_work_abort_all(s_at_aux[0]);
        for (i = 0u; i < 3u; i++) {
            if (at_work_get_state(&s_selftest_multi_contexts[i]) !=
                    AT_WORK_STAT_ABORT ||
                at_work_get_result(&s_selftest_multi_contexts[i]) !=
                    AT_RESP_ABORT) {
                s_selftest_active = false;
                xy_log_w("PLB-N32 AT selftest FAILED reason=MULTI_OBJ_ABORT_CONTEXT");
                return;
            }
        }
        s_selftest_multi_next[1] = 3u;
        for (i = 0u; i < 3u; i++) {
            if (!plb_n32_at_selftest_multi_enqueue(
                    s_at_aux[0], 8u + i, 1u, 3u + i, NULL)) {
                s_selftest_active = false;
                xy_log_w("PLB-N32 AT selftest FAILED reason=MULTI_OBJ_REQUEUE");
                return;
            }
        }
        s_selftest_multi_phase = 1u;
        xy_log_i("PLB-N32 AT selftest multi object pool competition passed total=8 reject=1 abort=B3 requeue=B3");
    } else if (s_selftest_multi_phase == 1u &&
               !at_obj_busy(s_at) &&
               !at_obj_busy(s_at_aux[0]) &&
               !at_obj_busy(s_at_aux[1])) {
        s_selftest_active = false;
        if (s_selftest_multi_count[0] != 3u ||
            s_selftest_multi_count[1] != 3u ||
            s_selftest_multi_count[2] != 2u ||
            !plb_n32_at_pm_can_sleep(NULL) ||
            g_n32_uart5_rx_drop_count != 0u) {
            xy_log_w("PLB-N32 AT selftest FAILED reason=MULTI_OBJ_FINAL count=%u/%u/%u drop=%u",
                     s_selftest_multi_count[0],
                     s_selftest_multi_count[1],
                     s_selftest_multi_count[2],
                     (unsigned int)g_n32_uart5_rx_drop_count);
        } else {
            xy_log_i("PLB-N32 AT selftest MULTI OBJECT PASSED pool=8 A=3 B=3 C=2 isolated=1 idle=3 drop=0");
        }
    }
}
#endif

#if PLB_N32_AT_SELFTEST_ERROR_CALLBACK
static const char *const s_selftest_error_commands[] = {
    "AT+SIMERR=ERROR",
    "AT+SIMERR=TIMEOUT",
    "AT+SIMBUF=256",
    "AT+SIMBUF=RECOVER",
};

static void plb_n32_at_selftest_error_hook(at_response_t *response)
{
    unsigned int expected_len = s_selftest_error_step == 0u ? 9u :
                                (s_selftest_error_step == 1u ? 0u : 255u);
    at_resp_code expected_code = s_selftest_error_step == 1u
                                 ? AT_RESP_TIMEOUT : AT_RESP_ERROR;

    s_selftest_error_hook_count++;
    if (response == NULL || response->obj != s_at ||
        response->params != &s_selftest_error_step ||
        response->code != expected_code ||
        response->recvbuf == NULL || response->recvcnt != expected_len ||
        response->prefix == NULL || response->suffix == NULL) {
        s_selftest_error_failed = true;
        return;
    }
    s_selftest_error_hook_response = *response;
}

static void plb_n32_at_selftest_error_response(at_response_t *response)
{
    at_resp_code expected_code = s_selftest_error_step == 1u
                                 ? AT_RESP_TIMEOUT :
                                 (s_selftest_error_step == 3u
                                  ? AT_RESP_OK : AT_RESP_ERROR);
    unsigned int expected_len = s_selftest_error_step == 0u ? 9u :
                                (s_selftest_error_step == 1u ? 0u :
                                 (s_selftest_error_step == 2u ? 255u : 25u));

    s_selftest_error_callback_count++;
    s_selftest_waiting = false;
    if (response == NULL || response->obj != s_at ||
        response->params != &s_selftest_error_step ||
        response->code != expected_code ||
        response->recvbuf == NULL || response->recvcnt != expected_len) {
        s_selftest_error_failed = true;
        return;
    }
    if (s_selftest_error_step < 3u &&
        (s_selftest_error_hook_count != s_selftest_error_step + 1u ||
         s_selftest_error_hook_response.obj != response->obj ||
         s_selftest_error_hook_response.params != response->params ||
         s_selftest_error_hook_response.code != response->code ||
         s_selftest_error_hook_response.recvbuf != response->recvbuf ||
         s_selftest_error_hook_response.recvcnt != response->recvcnt)) {
        s_selftest_error_failed = true;
        return;
    }
    if (s_selftest_error_step == 3u &&
        (s_selftest_error_hook_count != 3u ||
         strstr(response->recvbuf, "+RECOVER: READY") == NULL)) {
        s_selftest_error_failed = true;
        return;
    }
    xy_log_i("PLB-N32 AT selftest error callback step=%u code=%d len=%u hook=%u",
             s_selftest_error_step,
             (int)response->code,
             (unsigned int)response->recvcnt,
             s_selftest_error_hook_count);
    s_selftest_error_step++;
}

static void plb_n32_at_selftest_error_process(void)
{
    at_attr_t attr;

    if (!s_selftest_active) {
        return;
    }
    if (s_selftest_error_failed) {
        s_selftest_active = false;
        xy_log_w("PLB-N32 AT selftest FAILED reason=ERROR_CALLBACK_FIELDS step=%u hook=%u callback=%u",
                 s_selftest_error_step,
                 s_selftest_error_hook_count,
                 s_selftest_error_callback_count);
        return;
    }
    if (s_selftest_error_step == 4u && !at_obj_busy(s_at)) {
        s_selftest_active = false;
        if (s_selftest_error_hook_count != 3u ||
            s_selftest_error_callback_count != 4u ||
            !plb_n32_at_pm_can_sleep(NULL) ||
            g_n32_uart5_rx_drop_count != 0u) {
            xy_log_w("PLB-N32 AT selftest FAILED reason=ERROR_CALLBACK_FINAL hook=%u callback=%u drop=%u",
                     s_selftest_error_hook_count,
                     s_selftest_error_callback_count,
                     (unsigned int)g_n32_uart5_rx_drop_count);
        } else {
            xy_log_i("PLB-N32 AT selftest ERROR CALLBACK PASSED error=1 timeout=1 overflow=1 recover=1 hook=3 callback=4 drop=0");
        }
        return;
    }
    if (!s_selftest_waiting) {
        at_attr_deinit(&attr);
        attr.cb = plb_n32_at_selftest_error_response;
        attr.params = &s_selftest_error_step;
        attr.retry = 0u;
        attr.timeout = s_selftest_error_step == 1u ? 300u : 1000u;
        if (s_selftest_error_step == 2u) {
            attr.prefix = "+SIMBUF:";
        } else if (s_selftest_error_step == 3u) {
            attr.prefix = "+RECOVER:";
        }
        if (at_exec_cmd(s_at, &attr, "%s",
                        s_selftest_error_commands[s_selftest_error_step])) {
            s_selftest_waiting = true;
        }
    }
}
#endif

#if PLB_N32_AT_SELFTEST_DESTROY
static void plb_n32_at_selftest_destroy_hold(at_env_t *env)
{
    (void)env;
}

static void plb_n32_at_selftest_destroy_finish(at_env_t *env)
{
    unsigned char *id = (unsigned char *)env->params;

    if (id == NULL || *id != s_selftest_destroy_runs) {
        s_selftest_destroy_failed = true;
    } else {
        s_selftest_destroy_runs++;
    }
    env->finish(env, AT_RESP_OK);
}

static bool plb_n32_at_selftest_destroy_fill(
    void (*sender)(at_env_t *env), bool attach_contexts)
{
    at_attr_t attr;
    unsigned int i;

    for (i = 0u; i < 8u; i++) {
        at_attr_deinit(&attr);
        attr.params = &s_selftest_destroy_ids[i];
        if (attach_contexts) {
            at_context_init(&s_selftest_destroy_contexts[i], NULL, 0u);
            at_context_attach(&attr, &s_selftest_destroy_contexts[i]);
        }
        if (!at_custom_cmd(s_at, &attr, sender)) {
            return false;
        }
    }
    return !at_custom_cmd(s_at, NULL, sender);
}

static void plb_n32_at_selftest_destroy_process(void)
{
    unsigned int i;

    if (!s_selftest_active) {
        return;
    }
    if (s_selftest_destroy_failed) {
        s_selftest_active = false;
        xy_log_w("PLB-N32 AT selftest FAILED reason=DESTROY_RECREATE_ORDER runs=%u",
                 s_selftest_destroy_runs);
        return;
    }
    if (s_selftest_destroy_phase == 0u) {
        if (!plb_n32_at_selftest_destroy_fill(
                plb_n32_at_selftest_destroy_hold, true)) {
            s_selftest_active = false;
            xy_log_w("PLB-N32 AT selftest FAILED reason=DESTROY_FILL");
            return;
        }
        s_selftest_destroy_object_mem = at_cur_used_memory();
        s_selftest_destroy_phase = 1u;
    } else if (s_selftest_destroy_phase == 1u) {
        if (at_work_get_state(&s_selftest_destroy_contexts[0]) !=
                AT_WORK_STAT_RUN) {
            s_selftest_active = false;
            xy_log_w("PLB-N32 AT selftest FAILED reason=DESTROY_ACTIVE_MISSING state=%d",
                     (int)at_work_get_state(&s_selftest_destroy_contexts[0]));
            return;
        }
        at_obj_destroy(s_at);
        s_at = NULL;
        for (i = 0u; i < 8u; i++) {
            if (at_work_get_state(&s_selftest_destroy_contexts[i]) !=
                    AT_WORK_STAT_ABORT ||
                at_work_get_result(&s_selftest_destroy_contexts[i]) !=
                    AT_RESP_ABORT) {
                s_selftest_active = false;
                xy_log_w("PLB-N32 AT selftest FAILED reason=DESTROY_CONTEXT index=%u state=%d code=%d",
                         i,
                         (int)at_work_get_state(&s_selftest_destroy_contexts[i]),
                         (int)at_work_get_result(&s_selftest_destroy_contexts[i]));
                return;
            }
        }
        if (at_cur_used_memory() != 0u) {
            s_selftest_active = false;
            xy_log_w("PLB-N32 AT selftest FAILED reason=DESTROY_MEMORY current=%u",
                     at_cur_used_memory());
            return;
        }
        s_at = at_obj_create(&s_adapter);
        if (s_at == NULL || at_cur_used_memory() != s_selftest_destroy_object_mem ||
            !plb_n32_at_selftest_destroy_fill(
                plb_n32_at_selftest_destroy_finish, false)) {
            s_selftest_active = false;
            xy_log_w("PLB-N32 AT selftest FAILED reason=DESTROY_RECREATE mem=%u expected=%u",
                     at_cur_used_memory(), s_selftest_destroy_object_mem);
            return;
        }
        s_selftest_destroy_phase = 2u;
        xy_log_i("PLB-N32 AT selftest destroy active=1 queued=7 contexts=8 memory=0 recreate=1 pool=8");
    } else if (s_selftest_destroy_phase == 2u && !at_obj_busy(s_at)) {
        s_selftest_active = false;
        if (s_selftest_destroy_runs != 8u ||
            !plb_n32_at_pm_can_sleep(NULL) ||
            g_n32_uart5_rx_drop_count != 0u) {
            xy_log_w("PLB-N32 AT selftest FAILED reason=DESTROY_FINAL runs=%u drop=%u",
                     s_selftest_destroy_runs,
                     (unsigned int)g_n32_uart5_rx_drop_count);
        } else {
            xy_log_i("PLB-N32 AT selftest DESTROY RECREATE PASSED abort=8 recreate=1 refill=8 runs=8 drop=0");
        }
    }
}
#endif

#if PLB_N32_AT_SELFTEST_TICK_WRAP
static int plb_n32_at_selftest_wrap_urc(at_urc_info_t *info)
{
    const char *payload;
    int id;
    int payload_len;
    int rc;

    if (info->status == URC_RECV_TIMEOUT) {
        s_selftest_wrap_urc_timeouts++;
        return 0;
    }
    rc = at_urc_recv_split(info, plb_n32_at_parse_qirecv, 2,
                           &id, &payload, &payload_len);
    if (rc != 0) {
        return rc > 0 ? rc : 0;
    }
    if (id == 0 && payload_len == 7 &&
        memcmp(payload, "RECOVER", 7) == 0) {
        s_selftest_wrap_recoveries++;
    } else {
        s_selftest_wrap_failed = true;
    }
    return 0;
}

static const urc_item_t s_selftest_wrap_urc_table[] = {
    { "+QIURC: \"recv\"", '\n', plb_n32_at_selftest_wrap_urc },
};

static int plb_n32_at_selftest_wrap_wait(at_env_t *env)
{
    if (env->state == 0) {
        env->next_wait(env, 300u);
        env->state = 1;
        return 0;
    }
    s_selftest_wrap_wait_runs++;
    env->finish(env, AT_RESP_OK);
    return 0;
}

static void plb_n32_at_selftest_wrap_response(at_response_t *response)
{
    s_selftest_waiting = false;
    s_selftest_wrap_timeout_callbacks++;
    if (response == NULL || response->code != AT_RESP_TIMEOUT ||
        response->recvcnt != 0u) {
        s_selftest_wrap_failed = true;
    }
}

static void plb_n32_at_selftest_wrap_marker(at_response_t *response)
{
    s_selftest_waiting = false;
    if (response == NULL || response->code != AT_RESP_OK) {
        s_selftest_wrap_failed = true;
    } else {
        s_selftest_wrap_phase = 5u;
    }
}

static void plb_n32_at_selftest_wrap_process(void)
{
    at_attr_t attr;

    if (!s_selftest_active) {
        return;
    }
    if (s_selftest_wrap_failed) {
        xy_tick_set(s_selftest_wrap_original_tick + 2000u);
        s_selftest_active = false;
        xy_log_w("PLB-N32 AT selftest FAILED reason=TICK_WRAP phase=%u wait=%u timeout=%u urc=%u recover=%u",
                 s_selftest_wrap_phase,
                 s_selftest_wrap_wait_runs,
                 s_selftest_wrap_timeout_callbacks,
                 s_selftest_wrap_urc_timeouts,
                 s_selftest_wrap_recoveries);
        return;
    }
    if (s_selftest_wrap_phase == 0u) {
        xy_tick_set(0xffffff00u);
        if (!at_do_work(s_at, NULL, plb_n32_at_selftest_wrap_wait)) {
            s_selftest_wrap_failed = true;
            return;
        }
        s_selftest_wrap_phase = 1u;
    } else if (s_selftest_wrap_phase == 1u && !at_obj_busy(s_at)) {
        if (s_selftest_wrap_wait_runs != 1u || xy_tick_now() >= 0xffffff00u) {
            s_selftest_wrap_failed = true;
            return;
        }
        xy_log_i("PLB-N32 AT selftest tick wrap next_wait passed now=%u",
                 (unsigned int)xy_tick_now());
        xy_tick_set(0xffffff00u);
        at_attr_deinit(&attr);
        attr.cb = plb_n32_at_selftest_wrap_response;
        attr.timeout = 300u;
        attr.retry = 1u;
        if (!at_exec_cmd(s_at, &attr, "AT+SIMWRAP=TIMEOUT")) {
            s_selftest_wrap_failed = true;
            return;
        }
        s_selftest_waiting = true;
        s_selftest_wrap_phase = 2u;
    } else if (s_selftest_wrap_phase == 2u &&
               !s_selftest_waiting && !at_obj_busy(s_at)) {
        if (s_selftest_wrap_timeout_callbacks != 1u ||
            xy_tick_now() >= 0xffffff00u) {
            s_selftest_wrap_failed = true;
            return;
        }
        xy_log_i("PLB-N32 AT selftest tick wrap timeout retry passed callbacks=1 now=%u",
                 (unsigned int)xy_tick_now());
        xy_tick_set(0xffffff00u);
        at_obj_set_urc(s_at, s_selftest_wrap_urc_table, 1);
        at_attr_deinit(&attr);
        attr.cb = plb_n32_at_selftest_wrap_marker;
        attr.timeout = 1000u;
        attr.retry = 0u;
        if (!at_exec_cmd(s_at, &attr, "AT+SIMWRAP=URC")) {
            s_selftest_wrap_failed = true;
            return;
        }
        s_selftest_waiting = true;
        s_selftest_wrap_phase = 3u;
    } else if (s_selftest_wrap_phase == 5u &&
               s_selftest_wrap_urc_timeouts == 1u &&
               s_selftest_wrap_recoveries == 1u &&
               !at_obj_busy(s_at)) {
        xy_tick_set(s_selftest_wrap_original_tick + 2000u);
        s_selftest_active = false;
        if (!plb_n32_at_pm_can_sleep(NULL) ||
            g_n32_uart5_rx_drop_count != 0u) {
            xy_log_w("PLB-N32 AT selftest FAILED reason=TICK_WRAP_FINAL drop=%u",
                     (unsigned int)g_n32_uart5_rx_drop_count);
        } else {
            xy_log_i("PLB-N32 AT selftest TICK WRAP PASSED next_wait=1 retry=2 timeout=1 urc_timeout=1 recover=1 drop=0");
        }
    }
}
#endif

#if PLB_N32_AT_SELFTEST_URC_PREFIX
static int plb_n32_at_selftest_prefix_short(at_urc_info_t *info)
{
    if (info == NULL || info->status != URC_RECV_OK ||
        strstr(info->urcbuf, "+SIM: SHORT") == NULL) {
        s_selftest_prefix_unexpected++;
    } else {
        s_selftest_prefix_short++;
    }
    return 0;
}

static int plb_n32_at_selftest_prefix_long(at_urc_info_t *info)
{
    if (info == NULL || info->status != URC_RECV_OK) {
        s_selftest_prefix_unexpected++;
    } else if (strstr(info->urcbuf, "+SIM: LONG EMBED +SIM: SHORT") != NULL) {
        s_selftest_prefix_embedded++;
        s_selftest_prefix_long++;
    } else if (strstr(info->urcbuf, "+SIM: LONG RECOVER") != NULL) {
        s_selftest_prefix_recovery++;
        s_selftest_prefix_long++;
    } else if (strstr(info->urcbuf, "+SIM: LONG FIRST") != NULL) {
        s_selftest_prefix_long++;
    } else {
        s_selftest_prefix_unexpected++;
    }
    return 0;
}

static const urc_item_t s_selftest_prefix_urc_table[] = {
    { "+SIM:", '\n', plb_n32_at_selftest_prefix_short },
    { "+SIM: LONG", '\n', plb_n32_at_selftest_prefix_long },
};

static void plb_n32_at_selftest_prefix_marker(at_response_t *response)
{
    s_selftest_waiting = false;
    if (response == NULL || response->code != AT_RESP_OK) {
        s_selftest_prefix_unexpected++;
    } else {
        s_selftest_prefix_marker_done = true;
    }
}

static void plb_n32_at_selftest_prefix_process(void)
{
    at_attr_t attr;

    if (!s_selftest_active) {
        return;
    }
    if (s_selftest_prefix_unexpected != 0u) {
        s_selftest_active = false;
        xy_log_w("PLB-N32 AT selftest FAILED reason=URC_PREFIX_DISPATCH short=%u long=%u embedded=%u recovery=%u unexpected=%u",
                 s_selftest_prefix_short,
                 s_selftest_prefix_long,
                 s_selftest_prefix_embedded,
                 s_selftest_prefix_recovery,
                 s_selftest_prefix_unexpected);
        return;
    }
    if (!s_selftest_prefix_command_sent) {
        at_obj_set_urc(s_at, s_selftest_prefix_urc_table,
                       sizeof(s_selftest_prefix_urc_table) /
                       sizeof(s_selftest_prefix_urc_table[0]));
        at_attr_deinit(&attr);
        attr.cb = plb_n32_at_selftest_prefix_marker;
        attr.timeout = 1000u;
        attr.retry = 1u;
        if (at_exec_cmd(s_at, &attr, "AT+SIMURC=PREFIX")) {
            s_selftest_waiting = true;
            s_selftest_prefix_command_sent = true;
        }
    } else if (s_selftest_prefix_marker_done && !s_selftest_waiting &&
               !at_obj_busy(s_at) &&
               s_selftest_prefix_long == 3u &&
               s_selftest_prefix_short == 1u &&
               s_selftest_prefix_embedded == 1u &&
               s_selftest_prefix_recovery == 1u) {
        s_selftest_active = false;
        if (g_n32_uart5_rx_drop_count != 0u ||
            !plb_n32_at_pm_can_sleep(NULL)) {
            xy_log_w("PLB-N32 AT selftest FAILED reason=URC_PREFIX_FINAL drop=%u",
                     (unsigned int)g_n32_uart5_rx_drop_count);
        } else {
            xy_log_i("PLB-N32 AT selftest URC PREFIX PASSED long=3 short=1 embedded=1 malformed=0 recovery=1 drop=0");
        }
    }
}
#endif

static int plb_n32_at_selftest_send_work(at_env_t *env)
{
    int rc;
#if PLB_N32_AT_SELFTEST_URC_SEND_RESULT || \
    PLB_N32_AT_SELFTEST_URC_FAKE_PROMPT || \
    PLB_N32_AT_SELFTEST_URC_FAKE_SEND_OK || \
    PLB_N32_AT_SELFTEST_URC_FAKE_ERROR || \
    PLB_N32_AT_SELFTEST_URC_PARTIAL_ERROR || \
    PLB_N32_AT_SELFTEST_URC_PARTIAL_TAIL || \
    PLB_N32_AT_SELFTEST_URC_PARTIAL_SEND_OK || \
    PLB_N32_AT_SELFTEST_URC_PARTIAL_SEND_TAIL
    int previous_state = env->state;
#endif
    unsigned int send_len = (unsigned int)sizeof(s_selftest_payload) +
                            (PLB_N32_AT_SELFTEST_SEGMENTED ? 1u : 0u);

    if (env->state == 0) {
        env->println(env, "AT+QISEND=0,%u",
                     send_len);
        env->reset_timer(env);
        env->state = 1;
        return 0;
    }
#if PLB_N32_AT_SELFTEST_ABORT
    if (env->state == 1) {
        if (env->contains(env, ">")) {
            s_selftest_abort_armed = true;
            if (env->write == NULL ||
                !env->write(env, s_selftest_payload,
                            (unsigned int)sizeof(s_selftest_payload))) {
                s_selftest_active = false;
                s_selftest_abort_armed = false;
                s_selftest_measure_tx = false;
                xy_log_w("PLB-N32 AT selftest FAILED reason=ABORT_QUEUE");
                env->finish(env, AT_RESP_ERROR);
                return 0;
            }
            env->recvclr(env);
            env->reset_timer(env);
            env->state = 2;
        } else if (env->is_timeout(env, 5000u)) {
            s_selftest_active = false;
            s_selftest_measure_tx = false;
            xy_log_w("PLB-N32 AT selftest FAILED command=AT+QISEND");
            env->finish(env, AT_RESP_ERROR);
        }
        return 0;
    }
    return 0;
#endif
#if PLB_N32_AT_SELFTEST_SEGMENTED
    if (env->state == 1) {
        if (env->contains(env, ">")) {
            if (env->write == NULL ||
                !env->write(env, s_selftest_payload,
                            (unsigned int)sizeof(s_selftest_payload)) ||
                !env->write(env, &s_selftest_ctrl_z, 1u)) {
                s_selftest_active = false;
                s_selftest_measure_tx = false;
                xy_log_w("PLB-N32 AT selftest FAILED reason=SEGMENT_QUEUE");
                env->finish(env, AT_RESP_ERROR);
                return 0;
            }
            env->recvclr(env);
            env->reset_timer(env);
            env->state = 2;
        } else if (env->is_timeout(env, 5000u)) {
            s_selftest_active = false;
            s_selftest_measure_tx = false;
            xy_log_w("PLB-N32 AT selftest FAILED command=AT+QISEND");
            env->finish(env, AT_RESP_ERROR);
        }
        return 0;
    }
#endif
    rc = at_prompt_send_step(env, s_selftest_payload,
                             sizeof(s_selftest_payload),
                             "SEND OK", "OK", NULL, 5000u, 10000u);
#if PLB_N32_AT_SELFTEST_URC_SEND_RESULT || \
    PLB_N32_AT_SELFTEST_URC_FAKE_SEND_OK || \
    PLB_N32_AT_SELFTEST_URC_FAKE_ERROR || \
    PLB_N32_AT_SELFTEST_URC_PARTIAL_ERROR || \
    PLB_N32_AT_SELFTEST_URC_PARTIAL_TAIL || \
    PLB_N32_AT_SELFTEST_URC_PARTIAL_SEND_OK || \
    PLB_N32_AT_SELFTEST_URC_PARTIAL_SEND_TAIL
    if (previous_state == 1 && env->state == 2) {
        s_selftest_payload_queued = true;
    }
#endif
#if PLB_N32_AT_SELFTEST_URC_FAKE_PROMPT
    if (env->state == 2 && !s_selftest_fake_prompt_urc_received) {
        s_selftest_active = false;
        s_selftest_measure_tx = false;
        xy_log_w("PLB-N32 AT selftest FAILED reason=FAKE_PROMPT_URC_MISSING");
        env->finish(env, AT_RESP_ERROR);
        return 0;
    }
    if (previous_state == 1 && env->state == 2 &&
        (uint32_t)(xy_tick_now_ms() - s_selftest_fake_prompt_urc_ms) < 500u) {
        s_selftest_active = false;
        s_selftest_measure_tx = false;
        xy_log_w("PLB-N32 AT selftest FAILED reason=URC_FAKE_PROMPT_ACCEPTED");
        env->finish(env, AT_RESP_ERROR);
        return 0;
    }
#endif
    if (rc > 0) {
#if PLB_N32_AT_SELFTEST_URC_PARTIAL_SEND_OK
        s_selftest_active = false;
        s_selftest_measure_tx = false;
        xy_log_w("PLB-N32 AT selftest FAILED reason=PARTIAL_SEND_OK_ACCEPTED");
        env->finish(env, AT_RESP_ERROR);
        return 0;
#endif
#if PLB_N32_AT_SELFTEST_TX_ZERO_STALL
        if (!s_selftest_cmd_stall_done || !s_selftest_payload_stall_done ||
            !s_selftest_stall_pm_blocked || s_selftest_tx_zero_writes == 0u ||
            (uint32_t)(xy_tick_now_ms() - s_selftest_cmd_stall_ms) <
                PLB_N32_AT_TX_ZERO_STALL_MS ||
            (uint32_t)(xy_tick_now_ms() - s_selftest_payload_stall_ms) <
                PLB_N32_AT_TX_ZERO_STALL_MS) {
            s_selftest_active = false;
            s_selftest_measure_tx = false;
            xy_log_w("PLB-N32 AT selftest FAILED reason=TX_ZERO_STALL_INCOMPLETE");
            env->finish(env, AT_RESP_ERROR);
            return 0;
        }
#endif
#if PLB_N32_AT_SELFTEST_URC_PARTIAL_SEND_TAIL
        s_selftest_active = false;
        s_selftest_measure_tx = false;
        xy_log_w("PLB-N32 AT selftest FAILED reason=PARTIAL_SEND_TAIL_ACCEPTED");
        env->finish(env, AT_RESP_ERROR);
        return 0;
#endif
#if PLB_N32_AT_SELFTEST_URC_PROMPT
        if (!s_selftest_prompt_urc_received) {
            s_selftest_active = false;
            s_selftest_measure_tx = false;
            xy_log_w("PLB-N32 AT selftest FAILED reason=PROMPT_URC_MISSING");
            env->finish(env, AT_RESP_ERROR);
            return 0;
        }
#endif
#if PLB_N32_AT_SELFTEST_URC_SEND_RESULT
        if (!s_selftest_send_result_urc_received) {
            s_selftest_active = false;
            s_selftest_measure_tx = false;
            xy_log_w("PLB-N32 AT selftest FAILED reason=SEND_RESULT_URC_MISSING");
            env->finish(env, AT_RESP_ERROR);
            return 0;
        }
#endif
#if PLB_N32_AT_SELFTEST_URC_FAKE_SEND_OK
        if (!s_selftest_fake_send_ok_urc_received) {
            s_selftest_active = false;
            s_selftest_measure_tx = false;
            xy_log_w("PLB-N32 AT selftest FAILED reason=FAKE_SEND_OK_URC_MISSING");
            env->finish(env, AT_RESP_ERROR);
            return 0;
        }
        if ((uint32_t)(xy_tick_now_ms() - s_selftest_fake_send_ok_urc_ms) < 500u) {
            s_selftest_active = false;
            s_selftest_measure_tx = false;
            xy_log_w("PLB-N32 AT selftest FAILED reason=URC_FAKE_SEND_OK_ACCEPTED");
            env->finish(env, AT_RESP_ERROR);
            return 0;
        }
#endif
#if PLB_N32_AT_SELFTEST_URC_FAKE_ERROR
        if (!s_selftest_fake_error_urc_received) {
            s_selftest_active = false;
            s_selftest_measure_tx = false;
            xy_log_w("PLB-N32 AT selftest FAILED reason=FAKE_ERROR_URC_MISSING");
            env->finish(env, AT_RESP_ERROR);
            return 0;
        }
        if ((uint32_t)(xy_tick_now_ms() - s_selftest_fake_error_urc_ms) < 500u) {
            s_selftest_active = false;
            s_selftest_measure_tx = false;
            xy_log_w("PLB-N32 AT selftest FAILED reason=URC_FAKE_ERROR_ACCEPTED");
            env->finish(env, AT_RESP_ERROR);
            return 0;
        }
#endif
#if PLB_N32_AT_SELFTEST_URC_PARTIAL_ERROR
        if (!s_selftest_partial_error_timeout_received) {
            s_selftest_active = false;
            s_selftest_measure_tx = false;
            xy_log_w("PLB-N32 AT selftest FAILED reason=PARTIAL_ERROR_TIMEOUT_MISSING");
            env->finish(env, AT_RESP_ERROR);
            return 0;
        }
        if ((uint32_t)(xy_tick_now_ms() - s_selftest_partial_error_timeout_ms) < 100u) {
            s_selftest_active = false;
            s_selftest_measure_tx = false;
            xy_log_w("PLB-N32 AT selftest FAILED reason=PARTIAL_ERROR_EARLY_RESULT");
            env->finish(env, AT_RESP_ERROR);
            return 0;
        }
#endif
#if PLB_N32_AT_SELFTEST_URC_PARTIAL_TAIL
        if (!s_selftest_partial_tail_received || s_selftest_partial_tail_timeout) {
            s_selftest_active = false;
            s_selftest_measure_tx = false;
            xy_log_w("PLB-N32 AT selftest FAILED reason=PARTIAL_TAIL_INCOMPLETE");
            env->finish(env, AT_RESP_ERROR);
            return 0;
        }
        if ((uint32_t)(xy_tick_now_ms() - s_selftest_partial_tail_start_ms) < 200u) {
            s_selftest_active = false;
            s_selftest_measure_tx = false;
            xy_log_w("PLB-N32 AT selftest FAILED reason=PARTIAL_TAIL_EARLY_RESULT");
            env->finish(env, AT_RESP_ERROR);
            return 0;
        }
#endif
        if (sizeof(s_selftest_payload) > 512u &&
            s_selftest_tx_short_writes == 0u) {
            s_selftest_active = false;
            s_selftest_measure_tx = false;
            xy_log_w("PLB-N32 AT selftest FAILED reason=NO_SHORT_WRITE");
            env->finish(env, AT_RESP_ERROR);
            return 0;
        }
        xy_log_i("PLB-N32 AT selftest QISEND PASSED len=%u segments=%u",
                 send_len,
                 PLB_N32_AT_SELFTEST_SEGMENTED ? 2u : 1u);
        xy_log_i("PLB-N32 AT selftest TX calls=%u short=%u zero=%u",
                 s_selftest_tx_calls,
                 s_selftest_tx_short_writes,
                 s_selftest_tx_zero_writes);
        s_selftest_step++;
        s_selftest_waiting = false;
        env->finish(env, AT_RESP_OK);
    } else if (rc < 0) {
#if PLB_N32_AT_SELFTEST_URC_PARTIAL_SEND_OK
        if (!s_selftest_partial_send_ok_timeout_received) {
            s_selftest_active = false;
            s_selftest_measure_tx = false;
            xy_log_w("PLB-N32 AT selftest FAILED reason=PARTIAL_SEND_OK_TIMEOUT_MISSING");
            env->finish(env, AT_RESP_ERROR);
            return 0;
        }
        if ((uint32_t)(xy_tick_now_ms() - s_selftest_partial_send_ok_timeout_ms) < 100u) {
            s_selftest_active = false;
            s_selftest_measure_tx = false;
            xy_log_w("PLB-N32 AT selftest FAILED reason=PARTIAL_SEND_OK_EARLY_ERROR");
            env->finish(env, AT_RESP_ERROR);
            return 0;
        }
        s_selftest_partial_send_ok_real_error_received = true;
        s_selftest_step++;
        s_selftest_waiting = false;
        xy_log_i("PLB-N32 AT selftest QISEND expected ERROR received");
        env->finish(env, AT_RESP_ERROR);
        return 0;
#endif
#if PLB_N32_AT_SELFTEST_URC_PARTIAL_SEND_TAIL
        if (!s_selftest_partial_send_tail_received ||
            s_selftest_partial_send_tail_timeout) {
            s_selftest_active = false;
            s_selftest_measure_tx = false;
            xy_log_w("PLB-N32 AT selftest FAILED reason=PARTIAL_SEND_TAIL_INCOMPLETE");
            env->finish(env, AT_RESP_ERROR);
            return 0;
        }
        if ((uint32_t)(xy_tick_now_ms() - s_selftest_partial_send_tail_start_ms) < 200u) {
            s_selftest_active = false;
            s_selftest_measure_tx = false;
            xy_log_w("PLB-N32 AT selftest FAILED reason=PARTIAL_SEND_TAIL_EARLY_ERROR");
            env->finish(env, AT_RESP_ERROR);
            return 0;
        }
        s_selftest_partial_send_tail_real_error = true;
        s_selftest_step++;
        s_selftest_waiting = false;
        xy_log_i("PLB-N32 AT selftest QISEND expected ERROR received after URC tail");
        env->finish(env, AT_RESP_ERROR);
        return 0;
#endif
        s_selftest_active = false;
        s_selftest_measure_tx = false;
#if PLB_N32_AT_SELFTEST_URC_FAKE_ERROR
        if (s_selftest_fake_error_urc_received &&
            (uint32_t)(xy_tick_now_ms() - s_selftest_fake_error_urc_ms) < 500u) {
            xy_log_w("PLB-N32 AT selftest FAILED reason=URC_FAKE_ERROR_ACCEPTED");
        } else
#endif
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
                           :
#if PLB_N32_AT_SELFTEST_RESP_BOUNDARY
                             s_selftest_boundary_commands[
                                 s_selftest_step - command_count - 1u];
#elif PLB_N32_AT_SELFTEST_LATE_RESPONSE
                             s_selftest_step == command_count + 1u
                             ? "AT+SIMLATEA" : "AT+SIMLATEB";
#elif PLB_N32_AT_SELFTEST_URC_BURST
                             "AT+SIMURC=BURST";
#elif PLB_N32_AT_SELFTEST_URC_RECOVERY
                             "AT+SIMURC=RECOVER";
#elif PLB_N32_AT_SELFTEST_URC_OVERFLOW
                             "AT+SIMURC=OVERFLOW";
#elif PLB_N32_AT_SELFTEST_URC_INTERLEAVE
                             "AT+SIMURC=INTERLEAVE";
#elif PLB_N32_AT_SELFTEST_URC_PROMPT
                             "AT+SIMURC=PROMPT";
#elif PLB_N32_AT_SELFTEST_URC_SEND_RESULT
                             "AT+SIMURC=SENDRESULT";
#elif PLB_N32_AT_SELFTEST_URC_FAKE_PROMPT
                             "AT+SIMURC=FAKEPROMPT";
#elif PLB_N32_AT_SELFTEST_URC_FAKE_SEND_OK
                             "AT+SIMURC=FAKESENDOK";
#elif PLB_N32_AT_SELFTEST_URC_FAKE_ERROR
                             "AT+SIMURC=FAKEERROR";
#elif PLB_N32_AT_SELFTEST_URC_PARTIAL_ERROR
                             "AT+SIMURC=PARTIALERROR";
#elif PLB_N32_AT_SELFTEST_URC_PARTIAL_TAIL
                             "AT+SIMURC=PARTIALTAIL";
#elif PLB_N32_AT_SELFTEST_URC_PARTIAL_SEND_OK
                             "AT+SIMURC=PARTIALSENDOK";
#elif PLB_N32_AT_SELFTEST_URC_PARTIAL_SEND_TAIL
                             "AT+SIMURC=PARTIALSENDTAIL";
#elif PLB_N32_AT_SELFTEST_URC_BOUNDARY
                             "AT+SIMURC=BOUNDARY";
#else
                             "AT+SIMURC=RECV";
#endif

    s_selftest_waiting = false;
    xy_log_i("PLB-N32 AT selftest command=%s code=%d len=%u",
             command,
             (int)response->code,
             (unsigned int)response->recvcnt);
#if PLB_N32_AT_SELFTEST_RESP_BOUNDARY
    if (s_selftest_step > command_count) {
        unsigned int index = s_selftest_step - command_count - 1u;
        at_resp_code expected = (index == 1u || index == 3u)
                                ? AT_RESP_ERROR : AT_RESP_OK;
        unsigned int expected_len = (index == 0u) ? 255u :
                                    (expected == AT_RESP_ERROR ? 255u : 25u);

        if (response->code != expected || response->recvcnt != expected_len) {
            s_selftest_active = false;
            xy_log_w("PLB-N32 AT selftest FAILED reason=RESP_BOUNDARY index=%u code=%d len=%u",
                     index, (int)response->code, (unsigned int)response->recvcnt);
            return;
        }
        if (expected == AT_RESP_OK &&
            strstr(response->recvbuf, index == 0u ? "+SIMBUF: 255" :
                                                   "+RECOVER: READY") == NULL) {
            s_selftest_active = false;
            xy_log_w("PLB-N32 AT selftest FAILED reason=RESP_BOUNDARY_CONTENT index=%u",
                     index);
            return;
        }
        s_selftest_step++;
        if (index + 1u == sizeof(s_selftest_boundary_commands) /
                          sizeof(s_selftest_boundary_commands[0])) {
            s_selftest_active = false;
            if (g_n32_uart5_rx_drop_count == 0u) {
                xy_log_i("PLB-N32 AT selftest RESPONSE BOUNDARY PASSED ok=255 overflow=256/257 recover=2 drop=0");
            } else {
                xy_log_w("PLB-N32 AT selftest FAILED reason=RESP_BOUNDARY_DROP drop=%u",
                         (unsigned int)g_n32_uart5_rx_drop_count);
            }
        }
        return;
    }
#endif
    if (response->code != AT_RESP_OK) {
        s_selftest_active = false;
        xy_log_w("PLB-N32 AT selftest FAILED command=%s", command);
        return;
    }
#if PLB_N32_AT_SELFTEST_LATE_RESPONSE
    if (s_selftest_step == command_count + 1u) {
        if (strstr(response->recvbuf, "+SIMLATEA: FIRST") == NULL) {
            s_selftest_active = false;
            xy_log_w("PLB-N32 AT selftest FAILED reason=LATE_A_RESPONSE_MISSING");
            return;
        }
        s_selftest_late_a_received = true;
        s_selftest_step++;
        return;
    }
    if (s_selftest_step == command_count + 2u) {
        s_selftest_active = false;
        if (!s_selftest_late_a_received ||
            strstr(response->recvbuf, "+SIMLATEA: SECOND") == NULL ||
            strstr(response->recvbuf, "+SIMLATEB: CURRENT") == NULL) {
            xy_log_w("PLB-N32 AT selftest FAILED reason=LATE_RESPONSE_ISOLATION");
        } else if (g_n32_uart5_rx_drop_count != 0u) {
            xy_log_w("PLB-N32 AT selftest FAILED reason=LATE_RESPONSE_DROP drop=%u",
                     (unsigned int)g_n32_uart5_rx_drop_count);
        } else {
            xy_log_i("PLB-N32 AT selftest LATE RESPONSE PASSED attempts=2 callbacks=2 drop=0");
        }
        return;
    }
#endif
#if PLB_N32_AT_SELFTEST_URC_INTERLEAVE
    if (s_selftest_step == 1u) {
        if (strstr(response->recvbuf, "+CSQ: 18,0") == NULL) {
            s_selftest_active = false;
            xy_log_w("PLB-N32 AT selftest FAILED reason=INTERLEAVE_CSQ_MISSING");
            return;
        }
        if (!s_selftest_urc_received) {
            s_selftest_active = false;
            xy_log_w("PLB-N32 AT selftest FAILED reason=INTERLEAVE_URC_LATE");
            return;
        }
        s_selftest_interleave_csq_valid = true;
        xy_log_i("PLB-N32 AT selftest interleave CSQ and URC received");
    }
#endif

    s_selftest_step++;
    if (s_selftest_step == command_count + 2u) {
        s_selftest_urc_deadline_ms = xy_tick_now_ms() +
                                     PLB_N32_AT_URC_INACTIVITY_MS;
    }
}
#endif

static unsigned int plb_n32_at_write(const void *buf, unsigned int len)
{
    unsigned int written;

#if PLB_N32_AT_SELFTEST_TX_ZERO_STALL
    if (s_selftest_measure_tx && len > 0u) {
        uint32_t now = xy_tick_now_ms();
        const unsigned char *data = (const unsigned char *)buf;

        if (!s_selftest_cmd_stall_done && data[0] == 'A') {
            if (!s_selftest_cmd_stall_started) {
                s_selftest_cmd_stall_started = true;
                s_selftest_cmd_stall_ms = now;
                xy_log_i("PLB-N32 AT selftest TX command zero stall start");
            }
            if ((uint32_t)(now - s_selftest_cmd_stall_ms) <
                PLB_N32_AT_TX_ZERO_STALL_MS) {
                written = 0u;
                goto measured;
            }
            s_selftest_cmd_stall_done = true;
            xy_log_i("PLB-N32 AT selftest TX command zero stall recovered elapsed=%u",
                     (unsigned int)(now - s_selftest_cmd_stall_ms));
        } else if (s_selftest_cmd_stall_done &&
                   !s_selftest_payload_stall_done && data[0] == 'P') {
            if (!s_selftest_payload_stall_started) {
                s_selftest_payload_stall_started = true;
                s_selftest_payload_stall_ms = now;
                xy_log_i("PLB-N32 AT selftest TX payload zero stall start");
            }
            if ((uint32_t)(now - s_selftest_payload_stall_ms) <
                PLB_N32_AT_TX_ZERO_STALL_MS) {
                written = 0u;
                goto measured;
            }
            s_selftest_payload_stall_done = true;
            xy_log_i("PLB-N32 AT selftest TX payload zero stall recovered elapsed=%u",
                     (unsigned int)(now - s_selftest_payload_stall_ms));
        }
    }
#endif
    written = n32_uart5_write_nonblock(buf, len);

#if PLB_N32_ENABLE_AT_SELFTEST
#if PLB_N32_AT_SELFTEST_TX_ZERO_STALL
measured:
#endif
    if (s_selftest_measure_tx) {
        s_selftest_tx_calls++;
        if (written < len) {
            s_selftest_tx_short_writes++;
        }
        if (written == 0u && len != 0u) {
            s_selftest_tx_zero_writes++;
        }
#if PLB_N32_AT_SELFTEST_ABORT
        if (s_selftest_abort_armed) {
            s_selftest_abort_accepted += written;
            if (written < len) {
                s_selftest_abort_requested = true;
                s_selftest_abort_armed = false;
            }
        }
#endif
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
#if PLB_N32_AT_SELFTEST_ERROR_CALLBACK
    .error = plb_n32_at_selftest_error_hook,
#else
    .error = NULL,
#endif
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
#if PLB_N32_AT_SELFTEST_MULTI_OBJ
    if (s_at != NULL) {
        s_at_aux[0] = at_obj_create(&s_adapter);
        s_at_aux[1] = at_obj_create(&s_adapter);
        if (s_at_aux[0] == NULL || s_at_aux[1] == NULL) {
            xy_log_w("PLB-N32 AT multi object create failed A=%u B=%u C=%u mem=%u/%u",
                     s_at != NULL ? 1u : 0u,
                     s_at_aux[0] != NULL ? 1u : 0u,
                     s_at_aux[1] != NULL ? 1u : 0u,
                     at_cur_used_memory(),
                     at_max_used_memory());
            at_obj_destroy(s_at_aux[0]);
            at_obj_destroy(s_at_aux[1]);
            at_obj_destroy(s_at);
            s_at_aux[0] = NULL;
            s_at_aux[1] = NULL;
            s_at = NULL;
        } else {
            xy_log_i("PLB-N32 AT multi object create passed count=3 mem=%u/%u",
                     at_cur_used_memory(),
                     at_max_used_memory());
        }
    }
#endif
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
#if PLB_N32_AT_SELFTEST_LATE_RESPONSE
    s_selftest_late_a_received = false;
#endif
#if PLB_N32_AT_SELFTEST_URC_RECOVERY
    s_selftest_urc_timeout_received = false;
#endif
#if PLB_N32_AT_SELFTEST_URC_OVERFLOW
    s_selftest_urc_overflow_unexpected = false;
#endif
#if PLB_N32_AT_SELFTEST_URC_INTERLEAVE
    s_selftest_interleave_csq_valid = false;
#endif
#if PLB_N32_AT_SELFTEST_URC_PROMPT
    s_selftest_prompt_urc_received = false;
#endif
#if PLB_N32_AT_SELFTEST_URC_SEND_RESULT || \
    PLB_N32_AT_SELFTEST_URC_FAKE_SEND_OK || \
    PLB_N32_AT_SELFTEST_URC_FAKE_ERROR || \
    PLB_N32_AT_SELFTEST_URC_PARTIAL_ERROR || \
    PLB_N32_AT_SELFTEST_URC_PARTIAL_TAIL || \
    PLB_N32_AT_SELFTEST_URC_PARTIAL_SEND_OK || \
    PLB_N32_AT_SELFTEST_URC_PARTIAL_SEND_TAIL
    s_selftest_payload_queued = false;
#endif
#if PLB_N32_AT_SELFTEST_URC_SEND_RESULT
    s_selftest_send_result_urc_received = false;
#endif
#if PLB_N32_AT_SELFTEST_URC_FAKE_PROMPT
    s_selftest_fake_prompt_urc_received = false;
    s_selftest_fake_prompt_urc_ms = 0u;
#endif
#if PLB_N32_AT_SELFTEST_URC_FAKE_SEND_OK
    s_selftest_fake_send_ok_urc_received = false;
    s_selftest_fake_send_ok_urc_ms = 0u;
#endif
#if PLB_N32_AT_SELFTEST_URC_FAKE_ERROR
    s_selftest_fake_error_urc_received = false;
    s_selftest_fake_error_urc_ms = 0u;
#endif
#if PLB_N32_AT_SELFTEST_URC_PARTIAL_ERROR
    s_selftest_partial_error_timeout_received = false;
    s_selftest_partial_error_timeout_ms = 0u;
#endif
#if PLB_N32_AT_SELFTEST_URC_PARTIAL_TAIL
    s_selftest_partial_tail_received = false;
    s_selftest_partial_tail_timeout = false;
    s_selftest_partial_tail_start_ms = 0u;
#endif
#if PLB_N32_AT_SELFTEST_URC_PARTIAL_SEND_OK
    s_selftest_partial_send_ok_timeout_received = false;
    s_selftest_partial_send_ok_real_error_received = false;
    s_selftest_partial_send_ok_timeout_ms = 0u;
#endif
#if PLB_N32_AT_SELFTEST_URC_PARTIAL_SEND_TAIL
    s_selftest_partial_send_tail_received = false;
    s_selftest_partial_send_tail_timeout = false;
    s_selftest_partial_send_tail_real_error = false;
    s_selftest_partial_send_tail_start_ms = 0u;
#endif
#if PLB_N32_AT_SELFTEST_URC_BURST
    s_selftest_urc_burst_received = 0u;
    s_selftest_urc_burst_failed = false;
    s_selftest_urc_burst_excess = false;
    s_selftest_urc_settle_deadline_ms = 0u;
    s_selftest_urc_settle_rx_count = 0u;
#endif
#if PLB_N32_AT_SELFTEST_URC_BOUNDARY
    s_selftest_boundary_max_received = 0u;
    s_selftest_boundary_recovery_received = 0u;
    s_selftest_boundary_timeout_received = 0u;
    s_selftest_boundary_unexpected = 0u;
#endif
#if PLB_N32_AT_SELFTEST_MULTI_OBJ
    s_selftest_multi_phase = 0u;
    s_selftest_multi_failed = false;
    memset(s_selftest_multi_count, 0, sizeof(s_selftest_multi_count));
    memset(s_selftest_multi_next, 0, sizeof(s_selftest_multi_next));
#endif
#if PLB_N32_AT_SELFTEST_ERROR_CALLBACK
    s_selftest_error_step = 0u;
    s_selftest_error_hook_count = 0u;
    s_selftest_error_callback_count = 0u;
    s_selftest_error_failed = false;
    memset(&s_selftest_error_hook_response, 0,
           sizeof(s_selftest_error_hook_response));
#endif
#if PLB_N32_AT_SELFTEST_DESTROY
    s_selftest_destroy_phase = 0u;
    s_selftest_destroy_runs = 0u;
    s_selftest_destroy_object_mem = 0u;
    s_selftest_destroy_failed = false;
    for (i = 0u; i < sizeof(s_selftest_destroy_ids); i++) {
        s_selftest_destroy_ids[i] = (unsigned char)i;
    }
#endif
#if PLB_N32_AT_SELFTEST_TICK_WRAP
    s_selftest_wrap_phase = 0u;
    s_selftest_wrap_wait_runs = 0u;
    s_selftest_wrap_timeout_callbacks = 0u;
    s_selftest_wrap_urc_timeouts = 0u;
    s_selftest_wrap_recoveries = 0u;
    s_selftest_wrap_original_tick = xy_tick_now();
    s_selftest_wrap_failed = false;
#endif
#if PLB_N32_AT_SELFTEST_URC_PREFIX
    s_selftest_prefix_short = 0u;
    s_selftest_prefix_long = 0u;
    s_selftest_prefix_embedded = 0u;
    s_selftest_prefix_recovery = 0u;
    s_selftest_prefix_unexpected = 0u;
    s_selftest_prefix_command_sent = false;
    s_selftest_prefix_marker_done = false;
#endif
    s_selftest_tx_calls = 0u;
    s_selftest_tx_short_writes = 0u;
    s_selftest_tx_zero_writes = 0u;
    s_selftest_measure_tx = false;
#if PLB_N32_AT_SELFTEST_WORK_QUEUE
    s_selftest_queue_phase = 0u;
    s_selftest_queue_runs = 0u;
    s_selftest_queue_failed = false;
    memset(s_selftest_queue_order, 0, sizeof(s_selftest_queue_order));
    for (i = 0u; i < sizeof(s_selftest_queue_items) /
                    sizeof(s_selftest_queue_items[0]); i++) {
        s_selftest_queue_items[i].id = (unsigned char)i;
    }
    memset(s_selftest_abort_items, 0, sizeof(s_selftest_abort_items));
#endif
#if PLB_N32_AT_SELFTEST_TX_ZERO_STALL
    s_selftest_cmd_stall_started = false;
    s_selftest_cmd_stall_done = false;
    s_selftest_payload_stall_started = false;
    s_selftest_payload_stall_done = false;
    s_selftest_stall_pm_blocked = false;
    s_selftest_cmd_stall_ms = 0u;
    s_selftest_payload_stall_ms = 0u;
#endif
#if PLB_N32_AT_SELFTEST_ABORT
    s_selftest_abort_armed = false;
    s_selftest_abort_requested = false;
    s_selftest_abort_wait_idle = false;
    s_selftest_abort_accepted = 0u;
#endif
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
#if PLB_N32_AT_SELFTEST_MULTI_OBJ
        at_obj_process(s_at_aux[0]);
        at_obj_process(s_at_aux[1]);
#endif
#if PLB_N32_ENABLE_AT_SELFTEST
#if PLB_N32_AT_SELFTEST_URC_PREFIX
        plb_n32_at_selftest_prefix_process();
        return;
#endif
#if PLB_N32_AT_SELFTEST_TICK_WRAP
        plb_n32_at_selftest_wrap_process();
        return;
#endif
#if PLB_N32_AT_SELFTEST_DESTROY
        plb_n32_at_selftest_destroy_process();
        return;
#endif
#if PLB_N32_AT_SELFTEST_ERROR_CALLBACK
        plb_n32_at_selftest_error_process();
        return;
#endif
#if PLB_N32_AT_SELFTEST_MULTI_OBJ
        plb_n32_at_selftest_multi_process();
        return;
#endif
#if PLB_N32_AT_SELFTEST_WORK_QUEUE
        plb_n32_at_selftest_queue_process();
        return;
#endif
#if PLB_N32_AT_SELFTEST_TX_ZERO_STALL
        if ((s_selftest_cmd_stall_started && !s_selftest_cmd_stall_done) ||
            (s_selftest_payload_stall_started && !s_selftest_payload_stall_done)) {
            if (!at_obj_pm_can_sleep(s_at)) {
                s_selftest_stall_pm_blocked = true;
            } else {
                s_selftest_active = false;
                xy_log_w("PLB-N32 AT selftest FAILED reason=TX_ZERO_STALL_PM");
            }
        }
#endif
#if PLB_N32_AT_SELFTEST_ABORT
        if (s_selftest_abort_requested) {
            s_selftest_abort_requested = false;
            at_work_abort_all(s_at);
            s_selftest_abort_wait_idle = true;
            s_selftest_waiting = false;
            xy_log_i("PLB-N32 AT selftest abort requested accepted=%u",
                     s_selftest_abort_accepted);
        }
        if (s_selftest_abort_wait_idle && !at_obj_busy(s_at) &&
            at_obj_pm_can_sleep(s_at)) {
            s_selftest_abort_wait_idle = false;
            s_selftest_active = false;
            s_selftest_measure_tx = false;
            xy_log_i("PLB-N32 AT selftest ABORT PASSED accepted=%u calls=%u short=%u zero=%u",
                     s_selftest_abort_accepted,
                     s_selftest_tx_calls,
                     s_selftest_tx_short_writes,
                     s_selftest_tx_zero_writes);
        }
#endif
        if (s_selftest_active && !s_selftest_waiting
#if PLB_N32_AT_SELFTEST_ABORT
            && !s_selftest_abort_wait_idle
#endif
        ) {
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
#if PLB_N32_AT_SELFTEST_URC_PARTIAL_TAIL
                s_selftest_partial_tail_start_ms = xy_tick_now_ms();
#endif
#if PLB_N32_AT_SELFTEST_URC_PARTIAL_SEND_TAIL
                s_selftest_partial_send_tail_start_ms = xy_tick_now_ms();
#endif
                if (at_do_work(s_at, NULL, plb_n32_at_selftest_send_work)) {
                    s_selftest_waiting = true;
                } else {
                    s_selftest_measure_tx = false;
                }
            } else if (s_selftest_step == command_count + 1u) {
                s_selftest_measure_tx = false;
                at_attr_deinit(&attr);
                attr.cb = plb_n32_at_selftest_response;
#if PLB_N32_AT_SELFTEST_RESP_BOUNDARY
                attr.prefix = s_selftest_step == command_count + 1u
                              ? "+SIMBUF:" : "+RECOVER:";
                attr.timeout = 1000u;
                attr.retry = 0u;
                if (at_exec_cmd(s_at, &attr, "%s",
                                s_selftest_boundary_commands[0])) {
                    s_selftest_waiting = true;
                }
#elif PLB_N32_AT_SELFTEST_LATE_RESPONSE
                attr.prefix = "+SIMLATEA:";
                attr.timeout = 500u;
                attr.retry = 1u;
                if (at_exec_cmd(s_at, &attr, "AT+SIMLATEA")) {
                    s_selftest_waiting = true;
                }
#else
                attr.timeout = 1000u;
                attr.retry = 2u;
                if (at_exec_cmd(s_at, &attr,
#if PLB_N32_AT_SELFTEST_URC_BURST
                                "AT+SIMURC=BURST")) {
#elif PLB_N32_AT_SELFTEST_URC_RECOVERY
                                "AT+SIMURC=RECOVER")) {
#elif PLB_N32_AT_SELFTEST_URC_OVERFLOW
                                "AT+SIMURC=OVERFLOW")) {
#elif PLB_N32_AT_SELFTEST_URC_INTERLEAVE
                                "AT+SIMURC=INTERLEAVE")) {
#elif PLB_N32_AT_SELFTEST_URC_PROMPT
                                "AT+SIMURC=PROMPT")) {
#elif PLB_N32_AT_SELFTEST_URC_SEND_RESULT
                                "AT+SIMURC=SENDRESULT")) {
#elif PLB_N32_AT_SELFTEST_URC_FAKE_PROMPT
                                "AT+SIMURC=FAKEPROMPT")) {
#elif PLB_N32_AT_SELFTEST_URC_FAKE_SEND_OK
                                "AT+SIMURC=FAKESENDOK")) {
#elif PLB_N32_AT_SELFTEST_URC_FAKE_ERROR
                                "AT+SIMURC=FAKEERROR")) {
#elif PLB_N32_AT_SELFTEST_URC_PARTIAL_ERROR
                                "AT+SIMURC=PARTIALERROR")) {
#elif PLB_N32_AT_SELFTEST_URC_PARTIAL_TAIL
                                "AT+SIMURC=PARTIALTAIL")) {
#elif PLB_N32_AT_SELFTEST_URC_PARTIAL_SEND_OK
                                "AT+SIMURC=PARTIALSENDOK")) {
#elif PLB_N32_AT_SELFTEST_URC_PARTIAL_SEND_TAIL
                                "AT+SIMURC=PARTIALSENDTAIL")) {
#elif PLB_N32_AT_SELFTEST_URC_BOUNDARY
                                "AT+SIMURC=BOUNDARY")) {
#else
                                "AT+SIMURC=RECV")) {
#endif
                    s_selftest_waiting = true;
                }
#endif
#if PLB_N32_AT_SELFTEST_LATE_RESPONSE
            } else if (s_selftest_step == command_count + 2u) {
                at_attr_deinit(&attr);
                attr.cb = plb_n32_at_selftest_response;
                attr.prefix = "+SIMLATEB:";
                attr.timeout = 1000u;
                attr.retry = 0u;
                if (at_exec_cmd(s_at, &attr, "AT+SIMLATEB")) {
                    s_selftest_waiting = true;
                }
#endif
#if PLB_N32_AT_SELFTEST_RESP_BOUNDARY
            } else if (s_selftest_step > command_count + 1u &&
                       s_selftest_step <= command_count +
                           sizeof(s_selftest_boundary_commands) /
                           sizeof(s_selftest_boundary_commands[0])) {
                unsigned int index = s_selftest_step - command_count - 1u;

                at_attr_deinit(&attr);
                attr.cb = plb_n32_at_selftest_response;
                attr.prefix = (index == 2u || index == 4u)
                              ? "+RECOVER:" : "+SIMBUF:";
                attr.timeout = 1000u;
                attr.retry = 0u;
                if (at_exec_cmd(s_at, &attr, "%s",
                                s_selftest_boundary_commands[index])) {
                    s_selftest_waiting = true;
                }
#endif
#if PLB_N32_AT_SELFTEST_URC_BURST
            } else if (s_selftest_urc_burst_received == PLB_N32_AT_URC_BURST_COUNT &&
                       s_selftest_urc_settle_rx_count != g_n32_uart5_rx_count) {
                s_selftest_urc_settle_rx_count = g_n32_uart5_rx_count;
                s_selftest_urc_settle_deadline_ms = xy_tick_now_ms() +
                                                    PLB_N32_AT_URC_SETTLE_MS;
            } else if (s_selftest_urc_burst_failed) {
                s_selftest_active = false;
                xy_log_w("PLB-N32 AT selftest FAILED reason=%s received=%u",
                         s_selftest_urc_burst_excess ? "URC_BURST_EXCESS" :
                                                       "URC_BURST_ORDER",
                         s_selftest_urc_burst_received);
            } else if (s_selftest_urc_burst_received == PLB_N32_AT_URC_BURST_COUNT &&
                       (int32_t)(xy_tick_now_ms() -
                                 s_selftest_urc_settle_deadline_ms) >= 0 &&
                       n32_uart5_rx_pending() == 0u &&
                       !at_obj_busy(s_at)) {
                s_selftest_active = false;
                if (g_n32_uart5_rx_drop_count == 0u) {
                    xy_log_i("PLB-N32 AT selftest URC BURST PASSED count=%u drop=0",
                             s_selftest_urc_burst_received);
                } else {
                    xy_log_w("PLB-N32 AT selftest FAILED reason=URC_BURST_DROP drop=%u",
                             (unsigned int)g_n32_uart5_rx_drop_count);
                }
            } else if (s_selftest_urc_burst_received < PLB_N32_AT_URC_BURST_COUNT &&
                       (int32_t)(xy_tick_now_ms() -
                                 s_selftest_urc_deadline_ms) >= 0) {
                s_selftest_active = false;
                xy_log_w("PLB-N32 AT selftest FAILED reason=URC_TIMEOUT received=%u",
                         s_selftest_urc_burst_received);
            }
#else
#if PLB_N32_AT_SELFTEST_URC_OVERFLOW
            } else if (s_selftest_urc_overflow_unexpected) {
                s_selftest_active = false;
                xy_log_w("PLB-N32 AT selftest FAILED reason=URC_OVERFLOW_ACCEPTED");
#endif
            } else if (s_selftest_urc_received) {
                s_selftest_active = false;
#if PLB_N32_AT_SELFTEST_URC_RECOVERY
                if (g_n32_uart5_rx_drop_count == 0u) {
                    xy_log_i("PLB-N32 AT selftest URC RECOVERY PASSED timeout=1 drop=0");
                } else {
                    xy_log_w("PLB-N32 AT selftest FAILED reason=URC_RECOVERY_DROP drop=%u",
                             (unsigned int)g_n32_uart5_rx_drop_count);
                }
#elif PLB_N32_AT_SELFTEST_URC_OVERFLOW
                if (g_n32_uart5_rx_drop_count == 0u) {
                    xy_log_i("PLB-N32 AT selftest URC OVERFLOW RECOVERY PASSED drop=0");
                } else {
                    xy_log_w("PLB-N32 AT selftest FAILED reason=URC_OVERFLOW_DROP drop=%u",
                             (unsigned int)g_n32_uart5_rx_drop_count);
                }
#elif PLB_N32_AT_SELFTEST_URC_INTERLEAVE
                if (!s_selftest_interleave_csq_valid) {
                    xy_log_w("PLB-N32 AT selftest FAILED reason=INTERLEAVE_INCOMPLETE");
                } else if (g_n32_uart5_rx_drop_count != 0u) {
                    xy_log_w("PLB-N32 AT selftest FAILED reason=INTERLEAVE_DROP drop=%u",
                             (unsigned int)g_n32_uart5_rx_drop_count);
                } else {
                    xy_log_i("PLB-N32 AT selftest URC INTERLEAVE PASSED drop=0");
                }
#elif PLB_N32_AT_SELFTEST_URC_PROMPT
                if (!s_selftest_prompt_urc_received) {
                    xy_log_w("PLB-N32 AT selftest FAILED reason=PROMPT_URC_INCOMPLETE");
                } else if (g_n32_uart5_rx_drop_count != 0u) {
                    xy_log_w("PLB-N32 AT selftest FAILED reason=PROMPT_URC_DROP drop=%u",
                             (unsigned int)g_n32_uart5_rx_drop_count);
                } else {
                    xy_log_i("PLB-N32 AT selftest URC PROMPT PASSED drop=0");
                }
#elif PLB_N32_AT_SELFTEST_URC_SEND_RESULT
                if (!s_selftest_send_result_urc_received) {
                    xy_log_w("PLB-N32 AT selftest FAILED reason=SEND_RESULT_URC_INCOMPLETE");
                } else if (g_n32_uart5_rx_drop_count != 0u) {
                    xy_log_w("PLB-N32 AT selftest FAILED reason=SEND_RESULT_URC_DROP drop=%u",
                             (unsigned int)g_n32_uart5_rx_drop_count);
                } else {
                    xy_log_i("PLB-N32 AT selftest URC SEND RESULT PASSED drop=0");
                }
#elif PLB_N32_AT_SELFTEST_URC_FAKE_PROMPT
                if (!s_selftest_fake_prompt_urc_received) {
                    xy_log_w("PLB-N32 AT selftest FAILED reason=FAKE_PROMPT_INCOMPLETE");
                } else if (g_n32_uart5_rx_drop_count != 0u) {
                    xy_log_w("PLB-N32 AT selftest FAILED reason=FAKE_PROMPT_DROP drop=%u",
                             (unsigned int)g_n32_uart5_rx_drop_count);
                } else {
                    xy_log_i("PLB-N32 AT selftest URC FAKE PROMPT PASSED drop=0");
                }
#elif PLB_N32_AT_SELFTEST_URC_FAKE_SEND_OK
                if (!s_selftest_fake_send_ok_urc_received) {
                    xy_log_w("PLB-N32 AT selftest FAILED reason=FAKE_SEND_OK_INCOMPLETE");
                } else if (g_n32_uart5_rx_drop_count != 0u) {
                    xy_log_w("PLB-N32 AT selftest FAILED reason=FAKE_SEND_OK_DROP drop=%u",
                             (unsigned int)g_n32_uart5_rx_drop_count);
                } else {
                    xy_log_i("PLB-N32 AT selftest URC FAKE SEND OK PASSED drop=0");
                }
#elif PLB_N32_AT_SELFTEST_URC_FAKE_ERROR
                if (!s_selftest_fake_error_urc_received) {
                    xy_log_w("PLB-N32 AT selftest FAILED reason=FAKE_ERROR_INCOMPLETE");
                } else if (g_n32_uart5_rx_drop_count != 0u) {
                    xy_log_w("PLB-N32 AT selftest FAILED reason=FAKE_ERROR_DROP drop=%u",
                             (unsigned int)g_n32_uart5_rx_drop_count);
                } else {
                    xy_log_i("PLB-N32 AT selftest URC FAKE ERROR PASSED drop=0");
                }
#elif PLB_N32_AT_SELFTEST_URC_PARTIAL_ERROR
                if (!s_selftest_partial_error_timeout_received) {
                    xy_log_w("PLB-N32 AT selftest FAILED reason=PARTIAL_ERROR_INCOMPLETE");
                } else if (g_n32_uart5_rx_drop_count != 0u) {
                    xy_log_w("PLB-N32 AT selftest FAILED reason=PARTIAL_ERROR_DROP drop=%u",
                             (unsigned int)g_n32_uart5_rx_drop_count);
                } else {
                    xy_log_i("PLB-N32 AT selftest URC PARTIAL ERROR PASSED timeout=1 drop=0");
                }
#elif PLB_N32_AT_SELFTEST_URC_PARTIAL_TAIL
                if (!s_selftest_partial_tail_received || s_selftest_partial_tail_timeout) {
                    xy_log_w("PLB-N32 AT selftest FAILED reason=PARTIAL_TAIL_INCOMPLETE");
                } else if (g_n32_uart5_rx_drop_count != 0u) {
                    xy_log_w("PLB-N32 AT selftest FAILED reason=PARTIAL_TAIL_DROP drop=%u",
                             (unsigned int)g_n32_uart5_rx_drop_count);
                } else {
                    xy_log_i("PLB-N32 AT selftest URC PARTIAL TAIL PASSED timeout=0 drop=0");
                }
#elif PLB_N32_AT_SELFTEST_URC_PARTIAL_SEND_OK
                if (!s_selftest_partial_send_ok_timeout_received ||
                    !s_selftest_partial_send_ok_real_error_received) {
                    xy_log_w("PLB-N32 AT selftest FAILED reason=PARTIAL_SEND_OK_INCOMPLETE");
                } else if (g_n32_uart5_rx_drop_count != 0u) {
                    xy_log_w("PLB-N32 AT selftest FAILED reason=PARTIAL_SEND_OK_DROP drop=%u",
                             (unsigned int)g_n32_uart5_rx_drop_count);
                } else {
                    xy_log_i("PLB-N32 AT selftest URC PARTIAL SEND OK PASSED timeout=1 error=1 drop=0");
                }
#elif PLB_N32_AT_SELFTEST_URC_PARTIAL_SEND_TAIL
                if (!s_selftest_partial_send_tail_received ||
                    s_selftest_partial_send_tail_timeout ||
                    !s_selftest_partial_send_tail_real_error) {
                    xy_log_w("PLB-N32 AT selftest FAILED reason=PARTIAL_SEND_TAIL_INCOMPLETE");
                } else if (g_n32_uart5_rx_drop_count != 0u) {
                    xy_log_w("PLB-N32 AT selftest FAILED reason=PARTIAL_SEND_TAIL_DROP drop=%u",
                             (unsigned int)g_n32_uart5_rx_drop_count);
                } else {
                    xy_log_i("PLB-N32 AT selftest URC PARTIAL SEND TAIL PASSED timeout=0 error=1 drop=0");
                }
#elif PLB_N32_AT_SELFTEST_URC_BOUNDARY
                if (s_selftest_boundary_max_received != 1u ||
                    s_selftest_boundary_recovery_received != 1u ||
                    s_selftest_boundary_timeout_received != 1u ||
                    s_selftest_boundary_unexpected != 0u) {
                    xy_log_w("PLB-N32 AT selftest FAILED reason=URC_BOUNDARY_COUNTS max=%u recover=%u timeout=%u unexpected=%u",
                             s_selftest_boundary_max_received,
                             s_selftest_boundary_recovery_received,
                             s_selftest_boundary_timeout_received,
                             s_selftest_boundary_unexpected);
                } else if (g_n32_uart5_rx_drop_count != 0u) {
                    xy_log_w("PLB-N32 AT selftest FAILED reason=URC_BOUNDARY_DROP drop=%u",
                             (unsigned int)g_n32_uart5_rx_drop_count);
                } else {
                    xy_log_i("PLB-N32 AT selftest URC BOUNDARY PASSED max=232 over=233 malformed=0/-1/huge short=1 long=1 recover=1 drop=0");
                }
#else
#if PLB_N32_AT_SELFTEST_TX_ZERO_STALL
                if (!s_selftest_cmd_stall_done || !s_selftest_payload_stall_done ||
                    !s_selftest_stall_pm_blocked || s_selftest_tx_zero_writes == 0u) {
                    xy_log_w("PLB-N32 AT selftest FAILED reason=TX_ZERO_STALL_INCOMPLETE");
                } else if (g_n32_uart5_rx_drop_count != 0u) {
                    xy_log_w("PLB-N32 AT selftest FAILED reason=TX_ZERO_STALL_DROP drop=%u",
                             (unsigned int)g_n32_uart5_rx_drop_count);
                } else {
                    xy_log_i("PLB-N32 AT selftest TX ZERO STALL PASSED command=700 payload=700 zero=%u drop=0",
                             s_selftest_tx_zero_writes);
                }
#else
                xy_log_i("PLB-N32 AT selftest PASSED");
#endif
#endif
            } else if ((int32_t)(xy_tick_now_ms() -
                                 s_selftest_urc_deadline_ms) >= 0) {
                s_selftest_active = false;
                xy_log_w("PLB-N32 AT selftest FAILED reason=URC_TIMEOUT");
            }
#endif
        }
#endif
    }
}

at_obj_t *plb_n32_at_obj(void)
{
    return s_at;
}

bool plb_n32_at_pm_can_sleep(void *arg)
{
    (void)arg;

    if (!at_obj_pm_can_sleep(s_at)) {
        return false;
    }
#if PLB_N32_AT_SELFTEST_MULTI_OBJ
    if (!at_obj_pm_can_sleep(s_at_aux[0]) ||
        !at_obj_pm_can_sleep(s_at_aux[1])) {
        return false;
    }
#endif
    return true;
}
