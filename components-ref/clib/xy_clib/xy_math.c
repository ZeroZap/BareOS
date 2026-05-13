/**
 * @file xy_math.c
 * @brief XinYi Math Library Implementation
 * @version 2.0
 * @date 2026-02-28
 */

#include "xy_math.h"
#include <stdint.h>

/* Basic Math Functions */

int xy_abs(int x)
{
    return (x < 0) ? -x : x;
}

long xy_labs(long x)
{
    return (x < 0) ? -x : x;
}

long long xy_llabs(long long x)
{
    return (x < 0) ? -x : x;
}

xy_div_t xy_div(int numer, int denom)
{
    xy_div_t result;
    result.quot = numer / denom;
    result.rem = numer % denom;
    return result;
}

xy_ldiv_t xy_ldiv(long numer, long denom)
{
    xy_ldiv_t result;
    result.quot = numer / denom;
    result.rem = numer % denom;
    return result;
}

xy_lldiv_t xy_lldiv(long long numer, long long denom)
{
    xy_lldiv_t result;
    result.quot = numer / denom;
    result.rem = numer % denom;
    return result;
}

/* Power and Root Functions */

int xy_pow(int base, unsigned int exp)
{
    int result = 1;
    while (exp > 0) {
        if (exp & 1) {
            result *= base;
        }
        base *= base;
        exp >>= 1;
    }
    return result;
}

int xy_sqrt(int x)
{
    if (x <= 1) return x;
    
    int low = 0, high = x, result = 0;
    
    while (low <= high) {
        int mid = low + (high - low) / 2;
        long long sq = (long long)mid * mid;
        
        if (sq == x) {
            return mid;
        } else if (sq < x) {
            low = mid + 1;
            result = mid;
        } else {
            high = mid - 1;
        }
    }
    
    return result;
}

int xy_cbrt(int x)
{
    if (x == 0) return 0;
    
    int sign = 1;
    if (x < 0) {
        sign = -1;
        x = -x;
    }
    
    int low = 0, high = x;
    
    while (low <= high) {
        int mid = low + (high - low) / 2;
        long long cube = (long long)mid * mid * mid;
        
        if (cube == x) {
            return mid * sign;
        } else if (cube < x) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }
    
    return (high * sign);
}

/* Fixed-point Trigonometric Functions (using lookup tables) */

static const int32_t sin_table[91] = {
    0, 174, 348, 523, 697, 871, 1045, 1218, 1391, 1564, 
    1736, 1908, 2079, 2249, 2419, 2588, 2756, 2923, 3090, 3255, 
    3420, 3583, 3746, 3907, 4067, 4226, 4383, 4539, 4694, 4848, 
    5000, 5150, 5299, 5446, 5591, 5735, 5877, 6018, 6156, 6293, 
    6427, 6560, 6691, 6819, 6946, 7071, 7193, 7313, 7431, 7547, 
    7660, 7771, 7880, 7986, 8090, 8191, 8290, 8386, 8480, 8571, 
    8660, 8746, 8829, 8910, 8987, 9063, 9135, 9205, 9271, 9335, 
    9396, 9455, 9510, 9563, 9612, 9659, 9702, 9743, 9781, 9816, 
    9848, 9876, 9902, 9925, 9945, 9961, 9975, 9986, 9993, 9998, 
    10000
};

int32_t xy_sin_fixed(int32_t angle_degrees_x100)
{
    // Normalize angle to 0-36000 range
    int32_t normalized = angle_degrees_x100 % 36000;
    if (normalized < 0) normalized += 36000;
    
    // Convert to degrees
    int32_t degrees = normalized / 100;
    int32_t fraction = (normalized % 100) * 100; // 0-10000
    
    // Determine quadrant
    int32_t quadrant = degrees / 90;
    int32_t angle_in_quadrant = degrees % 90;
    
    int32_t result;
    if (quadrant == 0) {
        // 0-89 degrees
        if (angle_in_quadrant >= 90) angle_in_quadrant = 89;
        result = sin_table[angle_in_quadrant];
    } else if (quadrant == 1) {
        // 90-179 degrees: sin(180-x) = sin(x)
        result = sin_table[89 - angle_in_quadrant];
    } else if (quadrant == 2) {
        // 180-269 degrees: sin(180+x) = -sin(x)
        result = -sin_table[angle_in_quadrant];
    } else {
        // 270-359 degrees: sin(360-x) = -sin(x)
        result = -sin_table[89 - angle_in_quadrant];
    }
    
    return result;
}

int32_t xy_cos_fixed(int32_t angle_degrees_x100)
{
    // cos(x) = sin(90 - x)
    return xy_sin_fixed((9000 - angle_degrees_x100) % 36000);
}

