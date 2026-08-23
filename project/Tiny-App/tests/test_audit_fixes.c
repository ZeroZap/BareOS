/*
 * test_audit_fixes.c
 *
 * Regression tests pinning down the audit fixes from commit 2a39b2d and the
 * URC accumulator helper from commit 1502432. Each TEST_CASE name traces back
 * to the audit P-numbering documented in the commit message.
 *
 * Run via the bareos_tests CMake target (see CMakeLists.txt).
 */

#include "test_harness.h"

#include <stdlib.h>  /* strtol for the test-only header parser */

#include "xy_ais.h"
#include "xy_crc.h"
#include "xy_ctype.h"
#include "xy_mem.h"
#include "xy_tick.h"
#include "at_chat.h"

/* ── Test harness globals ──────────────────────────────────────────────── */
int g_test_failures = 0;
int g_test_count    = 0;
const char *g_test_current_name = NULL;
const char *g_test_filter       = NULL;
int g_test_list_only            = 0;

/* ── P0-C: AIS sign-extend at bit_count == 32 ──────────────────────────── */

/* Build a payload of 6-bit values from a hex bit pattern. Each input nibble
 * fills 4 bits MSB-first across the 6-bit array. For tests we just stuff a
 * 32-bit value into the first 6 six-bit slots (36 bits, more than enough). */
static void pack_bits(uint8_t *payload, int n_slots, uint32_t value, int bits)
{
    for (int i = 0; i < n_slots; i++) payload[i] = 0;
    for (int i = 0; i < bits; i++) {
        int bit_idx = bits - 1 - i;
        int set     = (value >> bit_idx) & 1u;
        int abs_bit = i;
        int slot    = abs_bit / 6;
        int pos     = 5 - (abs_bit % 6);
        if (slot < n_slots && set) payload[slot] |= (uint8_t)(1u << pos);
    }
}

static void test_ais_sign_extend(void)
{
    uint8_t payload[8];

    /* 32-bit sign-extend: full uint32_t value should pass through unchanged
     * (previously triggered UB via `1u << 32`). */
    TEST_CASE("P0-C: ais_getbits_signed bit_count=32 round-trip") {
        pack_bits(payload, 8, 0x80000000u, 32);
        int32_t v = ais_getbits_signed(payload, 0, 32);
        TEST_EQ(v, (int32_t)0x80000000);
    }

    TEST_CASE("P0-C: ais_getbits_signed bit_count=32 positive") {
        pack_bits(payload, 8, 0x7FFFFFFFu, 32);
        int32_t v = ais_getbits_signed(payload, 0, 32);
        TEST_EQ(v, 0x7FFFFFFF);
    }

    /* 28-bit (typical AIS longitude) negative sign-extends correctly. */
    TEST_CASE("ais_getbits_signed bit_count=28 negative") {
        /* 0x8000000 = MSB of 28-bit field set */
        pack_bits(payload, 8, 0x8000000u, 28);
        int32_t v = ais_getbits_signed(payload, 0, 28);
        TEST_EQ(v, (int32_t)0xF8000000); /* sign-extended */
    }

    TEST_CASE("ais_getbits_signed bit_count=0 returns 0") {
        pack_bits(payload, 8, 0, 0);
        TEST_EQ(ais_getbits_signed(payload, 0, 0), 0);
    }

    /* P1: defensive bound — reading past AIS_MAX_BIT_INDEX returns 0. */
    TEST_CASE("ais_getbits defensive cap past max payload") {
        memset(payload, 0xFF, sizeof(payload));
        /* AIS_MAX_BIT_INDEX = 672; request bit 700 — should clamp to 0. */
        uint32_t v = ais_getbits(payload, 700, 1);
        TEST_EQ(v, 0u);
    }
}

/* ── P0-D: CRC table-driven correctness for widths > 8 ─────────────────── */

static void test_crc_table_widths(void)
{
    const uint8_t data[] = "123456789"; /* canonical CRC test vector */

    /* Compare table-driven vs bit-by-bit software computation for several
     * standard CRCs. Previously the table path was broken for width > 8. */

    /* CRC-16/CCITT-FALSE: 0x29B1 */
    TEST_CASE("P0-D: CRC-16/CCITT-FALSE table == sw") {
        xy_crc_cfg_t cfg = {16, 0x1021, 0xFFFF, 0x0000, 0, 0};
        uint64_t table[256];
        TEST_EQ(xy_crc_make_table(&cfg, table), 0);
        uint64_t sw   = xy_crc_calc(&cfg, data, 9);
        uint64_t tbl  = xy_crc_calc_table(&cfg, table, data, 9);
        TEST_EQ(sw, 0x29B1u);
        TEST_EQ(tbl, sw);
    }

    /* CRC-16/XMODEM: 0x31C3 */
    TEST_CASE("P0-D: CRC-16/XMODEM table == sw") {
        xy_crc_cfg_t cfg = {16, 0x1021, 0x0000, 0x0000, 0, 0};
        uint64_t table[256];
        xy_crc_make_table(&cfg, table);
        uint64_t sw  = xy_crc_calc(&cfg, data, 9);
        uint64_t tbl = xy_crc_calc_table(&cfg, table, data, 9);
        TEST_EQ(sw, 0x31C3u);
        TEST_EQ(tbl, sw);
    }

    /* CRC-32/BZIP2 (no reflection): 0xFC891918 */
    TEST_CASE("P0-D: CRC-32/BZIP2 table == sw") {
        xy_crc_cfg_t cfg = {32, 0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF, 0, 0};
        uint64_t table[256];
        xy_crc_make_table(&cfg, table);
        uint64_t sw  = xy_crc_calc(&cfg, data, 9);
        uint64_t tbl = xy_crc_calc_table(&cfg, table, data, 9);
        TEST_EQ(sw, 0xFC891918ULL);
        TEST_EQ(tbl, sw);
    }
}

/* ── P1: xy_xdigit_val / xy_isxdigit ───────────────────────────────────── */

static void test_xdigit(void)
{
    TEST_CASE("P1: xy_xdigit_val decimal digits") {
        for (char c = '0'; c <= '9'; c++)
            TEST_EQ(xy_xdigit_val((int8_t)c), (uint8_t)(c - '0'));
    }
    TEST_CASE("P1: xy_xdigit_val uppercase hex") {
        for (char c = 'A'; c <= 'F'; c++)
            TEST_EQ(xy_xdigit_val((int8_t)c), (uint8_t)(c - 'A' + 10));
    }
    TEST_CASE("P1: xy_xdigit_val lowercase hex") {
        for (char c = 'a'; c <= 'f'; c++)
            TEST_EQ(xy_xdigit_val((int8_t)c), (uint8_t)(c - 'a' + 10));
    }
    TEST_CASE("P1: xy_xdigit_val invalid returns 0xFF") {
        TEST_EQ(xy_xdigit_val((int8_t)'G'), 0xFFu);
        TEST_EQ(xy_xdigit_val((int8_t)'g'), 0xFFu);
        TEST_EQ(xy_xdigit_val((int8_t)'/'), 0xFFu);
        TEST_EQ(xy_xdigit_val((int8_t)':'), 0xFFu);
        TEST_EQ(xy_xdigit_val((int8_t)0), 0xFFu);
    }
    TEST_CASE("P1: xy_isxdigit accepts hex, rejects others") {
        TEST_TRUE(xy_isxdigit('0'));
        TEST_TRUE(xy_isxdigit('9'));
        TEST_TRUE(xy_isxdigit('A'));
        TEST_TRUE(xy_isxdigit('f'));
        TEST_FALSE(xy_isxdigit('G'));
        TEST_FALSE(xy_isxdigit(' '));
    }
}

/* ── Sprint 1: at_urc_recv_split two-phase logic ───────────────────────── */

/* Header parser matching "+TEST,<id>,<len>:" */
static int parse_test_hdr(const char *buf, int len,
                          int *id, int *bytes, int *hdr)
{
    (void)len;
    const char *p = buf;
    while (*p && *p != ',') p++;
    if (*p != ',') return -1;
    p++;
    char *endp;
    *id    = (int)strtol(p, &endp, 10);
    if (*endp != ',') return -1;
    p = endp + 1;
    *bytes = (int)strtol(p, &endp, 10);
    if (*endp != ':') return -1;
    *hdr   = (int)(endp - buf) + 1;   /* include the ':' */
    return 0;
}

