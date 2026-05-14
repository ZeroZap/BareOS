/*
 * xy_nmea.c — Generic NMEA 0183 sentence parser implementation.
 */

#include "xy_nmea.h"
#include <string.h>

/* ── FSM states ───────────────────────────────────────────────────── */

#define ST_IDLE   0  // waiting for '$' or '!'
#define ST_BODY   1  // accumulating body bytes, running XOR
#define ST_CK1    2  // first hex checksum digit
#define ST_CK2    3  // second hex checksum digit

/* ── Internal helpers ─────────────────────────────────────────────── */

static uint8_t hex_val(uint8_t c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0xFF; // invalid
}

/*
 * Copy the n-th comma-delimited field (0 = sentence tag, 1 = first data field)
 * from buf into out[0..out_len-1] (NUL-terminated).
 * Returns true if the field exists and is non-empty.
 */
static bool _field(const char *buf, uint8_t n, char *out, uint8_t out_len)
{
    const char *p = buf;
    for (uint8_t i = 0; i < n; i++) {
        p = strchr(p, ',');
        if (!p) { out[0] = '\0'; return false; }
        p++;
    }
    const char *end = strchr(p, ',');
    if (!end) end = strchr(p, '*');
    if (!end) end = p + strlen(p);

    uint8_t len = (uint8_t)(end - p);
    if (len == 0) { out[0] = '\0'; return false; }
    if (len >= out_len) len = out_len - 1;
    memcpy(out, p, len);
    out[len] = '\0';
    return true;
}

/*
 * Parse an unsigned decimal ASCII string. Stops at first non-digit.
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

/*
 * Parse "DDmm.mmmmm" (or "DDDmm.mmmmm" for longitude) into int32_t × 1e7.
 * dir is 'N','S','E','W'.
 *
 * Algorithm (integer only):
 *   degrees  = digits before the last two digits before the decimal point
 *   min_int  = two digits immediately before the decimal point
 *   min_frac = up to 5 fractional decimal digits, zero-padded to 5 digits
 *
 *   total_minutes_1e5 = min_int * 100000 + min_frac
 *   result_1e7 = deg * 10000000 + total_minutes_1e5 * 10000000 / 60 / 100000
 *              = deg * 10000000 + total_minutes_1e5 * 100 / 60
 *
 * The multiply total_minutes_1e5 * 100 fits in int32_t for up to 59.99999 min
 * (max = 5999999 * 100 = 599999900 < 2^31). Safe.
 */
static int32_t _parse_coord(const char *s, char dir)
{
    const char *dot = strchr(s, '.');
    if (!dot || (dot - s) < 3) return 0;

    // number of degree digits = total digits before dot minus 2 minute digits
    int deg_digits = (int)(dot - s) - 2;
    int32_t deg = 0;
    for (int i = 0; i < deg_digits; i++) {
        deg = deg * 10 + (s[i] - '0');
    }

    int32_t min_int = (s[deg_digits] - '0') * 10 + (s[deg_digits + 1] - '0');

    // parse up to 5 fractional digits, pad on the right to reach exactly 5
    int32_t min_frac = 0;
    int nfrac = 0;
    for (int i = 1; i <= 5; i++) {
        char fc = dot[i];
        if (fc < '0' || fc > '9') break;
        min_frac = min_frac * 10 + (fc - '0');
        nfrac++;
    }
    for (int i = nfrac; i < 5; i++) min_frac *= 10;

    // total_minutes × 1e5
    int32_t min_1e5 = min_int * 100000L + min_frac;
    // convert to degrees × 1e7: min_1e5 / 60 * 1e7 = min_1e5 * 100 / 60 * ...
    // = min_1e5 * (1e7/60) = min_1e5 * 10000000/60; use: min_1e5*100/6 /10
    // Simplify: result = deg*1e7 + min_1e5*100/60
    int32_t result = deg * 10000000L + (min_1e5 * 100L) / 60L;

    if (dir == 'S' || dir == 'W') result = -result;
    return result;
}

/*
 * Parse "HHMMSS.ss" time field.
 */
