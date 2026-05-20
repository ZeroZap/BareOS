/**
 * @file xy_gnss.h
 * @brief GNSS NMEA parser — bare-metal, no heap allocation.
 *
 * Supports two operating modes:
 *
 * Mode A — NMEA stream (passive, no AT interaction):
 *   The GNSS module outputs $GPRMC / $GNRMC sentences continuously.
 *   Feed each received byte to xy_gnss_feed_byte(); the parser assembles
 *   complete sentences and updates the fix structure automatically.
 *
 *   // In UART2 RX ISR:
 *   xy_gnss_feed_byte(uart2_rx_byte());
 *
 *   // In main loop:
 *   if (xy_gnss_has_fix()) {
 *       xy_gnss_pos_t pos = xy_gnss_get_pos();
 *   }
 *
 * Mode B — AT-controlled (active, via AT-Command-V2):
 *   Issue AT+QGPSLOC or equivalent through g_at_gnss and parse the
 *   response using xy_gnss_parse_at_response().  Not required when the
 *   module streams NMEA natively.
 *
 * Coordinate format:
 *   latitude  — degrees × 1e7 (positive = North, negative = South)
 *   longitude — degrees × 1e7 (positive = East,  negative = West)
 *   Using integer × 1e7 avoids floating-point on MCUs without FPU.
 */

#ifndef XY_GNSS_H
#define XY_GNSS_H

#include <stdint.h>
#include "xy_typedef.h"

/* ── Position fix ─────────────────────────────────────────────────── */

typedef struct {
    int32_t  lat_1e7;       /* latitude  × 1e7, e.g. 314525000 = 31.4525° N */
    int32_t  lon_1e7;       /* longitude × 1e7, e.g. 1214525000 = 121.4525° E */
    uint16_t altitude_m;    /* altitude above sea level (metres)             */
    uint8_t  speed_knots;   /* speed over ground (knots, integer part)       */
    uint8_t  hdop_x10;      /* horizontal DOP × 10 (e.g. 12 = HDOP 1.2)     */
    uint8_t  satellites;    /* number of satellites used                     */
    /* UTC time */
    uint8_t  hour;
    uint8_t  minute;
    uint8_t  second;
    uint8_t  day;
    uint8_t  month;
    uint16_t year;
    bool     valid;         /* true = Active fix ('A' in RMC status field)   */
} xy_gnss_pos_t;

/* ── Module state ─────────────────────────────────────────────────── */

typedef enum {
    XY_GNSS_STATE_NO_FIX  = 0,
    XY_GNSS_STATE_FIX     = 1,
    XY_GNSS_STATE_TIMEOUT = 2,   /* set by application after fix_timeout_ms */
} xy_gnss_state_t;

/* ── Configuration ────────────────────────────────────────────────── */

#ifndef XY_GNSS_NMEA_LINE_MAX
#define XY_GNSS_NMEA_LINE_MAX  100   /* max chars in one NMEA sentence */
#endif

/* ── Lifecycle ────────────────────────────────────────────────────── */

/** Initialize parser state.  Call once at startup. */
void xy_gnss_init(void);

/* ── Mode A: byte-stream feed (call from UART ISR or main loop) ───── */

/**
 * Feed one received byte from the GNSS UART.
 * May be called from ISR context — all state is local, no malloc.
 * When a complete '$...CR LF' sentence is assembled, it is parsed
 * and the internal fix structure is updated atomically.
 */
void xy_gnss_feed_byte(uint8_t byte);

/* ── Mode B: AT response parse (single-call, not streaming) ─────── */

/**
 * Parse an AT+QGPSLOC (or equivalent) response string and populate pos.
 * Returns true on success.
 *
 *   Example response: "+QGPSLOC: 085959.0,3114.2506N,12131.4531E,1.1,50.0,2,..."
 */
bool xy_gnss_parse_at_response(const char *resp, xy_gnss_pos_t *pos);

/* ── Query ────────────────────────────────────────────────────────── */

/** Returns true if the last parsed RMC sentence had a valid fix. */
bool xy_gnss_has_fix(void);

/** Copy the latest fix data (safe to call from main loop). */
xy_gnss_pos_t xy_gnss_get_pos(void);

/** Returns the current parser state. */
xy_gnss_state_t xy_gnss_get_state(void);

/* ── Utility ─────────────────────────────────────────────────────── */

/**
 * Format a position as a compact distress message string.
 * Output: "LAT:31.45251N LON:121.45315E ALT:50m SPD:3kn UTC:08:59:59"
 * Returns number of characters written (excl. NUL).
 */
int xy_gnss_format_pos(const xy_gnss_pos_t *pos, char *buf, int bufsize);

/**
 * Convert lat_1e7/lon_1e7 to NMEA DDmm.mmmm format strings.
 * lat_str and lon_str must be at least 12 bytes.
 */
void xy_gnss_to_nmea(const xy_gnss_pos_t *pos,
                     char *lat_str, char *lat_dir,
                     char *lon_str, char *lon_dir);

#endif /* XY_GNSS_H */
