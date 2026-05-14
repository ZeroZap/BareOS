/*
 * xy_ubx.h — u-blox UBX binary protocol parser, bare-metal, no heap.
 *
 * Feed bytes one at a time via ubx_feed_byte(). On each complete frame that
 * passes the Fletcher-8 checksum, the registered callback is fired with the
 * raw payload bytes, class, and id. Use the ubx_decode_* helpers inside the
 * callback to get typed structs.
 *
 * UBX frame layout:
 *   0xB5 0x62 | class(1) | id(1) | len_L(1) len_H(1) | payload(len) | CK_A CK_B
 *
 * Fletcher-8 checksum over: class, id, len_L, len_H, payload[0..len-1].
 *   CK_A += byte;  CK_B += CK_A;   (initialised to 0)
 */

#ifndef XY_UBX_H
#define XY_UBX_H

#include <stdint.h>
#include <stdbool.h>

/* ── Message class / ID constants ────────────────────────────────── */

#define UBX_CLASS_NAV      0x01u
#define UBX_CLASS_ACK      0x05u

#define UBX_ID_NAV_PVT     0x07u
#define UBX_ID_NAV_STATUS  0x03u
#define UBX_ID_ACK_ACK     0x01u
#define UBX_ID_ACK_NAK     0x00u

/* ── Typed message structs ────────────────────────────────────────── */

/* NAV-PVT (0x01 0x07, 92 bytes) — Navigation Position Velocity Time.
 * All multi-byte fields are little-endian in the wire format; memcpy into
 * this packed struct works correctly on ARM Cortex-M regardless of alignment. */
typedef struct __attribute__((packed)) {
    uint32_t itow;           /* GPS time of week (ms) */
    uint16_t year;
    uint8_t  month, day, hour, min, sec;
    uint8_t  valid;          /* bit0=validDate, bit1=validTime, bit3=fullyResolved */
    uint32_t tAcc;           /* time accuracy estimate (ns) */
    int32_t  nano;           /* nanosecond fraction of second */
    uint8_t  fixType;        /* 0=no fix, 2=2D, 3=3D, 4=GNSS+DR */
    uint8_t  flags;          /* bit0=gnssFixOK, bit5=headVehValid */
    uint8_t  flags2;
    uint8_t  numSV;          /* number of satellites used */
    int32_t  lon_1e7;        /* longitude (degrees × 1e7) */
    int32_t  lat_1e7;        /* latitude  (degrees × 1e7) */
    int32_t  height_mm;      /* height above ellipsoid (mm) */
    int32_t  hMSL_mm;        /* height above MSL (mm) */
    uint32_t hAcc_mm;        /* horizontal accuracy (mm) */
    uint32_t vAcc_mm;        /* vertical accuracy (mm) */
    int32_t  velN;           /* north velocity (mm/s) */
    int32_t  velE;           /* east velocity (mm/s) */
    int32_t  velD;           /* down velocity (mm/s) */
    int32_t  gSpeed;         /* ground speed (mm/s) */
    int32_t  headMot;        /* heading of motion (degrees × 1e5) */
    uint32_t sAcc;           /* speed accuracy (mm/s) */
    uint32_t headAcc;        /* heading accuracy (degrees × 1e5) */
    uint16_t pDOP;           /* PDOP × 100 */
    uint8_t  flags3;
    uint8_t  _reserved[5];
    int32_t  headVeh;        /* vehicle heading (degrees × 1e5); valid when flags bit5=1 */
    int16_t  magDec;         /* magnetic declination (degrees × 100) */
    uint16_t magAcc;         /* magnetic declination accuracy (degrees × 100) */
} ubx_nav_pvt_t;

/* NAV-STATUS (0x01 0x03, 16 bytes) — Receiver Navigation Status. */
typedef struct __attribute__((packed)) {
    uint32_t itow;      /* GPS time of week (ms) */
    uint8_t  gpsFix;   /* 0=no fix, 2=2D, 3=3D, 5=time only */
    uint8_t  flags;    /* bit0=gpsFixOK */
    uint8_t  fixStat;
    uint8_t  flags2;
    uint32_t ttff_ms;  /* time to first fix (ms) */
    uint32_t msss;     /* ms since startup */
} ubx_nav_status_t;

/* ACK-ACK / ACK-NAK (0x05 0x01/0x00, 2 bytes). */
typedef struct {
    uint8_t cls; /* class of the acknowledged message */
    uint8_t id;  /* id of the acknowledged message */
} ubx_ack_t;

/* ── Callback ─────────────────────────────────────────────────────── */

/* Fired once per valid (checksum-verified) UBX frame.
 * payload points into the parser's internal buffer; copy before returning. */
typedef void (*ubx_msg_cb)(uint8_t cls, uint8_t id,
                           const uint8_t *payload, uint16_t len, void *user);

/* ── Parser state ─────────────────────────────────────────────────── */

/* Maximum payload size accepted. Frames with larger payloads are skipped. */
#define UBX_MAX_PAYLOAD 256u

typedef struct {
    uint8_t   buf[UBX_MAX_PAYLOAD]; /* payload accumulation buffer */
    uint16_t  payload_len;          /* declared payload length from header */
    uint16_t  payload_idx;          /* bytes received so far */
    uint8_t   cls, id;
    uint8_t   ck_a, ck_b;          /* Fletcher-8 accumulator */
    uint8_t   state;                /* internal FSM state */
    ubx_msg_cb cb;
    void      *user;
} ubx_parser_t;

/* ── API ──────────────────────────────────────────────────────────── */

/* Initialise parser. Must be called before the first ubx_feed_byte(). */
void ubx_parser_init(ubx_parser_t *p, ubx_msg_cb cb, void *user);

/* Feed one byte from the GNSS UART. Safe to call from ISR context.
 * All state lives in *p — no module-level globals. */
void ubx_feed_byte(ubx_parser_t *p, uint8_t ch);

/* Decode a NAV-PVT payload. Returns false if len != sizeof(ubx_nav_pvt_t). */
bool ubx_decode_nav_pvt(const uint8_t *payload, uint16_t len, ubx_nav_pvt_t *out);

/* Decode a NAV-STATUS payload. Returns false if len != sizeof(ubx_nav_status_t). */
bool ubx_decode_nav_status(const uint8_t *payload, uint16_t len, ubx_nav_status_t *out);

/*
 * Build a complete UBX frame into buf (sync + header + payload + checksum).
 * buf must be at least payload_len + 8 bytes.
 * Returns the total frame length written.
 */
uint16_t ubx_build_msg(uint8_t *buf, uint8_t cls, uint8_t id,
                       const uint8_t *payload, uint16_t payload_len);

#endif /* XY_UBX_H */
