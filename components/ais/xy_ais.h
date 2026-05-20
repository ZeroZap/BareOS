/*
 * xy_ais.h — AIS (Automatic Identification System) NMEA sentence parser.
 *            Bare-metal, no heap, no RTOS, no floats.
 *
 * Decodes !AIVDM (received) and !AIVDO (own ship) NMEA 0183 sentences.
 * Assembles multi-part messages (e.g. type 5 static data) across up to
 * AIS_SEQ_SLOTS simultaneous in-flight sequences.
 *
 * Usage:
 *   ais_parser_t parser;
 *   ais_parser_init(&parser, my_callback, NULL);
 *
 *   // From UART RX / DMA callback — feed complete NMEA lines:
 *   ais_feed_sentence(&parser, "!AIVDM,1,1,,B,15M67N0000...,0*73");
 *
 * Coordinate format (raw AIS): int32_t in 1/10000 arcminutes (lat_1e4 / lon_1e4).
 *   lat_1e4 not-available sentinel:  0x3412140 (91 deg × 600000)
 *   lon_1e4 not-available sentinel:  0x6791AC0 (181 deg × 600000)
 */

#ifndef XY_AIS_H
#define XY_AIS_H

#include <stdint.h>
#include "xy_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ── Message type constants ──────────────────────────────────────────── */

typedef uint8_t ais_msg_type_t;
#define AIS_MSG_POS_CLASS_A   1u   /* types 1, 2, 3 — Class A position report */
#define AIS_MSG_STATIC_VOYAGE 5u   /* static and voyage related data          */
#define AIS_MSG_SAFETY_BCAST  14u  /* safety related broadcast message        */
#define AIS_MSG_POS_CLASS_B   18u  /* Class B position report (simplified)    */
#define AIS_MSG_AIDS_NAV      21u  /* aid-to-navigation report                */
#define AIS_MSG_STATIC_B      24u  /* Class B CS static data report           */

/* ── Navigation status (types 1/2/3) ────────────────────────────────── */

typedef uint8_t ais_nav_status_t;
#define AIS_NAV_UNDERWAY_ENGINE   0u
#define AIS_NAV_ANCHORED          1u
#define AIS_NAV_NOT_UNDER_COMMAND 2u
#define AIS_NAV_RESTRICTED        3u
#define AIS_NAV_DISTRESS          14u
#define AIS_NAV_UNDEFINED         15u

/* ── Decoded message payloads ────────────────────────────────────────── */

/*
 * Class A position report — message types 1, 2, 3.
 *
 * lat_1e4 / lon_1e4: raw AIS 1/10000 arcminute signed integer.
 *   Positive = North / East.  Not-available: lat = 0x3412140, lon = 0x6791AC0.
 * sog_x10: speed over ground × 10 knots (1023 = not available).
 * cog_x10: course over ground × 10 degrees (3600 = not available).
 * true_heading: 0–359 degrees; 511 = not available.
 * second: UTC second 0–59; 60 = not available / default.
 */
typedef struct {
    uint32_t mmsi;
    uint8_t  nav_status;   /* ais_nav_status_t */
    int32_t  lat_1e4;      /* 1/10000 arcminute, signed */
    int32_t  lon_1e4;      /* 1/10000 arcminute, signed */
    uint16_t sog_x10;      /* speed over ground × 10 knots */
    uint16_t cog_x10;      /* course over ground × 10 degrees */
    uint16_t true_heading; /* 0–359; 511 = not available */
    uint8_t  second;       /* UTC second 0–59; 60 = not available */
    bool     raim;         /* Receiver Autonomous Integrity Monitoring */
} ais_pos_report_t;

/*
 * Static + voyage data — message type 5 (needs 2 sentences, 426 bits total).
 *
 * callsign / name / destination: space-padded 6-bit ASCII, stored NUL-terminated.
 * draught_x10: draught in 1/10 metre (e.g. 65 = 6.5 m).
 * dim_*: distance from reference point to vessel extremity, metres.
 */
typedef struct {
    uint32_t mmsi;
    uint32_t imo;
    char     callsign[8];     /* 7 chars + NUL */
    char     name[21];        /* 20 chars + NUL */
    uint8_t  ship_type;
    uint16_t dim_bow;         /* metres from reference to bow */
    uint16_t dim_stern;       /* metres from reference to stern */
    uint16_t dim_port;        /* metres from reference to port side */
    uint16_t dim_starboard;   /* metres from reference to starboard */
    uint8_t  draught_x10;     /* draught × 10 (1/10 metre) */
    char     destination[21]; /* 20 chars + NUL */
} ais_static_voyage_t;

/*
 * Class B position report — message type 18.
 */
typedef struct {
    uint32_t mmsi;
    int32_t  lat_1e4;
    int32_t  lon_1e4;
    uint16_t sog_x10;
    uint16_t cog_x10;
    uint16_t true_heading;
    bool     cs_unit;  /* true = Class B CS (Carrier Sense) unit */
    bool     raim;
} ais_pos_class_b_t;

