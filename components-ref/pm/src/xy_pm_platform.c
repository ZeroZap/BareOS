/**
 * @file xy_pm_platform.c
 * @brief Power Management Platform-Specific Implementations
 * @version 1.0.0
 * @date 2026-03-13
 *
 * @note This file provides platform detection and wrapper functions
 *       for different MCU platforms (STM32, CH32, HC32, etc.)
 */

#include "../inc/xy_pm.h"
#include <stdint.h>
#include <stdbool.h>

/* ============================================================
 * Platform Detection Macros
 * ============================================================ */
#if defined(STM32U5) || defined(STM32F4) || defined(STM32F1) || defined(STM32L4)
#define XY_PLATFORM_STM32     1
#else
#define XY_PLATFORM_STM32     0
#endif

#if defined(MCU_CH32) || defined(CH32V103) || defined(CH32V20X)
#define XY_PLATFORM_WCH       1
#else
#define XY_PLATFORM_WCH       0
#endif

#if defined(MCU_HC32) || defined(HC32L021) || defined(HC32L110)
#define XY_PLATFORM_HC32      1
#else
#define XY_PLATFORM_HC32      0
#endif

#if defined(CONFIG_PLATFORM_PC) || defined(PLATFORM_PC)
#define XY_PLATFORM_PC        1
#else
#define XY_PLATFORM_PC        0
#endif

/* ============================================================
 * OS Tick Wrapper - Replace stub with real OS tick
 * ============================================================ */

#if defined(XY_OSAL_AVAILABLE) || defined(CONFIG_XY_OSAL)
extern uint32_t xy_os_tick_get(void);
static inline uint32_t platform_tick_get(void) { return xy_os_tick_get(); }

#elif XY_PLATFORM_STM32
extern uint32_t HAL_GetTick(void);
static inline uint32_t platform_tick_get(void) { return HAL_GetTick(); }

#elif XY_PLATFORM_WCH
static volatile uint32_t g_wch_tick = 0;
static inline uint32_t platform_tick_get(void) { return g_wch_tick; }

#elif XY_PLATFORM_HC32
extern uint32_t xy_hal_sys_get_tick_count(void);
static inline uint32_t platform_tick_get(void) { return xy_hal_sys_get_tick_count(); }

#elif XY_PLATFORM_PC
#ifdef _WIN32
#include <windows.h>
static inline uint32_t platform_tick_get(void) { return GetTickCount(); }
#else
#include <time.h>
static inline uint32_t platform_tick_get(void) { return (uint32_t)(clock() * 1000 / CLOCKS_PER_SEC); }
#endif

#else
static volatile uint32_t g_stub_tick = 0;
static inline uint32_t platform_tick_get(void) { return g_stub_tick += 100; }
#endif

/**
 * @brief Get OS tick count (public interface)
 * @return Current tick count in milliseconds
 */
uint32_t xy_pm_tick_get(void)
{
    return platform_tick_get();
}

/* ============================================================
 * Charger GPIO Configuration
 * ============================================================ */

#ifndef CHARGER_EN_PORT
#define CHARGER_EN_PORT       0
#endif

#ifndef CHARGER_EN_PIN
#define CHARGER_EN_PIN        0
#endif

#ifndef CHARGER_EN_ACTIVE_HIGH
#define CHARGER_EN_ACTIVE_HIGH    1
#endif

/**
 * @brief Initialize charger hardware GPIO
 * @return XY_PM_OK on success, error code otherwise
 */
int xy_charger_hw_init(void)
{
#if XY_PLATFORM_STM32
    /* TODO: Initialize charger enable pin as output */
    return XY_PM_OK;

#elif XY_PLATFORM_WCH
    /* TODO: Initialize WCH charger GPIO */
    return XY_PM_OK;

#elif XY_PLATFORM_HC32
    /* TODO: Initialize HC32 charger GPIO */
    return XY_PM_OK;

#else
    return XY_PM_OK;
#endif
}

/**
 * @brief Enable charger hardware
 * @return XY_PM_OK on success, error code otherwise
 */
int xy_charger_hw_enable(void)
{
#if XY_PLATFORM_STM32
    /* TODO: xy_hal_gpio_write((void*)CHARGER_EN_PORT, CHARGER_EN_PIN, CHARGER_EN_ACTIVE_HIGH); */
    return XY_PM_OK;

#elif XY_PLATFORM_WCH
    /* TODO: WCH GPIO write for charger enable */
    return XY_PM_OK;

#elif XY_PLATFORM_HC32
    /* TODO: HC32 GPIO write for charger enable */
    return XY_PM_OK;

#else
    return XY_PM_OK;
#endif
}

/**
 * @brief Disable charger hardware
 * @return XY_PM_OK on success, error code otherwise
 */
int xy_charger_hw_disable(void)
{
    return xy_charger_hw_enable();
}

/**
 * @brief Get platform type string
 * @return Platform name string
 */
const char* xy_pm_get_platform_name(void)
{
#if XY_PLATFORM_STM32
    return "STM32";
#elif XY_PLATFORM_WCH
    return "WCH";
#elif XY_PLATFORM_HC32
    return "HC32";
#elif XY_PLATFORM_PC
    return "PC";
#else
    return "Unknown";
#endif
}

/**
 * @brief Check if running on specific platform
 * @param platform Platform identifier
 * @return true if running on specified platform
 */
bool xy_pm_is_platform(int platform)
{
    switch (platform) {
        case XY_PLATFORM_ID_STM32:
            return XY_PLATFORM_STM32 != 0;
        case XY_PLATFORM_ID_WCH:
            return XY_PLATFORM_WCH != 0;
        case XY_PLATFORM_ID_HC32:
            return XY_PLATFORM_HC32 != 0;
        case XY_PLATFORM_ID_PC:
            return XY_PLATFORM_PC != 0;
        default:
            return false;
    }
}
