/**
 * @file xy_flog.c
 * @brief Persistent variable-length log in Flash — sector-ring implementation.
 *
 * Sector header (4 bytes, at sector start):
 *   magic  uint16  0xA55A
 *   seq    uint16  monotonic sector sequence number (wraps at 65535)
 *
 * Entry in flash (immediately after sector header or previous entry):
 *   header   xy_vlog_hdr_t  8 bytes
 *   payload  uint8_t[]      header.payload_len bytes
 *   crc16    uint16_t       CRC-16/CCITT over header+payload, little-endian
 *
 * End-of-entries is detected by payload_len > XY_FLOG_MAX_PAYLOAD (all-0xFF
 * from blank flash gives payload_len = 0xFFFF) or by CRC mismatch.
 */

#include "xy_flog.h"
#include "xy_crc.h"
#include "xy_stdio.h"
#include "xy_string.h"
#include <stdarg.h>

/* ── Constants ───────────────────────────────────────────────────────── */

#define SECTOR_MAGIC    0xA55Au
#define SECTOR_HDR_SZ   4u                                  /* sizeof(sector_hdr_t) */
#define ENTRY_CRC_SZ    2u                                  /* CRC appended to each entry */
#define ENTRY_MIN_SZ    ((uint32_t)XY_VLOG_HEADER_SIZE + ENTRY_CRC_SZ)
#define ENTRY_BUF_SZ    ((uint32_t)XY_VLOG_HEADER_SIZE + XY_FLOG_MAX_PAYLOAD)

/* ── Types ───────────────────────────────────────────────────────────── */

typedef struct __attribute__((packed)) {
    uint16_t magic;
    uint16_t seq;
} sector_hdr_t;

/* ── Module state ────────────────────────────────────────────────────── */

static struct {
    uint32_t      base;
    uint32_t      sec_size;
    uint16_t      num_sec;
    uint16_t      write_sec;   /* index of the sector being written */
    uint32_t      write_off;   /* byte offset within write_sec (from sec start) */
    uint16_t      read_sec;    /* index of the oldest sector with data */
    uint16_t      next_seq;    /* sequence number for the next new sector */
    uint32_t      count;       /* cached total entry count */
    flog_read_fn  fn_read;
    flog_write_fn fn_write;
    flog_erase_fn fn_erase;
    bool          inited;
} s;

/* Static work buffer — avoids large stack allocations in scan/write paths. */
static uint8_t s_entry_buf[ENTRY_BUF_SZ];
static uint8_t s_printf_buf[XY_FLOG_MAX_PAYLOAD];

/* ── Helpers ─────────────────────────────────────────────────────────── */

static inline uint32_t sec_addr(uint16_t idx)
{
    return s.base + (uint32_t)idx * s.sec_size;
}

/* True if sequence number 'a' is strictly after 'b' (handles wrap-around). */
static inline bool seq_after(uint16_t a, uint16_t b)
{
    return (int16_t)(a - b) > 0;
}

/* Erase sector idx and write a fresh sector header with the given seq. */
static int _init_sector(uint16_t idx, uint16_t seq)
{
    if (s.fn_erase(sec_addr(idx), s.sec_size) != 0) return -1;
    sector_hdr_t hdr = { SECTOR_MAGIC, seq };
    return s.fn_write(sec_addr(idx), &hdr, sizeof(hdr));
}

/* ── Core sector scanner ─────────────────────────────────────────────── */

/**
 * Scan valid entries in sector @sec_idx from oldest to newest.
 *
 * If @cb is non-NULL, it is called for each valid entry; returning false
 * from @cb stops iteration early.
 * If @out_count is non-NULL, the count of valid entries found is added to it.
 * If @out_off is non-NULL, the write offset (end of last valid entry) is
 * stored; useful for locating where to continue writing.
 *
 * @return true  — scan completed normally (or cb never returned false).
 *         false — cb returned false (early stop).
 */
