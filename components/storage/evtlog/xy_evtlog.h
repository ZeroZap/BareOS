/**
 * @file xy_evtlog.h
 * @brief PLB event log — fixed-length ring buffer in Flash, no heap.
 *
 * Stores a fixed-size circular log of distress events and position
 * transmissions directly in a dedicated Flash region.  Oldest records
 * are overwritten when the ring is full.
 *
 * Record layout (16 bytes, flash-word aligned):
 *   [0..3]  timestamp_s  — seconds since epoch (or sys_tick / 1000)
 *   [4]     event_type   — xy_evtlog_type_t
 *   [5]     status       — event-specific status byte
 *   [6..9]  lat_1e7      — latitude  × 1e7 (int32_t)
 *   [10..13] lon_1e7     — longitude × 1e7 (int32_t)
 *   [14..15] crc16       — CRC-16/CCITT of bytes [0..13]
 *
 * Usage:
 *   // Init once at startup (point at a raw Flash sector):
 *   xy_evtlog_init(FLASH_LOG_BASE_ADDR, FLASH_LOG_SECTOR_COUNT, FLASH_PAGE_SIZE,
 *                  flash_erase_fn, flash_write_fn, flash_read_fn);
 *
 *   // Write:
 *   xy_evtlog_write(XY_EVT_SOS_TRIGGERED, 0, lat, lon, timestamp);
 *
 *   // Read last N entries:
 *   xy_evtlog_entry_t buf[10];
 *   int n = xy_evtlog_read_last(buf, 10);
 */

#ifndef XY_EVTLOG_H
#define XY_EVTLOG_H

#include <stdint.h>
#include "xy_typedef.h"

/* ── Event types ────────────────────────────────────────────────────── */

typedef enum {
    XY_EVT_NONE          = 0x00,
    XY_EVT_SOS_TRIGGERED = 0x01,   /* SOS button pressed                  */
    XY_EVT_SOS_CANCELLED = 0x02,   /* SOS cancelled by operator           */
    XY_EVT_POS_TX_OK     = 0x10,   /* Position transmitted successfully   */
    XY_EVT_POS_TX_FAIL   = 0x11,   /* Position TX failed (no network)     */
    XY_EVT_GNSS_FIX      = 0x20,   /* GNSS fix acquired                   */
    XY_EVT_GNSS_LOST     = 0x21,   /* GNSS fix lost                       */
    XY_EVT_POWER_ON      = 0x30,   /* Device powered on                   */
    XY_EVT_POWER_OFF     = 0x31,   /* Device powered off (graceful)       */
    XY_EVT_WATCHDOG_RST  = 0x32,   /* Watchdog reset occurred             */
    XY_EVT_LOW_BATTERY   = 0x40,   /* Battery below threshold             */
} xy_evtlog_type_t;

/* ── Record ─────────────────────────────────────────────────────────── */

#define XY_EVTLOG_RECORD_SIZE  16u  /* bytes per record (must be power of 2) */

typedef struct __attribute__((packed)) {
    uint32_t timestamp_s;   /* seconds since epoch or boot tick/1000 */
    uint8_t  event_type;    /* xy_evtlog_type_t                      */
    uint8_t  status;        /* event-specific extra info             */
    int32_t  lat_1e7;       /* latitude  × 1e7                       */
    int32_t  lon_1e7;       /* longitude × 1e7                       */
    uint16_t crc16;         /* CRC-16/CCITT of bytes [0..13]         */
} xy_evtlog_entry_t;        /* total: 16 bytes                       */

/* ── Flash operation callbacks ─────────────────────────────────────── */

typedef int (*evtlog_erase_fn)(uint32_t addr, uint32_t size);
typedef int (*evtlog_write_fn)(uint32_t addr, const void *buf, uint32_t len);
typedef int (*evtlog_read_fn) (uint32_t addr, void *buf, uint32_t len);

/* ── Init ───────────────────────────────────────────────────────────── */

/**
 * Initialise the event log.
 *
 * @param base_addr      First byte address of the Flash region reserved for the log.
 * @param region_size    Total size of the Flash region in bytes.
 * @param page_size      Erase granularity (bytes); typically 4096 or 512.
 * @param erase          Flash erase callback (erases page_size bytes at addr).
 * @param write          Flash write callback.
 * @param read           Flash read callback.
 */
void xy_evtlog_init(uint32_t      base_addr,
                    uint32_t      region_size,
                    uint32_t      page_size,
                    evtlog_erase_fn erase,
                    evtlog_write_fn write,
                    evtlog_read_fn  read);

/* ── Write ──────────────────────────────────────────────────────────── */

/**
 * Append one event record.  If the ring is full the oldest record is
 * silently overwritten.  CRC is computed automatically.
 *
 * @return 0 on success, -1 on Flash write error.
 */
int xy_evtlog_write(xy_evtlog_type_t type,
                    uint8_t          status,
                    int32_t          lat_1e7,
                    int32_t          lon_1e7,
                    uint32_t         timestamp_s);

/* ── Read ───────────────────────────────────────────────────────────── */

/**
 * Read up to @count of the most recent entries into @buf (newest last).
 * Entries with invalid CRC are skipped.
 * Returns the actual number of valid entries read (0..count).
 */
int xy_evtlog_read_last(xy_evtlog_entry_t *buf, int count);

/**
 * Return the total number of valid records currently stored.
 */
int xy_evtlog_count(void);

/**
 * Erase all log records.
 * Returns 0 on success.
 */
int xy_evtlog_clear(void);

#endif /* XY_EVTLOG_H */
