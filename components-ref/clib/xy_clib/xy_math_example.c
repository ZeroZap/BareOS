/**
 * @file xy_math_example.c
 * @brief Example usage of xy_math library
 *
 * Demonstrates how to use xy_math functions on Cortex-M0 MCUs
 * without hardware division support.
 */

#include <stdint.h>
#include "xy_math.h"
#include "xy_stdio.h"

/**
 * @brief Example: Software Division
 */
void example_division(void)
{
    uint32_t quotient, remainder;
    int32_t signed_result;

    xy_stdio_printf("\n=== Software Division Example ===\n");

    /* Basic unsigned division */
    quotient = xy_udiv32(1000, 7);
    xy_stdio_printf("1000 / 7 = %u\n", quotient);

    /* Division with remainder */
    quotient = xy_udivmod32(1000, 7, &remainder);
    xy_stdio_printf("1000 / 7 = %u remainder %u\n", quotient, remainder);

    /* Signed division */
    signed_result = xy_sdiv32(-1000, 7);
    xy_stdio_printf("-1000 / 7 = %d\n", signed_result);

    /* 64-bit division */
    uint64_t large_num = 1000000000ULL;
    uint64_t result64 = xy_udiv64(large_num, 999);
    xy_stdio_printf("1000000000 / 999 = %llu\n", result64);
}

/**
 * @brief Example: Integer Square Root
 */
void example_sqrt(void)
{
    uint32_t num, root;

    xy_stdio_printf("\n=== Integer Square Root Example ===\n");

    num = 1024;
    root = xy_isqrt32(num);
    xy_stdio_printf("sqrt(%u) = %u\n", num, root);

    num = 10000;
    root = xy_isqrt32(num);
    xy_stdio_printf("sqrt(%u) = %u\n", num, root);

    /* For non-perfect squares, returns floor value */
    num = 1000;
    root = xy_isqrt32(num);
    xy_stdio_printf("sqrt(%u) ≈ %u (floor)\n", num, root);
}

/**
 * @brief Example: Integer Power
 */
void example_power(void)
{
    uint32_t result;

    xy_stdio_printf("\n=== Integer Power Example ===\n");

    result = xy_ipow(2, 10);
    xy_stdio_printf("2^10 = %u\n", result);

    result = xy_ipow(10, 6);
    xy_stdio_printf("10^6 = %u\n", result);

    result = xy_ipow(3, 8);
    xy_stdio_printf("3^8 = %u\n", result);
}

/**
 * @brief Example: GCD and LCM
 */
void example_gcd_lcm(void)
{
    uint32_t a, b, result;

    xy_stdio_printf("\n=== GCD and LCM Example ===\n");

    a = 48;
    b = 18;
    result = xy_gcd(a, b);
    xy_stdio_printf("GCD(%u, %u) = %u\n", a, b, result);

    result = xy_lcm(a, b);
    xy_stdio_printf("LCM(%u, %u) = %u\n", a, b, result);

    /* For coprime numbers */
    a = 17;
    b = 19;
    result = xy_gcd(a, b);
    xy_stdio_printf("GCD(%u, %u) = %u (coprime)\n", a, b, result);
}

/**
 * @brief Example: Bit Operations
 */
void example_bit_ops(void)
{
    uint32_t num, result;
    int count;

    xy_stdio_printf("\n=== Bit Operations Example ===\n");

    /* Check if power of 2 */
    num = 64;
    if (xy_is_power_of_2(num)) {
        xy_stdio_printf("%u is a power of 2\n", num);
    }

    /* Round up to next power of 2 */
    num = 100;
    result = xy_next_power_of_2(num);
    xy_stdio_printf("Next power of 2 after %u is %u\n", num, result);

    /* Count leading zeros */
    num = 0x00001234;
    count = xy_clz32(num);
    xy_stdio_printf("Leading zeros in 0x%08X: %d\n", num, count);

    /* Count set bits */
    num = 0xFF;
    count = xy_popcount32(num);
    xy_stdio_printf("Number of 1-bits in 0x%02X: %d\n", num, count);
}

/**
 * @brief Example: Fixed-Point Math (Q16.16)
 */
