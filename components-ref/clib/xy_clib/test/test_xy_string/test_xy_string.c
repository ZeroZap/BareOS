#include "unity.h"
#include "xy_string.h"
#include <string.h>

static void test_xy_strchr(void)
{
    const char *s = "hello";
    TEST_ASSERT_EQUAL_PTR(s + 1, xy_strchr(s, 'e'));
    TEST_ASSERT_NULL(xy_strchr(s, 'x'));
    TEST_ASSERT_NULL(xy_strchr(NULL, 'h'));
}

static void test_xy_strcspn(void)
{
    TEST_ASSERT_EQUAL_UINT32(
        3, xy_strcspn("abcdef", "def")); // first match at index 3
    TEST_ASSERT_EQUAL_UINT32(
        6, xy_strcspn("abcdef", "xyz")); // no match returns length
    TEST_ASSERT_EQUAL_UINT32(0, xy_strcspn(NULL, "a"));
}

static void test_xy_strpbrk(void)
{
    TEST_ASSERT_EQUAL_INT(1, xy_strpbrk("hello", "e"));
    TEST_ASSERT_EQUAL_INT(-1, xy_strpbrk("hello", "xyz"));
    TEST_ASSERT_EQUAL_INT(-1, xy_strpbrk(NULL, "h"));
}

static void test_xy_strstr(void)
{
    const char *h = "embedded system";
    TEST_ASSERT_EQUAL_PTR(h + 9, xy_strstr(h, "system"));
    TEST_ASSERT_NULL(xy_strstr(h, "xyz"));
    TEST_ASSERT_NULL(xy_strstr(NULL, "a"));
    TEST_ASSERT_EQUAL_PTR(h, xy_strstr(h, ""));
}

static void test_xy_strncpy(void)
{
    char buf[8];
    xy_memset(buf, 0xAA, sizeof(buf));
    xy_strncpy(buf, "hi", 2);
    TEST_ASSERT_EQUAL_UINT8('h', buf[0]);
    TEST_ASSERT_EQUAL_UINT8('i', buf[1]);
}

int test_xy_string(void)
{
    RUN_TEST(test_xy_strchr);
    RUN_TEST(test_xy_strcspn);
    RUN_TEST(test_xy_strpbrk);
    RUN_TEST(test_xy_strstr);
    RUN_TEST(test_xy_strncpy);
    return 0;
}