int32_t xy_tan_fixed(int32_t angle_degrees_x100)
{
    int32_t sin_val = xy_sin_fixed(angle_degrees_x100);
    int32_t cos_val = xy_cos_fixed(angle_degrees_x100);
    
    if (cos_val == 0) {
        // Return large value for undefined tangent
        return (sin_val >= 0) ? 1000000 : -1000000;
    }
    
    // Perform fixed-point division: (sin * 10000) / cos
    return (sin_val * 10000) / cos_val;
}

/* Bit Manipulation */

int xy_popcount(uint32_t value)
{
    int count = 0;
    while (value) {
        count += value & 1;
        value >>= 1;
    }
    return count;
}

int xy_clz(uint32_t value)
{
    if (value == 0) return 32;
    
    int count = 0;
    while ((value & 0x80000000) == 0) {
        count++;
        value <<= 1;
    }
    return count;
}

int xy_ctz(uint32_t value)
{
    if (value == 0) return 32;
    
    int count = 0;
    while ((value & 1) == 0) {
        count++;
        value >>= 1;
    }
    return count;
}

uint32_t xy_round_up_pow2(uint32_t value)
{
    if (value == 0) return 1;
    if (value == 1) return 1;
    
    value--;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value++;
    
    return value;
}

int xy_is_pow2(uint32_t value)
{
    return (value != 0) && ((value & (value - 1)) == 0);
}

/* Fixed-point Math */

int32_t xy_fixed_mul(int32_t a, int32_t b)
{
    // For Q16.16, multiply and shift right by 16
    return ((int64_t)a * b) >> 16;
}

int32_t xy_fixed_div(int32_t a, int32_t b)
{
    // For Q16.16, shift left by 16 and divide
    if (b == 0) return 0; // Avoid division by zero
    return ((int64_t)a << 16) / b;
}

/* Random Number Generation */

void xy_rand_init(xy_rand_state_t *state, uint32_t seed)
{
    if (state) {
        state->state = seed ? seed : 1;
    }
}

int32_t xy_rand(xy_rand_state_t *state)
{
    if (!state) return 0;
    
    // Linear Congruential Generator
    state->state = state->state * 1103515245 + 12345;
    return (int32_t)(state->state >> 16) & 0x7FFF;
}

int32_t xy_rand_range(xy_rand_state_t *state, int32_t min, int32_t max)
{
    if (!state || min > max) return 0;
    
    if (min == max) return min;
    
    int32_t range = max - min + 1;
    int32_t rand_val = xy_rand(state);
    
    return min + (rand_val % range);
}

/* Floating Point Approximation */

float xy_fast_inv_sqrt(float x)
{
    // Quake's fast inverse square root algorithm
    union { float f; uint32_t i; } u;
    u.f = x;
    u.i = 0x5F3759DF - (u.i >> 1);
    return u.f * (1.5f - 0.5f * x * u.f * u.f);
}

float xy_fast_sin(float x)
{
    // Fast sine approximation using Taylor series
    // Reduce to [-π, π] range
    while (x > 3.14159265f) x -= 6.28318531f;
    while (x < -3.14159265f) x += 6.28318531f;
    
    // For small angles, use x - x^3/6
    float x2 = x * x;
    return x * (1.0f - x2 * (1.0f/6.0f - x2 * (1.0f/120.0f)));
}

float xy_fast_cos(float x)
{
    // cos(x) = sin(x + π/2)
    return xy_fast_sin(x + 1.57079632f);
}

/* Additional utility functions */

/**
 * @brief Calculate greatest common divisor
 * @param a First number
 * @param b Second number
 * @return GCD of a and b
 */
int32_t xy_gcd(int32_t a, int32_t b)
{
    while (b != 0) {
        int32_t temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

/**
 * @brief Calculate least common multiple
 * @param a First number
 * @param b Second number
 * @return LCM of a and b
 */
int32_t xy_lcm(int32_t a, int32_t b)
{
    if (a == 0 || b == 0) return 0;
    return (a / xy_gcd(a, b)) * b;
}

/**
 * @brief Constrain value to range
 * @param x Value to constrain
 * @param min Minimum value
 * @param max Maximum value
 * @return Constrained value
 */
int32_t xy_constrain(int32_t x, int32_t min, int32_t max)
{
    if (x < min) return min;
    if (x > max) return max;
    return x;
}

/**
 * @brief Map value from one range to another
 * @param x Value to map
 * @param in_min Input range minimum
 * @param in_max Input range maximum
 * @param out_min Output range minimum
 * @param out_max Output range maximum
 * @return Mapped value
 */
int32_t xy_map(int32_t x, int32_t in_min, int32_t in_max, 
               int32_t out_min, int32_t out_max)
{
    if (in_min == in_max) return out_min;
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

/**
 * @brief Calculate factorial
 * @param n Input value
 * @return Factorial of n
 */
int xy_factorial(int n)
{
    if (n < 0) return 0;
    if (n <= 1) return 1;
    
    int result = 1;
    for (int i = 2; i <= n; i++) {
        result *= i;
    }
    return result;
}