static void test_urc_recv_split(void)
{
    /* First call: header only, expect "request len bytes" */
    TEST_CASE("Sprint1: at_urc_recv_split first call returns byte count") {
        const char *hdr = "+TEST,2,7:";
        at_urc_info_t info = {0, (char *)hdr, (int)strlen(hdr)};
        int id, plen;
        const char *payload;
        int rc = at_urc_recv_split(&info, parse_test_hdr, 0,
                                   &id, &payload, &plen);
        TEST_EQ(rc, 7);
    }

    /* First call with trail_bytes=2: expect len+2 */
    TEST_CASE("Sprint1: at_urc_recv_split first call with trail bytes") {
        const char *hdr = "+TEST,2,7:";
        at_urc_info_t info = {0, (char *)hdr, (int)strlen(hdr)};
        int id, plen;
        const char *payload;
        int rc = at_urc_recv_split(&info, parse_test_hdr, 2,
                                   &id, &payload, &plen);
        TEST_EQ(rc, 9);
    }

    /* Second call: full buffer; expect payload pointer + length */
    TEST_CASE("Sprint1: at_urc_recv_split second call returns payload") {
        const char *full = "+TEST,3,5:HELLO";
        at_urc_info_t info = {0, (char *)full, (int)strlen(full)};
        int id = -99, plen = -99;
        const char *payload = NULL;
        int rc = at_urc_recv_split(&info, parse_test_hdr, 0,
                                   &id, &payload, &plen);
        TEST_EQ(rc, 0);
        TEST_EQ(id, 3);
        TEST_EQ(plen, 5);
        TEST_TRUE(payload != NULL);
        TEST_MEM_EQ(payload, "HELLO", 5);
    }

    TEST_CASE("Sprint1: at_urc_recv_split excludes trailing CRLF from payload") {
        const char *full = "+TEST,3,5:HELLO\r\n";
        at_urc_info_t info = {URC_RECV_OK, (char *)full, (int)strlen(full)};
        int id = -99, plen = -99;
        const char *payload = NULL;
        int rc = at_urc_recv_split(&info, parse_test_hdr, 2,
                                   &id, &payload, &plen);

        TEST_EQ(rc, 0);
        TEST_EQ(id, 3);
        TEST_EQ(plen, 5);
        TEST_MEM_EQ(payload, "HELLO", 5);
    }

    /* Bad header (no comma) → rc < 0 */
    TEST_CASE("Sprint1: at_urc_recv_split rejects malformed header") {
        const char *bad = "+TEST garbage";
        at_urc_info_t info = {0, (char *)bad, (int)strlen(bad)};
        int id, plen;
        const char *payload;
        int rc = at_urc_recv_split(&info, parse_test_hdr, 0,
                                   &id, &payload, &plen);
        TEST_TRUE(rc < 0);
    }

    /* NULL info / NULL parse → rc < 0 */
    TEST_CASE("Sprint1: at_urc_recv_split rejects NULL params") {
        int id, plen;
        const char *payload;
        TEST_TRUE(at_urc_recv_split(NULL, parse_test_hdr, 0,
                                    &id, &payload, &plen) < 0);
        at_urc_info_t info = {0, "x", 1};
        TEST_TRUE(at_urc_recv_split(&info, NULL, 0,
                                    &id, &payload, &plen) < 0);
    }

    TEST_CASE("Sprint1: at_urc_recv_split rejects timeout and malformed trail") {
        int id, plen;
        const char *payload;
        char timeout_frame[] = "+TEST,0,5:ABC";
        char bad_trail[] = "+TEST,0,3:TOOL\r";
        at_urc_info_t timeout_info = {
            URC_RECV_TIMEOUT, timeout_frame, (int)strlen(timeout_frame)
        };
        at_urc_info_t trail_info = {
            URC_RECV_OK, bad_trail, (int)strlen(bad_trail)
        };

        TEST_TRUE(at_urc_recv_split(&timeout_info, parse_test_hdr, 0,
                                    &id, &payload, &plen) < 0);
        TEST_TRUE(at_urc_recv_split(&trail_info, parse_test_hdr, 2,
                                    &id, &payload, &plen) < 0);
    }
}

/* ── Sprint 4: at_prompt_send_step state machine ───────────────────────── */

/* Fake adapter to record the raw bytes written during the prompt sequence */
XY_MEM_POOL_DECLARE(s_at_test_mem, 8192);

static uint8_t s_fake_write_buf[512];
static unsigned s_fake_write_len = 0;
static uint8_t s_fake_read_buf[512];
static unsigned s_fake_read_len;
static unsigned s_fake_read_pos;
static unsigned int s_fake_write_script[16];
static unsigned int s_fake_write_script_len;
static unsigned int s_fake_write_script_pos;
static unsigned int s_fake_rx_pending;
static bool s_fake_tx_idle;
#if AT_RAW_TRANSPARENT_EN
static uint8_t s_peer_read_buf[64];
static unsigned int s_peer_read_len;
static unsigned int s_peer_read_pos;
static uint8_t s_peer_write_buf[64];
static unsigned int s_peer_write_len;
static unsigned int s_peer_write_script[8];
static unsigned int s_peer_write_script_len;
static unsigned int s_peer_write_script_pos;
static at_obj_t *s_raw_exit_obj;
#endif
static unsigned int fake_write(const void *data, unsigned int len)
{
    unsigned int copy = len;

    if (s_fake_write_script_pos < s_fake_write_script_len &&
        copy > s_fake_write_script[s_fake_write_script_pos]) {
        copy = s_fake_write_script[s_fake_write_script_pos];
    }
    if (s_fake_write_script_pos < s_fake_write_script_len) {
        s_fake_write_script_pos++;
    }
    if (copy > sizeof(s_fake_write_buf) - s_fake_write_len) {
        copy = (unsigned int)(sizeof(s_fake_write_buf) - s_fake_write_len);
    }
    memcpy(&s_fake_write_buf[s_fake_write_len], data, copy);
    s_fake_write_len += copy;
    return copy;
}
static unsigned int fake_rx_pending(void) { return s_fake_rx_pending; }
static bool fake_tx_idle(void) { return s_fake_tx_idle; }
static unsigned int fake_read(void *data, unsigned int len)
{
    unsigned int available = s_fake_read_len - s_fake_read_pos;

    if (available > len) available = len;
    if (available != 0u) {
        memcpy(data, &s_fake_read_buf[s_fake_read_pos], available);
        s_fake_read_pos += available;
    }
    return available;
}

#if AT_RAW_TRANSPARENT_EN
static unsigned int fake_peer_write(const void *data, unsigned int len)
{
    unsigned int copy = len;

    if (s_peer_write_script_pos < s_peer_write_script_len &&
        copy > s_peer_write_script[s_peer_write_script_pos]) {
        copy = s_peer_write_script[s_peer_write_script_pos];
    }
    if (s_peer_write_script_pos < s_peer_write_script_len) {
        s_peer_write_script_pos++;
    }
    if (copy > sizeof(s_peer_write_buf) - s_peer_write_len) {
        copy = (unsigned int)(sizeof(s_peer_write_buf) - s_peer_write_len);
    }
    memcpy(&s_peer_write_buf[s_peer_write_len], data, copy);
    s_peer_write_len += copy;
    return copy;
}

static unsigned int fake_peer_read(void *data, unsigned int len)
{
    unsigned int available = s_peer_read_len - s_peer_read_pos;

    if (available > len) available = len;
    if (available != 0u) {
        memcpy(data, &s_peer_read_buf[s_peer_read_pos], available);
        s_peer_read_pos += available;
    }
    return available;
}

static void fake_raw_exit(void)
{
    at_raw_transport_exit(s_raw_exit_obj);
}
#endif

static at_adapter_t s_fake_adap = {
    .lock = NULL, .unlock = NULL,
    .write = fake_write, .read = fake_read,
};

/* Fake at_env_t that just returns programmable values */
static char *s_recv_inject = "";
static int   s_state_done  = 0;

static void fake_reset_io(void)
{
    s_fake_write_len = 0u;
    s_fake_read_len = 0u;
    s_fake_read_pos = 0u;
    s_fake_write_script_len = 0u;
    s_fake_write_script_pos = 0u;
    s_fake_rx_pending = 0u;
    s_fake_tx_idle = true;
#if AT_RAW_TRANSPARENT_EN
    s_peer_read_len = 0u;
    s_peer_read_pos = 0u;
    s_peer_write_len = 0u;
    s_peer_write_script_len = 0u;
    s_peer_write_script_pos = 0u;
#endif
}

static void fake_inject(const char *data)
{
    s_fake_read_len = (unsigned int)strlen(data);
    s_fake_read_pos = 0u;
    memcpy(s_fake_read_buf, data, s_fake_read_len);
}

static void fake_inject_bytes(const unsigned char *data, unsigned int len)
{
    s_fake_read_len = len;
    s_fake_read_pos = 0u;
    memcpy(s_fake_read_buf, data, len);
}

