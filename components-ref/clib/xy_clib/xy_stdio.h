/**
 * @file xy_stdio.h
 * @brief XinYi Standard I/O Library
 * @version 2.0
 * @date 2026-02-28
 */

#ifndef XY_STDIO_H
#define XY_STDIO_H

/* Optional HAL include - only include if available */
#ifdef XY_HAL_H
#include XY_HAL_H
#endif

#include <stdarg.h>
#include <stdint.h>

/**
 * @brief Printf buffer size
 */
#ifndef XY_PRINTF_BUFSIZE
#define XY_PRINTF_BUFSIZE 1024
#endif

/**
 * @brief Print function type - accepts a string
 */
typedef void (*xy_print_char_t)(char *str);

/**
 * @brief Get input function pointer
 */
typedef int (*xy_get_input_t)(void);

/**
 * @brief Initialize printf function
 * @param print_char Function to print character
 */
void xy_stdio_printf_init(xy_print_char_t print_char);

/**
 * @brief Initialize scanf function
 * @param get_input Function to get input
 */
void xy_stdio_scanf_init(xy_get_input_t get_input);

/**
 * @brief Formatted print to string with variable arguments
 * @param buf Output buffer
 * @param fmt Format string
 * @param args Variable arguments
 * @return Number of characters printed, negative on error
 */
int32_t xy_vsprintf(char *buf, const char *fmt, va_list args);

/**
 * @brief Formatted print to string
 * @param buf Output buffer
 * @param fmt Format string
 * @param ... Arguments
 * @return Number of characters printed, negative on error
 */
int32_t xy_sprintf(char *buf, const char *fmt, ...);

/**
 * @brief Formatted print to string with size limit
 * @param buf Output buffer
 * @param size Buffer size limit
 * @param fmt Format string
 * @param ... Arguments
 * @return Number of characters printed, negative on error
 */
int32_t xy_snprintf(char *buf, uint32_t size, const char *fmt, ...);

/**
 * @brief Formatted print to string with variable arguments (internal)
 * @param buf Output buffer
 * @param fmt Format string
 * @param args Variable arguments
 * @return Number of characters printed, negative on error
 */
int32_t xy_vsprintf(char *buf, const char *fmt, va_list args);

/**
 * @brief Formatted print to output device
 * @param fmt Format string
 * @param ... Arguments
 * @return Number of characters printed, negative on error
 */
int32_t xy_printf(const char *fmt, ...);

/**
 * @brief Formatted print with variable arguments to output device
 * @param fmt Format string
 * @param args Variable arguments
 * @return Number of characters printed, negative on error
 */
int32_t xy_vprintf(const char *fmt, va_list args);

/**
 * @brief Scan formatted input from input device
 * @param fmt Format string
 * @param ... Arguments
 * @return Number of items scanned, negative on error
 */
int32_t xy_scanf(const char *fmt, ...);

/**
 * @brief Scan formatted input from string
 * @param str Input string
 * @param fmt Format string
 * @param ... Arguments
 * @return Number of items scanned, negative on error
 */
int32_t xy_sscanf(const char *str, const char *fmt, ...);

/**
 * @brief Scan formatted input with variable arguments
 * @param fmt Format string
 * @param args Variable arguments
 * @return Number of items scanned, negative on error
 */
int32_t xy_vscanf(const char *fmt, va_list args);

/**
 * @brief Scan formatted input from string with variable arguments
 * @param str Input string
 * @param fmt Format string
 * @param args Variable arguments
 * @return Number of items scanned, negative on error
 */
int32_t xy_vsscanf(const char *str, const char *fmt, va_list args);

/**
 * @brief Convert string to unsigned long integer
 * @param str Input string
 * @param endptr Pointer to end of parsed string
 * @param base Number base (2-36)
 * @return Converted value
 */
unsigned long xy_strtoul(const char *str, char **endptr, int base);

/**
 * @brief Convert string to long integer
 * @param str Input string
 * @param endptr Pointer to end of parsed string
 * @param base Number base (2-36)
 * @return Converted value
 */
long xy_strtol(const char *str, char **endptr, int base);

/**
 * @brief Convert string to double
 * @param str Input string
 * @param endptr Pointer to end of parsed string
 * @return Converted value
 */
double xy_strtod(const char *str, char **endptr);

/**
 * @brief Convert string to float
 * @param str Input string
 * @param endptr Pointer to end of parsed string
 * @return Converted value
 */
float xy_strtof(const char *str, char **endptr);

/**
 * @brief Convert string to double (simple)
 * @param str Input string
 * @return Converted value
 */
double xy_atof(const char *str);

/**
 * @brief Convert string to integer
 * @param str Input string
 * @return Converted value
 */
int xy_atoi(const char *str);

/**
 * @brief Convert string to long
 * @param str Input string
 * @return Converted value
 */
long xy_atol(const char *str);

/**
 * @brief Convert string to long long
 * @param str Input string
 * @return Converted value
 */
long long xy_atoll(const char *str);

#endif /* XY_STDIO_H */
