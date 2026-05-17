/**
 * @file xy_assert.h
 * @brief XinYi Assertion Library
 * @version 2.0
 * @date 2026-02-28
 */

#ifndef XY_ASSERT_H
#define XY_ASSERT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Assertion handler function pointer
 */
typedef void (*xy_assert_handler_t)(const char *file, int line, const char *expr);

/**
 * @brief Set assertion handler
 * @param handler Handler function
 */
void xy_assert_set_handler(xy_assert_handler_t handler);

/**
 * @brief Default assertion handler
 * @param file Source file name
 * @param line Line number
 * @param expr Assertion expression
 */
void xy_assert_default_handler(const char *file, int line, const char *expr);

/**
 * @brief BSP-overridable hook invoked before the default assert handler
 *        enters its halt loop. Use to flush logs, save crash dump, kick
 *        the watchdog into a forced reset, etc. Default is a no-op.
 */
void xy_assert_before_halt(void);

/**
 * @brief Assertion macro
 * @param expr Expression to evaluate
 */
#ifdef XY_DISABLE_ASSERT
#define XY_ASSERT(expr)     ((void)0)
#else
#define XY_ASSERT(expr) \
    do { \
        if (!(expr)) { \
            xy_assert_default_handler(__FILE__, __LINE__, #expr); \
        } \
    } while(0)
#endif

/**
 * @brief Assertion with custom handler
 * @param expr Expression to evaluate
 * @param handler Custom handler function
 */
#define XY_ASSERT_CUSTOM(expr, handler) \
    do { \
        if (!(expr)) { \
            handler(__FILE__, __LINE__, #expr); \
        } \
    } while(0)

/**
 * @brief Runtime assertion check
 * @param condition Condition to check
 * @param error_code Error code to return if assertion fails
 * @return 0 if condition is true, error_code otherwise
 */
#define XY_RUNTIME_CHECK(condition, error_code) \
    do { \
        if (!(condition)) { \
            return (error_code); \
        } \
    } while(0)

/**
 * @brief Compile-time assertion (C11 static_assert)
 */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define XY_STATIC_ASSERT(condition, message) _Static_assert(condition, message)
#else
/* Fallback for older C standards */
#define XY_STATIC_ASSERT(condition, message) \
    do { \
        switch(0) { case 0: case !(condition): ; } \
    } while(0)
#endif

/**
 * @brief Null pointer check
 * @param ptr Pointer to check
 * @param error_code Error code to return if null
 */
#define XY_NULL_CHECK(ptr, error_code) \
    XY_RUNTIME_CHECK((ptr) != NULL, (error_code))

/**
 * @brief Range check
 * @param value Value to check
 * @param min Minimum value (inclusive)
 * @param max Maximum value (inclusive)
 * @param error_code Error code to return if out of range
 */
#define XY_RANGE_CHECK(value, min, max, error_code) \
    XY_RUNTIME_CHECK(((value) >= (min)) && ((value) <= (max)), (error_code))

/**
 * @brief Array bounds check
 * @param index Index to check
 * @param size Array size
 * @param error_code Error code to return if out of bounds
 */
#define XY_BOUNDS_CHECK(index, size, error_code) \
    XY_RUNTIME_CHECK((index) < (size), (error_code))

#ifdef __cplusplus
}
#endif

#endif /* XY_ASSERT_H */
