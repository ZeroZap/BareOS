#include "unity.h"
#include "xy_stdio.h"
#include "xy_string.h"
#include <string.h>
#include <stdarg.h>

// Mock print function for testing
static char g_print_buf[256] = { 0 };
static void test_print(char *str)
{
    strcpy(g_print_buf, str);
}

void test_sprintf_basic(void)
{
    char buf[256];
    int ret;

    // Test negative number
    memset(buf, 0, sizeof(buf));
    ret = xy_stdio_sprintf(buf, "Value: %d", -42);
    TEST_ASSERT_EQUAL_STRING("Value: -42", buf);
    TEST_ASSERT_EQUAL(10, ret); // "Value: -42" is 10 characters

    // Test large negative number
    memset(buf, 0, sizeof(buf));
    ret = xy_stdio_sprintf(buf, "%d", -2147483647);
    TEST_ASSERT_EQUAL_STRING("-2147483647", buf);
    TEST_ASSERT_EQUAL(11, ret);

    // Test with padding
    memset(buf, 0, sizeof(buf));
    ret = xy_stdio_sprintf(buf, "%8d", -42);
    TEST_ASSERT_EQUAL_STRING("     -42", buf);
    TEST_ASSERT_EQUAL(8, ret);

    // Test with zero padding
    memset(buf, 0, sizeof(buf));
    ret = xy_stdio_sprintf(buf, "%08d", -42);
    TEST_ASSERT_EQUAL_STRING("-0000042", buf);
    TEST_ASSERT_EQUAL(8, ret);
}

void test_sprintf_numbers(void)
{
    char buf[256];

    // Test integers
    xy_stdio_sprintf(buf, "%d", 12345);
    TEST_ASSERT_EQUAL_STRING("12345", buf);

    xy_stdio_sprintf(buf, "%d", -12345);
    TEST_ASSERT_EQUAL_STRING("-12345", buf);

    // Test hex
    xy_stdio_sprintf(buf, "%x", 0xabcd);
    TEST_ASSERT_EQUAL_STRING("abcd", buf);

    xy_stdio_sprintf(buf, "%X", 0xABCD);
    TEST_ASSERT_EQUAL_STRING("ABCD", buf);
}

void test_sprintf_padding(void)
{
    char buf[256];

    // Test width padding
    xy_stdio_sprintf(buf, "%5d", 42);
    TEST_ASSERT_EQUAL_STRING("   42", buf);

    xy_stdio_sprintf(buf, "%-5d", 42);
    TEST_ASSERT_EQUAL_STRING("42   ", buf);

    xy_stdio_sprintf(buf, "%05d", 42);
    TEST_ASSERT_EQUAL_STRING("00042", buf);
}

void test_printf(void)
{
    int ret;

    ret = xy_stdio_printf("Test");
    TEST_ASSERT_EQUAL_STRING("Test", g_print_buf);
    TEST_ASSERT_EQUAL(4, ret);

    ret = xy_stdio_printf("Value: %d", 42);
    TEST_ASSERT_EQUAL_STRING("Value: 42", g_print_buf);
    TEST_ASSERT_EQUAL(9, ret);
}

void test_snprintf(void)
{
    char buf[8];
    int ret;

    // Test truncation
    ret = xy_stdio_snprintf(buf, sizeof(buf), "1234567890");
    TEST_ASSERT_EQUAL_STRING("1234567", buf);
    TEST_ASSERT_EQUAL(7, ret);

    // Test exact fit
    ret = xy_stdio_snprintf(buf, sizeof(buf), "1234567");
    TEST_ASSERT_EQUAL_STRING("1234567", buf);
    TEST_ASSERT_EQUAL(7, ret);
}

void test_scanf(void)
{
    int value;
    char str[32];

    // Test integer scan
    xy_memset(g_print_buf, 0, sizeof(g_print_buf));
    strcpy(g_print_buf, "42");
    TEST_ASSERT_EQUAL(1, xy_stdio_scanf("%d", &value));
    TEST_ASSERT_EQUAL(42, value);

    // Test string scan
    xy_memset(g_print_buf, 0, sizeof(g_print_buf));
    strcpy(g_print_buf, "Hello");
    TEST_ASSERT_EQUAL(1, xy_stdio_scanf("%s", str));
    TEST_ASSERT_EQUAL_STRING("Hello", str);
}

