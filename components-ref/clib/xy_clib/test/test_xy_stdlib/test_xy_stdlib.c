#include "unity.h"
#include "xy_stdlib.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <inttypes.h> // 添加此头文件

#include <inttypes.h>

#define TEST_ASSERT_EQUAL_LONG(expected, actual)       \
    if ((expected) != (actual)) {                      \
        UnityFail("Long comparison failed", __LINE__); \
    }

// Test cases for xy_atof
void test_xy_atof_basic(void)
{
    TEST_ASSERT_EQUAL_DOUBLE(123.456, xy_atof("123.456"));
    TEST_ASSERT_EQUAL_DOUBLE(-123.456, xy_atof("-123.456"));
    TEST_ASSERT_EQUAL_DOUBLE(0.0, xy_atof("0"));
    TEST_ASSERT_EQUAL_DOUBLE(1.0, xy_atof("1"));
    TEST_ASSERT_EQUAL_DOUBLE(0.5, xy_atof("0.5"));
}

void test_xy_atof_scientific(void)
{
    TEST_ASSERT_EQUAL_DOUBLE(1230.0, xy_atof("1.23e3"));
    TEST_ASSERT_EQUAL_DOUBLE(0.00123, xy_atof("1.23e-3"));
    TEST_ASSERT_EQUAL_DOUBLE(1230000.0, xy_atof("1.23E6"));
}

void test_xy_atof_whitespace(void)
{
    TEST_ASSERT_EQUAL_DOUBLE(123.456, xy_atof("   123.456"));
    TEST_ASSERT_EQUAL_DOUBLE(123.456, xy_atof("123.456   "));
    TEST_ASSERT_EQUAL_DOUBLE(123.456, xy_atof("   123.456   "));
}

// Test cases for xy_atoi
void test_xy_atoi_basic(void)
{
    TEST_ASSERT_EQUAL_INT(123, xy_atoi("123"));
    TEST_ASSERT_EQUAL_INT(-123, xy_atoi("-123"));
    TEST_ASSERT_EQUAL_INT(0, xy_atoi("0"));
    TEST_ASSERT_EQUAL_INT(1, xy_atoi("1"));
}

void test_xy_atoi_whitespace(void)
{
    TEST_ASSERT_EQUAL_INT(123, xy_atoi("   123"));
    TEST_ASSERT_EQUAL_INT(123, xy_atoi("123   "));
    TEST_ASSERT_EQUAL_INT(123, xy_atoi("   123   "));
}

void test_xy_atoi_overflow(void)
{
    TEST_ASSERT_EQUAL_INT(INT_MAX, xy_atoi("2147483648"));  // INT_MAX + 1
    TEST_ASSERT_EQUAL_INT(INT_MIN, xy_atoi("-2147483649")); // INT_MIN - 1
}

// Test cases for xy_atol
void test_xy_atol_basic(void)
{
    TEST_ASSERT_EQUAL_LONG(123L, xy_atol("123"));
    TEST_ASSERT_EQUAL_LONG(-123L, xy_atol("-123"));
    TEST_ASSERT_EQUAL_LONG(0L, xy_atol("0"));
    TEST_ASSERT_EQUAL_LONG(1L, xy_atol("1"));
}

void test_xy_atol_boundaries(void)
{
    // 正数边界
    TEST_ASSERT_EQUAL_LONG(2147483647L, xy_atol("2147483647")); // INT_MAX
    TEST_ASSERT_EQUAL_LONG(LONG_MAX, xy_atol("9223372036854775807"));

    // 负数边界
    TEST_ASSERT_EQUAL_LONG(-2147483648L, xy_atol("-2147483648")); // INT_MIN
    TEST_ASSERT_EQUAL_LONG(-2147483648L, xy_atol("-2147483649")); // 应正确转换
    TEST_ASSERT_EQUAL_LONG(LONG_MIN, xy_atol("-9223372036854775808"));

    // 溢出测试
    TEST_ASSERT_EQUAL_LONG(
        LONG_MAX, xy_atol("9223372036854775808")); // 超过 LONG_MAX
    TEST_ASSERT_EQUAL_LONG(
        LONG_MIN, xy_atol("-9223372036854775809")); // 超过 LONG_MIN
}