static void _parse_time(const char *f, uint8_t *h, uint8_t *m, uint8_t *s)
{
    if (f[0] < '0' || f[5] < '0') return;
    *h = (uint8_t)((f[0] - '0') * 10 + (f[1] - '0'));
    *m = (uint8_t)((f[2] - '0') * 10 + (f[3] - '0'));
    *s = (uint8_t)((f[4] - '0') * 10 + (f[5] - '0'));
}

/*
 * Parse a "D.D" fixed-point field where the caller specifies the scale of
 * the integer result. We want value×10 so we read one decimal digit.
 * E.g. "1.8" → 18,  "12.0" → 120.
 * Returns uint8_t; saturates at 255.
 */
static uint8_t _parse_dop_x10(const char *f)
{
    if (!f || !f[0]) return 0;
    uint32_t ipart = 0;
    const char *p = f;
    while (*p >= '0' && *p <= '9') { ipart = ipart * 10 + (uint32_t)(*p - '0'); p++; }
    uint8_t frac = 0;
    if (*p == '.') {
        p++;
        if (*p >= '0' && *p <= '9') frac = (uint8_t)(*p - '0');
    }
    uint32_t v = ipart * 10u + frac;
    return (uint8_t)(v > 255u ? 255u : v);
}

/* ── Sentence parsers ─────────────────────────────────────────────── */

static void _parse_gga(const char *buf, nmea_sentence_t *out)
{
    char f[16];
    char f2[4];
    out->type = NMEA_GGA;
    nmea_gga_t *g = &out->gga;

    // field 1: HHMMSS.ss
    if (_field(buf, 1, f, sizeof(f))) _parse_time(f, &g->hour, &g->min, &g->sec);

    // fields 2,3: lat value, N/S
    if (_field(buf, 2, f, sizeof(f)) && _field(buf, 3, f2, sizeof(f2)))
        g->lat_1e7 = _parse_coord(f, f2[0]);

    // fields 4,5: lon value, E/W
    if (_field(buf, 4, f, sizeof(f)) && _field(buf, 5, f2, sizeof(f2)))
        g->lon_1e7 = _parse_coord(f, f2[0]);

    // field 6: fix quality
    if (_field(buf, 6, f, sizeof(f))) g->quality = (uint8_t)_parse_uint(f);

    // field 7: number of satellites
    if (_field(buf, 7, f, sizeof(f))) g->num_sats = (uint8_t)_parse_uint(f);

    // field 8: HDOP
    if (_field(buf, 8, f, sizeof(f))) g->hdop_x10 = _parse_dop_x10(f);

    // field 9: altitude (metres, float string); convert to cm via integer part × 100
    // plus one fractional decimal digit × 10.
    if (_field(buf, 9, f, sizeof(f))) {
        const char *p = f;
        int32_t ipart = 0;
        int sign = 1;
        if (*p == '-') { sign = -1; p++; }
        while (*p >= '0' && *p <= '9') { ipart = ipart * 10 + (*p - '0'); p++; }
        int32_t frac_cm = 0;
        if (*p == '.') {
            p++;
            // two fractional digits = centimetres
            if (*p >= '0' && *p <= '9') { frac_cm = (*p - '0') * 10; p++; }
            if (*p >= '0' && *p <= '9') { frac_cm += (*p - '0'); }
        }
        g->alt_cm = sign * (ipart * 100L + frac_cm);
    }
}

