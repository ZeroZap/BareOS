/**
 * @file xy_config.h
 * @brief Configuration file for xy_clib library
 *
 * This file contains compile-time configuration options for the xy_clib
 * library.
 */

#ifndef _XY_CONFIG_H_
#define _XY_CONFIG_H_

/* Version information */
#define _VERSION    0
#define _SUBVERSION 0
#define _REVISION   1
#define XY_VERSION  ((_VERSION << 16) | (_SUBVERSION << 8) | (_REVISION))

/**
 * @brief Enable software division (no hardware divide instruction)
 *
 * When enabled, uses shift and multiply approximations instead of division.
 * Useful for MCUs without hardware divide or for optimization.
 * 0 = Use hardware division
 * 1 = Use software division
 */
#ifndef XY_USE_SOFT_DIV
#define XY_USE_SOFT_DIV 0
#endif

/**
 * @brief Enable floating point support in printf
 *
 * When enabled, xy_stdio_printf supports %f format specifier.
 * Increases code size and stack usage.
 */
#ifndef XY_PRINTF_FLOAT_ENABLE
#define XY_PRINTF_FLOAT_ENABLE 1
#endif

/**
 * @brief Global printf buffer size
 *
 * Size of the static buffer used by printf functions.
 * Larger values support longer formatted strings but use more RAM.
 */
#ifndef XY_PRINTF_BUFSIZE
#define XY_PRINTF_BUFSIZE 1024
#endif

/**
 * @brief Use custom memory functions
 *
 * When defined, xy_string functions use custom implementations.
 */
#ifndef _XY_STRING_USED_XY_MEM_
#define _XY_STRING_USED_XY_MEM_ 1
#endif

/**
 * @brief Minimize character attribute table
 *
 * 0 = Use 256-byte table (full ASCII range)
 * 1 = Use 128-byte table (reduced memory, additional bounds checking)
 */
#ifndef MINIMIZE_CATTR_TABLE
#define MINIMIZE_CATTR_TABLE 0
#endif

#endif /* _XY_CONFIG_H_ */
