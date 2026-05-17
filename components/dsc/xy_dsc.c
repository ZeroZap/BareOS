#include "xy_dsc.h"
#include "xy_string.h"
#include "xy_ctype.h"

/* -----------------------------------------------------------------------
 * Internal helpers
 * ----------------------------------------------------------------------- */

/* ASCII hex digit value, or 0xFF on non-hex (shared via xy_ctype). */
#define hex_val(c)  xy_xdigit_val((int8_t)(c))

/* Validate NMEA XOR checksum.  sentence points to '$'.
 * Returns true if checksum matches or sentence has no '*'. */
static bool nmea_checksum_ok(const char *sentence)
{
    /* skip '$' */
    const char *p = sentence + 1;
    uint8_t csum = 0u;
    while (*p && *p != '*') {
        csum ^= (uint8_t)*p;
        p++;
    }
    if (*p != '*') return false;          /* no checksum field */
    p++;
    uint8_t hi = hex_val(p[0]);
    uint8_t lo = hex_val(p[1]);
    if (hi == 0xFFu || lo == 0xFFu) return false;
    return csum == (uint8_t)((hi << 4) | lo);
}

/* Copy at most dst_size-1 chars from src up to stop char or NUL.
 * Returns pointer to next char in src after the stop char (or to NUL). */
static const char *field_copy(char *dst, int dst_size, const char *src, char stop)
{
    int i = 0;
    while (*src && *src != stop && *src != '*') {
        if (i < dst_size - 1) dst[i++] = *src;
        src++;
    }
    dst[i] = '\0';
    if (*src == stop) src++;
    return src;
}

/* Parse a 4-digit HHMM string into hour and minute.
 * Returns false on any non-digit or invalid value. */
static bool parse_hhmm(const char *s, uint8_t *hour, uint8_t *min)
{
    if (s[0] < '0' || s[0] > '9') return false;
    if (s[1] < '0' || s[1] > '9') return false;
    if (s[2] < '0' || s[2] > '9') return false;
    if (s[3] < '0' || s[3] > '9') return false;
    uint8_t h = (uint8_t)((s[0] - '0') * 10u + (uint8_t)(s[1] - '0'));
    uint8_t m = (uint8_t)((s[2] - '0') * 10u + (uint8_t)(s[3] - '0'));
    if (h > 23u || m > 59u) return false;
    *hour = h;
    *min  = m;
    return true;
}

/* Accept "CD", "DS", "HE" talker + "DSC" or "DSE" type.
 * sentence points after '$'.
 * Returns pointer to first field after type+comma, or NULL on mismatch. */
static const char *match_talker_type(const char *sentence, const char *type)
{
    /* talker is 2 chars */
    if (sentence[0] == 'C' && sentence[1] == 'D') { /* CD */ }
    else if (sentence[0] == 'D' && sentence[1] == 'S') { /* DS */ }
    else if (sentence[0] == 'H' && sentence[1] == 'E') { /* HE */ }
    else return NULL;

    const char *p = sentence + 2;
    while (*type) {
        if (*p != *type) return NULL;
        p++; type++;
    }
    if (*p != ',') return NULL;
    return p + 1;
}

/* Parse uint8 from decimal string field (comma/star terminated). */
static uint8_t field_u8(const char *s)
{
    uint16_t v = 0u;
    while (*s && *s != ',' && *s != '*') {
        if (*s >= '0' && *s <= '9') v = (uint16_t)(v * 10u + (uint8_t)(*s - '0'));
        s++;
    }
    return (uint8_t)v;
}

/* -----------------------------------------------------------------------
 * $CDDSC parser
 * Fields: fmt, addr, cat, nat_dist, subarea_or_coords, pos, time, eos, ack
 *
 * Actual 9-field CDDSC layout (0-based from first payload field):
 *   [0] format
 *   [1] address (MMSI)
 *   [2] category
 *   [3] nature/distress type (or subarea)
 *   [4] subarea or position latitude  (lat DDMM+N/S or area code)
 *   [5] position longitude  (lon DDDMM+E/W) — may be empty
 *   [6] time HHMM
 *   [7] EOS
 *   [8] ack type
 * ----------------------------------------------------------------------- */
