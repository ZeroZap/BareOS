#include "unity.h"
#include "xy_common.h"


static void test_xy_bcd2dec_basic(void)
{
    TEST_ASSERT_EQUAL(0, xy_bcd2dec(0x00));
    TEST_ASSERT_EQUAL(5, xy_bcd2dec(0x05));
    TEST_ASSERT_EQUAL(9, xy_bcd2dec(0x09));
    TEST_ASSERT_EQUAL(99, xy_bcd2dec(0x99));
    TEST_ASSERT_EQUAL(1234, xy_bcd2dec(0x1234));
    TEST_ASSERT_EQUAL(12345678, xy_bcd2dec(0x12345678));
}


static void test_xy_bcd2dec_edge_cases(void)
{
    TEST_ASSERT_EQUAL(1, xy_bcd2dec(0x1));
    TEST_ASSERT_EQUAL(10, xy_bcd2dec(0x10));
    TEST_ASSERT_EQUAL(1000, xy_bcd2dec(0x1000));
    TEST_ASSERT_EQUAL(99999999, xy_bcd2dec(0x99999999));
}

static void test_xy_bcd2dec_invalid_input(void)
{
    TEST_ASSERT_EQUAL(0, xy_bcd2dec(0xA));
    TEST_ASSERT_EQUAL(0, xy_bcd2dec(0x1A));
    TEST_ASSERT_EQUAL(0, xy_bcd2dec(0x10000A));
}


int test_xy_bcd2dec(void)
{
    RUN_TEST(test_xy_bcd2dec_basic);
    RUN_TEST(test_xy_bcd2dec_edge_cases);
    RUN_TEST(test_xy_bcd2dec_invalid_input);
}
