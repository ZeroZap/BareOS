/*
 * xy_nmea.h — Generic NMEA 0183 sentence parser, bare-metal, no heap.
 *
 * Feed bytes one at a time via nmea_feed_byte(). On each complete sentence
 * that passes the XOR checksum, the registered callback is fired with a
 * pointer to the decoded nmea_sentence_t.
 *
 * Supported talker prefixes: GP, GN, GL, GA, GB, GQ (any two-char prefix).
 * Supported sentence types: GGA, RMC, GSA, VTG, GSV.
 * Unknown types are silently discarded.
 *
 * Coordinate format: int32_t × 1e7 (positive = North / East).
 */

#ifndef XY_NMEA_H
#define XY_NMEA_H

#include <stdint.h>
#include <stdbool.h>

/* ── Sentence type tags ───────────────────────────────────────────── */

typedef uint8_t nmea_type_t;
#define NMEA_GGA  1
#define NMEA_RMC  2
#define NMEA_GSA  3
#define NMEA_VTG  4
#define NMEA_GSV  5

/* ── Per-sentence data structures ────────────────────────────────── */

typedef struct {
    int32_t  lat_1e7, lon_1e7;
    uint8_t  quality;      /* 0=invalid, 1=GPS, 2=DGPS, 4=RTK */
    uint8_t  num_sats;
    uint8_t  hdop_x10;     /* HDOP × 10, e.g. 12 → HDOP 1.2 */
    int32_t  alt_cm;       /* altitude above MSL in centimetres */
    uint8_t  hour, min, sec;
} nmea_gga_t;

typedef struct {
    int32_t  lat_1e7, lon_1e7;
    bool     valid;
    uint16_t speed_kn_x10; /* knots × 10 */
    uint16_t course_x10;   /* degrees × 10 */
    uint8_t  hour, min, sec;
    uint8_t  day, month;
    uint16_t year;
} nmea_rmc_t;

typedef struct {
    uint8_t fix_type;   /* 1=none, 2=2D, 3=3D */
    uint8_t pdop_x10;
    uint8_t hdop_x10;
    uint8_t vdop_x10;
} nmea_gsa_t;

typedef struct {
    uint16_t cog_true_x10;  /* course over ground, true, degrees × 10 */
    uint16_t sog_kn_x10;    /* speed over ground, knots × 10 */
    uint16_t sog_kph_x10;   /* speed over ground, km/h × 10 */
} nmea_vtg_t;

typedef struct {
    uint8_t  total_sats;  /* satellites in view (across all GSV messages) */
    uint8_t  best_snr;    /* highest C/N0 seen in this GSV group (dBHz) */
} nmea_gsv_t;

/* ── Union result ─────────────────────────────────────────────────── */

typedef struct {
    nmea_type_t type;
    union {
        nmea_gga_t gga;
        nmea_rmc_t rmc;
        nmea_gsa_t gsa;
        nmea_vtg_t vtg;
        nmea_gsv_t gsv;
    };
} nmea_sentence_t;

/* ── Callback ─────────────────────────────────────────────────────── */

/* Fired once per valid, checksum-verified sentence. Called from nmea_feed_byte()
 * context — keep the handler short (copy data, set a flag). */
typedef void (*nmea_sentence_cb)(const nmea_sentence_t *s, void *user);

/* ── Parser state ─────────────────────────────────────────────────── */

#define NMEA_LINE_MAX 100

typedef struct {
    char             buf[NMEA_LINE_MAX + 2];
    uint8_t          len;
    uint8_t          xor_accum;    /* running XOR while in BODY state */
    uint8_t          state;        /* internal FSM state */
    nmea_sentence_cb cb;
    void            *user;
} nmea_parser_t;

/* ── API ──────────────────────────────────────────────────────────── */

/* Initialise parser. Must be called before the first nmea_feed_byte(). */
void nmea_parser_init(nmea_parser_t *p, nmea_sentence_cb cb, void *user);

/* Feed one byte from the GNSS UART. Safe to call from ISR context.
 * All state lives in *p — no module-level globals. */
void nmea_feed_byte(nmea_parser_t *p, uint8_t ch);

#endif /* XY_NMEA_H */
