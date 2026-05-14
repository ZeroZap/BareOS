/*
 * xy_ais.c — AIS NMEA sentence parser implementation.
 *
 * No heap, no floats, no RTOS dependencies.
 * All state lives in the caller-supplied ais_parser_t.
 *
 * Decoding references:
 *   ITU-R M.1371-5 — Technical characteristics for an AIS
 *   IEC 62287-1     — Class B AIS
 *   NMEA 0183 standard v4.11
 */

#include "xy_ais.h"
#include <string.h>

/* ── Internal limits ─────────────────────────────────────────────────── */

/* Maximum assembled payload across all sentences of one message.
 * Type 5 needs 71 × 6-bit chars = 426 bits.  Type 21 (AtoN) needs ~360 bits.
 * Two sentences of 56 chars = 112 chars total — enough for all types. */
#define AIS_MAX_ASSEMBLED_PAYLOAD (AIS_PAYLOAD_MAX * 2u)  /* 112 chars */

/* ── Internal helpers ────────────────────────────────────────────────── */

/*
 * Convert a single hex character to its nibble value.
 * Returns 0xFF on invalid input.
 */
static uint8_t _hex_nibble(char c)
{
    if (c >= '0' && c <= '9') return (uint8_t)(c - '0');
    if (c >= 'A' && c <= 'F') return (uint8_t)(c - 'A' + 10);
    if (c >= 'a' && c <= 'f') return (uint8_t)(c - 'a' + 10);
    return 0xFFu;
}

/*
 * Locate the n-th comma-separated field pointer within a raw NMEA sentence
 * body (everything after the leading '!', before '*').
 *
 * Returns a pointer to the first character of the field, or NULL if the
 * field index is out of range.  The returned pointer remains valid for
 * reading until the next comma, '*', or NUL.
 */
static const char *_field_ptr(const char *sentence, uint8_t n)
{
    const char *p = sentence;
    /* Skip leading '!' */
    if (*p == '!') p++;

    for (uint8_t i = 0; i < n; i++) {
        p = strchr(p, ',');
        if (!p) return NULL;
        p++;
    }
    return p;
}

/*
 * Copy the n-th comma-separated field into out[0..out_len-1], NUL-terminated.
 * Returns the field length (0 if empty or not found).
 */
static uint8_t _field_copy(const char *sentence, uint8_t n,
                           char *out, uint8_t out_len)
{
    const char *p = _field_ptr(sentence, n);
    if (!p) { out[0] = '\0'; return 0; }

    const char *end = p;
    while (*end && *end != ',' && *end != '*' && *end != '\r' && *end != '\n') {
        end++;
    }

    uint8_t len = (uint8_t)(end - p);
    if (len == 0) { out[0] = '\0'; return 0; }
    if (len >= out_len) len = (uint8_t)(out_len - 1u);
    memcpy(out, p, len);
    out[len] = '\0';
    return len;
}

/*
 * Parse an ASCII decimal unsigned integer from a NUL/comma/asterisk-terminated
 * string.  Returns the value; stops at first non-digit.
 */
static uint32_t _parse_uint(const char *s)
{
    uint32_t v = 0;
    while (*s >= '0' && *s <= '9') {
        v = v * 10u + (uint32_t)(*s - '0');
        s++;
    }
    return v;
}

/* ── NMEA checksum validation ────────────────────────────────────────── */

/*
 * Validate the NMEA *XX checksum.
 * XOR of all bytes between '!' (exclusive) and '*' (exclusive).
 *
 * sentence must be NUL-terminated and start with '!'.
 * Returns true if checksum is present and correct.
 */
static bool _nmea_checksum_ok(const char *sentence)
{
    const char *p = sentence;
    if (*p == '!') p++;   /* skip leading delimiter */

    uint8_t xor_val = 0;
    while (*p && *p != '*') {
        xor_val ^= (uint8_t)*p;
        p++;
    }

    if (*p != '*') return false;  /* no checksum field */
    p++;

    uint8_t hi = _hex_nibble(p[0]);
    uint8_t lo = _hex_nibble(p[1]);
    if (hi == 0xFFu || lo == 0xFFu) return false;

    uint8_t given = (uint8_t)((hi << 4) | lo);
    return given == xor_val;
}