static char *fake_contains(at_env_t *e, const char *s)
{ (void)e; return strstr(s_recv_inject, s); }
static bool fake_is_timeout(at_env_t *e, unsigned int ms)
{ (void)e; (void)ms; return false; }
static void fake_println(at_env_t *e, const char *fmt, ...) { (void)e; (void)fmt; }
static void fake_recvclr(at_env_t *e) { (void)e; s_recv_inject = ""; }
static void fake_reset_timer(at_env_t *e) { (void)e; }
static bool fake_env_write(at_env_t *e, const void *data, unsigned int len)
{ (void)e; return fake_write(data, len) == len; }
static void fake_finish(at_env_t *e, at_resp_code c)
{ (void)e; (void)c; s_state_done = 1; }

static void test_prompt_send_step(void)
{
    /* Minimal fake obj/env enough for at_prompt_send_step to call write() */
    static at_obj_t fake_obj;
    fake_obj.adap = &s_fake_adap;

    at_env_t env = {0};
    env.obj          = &fake_obj;
    env.contains     = fake_contains;
    env.is_timeout   = fake_is_timeout;
    env.println      = fake_println;
    env.recvclr      = fake_recvclr;
    env.write        = fake_env_write;
    env.reset_timer  = fake_reset_timer;
    env.finish       = fake_finish;

    TEST_CASE("Sprint4: prompt_send_step state 1 sees '>' then writes payload") {
        env.state = 1;
        s_recv_inject = "x > y";
        s_fake_write_len = 0;
        int rc = at_prompt_send_step(&env, "HELLO", 5, "OK", NULL, NULL,
                                     5000, 10000);
        TEST_EQ(rc, 0);                 /* still running */
        TEST_EQ(env.state, 2);          /* advanced */
        TEST_EQ(s_fake_write_len, 5);
        TEST_MEM_EQ(s_fake_write_buf, "HELLO", 5);
    }

    TEST_CASE("Sprint4: prompt_send_step state 2 matches ok1") {
        env.state = 2;
        s_recv_inject = "ok blah SEND OK\r\n";
        int rc = at_prompt_send_step(&env, NULL, 0, "SEND OK", NULL, NULL,
                                     5000, 10000);
        TEST_EQ(rc, 1);
    }

    TEST_CASE("Sprint4: prompt_send_step state 2 matches ok2") {
        env.state = 2;
        s_recv_inject = "DATA ACCEPT 5\r\n";
        int rc = at_prompt_send_step(&env, NULL, 0, "SEND OK", "DATA ACCEPT",
                                     NULL, 5000, 10000);
        TEST_EQ(rc, 1);
    }

    TEST_CASE("Sprint4: prompt_send_step state 2 explicit error keyword") {
        env.state = 2;
        s_recv_inject = "SEND FAIL\r\n";
        int rc = at_prompt_send_step(&env, NULL, 0, "SEND OK", NULL,
                                     "SEND FAIL", 5000, 10000);
        TEST_EQ(rc, -1);
    }

    TEST_CASE("Sprint4: prompt_send_step state 2 generic ERROR") {
        env.state = 2;
        s_recv_inject = "ERROR\r\n";
        int rc = at_prompt_send_step(&env, NULL, 0, "SEND OK", NULL, NULL,
                                     5000, 10000);
        TEST_EQ(rc, -1);
    }

    TEST_CASE("Sprint4: prompt_send_step rejects NULL env / NULL ok1") {
        TEST_EQ(at_prompt_send_step(NULL, NULL, 0, "OK", NULL, NULL,
                                    0, 0), -1);
        env.state = 1;
        TEST_EQ(at_prompt_send_step(&env, NULL, 0, NULL, NULL, NULL,
                                    0, 0), -1);
    }

    TEST_CASE("Sprint4: prompt_send_step rejects unexpected state 0/3") {
        env.state = 0;
        TEST_EQ(at_prompt_send_step(&env, NULL, 0, "OK", NULL, NULL,
                                    0, 0), -1);
        env.state = 3;
        TEST_EQ(at_prompt_send_step(&env, NULL, 0, "OK", NULL, NULL,
                                    0, 0), -1);
    }
}

/* ── AT command queue end-to-end behavior ─────────────────────────────── */

static at_resp_code s_callback_code;
static unsigned int s_callback_count;
static unsigned int s_callback_recv_len;
static char s_callback_response[128];
static at_response_t s_error_hook_response;
static unsigned int s_error_hook_count;
static bool s_error_hook_before_callback;

static void command_error_hook(at_response_t *response)
{
    s_error_hook_before_callback = s_callback_count == 0u;
    s_error_hook_response = *response;
    s_error_hook_count++;
}

static void command_callback(at_response_t *response)
{
    unsigned int copy = response->recvcnt;

    if (copy >= sizeof(s_callback_response)) copy = sizeof(s_callback_response) - 1u;
    memcpy(s_callback_response, response->recvbuf, copy);
    s_callback_response[copy] = '\0';
    s_callback_code = response->code;
    s_callback_recv_len = response->recvcnt;
    s_callback_count++;
}

static void command_fixture_reset(void)
{
    XY_MEM_POOL_INIT(s_at_test_mem);
    xy_tick_init();
    fake_reset_io();
    s_callback_code = AT_RESP_OK;
    s_callback_count = 0u;
    s_callback_recv_len = 0u;
    s_callback_response[0] = '\0';
    memset(&s_error_hook_response, 0, sizeof(s_error_hook_response));
    s_error_hook_count = 0u;
    s_error_hook_before_callback = false;
}

static int failing_println_work(at_env_t *env)
{
    static char oversized[AT_MAX_CMD_LEN + 1u];

    if (env->state == 0) {
        memset(oversized, 'X', AT_MAX_CMD_LEN);
        oversized[AT_MAX_CMD_LEN] = '\0';
        env->println(env, "%s", oversized);
        env->state = 1;
    } else {
        env->finish(env, AT_RESP_ERROR);
    }
    return 0;
}

static int double_println_work(at_env_t *env)
{
    if (env->state == 0) {
        env->println(env, "AT+FIRST");
        env->println(env, "AT+SECOND");
        env->state = 1;
    } else {
        env->finish(env, AT_RESP_OK);
    }
    return 0;
}

static void hold_custom_command(at_env_t *env)
{
    (void)env;
}

static unsigned int s_wrap_wait_runs;

static int wrap_wait_work(at_env_t *env)
{
    if (env->state == 0) {
        env->next_wait(env, 300u);
        env->state = 1;
        return 0;
    }
    s_wrap_wait_runs++;
    env->finish(env, AT_RESP_OK);
    return 0;
}

static unsigned int s_queue_order[AT_LIST_WORK_COUNT];
static unsigned int s_queue_order_count;

static void queue_record_work(at_env_t *env)
{
    if (s_queue_order_count < AT_LIST_WORK_COUNT) {
        s_queue_order[s_queue_order_count++] = *(unsigned int *)env->params;
    }
    env->finish(env, AT_RESP_OK);
}

static at_obj_t *command_create_object(void)
{
    static const at_adapter_t adapter = {
        .lock = NULL,
        .unlock = NULL,
        .write = fake_write,
        .read = fake_read,
        .error = NULL,
        .debug = NULL,
#if AT_URC_WARCH_EN
        .urc_bufsize = 128u,
#endif
        .recv_bufsize = 128u,
        .rx_pending = fake_rx_pending,
        .tx_idle = fake_tx_idle,
    };

    return at_obj_create(&adapter);
}

static at_obj_t *command_create_error_object(void)
{
    static const at_adapter_t adapter = {
        .lock = NULL,
        .unlock = NULL,
        .write = fake_write,
        .read = fake_read,
        .error = command_error_hook,
        .debug = NULL,
#if AT_URC_WARCH_EN
        .urc_bufsize = 128u,
#endif
        .recv_bufsize = 128u,
        .rx_pending = fake_rx_pending,
        .tx_idle = fake_tx_idle,
    };

    return at_obj_create(&adapter);
}

static unsigned int count_command_writes(const char *command)
{
    unsigned int count = 0u;
    size_t command_len = strlen(command);

    for (unsigned int i = 0u; i + command_len <= s_fake_write_len; i++) {
        if (memcmp(&s_fake_write_buf[i], command, command_len) == 0) count++;
    }
    return count;
}

static unsigned int s_fake_urc_count;
static unsigned int s_fake_urc_timeout_count;

static int fake_prompt_urc_handler(at_urc_info_t *info)
{
    s_fake_urc_count++;
    if (info->status == URC_RECV_TIMEOUT) s_fake_urc_timeout_count++;
    return 0;
}

