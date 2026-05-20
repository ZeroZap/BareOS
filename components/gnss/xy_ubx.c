/*
 * xy_ubx.c — u-blox UBX binary protocol parser implementation.
 */

#include "xy_ubx.h"
#include "xy_string.h"

/* ── FSM states ───────────────────────────────────────────────────── */

#define ST_SYNC1    0u  // waiting for 0xB5
#define ST_SYNC2    1u  // waiting for 0x62
#define ST_CLASS    2u
#define ST_ID       3u
#define ST_LEN_L    4u
#define ST_LEN_H    5u
#define ST_PAYLOAD  6u  // collecting payload_len bytes
#define ST_CK_A     7u
#define ST_CK_B     8u

#define UBX_SYNC1  0xB5u
#define UBX_SYNC2  0x62u

/* ── Fletcher-8 helper ────────────────────────────────────────────── */

/*
 * Fold one byte into the running Fletcher-8 checksum.
 * ck_a and ck_b are passed by pointer so the compiler can inline this
 * and keep them in registers across the payload loop.
 */
static inline void _ck_accum(uint8_t *ck_a, uint8_t *ck_b, uint8_t byte)
{
    *ck_a += byte;
    *ck_b += *ck_a;
}

/* ── Public API ───────────────────────────────────────────────────── */

void ubx_parser_init(ubx_parser_t *p, ubx_msg_cb cb, void *user)
{
    memset(p, 0, sizeof(*p));
    p->cb   = cb;
    p->user = user;
    // state already 0 = ST_SYNC1
}

void ubx_feed_byte(ubx_parser_t *p, uint8_t ch)
{
    switch (p->state) {
    case ST_SYNC1:
        if (ch == UBX_SYNC1) p->state = ST_SYNC2;
        break;

    case ST_SYNC2:
        if (ch == UBX_SYNC2) {
            p->ck_a = 0;
            p->ck_b = 0;
            p->state = ST_CLASS;
        } else {
            // 0xB5 followed by something other than 0x62; check if it's another 0xB5
            p->state = (ch == UBX_SYNC1) ? ST_SYNC2 : ST_SYNC1;
        }
        break;

    case ST_CLASS:
        p->cls = ch;
        _ck_accum(&p->ck_a, &p->ck_b, ch);
        p->state = ST_ID;
        break;

    case ST_ID:
        p->id = ch;
        _ck_accum(&p->ck_a, &p->ck_b, ch);
        p->state = ST_LEN_L;
        break;

    case ST_LEN_L:
        p->payload_len = ch;
        _ck_accum(&p->ck_a, &p->ck_b, ch);
        p->state = ST_LEN_H;
        break;

    case ST_LEN_H:
        p->payload_len |= (uint16_t)((uint16_t)ch << 8);
        _ck_accum(&p->ck_a, &p->ck_b, ch);
        p->payload_idx = 0;
        // If payload exceeds our buffer, we still consume the bytes to stay
        // in sync but do not store them (payload_idx will still advance).
        p->state = (p->payload_len == 0) ? ST_CK_A : ST_PAYLOAD;
        break;

    case ST_PAYLOAD:
        _ck_accum(&p->ck_a, &p->ck_b, ch);
        // Only store if within buffer; otherwise drain silently.
        if (p->payload_idx < UBX_MAX_PAYLOAD) {
            p->buf[p->payload_idx] = ch;
        }
        p->payload_idx++;
        if (p->payload_idx >= p->payload_len) {
            p->state = ST_CK_A;
        }
        break;

    case ST_CK_A:
        // ch is the received CK_A byte
        if (ch != p->ck_a) {
            // checksum mismatch — resync
            p->state = (ch == UBX_SYNC1) ? ST_SYNC2 : ST_SYNC1;
        } else {
            p->state = ST_CK_B;
        }
        break;

    case ST_CK_B:
        if (ch == p->ck_b) {
            // Frame valid. Fire callback only if payload fit in the buffer.
            if (p->payload_len <= UBX_MAX_PAYLOAD && p->cb) {
                p->cb(p->cls, p->id, p->buf, p->payload_len, p->user);
            }
        }
        // Whether the frame was good or bad, restart the FSM.
        p->state = ST_SYNC1;
        break;

    default:
        p->state = ST_SYNC1;
        break;
    }
}

bool ubx_decode_nav_pvt(const uint8_t *payload, uint16_t len, ubx_nav_pvt_t *out)
{
    if (len < (uint16_t)sizeof(ubx_nav_pvt_t)) return false;
    // memcpy handles unaligned access on ARM Cortex-M correctly for packed structs.
    memcpy(out, payload, sizeof(ubx_nav_pvt_t));
    return true;
}

bool ubx_decode_nav_status(const uint8_t *payload, uint16_t len, ubx_nav_status_t *out)
{
    if (len < (uint16_t)sizeof(ubx_nav_status_t)) return false;
    memcpy(out, payload, sizeof(ubx_nav_status_t));
    return true;
}

uint16_t ubx_build_msg(uint8_t *buf, uint8_t cls, uint8_t id,
                       const uint8_t *payload, uint16_t payload_len)
{
    uint8_t ck_a = 0, ck_b = 0;

    buf[0] = UBX_SYNC1;
    buf[1] = UBX_SYNC2;
    buf[2] = cls;
    buf[3] = id;
    buf[4] = (uint8_t)(payload_len & 0xFFu);
    buf[5] = (uint8_t)(payload_len >> 8);

    // checksum covers bytes 2..5+payload_len-1 (class through last payload byte)
    _ck_accum(&ck_a, &ck_b, cls);
    _ck_accum(&ck_a, &ck_b, id);
    _ck_accum(&ck_a, &ck_b, buf[4]);
    _ck_accum(&ck_a, &ck_b, buf[5]);

    for (uint16_t i = 0; i < payload_len; i++) {
        buf[6 + i] = payload[i];
        _ck_accum(&ck_a, &ck_b, payload[i]);
    }

    buf[6 + payload_len]     = ck_a;
    buf[6 + payload_len + 1] = ck_b;

    return (uint16_t)(8u + payload_len);
}