/* ── 6-bit payload decoding ──────────────────────────────────────────── */

/*
 * Decode AIS payload ASCII characters into an array of 6-bit values.
 * Each ASCII char c → v = c - 48; if v > 40 then v -= 8.
 * Result is in range [0, 63]; stored one value per byte (upper 2 bits = 0).
 *
 * payload_ascii — pointer to first payload character in raw sentence.
 * len           — number of payload characters.
 * out           — output array, must hold at least len bytes.
 */
static void _decode_payload(const char *payload_ascii, uint8_t len,
                            uint8_t *out)
{
    for (uint8_t i = 0; i < len; i++) {
        uint8_t v = (uint8_t)(payload_ascii[i] - 48u);
        if (v > 40u) v -= 8u;
        out[i] = v;
    }
}

/* ── Public bit-extraction primitives ───────────────────────────────── */

uint32_t ais_getbits(const uint8_t *payload, uint16_t bit_offset,
                     uint8_t bit_count)
{
    uint32_t result = 0;
    for (uint8_t i = 0; i < bit_count; i++) {
        uint16_t char_idx = (uint16_t)((bit_offset + i) / 6u);
        uint8_t  bit_pos  = (uint8_t)(5u - ((bit_offset + i) % 6u));
        result = (result << 1) | ((payload[char_idx] >> bit_pos) & 1u);
    }
    return result;
}

int32_t ais_getbits_signed(const uint8_t *payload, uint16_t bit_offset,
                            uint8_t bit_count)
{
    uint32_t v = ais_getbits(payload, bit_offset, bit_count);
    /* Sign-extend: if the MSB of the field is set, fill upper bits with 1s */
    if (v & (1u << (bit_count - 1u))) {
        v |= ~((1u << bit_count) - 1u);
    }
    return (int32_t)v;
}

void ais_get_text(const uint8_t *payload, uint16_t bit_offset,
                  uint8_t char_count, char *out)
{
    uint8_t i;
    for (i = 0; i < char_count; i++) {
        uint8_t val = (uint8_t)ais_getbits(payload, bit_offset + (uint16_t)(i * 6u), 6u);
        /* AIS 6-bit ASCII: 0–31 maps to '@'–'_' (+64); 32–63 maps to ' '–'_' (keep) */
        out[i] = (char)((val < 32u) ? (val + 64u) : val);
    }
    out[char_count] = '\0';

    /* Trim trailing spaces */
    int32_t last = (int32_t)char_count - 1;
    while (last >= 0 && out[last] == ' ') {
        out[last] = '\0';
        last--;
    }
}

/* ── Message decoders (static) ───────────────────────────────────────── */

/*
 * Decode Class A position report — message types 1, 2, 3.
 * Minimum required payload: 168 bits = 28 chars.
 */
static void _decode_pos_a(const uint8_t *payload, uint8_t payload_len,
                           ais_message_t *msg)
{
    if (payload_len < 28u) return;  /* need at least 168 bits */

    ais_pos_report_t *r = &msg->pos_a;

    r->mmsi         = ais_getbits(payload,  8u, 30u);
    r->nav_status   = (uint8_t)ais_getbits(payload, 38u,  4u);
    r->sog_x10      = (uint16_t)ais_getbits(payload, 50u, 10u);
    /* bit 60: position accuracy (ignored) */
    r->lon_1e4      = ais_getbits_signed(payload,  61u, 28u);
    r->lat_1e4      = ais_getbits_signed(payload,  89u, 27u);
    r->cog_x10      = (uint16_t)ais_getbits(payload, 116u, 12u);
    r->true_heading = (uint16_t)ais_getbits(payload, 128u,  9u);
    r->second       = (uint8_t)ais_getbits(payload, 137u,  6u);
    /* bit 148: RAIM */
    r->raim         = (ais_getbits(payload, 148u, 1u) != 0u);
}

/*
 * Decode Class B position report — message type 18.
 * Minimum required payload: 168 bits = 28 chars.
 */
