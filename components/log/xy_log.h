/**
 * @file xy_log.h
 * @brief Logging System
 *
 * BSP integration requirement:
 *   The application BSP must provide:
 *     void xy_log_char(char ch);   // output one byte (e.g. to UART TX)
 *   Then call xy_log_init() once during startup to wire xy_printf to it.
 */

#ifndef XY_LOG_H
#define XY_LOG_H

#include <stddef.h>
#include <stdint.h>
#include "xy_stdio.h"

/* ── Log levels ─────────────────────────────────────────────────────── */

#define XY_LOG_LEVEL_ERROR 0
#define XY_LOG_LEVEL_WARN  1
#define XY_LOG_LEVEL_INFO  2
#define XY_LOG_LEVEL_DEBUG 3

#ifndef XY_LOG_LEVEL
#define XY_LOG_LEVEL XY_LOG_LEVEL_INFO
#endif

/* ── Dynamic (runtime) level ────────────────────────────────────────── */

extern uint8_t g_xy_log_dinamic_level;

/* ── Log macros ─────────────────────────────────────────────────────── */

#if XY_LOG_LEVEL >= XY_LOG_LEVEL_ERROR
#define XY_LOG_E(fmt, ...) \
    do { if (g_xy_log_dinamic_level >= XY_LOG_LEVEL_ERROR) xy_printf("[E] " fmt "\r\n", ##__VA_ARGS__); } while (0)
#else
#define XY_LOG_E(fmt, ...)
#endif

#if XY_LOG_LEVEL >= XY_LOG_LEVEL_WARN
#define XY_LOG_W(fmt, ...) \
    do { if (g_xy_log_dinamic_level >= XY_LOG_LEVEL_WARN)  xy_printf("[W] " fmt "\r\n", ##__VA_ARGS__); } while (0)
#else
#define XY_LOG_W(fmt, ...)
#endif

#if XY_LOG_LEVEL >= XY_LOG_LEVEL_INFO
#define XY_LOG_I(fmt, ...) \
    do { if (g_xy_log_dinamic_level >= XY_LOG_LEVEL_INFO)  xy_printf("[I] " fmt "\r\n", ##__VA_ARGS__); } while (0)
#else
#define XY_LOG_I(fmt, ...)
#endif

#if XY_LOG_LEVEL >= XY_LOG_LEVEL_DEBUG
#define XY_LOG_D(fmt, ...) \
    do { if (g_xy_log_dinamic_level >= XY_LOG_LEVEL_DEBUG) xy_printf("[D] " fmt "\r\n", ##__VA_ARGS__); } while (0)
#else
#define XY_LOG_D(fmt, ...)
#endif

/* ── API ─────────────────────────────────────────────────────────────── */

/**
 * Wire xy_printf to the BSP-provided xy_log_char() output.
 * Call once during BSP init before any log macros are used.
 */
void xy_log_init(void);

/**
 * Output raw bytes (no formatting) directly via xy_log_char().
 */
void xy_log_raw(char *data, size_t len);

/**
 * Change the runtime log level (XY_LOG_LEVEL_ERROR .. XY_LOG_LEVEL_DEBUG).
 */
void xy_log_set_dynamic_level(uint8_t level);

/**
 * Return the current runtime log level.
 */
uint8_t xy_log_dynamic_level(void);

#endif /* XY_LOG_H */