static void _parse_rmc(const char *buf, nmea_sentence_t *out)
{
    char f[16];
    char f2[4];
    out->type = NMEA_RMC;
    nmea_rmc_t *r = &out->rmc;

    if (_field(buf, 1, f, sizeof(f))) _parse_time(f, &r->hour, &r->min, &r->sec);

    if (_field(buf, 2, f, sizeof(f))) r->valid = (f[0] == 'A');

    if (_field(buf, 3, f, sizeof(f)) && _field(buf, 4, f2, sizeof(f2)))
        r->lat_1e7 = _parse_coord(f, f2[0]);

    if (_field(buf, 5, f, sizeof(f)) && _field(buf, 6, f2, sizeof(f2)))
        r->lon_1e7 = _parse_coord(f, f2[0]);

    // field 7: speed over ground in knots (float); store as × 10
    if (_field(buf, 7, f, sizeof(f))) {
        const char *p = f;
        uint32_t ipart = 0;
        while (*p >= '0' && *p <= '9') { ipart = ipart * 10 + (uint32_t)(*p - '0'); p++; }
        uint8_t frac = 0;
        if (*p == '.') { p++; if (*p >= '0' && *p <= '9') frac = (uint8_t)(*p - '0'); }
        r->speed_kn_x10 = (uint16_t)(ipart * 10u + frac);
    }

    // field 8: course over ground (float); store as × 10
    if (_field(buf, 8, f, sizeof(f))) {
        const char *p = f;
        uint32_t ipart = 0;
        while (*p >= '0' && *p <= '9') { ipart = ipart * 10 + (uint32_t)(*p - '0'); p++; }
        uint8_t frac = 0;
        if (*p == '.') { p++; if (*p >= '0' && *p <= '9') frac = (uint8_t)(*p - '0'); }
        r->course_x10 = (uint16_t)(ipart * 10u + frac);
    }

    // field 9: DDMMYY
    if (_field(buf, 9, f, sizeof(f)) && strlen(f) >= 6) {
        r->day   = (uint8_t) ((f[0]-'0')*10 + (f[1]-'0'));
        r->month = (uint8_t) ((f[2]-'0')*10 + (f[3]-'0'));
        r->year  = (uint16_t)(2000u + (f[4]-'0')*10u + (f[5]-'0'));
    }
}

static void _parse_gsa(const char *buf, nmea_sentence_t *out)
{
    char f[16];
    out->type = NMEA_GSA;
    nmea_gsa_t *g = &out->gsa;

    // field 2: fix type (1/2/3)
    if (_field(buf, 2, f, sizeof(f))) g->fix_type = (uint8_t)_parse_uint(f);

    // fields 15,16,17: PDOP, HDOP, VDOP
    if (_field(buf, 15, f, sizeof(f))) g->pdop_x10 = _parse_dop_x10(f);
    if (_field(buf, 16, f, sizeof(f))) g->hdop_x10 = _parse_dop_x10(f);
    if (_field(buf, 17, f, sizeof(f))) g->vdop_x10 = _parse_dop_x10(f);
}

static void _parse_vtg(const char *buf, nmea_sentence_t *out)
{
    char f[16];
    out->type = NMEA_VTG;
    nmea_vtg_t *v = &out->vtg;

    // field 1: COG true
    if (_field(buf, 1, f, sizeof(f))) {
        const char *p = f;
        uint32_t ipart = 0;
        while (*p >= '0' && *p <= '9') { ipart = ipart * 10 + (uint32_t)(*p - '0'); p++; }
        uint8_t frac = 0;
        if (*p == '.') { p++; if (*p >= '0' && *p <= '9') frac = (uint8_t)(*p - '0'); }
        v->cog_true_x10 = (uint16_t)(ipart * 10u + frac);
    }

    // field 5: SOG knots
    if (_field(buf, 5, f, sizeof(f))) {
        const char *p = f;
        uint32_t ipart = 0;
        while (*p >= '0' && *p <= '9') { ipart = ipart * 10 + (uint32_t)(*p - '0'); p++; }
        uint8_t frac = 0;
        if (*p == '.') { p++; if (*p >= '0' && *p <= '9') frac = (uint8_t)(*p - '0'); }
        v->sog_kn_x10 = (uint16_t)(ipart * 10u + frac);
    }

    // field 7: SOG km/h
    if (_field(buf, 7, f, sizeof(f))) {
        const char *p = f;
        uint32_t ipart = 0;
        while (*p >= '0' && *p <= '9') { ipart = ipart * 10 + (uint32_t)(*p - '0'); p++; }
        uint8_t frac = 0;
        if (*p == '.') { p++; if (*p >= '0' && *p <= '9') frac = (uint8_t)(*p - '0'); }
        v->sog_kph_x10 = (uint16_t)(ipart * 10u + frac);
    }
}

/*
 * GSV sentences come in a group (multiple messages per talker cycle).
 * Each message contains up to 4 satellite records at fields [4..19].
 * SNR is at offsets +3 within each 4-field block: fields 7, 11, 15, 19.
 * We accumulate best_snr across all messages and report total_sats from
 * field 3 of any message (all messages in the group carry the same value).
 *
 * Because GSV arrives as multiple frames, and we don't want per-group state
 * in the parser (no module globals), we report each GSV sentence individually.
 * The caller can accumulate best_snr across callbacks for the same talker.
 */
