/**
 * @file xy_vlog.c
 * @brief Variable-length typed log — RAM ring buffer, no heap.
 *
 * Layout of one entry in the byte ring:
 *   [xy_vlog_hdr_t (8 bytes)][payload (hdr.payload_len bytes)]
 *
 * The ring is managed with byte-level head/tail indices.  When writing an
 * entry that would exceed available space, old entries are evicted from the
 * tail (oldest-first).
 *
 * All payload reads use a single module-level static buffer (s_read_buf) so
 * no stack space beyond a few locals is consumed during iteration.
 */

#include "xy_vlog.h"
#include "xy_stdio.h"   /* xy_vsnprintf */
#include <string.h>

/* ── Module state ───────────────────────────────────────────────────── */

static struct {
    uint8_t  *buf;
    uint32_t  cap;
    uint32_t  head;     /* next byte to write (always < cap) */
    uint32_t  tail;     /* oldest entry start (always < cap) */
    uint32_t  used;
    uint32_t  count;
    bool      inited;
} s;

/* Static read buffer — avoids a 256-byte stack allocation per iteration. */
static uint8_t s_read_buf[XY_VLOG_MAX_PAYLOAD];

/* ── Ring helpers ────────────────────────────────────────────────────── */

static inline uint32_t ring_wrap(uint32_t idx)
{
    return idx % s.cap;
}

static void ring_write(uint32_t start, const void *src, uint32_t len)
{
    const uint8_t *p = (const uint8_t *)src;
    for (uint32_t i = 0; i < len; i++) {
        s.buf[ring_wrap(start + i)] = p[i];
    }
}

static void ring_read(uint32_t start, void *dst, uint32_t len)
{
    uint8_t *p = (uint8_t *)dst;
    for (uint32_t i = 0; i < len; i++) {
        p[i] = s.buf[ring_wrap(start + i)];
    }
}

/* Advance tail past the oldest entry. */
static void consume_one(void)
{
    if (s.count == 0) return;
    xy_vlog_hdr_t hdr;
    ring_read(s.tail, &hdr, XY_VLOG_HEADER_SIZE);
    uint32_t entry_size = XY_VLOG_HEADER_SIZE + hdr.payload_len;
    s.tail  = ring_wrap(s.tail + entry_size);
    s.used -= entry_size;
    s.count--;
}

/* ── Init ───────────────────────────────────────────────────────────── */

void xy_vlog_init(uint8_t *buf, uint32_t size)
{
    s.buf    = buf;
    s.cap    = size;
    s.head   = 0;
    s.tail   = 0;
    s.used   = 0;
    s.count  = 0;
    s.inited = true;
}

/* ── Write ──────────────────────────────────────────────────────────── */

int xy_vlog_write(xy_vlog_type_t type,
                  const void    *payload,
                  uint16_t       payload_len,
                  uint32_t       timestamp_ms)
{
    if (!s.inited) return -1;

    if (payload_len > XY_VLOG_MAX_PAYLOAD)
        payload_len = (uint16_t)XY_VLOG_MAX_PAYLOAD;

    uint32_t entry_size = XY_VLOG_HEADER_SIZE + payload_len;
    if (entry_size > s.cap) return -1;

    /* Evict old entries until there is room. */
    while (s.cap - s.used < entry_size)
        consume_one();

    /* Write header then payload. */
    xy_vlog_hdr_t hdr = {
        .timestamp_ms = timestamp_ms,
        .log_type     = type,
        .reserved     = 0,
        .payload_len  = payload_len,
    };
    ring_write(s.head, &hdr, XY_VLOG_HEADER_SIZE);
    if (payload && payload_len > 0)
        ring_write(s.head + XY_VLOG_HEADER_SIZE, payload, payload_len);

    s.head  = ring_wrap(s.head + entry_size);
    s.used += entry_size;
    s.count++;
    return 0;
}

int xy_vlog_printf(xy_vlog_type_t type, uint32_t timestamp_ms,
                   const char *fmt, ...)
{
    char buf[XY_VLOG_MAX_PAYLOAD];
    va_list ap;
    va_start(ap, fmt);
    int n = (int)xy_vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n <= 0) return -1;
    return xy_vlog_write(type, buf, (uint16_t)n, timestamp_ms);
}

/* ── Read / iterate ─────────────────────────────────────────────────── */

int xy_vlog_foreach(xy_vlog_cb_t cb, void *user)
{
    if (!s.inited || !cb || s.count == 0) return 0;

    int      visited = 0;
    uint32_t pos     = s.tail;

    for (uint32_t i = 0; i < s.count; i++) {
        xy_vlog_hdr_t hdr;
        ring_read(pos, &hdr, XY_VLOG_HEADER_SIZE);

        uint16_t plen = hdr.payload_len;
        if (plen > XY_VLOG_MAX_PAYLOAD) plen = XY_VLOG_MAX_PAYLOAD;

        /* Use the module-level static buffer — no stack pressure. */
        if (plen > 0)
            ring_read(pos + XY_VLOG_HEADER_SIZE, s_read_buf, plen);

        visited++;
        bool cont = cb(&hdr, plen > 0 ? s_read_buf : NULL, user);
        if (!cont) break;

        pos = ring_wrap(pos + XY_VLOG_HEADER_SIZE + plen);
    }
    return visited;
}

int xy_vlog_read_last(xy_vlog_entry_t *out, int count)
{
    if (!s.inited || !out || count <= 0 || s.count == 0) return 0;

    /* How many can we actually return? */
    int avail = (int)s.count;
    int skip  = (avail > count) ? (avail - count) : 0;

    /* Walk forward to the (avail - count)-th entry. */
    uint32_t pos = s.tail;
    for (int i = 0; i < skip; i++) {
        xy_vlog_hdr_t hdr;
        ring_read(pos, &hdr, XY_VLOG_HEADER_SIZE);
        uint16_t plen = hdr.payload_len;
        if (plen > XY_VLOG_MAX_PAYLOAD) plen = XY_VLOG_MAX_PAYLOAD;
        pos = ring_wrap(pos + XY_VLOG_HEADER_SIZE + plen);
    }

    /* Read up to 'count' entries into the output array. */
    int found = 0;
    int limit = (avail < count) ? avail : count;
    for (int i = 0; i < limit; i++) {
        ring_read(pos, &out[i].hdr, XY_VLOG_HEADER_SIZE);
        uint16_t plen = out[i].hdr.payload_len;
        if (plen > XY_VLOG_MAX_PAYLOAD) plen = XY_VLOG_MAX_PAYLOAD;
        if (plen > 0)
            ring_read(pos + XY_VLOG_HEADER_SIZE, out[i].payload, plen);
        out[i].payload_len = plen;
        pos = ring_wrap(pos + XY_VLOG_HEADER_SIZE + plen);
        found++;
    }
    return found;  /* oldest first */
}

/* ── Misc ───────────────────────────────────────────────────────────── */

int xy_vlog_count(void)
{
    return (int)s.count;
}

void xy_vlog_clear(void)
{
    if (!s.inited) return;
    s.head  = 0;
    s.tail  = 0;
    s.used  = 0;
    s.count = 0;
}

uint32_t xy_vlog_used_bytes(void)
{
    return s.used;
}
