/**
 * @file xy_sys.h
 * @brief System control: reset cause, software reset, watchdog, version info.
 *
 * BSP integration:
 *   The BSP must provide __attribute__((weak)) overrides (or direct calls) for:
 *     - xy_sys_hw_reset()           — trigger MCU reset (e.g. NVIC_SystemReset())
 *     - xy_sys_get_reset_cause_hw() — read MCU reset-cause register
 *     - xy_sys_watchdog_kick_hw()   — pet the hardware watchdog
 *     - xy_sys_get_chip_id_hw()     — read chip UID registers
 */

#ifndef XY_SYS_H
#define XY_SYS_H

#include <stdint.h>
#include "xy_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ── Reset causes ───────────────────────────────────────────────────── */

typedef enum {
    XY_RESET_CAUSE_UNKNOWN    = 0x00,
    XY_RESET_CAUSE_POWER_ON   = 0x01,   /* POR / BOR                      */
    XY_RESET_CAUSE_EXTERNAL   = 0x02,   /* NRST pin                       */
    XY_RESET_CAUSE_SOFTWARE   = 0x04,   /* xy_sys_reset() / NVIC_SystemReset */
    XY_RESET_CAUSE_WATCHDOG   = 0x08,   /* IWDG or WWDG fired             */
    XY_RESET_CAUSE_LOCKUP     = 0x10,   /* CPU lockup / hard-fault reset  */
    XY_RESET_CAUSE_SLEEP      = 0x20,   /* Woke from deep-sleep / standby */
} xy_reset_cause_t;

/* ── Reset ──────────────────────────────────────────────────────────── */

/**
 * Trigger an immediate software reset.
 * BSP must implement: void xy_sys_hw_reset(void);
 */
void xy_sys_reset(void);

/**
 * Return the reset cause detected on the last boot.
 * Calls BSP hook xy_sys_get_reset_cause_hw() which reads MCU registers.
 * The result is cached after first call.
 */
xy_reset_cause_t xy_sys_get_reset_cause(void);

/**
 * Return a printable string for a reset cause.
 */
const char *xy_sys_reset_cause_str(xy_reset_cause_t cause);

/* ── Watchdog ───────────────────────────────────────────────────────── */

/**
 * Kick / refresh the hardware watchdog.
 * BSP must implement: void xy_sys_watchdog_kick_hw(void);
 * If no watchdog is used, leave the BSP stub empty.
 */
void xy_sys_watchdog_kick(void);

/* ── Chip identity ──────────────────────────────────────────────────── */

/**
 * Copy at most @maxlen bytes of the chip UID into @buf.
 * BSP must implement: int xy_sys_get_chip_id_hw(uint8_t *buf, int maxlen);
 * Returns number of bytes written, or 0 if not supported.
 */
int xy_sys_get_chip_id(uint8_t *buf, int maxlen);

/* ── Version strings ────────────────────────────────────────────────── */

/**
 * Application must define XY_SW_VERSION_STR (e.g. "1.2.3") and
 * XY_HW_VERSION_STR (e.g. "PCB-rev2") in its build config or main.h.
 * These fall back to "unknown" if not defined.
 */
#ifndef XY_SW_VERSION_STR
#define XY_SW_VERSION_STR  "0.0.0"
#endif

#ifndef XY_HW_VERSION_STR
#define XY_HW_VERSION_STR  "unknown"
#endif

static inline const char *xy_sys_get_sw_ver(void) { return XY_SW_VERSION_STR; }
static inline const char *xy_sys_get_hw_ver(void) { return XY_HW_VERSION_STR; }

/* ── BSP hooks (implement in BSP layer) ─────────────────────────────── */

void xy_sys_hw_reset(void);
xy_reset_cause_t xy_sys_get_reset_cause_hw(void);
void xy_sys_watchdog_kick_hw(void);
int  xy_sys_get_chip_id_hw(uint8_t *buf, int maxlen);

#ifdef __cplusplus
}
#endif

#endif /* XY_SYS_H */