static bool _scan_sector(uint16_t sec_idx, xy_flog_cb_t cb, void *user,
                          uint32_t *out_count, uint32_t *out_off)
{
    uint32_t off = SECTOR_HDR_SZ;

    while (off + ENTRY_MIN_SZ <= s.sec_size) {
        /* Read header — if payload_len > MAX, the area is blank or corrupt. */
        if (s.fn_read(sec_addr(sec_idx) + off, s_entry_buf, XY_VLOG_HEADER_SIZE) != 0)
            break;

        const xy_vlog_hdr_t *hdr = (const xy_vlog_hdr_t *)s_entry_buf;
        if (hdr->payload_len > XY_FLOG_MAX_PAYLOAD) break;

        uint32_t data_sz  = (uint32_t)XY_VLOG_HEADER_SIZE + hdr->payload_len;
        uint32_t entry_sz = data_sz + ENTRY_CRC_SZ;
        if (off + entry_sz > s.sec_size) break;

        /* Read payload (appended to header already in s_entry_buf). */
        if (hdr->payload_len > 0u) {
            if (s.fn_read(sec_addr(sec_idx) + off + XY_VLOG_HEADER_SIZE,
                          s_entry_buf + XY_VLOG_HEADER_SIZE,
                          hdr->payload_len) != 0)
                break;
        }

        /* Verify CRC. */
        uint16_t computed = xy_crc16(s_entry_buf, data_sz);
        uint16_t stored   = 0u;
        if (s.fn_read(sec_addr(sec_idx) + off + data_sz,
                      (uint8_t *)&stored, ENTRY_CRC_SZ) != 0)
            break;
        if (computed != stored) break;   /* CRC mismatch — corrupt or blank */

        off += entry_sz;

        if (out_count) (*out_count)++;

        if (cb) {
            const uint8_t *pl = (hdr->payload_len > 0u)
                                 ? s_entry_buf + XY_VLOG_HEADER_SIZE : NULL;
            if (!cb(hdr, pl, user)) {
                if (out_off) *out_off = off;
                return false;   /* early stop */
            }
        }
    }

    if (out_off) *out_off = off;
    return true;
}

/* ── Init ────────────────────────────────────────────────────────────── */

void xy_flog_init(uint32_t      base_addr,
                  uint32_t      region_size,
                  uint32_t      sector_size,
                  flog_read_fn  read,
                  flog_write_fn write,
                  flog_erase_fn erase)
{
    s.base     = base_addr;
    s.sec_size = sector_size;
    s.fn_read  = read;
    s.fn_write = write;
    s.fn_erase = erase;
    s.inited   = false;

    uint16_t num = (uint16_t)(region_size / sector_size);
    if (num > XY_FLOG_MAX_SECTORS) num = XY_FLOG_MAX_SECTORS;
    if (num < 2u) return;   /* need at least 2 sectors for ring rotation */
    s.num_sec = num;

    /* ── Pass 1: find the newest and oldest valid sectors ── */
    bool     any        = false;
    uint16_t max_seq    = 0u, min_seq = 0u;
    uint16_t max_idx    = 0u, min_idx = 0u;

    for (uint16_t i = 0u; i < s.num_sec; i++) {
        sector_hdr_t shdr;
        if (s.fn_read(sec_addr(i), &shdr, sizeof(shdr)) != 0) continue;
        if (shdr.magic != SECTOR_MAGIC) continue;

        if (!any) {
            max_seq = min_seq = shdr.seq;
            max_idx = min_idx = i;
            any = true;
        } else {
            if (seq_after(shdr.seq, max_seq)) { max_seq = shdr.seq; max_idx = i; }
            if (!seq_after(shdr.seq, min_seq)) { min_seq = shdr.seq; min_idx = i; }
        }
    }

    if (!any) {
        /* Blank flash — format sector 0. */
        if (_init_sector(0u, 0u) != 0) return;
        s.write_sec = 0u;
        s.read_sec  = 0u;
        s.write_off = SECTOR_HDR_SZ;
        s.next_seq  = 1u;
        s.count     = 0u;
        s.inited    = true;
        return;
    }

    s.write_sec = max_idx;
    s.read_sec  = min_idx;
    s.next_seq  = (uint16_t)(max_seq + 1u);

    /* ── Pass 2: walk sectors oldest→newest, count entries, find write_off ── */
    s.count = 0u;
    uint16_t sec = s.read_sec;

    for (;;) {
        sector_hdr_t shdr;
        if (s.fn_read(sec_addr(sec), &shdr, sizeof(shdr)) == 0 &&
            shdr.magic == SECTOR_MAGIC) {

            uint32_t off = 0u;
            _scan_sector(sec, NULL, NULL, &s.count, &off);

            if (sec == s.write_sec)
                s.write_off = off;
        }

        if (sec == s.write_sec) break;
        sec = (uint16_t)((sec + 1u) % s.num_sec);
    }

    s.inited = true;
}

