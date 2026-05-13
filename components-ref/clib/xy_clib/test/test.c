#include "stdio.h"
#include "unity/unity.h"
#include "./test_xy_common/test_xy_common.h"
#include "./test_xy_stdlib/test_xy_stdlib.h"
#include "./test_xy_stdio/test_xy_stdio.h"

int main(void)
{
    UNITY_BEGIN();
    test_xy_common();
    test_xy_stdlib();
    test_xy_stdio();
    return UNITY_END();
}