static void _decode_pos_b(const uint8_t *payload, uint8_t payload_len,
                           ais_message_t *msg)
{
    if (payload_len < 28u) return;

    ais_pos_class_b_t *r = &msg->pos_b;

    r->mmsi         = ais_getbits(payload,   8u, 30u);
    /* bits 38-45: reserved */
    r->sog_x10      = (uint16_t)ais_getbits(payload,  46u, 10u);
    /* bit 56: position accuracy */
    r->lon_1e4      = ais_getbits_signed(payload,  57u, 28u);
    r->lat_1e4      = ais_getbits_signed(payload,  85u, 27u);
    r->cog_x10      = (uint16_t)ais_getbits(payload, 112u, 12u);
    r->true_heading = (uint16_t)ais_getbits(payload, 124u,  9u);
    /* bit 133-138: timestamp (ignored) */
    /* bit 139-140: regional reserved */
    r->cs_unit      = (ais_getbits(payload, 141u, 1u) != 0u);
    /* bits 142-146: display, DSC, band, msg22 flags — not stored */
    /* bit 147: assigned */
    r->raim         = (ais_getbits(payload, 147u, 1u) != 0u);
}

/*
 * Decode static and voyage data — message type 5.
 * Requires 2 assembled sentences (≥ 71 chars = 426 bits).
 */
static void _decode_type5(const uint8_t *payload, uint8_t payload_len,
                           ais_message_t *msg)
{
    if (payload_len < 71u) return;  /* need at least 426 bits */

    ais_static_voyage_t *r = &msg->static_v;

    r->mmsi         = ais_getbits(payload,   8u, 30u);
    r->imo          = ais_getbits(payload,  40u, 30u);

    ais_get_text(payload,  70u,  7u, r->callsign);   /* 42 bits, 7 chars */
    ais_get_text(payload, 112u, 20u, r->name);        /* 120 bits, 20 chars */

    r->ship_type     = (uint8_t)ais_getbits(payload, 232u,  8u);
    r->dim_bow       = (uint16_t)ais_getbits(payload, 240u, 9u);
    r->dim_stern     = (uint16_t)ais_getbits(payload, 249u, 9u);
    r->dim_port      = (uint16_t)ais_getbits(payload, 258u, 6u);
    r->dim_starboard = (uint16_t)ais_getbits(payload, 264u, 6u);
    /* bits 270-273: EPFD type */
    /* bits 274-291: ETA (month, day, hour, minute) — not stored */
    r->draught_x10   = (uint8_t)ais_getbits(payload, 292u, 8u);

    ais_get_text(payload, 300u, 20u, r->destination); /* 120 bits, 20 chars */
}

/*
 * Decode safety-related broadcast message — message type 14.
 * Text starts at bit 40; variable length up to 968 bits (161 × 6-bit chars).
 */
static void _decode_type14(const uint8_t *payload, uint8_t payload_len,
                            ais_message_t *msg)
{
    ais_safety_bcast_t *r = &msg->safety;

    r->mmsi = ais_getbits(payload, 8u, 30u);

    /* Text starts at bit 40; max 161 characters */
    uint16_t text_bits = (uint16_t)payload_len * 6u;
    uint16_t avail_bits = (text_bits > 40u) ? (text_bits - 40u) : 0u;
    uint8_t  nchars = (uint8_t)(avail_bits / 6u);
    if (nchars > 160u) nchars = 160u;

    if (nchars > 0u) {
        ais_get_text(payload, 40u, nchars, r->text);
    } else {
        r->text[0] = '\0';
    }
}

/*
 * Decode Aid-to-Navigation report — message type 21.
 * Minimum required payload: 360 bits = 60 chars (name only variant).
 * With name extension the full record is 269 bits = 45 chars; allow 45.
 */
