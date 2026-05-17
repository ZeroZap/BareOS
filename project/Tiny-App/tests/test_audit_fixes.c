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
}

/* ── Sprint 4: at_prompt_send_step state machine ───────────────────────── */

/* Fake adapter to record the raw bytes written during the prompt sequence */
static uint8_t s_fake_write_buf[64];
static unsigned s_fake_write_len = 0;
static unsigned int fake_write(const void *data, unsigned int len)
{
    if (len > sizeof(s_fake_write_buf)) len = sizeof(s_fake_write_buf);
    memcpy(s_fake_write_buf, data, len);
    s_fake_write_len = len;
    return len;
}
static unsigned int fake_read(void *data, unsigned int len)
{ (void)data; (void)len; return 0; }

static at_adapter_t s_fake_adap = {
    .lock = NULL, .unlock = NULL,
    .write = fake_write, .read = fake_read,
};

/* Fake at_env_t that just returns programmable values */
static char *s_recv_inject = "";
static int   s_state_done  = 0;

static char *fake_contains(at_env_t *e, const char *s)
{ (void)e; return strstr(s_recv_inject, s); }
static bool fake_is_timeout(at_env_t *e, unsigned int ms)
{ (void)e; (void)ms; return false; }
static void fake_println(at_env_t *e, const char *fmt, ...) { (void)e; (void)fmt; }
static void fake_recvclr(at_env_t *e) { (void)e; s_recv_inject = ""; }
static void fake_reset_timer(at_env_t *e) { (void)e; }
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

    if (g_test_list_only)
        return 0;

    fprintf(stderr, "==== %d tests, %d failures ====\n",
            g_test_count, g_test_failures);
    return g_test_failures;
}
