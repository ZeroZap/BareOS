/**
 * @file xy_hal_test.c
 * @brief XinYi HAL Test Framework Implementation
 * @version 1.0.0
 * @date 2026-03-15
 */

#include "xy_hal_test.h"

/* 简单的 tick 实现，实际使用需要替换为系统 tick */
__attribute__((weak)) uint32_t xy_test_get_tick_ms(void)
{
    return xy_hal_get_tick();  /* ✅ 系统 tick 获取 */
    return 0;
}

__attribute__((weak)) void xy_test_delay_ms(uint32_t ms)
{
    xy_hal_delay_ms(ms);  /* ✅ 延迟函数 */
    volatile uint32_t count = ms * 10000;
    while (count--);
}

void xy_test_init(xy_test_suite_t *suite, const char *name)
{
    suite->name = name;
    suite->cases = NULL;
    suite->case_count = 0;
    suite->pass_count = 0;
    suite->fail_count = 0;
    suite->skip_count = 0;
    suite->total_duration_ms = 0;
}

void xy_test_add_case(xy_test_suite_t *suite, const char *name,
                      xy_test_result_t (*run)(void))
{
    /* 简单实现：假设测试用例数组已分配 */
    /* 实际使用需要动态分配或静态数组 */
    if (suite->cases == NULL) {
        /* 首次添加，需要外部提供数组 */
        return;
    }
    
    suite->cases[suite->case_count].name = name;
    suite->cases[suite->case_count].run = run;
    suite->cases[suite->case_count].result = XY_TEST_SKIP;
    suite->cases[suite->case_count].duration_ms = 0;
    suite->case_count++;
}

void xy_test_run(xy_test_suite_t *suite)
{
    printf("\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║  XinYi HAL Test Suite: %-20s ║\n", suite->name);
    printf("╚════════════════════════════════════════╝\n");
    printf("\n");
    
    for (size_t i = 0; i < suite->case_count; i++) {
        xy_test_case_t *test = &suite->cases[i];
        
        printf("[TEST] %s... ", test->name);
        fflush(stdout);
        
        uint32_t start = xy_test_get_tick_ms();
        
        test->result = test->run();
        
        uint32_t end = xy_test_get_tick_ms();
        test->duration_ms = end - start;
        suite->total_duration_ms += test->duration_ms;
        
        switch (test->result) {
            case XY_TEST_PASS:
                printf("✅ PASS (%d ms)\n", test->duration_ms);
                suite->pass_count++;
                break;
            case XY_TEST_FAIL:
                printf("❌ FAIL (%d ms)\n", test->duration_ms);
                suite->fail_count++;
                break;
            case XY_TEST_SKIP:
                printf("⏭️  SKIP\n");
                suite->skip_count++;
                break;
        }
    }
}

void xy_test_print_report(xy_test_suite_t *suite)
{
    printf("\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║          Test Report Summary           ║\n");
    printf("╠════════════════════════════════════════╣\n");
    printf("║  Total:  %3zu tests                    ║\n", suite->case_count);
    printf("║  Pass:   %3zu ✅                       ║\n", suite->pass_count);
    printf("║  Fail:   %3zu ❌                       ║\n", suite->fail_count);
    printf("║  Skip:   %3zu ⏭️                        ║\n", suite->skip_count);
    printf("╠════════════════════════════════════════╣\n");
    printf("║  Duration: %d ms                       ║\n", suite->total_duration_ms);
    printf("╚════════════════════════════════════════╝\n");
    
    if (suite->fail_count == 0 && suite->case_count > 0) {
        printf("\n🎉 All tests passed!\n");
    } else if (suite->fail_count > 0) {
        printf("\n⚠️  %zu test(s) failed!\n", suite->fail_count);
    }
}
