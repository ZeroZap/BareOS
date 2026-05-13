/**
 * @file main.c
 * @brief XinYi Device Model Test Suite Main Entry
 * @version 1.0.0
 * @date 2026-03-15
 */

#include "xy_hal_test.h"

/* 外部测试套件函数声明 */
extern void run_pm_tests(void);
extern void run_async_tests(void);

/* ==================== Main ==================== */

int main(void)
{
    printf("\n");
    printf("╔═══════════════════════════════════════════╗\n");
    printf("║                                           ║\n");
    printf("║     XinYi Device Model Test Suite v1.0    ║\n");
    printf("║                                           ║\n");
    printf("╚═══════════════════════════════════════════╝\n");
    printf("\n");
    
    /* 运行各模块测试 */
    run_pm_tests();
    printf("\n");
    
    run_async_tests();
    printf("\n");
    
    /* 总体汇总 */
    printf("╔═══════════════════════════════════════════╗\n");
    printf("║         All Test Suites Completed         ║\n");
    printf("╚═══════════════════════════════════════════╝\n");
    printf("\n");
    
    return 0;
}
