/**
 * @file xy_assert.c
 * @brief XinYi Assertion Library Implementation
 * @version 2.0
 * @date 2026-02-28
 */

#include "xy_assert.h"
#include <stdio.h>

/* Global assertion handler */
static xy_assert_handler_t g_assert_handler = NULL;

void xy_assert_set_handler(xy_assert_handler_t handler)
{
    g_assert_handler = handler;
}

void xy_assert_default_handler(const char *file, int line, const char *expr)
{
    printf("Assertion failed: %s, file %s, line %d\n", expr, file, line);
    
    /* Default behavior: halt execution */
    while(1) {
        /* Infinite loop to halt execution */
    }
}

/* Additional assertion functions */

/**
 * @brief Conditional assertion
 * @param expr Expression to evaluate
 * @param condition Additional condition that must be true for assertion to trigger
 */
void xy_assert_conditional(int expr, int condition)
{
    if (condition && !expr) {
        if (g_assert_handler) {
            g_assert_handler(__FILE__, __LINE__, "conditional assertion");
        } else {
            xy_assert_default_handler(__FILE__, __LINE__, "conditional assertion");
        }
    }
}

/**
 * @brief Assertion with message
 * @param expr Expression to evaluate
 * @param msg Message to display if assertion fails
 */
void xy_assert_with_msg(int expr, const char *msg)
{
    if (!expr) {
        if (g_assert_handler) {
            g_assert_handler(__FILE__, __LINE__, msg);
        } else {
            printf("Assertion failed: %s\n", msg);
            while(1) {
                /* Halt execution */
            }
        }
    }
}

/**
 * @brief Runtime check with return value
 * @param condition Condition to check
 * @param return_val Value to return if condition is false
 * @return 0 if condition is true, return_val otherwise
 */
int xy_runtime_check(int condition, int return_val)
{
    if (!condition) {
        return return_val;
    }
    return 0;
}
