/**
 * @file xy_hal_test.h
 * @brief XinYi HAL Test Framework
 * @version 1.0.0
 * @date 2026-03-15
 * 
 * @note 轻量级 HAL 测试框架，无需外部依赖
 */

#ifndef XY_HAL_TEST_H
#define XY_HAL_TEST_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Test Result Types ==================== */

typedef enum {
    XY_TEST_PASS = 0,
    XY_TEST_FAIL,
    XY_TEST_SKIP,
} xy_test_result_t;

/* ==================== Test Case Structure ==================== */

typedef struct {
    const char *name;
    xy_test_result_t (*run)(void);
    xy_test_result_t result;
    uint32_t duration_ms;
} xy_test_case_t;

/* ==================== Test Suite Structure ==================== */

typedef struct {
    const char *name;
    xy_test_case_t *cases;
    size_t case_count;
    size_t pass_count;
    size_t fail_count;
    size_t skip_count;
    uint32_t total_duration_ms;
} xy_test_suite_t;

/* ==================== Test Framework API ==================== */

/**
 * @brief 初始化测试套件
 */
void xy_test_init(xy_test_suite_t *suite, const char *name);

/**
 * @brief 添加测试用例
 */
void xy_test_add_case(xy_test_suite_t *suite, const char *name, 
                      xy_test_result_t (*run)(void));

/**
 * @brief 运行所有测试
 */
void xy_test_run(xy_test_suite_t *suite);

/**
 * @brief 打印测试结果
 */
void xy_test_print_report(xy_test_suite_t *suite);

/**
 * @brief 断言宏
 */
#define XY_TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            printf("  ❌ ASSERT FAILED: %s:%d - %s\n", __FILE__, __LINE__, #cond); \
            return XY_TEST_FAIL; \
        } \
    } while(0)

#define XY_TEST_ASSERT_EQ(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            printf("  ❌ ASSERT EQ FAILED: %s:%d - expected %ld, got %ld\n", \
                   __FILE__, __LINE__, (long)(expected), (long)(actual)); \
            return XY_TEST_FAIL; \
        } \
    } while(0)

#define XY_TEST_ASSERT_NULL(ptr) \
    do { \
        if ((ptr) != NULL) { \
            printf("  ❌ ASSERT NULL FAILED: %s:%d\n", __FILE__, __LINE__); \
            return XY_TEST_FAIL; \
        } \
    } while(0)

#define XY_TEST_ASSERT_NOT_NULL(ptr) \
    do { \
        if ((ptr) == NULL) { \
            printf("  ❌ ASSERT NOT NULL FAILED: %s:%d\n", __FILE__, __LINE__); \
            return XY_TEST_FAIL; \
        } \
    } while(0)

/* ==================== Test Utilities ==================== */

/**
 * @brief 获取当前时间戳 (毫秒)
 */
uint32_t xy_test_get_tick_ms(void);

/**
 * @brief 延迟指定毫秒
 */
void xy_test_delay_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* XY_HAL_TEST_H */
