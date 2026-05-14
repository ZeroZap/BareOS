/**
 * @file xy_gnss.c
 * @brief GNSS NMEA parser — no heap, no RTOS, ISR-safe byte feeding.
 *
 * Parses $GPRMC / $GNRMC sentences.  These two sentence types provide
 * all the data needed for a vessel distress report:
 *   - UTC time
 *   - Latitude / Longitude
 *   - Speed over ground
 *   - Fix validity
 */

#include "xy_gnss.h"
#include <string.h>
#include <stddef.h>

/* ── Internal parser state ────────────────────────────────────────── */

static struct {
    /* Byte-stream accumulator */
    char    line[XY_GNSS_NMEA_LINE_MAX];
    uint8_t len;
    bool    in_sentence;  /* set when '$' received */

    /* Latest parsed fix (double-buffered by valid flag) */
    xy_gnss_pos_t pos;
} s;

/* ── Helpers ──────────────────────────────────────────────────────── */

/* Extract the Nth field (0-indexed) from a comma-separated NMEA sentence.
 * Writes field text into buf (max buflen bytes including NUL).
 * Returns true if field exists. */
static bool nmea_field(const char *sentence, int n, char *buf, int buflen)
{
    const char *p = sentence;
    for (int i = 0; i < n; i++) {
        p = strchr(p, ',');
        if (!p) return false;
        p++;
    }
    const char *end = strchr(p, ',');
    if (!end) end = strchr(p, '*');
    if (!end) end = p + strlen(p);

    int len = (int)(end - p);
    if (len >= buflen) len = buflen - 1;
    memcpy(buf, p, (size_t)len);
    buf[len] = '\0';
    return (len > 0);
}

/* Parse NMEA time field "HHMMSS.ss" into hour/minute/second. */
static void parse_time(const char *f, uint8_t *h, uint8_t *m, uint8_t *s)
{
    if (strlen(f) < 6) return;
    *h = (uint8_t)((f[0]-'0')*10 + (f[1]-'0'));
    *m = (uint8_t)((f[2]-'0')*10 + (f[3]-'0'));
    *s = (uint8_t)((f[4]-'0')*10 + (f[5]-'0'));
}

/* Parse NMEA date field "DDMMYY" into day/month/year. */
static void parse_date(const char *f, uint8_t *d, uint8_t *mo, uint16_t *y)
{
    if (strlen(f) < 6) return;
    *d  = (uint8_t)((f[0]-'0')*10 + (f[1]-'0'));
    *mo = (uint8_t)((f[2]-'0')*10 + (f[3]-'0'));
    *y  = (uint16_t)(2000 + (f[4]-'0')*10 + (f[5]-'0'));
}

/* Parse NMEA lat/lon "DDmm.mmmm" + direction into int32 × 1e7. */
static int32_t parse_latlon(const char *value, char dir)
{
    /* Find decimal point to split degrees from minutes. */
    const char *dot = strchr(value, '.');
    if (!dot || dot - value < 3) return 0;

    /* Degrees: everything before the last two digits before the dot. */
    int deg_chars = (int)(dot - value) - 2;
    int32_t deg = 0;
    for (int i = 0; i < deg_chars; i++) {
        deg = deg * 10 + (value[i] - '0');
    }

    /* Minutes integer part (2 digits). */
    int32_t min_int = (value[deg_chars]-'0')*10 + (value[deg_chars+1]-'0');

    /* Minutes fraction (up to 4 decimal digits). */
    int32_t min_frac = 0;
    int frac_digits = 0;
    for (int i = 1; i <= 4 && dot[i] >= '0' && dot[i] <= '9'; i++) {
        min_frac = min_frac * 10 + (dot[i] - '0');
        frac_digits++;
    }
    /* Normalise to 4 decimal places. */
    for (int i = frac_digits; i < 4; i++) min_frac *= 10;

    /* Total minutes × 1e4 */
    int32_t min_total_1e4 = min_int * 10000 + min_frac;

    /* degrees × 1e7 = deg * 1e7 + (min_total_1e4 / 60) * 1e3 */
    int32_t result = deg * 10000000L + (min_total_1e4 * 1000L) / 60;

    if (dir == 'S' || dir == 'W') result = -result;
    return result;
}

