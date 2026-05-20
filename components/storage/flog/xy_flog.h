/**
 * @file xy_flog.h
 * @brief Persistent variable-length log in Flash — sector-ring, no heap.
 *
 * Fills the gap between xy_vlog (RAM-only, lost on reset) and xy_evtlog
 * (fixed 16-byte records in Flash).  xy_flog stores variable-length entries
 * (same header format as xy_vlog) persistently across resets.
 *
 * ── Flash layout ───────────────────────────────────────────────────────
 *
 *   Region = N sectors of sector_size bytes each.
 *
 *   Each sector:
 *     [4-byte sector header: magic(2) + seq(2)]
 *     [entry 0: hdr(8) + payload + crc16(2)]
 *     [entry 1: hdr(8) + payload + crc16(2)]
 *     ...
 *     [0xFF…  (erased, unused space)]
 *
 *   Sectors form a ring: when the current write sector is full, the next
 *   sector is erased and becomes the new write sector.  If that sector
 *   was the oldest, its entries are silently discarded (ring overflow).
 *
 * ── Survives reset ─────────────────────────────────────────────────────
 *
 *   xy_flog_init() scans all sector headers (4 bytes each) to find the
 *   write position and oldest valid sector, then picks up where it left off.
 *
 * ── Flash backend ──────────────────────────────────────────────────────
 *
 *   Three callbacks abstract the hardware:
 *     flog_read_fn  — read bytes (must work at byte granularity)
 *     flog_write_fn — program bytes (NOR page-program; BSP splits across pages)
 *     flog_erase_fn — erase exactly sector_size bytes at the given address
 *
 *   Works with both internal MCU flash (e.g. STM32 with 2 KB pages) and
 *   external NOR flash via xy_nor (4 KB sectors).
 *
 * ── Quick start ────────────────────────────────────────────────────────
 *
 *   // Internal flash:
 *   xy_flog_init(FLASH_LOG_BASE, FLASH_LOG_SIZE, 2048,
 *                intflash_read, intflash_write, intflash_erase);
 *
 *   // External NOR via xy_nor:
 *   xy_flog_init(0x100000, 256*4096, 4096,
 *                nor_read_cb, nor_write_cb, nor_erase_cb);
 *
 *   xy_flog_write(XY_VLOG_INFO, buf, len, g_sys_tick_ms);
 *   xy_flog_printf(XY_VLOG_ERROR, g_sys_tick_ms, "GNSS fix lost at %u", lat);
 *
 *   xy_flog_foreach(my_print_cb, NULL);
 */

#ifndef XY_FLOG_H
#define XY_FLOG_H

#include "xy_vlog.h"   /* xy_vlog_hdr_t, xy_vlog_type_t, XY_VLOG_HEADER_SIZE */
#include <stdint.h>
#include "xy_typedef.h"
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Configuration ──────────────────────────────────────────────────── */

/** Maximum number of sectors in the flash region.  Increase if needed. */
#ifndef XY_FLOG_MAX_SECTORS
#define XY_FLOG_MAX_SECTORS  64u
#endif

/** Maximum payload bytes per entry (clamped on write, used to size buffers). */
#ifndef XY_FLOG_MAX_PAYLOAD
#define XY_FLOG_MAX_PAYLOAD  255u
#endif

/* ── Flash I/O callbacks ─────────────────────────────────────────────── */

/** Read @len bytes from @addr into @buf.  Return 0 on success. */
typedef int (*flog_read_fn)(uint32_t addr, void *buf, uint32_t len);

/** Program @len bytes from @buf to @addr (NOR flash page-program semantics).
 *  BSP is responsible for splitting writes across page boundaries.
 *  Return 0 on success. */
typedef int (*flog_write_fn)(uint32_t addr, const void *buf, uint32_t len);

/** Erase exactly @size bytes (one sector) starting at @addr.  Return 0 on success. */
typedef int (*flog_erase_fn)(uint32_t addr, uint32_t size);

/* ── Iteration callback ─────────────────────────────────────────────── */

/**
 * Called by xy_flog_foreach() for each valid entry, oldest first.
 * @param hdr      Entry header (timestamp, type, payload_len).
 * @param payload  Payload bytes; NULL if payload_len == 0.
 * @param user     Caller context.
 * @return false to stop iteration, true to continue.
 */
typedef bool (*xy_flog_cb_t)(const xy_vlog_hdr_t *hdr,
                              const uint8_t       *payload,
                              void                *user);

/* ── API ─────────────────────────────────────────────────────────────── */

/**
 * Initialise the flash log.
 *
 * Scans the flash region to locate the current write position and oldest
 * valid sector.  Safe to call on blank (all-0xFF) flash — initialises the
 * first sector automatically.
 *
 * @param base_addr   Start address of the flash region reserved for the log.
 * @param region_size Total size of the region in bytes.  Must be an exact
 *                    multiple of @sector_size.
 * @param sector_size Erase granularity in bytes (e.g. 2048 for STM32 F4
 *                    internal flash, 4096 for most NOR flash).  The region
 *                    must contain at least 2 sectors.
 * @param read        Flash read callback.
 * @param write       Flash write (program) callback.
 * @param erase       Flash erase callback.
 */
void xy_flog_init(uint32_t      base_addr,
                  uint32_t      region_size,
                  uint32_t      sector_size,
                  flog_read_fn  read,
                  flog_write_fn write,
                  flog_erase_fn erase);

/**
 * Append a binary log entry to flash.
 * @param type         Entry type (XY_VLOG_INFO, XY_VLOG_ERROR, …).
 * @param payload      Payload bytes; may be NULL when payload_len == 0.
 * @param payload_len  Bytes of payload (clamped to XY_FLOG_MAX_PAYLOAD).
 * @param timestamp_ms Caller-supplied timestamp (typically g_sys_tick_ms).
 * @return 0 on success, -1 if not initialised or flash write failed.
 */
int xy_flog_write(xy_vlog_type_t type,
                  const void    *payload,
                  uint16_t       payload_len,
                  uint32_t       timestamp_ms);

/**
 * Append a printf-formatted text entry to flash.
 * The formatted string (without NUL) is the payload.
 * @return 0 on success, -1 on error.
 */
int xy_flog_printf(xy_vlog_type_t type, uint32_t timestamp_ms,
                   const char *fmt, ...);

/**
 * Iterate all stored valid entries from oldest to newest.
 * @param cb    Called for each valid entry; return false to stop early.
 * @param user  Passed through to @cb.
 * @return Number of valid entries visited.
 */
int xy_flog_foreach(xy_flog_cb_t cb, void *user);

/**
 * Return the number of valid log entries currently stored.
 * (Cached — no flash scan required.)
 */
int xy_flog_count(void);

/**
 * Erase all entries (erases the entire flash region).
 * Reinitialises sector 0 after erasing.
 * @return 0 on success, -1 on flash error.
 */
int xy_flog_clear(void);

#ifdef __cplusplus
}
#endif

#endif /* XY_FLOG_H */