static const urc_item_t s_fake_prompt_urc_table[] = {
    { "+FAKE: >", '\n', fake_prompt_urc_handler },
    { "+FAKE: SEND OK", '\n', fake_prompt_urc_handler },
    { "+FAKE: ERROR", '\n', fake_prompt_urc_handler },
};

static int fake_partial_urc_handler(at_urc_info_t *info)
{
    s_fake_urc_count++;
    if (info->status == URC_RECV_TIMEOUT) {
        s_fake_urc_timeout_count++;
        return 0;
    }
    return info->urclen <= 10 ? 7 : 0;
}

static const urc_item_t s_fake_partial_urc_table[] = {
    { "+FAKEBIN", '\n', fake_partial_urc_handler },
};

static int fake_partial_send_ok_urc_handler(at_urc_info_t *info)
{
    s_fake_urc_count++;
    if (info->status == URC_RECV_TIMEOUT) {
        s_fake_urc_timeout_count++;
        return 0;
    }
    return info->urclen <= 10 ? 9 : 0;
}

static const urc_item_t s_fake_partial_send_ok_urc_table[] = {
    { "+FAKEBIN", '\n', fake_partial_send_ok_urc_handler },
};

static unsigned int s_prefix_short_count;
static unsigned int s_prefix_long_count;

static int prefix_short_handler(at_urc_info_t *info)
{
    (void)info;
    s_prefix_short_count++;
    return 0;
}

static int prefix_long_handler(at_urc_info_t *info)
{
    (void)info;
    s_prefix_long_count++;
    return 0;
}

static const urc_item_t s_prefix_overlap_table[] = {
    { "+SIM:", '\n', prefix_short_handler },
    { "+SIM: LONG", '\n', prefix_long_handler },
};

static int prompt_keyword_isolation_work(at_env_t *env)
{
    static const char payload[] = "ABC";

    if (env->state == 0) {
        env->println(env, "AT+SEND");
        env->reset_timer(env);
        env->state = 1;
        return 0;
    }
    int rc = at_prompt_send_step(env, payload, sizeof(payload) - 1u,
                                 "SEND OK", NULL, NULL, 5000u, 10000u);
    if (rc > 0) env->finish(env, AT_RESP_OK);
    else if (rc < 0) env->finish(env, AT_RESP_ERROR);
    return 0;
}

