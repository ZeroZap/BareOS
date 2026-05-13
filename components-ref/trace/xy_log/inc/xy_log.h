/**
 * @file xy_log.h
 * @brief Logging System
 */

#ifndef XY_LOG_H
#define XY_LOG_H

#include "xy_stdio.h"

/* Log levels */
#define XY_LOG_LEVEL_ERROR 0
#define XY_LOG_LEVEL_WARN  1
#define XY_LOG_LEVEL_INFO  2
#define XY_LOG_LEVEL_DEBUG 3

#ifndef XY_LOG_LEVEL
#define XY_LOG_LEVEL XY_LOG_LEVEL_INFO
#endif

/* Log macros */
#if XY_LOG_LEVEL >= XY_LOG_LEVEL_ERROR
#define XY_LOG_E(fmt, ...) xy_printf("[E] " fmt, ##__VA_ARGS__)
#else
#define XY_LOG_E(fmt, ...)
#endif

#if XY_LOG_LEVEL >= XY_LOG_LEVEL_INFO
#define XY_LOG_I(fmt, ...) xy_printf("[I] " fmt, ##__VA_ARGS__)
#else
#define XY_LOG_I(fmt, ...)
#endif

#if XY_LOG_LEVEL >= XY_LOG_LEVEL_DEBUG
#define XY_LOG_D(fmt, ...) xy_printf("[D] " fmt, ##__VA_ARGS__)
#else
#define XY_LOG_D(fmt, ...)
#endif

#endif /* XY_LOG_H */