static void _decode_type21(const uint8_t *payload, uint8_t payload_len,
                            ais_message_t *msg)
{
    if (payload_len < 45u) return;  /* need at least 270 bits */

    ais_aid_nav_t *r = &msg->aid_nav;

    r->type_of_aid  = (uint8_t)ais_getbits(payload,  38u,  5u);
    ais_get_text(payload, 43u, 20u, r->name);        /* 120 bits, 20 chars */
    /* bit 163: position accuracy */
    r->lon_1e4      = ais_getbits_signed(payload, 164u, 28u);
    r->lat_1e4      = ais_getbits_signed(payload, 192u, 27u);
    r->mmsi         = ais_getbits(payload,          8u, 30u);
    /* bits 219-248: dimensions (not stored) */
    /* bits 249-252: EPFD type */
    /* bits 253-258: UTC second */
    r->off_position = (ais_getbits(payload, 259u, 1u) != 0u);
    /* bits 260-267: regional reserved */
    r->raim         = (ais_getbits(payload, 268u, 1u) != 0u);
}

/* ── Dispatch: decode assembled payload into ais_message_t ──────────── */

static void _dispatch(ais_parser_t *p, const uint8_t *payload,
                      uint8_t payload_len, uint8_t fill_bits)
{
    (void)fill_bits;  /* available for future use; ignored by current decoders */

    if (payload_len == 0u || !p->cb) return;

    uint8_t msg_type = (uint8_t)ais_getbits(payload, 0u, 6u);

    ais_message_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.type = msg_type;

    switch (msg_type) {
    case 1u:
    case 2u:
    case 3u:
        _decode_pos_a(payload, payload_len, &msg);
        break;

    case 5u:
        _decode_type5(payload, payload_len, &msg);
        break;

    case 14u:
        _decode_type14(payload, payload_len, &msg);
        break;

    case 18u:
        _decode_pos_b(payload, payload_len, &msg);
        break;

    case 21u:
        _decode_type21(payload, payload_len, &msg);
        break;

    default:
        /* Unsupported message type — still fire callback so caller can log */
        break;
    }

    p->cb(&msg, p->user);
}

/* ── Multi-part slot management ─────────────────────────────────────── */

/*
 * Find a slot matching seq_id, or return the index of an empty slot.
 * If all slots are occupied by different seq_ids, evict slot 0 (oldest).
 * Returns slot index 0..AIS_SEQ_SLOTS-1.
 */
static uint8_t _find_slot(ais_parser_t *p, uint8_t seq_id)
{
    /* Look for existing slot with matching seq_id */
    for (uint8_t i = 0; i < AIS_SEQ_SLOTS; i++) {
        if (p->slots[i].total > 0u && p->slots[i].seq_id == seq_id) {
            return i;
        }
    }
    /* Look for an empty (unused) slot */
    for (uint8_t i = 0; i < AIS_SEQ_SLOTS; i++) {
        if (p->slots[i].total == 0u) {
            return i;
        }
    }
    /* All slots in use: evict slot 0 */
    memset(&p->slots[0], 0, sizeof(p->slots[0]));
    return 0u;
}

/* ── Public API ──────────────────────────────────────────────────────── */

void ais_parser_init(ais_parser_t *p, ais_msg_cb cb, void *user)
{
    memset(p, 0, sizeof(*p));
    p->cb   = cb;
    p->user = user;
}

