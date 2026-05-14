#include "xy_epirb.h"
#include "xy_stdio.h"
#include <string.h>

/* -----------------------------------------------------------------------
 * Internal helpers
 * ----------------------------------------------------------------------- */

static uint8_t hex_nibble(char c)
{
    if (c >= '0' && c <= '9') return (uint8_t)(c - '0');
    if (c >= 'A' && c <= 'F') return (uint8_t)(c - 'A' + 10u);
    if (c >= 'a' && c <= 'f') return (uint8_t)(c - 'a' + 10u);
    return 0xFFu;
}

/* -----------------------------------------------------------------------
 * epirb_getbits
 *
 * raw[0] holds bits 1-8, raw[1] bits 9-16, ... raw[7] bits 57-60 in MSBs.
 * bit_offset is 1-based.
 * ----------------------------------------------------------------------- */
uint32_t epirb_getbits(const epirb_id_t *id, uint8_t bit_offset, uint8_t bit_count)
{
    uint32_t result = 0u;
    /* iterate bit by bit from MSB of the field */
    for (uint8_t i = 0u; i < bit_count; i++) {
        uint8_t abs_bit = (uint8_t)(bit_offset - 1u + i);  /* 0-based */
        uint8_t byte_idx = abs_bit >> 3u;
        uint8_t bit_in_byte = (uint8_t)(7u - (abs_bit & 7u)); /* MSB first */
        uint8_t bit_val = (id->raw[byte_idx] >> bit_in_byte) & 1u;
        result = (result << 1u) | bit_val;
    }
    return result;
}

/* -----------------------------------------------------------------------
 * epirb_decode
 * ----------------------------------------------------------------------- */
epirb_result_t epirb_decode(const char *hex15, epirb_id_t *out)
{
    if (!hex15 || !out) return EPIRB_ERR_INVALID_LEN;

    /* Count exactly 15 hex chars */
    const char *p = hex15;
    for (int i = 0; i < 15; i++) {
        if (hex_nibble(p[i]) == 0xFFu) return EPIRB_ERR_INVALID_HEX;
    }
    /* length: string must have exactly 15 hex chars before NUL or non-hex */
    /* (hex15[15] may be NUL or anything — we only consume 15) */

    memset(out, 0, sizeof(*out));

    /* Pack 15 hex nibbles (60 bits) into raw[0..7], big-endian.
     * Nibbles: N0 N1 ... N14, total 60 bits.
     * raw[0] = N0<<4 | N1, raw[1] = N2<<4 | N3, ..., raw[6] = N12<<4|N13
     * raw[7] = N14 << 4  (top 4 bits; bottom 4 bits unused = 0) */
    for (int i = 0; i < 7; i++) {
        uint8_t hi = hex_nibble(p[i * 2]);
        uint8_t lo = hex_nibble(p[i * 2 + 1]);
        out->raw[i] = (uint8_t)((hi << 4u) | lo);
    }
    out->raw[7] = (uint8_t)(hex_nibble(p[14]) << 4u);

    /* Extract fields */
    out->country_code = (uint16_t)epirb_getbits(out, 1u, 10u);

    uint32_t proto_flag = epirb_getbits(out, 11u, 1u);

    if (proto_flag == 0u) {
        /* User protocol — bits 12-13 give sub-type */
        uint32_t sub = epirb_getbits(out, 12u, 2u);
        out->protocol    = (epirb_protocol_t)sub;
        out->beacon_type = EPIRB_TYPE_UNKNOWN;

        if (sub == 0u) {
            /* Maritime user: bits 14-43 = 30-bit MMSI */
            out->mmsi   = epirb_getbits(out, 14u, 30u);
            /* bits 44-60 = 17-bit specific beacon ID */
            out->serial = epirb_getbits(out, 44u, 17u);
        } else {
            /* For other user protocols store remaining bits as serial */
            out->serial = epirb_getbits(out, 14u, 46u > 32u ? 32u : 46u);
        }
    } else {
        /* Standard location protocol */
        out->protocol = EPIRB_PROTO_LOCATION;

        uint32_t btype = epirb_getbits(out, 12u, 4u);
        switch (btype) {
            case 0u:  out->beacon_type = EPIRB_TYPE_EPIRB; break;
            case 1u:  out->beacon_type = EPIRB_TYPE_PLB;   break;
            case 2u:  out->beacon_type = EPIRB_TYPE_ELT;   break;
            default:  out->beacon_type = EPIRB_TYPE_UNKNOWN; break;
        }

        /* bits 16-25: redundant country code (skip) */
        /* bits 26-49: 24-bit ICAO or serial */
        uint32_t id24 = epirb_getbits(out, 26u, 24u);
        if (out->beacon_type == EPIRB_TYPE_ELT) {
            out->icao_addr = id24;
        } else {
            out->serial = id24;
        }
    }

    return EPIRB_OK;
}

/* -----------------------------------------------------------------------
 * epirb_format
 * ----------------------------------------------------------------------- */
int epirb_format(const epirb_id_t *id, char *buf, int bufsize)
{
    if (!id || !buf || bufsize <= 0) return 0;

    static const char *proto_names[] = {
        "Maritime", "Radio/EPIRB+Pos", "Serial", "National"
    };

    const char *proto_str;
    if (id->protocol == EPIRB_PROTO_LOCATION) {
        proto_str = "LocationProto";
    } else if (id->protocol <= EPIRB_PROTO_USER_NATIONAL) {
        proto_str = proto_names[id->protocol];
    } else {
        proto_str = "Unknown";
    }

    int n = 0;

    if (id->protocol == EPIRB_PROTO_LOCATION) {
        static const char *type_names[] = { "EPIRB", "PLB", "ELT" };
        const char *type_str = (id->beacon_type <= EPIRB_TYPE_ELT)
                               ? type_names[id->beacon_type] : "Unknown";

        if (id->beacon_type == EPIRB_TYPE_ELT) {
            n = xy_snprintf(buf, (uint32_t)bufsize,
                "CC=%u Proto=%s Type=%s ICAO=%06lX",
                (unsigned)id->country_code, proto_str, type_str,
                (unsigned long)id->icao_addr);
        } else {
            n = xy_snprintf(buf, (uint32_t)bufsize,
                "CC=%u Proto=%s Type=%s Serial=%u",
                (unsigned)id->country_code, proto_str, type_str,
                (unsigned)id->serial);
        }
    } else if (id->protocol == EPIRB_PROTO_USER_MARITIME) {
        n = xy_snprintf(buf, (uint32_t)bufsize,
            "CC=%u Proto=%s MMSI=%09lu Serial=%u",
            (unsigned)id->country_code, proto_str,
            (unsigned long)id->mmsi,
            (unsigned)id->serial);
    } else {
        n = xy_snprintf(buf, (uint32_t)bufsize,
            "CC=%u Proto=%s Serial=%u",
            (unsigned)id->country_code, proto_str,
            (unsigned)id->serial);
    }

    return n;
}