// Test cases for xy_strtod
void test_xy_strtod_basic(void)
{
    char *endptr;
    TEST_ASSERT_EQUAL_DOUBLE(123.456, xy_strtod("123.456", &endptr));
    TEST_ASSERT_EQUAL_STRING("", endptr);
    TEST_ASSERT_EQUAL_DOUBLE(-123.456, xy_strtod("-123.456", &endptr));
    TEST_ASSERT_EQUAL_STRING("", endptr);
}

void test_xy_strtod_trailing(void)
{
    char *endptr;
    TEST_ASSERT_EQUAL_DOUBLE(123.456, xy_strtod("123.456abc", &endptr));
    TEST_ASSERT_EQUAL_STRING("abc", endptr);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, xy_strtod("abc123.456", &endptr));
    TEST_ASSERT_EQUAL_STRING("abc123.456", endptr);
}

// Test cases for xy_strtol
void test_xy_strtol_basic(void)
{
    char *endptr;
    TEST_ASSERT_EQUAL_LONG(123L, xy_strtol("123", &endptr));
    TEST_ASSERT_EQUAL_STRING("", endptr);
    TEST_ASSERT_EQUAL_LONG(-123L, xy_strtol("-123", &endptr));
    TEST_ASSERT_EQUAL_STRING("", endptr);
}

void test_xy_strtol_base_detection(void)
{
    char *endptr;
    // 0 prefix means octal
    TEST_ASSERT_EQUAL_LONG(8L, xy_strtol("010", &endptr));
    TEST_ASSERT_EQUAL_STRING("", endptr);

    // 0x prefix means hex
    TEST_ASSERT_EQUAL_LONG(16L, xy_strtol("0x10", &endptr));
    TEST_ASSERT_EQUAL_STRING("", endptr);

    // Hex without prefix
    TEST_ASSERT_EQUAL_LONG(0L, xy_strtol("FF", &endptr));
    TEST_ASSERT_EQUAL_STRING("FF", endptr);
}

// Test cases for xy_qsort and xy_bsearch
int compare_ints(const void *a, const void *b)
{
    int arg1 = *(const int *)a;
    int arg2 = *(const int *)b;
    return (arg1 > arg2) - (arg1 < arg2);
}

void test_xy_qsort_bsearch(void)
{
    int arr[]    = { 9, 3, 7, 5, 6, 4, 8, 2, 1, 0 };
    int sorted[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    int key      = 5;

    xy_qsort(arr, 10, sizeof(int), compare_ints);
    for (int i = 0; i < 10; i++) {
        TEST_ASSERT_EQUAL_INT(sorted[i], arr[i]);
    }

    int *found = xy_bsearch(&key, arr, 10, sizeof(int), compare_ints);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_INT(key, *found);

    key   = 11;
    found = xy_bsearch(&key, arr, 10, sizeof(int), compare_ints);
    TEST_ASSERT_NULL(found);
}

// Test cases for xy_abs
void test_xy_abs(void)
{
    TEST_ASSERT_EQUAL_INT(0, xy_abs(0));
    TEST_ASSERT_EQUAL_INT(1, xy_abs(1));
    TEST_ASSERT_EQUAL_INT(1, xy_abs(-1));
    TEST_ASSERT_EQUAL_INT(INT_MAX, xy_abs(INT_MAX));
    TEST_ASSERT_EQUAL_INT(INT_MAX, xy_abs(-INT_MAX));
}

int test_xy_stdlib(void)
{


    RUN_TEST(test_xy_atof_basic);
    RUN_TEST(test_xy_atof_scientific);
    RUN_TEST(test_xy_atof_whitespace);

    RUN_TEST(test_xy_atoi_basic);
    RUN_TEST(test_xy_atoi_whitespace);
    RUN_TEST(test_xy_atoi_overflow);

    RUN_TEST(test_xy_atol_basic);
    RUN_TEST(test_xy_atol_boundaries);

    RUN_TEST(test_xy_strtod_basic);
    RUN_TEST(test_xy_strtod_trailing);

    RUN_TEST(test_xy_strtol_basic);
    RUN_TEST(test_xy_strtol_base_detection);

    RUN_TEST(test_xy_qsort_bsearch);

    RUN_TEST(test_xy_abs);
}