bool ais_feed_sentence(ais_parser_t *p, const char *sentence)
{
    if (!p || !sentence) return false;

    /* ── 1. Find the start of the sentence ──────────────────────────── */
    const char *s = sentence;
    /* Strip any leading whitespace */
    while (*s == ' ' || *s == '\t') s++;

    if (*s != '!') return false;

    /* ── 2. Validate NMEA checksum ───────────────────────────────────── */
    if (!_nmea_checksum_ok(s)) return false;

    /* ── 3. Verify talker / sentence ID (AIVDM or AIVDO) ────────────── */
    char tag[8];
    if (_field_copy(s, 0u, tag, sizeof(tag)) == 0u) return false;
    if (strncmp(tag, "AIVDM", 5u) != 0 && strncmp(tag, "AIVDO", 5u) != 0) {
        return false;
    }

    /* ── 4. Parse header fields ──────────────────────────────────────── */
    char field[8];

    /* Field 1: total sentences in this message (1-based) */
    if (_field_copy(s, 1u, field, sizeof(field)) == 0u) return false;
    uint8_t total = (uint8_t)_parse_uint(field);
    /* Practical AIS limit: type 5 static data uses 2 sentences; nothing
     * in the standard exceeds 5.  Our assembled buffer fits 2 × AIS_PAYLOAD_MAX
     * chars.  Reject messages requiring more sentences than we can buffer. */
    if (total == 0u || total > 2u) return false;

    /* Field 2: sentence number within this message (1-based) */
    if (_field_copy(s, 2u, field, sizeof(field)) == 0u) return false;
    uint8_t sentence_num = (uint8_t)_parse_uint(field);
    if (sentence_num == 0u || sentence_num > total) return false;

    /* Field 3: sequential message ID (0–9, or empty for single-sentence) */
    uint8_t seq_id = 0u;
    if (_field_copy(s, 3u, field, sizeof(field)) > 0u) {
        seq_id = (uint8_t)_parse_uint(field);
    }

    /* Field 4: channel (A or B) — accepted but not stored */
    /* Field 5: payload characters */
    const char *payload_start = _field_ptr(s, 5u);
    if (!payload_start) return false;

    /* Measure payload length (up to next comma or '*') */
    const char *pp = payload_start;
    while (*pp && *pp != ',' && *pp != '*' && *pp != '\r' && *pp != '\n') pp++;
    uint8_t plen = (uint8_t)(pp - payload_start);
    if (plen == 0u || plen > AIS_PAYLOAD_MAX) return false;

    /* Field 6: fill bits (0–5) */
    if (_field_copy(s, 6u, field, sizeof(field)) == 0u) return false;
    uint8_t fill_bits = (uint8_t)_parse_uint(field);
    if (fill_bits > 5u) return false;

    /* ── 5. Decode payload ASCII → 6-bit values ─────────────────────── */
    uint8_t decoded[AIS_PAYLOAD_MAX];
    _decode_payload(payload_start, plen, decoded);

    /* ── 6. Single-sentence fast path ───────────────────────────────── */
    if (total == 1u) {
        _dispatch(p, decoded, plen, fill_bits);
        return true;
    }

    /* ── 7. Multi-part assembly ──────────────────────────────────────── */
    uint8_t slot_idx = _find_slot(p, seq_id);

    /* Use a named pointer to the slot entry within the parser struct */
#define SLOT (p->slots[slot_idx])

    /* If this is sentence 1 (first of a group), (re-)initialise the slot */
    if (sentence_num == 1u) {
        memset(&SLOT, 0, sizeof(SLOT));
        SLOT.seq_id = seq_id;
        SLOT.total  = total;
    } else {
        /* Verify this sentence belongs to the slot we found */
        if (SLOT.total == 0u || SLOT.seq_id != seq_id) {
            /* Out-of-order fragment without a prior part-1 — discard */
            return false;
        }
        /* Guard against duplicate or out-of-sequence delivery */
        if (sentence_num != (uint8_t)(SLOT.received + 1u)) {
            /* Reset slot and discard */
            memset(&SLOT, 0, sizeof(SLOT));
            return false;
        }
    }

    /* Append decoded payload — check for overflow */
    if ((uint16_t)SLOT.payload_len + plen > AIS_MAX_ASSEMBLED_PAYLOAD) {
        memset(&SLOT, 0, sizeof(SLOT));
        return false;
    }
    memcpy(SLOT.payload + SLOT.payload_len, decoded, plen);
    SLOT.payload_len = (uint8_t)(SLOT.payload_len + plen);
    SLOT.received++;

    /* Update fill_bits only for the last sentence */
    if (sentence_num == total) {
        SLOT.fill_bits = fill_bits;
    }

    /* ── 8. Complete? ────────────────────────────────────────────────── */
    if (SLOT.received == SLOT.total) {
        uint8_t final_len  = SLOT.payload_len;
        uint8_t final_fill = SLOT.fill_bits;

        /* Copy assembled payload to stack before clearing slot (avoids
         * any aliasing if the callback calls ais_feed_sentence() again). */
        uint8_t assembled[AIS_MAX_ASSEMBLED_PAYLOAD];
        memcpy(assembled, SLOT.payload, final_len);

        /* Release slot before invoking callback */
        memset(&SLOT, 0, sizeof(SLOT));

        _dispatch(p, assembled, final_len, final_fill);
    }

#undef SLOT

    return true;
}
