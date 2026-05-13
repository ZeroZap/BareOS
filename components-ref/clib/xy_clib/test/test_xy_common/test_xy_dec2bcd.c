#include "unity.h"
#include "xy_common.h"


static void test_xy_dec2bcd_basic(void)
{
    TEST_ASSERT_EQUAL_HEX32(0x0, xy_dec2bcd(0));
    TEST_ASSERT_EQUAL_HEX32(0x9, xy_dec2bcd(9));
    TEST_ASSERT_EQUAL_HEX32(0x99, xy_dec2bcd(99));
    TEST_ASSERT_EQUAL_HEX32(0x1234, xy_dec2bcd(1234));
    TEST_ASSERT_EQUAL_HEX32(0x9999, xy_dec2bcd(9999));
    TEST_ASSERT_EQUAL_HEX32(0x12345678, xy_dec2bcd(12345678));
}


static void test_xy_dec2bcd_edge_cases(void)
{
    TEST_ASSERT_EQUAL_HEX32(0x1, xy_dec2bcd(1));
    TEST_ASSERT_EQUAL_HEX32(0x10, xy_dec2bcd(10));
    TEST_ASSERT_EQUAL_HEX32(0x100, xy_dec2bcd(100));
    TEST_ASSERT_EQUAL_HEX32(0x1000, xy_dec2bcd(1000));
    TEST_ASSERT_EQUAL_HEX32(0x99999999, xy_dec2bcd(99999999));
}

void test_xy_dec2bcd_large_numbers(void)
{
    TEST_ASSERT_EQUAL_HEX32(0x12345678, xy_dec2bcd(12345678));
    TEST_ASSERT_EQUAL_HEX32(0x98765432, xy_dec2bcd(98765432));
    TEST_ASSERT_EQUAL_HEX32(0x10000000, xy_dec2bcd(10000000));
}


void test_xy_decxbdc_round_trip_conversion(void)
{
    uint32_t original       = 12345678;
    uint32_t bcd            = xy_dec2bcd(original);
    uint32_t converted_back = xy_bcd2dec(bcd);
    TEST_ASSERT_EQUAL_UINT32(original, converted_back);

    original       = 9999;
    bcd            = xy_dec2bcd(original);
    converted_back = xy_bcd2dec(bcd);
    TEST_ASSERT_EQUAL_UINT32(original, converted_back);
}


int test_xy_dec2bcd(void)
{
    RUN_TEST(test_xy_dec2bcd_basic);
    RUN_TEST(test_xy_dec2bcd_edge_cases);
    RUN_TEST(test_xy_dec2bcd_large_numbers);
    RUN_TEST(test_xy_decxbdc_round_trip_conversion);
}