static bool parse_cddsc(dsc_parser_t *p, const char *fields)
{
    char tmp[16];
    dsc_call_t *c = &p->pending;
    memset(c, 0, sizeof(*c));

    const char *f = fields;

    /* [0] format */
    f = field_copy(tmp, sizeof(tmp), f, ',');
    c->format = field_u8(tmp);

    /* [1] address */
    f = field_copy(c->address, sizeof(c->address), f, ',');

    /* [2] category */
    f = field_copy(tmp, sizeof(tmp), f, ',');
    c->category = field_u8(tmp);

    /* [3] nature / distress type */
    f = field_copy(tmp, sizeof(tmp), f, ',');
    c->distress_type = field_u8(tmp);

    /* [4] latitude or area code */
    f = field_copy(tmp, sizeof(tmp), f, ',');
    /* Detect position: latitude looks like digits then N or S.
     * Minimum: "DDMMN" = 5 chars.  Store in lat_str if it ends with N/S. */
    {
        int len = (int)strlen(tmp);
        if (len >= 5 && (tmp[len - 1] == 'N' || tmp[len - 1] == 'S')) {
            /* copy up to 7 chars */
            int n = len < 7 ? len : 7;
            memcpy(c->lat_str, tmp, (size_t)n);
            c->lat_str[n] = '\0';
        }
    }

    /* [5] longitude — may be empty */
    f = field_copy(tmp, sizeof(tmp), f, ',');
    {
        int len = (int)strlen(tmp);
        if (len >= 6 && (tmp[len - 1] == 'E' || tmp[len - 1] == 'W')) {
            int n = len < 8 ? len : 8;
            memcpy(c->lon_str, tmp, (size_t)n);
            c->lon_str[n] = '\0';
        }
    }

    c->has_position = (c->lat_str[0] != '\0' && c->lon_str[0] != '\0');

    /* [6] time HHMM */
    f = field_copy(tmp, sizeof(tmp), f, ',');
    if (tmp[0] != '\0') {
        parse_hhmm(tmp, &c->utc_hour, &c->utc_min);
    }

    /* remaining fields ([7] EOS, [8] ack) — not stored */

    c->has_expansion = false;
    p->has_pending   = true;
    return true;
}

/* -----------------------------------------------------------------------
 * $CDDSE parser
 * Fields: total, num, ref, mmsi_src, mmsi_rcvd
 * ----------------------------------------------------------------------- */
static bool parse_cddse(dsc_parser_t *p, const char *fields)
{
    if (!p->has_pending) return false;

    char tmp[16];
    const char *f = fields;

    /* [0] total */
    f = field_copy(tmp, sizeof(tmp), f, ',');
    /* [1] num */
    f = field_copy(tmp, sizeof(tmp), f, ',');
    /* [2] ref */
    f = field_copy(tmp, sizeof(tmp), f, ',');

    /* [3] mmsi_src */
    f = field_copy(p->pending.src_mmsi, sizeof(p->pending.src_mmsi), f, ',');

    /* [4] mmsi_rcvd */
    field_copy(p->pending.rcvd_mmsi, sizeof(p->pending.rcvd_mmsi), f, ',');

    p->pending.has_expansion = true;
    return true;
}

/* -----------------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------------- */

void dsc_parser_init(dsc_parser_t *p, dsc_call_cb cb, void *user)
{
    memset(p, 0, sizeof(*p));
    p->cb   = cb;
    p->user = user;
}

bool dsc_feed_sentence(dsc_parser_t *p, const char *sentence)
{
    if (!sentence || sentence[0] != '$') return false;
    if (!nmea_checksum_ok(sentence)) return false;

    const char *body = sentence + 1;   /* skip '$' */

    const char *fields = match_talker_type(body, "DSC");
    if (fields) {
        if (!parse_cddsc(p, fields)) return false;
        if (p->cb) p->cb(&p->pending, p->user);
        return true;
    }

    fields = match_talker_type(body, "DSE");
    if (fields) {
        if (!parse_cddse(p, fields)) return false;
        if (p->cb) p->cb(&p->pending, p->user);
        p->has_pending = false;
        return true;
    }

    return false;
}
