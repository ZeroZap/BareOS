/**
 * @file xy_math.h
 * @brief XinYi Math Library - Mathematical Functions for Embedded Systems
 * @version 2.0
 * @date 2026-02-28
 */

#ifndef XY_MATH_H
#define XY_MATH_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Basic Math Functions */

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

/* Min/Max Functions */

/**
 * @brief Get minimum of two values
 * @param a First value
 * @param b Second value
 * @return Minimum value
 */
#define xy_min(a, b) (((a) < (b)) ? (a) : (b))

/**
 * @brief Get maximum of two values
 * @param a First value
 * @param b Second value
 * @return Maximum value
 */
#define xy_max(a, b) (((a) > (b)) ? (a) : (b))

/**
 * @brief Clamp value to range
 * @param x Value to clamp
 * @param low Lower bound
 * @param high Upper bound
 * @return Clamped value
 */
#define xy_clamp(x, low, high) \
    (((x) < (low)) ? (low) : (((x) > (high)) ? (high) : (x)))

/* Power and Root Functions */

/**
 * @brief Calculate integer power (base^exp)
 * @param base Base value
 * @param exp Exponent
 * @return Base raised to exponent
 */
int xy_pow(int base, unsigned int exp);

/**
 * @brief Calculate integer square root
 * @param x Input value
 * @return Square root of x
 */
int xy_sqrt(int x);

/**
 * @brief Calculate integer cube root
 * @param x Input value
 * @return Cube root of x
 */
int xy_cbrt(int x);

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

/* Random Number Generation */

/**
 * @brief Random number generator state
 */
typedef struct {
    uint32_t state;  /**< Current state */
} xy_rand_state_t;

/**
 * @brief Initialize random number generator
 * @param state Random number state
 * @param seed Seed value
 */
void xy_rand_init(xy_rand_state_t *state, uint32_t seed);

/**
 * @brief Generate random number
 * @param state Random number state
 * @return Random number (0-0x7FFFFFFF)
 */
int32_t xy_rand(xy_rand_state_t *state);

/**
 * @brief Generate random number in range [min, max]
 * @param state Random number state
 * @param min Minimum value
 * @param max Maximum value
 * @return Random number in range
 */
int32_t xy_rand_range(xy_rand_state_t *state, int32_t min, int32_t max);

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

#endif /* XY_MATH_H */
