/**
 * @file xy_param.h
 * @brief PLB device parameter storage — NVM key-value over Flash FEE Nano.
 *
 * Stores all persistent configuration items for the vessel distress module:
 *   - Device identity (MMSI, serial number)
 *   - Communication settings (server address, TX interval)
 *   - Operational flags
 *   - Last known position (cache for quick start)
 *
 * Built on top of xy_fee_nano (Flash EEPROM Emulation, no heap, bare-metal).
 *
 * Usage:
 *   // In bsp_init():
 *   static eflash_t fee;
 *   static uint8_t  fee_buf[FEE_TOTAL_SIZE];          // or map to real Flash
 *   eflash_init_with_buffer(&fee, fee_buf, sizeof(fee_buf), FEE_PAGE_SIZE);
 *   xy_param_init(&fee);
 *
 *   // Read:
 *   uint32_t mmsi = xy_param_get_mmsi();
 *
 *   // Write:
 *   xy_param_set_mmsi(123456789UL);
 *   xy_param_save();    // flush dirty items to Flash
 */

#ifndef XY_PARAM_H
#define XY_PARAM_H

#include <stdint.h>
#include <stdbool.h>
#include "../fee/xy_fee_nano.h"

/* ── Parameter IDs (logical address in FEE) ────────────────────────── */

typedef enum {
    XY_PARAM_ID_MMSI           = 0x01,  /* 4 bytes  — Maritime Mobile Service Identity */
    XY_PARAM_ID_SERIAL         = 0x02,  /* 16 bytes — Device serial number (ASCII)     */
    XY_PARAM_ID_TX_INTERVAL    = 0x03,  /* 1 byte   — Periodic TX interval (minutes)   */
    XY_PARAM_ID_FLAGS          = 0x04,  /* 1 byte   — Operational flags (see below)    */
    XY_PARAM_ID_SERVER_ADDR    = 0x05,  /* 32 bytes — Primary server hostname/IP:port  */
    XY_PARAM_ID_LAST_LAT       = 0x06,  /* 4 bytes  — Last fix latitude  × 1e7         */
    XY_PARAM_ID_LAST_LON       = 0x07,  /* 4 bytes  — Last fix longitude × 1e7         */
    XY_PARAM_ID_TX_COUNT       = 0x08,  /* 4 bytes  — Lifetime distress TX counter      */
    XY_PARAM_ID_MAX
} xy_param_id_t;

/* ── Operational flags (XY_PARAM_ID_FLAGS) ─────────────────────────── */

#define XY_PARAM_FLAG_SOS_LATCH      (1u << 0)  /* SOS active (survives reboot)     */
#define XY_PARAM_FLAG_SILENT_MODE    (1u << 1)  /* suppress LED/buzzer              */
#define XY_PARAM_FLAG_GNSS_ALWAYS_ON (1u << 2)  /* keep GNSS powered between TX     */

/* ── Defaults ───────────────────────────────────────────────────────── */

#define XY_PARAM_DEFAULT_MMSI        0UL          /* invalid; must be configured     */
#define XY_PARAM_DEFAULT_TX_INTERVAL 5U           /* 5 minutes between position TX   */
#define XY_PARAM_DEFAULT_FLAGS       0U
#define XY_PARAM_DEFAULT_SERVER      "0.0.0.0:8883"

/* ── Init ───────────────────────────────────────────────────────────── */

/**
 * Bind the param module to an initialised eflash_t instance and load
 * all parameters from Flash into the RAM shadow.
 * Call once during BSP / application init.
 */
void xy_param_init(eflash_t *fee);

/* ── Getters ────────────────────────────────────────────────────────── */

uint32_t xy_param_get_mmsi(void);
void     xy_param_get_serial(char *out, int maxlen);   /* NUL-terminated */
uint8_t  xy_param_get_tx_interval(void);               /* minutes        */
uint8_t  xy_param_get_flags(void);
void     xy_param_get_server(char *out, int maxlen);
int32_t  xy_param_get_last_lat(void);
int32_t  xy_param_get_last_lon(void);
uint32_t xy_param_get_tx_count(void);

/* ── Setters (updates RAM shadow only; call xy_param_save() to persist) */

void xy_param_set_mmsi(uint32_t mmsi);
void xy_param_set_serial(const char *serial);
void xy_param_set_tx_interval(uint8_t minutes);
void xy_param_set_flags(uint8_t flags);
void xy_param_set_flag(uint8_t mask, bool value);  /* set/clear single bit */
void xy_param_set_server(const char *addr);
void xy_param_set_last_pos(int32_t lat_1e7, int32_t lon_1e7);
void xy_param_inc_tx_count(void);   /* atomically increments + marks dirty */

/* ── Persistence ────────────────────────────────────────────────────── */

/**
 * Flush all dirty (changed) parameters to Flash.
 * Call after any set_* sequence completes.
 * Returns 0 on success, negative on Flash error.
 */
int xy_param_save(void);

/**
 * Erase all stored parameters and restore factory defaults.
 * Returns 0 on success.
 */
int xy_param_factory_reset(void);

#endif /* XY_PARAM_H */