/* Verify NMEA checksum: XOR of all bytes between '$' and '*'. */
static bool nmea_checksum_ok(const char *sentence)
{
    const char *p = sentence;
    if (*p == '$') p++;
    uint8_t calc = 0;
    while (*p && *p != '*') {
        calc ^= (uint8_t)*p++;
    }
    if (*p != '*') return false;
    uint8_t given = (uint8_t)(
        ((*(p+1) >= 'A') ? (*(p+1) - 'A' + 10) : (*(p+1) - '0')) * 16 +
        ((*(p+2) >= 'A') ? (*(p+2) - 'A' + 10) : (*(p+2) - '0'))
    );
    return calc == given;
}

/* ── RMC sentence parser ──────────────────────────────────────────── */

static void parse_rmc(const char *sentence)
{
    char f[16];
    xy_gnss_pos_t p = {0};

    /* Field 1: time */
    if (nmea_field(sentence, 1, f, sizeof(f))) parse_time(f, &p.hour, &p.minute, &p.second);

    /* Field 2: status (A=active, V=void) */
    if (!nmea_field(sentence, 2, f, sizeof(f))) return;
    p.valid = (f[0] == 'A');

    /* Field 3+4: latitude */
    char lat_dir[4] = "";
    char lat_val[16] = "";
    nmea_field(sentence, 3, lat_val, sizeof(lat_val));
    nmea_field(sentence, 4, lat_dir, sizeof(lat_dir));
    p.lat_1e7 = parse_latlon(lat_val, lat_dir[0]);

    /* Field 5+6: longitude */
    char lon_dir[4] = "";
    char lon_val[16] = "";
    nmea_field(sentence, 5, lon_val, sizeof(lon_val));
    nmea_field(sentence, 6, lon_dir, sizeof(lon_dir));
    p.lon_1e7 = parse_latlon(lon_val, lon_dir[0]);

    /* Field 7: speed over ground (knots) */
    if (nmea_field(sentence, 7, f, sizeof(f))) {
        int spd = 0;
        for (int i = 0; f[i] >= '0' && f[i] <= '9'; i++) spd = spd*10 + (f[i]-'0');
        p.speed_knots = (uint8_t)spd;
    }

    /* Field 9: date */
    if (nmea_field(sentence, 9, f, sizeof(f))) parse_date(f, &p.day, &p.month, &p.year);

    s.pos = p;
}

/* ── Public API ───────────────────────────────────────────────────── */

void xy_gnss_init(void)
{
    memset(&s, 0, sizeof(s));
}

void xy_gnss_feed_byte(uint8_t byte)
{
    char c = (char)byte;

    if (c == '$') {
        s.in_sentence = true;
        s.len = 0;
        s.line[0] = '$';
        s.len = 1;
        return;
    }

    if (!s.in_sentence) return;

    if (s.len >= XY_GNSS_NMEA_LINE_MAX - 1) {
        s.in_sentence = false;
        s.len = 0;
        return;
    }

    s.line[s.len++] = c;

    if (c == '\n') {
        s.line[s.len] = '\0';
        s.in_sentence = false;

        if (!nmea_checksum_ok(s.line)) {
            s.len = 0;
            return;
        }

        /* Identify sentence type (positions 1–5 after '$'). */
        if (strncmp(s.line + 3, "RMC,", 4) == 0 ||  /* $GPRMC / $GNRMC */
            strncmp(s.line + 2, "RMC,", 4) == 0) {   /* $GPRMC variant  */
            parse_rmc(s.line);
        }
        s.len = 0;
    }
}