static void test_at_command_queue(void)
{
    TEST_CASE("AT create: rejects missing adapter callbacks") {
        at_adapter_t adapter = {0};

        command_fixture_reset();
        TEST_TRUE(at_obj_create(NULL) == NULL);
        TEST_TRUE(at_obj_create(&adapter) == NULL);
        adapter.write = fake_write;
        TEST_TRUE(at_obj_create(&adapter) == NULL);
    }

    TEST_CASE("AT queue: priority full abort and immediate static pool reuse") {
        at_context_t contexts[AT_LIST_WORK_COUNT];
        unsigned int ids[AT_LIST_WORK_COUNT];
        at_attr_t attr;
        at_obj_t *at;

        command_fixture_reset();
        at = command_create_object();
        TEST_TRUE(at != NULL);
        if (at != NULL) {
            s_queue_order_count = 0u;
            for (unsigned int i = 0u; i < AT_LIST_WORK_COUNT; i++) {
                ids[i] = i;
                at_attr_deinit(&attr);
                at_context_init(&contexts[i], NULL, 0u);
                at_context_attach(&attr, &contexts[i]);
                attr.params = &ids[i];
                attr.priority = i < AT_LIST_WORK_COUNT / 2u
                                ? AT_PRIORITY_LOW : AT_PRIORITY_HIGH;
                TEST_TRUE(at_custom_cmd(at, &attr, queue_record_work));
            }
            TEST_FALSE(at_custom_cmd(at, NULL, queue_record_work));

            at_work_abort_all(at);
            TEST_FALSE(at_obj_busy(at));
            for (unsigned int i = 0u; i < AT_LIST_WORK_COUNT; i++) {
                TEST_EQ(at_work_get_state(&contexts[i]), AT_WORK_STAT_ABORT);
                TEST_EQ(at_work_get_result(&contexts[i]), AT_RESP_ABORT);
            }

            for (unsigned int i = 0u; i < AT_LIST_WORK_COUNT; i++) {
                at_attr_deinit(&attr);
                attr.params = &ids[i];
                attr.priority = i < AT_LIST_WORK_COUNT / 2u
                                ? AT_PRIORITY_LOW : AT_PRIORITY_HIGH;
                TEST_TRUE(at_custom_cmd(at, &attr, queue_record_work));
            }
            TEST_FALSE(at_custom_cmd(at, NULL, queue_record_work));
            while (at_obj_busy(at)) at_obj_process(at);

            TEST_EQ(s_queue_order_count, AT_LIST_WORK_COUNT);
            for (unsigned int i = 0u; i < AT_LIST_WORK_COUNT / 2u; i++) {
                TEST_EQ(s_queue_order[i], i + AT_LIST_WORK_COUNT / 2u);
                TEST_EQ(s_queue_order[i + AT_LIST_WORK_COUNT / 2u], i);
            }
            at_obj_destroy(at);
        }
    }

    TEST_CASE("AT queue: multiple objects share pool without cross-object abort") {
        at_context_t contexts[8];
        unsigned int ids[8];
        at_attr_t attr;
        at_obj_t *objects[3];

        command_fixture_reset();
        objects[0] = command_create_object();
        objects[1] = command_create_object();
        objects[2] = command_create_object();
        TEST_TRUE(objects[0] != NULL);
        TEST_TRUE(objects[1] != NULL);
        TEST_TRUE(objects[2] != NULL);
        if (objects[0] != NULL && objects[1] != NULL && objects[2] != NULL) {
            for (unsigned int i = 0u; i < 8u; i++) {
                unsigned int object = i < 3u ? 0u : (i < 6u ? 1u : 2u);

                ids[i] = i;
                at_attr_deinit(&attr);
                at_context_init(&contexts[i], NULL, 0u);
                at_context_attach(&attr, &contexts[i]);
                attr.params = &ids[i];
                TEST_TRUE(at_custom_cmd(objects[object], &attr, queue_record_work));
            }
            TEST_FALSE(at_custom_cmd(objects[0], NULL, queue_record_work));

            at_work_abort_all(objects[1]);
            for (unsigned int i = 0u; i < 8u; i++) {
                at_work_state expected = i >= 3u && i < 6u
                                         ? AT_WORK_STAT_ABORT
                                         : AT_WORK_STAT_READY;
                TEST_EQ(at_work_get_state(&contexts[i]), expected);
            }
            TEST_TRUE(at_obj_busy(objects[0]));
            TEST_FALSE(at_obj_busy(objects[1]));
            TEST_TRUE(at_obj_busy(objects[2]));

            for (unsigned int i = 0u; i < 3u; i++) {
                at_attr_deinit(&attr);
                attr.params = &ids[3u + i];
                TEST_TRUE(at_custom_cmd(objects[1], &attr, queue_record_work));
            }
            while (at_obj_busy(objects[0]) || at_obj_busy(objects[1]) ||
                   at_obj_busy(objects[2])) {
                at_obj_process(objects[0]);
                at_obj_process(objects[1]);
                at_obj_process(objects[2]);
            }
            TEST_TRUE(at_obj_pm_can_sleep(objects[0]));
            TEST_TRUE(at_obj_pm_can_sleep(objects[1]));
            TEST_TRUE(at_obj_pm_can_sleep(objects[2]));
        }
        at_obj_destroy(objects[0]);
        at_obj_destroy(objects[1]);
        at_obj_destroy(objects[2]);
    }

    TEST_CASE("AT destroy aborts active and queued contexts then reuses pool") {
        at_context_t contexts[AT_LIST_WORK_COUNT];
        unsigned int ids[AT_LIST_WORK_COUNT];
        at_attr_t attr;
        at_obj_t *at;

        command_fixture_reset();
        at = command_create_object();
        TEST_TRUE(at != NULL);
        if (at != NULL) {
            for (unsigned int i = 0u; i < AT_LIST_WORK_COUNT; i++) {
                ids[i] = i;
                at_attr_deinit(&attr);
                at_context_init(&contexts[i], NULL, 0u);
                at_context_attach(&attr, &contexts[i]);
                attr.params = &ids[i];
                TEST_TRUE(at_custom_cmd(at, &attr, hold_custom_command));
            }
            at_obj_process(at);
            TEST_EQ(at_work_get_state(&contexts[0]), AT_WORK_STAT_RUN);

            at_obj_destroy(at);
            for (unsigned int i = 0u; i < AT_LIST_WORK_COUNT; i++) {
                TEST_EQ(at_work_get_state(&contexts[i]), AT_WORK_STAT_ABORT);
                TEST_EQ(at_work_get_result(&contexts[i]), AT_RESP_ABORT);
            }

            at = command_create_object();
            TEST_TRUE(at != NULL);
            if (at != NULL) {
                for (unsigned int i = 0u; i < AT_LIST_WORK_COUNT; i++) {
                    at_attr_deinit(&attr);
                    attr.params = &ids[i];
                    TEST_TRUE(at_custom_cmd(at, &attr, queue_record_work));
                }
                TEST_FALSE(at_custom_cmd(at, NULL, queue_record_work));
                at_work_abort_all(at);
                TEST_FALSE(at_obj_busy(at));
                at_obj_destroy(at);
            }
        }
    }

    TEST_CASE("AT queue: command appends CRLF and accepts fragmented response") {
        at_attr_t attr;
        at_obj_t *at;

        command_fixture_reset();
        at = command_create_object();
        TEST_TRUE(at != NULL);
        if (at != NULL) {
            at_attr_deinit(&attr);
            attr.cb = command_callback;
            attr.prefix = "+CSQ:";
            attr.timeout = 50u;
            attr.retry = 0u;
            TEST_TRUE(at_exec_cmd(at, &attr, "AT+CSQ"));
            TEST_TRUE(at_obj_busy(at));

            at_obj_process(at);
            TEST_EQ(s_fake_write_len, 8u);
            TEST_MEM_EQ(s_fake_write_buf, "AT+CSQ\r\n", 8u);

            fake_inject("\r\n+CSQ: 18");
            at_obj_process(at);
            TEST_EQ(s_callback_count, 0u);
            fake_inject(",0\r\nOK\r\n");
            at_obj_process(at);
            TEST_EQ(s_callback_count, 1u);
            TEST_EQ(s_callback_code, AT_RESP_OK);
            TEST_TRUE(strstr(s_callback_response, "+CSQ: 18,0") != NULL);
            TEST_TRUE(s_callback_recv_len > 0u);
            TEST_FALSE(at_obj_busy(at));
            at_obj_destroy(at);
        }
    }

    TEST_CASE("AT error hook receives initialized response before callback") {
        unsigned int params = 0x12345678u;
        at_attr_t attr;
        at_obj_t *at;

        command_fixture_reset();
        at = command_create_error_object();
        TEST_TRUE(at != NULL);
        if (at != NULL) {
            at_attr_deinit(&attr);
            attr.cb = command_callback;
            attr.params = &params;
            attr.retry = 0u;
            TEST_TRUE(at_exec_cmd(at, &attr, "AT+FAIL"));
            at_obj_process(at);
            fake_inject("\r\nERROR\r\n");
            at_obj_process(at);

            TEST_EQ(s_error_hook_count, 1u);
            TEST_TRUE(s_error_hook_before_callback);
            TEST_EQ(s_callback_count, 1u);
            TEST_TRUE(s_error_hook_response.obj == at);
            TEST_TRUE(s_error_hook_response.params == &params);
            TEST_EQ(s_error_hook_response.code, AT_RESP_ERROR);
            TEST_EQ(s_error_hook_response.recvcnt, 9u);
            TEST_TRUE(s_error_hook_response.recvbuf != NULL);
            TEST_TRUE(strstr(s_error_hook_response.recvbuf, "ERROR") != NULL);
            TEST_FALSE(at_obj_busy(at));
            at_obj_destroy(at);
        }
    }

    TEST_CASE("AT queue: idle input does not pollute next command response") {
        at_attr_t attr;
        at_obj_t *at;

        command_fixture_reset();
        at = command_create_object();
        TEST_TRUE(at != NULL);
        if (at != NULL) {
            fake_inject("\r\nIDLE-NOISE\r\n");
            at_obj_process(at);

            at_attr_deinit(&attr);
            attr.cb = command_callback;
            attr.timeout = 50u;
            attr.retry = 0u;
            TEST_TRUE(at_exec_cmd(at, &attr, "AT"));
            at_obj_process(at);
            fake_inject("\r\nOK\r\n");
            at_obj_process(at);

            TEST_EQ(s_callback_count, 1u);
            TEST_EQ(s_callback_code, AT_RESP_OK);
            TEST_TRUE(strstr(s_callback_response, "IDLE-NOISE") == NULL);
            TEST_FALSE(at_obj_busy(at));
            at_obj_destroy(at);
        }
    }

    TEST_CASE("AT URC isolation: payload keyword cannot satisfy prompt") {
        at_obj_t *at;

        command_fixture_reset();
        s_fake_urc_count = 0u;
        at = command_create_object();
        TEST_TRUE(at != NULL);
        if (at != NULL) {
            at_obj_set_urc(at, s_fake_prompt_urc_table,
                           sizeof(s_fake_prompt_urc_table) /
                           sizeof(s_fake_prompt_urc_table[0]));
            TEST_TRUE(at_do_work(at, NULL, prompt_keyword_isolation_work));
            at_obj_process(at);
            TEST_MEM_EQ(s_fake_write_buf, "AT+SEND\r\n", 9u);

            fake_inject("\r\n+FAKE: ");
            at_obj_process(at);
            TEST_EQ(s_fake_write_len, 9u);
            fake_inject(">\n");
            at_obj_process(at);
            TEST_EQ(s_fake_urc_count, 1u);
            TEST_EQ(s_fake_write_len, 9u);

            fake_inject(">");
            at_obj_process(at);
            TEST_EQ(s_fake_write_len, 12u);
            TEST_MEM_EQ(&s_fake_write_buf[9], "ABC", 3u);

            fake_inject("\r\nSEND OK\r\n");
            at_obj_process(at);
            TEST_FALSE(at_obj_busy(at));
            at_obj_destroy(at);
        }
    }

    TEST_CASE("AT URC: longest anchored prefix wins independent of table order") {
        at_obj_t *at;

        command_fixture_reset();
        s_prefix_short_count = 0u;
        s_prefix_long_count = 0u;
        at = command_create_object();
        TEST_TRUE(at != NULL);
        if (at != NULL) {
            at_obj_set_urc(at, s_prefix_overlap_table,
                           sizeof(s_prefix_overlap_table) /
                           sizeof(s_prefix_overlap_table[0]));
            fake_inject("\r\n+SIM: LONG FIRST\r\n");
            at_obj_process(at);
            fake_inject("\r\n+SIM: SHORT\r\n");
            at_obj_process(at);
            fake_inject("\r\n+SIM: LONG EMBED +SIM: SHORT\r\n");
            at_obj_process(at);
            fake_inject("\r\n+SIMX: MALFORMED\r\n");
            at_obj_process(at);

            TEST_EQ(s_prefix_long_count, 2u);
            TEST_EQ(s_prefix_short_count, 1u);
            TEST_FALSE(at_obj_busy(at));
            at_obj_destroy(at);
        }
    }

    TEST_CASE("AT URC isolation: payload keyword cannot satisfy send result") {
        at_obj_t *at;

        command_fixture_reset();
        s_fake_urc_count = 0u;
        at = command_create_object();
        TEST_TRUE(at != NULL);
        if (at != NULL) {
            at_obj_set_urc(at, s_fake_prompt_urc_table,
                           sizeof(s_fake_prompt_urc_table) /
                           sizeof(s_fake_prompt_urc_table[0]));
            TEST_TRUE(at_do_work(at, NULL, prompt_keyword_isolation_work));
            at_obj_process(at);
            fake_inject(">");
            at_obj_process(at);
            TEST_EQ(s_fake_write_len, 12u);

            fake_inject("\r\n+FAKE: SEND OK\n");
            at_obj_process(at);
            TEST_EQ(s_fake_urc_count, 1u);
            TEST_TRUE(at_obj_busy(at));

            fake_inject("\r\nSEND OK\r\n");
            at_obj_process(at);
            TEST_FALSE(at_obj_busy(at));
            at_obj_destroy(at);
        }
    }

    TEST_CASE("AT URC isolation: payload keyword cannot fail send result") {
        at_obj_t *at;

        command_fixture_reset();
        s_fake_urc_count = 0u;
        at = command_create_object();
        TEST_TRUE(at != NULL);
        if (at != NULL) {
            at_obj_set_urc(at, s_fake_prompt_urc_table,
                           sizeof(s_fake_prompt_urc_table) /
                           sizeof(s_fake_prompt_urc_table[0]));
            TEST_TRUE(at_do_work(at, NULL, prompt_keyword_isolation_work));
            at_obj_process(at);
            fake_inject(">");
            at_obj_process(at);
            TEST_EQ(s_fake_write_len, 12u);

            fake_inject("\r\n+FAKE: ERROR\n");
            at_obj_process(at);
            TEST_EQ(s_fake_urc_count, 1u);
            TEST_TRUE(at_obj_busy(at));

            fake_inject("\r\nSEND OK\r\n");
            at_obj_process(at);
            TEST_FALSE(at_obj_busy(at));
            at_obj_destroy(at);
        }
    }

    TEST_CASE("AT URC isolation: partial error waits for timeout cleanup") {
        at_obj_t *at;

        command_fixture_reset();
        s_fake_urc_count = 0u;
        s_fake_urc_timeout_count = 0u;
        at = command_create_object();
        TEST_TRUE(at != NULL);
        if (at != NULL) {
            at_obj_set_urc(at, s_fake_partial_urc_table,
                           sizeof(s_fake_partial_urc_table) /
                           sizeof(s_fake_partial_urc_table[0]));
            TEST_TRUE(at_do_work(at, NULL, prompt_keyword_isolation_work));
            at_obj_process(at);
            fake_inject(">");
            at_obj_process(at);
            TEST_EQ(s_fake_write_len, 12u);

            fake_inject("\r\n+FAKEBIN\nERROR");
            at_obj_process(at);
            TEST_EQ(s_fake_urc_count, 1u);
            TEST_TRUE(at_obj_busy(at));

            xy_tick_advance(xy_tick_from_ms(499u));
            at_obj_process(at);
            TEST_EQ(s_fake_urc_timeout_count, 0u);
            TEST_TRUE(at_obj_busy(at));

            xy_tick_advance(xy_tick_from_ms(1u));
            at_obj_process(at);
            TEST_EQ(s_fake_urc_timeout_count, 0u);
            TEST_TRUE(at_obj_busy(at));

            xy_tick_advance(xy_tick_from_ms(1u));
            at_obj_process(at);
            TEST_EQ(s_fake_urc_count, 2u);
            TEST_EQ(s_fake_urc_timeout_count, 1u);
            TEST_TRUE(at_obj_busy(at));

            fake_inject("\r\nSEND OK\r\n");
            at_obj_process(at);
            TEST_FALSE(at_obj_busy(at));
            at_obj_destroy(at);
        }
    }

    TEST_CASE("AT URC isolation: response can supply partial URC tail") {
        at_obj_t *at;

        command_fixture_reset();
        s_fake_urc_count = 0u;
        s_fake_urc_timeout_count = 0u;
        at = command_create_object();
        TEST_TRUE(at != NULL);
        if (at != NULL) {
            at_obj_set_urc(at, s_fake_partial_urc_table,
                           sizeof(s_fake_partial_urc_table) /
                           sizeof(s_fake_partial_urc_table[0]));
            TEST_TRUE(at_do_work(at, NULL, prompt_keyword_isolation_work));
            at_obj_process(at);
            fake_inject(">");
            at_obj_process(at);
            TEST_EQ(s_fake_write_len, 12u);

            fake_inject("\r\n+FAKEBIN\nERROR");
            at_obj_process(at);
            TEST_EQ(s_fake_urc_count, 1u);
            TEST_TRUE(at_obj_busy(at));

            xy_tick_advance(xy_tick_from_ms(300u));
            fake_inject("\r\nSEND OK\r\n");
            at_obj_process(at);
            TEST_EQ(s_fake_urc_count, 2u);
            TEST_EQ(s_fake_urc_timeout_count, 0u);
            TEST_FALSE(at_obj_busy(at));
            at_obj_destroy(at);
        }
    }

    TEST_CASE("AT URC isolation: partial SEND OK cannot hide real error") {
        at_obj_t *at;

        command_fixture_reset();
        s_fake_urc_count = 0u;
        s_fake_urc_timeout_count = 0u;
        at = command_create_object();
        TEST_TRUE(at != NULL);
        if (at != NULL) {
            at_obj_set_urc(at, s_fake_partial_send_ok_urc_table,
                           sizeof(s_fake_partial_send_ok_urc_table) /
                           sizeof(s_fake_partial_send_ok_urc_table[0]));
            TEST_TRUE(at_do_work(at, NULL, prompt_keyword_isolation_work));
            at_obj_process(at);
            fake_inject(">");
            at_obj_process(at);
            TEST_EQ(s_fake_write_len, 12u);

            fake_inject("\r\n+FAKEBIN\nSEND OK");
            at_obj_process(at);
            TEST_EQ(s_fake_urc_count, 1u);
            TEST_TRUE(at_obj_busy(at));

            xy_tick_advance(xy_tick_from_ms(501u));
            at_obj_process(at);
            TEST_EQ(s_fake_urc_count, 2u);
            TEST_EQ(s_fake_urc_timeout_count, 1u);
            TEST_TRUE(at_obj_busy(at));

            xy_tick_advance(xy_tick_from_ms(199u));
            fake_inject("\r\nERROR\r\n");
            at_obj_process(at);
            TEST_FALSE(at_obj_busy(at));
            at_obj_destroy(at);
        }
    }

    TEST_CASE("AT URC isolation: real error can supply fake success tail") {
        at_obj_t *at;

        command_fixture_reset();
        s_fake_urc_count = 0u;
        s_fake_urc_timeout_count = 0u;
        at = command_create_object();
        TEST_TRUE(at != NULL);
        if (at != NULL) {
            at_obj_set_urc(at, s_fake_partial_send_ok_urc_table,
                           sizeof(s_fake_partial_send_ok_urc_table) /
                           sizeof(s_fake_partial_send_ok_urc_table[0]));
            TEST_TRUE(at_do_work(at, NULL, prompt_keyword_isolation_work));
            at_obj_process(at);
            fake_inject(">");
            at_obj_process(at);
            TEST_EQ(s_fake_write_len, 12u);

            fake_inject("\r\n+FAKEBIN\nSEND OK");
            at_obj_process(at);
            TEST_EQ(s_fake_urc_count, 1u);
            TEST_TRUE(at_obj_busy(at));

            xy_tick_advance(xy_tick_from_ms(300u));
            fake_inject("\r\nERROR\r\n");
            at_obj_process(at);
            TEST_EQ(s_fake_urc_count, 2u);
            TEST_EQ(s_fake_urc_timeout_count, 0u);
            TEST_FALSE(at_obj_busy(at));
            at_obj_destroy(at);
        }
    }

    TEST_CASE("AT queue: ERROR performs initial send plus two delayed retries") {
        at_attr_t attr;
        at_obj_t *at;

        command_fixture_reset();
        at = command_create_object();
        TEST_TRUE(at != NULL);
        if (at != NULL) {
            at_attr_deinit(&attr);
            attr.cb = command_callback;
            attr.timeout = 50u;
            attr.retry = 2u;
            TEST_TRUE(at_exec_cmd(at, &attr, "AT+FAIL"));

            for (unsigned int attempt = 0u; attempt < 3u; attempt++) {
                at_obj_process(at);
                fake_inject("\r\nERROR\r\n");
                at_obj_process(at);
                if (attempt < 2u) {
                    xy_tick_advance(xy_tick_from_ms(101u));
                    at_obj_process(at);
                }
            }

            TEST_EQ(count_command_writes("AT+FAIL\r\n"), 3u);
            TEST_EQ(s_callback_count, 1u);
            TEST_EQ(s_callback_code, AT_RESP_ERROR);
            TEST_FALSE(at_obj_busy(at));
            at_obj_destroy(at);
        }
    }

    TEST_CASE("AT queue: timeout performs initial send plus two retries") {
        at_attr_t attr;
        at_obj_t *at;

        command_fixture_reset();
        at = command_create_object();
        TEST_TRUE(at != NULL);
        if (at != NULL) {
            at_attr_deinit(&attr);
            attr.cb = command_callback;
            attr.timeout = 10u;
            attr.retry = 2u;
            TEST_TRUE(at_exec_cmd(at, &attr, "AT+NORESP"));

            for (unsigned int attempt = 0u; attempt < 3u; attempt++) {
                at_obj_process(at);
                xy_tick_advance(xy_tick_from_ms(11u));
                at_obj_process(at);
            }

            TEST_EQ(count_command_writes("AT+NORESP\r\n"), 3u);
            TEST_EQ(s_callback_count, 1u);
            TEST_EQ(s_callback_code, AT_RESP_TIMEOUT);
            TEST_FALSE(at_obj_busy(at));
            at_obj_destroy(at);
        }
    }

    TEST_CASE("AT timing: next_wait and timeout retry survive uint32 wrap") {
        at_attr_t attr;
        at_obj_t *at;

        command_fixture_reset();
        at = command_create_object();
        TEST_TRUE(at != NULL);
        if (at != NULL) {
            xy_tick_set(0xffffff00u);
            s_wrap_wait_runs = 0u;
            TEST_TRUE(at_do_work(at, NULL, wrap_wait_work));
            at_obj_process(at);
            xy_tick_advance(299u);
            at_obj_process(at);
            TEST_EQ(s_wrap_wait_runs, 0u);
            xy_tick_advance(2u);
            at_obj_process(at);
            TEST_EQ(s_wrap_wait_runs, 1u);
            TEST_TRUE(xy_tick_now() < 0xffffff00u);

            xy_tick_set(0xffffff00u);
            at_attr_deinit(&attr);
            attr.cb = command_callback;
            attr.timeout = 300u;
            attr.retry = 1u;
            TEST_TRUE(at_exec_cmd(at, &attr, "AT+WRAP"));
            for (unsigned int attempt = 0u; attempt < 2u; attempt++) {
                at_obj_process(at);
                xy_tick_advance(301u);
                at_obj_process(at);
            }
            TEST_EQ(count_command_writes("AT+WRAP\r\n"), 2u);
            TEST_EQ(s_callback_count, 1u);
            TEST_EQ(s_callback_code, AT_RESP_TIMEOUT);
            TEST_TRUE(xy_tick_now() < 0xffffff00u);
            TEST_FALSE(at_obj_busy(at));
            at_obj_destroy(at);
        }
    }

    TEST_CASE("AT queue: late retry response cannot complete next prefixed work") {
        at_attr_t attr;
        at_obj_t *at;

        command_fixture_reset();
        at = command_create_object();
        TEST_TRUE(at != NULL);
        if (at != NULL) {
            at_attr_deinit(&attr);
            attr.cb = command_callback;
            attr.prefix = "+SIMLATEA:";
            attr.timeout = 500u;
            attr.retry = 1u;
            TEST_TRUE(at_exec_cmd(at, &attr, "AT+SIMLATEA"));
            at_obj_process(at);

            xy_tick_advance(xy_tick_from_ms(501u));
            at_obj_process(at);
            at_obj_process(at);
            TEST_EQ(count_command_writes("AT+SIMLATEA\r\n"), 2u);

            fake_inject("\r\n+SIMLATEA: FIRST\r\n\r\nOK\r\n");
            at_obj_process(at);
            TEST_EQ(s_callback_count, 1u);
            TEST_EQ(s_callback_code, AT_RESP_OK);
            TEST_FALSE(at_obj_busy(at));

            at_attr_deinit(&attr);
            attr.cb = command_callback;
            attr.prefix = "+SIMLATEB:";
            attr.timeout = 1000u;
            attr.retry = 0u;
            TEST_TRUE(at_exec_cmd(at, &attr, "AT+SIMLATEB"));
            at_obj_process(at);

            fake_inject("\r\n+SIMLATEA: SECOND\r\n\r\nOK\r\n");
            at_obj_process(at);
            TEST_EQ(s_callback_count, 1u);
            TEST_TRUE(at_obj_busy(at));

            fake_inject("\r\n+SIMLATEB: CURRENT\r\n\r\nOK\r\n");
            at_obj_process(at);
            TEST_EQ(s_callback_count, 2u);
            TEST_EQ(s_callback_code, AT_RESP_OK);
            TEST_TRUE(strstr(s_callback_response, "+SIMLATEA: SECOND") != NULL);
            TEST_TRUE(strstr(s_callback_response, "+SIMLATEB: CURRENT") != NULL);
            TEST_FALSE(at_obj_busy(at));
            at_obj_destroy(at);
        }
    }

    TEST_CASE("AT queue: response buffer boundary reports overflow without wrap") {
        at_attr_t attr;
        at_obj_t *at;
        char response[130];

        command_fixture_reset();
        at = command_create_object();
        TEST_TRUE(at != NULL);
        if (at != NULL) {
            memset(response, 'X', 127u);
            memcpy(response, "+SIMBUF:", 8u);
            memcpy(response + 121u, "\r\nOK\r\n", 6u);

            at_attr_deinit(&attr);
            attr.cb = command_callback;
            attr.prefix = "+SIMBUF:";
            attr.retry = 0u;
            TEST_TRUE(at_exec_cmd(at, &attr, "AT+SIMBUF=127"));
            at_obj_process(at);
            fake_inject_bytes((const unsigned char *)response, 127u);
            at_obj_process(at);
            TEST_EQ(s_callback_count, 0u);
            at_obj_process(at);
            TEST_EQ(s_callback_code, AT_RESP_OK);
            TEST_EQ(s_callback_recv_len, 127u);

            memset(response, 'X', 128u);
            memcpy(response, "+SIMBUF:", 8u);
            memcpy(response + 122u, "\r\nOK\r\n", 6u);
            at_attr_deinit(&attr);
            attr.cb = command_callback;
            attr.prefix = "+SIMBUF:";
            attr.retry = 0u;
            TEST_TRUE(at_exec_cmd(at, &attr, "AT+SIMBUF=128"));
            at_obj_process(at);
            fake_inject_bytes((const unsigned char *)response, 128u);
            at_obj_process(at);
            TEST_EQ(s_callback_code, AT_RESP_OK);
            at_obj_process(at);
            TEST_EQ(s_callback_code, AT_RESP_ERROR);
            TEST_EQ(s_callback_recv_len, 127u);
            TEST_FALSE(at_obj_busy(at));

            at_attr_deinit(&attr);
            attr.cb = command_callback;
            attr.prefix = "+RECOVER:";
            attr.retry = 0u;
            TEST_TRUE(at_exec_cmd(at, &attr, "AT+SIMBUF=RECOVER"));
            at_obj_process(at);
            fake_inject("\r\n+RECOVER: READY\r\n\r\nOK\r\n");
            at_obj_process(at);
            TEST_EQ(s_callback_code, AT_RESP_OK);
            TEST_FALSE(at_obj_busy(at));
            at_obj_destroy(at);
        }
    }

    TEST_CASE("AT queue: short writes resume without loss or duplication") {
        static const unsigned int script[] = {3u, 0u, 2u, 64u};
        at_attr_t attr;
        at_obj_t *at;

        command_fixture_reset();
        memcpy(s_fake_write_script, script, sizeof(script));
        s_fake_write_script_len = sizeof(script) / sizeof(script[0]);
        at = command_create_object();
        TEST_TRUE(at != NULL);
        if (at != NULL) {
            at_attr_deinit(&attr);
            attr.cb = command_callback;
            attr.timeout = 50u;
            attr.retry = 0u;
            TEST_TRUE(at_exec_cmd(at, &attr, "AT+LONG"));

            at_obj_process(at);
            TEST_EQ(s_fake_write_len, 3u);
            at_obj_process(at);
            TEST_EQ(s_fake_write_len, 3u);
            at_obj_process(at);
            TEST_EQ(s_fake_write_len, 5u);
            at_obj_process(at);
            TEST_EQ(s_fake_write_len, 9u);
            TEST_MEM_EQ(s_fake_write_buf, "AT+LONG\r\n", 9u);

            fake_inject("\r\nOK\r\n");
            at_obj_process(at);
            TEST_EQ(s_callback_count, 1u);
            TEST_EQ(s_callback_code, AT_RESP_OK);
            at_obj_destroy(at);
        }
    }

    TEST_CASE("AT queue: stalled write does not start response timeout") {
        static const unsigned int script[] = {0u, 0u, 64u};
        at_attr_t attr;
        at_obj_t *at;

        command_fixture_reset();
        memcpy(s_fake_write_script, script, sizeof(script));
        s_fake_write_script_len = sizeof(script) / sizeof(script[0]);
        at = command_create_object();
        TEST_TRUE(at != NULL);
        if (at != NULL) {
            at_attr_deinit(&attr);
            attr.cb = command_callback;
            attr.timeout = 10u;
            attr.retry = 0u;
            TEST_TRUE(at_exec_cmd(at, &attr, "AT+STALL"));

            at_obj_process(at);
            xy_tick_advance(xy_tick_from_ms(100u));
            at_obj_process(at);
            TEST_EQ(s_callback_count, 0u);
            TEST_TRUE(at_obj_busy(at));
            TEST_FALSE(at_obj_pm_can_sleep(at));

            at_obj_process(at);
            TEST_MEM_EQ(s_fake_write_buf, "AT+STALL\r\n", 10u);
            xy_tick_advance(xy_tick_from_ms(9u));
            at_obj_process(at);
            TEST_EQ(s_callback_count, 0u);
            xy_tick_advance(xy_tick_from_ms(2u));
            at_obj_process(at);
            TEST_EQ(s_callback_count, 1u);
            TEST_EQ(s_callback_code, AT_RESP_TIMEOUT);
            at_obj_destroy(at);
        }
    }

    TEST_CASE("AT queue: rejects a formatted command at the length limit") {
        char command[AT_MAX_CMD_LEN + 1u];
        at_obj_t *at;

        command_fixture_reset();
        memset(command, 'A', AT_MAX_CMD_LEN);
        command[AT_MAX_CMD_LEN] = '\0';
        at = command_create_object();
        TEST_TRUE(at != NULL);
        if (at != NULL) {
            TEST_FALSE(at_exec_cmd(at, NULL, "%s", command));
            TEST_FALSE(at_obj_busy(at));
            at_obj_destroy(at);
        }
    }

    TEST_CASE("AT queue: rejects invalid single and multiline commands") {
        char oversized[AT_MAX_CMD_LEN + 1u];
        const char *multiline[] = {"AT", oversized, NULL};
        at_obj_t *at;

        command_fixture_reset();
        memset(oversized, 'A', AT_MAX_CMD_LEN);
        oversized[AT_MAX_CMD_LEN] = '\0';
        at = command_create_object();
        TEST_TRUE(at != NULL);
        if (at != NULL) {
            TEST_FALSE(at_send_singlline(at, NULL, NULL));
            TEST_FALSE(at_send_singlline(at, NULL, oversized));
            TEST_FALSE(at_send_multiline(at, NULL, NULL));
            TEST_FALSE(at_send_multiline(at, NULL, multiline));
            TEST_FALSE(at_obj_busy(at));
            at_obj_destroy(at);
        }
    }

    TEST_CASE("AT queue: mutated single line fails without staying busy") {
        char command[AT_MAX_CMD_LEN + 1u] = "AT";
        at_attr_t attr;
        at_obj_t *at;

        command_fixture_reset();
        at = command_create_object();
        TEST_TRUE(at != NULL);
        if (at != NULL) {
            at_attr_deinit(&attr);
            attr.cb = command_callback;
            TEST_TRUE(at_send_singlline(at, &attr, command));
            memset(command, 'A', AT_MAX_CMD_LEN);
            command[AT_MAX_CMD_LEN] = '\0';
            at_obj_process(at);
            TEST_EQ(s_callback_count, 1u);
            TEST_EQ(s_callback_code, AT_RESP_ERROR);
            TEST_FALSE(at_obj_busy(at));
            at_obj_destroy(at);
        }
    }

    TEST_CASE("AT queue: failed custom println remains owned by its work") {
        at_obj_t *at;

        command_fixture_reset();
        at = command_create_object();
        TEST_TRUE(at != NULL);
        if (at != NULL) {
            TEST_TRUE(at_do_work(at, NULL, failing_println_work));
            at_obj_process(at);
            TEST_TRUE(at_obj_busy(at));
            at_obj_process(at);
            TEST_FALSE(at_obj_busy(at));
            at_obj_destroy(at);
        }
    }

    TEST_CASE("AT queue: second custom println cannot overwrite pending line") {
        static const unsigned int script[] = {3u, 0u, 64u};
        at_obj_t *at;

        command_fixture_reset();
        memcpy(s_fake_write_script, script, sizeof(script));
        s_fake_write_script_len = sizeof(script) / sizeof(script[0]);
        at = command_create_object();
        TEST_TRUE(at != NULL);
        if (at != NULL) {
            TEST_TRUE(at_do_work(at, NULL, double_println_work));
            at_obj_process(at);
            at_obj_process(at);
            at_obj_process(at);
            TEST_MEM_EQ(s_fake_write_buf, "AT+FIRST\r\n", 10u);
            TEST_EQ(s_fake_write_len, 10u);
            TEST_FALSE(at_obj_busy(at));
            at_obj_destroy(at);
        }
    }

#if AT_RAW_TRANSPARENT_EN
    TEST_CASE("AT raw: short writes resume in both directions") {
        static const unsigned int script[] = {1u, 0u, 64u};
        static const unsigned int peer_script[] = {2u, 0u, 64u};
        static const at_raw_trans_conf_t raw = {
            .exit_cmd = NULL,
            .on_exit = NULL,
            .write = fake_peer_write,
            .read = fake_peer_read,
        };
        at_obj_t *at;

        command_fixture_reset();
        fake_inject("MODEM");
        memcpy(s_peer_read_buf, "HOST", 4u);
        s_peer_read_len = 4u;
        memcpy(s_fake_write_script, script, sizeof(script));
        s_fake_write_script_len = sizeof(script) / sizeof(script[0]);
        memcpy(s_peer_write_script, peer_script, sizeof(peer_script));
        s_peer_write_script_len = sizeof(peer_script) / sizeof(peer_script[0]);
        at = command_create_object();
        TEST_TRUE(at != NULL);
        if (at != NULL) {
            at_raw_transport_enter(at, &raw);
            for (unsigned int i = 0u; i < 5u; i++) at_obj_process(at);
            TEST_EQ(s_peer_write_len, 5u);
            TEST_MEM_EQ(s_peer_write_buf, "MODEM", 5u);
            TEST_EQ(s_fake_write_len, 4u);
            TEST_MEM_EQ(s_fake_write_buf, "HOST", 4u);
            at_raw_transport_exit(at);
            at_obj_destroy(at);
        }
    }


    TEST_CASE("AT raw: exit waits until the current chunk is forwarded") {
        static const unsigned int script[] = {3u, 0u, 64u};
        static const at_raw_trans_conf_t raw = {
            .exit_cmd = "EXIT",
            .on_exit = fake_raw_exit,
            .write = fake_peer_write,
            .read = fake_peer_read,
        };
        at_obj_t *at;

        command_fixture_reset();
        memcpy(s_peer_read_buf, "PRE\rEXIT\r", 9u);
        s_peer_read_len = 9u;
        memcpy(s_fake_write_script, script, sizeof(script));
        s_fake_write_script_len = sizeof(script) / sizeof(script[0]);
        at = command_create_object();
        TEST_TRUE(at != NULL);
        if (at != NULL) {
            s_raw_exit_obj = at;
            at_raw_transport_enter(at, &raw);
            at_obj_process(at);
            TEST_EQ(s_fake_write_len, 3u);
            TEST_TRUE(at_obj_busy(at));
            at_obj_process(at);
            TEST_EQ(s_fake_write_len, 3u);
            at_obj_process(at);
            TEST_EQ(s_fake_write_len, 9u);
            TEST_MEM_EQ(s_fake_write_buf, "PRE\rEXIT\r", 9u);
            TEST_FALSE(at_obj_busy(at));
            s_raw_exit_obj = NULL;
            at_obj_destroy(at);
        }
    }
#endif

    TEST_CASE("AT PM: pending transport RX or TX rejects sleep") {
        at_obj_t *at;

        command_fixture_reset();
        at = command_create_object();
        TEST_TRUE(at != NULL);
        if (at != NULL) {
            TEST_TRUE(at_obj_pm_can_sleep(at));
            s_fake_rx_pending = 1u;
            TEST_FALSE(at_obj_pm_can_sleep(at));
            s_fake_rx_pending = 0u;
            s_fake_tx_idle = false;
            TEST_FALSE(at_obj_pm_can_sleep(at));
            s_fake_tx_idle = true;
            TEST_TRUE(at_obj_pm_can_sleep(at));
            at_obj_destroy(at);
        }
    }
}

/* ── Main entry ────────────────────────────────────────────────────────── */

static void print_usage(const char *prog)
{
    fprintf(stderr,
        "usage: %s [--list] [--filter=SUBSTR]\n"
        "  --list             Print test case names and exit.\n"
        "  --filter=SUBSTR    Run only cases whose name contains SUBSTR.\n",
        prog);
}

int main(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--list") == 0) {
            g_test_list_only = 1;
        } else if (strncmp(argv[i], "--filter=", 9) == 0) {
            g_test_filter = argv[i] + 9;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 2;
        }
    }

    if (!g_test_list_only)
        fprintf(stderr, "==== BareOS audit-fix regression suite ====\n");

    test_ais_sign_extend();
    test_crc_table_widths();
    test_xdigit();
    test_urc_recv_split();
    test_prompt_send_step();
    test_at_command_queue();

    if (g_test_list_only)
        return 0;

    fprintf(stderr, "==== %d tests, %d failures ====\n",
            g_test_count, g_test_failures);
    return g_test_failures;
}
