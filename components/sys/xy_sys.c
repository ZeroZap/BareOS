/**
 * @file xy_sys.c
 * @brief System control implementation.
 *
 * Calls BSP-provided weak hooks.  The BSP overrides the weak stubs below
 * with MCU-specific register accesses (e.g. STM32 RCC_CSR, NVIC_SystemReset).
 */

#include "xy_sys.h"

/* ── Cached reset cause ─────────────────────────────────────────────── */

static xy_reset_cause_t s_reset_cause = (xy_reset_cause_t)0xFFu;

xy_reset_cause_t xy_sys_get_reset_cause(void)
{
    if (s_reset_cause == (xy_reset_cause_t)0xFFu) {
        s_reset_cause = xy_sys_get_reset_cause_hw();
    }
    return s_reset_cause;
}

/* ── Reset ──────────────────────────────────────────────────────────── */

void xy_sys_reset(void)
{
    xy_sys_hw_reset();
    /* Should not reach here; spin if BSP stub is empty */
    for (;;) {}
}

/* ── Watchdog ───────────────────────────────────────────────────────── */

void xy_sys_watchdog_kick(void)
{
    xy_sys_watchdog_kick_hw();
}

/* ── Chip identity ──────────────────────────────────────────────────── */

int xy_sys_get_chip_id(uint8_t *buf, int maxlen)
{
    return xy_sys_get_chip_id_hw(buf, maxlen);
}

/* ── Reset cause string ─────────────────────────────────────────────── */

const char *xy_sys_reset_cause_str(xy_reset_cause_t cause)
{
    switch (cause) {
    case XY_RESET_CAUSE_POWER_ON:  return "power-on";
    case XY_RESET_CAUSE_EXTERNAL:  return "external-pin";
    case XY_RESET_CAUSE_SOFTWARE:  return "software";
    case XY_RESET_CAUSE_WATCHDOG:  return "watchdog";
    case XY_RESET_CAUSE_LOCKUP:    return "lockup";
    case XY_RESET_CAUSE_SLEEP:     return "sleep-wake";
    default:                       return "unknown";
    }
}

/* ── Weak BSP stubs (override in BSP layer) ─────────────────────────── */

__attribute__((weak)) void xy_sys_hw_reset(void)
{
    /* BSP override: call NVIC_SystemReset() or equivalent */
}

__attribute__((weak)) xy_reset_cause_t xy_sys_get_reset_cause_hw(void)
{
    /* BSP override: read MCU reset-cause flags register */
    return XY_RESET_CAUSE_UNKNOWN;
}

__attribute__((weak)) void xy_sys_watchdog_kick_hw(void)
{
    /* BSP override: write watchdog reload key register */
}

__attribute__((weak)) int xy_sys_get_chip_id_hw(uint8_t *buf, int maxlen)
{
    /* BSP override: copy UID registers into buf */
    (void)buf; (void)maxlen;
    return 0;
}
