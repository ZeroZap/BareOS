#include "xy_uwb.h"
#include "xy_string.h"

/* -----------------------------------------------------------------------
 * CRC-16/CCITT  poly=0x1021  init=0x0000  no reflection
 * ----------------------------------------------------------------------- */
uint16_t uwb_crc16(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0x0000u;
    for (uint16_t i = 0u; i < len; i++) {
        crc ^= (uint16_t)((uint16_t)data[i] << 8u);
        for (uint8_t b = 0u; b < 8u; b++) {
            if (crc & 0x8000u) {
                crc = (uint16_t)((crc << 1u) ^ 0x1021u);
            } else {
                crc = (uint16_t)(crc << 1u);
            }
        }
    }
    return crc;
}

/* -----------------------------------------------------------------------
 * Read LE uint16
 * ----------------------------------------------------------------------- */
static uint16_t rd_u16_le(const uint8_t *p)
{
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8u));
}

/* Read LE uint64 */
static uint64_t rd_u64_le(const uint8_t *p)
{
    uint64_t v = 0u;
    for (int i = 7; i >= 0; i--) {
        v = (v << 8u) | p[i];
    }
    return v;
}

/* -----------------------------------------------------------------------
 * uwb_parse_frame
 *
 * Frame layout (little-endian):
 *   [0-1]  Frame Control
 *   [2]    Sequence Number
 *   [3..]  Addressing fields (variable)
 *   [..]   Payload
 *   [n-1, n]  FCS (CRC-16)
 * ----------------------------------------------------------------------- */
bool uwb_parse_frame(uwb_parser_t *p, const uint8_t *data, uint16_t len)
{
    /* Minimum: FC(2) + Seq(1) + FCS(2) = 5 bytes */
    if (len < 5u) return false;

    uwb_mac_frame_t frame;
    memset(&frame, 0, sizeof(frame));
    frame.dst_pan = 0xFFFFu;

    /* Frame Control */
    uint16_t fc = rd_u16_le(data);
    frame.frame_type    = (uwb_frame_type_t)(fc & 0x07u);
    /* bit 3: security, bit 4: frame pending, bit 5: ACK request */
    frame.ack_request    = (bool)((fc >> 5u) & 1u);
    frame.pan_id_compress = (bool)((fc >> 6u) & 1u);
    frame.dst_mode       = (uwb_addr_mode_t)((fc >> 10u) & 0x03u);
    frame.frame_version  = (uint8_t)((fc >> 12u) & 0x03u);
    frame.src_mode       = (uwb_addr_mode_t)((fc >> 14u) & 0x03u);

    frame.seq_num = data[2];

    /* Walk addressing fields starting at offset 3 */
    uint16_t off = 3u;

    /* Destination PAN ID (present if dst_mode != NONE) */
    if (frame.dst_mode != UWB_ADDR_NONE) {
        if (off + 2u > len - 2u) return false;
        frame.dst_pan = rd_u16_le(data + off);
        off += 2u;
    }

    /* Destination address */
    if (frame.dst_mode == UWB_ADDR_SHORT) {
        if (off + 2u > len - 2u) return false;
        frame.dst_short = rd_u16_le(data + off);
        off += 2u;
    } else if (frame.dst_mode == UWB_ADDR_LONG) {
        if (off + 8u > len - 2u) return false;
        frame.dst_long = rd_u64_le(data + off);
        off += 8u;
    }

    /* Source PAN ID: omitted if PAN ID compression and dst present */
    bool src_pan_omitted = (frame.pan_id_compress && frame.dst_mode != UWB_ADDR_NONE);
    if (frame.src_mode != UWB_ADDR_NONE && !src_pan_omitted) {
        if (off + 2u > len - 2u) return false;
        frame.src_pan = rd_u16_le(data + off);
        off += 2u;
    } else if (src_pan_omitted) {
        frame.src_pan = frame.dst_pan;
    }

    /* Source address */
    if (frame.src_mode == UWB_ADDR_SHORT) {
        if (off + 2u > len - 2u) return false;
        frame.src_short = rd_u16_le(data + off);
        off += 2u;
    } else if (frame.src_mode == UWB_ADDR_LONG) {
        if (off + 8u > len - 2u) return false;
        frame.src_long = rd_u64_le(data + off);
        off += 8u;
    }

    /* Payload: everything between addressing and FCS */
    uint16_t fcs_off = len - 2u;
    if (off > fcs_off) return false;

    frame.payload     = data + off;
    frame.payload_len = fcs_off - off;

    /* Validate FCS: CRC-16/CCITT over bytes [0 .. fcs_off-1] */
    uint16_t computed = uwb_crc16(data, fcs_off);
    uint16_t received = rd_u16_le(data + fcs_off);
    frame.fcs_ok = (computed == received);

    if (p->cb) p->cb(&frame, p->user);
    return true;
}

