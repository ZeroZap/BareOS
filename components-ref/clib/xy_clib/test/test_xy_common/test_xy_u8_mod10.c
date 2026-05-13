#include "unity.h"
#include "xy_common.h" // 假设函数声明在此头文件中

static void test_zero_input(void)
{
    TEST_ASSERT_EQUAL(0, xy_u8_mod10(0));
}

static void test_single_digit_numbers(void)
{
    TEST_ASSERT_EQUAL(3, xy_u8_mod10(3));
    TEST_ASSERT_EQUAL(9, xy_u8_mod10(9));
}

static void test_multiples_of_10(void)
{
    TEST_ASSERT_EQUAL(0, xy_u8_mod10(10));
    TEST_ASSERT_EQUAL(0, xy_u8_mod10(100));
    TEST_ASSERT_EQUAL(
        0, xy_u8_mod10(XY_U8_MAX - 5)); // 假设UINT32_MAX+1是10的倍数
}

static void test_random_cases(void)
{
    TEST_ASSERT_EQUAL(7, xy_u8_mod10(17));
    TEST_ASSERT_EQUAL(6, xy_u8_mod10(236));
    TEST_ASSERT_EQUAL(5, xy_u8_mod10(XY_U8_MAX));
}

int test_xy_u8_mod10(void)
{
    RUN_TEST(test_zero_input);
    RUN_TEST(test_single_digit_numbers);
    RUN_TEST(test_multiples_of_10);
    RUN_TEST(test_random_cases);
}