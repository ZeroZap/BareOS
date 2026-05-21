/**
 * @file xy_evtlog.c
 * @brief PLB event log — ring buffer over raw Flash, no heap.
 *
 * Layout in Flash:
 *   [base_addr .. base_addr + region_size)
 *   Slots: region_size / XY_EVTLOG_RECORD_SIZE
 *   write_idx points to the slot to be written next (ring).
 *
 * Ring state is recovered at power-on by scanning all slots:
 *   - Empty slot: all 0xFF bytes
 *   - Valid slot: passes CRC check
 *   - Corrupt slot: non-0xFF but CRC fails → treated as empty
 *
 * No separate metadata sector needed: write_idx is inferred from the
 * transition between the last valid record and the first empty slot.
 */

#include "xy_evtlog.h"
#include "xy_string.h"

/* ── CRC-16/CCITT ───────────────────────────────────────────────────── */

static uint16_t crc16(const uint8_t *data, uint32_t len)
{
    uint16_t crc = 0xFFFFu;
    for (uint32_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) {
            if (crc & 1u) crc = (uint16_t)((crc >> 1) ^ 0xA001u);
            else          crc >>= 1;
        }
    }
    return crc;
}

/* ── Module state ───────────────────────────────────────────────────── */

static struct {
    uint32_t         base;
    uint32_t         region_size;
    uint32_t         page_size;
    uint32_t         total_slots;
    uint32_t         write_idx;   /* next slot to write into (ring) */
    evtlog_erase_fn  erase;
    evtlog_write_fn  write;
    evtlog_read_fn   read;
    bool             inited;
} s;

/* ── Internal helpers ───────────────────────────────────────────────── */

static uint32_t slot_addr(uint32_t idx)
{
    return s.base + (idx % s.total_slots) * XY_EVTLOG_RECORD_SIZE;
}

static bool slot_is_empty(uint32_t idx)
{
    uint8_t buf[XY_EVTLOG_RECORD_SIZE];
    s.read(slot_addr(idx), buf, XY_EVTLOG_RECORD_SIZE);
    for (int i = 0; i < XY_EVTLOG_RECORD_SIZE; i++) {
        if (buf[i] != 0xFF) return false;
    }
    return true;
}

static bool slot_is_valid(uint32_t idx, xy_evtlog_entry_t *out)
{
    s.read(slot_addr(idx), out, XY_EVTLOG_RECORD_SIZE);
    uint16_t calc = crc16((const uint8_t *)out,
                          XY_EVTLOG_RECORD_SIZE - sizeof(uint16_t));
    return (calc == out->crc16);
}

/* Erase the Flash page(s) covering slot @idx if they are full. */
static void erase_if_needed(uint32_t idx)
{
    uint32_t addr    = slot_addr(idx);
    uint32_t pg_base = (addr / s.page_size) * s.page_size;
    if (!slot_is_empty(idx)) {
        s.erase(pg_base, s.page_size);
    }
}

/* Scan Flash to find the write_idx (next empty slot after last valid). */
static void recover_write_idx(void)
{
    /* Find the first empty slot after a run of valid slots. */
    uint32_t i;
    for (i = 0; i < s.total_slots; i++) {
        if (slot_is_empty(i)) break;
    }
    s.write_idx = i % s.total_slots;
}

/* ── Public API ─────────────────────────────────────────────────────── */

void xy_evtlog_init(uint32_t base_addr,
                    uint32_t region_size,
                    uint32_t page_size,
                    evtlog_erase_fn erase,
                    evtlog_write_fn write,
                    evtlog_read_fn  read)
{
    s.base        = base_addr;
    s.region_size = region_size;
    s.page_size   = page_size;
    s.total_slots = region_size / XY_EVTLOG_RECORD_SIZE;
    s.erase       = erase;
    s.write       = write;
    s.read        = read;
    s.inited      = true;

    recover_write_idx();
}

int xy_evtlog_write(xy_evtlog_type_t type,
                    uint8_t          status,
                    int32_t          lat_1e7,
                    int32_t          lon_1e7,
                    uint32_t         timestamp_s)
{
    if (!s.inited) return -1;

    xy_evtlog_entry_t e;
    e.timestamp_s = timestamp_s;
    e.event_type  = (uint8_t)type;
    e.status      = status;
    e.lat_1e7     = lat_1e7;
    e.lon_1e7     = lon_1e7;
    e.crc16       = crc16((const uint8_t *)&e,
                          XY_EVTLOG_RECORD_SIZE - sizeof(uint16_t));

    /* Erase the target Flash page before overwriting if needed. */
    erase_if_needed(s.write_idx);

    int rc = s.write(slot_addr(s.write_idx), &e, XY_EVTLOG_RECORD_SIZE);

    s.write_idx = (s.write_idx + 1) % s.total_slots;
    return (rc == 0) ? 0 : -1;
}

int xy_evtlog_read_last(xy_evtlog_entry_t *buf, int count)
{
    if (!s.inited || !buf || count <= 0) return 0;

    /* Walk backwards from write_idx - 1. */
    int found = 0;
    uint32_t idx = (s.write_idx + s.total_slots - 1) % s.total_slots;

    for (uint32_t i = 0; i < s.total_slots && found < count; i++) {
        xy_evtlog_entry_t tmp;
        if (slot_is_valid(idx, &tmp)) {
            buf[found++] = tmp;
        }
        if (idx == 0) idx = s.total_slots - 1;
        else          idx--;
    }

    /* Reverse so oldest is first in the output array. */
    for (int lo = 0, hi = found - 1; lo < hi; lo++, hi--) {
        xy_evtlog_entry_t t = buf[lo];
        buf[lo] = buf[hi];
        buf[hi] = t;
    }
    return found;
}

int xy_evtlog_count(void)
{
    if (!s.inited) return 0;
    int cnt = 0;
    for (uint32_t i = 0; i < s.total_slots; i++) {
        xy_evtlog_entry_t tmp;
        if (slot_is_valid(i, &tmp)) cnt++;
    }
    return cnt;
}

int xy_evtlog_clear(void)
{
    if (!s.inited) return -1;
    for (uint32_t addr = s.base;
         addr < s.base + s.region_size;
         addr += s.page_size) {
        int rc = s.erase(addr, s.page_size);
        if (rc != 0) return -1;
    }
    s.write_idx = 0;
    return 0;
}
