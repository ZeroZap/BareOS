#include "unity.h"
#include "xy_common.h"


static void test_xy_bcd2hex_basic(void)
{
    TEST_ASSERT_EQUAL_UINT32(0, xy_bcd2hex(0x0));
    TEST_ASSERT_EQUAL_UINT32(9, xy_bcd2hex(0x9));
    TEST_ASSERT_EQUAL_UINT32(99, xy_bcd2hex(0x99));
    TEST_ASSERT_EQUAL_UINT32(1234, xy_bcd2hex(0x1234));
    TEST_ASSERT_EQUAL_UINT32(9999, xy_bcd2hex(0x9999));
    TEST_ASSERT_EQUAL_UINT32(12345678, xy_bcd2hex(0x12345678));
}


static void test_xy_bcd2hex_edge_cases(void)
{
    TEST_ASSERT_EQUAL_UINT32(1, xy_bcd2hex(0x1));
    TEST_ASSERT_EQUAL_UINT32(10, xy_bcd2hex(0x10));
    TEST_ASSERT_EQUAL_UINT32(100, xy_bcd2hex(0x100));
    TEST_ASSERT_EQUAL_UINT32(1000, xy_bcd2hex(0x1000));
    TEST_ASSERT_EQUAL_UINT32(99999999, xy_bcd2hex(0x99999999));
}


int test_xy_bcd2hex(void)
{
    RUN_TEST(test_xy_bcd2hex_basic);
    RUN_TEST(test_xy_bcd2hex_edge_cases);
}
