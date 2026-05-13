#include "unity.h"
#include "xy_common.h"

void test_xy_set_bit(void)
{
    uint32_t value = 0;

    xy_set_bit(value, 3);
    TEST_ASSERT_EQUAL_HEX32(0x08, value);

    xy_set_bit(value, 0);
    TEST_ASSERT_EQUAL_HEX32(0x09, value);
}

// 测试 xy_set_bits 宏
void test_xy_set_bits(void)
{
    uint32_t value = 0;

    xy_set_bits(value, 4, 0x7);
    TEST_ASSERT_EQUAL_HEX32(0x70, value);

    xy_set_bits(value, 0, 0x3);
    TEST_ASSERT_EQUAL_HEX32(0x73, value);
}

void test_xy_clear_bit(void)
{
    uint32_t value = 0xFF;

    xy_clear_bit(value, 2);
    TEST_ASSERT_EQUAL_HEX32(0xFB, value);

    xy_clear_bit(value, 7);
    TEST_ASSERT_EQUAL_HEX32(0x7B, value);
}

void test_xy_clear_bits(void)
{
    uint32_t value = 0xFFFFFFFF;

    xy_clear_bits(value, 8, 0xFF);
    TEST_ASSERT_EQUAL_HEX32(0xFFFF00FF, value);

    xy_clear_bits(value, 0, 0xF);
    TEST_ASSERT_EQUAL_HEX32(0xFFFF00F0, value);
}

void test_xy_toggle_bit(void)
{
    uint32_t value = 0x55;

    xy_toggle_bit(value, 1);
    TEST_ASSERT_EQUAL_HEX32(0x57, value);

    xy_toggle_bit(value, 7);
    TEST_ASSERT_EQUAL_HEX32(0xD7, value);
}


void test_xy_toggle_bits(void)
{
    uint32_t value = 0x5555;

    xy_toggle_bits(value, 4, 0xF);
    TEST_ASSERT_EQUAL_HEX32(0x55A5, value);

    xy_toggle_bits(value, 0, 0x3);
    TEST_ASSERT_EQUAL_HEX32(0x55A6, value);
}

void test_xy_get_bit(void)
{
    uint32_t value = 0x0A;

    TEST_ASSERT_EQUAL(0, xy_get_bit(value, 0));
    TEST_ASSERT_NOT_EQUAL(0, xy_get_bit(value, 1));
    TEST_ASSERT_EQUAL(0, xy_get_bit(value, 2));
    TEST_ASSERT_NOT_EQUAL(0, xy_get_bit(value, 3));
}

// 测试 xy_get_bits 宏 - 修正版
void test_xy_get_bits(void)
{
    uint32_t value = 0x12345678;

    TEST_ASSERT_EQUAL_HEX32(0x7, (xy_get_bits(value, 4, 0xF) >> 4));
    TEST_ASSERT_EQUAL_HEX32(0x456, (xy_get_bits(value, 8, 0xFFF) >> 8));
}

int test_xy_bits(void)
{
    RUN_TEST(test_xy_set_bit);
    RUN_TEST(test_xy_set_bits);
    RUN_TEST(test_xy_clear_bit);
    RUN_TEST(test_xy_clear_bits);
    RUN_TEST(test_xy_toggle_bit);
    RUN_TEST(test_xy_toggle_bits);
    RUN_TEST(test_xy_get_bit);
    RUN_TEST(test_xy_get_bits);
}