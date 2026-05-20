#ifndef XY_UWB_H
#define XY_UWB_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "xy_typedef.h"

/* -----------------------------------------------------------------------
 * IEEE 802.15.4 Frame Control — frame type field values
 * ----------------------------------------------------------------------- */
typedef uint8_t uwb_frame_type_t;
#define UWB_FRAME_BEACON   0u
#define UWB_FRAME_DATA     1u
#define UWB_FRAME_ACK      2u
#define UWB_FRAME_MAC_CMD  3u

/* -----------------------------------------------------------------------
 * Addressing modes
 * ----------------------------------------------------------------------- */
typedef uint8_t uwb_addr_mode_t;
#define UWB_ADDR_NONE   0u
#define UWB_ADDR_SHORT  2u   /* 16-bit */
#define UWB_ADDR_LONG   3u   /* 64-bit */

/* -----------------------------------------------------------------------
 * TWR application-layer function codes (payload byte 0)
 * ----------------------------------------------------------------------- */
typedef uint8_t uwb_range_func_t;
#define UWB_FUNC_POLL         0x21u
#define UWB_FUNC_POLL_ACK     0x10u
#define UWB_FUNC_FINAL        0x23u
#define UWB_FUNC_RANGE_REPORT 0x50u

/* -----------------------------------------------------------------------
 * Decoded MAC frame
 * ----------------------------------------------------------------------- */
typedef struct {
    uwb_frame_type_t frame_type;
    uint8_t          seq_num;
    bool             ack_request;
    bool             pan_id_compress;
    uwb_addr_mode_t  dst_mode;
    uwb_addr_mode_t  src_mode;
    uint8_t          frame_version;
    uint16_t         dst_pan;     /* 0xFFFF if broadcast or not present */
    uint16_t         dst_short;   /* valid when dst_mode == UWB_ADDR_SHORT */
    uint64_t         dst_long;    /* valid when dst_mode == UWB_ADDR_LONG */
    uint16_t         src_pan;
    uint16_t         src_short;
    uint64_t         src_long;
    const uint8_t   *payload;     /* pointer into caller's buffer */
    uint16_t         payload_len;
    bool             fcs_ok;
} uwb_mac_frame_t;

/* -----------------------------------------------------------------------
 * TWR range result (parsed from RANGE_REPORT payload)
 * ----------------------------------------------------------------------- */
typedef struct {
    uint32_t distance_mm;
    uint32_t tx_timestamp;   /* lower 32 bits of 40-bit DW timestamp */
    uint32_t rx_timestamp;
} uwb_range_result_t;

typedef void (*uwb_frame_cb)(const uwb_mac_frame_t *frame, void *user);

#define UWB_MAX_FRAME  128u

/* -----------------------------------------------------------------------
 * Stream parser state (caller-allocated)
 * ----------------------------------------------------------------------- */
typedef struct {
    uint8_t      buf[UWB_MAX_FRAME];
    uint16_t     len;
    uint16_t     expected;
    uint8_t      state;          /* 0=len_lo, 1=len_hi, 2=data */
    uwb_frame_cb cb;
    void        *user;
} uwb_parser_t;

/* -----------------------------------------------------------------------
 * API
 * ----------------------------------------------------------------------- */

void uwb_parser_init(uwb_parser_t *p, uwb_frame_cb cb, void *user);

/**
 * Parse a complete raw frame buffer (e.g. from DW SPI RX buffer).
 * Fires cb if frame is valid.  Returns true on success.
 */
bool uwb_parse_frame(uwb_parser_t *p, const uint8_t *data, uint16_t len);

/**
 * Feed one byte from a length-prefixed UART stream.
 * Format: [len_lo][len_hi][frame bytes ...].
 * Calls uwb_parse_frame when a complete frame is assembled.
 */
void uwb_feed_byte(uwb_parser_t *p, uint8_t ch);

/**
 * Parse TWR RANGE_REPORT payload.
 * payload[0] must equal UWB_FUNC_RANGE_REPORT (0x50).
 * Returns false if payload is too short or func code does not match.
 */
bool uwb_parse_range_result(const uint8_t *payload, uint16_t len,
                            uwb_range_result_t *out);

/**
 * CRC-16/CCITT: poly 0x1021, init 0x0000, no reflection.
 */
uint16_t uwb_crc16(const uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* XY_UWB_H */