static void _parse_gsv(const char *buf, nmea_sentence_t *out)
{
    char f[8];
    out->type = NMEA_GSV;
    nmea_gsv_t *g = &out->gsv;

    // field 3: total satellites in view
    if (_field(buf, 3, f, sizeof(f))) g->total_sats = (uint8_t)_parse_uint(f);

    // SNR fields: 7, 11, 15, 19 (one per satellite record)
    uint8_t best = 0;
    static const uint8_t snr_fields[4] = {7, 11, 15, 19};
    for (uint8_t i = 0; i < 4; i++) {
        if (_field(buf, snr_fields[i], f, sizeof(f)) && f[0] != '\0') {
            uint8_t snr = (uint8_t)_parse_uint(f);
            if (snr > best) best = snr;
        }
    }
    g->best_snr = best;
}

/* ── Dispatch after checksum passes ──────────────────────────────── */

static void _dispatch(nmea_parser_t *p)
{
    /*
     * buf holds "$GPxxx,...*HH\0" (or $GN, $GL, etc.)
     * The sentence tag starts at buf[1] (skip '$').
     * Talker is two chars [1..2], type is three chars [3..5].
     * We accept any two-char talker and match the three-char type.
     */
    const char *tag = p->buf + 3; // points at first char of 3-letter type
    nmea_sentence_t s;
    memset(&s, 0, sizeof(s));

    if      (strncmp(tag, "GGA", 3) == 0) _parse_gga(p->buf, &s);
    else if (strncmp(tag, "RMC", 3) == 0) _parse_rmc(p->buf, &s);
    else if (strncmp(tag, "GSA", 3) == 0) _parse_gsa(p->buf, &s);
    else if (strncmp(tag, "VTG", 3) == 0) _parse_vtg(p->buf, &s);
    else if (strncmp(tag, "GSV", 3) == 0) _parse_gsv(p->buf, &s);
    else return; // unknown type, silently discard

    if (p->cb) p->cb(&s, p->user);
}

/* ── Public API ───────────────────────────────────────────────────── */

void nmea_parser_init(nmea_parser_t *p, nmea_sentence_cb cb, void *user)
{
    memset(p, 0, sizeof(*p));
    p->cb   = cb;
    p->user = user;
}

void nmea_feed_byte(nmea_parser_t *p, uint8_t ch)
{
    switch (p->state) {
    case ST_IDLE:
        if (ch == '$' || ch == '!') {
            p->len        = 0;
            p->xor_accum  = 0;
            p->buf[p->len++] = (char)ch;
            p->state      = ST_BODY;
        }
        break;

    case ST_BODY:
        if (ch == '$' || ch == '!') {
            // framing error — restart
            p->len       = 0;
            p->xor_accum = 0;
            p->buf[p->len++] = (char)ch;
            break;
        }
        if (ch == '\r' || ch == '\n') {
            // sentence ended without '*' checksum — discard
            p->state = ST_IDLE;
            break;
        }
        if (ch == '*') {
            // stop XOR-ing; next two bytes are the checksum
            p->buf[p->len++] = '*';
            p->state = ST_CK1;
            break;
        }
        if (p->len >= NMEA_LINE_MAX) {
            // sentence too long — discard and wait for next '$'
            p->state = ST_IDLE;
            break;
        }
        // accumulate body byte into XOR (everything between '$' and '*')
        p->xor_accum ^= ch;
        p->buf[p->len++] = (char)ch;
        break;

    case ST_CK1:
        p->buf[p->len++] = (char)ch;
        p->state = ST_CK2;
        break;

    case ST_CK2: {
        p->buf[p->len] = '\0';
        // compare parsed checksum to accumulated XOR
        uint8_t hi = hex_val(p->buf[p->len - 1]); // CK1 char
        uint8_t lo = hex_val((uint8_t)ch);          // CK2 char
        if (hi != 0xFF && lo != 0xFF) {
            uint8_t given = (uint8_t)((hi << 4) | lo);
            if (given == p->xor_accum) {
                p->buf[p->len++] = (char)ch;
                p->buf[p->len]   = '\0';
                _dispatch(p);
            }
        }
        p->state = ST_IDLE;
        break;
    }

    default:
        p->state = ST_IDLE;
        break;
    }
}