bool xy_gnss_parse_at_response(const char *resp, xy_gnss_pos_t *pos)
{
    /* Quectel format: "+QGPSLOC: HHMMSS.s,DDmm.mmmmN/S,DDDmm.mmmmE/W,HDOP,Alt,Fix,..." */
    if (!resp || !pos) return false;
    const char *p = strchr(resp, ' ');
    if (!p) p = resp;
    else p++;

    char fields[8][20];
    int f = 0;
    while (f < 8) {
        const char *end = strchr(p, ',');
        if (!end) end = p + strlen(p);
        int len = (int)(end - p);
        if (len >= 20) len = 19;
        memcpy(fields[f], p, (size_t)len);
        fields[f][len] = '\0';
        f++;
        if (!*end) break;
        p = end + 1;
    }

    if (f < 3) return false;

    parse_time(fields[0], &pos->hour, &pos->minute, &pos->second);

    /* Lat: last char is N/S */
    int llen = (int)strlen(fields[1]);
    if (llen > 1) {
        char dir = fields[1][llen-1];
        fields[1][llen-1] = '\0';
        pos->lat_1e7 = parse_latlon(fields[1], dir);
    }

    /* Lon: last char is E/W */
    int lolen = (int)strlen(fields[2]);
    if (lolen > 1) {
        char dir = fields[2][lolen-1];
        fields[2][lolen-1] = '\0';
        pos->lon_1e7 = parse_latlon(fields[2], dir);
    }

    /* HDOP */
    if (f > 3) {
        int hdop = 0;
        for (int i = 0; fields[3][i] >= '0' && fields[3][i] <= '9'; i++)
            hdop = hdop * 10 + (fields[3][i] - '0');
        pos->hdop_x10 = (uint8_t)(hdop * 10);  /* integer part * 10 */
    }

    /* Altitude */
    if (f > 4) {
        int alt = 0;
        for (int i = 0; fields[4][i] >= '0' && fields[4][i] <= '9'; i++)
            alt = alt * 10 + (fields[4][i] - '0');
        pos->altitude_m = (uint16_t)alt;
    }

    pos->valid = true;
    return true;
}

bool xy_gnss_has_fix(void)
{
    return s.pos.valid;
}

xy_gnss_pos_t xy_gnss_get_pos(void)
{
    return s.pos;
}

xy_gnss_state_t xy_gnss_get_state(void)
{
    return s.pos.valid ? XY_GNSS_STATE_FIX : XY_GNSS_STATE_NO_FIX;
}

int xy_gnss_format_pos(const xy_gnss_pos_t *pos, char *buf, int bufsize)
{
    int32_t lat = pos->lat_1e7 < 0 ? -pos->lat_1e7 : pos->lat_1e7;
    int32_t lon = pos->lon_1e7 < 0 ? -pos->lon_1e7 : pos->lon_1e7;
    char lat_dir = pos->lat_1e7 >= 0 ? 'N' : 'S';
    char lon_dir = pos->lon_1e7 >= 0 ? 'E' : 'W';

    /* Format: "LAT:31.4525100N LON:121.4525100E" */
    int n = 0;
    n += snprintf(buf + n, (size_t)(bufsize - n),
                  "LAT:%ld.%07ld%c LON:%ld.%07ld%c SPD:%ukn UTC:%02u:%02u:%02u",
                  (long)(lat / 10000000L), (long)(lat % 10000000L), lat_dir,
                  (long)(lon / 10000000L), (long)(lon % 10000000L), lon_dir,
                  pos->speed_knots,
                  pos->hour, pos->minute, pos->second);
    return n;
}

void xy_gnss_to_nmea(const xy_gnss_pos_t *pos,
                     char *lat_str, char *lat_dir,
                     char *lon_str, char *lon_dir)
{
    int32_t lat = pos->lat_1e7 < 0 ? -pos->lat_1e7 : pos->lat_1e7;
    int32_t lon = pos->lon_1e7 < 0 ? -pos->lon_1e7 : pos->lon_1e7;

    /* Convert back to DDmm.mmmm */
    int32_t lat_deg  = lat / 10000000L;
    int32_t lat_min_1e4 = (lat % 10000000L) * 60L / 1000L;
    int32_t lon_deg  = lon / 10000000L;
    int32_t lon_min_1e4 = (lon % 10000000L) * 60L / 1000L;

    snprintf(lat_str, 12, "%02ld%02ld.%04ld",
             (long)lat_deg, (long)(lat_min_1e4 / 10000), (long)(lat_min_1e4 % 10000));
    snprintf(lon_str, 12, "%03ld%02ld.%04ld",
             (long)lon_deg, (long)(lon_min_1e4 / 10000), (long)(lon_min_1e4 % 10000));

    *lat_dir = pos->lat_1e7 >= 0 ? 'N' : 'S';
    *lon_dir = pos->lon_1e7 >= 0 ? 'E' : 'W';
}