/* -----------------------------------------------------------------------
 * uwb_parser_init / uwb_feed_byte
 * Length-prefixed stream: [len_lo][len_hi][frame...]
 * ----------------------------------------------------------------------- */
void uwb_parser_init(uwb_parser_t *p, uwb_frame_cb cb, void *user)
{
    memset(p, 0, sizeof(*p));
    p->cb   = cb;
    p->user = user;
    /* state 0 = waiting for len_lo */
}

void uwb_feed_byte(uwb_parser_t *p, uint8_t ch)
{
    switch (p->state) {
    case 0u:  /* len low byte */
        p->expected = ch;
        p->len      = 0u;
        p->state    = 1u;
        break;

    case 1u:  /* len high byte */
        p->expected |= (uint16_t)((uint16_t)ch << 8u);
        if (p->expected == 0u || p->expected > UWB_MAX_FRAME) {
            /* invalid length — resync */
            p->state = 0u;
        } else {
            p->state = 2u;
        }
        break;

    case 2u:  /* accumulate frame bytes */
        if (p->len < UWB_MAX_FRAME) {
            p->buf[p->len] = ch;
        }
        p->len++;
        if (p->len >= p->expected) {
            uint16_t safe_len = (p->expected <= UWB_MAX_FRAME)
                                ? p->expected : UWB_MAX_FRAME;
            uwb_parse_frame(p, p->buf, safe_len);
            p->state = 0u;
        }
        break;

    default:
        p->state = 0u;
        break;
    }
}

/* -----------------------------------------------------------------------
 * uwb_parse_range_result
 *
 * payload[0]   = 0x50 (UWB_FUNC_RANGE_REPORT)
 * payload[1-4] = distance_mm LE uint32
 * payload[5-8] = tx_timestamp LE uint32
 * payload[9-12]= rx_timestamp LE uint32
 * ----------------------------------------------------------------------- */
bool uwb_parse_range_result(const uint8_t *payload, uint16_t len,
                            uwb_range_result_t *out)
{
    if (!payload || !out || len < 13u) return false;
    if (payload[0] != UWB_FUNC_RANGE_REPORT) return false;

    out->distance_mm  = (uint32_t)(payload[1])
                      | ((uint32_t)(payload[2]) << 8u)
                      | ((uint32_t)(payload[3]) << 16u)
                      | ((uint32_t)(payload[4]) << 24u);

    out->tx_timestamp = (uint32_t)(payload[5])
                      | ((uint32_t)(payload[6]) << 8u)
                      | ((uint32_t)(payload[7]) << 16u)
                      | ((uint32_t)(payload[8]) << 24u);

    out->rx_timestamp = (uint32_t)(payload[9])
                      | ((uint32_t)(payload[10]) << 8u)
                      | ((uint32_t)(payload[11]) << 16u)
                      | ((uint32_t)(payload[12]) << 24u);

    return true;
}
