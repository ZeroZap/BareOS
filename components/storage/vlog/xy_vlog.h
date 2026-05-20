/**
 * @file xy_vlog.h
 * @brief Variable-length typed log — RAM ring buffer, no heap.
 *
 * Stores variable-length log entries in a static RAM ring buffer.
 * Each entry has a fixed 8-byte header followed by a variable payload:
 *
 *   [0..3]  timestamp_ms  — ms since boot (or epoch)
 *   [4]     log_type      — xy_vlog_type_t (application-defined)
 *   [5]     reserved      — 0x00
 *   [6..7]  payload_len   — length of the payload that follows (0..XY_VLOG_MAX_PAYLOAD)
 *   [8..]   payload       — raw bytes (up to payload_len)
 *
 * The ring buffer uses byte-level addressing.  When the buffer is full,
 * the oldest entry is silently overwritten (oldest-first eviction).
 *
 * This is a RAM-only log — entries are lost on reset.  For persistent
 * storage use components/storage/evtlog/ (fixed 16-byte Flash ring).
 *
 * Usage:
 *   // Static backing buffer (must be ≥ XY_VLOG_BUF_SIZE bytes):
 *   static uint8_t vlog_buf[XY_VLOG_BUF_SIZE];
 *   xy_vlog_init(vlog_buf, sizeof(vlog_buf));
 *
 *   // Write:
 *   xy_vlog_write(XY_VLOG_INFO, "GPS fix", 7, xy_sys_tick_ms());
 *
 *   // Iterate (oldest first):
 *   xy_vlog_foreach(my_print_cb, NULL);
 */

#ifndef XY_VLOG_H
#define XY_VLOG_H

#include <stdint.h>
#include "xy_typedef.h"
#include "xy_typedef.h"
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Configuration ──────────────────────────────────────────────────── */

#ifndef XY_VLOG_BUF_SIZE
#define XY_VLOG_BUF_SIZE    2048u   /* total RAM for the log ring buffer */
#endif

#ifndef XY_VLOG_MAX_PAYLOAD
#define XY_VLOG_MAX_PAYLOAD 255u    /* max payload bytes per entry       */
#endif

/* ── Log types ──────────────────────────────────────────────────────── */

typedef uint8_t xy_vlog_type_t;

#define XY_VLOG_DEBUG       0x00u
#define XY_VLOG_INFO        0x01u
#define XY_VLOG_WARN        0x02u
#define XY_VLOG_ERROR       0x03u
#define XY_VLOG_GNSS        0x10u   /* GNSS-related data                 */
#define XY_VLOG_COMM        0x11u   /* Network / modem events            */
#define XY_VLOG_SOS         0x12u   /* Distress events                   */
#define XY_VLOG_POWER       0x13u   /* Battery / power events            */
#define XY_VLOG_USER_BASE   0x20u   /* Application-defined types start   */

/* ── Entry header (8 bytes, packed) ─────────────────────────────────── */

#define XY_VLOG_HEADER_SIZE 8u

typedef struct __attribute__((packed)) {
    uint32_t timestamp_ms;  /* ms since boot or epoch              */
    uint8_t  log_type;      /* xy_vlog_type_t                      */
    uint8_t  reserved;      /* 0x00                                */
    uint16_t payload_len;   /* bytes that follow this header       */
} xy_vlog_hdr_t;

/* ── Callback for iteration ─────────────────────────────────────────── */

/**
 * Called by xy_vlog_foreach() for each valid entry, oldest first.
 * @param hdr      Pointer to the entry header.
 * @param payload  Pointer to payload bytes (hdr->payload_len bytes).
 * @param user     User context passed to xy_vlog_foreach().
 * @return false to stop iteration early, true to continue.
 */
typedef bool (*xy_vlog_cb_t)(const xy_vlog_hdr_t *hdr,
                              const uint8_t       *payload,
                              void                *user);

/* ── Entry struct for xy_vlog_read_last() ───────────────────────────── */

typedef struct {
    xy_vlog_hdr_t hdr;
    uint8_t       payload[XY_VLOG_MAX_PAYLOAD];
    uint16_t      payload_len;  /* actual bytes valid in payload[]  */
} xy_vlog_entry_t;

/* ── API ─────────────────────────────────────────────────────────────── */

/**
 * Initialise the log with a caller-supplied backing buffer.
 * @param buf   Static byte array for the ring.
 * @param size  sizeof(buf); must be > XY_VLOG_HEADER_SIZE.
 */
void xy_vlog_init(uint8_t *buf, uint32_t size);

/**
 * Append one log entry.
 * @param type         Entry type (xy_vlog_type_t).
 * @param payload      Payload bytes (may be NULL if payload_len == 0).
 * @param payload_len  Number of payload bytes (clamped to XY_VLOG_MAX_PAYLOAD).
 * @param timestamp_ms Caller-supplied timestamp (typically g_sys_tick_ms).
 * @return 0 on success, -1 if not initialised or entry too large.
 */
int xy_vlog_write(xy_vlog_type_t type,
                  const void    *payload,
                  uint16_t       payload_len,
                  uint32_t       timestamp_ms);

/**
 * Write a printf-formatted text entry.
 * The formatted string becomes the payload (NUL not stored).
 * @return 0 on success, -1 on error.
 */
int xy_vlog_printf(xy_vlog_type_t type, uint32_t timestamp_ms,
                   const char *fmt, ...);

/**
 * Read up to @count of the most recent entries into @out (oldest first).
 * @return Actual number of entries filled (0 .. count).
 */
int xy_vlog_read_last(xy_vlog_entry_t *out, int count);

/**
 * Iterate all stored entries from oldest to newest.
 * @param cb    Callback invoked per entry.
 * @param user  Passed through to @cb.
 * @return Number of entries visited.
 */
int xy_vlog_foreach(xy_vlog_cb_t cb, void *user);

/**
 * Return the number of complete entries currently stored.
 */
int xy_vlog_count(void);

/**
 * Discard all log entries.
 */
void xy_vlog_clear(void);

/**
 * Return how many bytes of the backing buffer are currently in use.
 */
uint32_t xy_vlog_used_bytes(void);

#ifdef __cplusplus
}
#endif

#endif /* XY_VLOG_H */