void example_fixed_point(void)
{
    xy_fixed_t a, b, result;

    xy_stdio_printf("\n=== Fixed-Point Math Example (Q16.16) ===\n");

    /* Convert integers to fixed-point */
    a = xy_int_to_fixed(5);  /* 5.0 */
    b = xy_int_to_fixed(3);  /* 3.0 */

    /* Fixed-point multiplication: 5.0 * 3.0 = 15.0 */
    result = xy_fixed_mul(a, b);
    xy_stdio_printf("5.0 * 3.0 = %d.%03u\n",
                    xy_fixed_to_int(result),
                    (uint32_t)(((result & 0xFFFF) * 1000) >> 16));

    /* Fixed-point division: 10.0 / 4.0 = 2.5 */
    a = xy_int_to_fixed(10);
    b = xy_int_to_fixed(4);
    result = xy_fixed_div(a, b);
    xy_stdio_printf("10.0 / 4.0 = %d.%03u\n",
                    xy_fixed_to_int(result),
                    (uint32_t)(((result & 0xFFFF) * 1000) >> 16));

    /* Fixed-point square root */
    a = xy_int_to_fixed(100);
    result = xy_fixed_sqrt(a);
    xy_stdio_printf("sqrt(100.0) = %d.%03u\n",
                    xy_fixed_to_int(result),
                    (uint32_t)(((result & 0xFFFF) * 1000) >> 16));
}

/**
 * @brief Example: Trigonometric Functions
 */
void example_trig(void)
{
    int16_t angle;
    int16_t sin_val, cos_val, tan_val;

    xy_stdio_printf("\n=== Trigonometric Functions Example ===\n");

    /* Calculate sin, cos, tan for various angles */
    angle = 0;
    sin_val = xy_sin_deg(angle);
    cos_val = xy_cos_deg(angle);
    xy_stdio_printf("sin(%d°) = %d, cos(%d°) = %d\n",
                    angle, sin_val, angle, cos_val);

    angle = 45;
    sin_val = xy_sin_deg(angle);
    cos_val = xy_cos_deg(angle);
    xy_stdio_printf("sin(%d°) = %d, cos(%d°) = %d\n",
                    angle, sin_val, angle, cos_val);

    angle = 90;
    sin_val = xy_sin_deg(angle);
    cos_val = xy_cos_deg(angle);
    xy_stdio_printf("sin(%d°) = %d, cos(%d°) = %d\n",
                    angle, sin_val, angle, cos_val);

    /* Note: Values are scaled by 32767 (Q0.15 format) */
    xy_stdio_printf("Note: Values are in Q0.15 format (scaled by 32767)\n");
}

/**
 * @brief Example: Utility Macros
 */
void example_macros(void)
{
    int a = 10, b = 20;
    int x = 15;

    xy_stdio_printf("\n=== Utility Macros Example ===\n");

    xy_stdio_printf("MIN(%d, %d) = %d\n", a, b, XY_MIN(a, b));
    xy_stdio_printf("MAX(%d, %d) = %d\n", a, b, XY_MAX(a, b));
    xy_stdio_printf("CLAMP(%d, 0, 10) = %d\n", x, XY_CLAMP(x, 0, 10));
    xy_stdio_printf("ABS(-42) = %d\n", XY_ABS(-42));
    xy_stdio_printf("SIGN(-10) = %d\n", XY_SIGN(-10));
    xy_stdio_printf("SIGN(10) = %d\n", XY_SIGN(10));
}

/**
 * @brief Main function
 */
int main(void)
{
    xy_stdio_printf("\n");
    xy_stdio_printf("========================================\n");
    xy_stdio_printf("  XY_MATH Library Usage Examples\n");
    xy_stdio_printf("  Optimized for Cortex-M0 MCUs\n");
    xy_stdio_printf("========================================\n");

    /* Run all examples */
    example_division();
    example_sqrt();
    example_power();
    example_gcd_lcm();
    example_bit_ops();
    example_fixed_point();
    example_trig();
    example_macros();

    xy_stdio_printf("\n");
    xy_stdio_printf("========================================\n");
    xy_stdio_printf("  All examples completed!\n");
    xy_stdio_printf("========================================\n");

    return 0;
}