void test_sscanf(void)
{
    int value;
    char str[32];

    // Test integer scan
    TEST_ASSERT_EQUAL(1, xy_stdio_sscanf("42", "%d", &value));
    TEST_ASSERT_EQUAL(42, value);

    // Test string scan
    TEST_ASSERT_EQUAL(1, xy_stdio_sscanf("Hello World", "%s", str));
    TEST_ASSERT_EQUAL_STRING("Hello", str);

    // Test multiple scans
    int a, b;
    TEST_ASSERT_EQUAL(2, xy_stdio_sscanf("123 456", "%d %d", &a, &b));
    TEST_ASSERT_EQUAL(123, a);
    TEST_ASSERT_EQUAL(456, b);
}

#ifdef XY_PRINTF_FLOAT_ENABLE
void test_float_printing(void)
{
    char buf[256];

    // Test basic float
    xy_stdio_sprintf(buf, "%.2f", 3.14159);
    TEST_ASSERT_EQUAL_STRING("3.14", buf);

    // Test negative float
    xy_stdio_sprintf(buf, "%.3f", -3.14159);
    TEST_ASSERT_EQUAL_STRING("-3.142", buf);

    // Test zero
    xy_stdio_sprintf(buf, "%.1f", 0.0);
    TEST_ASSERT_EQUAL_STRING("0.0", buf);
}
#endif

// Helper function to test vsprintf with variable arguments
static void test_vsprintf_helper(char *buf, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    xy_stdio_vsprintf(buf, fmt, args);
    va_end(args);
}

void test_vsprintf(void)
{
    char buf[256];

    // Test with integer
    test_vsprintf_helper(buf, "%d", 42);
    TEST_ASSERT_EQUAL_STRING("42", buf);

    // Test with string
    test_vsprintf_helper(buf, "%s", "test");
    TEST_ASSERT_EQUAL_STRING("test", buf);

    // Test with multiple args
    test_vsprintf_helper(buf, "Value: %d %s", 42, "test");
    TEST_ASSERT_EQUAL_STRING("Value: 42 test", buf);
}

// Helper function to test vsnprintf with variable arguments
static void test_vsnprintf_helper(char *buf, size_t size, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    xy_stdio_vsnprintf(buf, size, fmt, args);
    va_end(args);
}

void test_vsnprintf(void)
{
    char buf[8];

    // Test truncation
    test_vsnprintf_helper(buf, sizeof(buf), "%s", "1234567890");
    TEST_ASSERT_EQUAL_STRING("1234567", buf);

    // Test exact fit
    test_vsnprintf_helper(buf, sizeof(buf), "%s", "1234567");
    TEST_ASSERT_EQUAL_STRING("1234567", buf);
}

// Helper function to test vsscanf with variable arguments
static int test_vsscanf_helper(const char *input, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int ret = xy_stdio_vsscanf(input, fmt, args);
    va_end(args);
    return ret;
}

void test_vsscanf(void)
{
    int value;
    char str[32];

    // Test integer scan
    TEST_ASSERT_EQUAL(1, test_vsscanf_helper("42", "%d", &value));
    TEST_ASSERT_EQUAL(42, value);

    // Test string scan
    TEST_ASSERT_EQUAL(1, test_vsscanf_helper("Hello World", "%s", str));
    TEST_ASSERT_EQUAL_STRING("Hello", str);

    // Test multiple scans
    int a, b;
    TEST_ASSERT_EQUAL(2, test_vsscanf_helper("123 456", "%d %d", &a, &b));
    TEST_ASSERT_EQUAL(123, a);
    TEST_ASSERT_EQUAL(456, b);
}

void test_xy_stdio(void)
{
    xy_stdio_printf_init(test_print);
    RUN_TEST(test_sprintf_basic);
    RUN_TEST(test_sprintf_numbers);
    RUN_TEST(test_sprintf_padding);
    RUN_TEST(test_printf);
    RUN_TEST(test_snprintf);
    RUN_TEST(test_scanf);
    RUN_TEST(test_sscanf);
    RUN_TEST(test_vsprintf);
    RUN_TEST(test_vsnprintf);
    RUN_TEST(test_vsscanf);
#ifdef XY_PRINTF_FLOAT_ENABLE
    RUN_TEST(test_float_printing);
#endif
}