/* ── Write ───────────────────────────────────────────────────────────── */

int xy_flog_write(xy_vlog_type_t type,
                  const void    *payload,
                  uint16_t       payload_len,
                  uint32_t       timestamp_ms)
{
    if (!s.inited) return -1;
    if (payload_len > XY_FLOG_MAX_PAYLOAD)
        payload_len = (uint16_t)XY_FLOG_MAX_PAYLOAD;

    uint32_t data_sz  = (uint32_t)XY_VLOG_HEADER_SIZE + payload_len;
    uint32_t entry_sz = data_sz + ENTRY_CRC_SZ;

    /* ── Advance to a new sector if the current one is full ── */
    if (s.write_off + entry_sz > s.sec_size) {
        uint16_t new_sec = (uint16_t)((s.write_sec + 1u) % s.num_sec);

        /* If new_sec is the oldest sector, discard it and advance read_sec. */
        if (new_sec == s.read_sec) {
            uint32_t lost = 0u;
            _scan_sector(s.read_sec, NULL, NULL, &lost, NULL);
            s.count    -= lost;
            s.read_sec  = (uint16_t)((s.read_sec + 1u) % s.num_sec);
        }

        if (_init_sector(new_sec, s.next_seq++) != 0) return -1;
        s.write_sec = new_sec;
        s.write_off = SECTOR_HDR_SZ;
    }

    /* ── Build entry in s_entry_buf ── */
    xy_vlog_hdr_t hdr = {
        .timestamp_ms = timestamp_ms,
        .log_type     = type,
        .reserved     = 0u,
        .payload_len  = payload_len,
    };
    memcpy(s_entry_buf, &hdr, XY_VLOG_HEADER_SIZE);
    if (payload && payload_len > 0u)
        memcpy(s_entry_buf + XY_VLOG_HEADER_SIZE, payload, payload_len);

    uint16_t crc = xy_crc16(s_entry_buf, data_sz);

    /* ── Write header + payload, then CRC ── */
    uint32_t dst = sec_addr(s.write_sec) + s.write_off;
    if (s.fn_write(dst, s_entry_buf, data_sz) != 0) return -1;

    uint8_t crc_le[2] = { (uint8_t)(crc & 0xFFu), (uint8_t)(crc >> 8u) };
    if (s.fn_write(dst + data_sz, crc_le, ENTRY_CRC_SZ) != 0) return -1;

    s.write_off += entry_sz;
    s.count++;
    return 0;
}

int xy_flog_printf(xy_vlog_type_t type, uint32_t timestamp_ms,
                   const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int n = (int)xy_vsnprintf((char *)s_printf_buf, sizeof(s_printf_buf), fmt, ap);
    va_end(ap);
    if (n <= 0) return -1;
    return xy_flog_write(type, s_printf_buf, (uint16_t)n, timestamp_ms);
}

/* ── Read / iterate ──────────────────────────────────────────────────── */

int xy_flog_foreach(xy_flog_cb_t cb, void *user)
{
    if (!s.inited || !cb || s.count == 0u) return 0;

    uint32_t total = 0u;
    uint16_t sec   = s.read_sec;

    for (;;) {
        sector_hdr_t shdr;
        if (s.fn_read(sec_addr(sec), &shdr, sizeof(shdr)) == 0 &&
            shdr.magic == SECTOR_MAGIC) {

            bool cont = _scan_sector(sec, cb, user, &total, NULL);
            if (!cont) break;   /* cb returned false */
        }

        if (sec == s.write_sec) break;
        sec = (uint16_t)((sec + 1u) % s.num_sec);
    }

    return (int)total;
}

/* ── Misc ────────────────────────────────────────────────────────────── */

int xy_flog_count(void)
{
    return (int)s.count;
}

int xy_flog_clear(void)
{
    if (!s.inited) return -1;

    for (uint16_t i = 0u; i < s.num_sec; i++) {
        if (s.fn_erase(sec_addr(i), s.sec_size) != 0) return -1;
    }

    if (_init_sector(0u, 0u) != 0) return -1;
    s.write_sec = 0u;
    s.read_sec  = 0u;
    s.write_off = SECTOR_HDR_SZ;
    s.next_seq  = 1u;
    s.count     = 0u;
    return 0;
}
