/**
 * @file xy_stdlib.h
 * @brief XinYi Standard Library - Unified header for embedded C library
 * @version 2.0
 * @date 2026-02-28
 */

#ifndef XY_STDLIB_H
#define XY_STDLIB_H

#ifdef __cplusplus
extern "C" {
#endif

/* Include core type definitions */
#include "xy_typedef.h"
#include "xy_config.h"

/* Include all xy_clib sub-headers */
#include "xy_string.h"
#include "xy_stdio.h"
#include "xy_ctype.h"
#include "xy_math.h"
#include "xy_assert.h"
#include "xy_error.h"
#include "xy_helper.h"
#include "xy_time.h"
#include "xy_stddef.h"
#include "xy_stdarg.h"

/* Memory Management Functions */

/**
 * @brief Allocate memory block
 * @param size Size of memory block in bytes
 * @return Pointer to allocated memory, or NULL on failure
 */
void *xy_malloc(size_t size);

/**
 * @brief Allocate and zero-initialize memory
 * @param nmemb Number of elements
 * @param size Size of each element
 * @return Pointer to allocated memory, or NULL on failure
 */
void *xy_calloc(size_t nmemb, size_t size);

/**
 * @brief Reallocate memory block
 * @param ptr Pointer to previously allocated memory
 * @param size New size in bytes
 * @return Pointer to reallocated memory, or NULL on failure
 */
void *xy_realloc(void *ptr, size_t size);

/**
 * @brief Free allocated memory
 * @param ptr Pointer to memory to free
 */
void xy_free(void *ptr);

/**
 * @brief Safely free allocated memory and set pointer to NULL
 * @param ptr Pointer to pointer to memory to free
 */
void xy_safe_free(void **ptr);

/* Additional utility macros */
#define xy_safe_delete(ptr)            \
    do {                               \
        xy_safe_free((void **)&(ptr)); \
    } while (0)

/* String Conversion Functions */

/**
 * @brief Convert string to double
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

/**
 * @brief Convert string to double (with end pointer)
 * @param str Input string
 * @param endptr Pointer to end of parsed string
 * @return Converted value
 */
double xy_strtod(const char *str, char **endptr);

/**
 * @brief Convert string to float (with end pointer)
 * @param str Input string
 * @param endptr Pointer to end of parsed string
 * @return Converted value
 */
float xy_strtof(const char *str, char **endptr);

/**
 * @brief Convert string to long (with end pointer and base)
 * @param str Input string
 * @param endptr Pointer to end of parsed string
 * @param base Number base (2-36)
 * @return Converted value
 */
long xy_strtol(const char *str, char **endptr, int base);

/**
 * @brief Convert string to unsigned long (with end pointer and base)
 * @param str Input string
 * @param endptr Pointer to end of parsed string
 * @param base Number base (2-36)
 * @return Converted value
 */
unsigned long xy_strtoul(const char *str, char **endptr, int base);

/**
 * @brief Convert string to long long (with end pointer and base)
 * @param str Input string
 * @param endptr Pointer to end of parsed string
 * @param base Number base (2-36)
 * @return Converted value
 */
long long xy_strtoll(const char *str, char **endptr, int base);

/**
 * @brief Convert string to unsigned long long (with end pointer and base)
 * @param str Input string
 * @param endptr Pointer to end of parsed string
 * @param base Number base (2-36)
 * @return Converted value
 */
unsigned long long xy_strtoull(const char *str, char **endptr, int base);

/* Number to String Conversion */

/**
 * @brief Convert integer to string
 * @param value Value to convert
 * @param str Output string buffer
 * @param base Number base (2-36)
 * @return Pointer to output string
 */
char *xy_itoa(int value, char *str, int base);

/**
 * @brief Convert long to string
 * @param value Value to convert
 * @param str Output string buffer
 * @param base Number base (2-36)
 * @return Pointer to output string
 */
char *xy_ltoa(long value, char *str, int base);

/**
 * @brief Convert unsigned long to string
 * @param value Value to convert
 * @param str Output string buffer
 * @param base Number base (2-36)
 * @return Pointer to output string
 */
char *xy_ultoa(unsigned long value, char *str, int base);

/**
 * @brief Convert unsigned integer to string
 * @param value Value to convert
 * @param str Output string buffer
 * @param base Number base (2-36)
 * @return Pointer to output string
 */
char *xy_utoa(unsigned int value, char *str, int base);

/* Sorting and Searching */

/**
 * @brief Sort array using quicksort
 * @param base Array to sort
 * @param num Number of elements
 * @param size Size of each element
 * @param compar Comparison function
 */
void xy_qsort(void *base, size_t num, size_t size,
              int (*compar)(const void *, const void *));

/**
 * @brief Binary search in sorted array
 * @param key Key to search for
 * @param base Sorted array
 * @param num Number of elements
 * @param size Size of each element
 * @param compar Comparison function
 * @return Pointer to found element, or NULL if not found
 */
void *xy_bsearch(const void *key, const void *base, size_t num, size_t size,
                 int (*compar)(const void *, const void *));

/* Math Functions */

/**
 * @brief Absolute value of integer
 * @param x Input value
 * @return Absolute value
 */
int xy_abs(int x);

/**
 * @brief Absolute value of long integer
 * @param x Input value
 * @return Absolute value
 */
long xy_labs(long x);

/**
 * @brief Absolute value of long long integer
 * @param x Input value
 * @return Absolute value
 */
long long xy_llabs(long long x);

#ifndef XY_DIV_T_DEFINED
#define XY_DIV_T_DEFINED
/**
 * @brief Divide integers and return quotient and remainder
 * @param numer Numerator
 * @param denom Denominator
 * @return xy_div_t structure with quotient and remainder
 */
typedef struct {
    int quot;  /**< Quotient */
    int rem;   /**< Remainder */
} xy_div_t;

xy_div_t xy_div(int numer, int denom);
#endif

#ifndef XY_LDIV_T_DEFINED
#define XY_LDIV_T_DEFINED
/**
 * @brief Divide long integers and return quotient and remainder
 * @param numer Numerator
 * @param denom Denominator
 * @return xy_ldiv_t structure with quotient and remainder
 */
typedef struct {
    long quot;  /**< Quotient */
    long rem;   /**< Remainder */
} xy_ldiv_t;

xy_ldiv_t xy_ldiv(long numer, long denom);
#endif

#ifndef XY_LLDIV_T_DEFINED
#define XY_LLDIV_T_DEFINED
/**
 * @brief Divide long long integers and return quotient and remainder
 * @param numer Numerator
 * @param denom Denominator
 * @return xy_lldiv_t structure with quotient and remainder
 */
typedef struct {
    long long quot;  /**< Quotient */
    long long rem;   /**< Remainder */
} xy_lldiv_t;

xy_lldiv_t xy_lldiv(long long numer, long long denom);
#endif

/* Random Number Generation */

/**
 * @brief Maximum random number value
 */
#define XY_RAND_MAX 32767



/**
 * @brief Seed random number generator
 * @param seed Seed value
 */
void xy_srand(unsigned int seed);

/* Environment Functions */

/**
 * @brief Abort program execution
 */
void xy_abort(void);

/**
 * @brief Exit program with status
 * @param status Exit status
 */
void xy_exit(int status);

/**
 * @brief Get environment variable
 * @param name Environment variable name
 * @return Value of environment variable, or NULL if not found
 */
char *xy_getenv(const char *name);

/**
 * @brief Execute system command
 * @param command Command to execute
 * @return Command exit status
 */
int xy_system(const char *command);

/* Utility Functions */

/**
 * @brief Convert integer to ASCII and append to string
 * @param value Value to convert
 * @param str String to append to
 * @return Pointer to string
 */
char *xy_itoa_append(int value, char *str);



/**
 * @brief Calculate square root
 * @param x Input value
 * @return Square root of x
 */
int xy_sqrt(int x);

/**
 * @brief Calculate cube root
 * @param x Input value
 * @return Cube root of x
 */
int xy_cbrt(int x);



/**
 * @brief Map value from one range to another
 * @param x Value to map
 * @param in_min Input range minimum
 * @param in_max Input range maximum
 * @param out_min Output range minimum
 * @param out_max Output range maximum
 * @return Mapped value
 */
int xy_map(int x, int in_min, int in_max, int out_min, int out_max);

/**
 * @brief Constrain value to range
 * @param x Value to constrain
 * @param min Minimum value
 * @param max Maximum value
 * @return Constrained value
 */
int xy_constrain(int x, int min, int max);

/**
 * @brief Calculate greatest common divisor
 * @param a First number
 * @param b Second number
 * @return GCD of a and b
 */
int xy_gcd(int a, int b);

/**
 * @brief Calculate least common multiple
 * @param a First number
 * @param b Second number
 * @return LCM of a and b
 */
int xy_lcm(int a, int b);

/**
 * @brief Calculate factorial
 * @param n Input value
 * @return Factorial of n
 */
int xy_factorial(int n);

/* Fixed-point Math */

/**
 * @brief Multiply two fixed-point numbers (Q16.16 format)
 * @param a First number
 * @param b Second number
 * @return Product
 */
int32_t xy_fixed_mul(int32_t a, int32_t b);

/**
 * @brief Divide two fixed-point numbers (Q16.16 format)
 * @param a Dividend
 * @param b Divisor
 * @return Quotient
 */
int32_t xy_fixed_div(int32_t a, int32_t b);

/* Trigonometric Functions (Fixed-point) */

/**
 * @brief Sine function (fixed-point, angle in degrees * 100)
 * @param angle Angle in degrees * 100
 * @return Sine value * 10000
 */
int32_t xy_sin_fixed(int32_t angle);

/**
 * @brief Cosine function (fixed-point, angle in degrees * 100)
 * @param angle Angle in degrees * 100
 * @return Cosine value * 10000
 */
int32_t xy_cos_fixed(int32_t angle);

/**
 * @brief Tangent function (fixed-point, angle in degrees * 100)
 * @param angle Angle in degrees * 100
 * @return Tangent value * 10000
 */
int32_t xy_tan_fixed(int32_t angle);

/* Bit Manipulation */

/**
 * @brief Count number of set bits in a value
 * @param value Input value
 * @return Number of set bits
 */
int xy_popcount(uint32_t value);

/**
 * @brief Count leading zeros
 * @param value Input value
 * @return Number of leading zeros
 */
int xy_clz(uint32_t value);

/**
 * @brief Count trailing zeros
 * @param value Input value
 * @return Number of trailing zeros
 */
int xy_ctz(uint32_t value);

/**
 * @brief Round up to next power of 2
 * @param value Input value
 * @return Next power of 2
 */
uint32_t xy_round_up_pow2(uint32_t value);

/**
 * @brief Check if value is power of 2
 * @param value Input value
 * @return 1 if power of 2, 0 otherwise
 */
int xy_is_pow2(uint32_t value);



/* Floating Point Approximation (when hardware FPU not available) */

/**
 * @brief Fast inverse square root (Quake algorithm)
 * @param x Input value
 * @return 1/sqrt(x)
 */
float xy_fast_inv_sqrt(float x);

/**
 * @brief Fast sine approximation
 * @param x Angle in radians
 * @return Sine value
 */
float xy_fast_sin(float x);

/**
 * @brief Fast cosine approximation
 * @param x Angle in radians
 * @return Cosine value
 */
float xy_fast_cos(float x);

#ifdef __cplusplus
}
#endif

#endif /* XY_STDLIB_H */