/*
 * Aid-to-Navigation report — message type 21.
 *
 * type_of_aid: 1–31 per ITU-R M.1371 table 68.
 * off_position: true if AtoN not on charted position.
 */
typedef struct {
    uint32_t mmsi;
    uint8_t  type_of_aid;
    char     name[21];     /* 20 chars + NUL */
    int32_t  lat_1e4;
    int32_t  lon_1e4;
    bool     off_position;
    bool     raim;
} ais_aid_nav_t;

/*
 * Safety-related broadcast message — message type 14.
 */
typedef struct {
    uint32_t mmsi;
    char     text[161];  /* up to 160 chars + NUL */
} ais_safety_bcast_t;

/* ── Decoded AIS message container ──────────────────────────────────── */

typedef struct {
    uint8_t type;  /* raw message type 1–27 */
    union {
        ais_pos_report_t    pos_a;    /* types 1, 2, 3 */
        ais_static_voyage_t static_v; /* type 5        */
        ais_safety_bcast_t  safety;   /* type 14       */
        ais_pos_class_b_t   pos_b;    /* type 18       */
        ais_aid_nav_t       aid_nav;  /* type 21       */
    };
} ais_message_t;

/* ── Callback ────────────────────────────────────────────────────────── */

/*
 * Fired once per complete (possibly multi-part) decoded message.
 * Called from ais_feed_sentence() context — keep the handler brief.
 * The msg pointer is valid only for the duration of the callback.
 */
typedef void (*ais_msg_cb)(const ais_message_t *msg, void *user);

/* ── Parser limits ───────────────────────────────────────────────────── */

#define AIS_NMEA_MAX    83  /* max NMEA sentence length (standard: 82 + NUL) */
#define AIS_PAYLOAD_MAX 56  /* max 6-bit chars in one sentence (56 × 6 = 336 bits) */
#define AIS_SEQ_SLOTS   2   /* in-flight multi-part sequences tracked simultaneously */

/* ── Parser state ────────────────────────────────────────────────────── */

/*
 * All state lives in caller-allocated ais_parser_t.
 * No globals, no malloc.
 */
typedef struct {
    /* Multi-part assembly slots, one per active sequential message ID */
    struct {
        uint8_t seq_id;                       /* sequential message ID 1–9  */
        uint8_t total;                        /* total sentences in message  */
        uint8_t received;                     /* sentences received so far   */
        uint8_t payload[AIS_PAYLOAD_MAX * 2]; /* assembled 6-bit values      */
        uint8_t payload_len;                  /* number of 6-bit values      */
        uint8_t fill_bits;                    /* fill bits from last sentence */
    } slots[AIS_SEQ_SLOTS];

    ais_msg_cb cb;
    void      *user;
} ais_parser_t;

/* ── Public API ──────────────────────────────────────────────────────── */

/*
 * Initialise parser state.  Must be called before ais_feed_sentence().
 * cb    — callback invoked for each decoded message (may be NULL for testing).
 * user  — opaque pointer passed unchanged to cb.
 */
void ais_parser_init(ais_parser_t *p, ais_msg_cb cb, void *user);

/*
 * Feed one complete !AIVDM or !AIVDO sentence (NUL-terminated).
 * The sentence may optionally carry a trailing \r and/or \n.
 * Validates NMEA checksum (*XX).
 * Assembles multi-part messages; fires cb when the message is complete.
 * Returns true  — sentence was syntactically valid and accepted.
 * Returns false — checksum mismatch, malformed fields, or buffer overflow.
 */
bool ais_feed_sentence(ais_parser_t *p, const char *sentence);

/*
 * Extract an unsigned bit field from a 6-bit-value array.
 * payload    — array of uint8_t, each holding one 6-bit value (upper 2 bits 0).
 * bit_offset — 0-based, MSB-first within the AIS bit stream.
 * bit_count  — number of bits to extract (1–32).
 */
uint32_t ais_getbits(const uint8_t *payload, uint16_t bit_offset,
                     uint8_t bit_count);

/*
 * Extract a signed bit field; sign-extends from bit_count bits to int32_t.
 */
int32_t ais_getbits_signed(const uint8_t *payload, uint16_t bit_offset,
                            uint8_t bit_count);

/*
 * Decode AIS 6-bit text into a NUL-terminated ASCII string.
 * bit_offset — start bit in the payload array.
 * char_count — number of 6-bit characters to decode.
 * out        — destination buffer; must hold at least char_count + 1 bytes.
 * Trailing spaces (0x20) are stripped; result is always NUL-terminated.
 */
void ais_get_text(const uint8_t *payload, uint16_t bit_offset,
                  uint8_t char_count, char *out);

#ifdef __cplusplus
}
#endif

#endif /* XY_AIS_H */
