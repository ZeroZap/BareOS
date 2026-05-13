#include "unity.h"
#include "xy_common.h" // 假设函数声明在此头文件中


static void test_xy_hex2bcd_basic(void)
{
    TEST_ASSERT_EQUAL_HEX32(0x0, xy_hex2bcd(0x0));
    TEST_ASSERT_EQUAL_HEX32(0x9, xy_hex2bcd(0x9));
    TEST_ASSERT_EQUAL_HEX32(0x99, xy_hex2bcd(0x99));
    TEST_ASSERT_EQUAL_HEX32(0x1234, xy_hex2bcd(0x1234));
    TEST_ASSERT_EQUAL_HEX32(0x9999, xy_hex2bcd(0x9999));
    TEST_ASSERT_EQUAL_HEX32(0x12345678, xy_hex2bcd(0x12345678));
}


static void test_xy_hex2bcd_edge_cases(void)
{
    TEST_ASSERT_EQUAL_HEX32(0x1, xy_hex2bcd(0x1));
    TEST_ASSERT_EQUAL_HEX32(0x10, xy_hex2bcd(0x10));
    TEST_ASSERT_EQUAL_HEX32(0x100, xy_hex2bcd(0x100));
    TEST_ASSERT_EQUAL_HEX32(0x1000, xy_hex2bcd(0x1000));
    TEST_ASSERT_EQUAL_HEX32(0x99999999, xy_hex2bcd(0x99999999));
}

static void test_xy_hex2bcd_invalid_input(void)
{
    TEST_ASSERT_EQUAL_HEX32(0x0, xy_hex2bcd(0xA));
    TEST_ASSERT_EQUAL_HEX32(0x0, xy_hex2bcd(0x1A));
    TEST_ASSERT_EQUAL_HEX32(0x0, xy_hex2bcd(0x10000A));
}


int test_xy_hex2bcd(void)
{
    RUN_TEST(test_xy_hex2bcd_basic);
    RUN_TEST(test_xy_hex2bcd_edge_cases);
    RUN_TEST(test_xy_hex2bcd_invalid_input);
}
