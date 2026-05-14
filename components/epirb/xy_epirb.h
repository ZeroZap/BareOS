#ifndef XY_EPIRB_H
#define XY_EPIRB_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* -----------------------------------------------------------------------
 * Protocol identifiers (bit 11 of beacon ID)
 * ----------------------------------------------------------------------- */
typedef uint8_t epirb_protocol_t;
#define EPIRB_PROTO_USER_MARITIME   0u   /* bits 12-13 = 00 */
#define EPIRB_PROTO_USER_RADIO      1u   /* bits 12-13 = 01 */
#define EPIRB_PROTO_USER_SERIAL     2u   /* bits 12-13 = 10 */
#define EPIRB_PROTO_USER_NATIONAL   3u   /* bits 12-13 = 11 */
#define EPIRB_PROTO_LOCATION        4u   /* bit 11 = 1: standard location */
#define EPIRB_PROTO_UNKNOWN         0xFFu

/* -----------------------------------------------------------------------
 * Beacon type (for EPIRB_PROTO_LOCATION, bits 12-15)
 * ----------------------------------------------------------------------- */
typedef uint8_t epirb_beacon_type_t;
#define EPIRB_TYPE_EPIRB    0u
#define EPIRB_TYPE_PLB      1u
#define EPIRB_TYPE_ELT      2u
#define EPIRB_TYPE_UNKNOWN  0xFFu

/* -----------------------------------------------------------------------
 * Decoded beacon ID
 * ----------------------------------------------------------------------- */
typedef struct {
    uint16_t            country_code;  /* 10-bit MID */
    epirb_protocol_t    protocol;
    epirb_beacon_type_t beacon_type;   /* valid for EPIRB_PROTO_LOCATION */
    uint32_t            mmsi;          /* valid for EPIRB_PROTO_USER_MARITIME */
    uint32_t            serial;        /* beacon serial / specific ID */
    uint32_t            icao_addr;     /* 24-bit ICAO address (ELT only) */
    uint8_t             raw[8];        /* 60 bits packed big-endian; top 4 bits of [7] unused */
} epirb_id_t;

/* -----------------------------------------------------------------------
 * Decode result codes
 * ----------------------------------------------------------------------- */
typedef enum {
    EPIRB_OK = 0,
    EPIRB_ERR_INVALID_HEX,
    EPIRB_ERR_INVALID_LEN,
} epirb_result_t;

/* -----------------------------------------------------------------------
 * API
 * ----------------------------------------------------------------------- */

/**
 * Decode a 15-character hex string (upper or lower case) into epirb_id_t.
 * Exactly 15 hex characters are consumed; trailing NUL is allowed.
 */
epirb_result_t epirb_decode(const char *hex15, epirb_id_t *out);

/**
 * Extract a bit field from the raw 60-bit beacon ID.
 * bit_offset: 1-based, bit 1 = MSB of raw[0].
 * bit_count: 1-32.
 */
uint32_t epirb_getbits(const epirb_id_t *id, uint8_t bit_offset, uint8_t bit_count);

/**
 * Format beacon ID as a human-readable string.
 * buf should be at least 80 bytes.
 * Returns number of characters written (excluding NUL).
 */
int epirb_format(const epirb_id_t *id, char *buf, int bufsize);

#ifdef __cplusplus
}
#endif

#endif /* XY_EPIRB_H */
