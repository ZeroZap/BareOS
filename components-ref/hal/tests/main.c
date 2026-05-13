/**
 * @file main.c
 * @brief XinYi HAL Test Suite Main Entry
 * @version 1.0.0
 * @date 2026-03-15
 * 
 * @note 运行所有 HAL 测试用例
 */

#include "xy_hal_test.h"

/* 外部测试套件函数声明 */
extern void run_gpio_tests(void);
extern void run_uart_tests(void);
extern void run_spi_tests(void);
extern void run_i2c_tests(void);
extern void run_stress_tests(void);

/* ==================== Main ==================== */

int main(void)
{
    printf("\n");
    printf("╔═══════════════════════════════════════════╗\n");
    printf("║                                           ║\n");
    printf("║     XinYi HAL Test Suite v1.0.0           ║\n");
    printf("║     + Stress & Edge Case Tests            ║\n");
    printf("║                                           ║\n");
    printf("╚═══════════════════════════════════════════╝\n");
    printf("\n");
    
    /* 运行各模块测试 */
    printf("=== GPIO Tests ===\n");
    run_gpio_tests();
    printf("\n");
    
    printf("=== UART Tests ===\n");
    run_uart_tests();
    printf("\n");
    
    printf("=== SPI Tests ===\n");
    run_spi_tests();
    printf("\n");
    
    printf("=== I2C Tests ===\n");
    run_i2c_tests();
    printf("\n");
    
    printf("=== Stress Tests ===\n");
    run_stress_tests();
    printf("\n");
    
    /* 总体汇总 */
    printf("╔═══════════════════════════════════════════╗\n");
    printf("║          All Test Suites Completed        ║\n");
    printf("║     ✅ Core + Edge Cases + Stress         ║\n");
    printf("╚═══════════════════════════════════════════╝\n");
    printf("\n");
    
    return 0;
}
