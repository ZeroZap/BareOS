#include "test_xy_common.h"
#include "unity.h"

extern int test_xy_u8_mod10(void);
extern int test_xy_u16_mod10(void);
extern int test_xy_u32_mod10(void);
extern int test_xy_hex2bcd(void);
extern int test_xy_bcd2hex(void);
extern int test_xy_dec2bcd(void);
extern int test_xy_bcd2dec(void);
extern int test_xy_bits(void);
extern int test_xy_list(void);

void setUp(void)
{
}
void tearDown(void)
{
}

int test_xy_common(void)
{
    test_xy_u8_mod10();
    test_xy_u16_mod10();
    test_xy_u32_mod10();
    test_xy_hex2bcd();
    test_xy_bcd2hex();
    test_xy_bcd2dec();
    test_xy_dec2bcd();
    test_xy_bits();
    test_xy_list();
    return 0;
}