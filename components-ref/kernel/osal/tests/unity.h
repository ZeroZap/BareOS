/* =========================================================================
    Unity - A Test Framework for C
    Copyright (c) 2007-2023 ThrowTheSwitchOrg
    License: MIT
    https://github.com/ThrowTheSwitch/Unity
   ========================================================================= */

#ifndef _UNITY_H
#define _UNITY_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* Unity Configuration */
#ifndef UNITY_INT_WIDTH
#define UNITY_INT_WIDTH 32
#endif

#ifndef UNITY_POINTER_WIDTH
#define UNITY_POINTER_WIDTH 32
#endif

/* Unity Types */
typedef int32_t UNITY_INT;
typedef uint32_t UNITY_UINT;
typedef int8_t UNITY_INT8;
typedef uint8_t UNITY_UINT8;
typedef int16_t UNITY_INT16;
typedef uint16_t UNITY_UINT16;
typedef int32_t UNITY_INT32;
typedef uint32_t UNITY_UINT32;

/* Unity Test Result */
typedef enum {
    UNITY_IGNORED,
    UNITY_PASSED,
    UNITY_FAILED
} UNITY_TEST_RESULT;

/* Unity Test Structure */
typedef struct {
    UNITY_TEST_RESULT status;
    const char *name;
    const char *file;
    int line_number;
} UNITY_TEST_T;

/* Unity Globals */
extern UNITY_TEST_T Unity;
extern int UnityExecTests;
extern int UnityTestsToRun;
extern int UnityTestsFailures;
extern int UnityTestsIgnored;

/* Test Control */
#define TEST_PASS() { Unity.status = UNITY_PASSED; return; }
#define TEST_FAIL() { Unity.status = UNITY_FAILED; return; }
#define TEST_FAIL_MESSAGE(message) { UnityAssertEqualString((const char*)(message), NULL, __LINE__); TEST_FAIL(); }
#define TEST_IGNORE() { Unity.status = UNITY_IGNORED; UnityTestsIgnored++; return; }

/* Test Assertions */
#define TEST_ASSERT(condition) \
    if (!(condition)) { TEST_FAIL_MESSAGE("Assertion failed: " #condition); }

#define TEST_ASSERT_TRUE(condition) \
    TEST_ASSERT(condition)

#define TEST_ASSERT_FALSE(condition) \
    TEST_ASSERT(!(condition))

#define TEST_ASSERT_NULL(pointer) \
    TEST_ASSERT((pointer) == NULL)

#define TEST_ASSERT_NOT_NULL(pointer) \
    TEST_ASSERT((pointer) != NULL)

#define TEST_ASSERT_EQUAL(expected, actual) \
    UnityAssertEqualNumber((UNITY_INT)(expected), (UNITY_INT)(actual), __LINE__)

#define TEST_ASSERT_NOT_EQUAL(expected, actual) \
    TEST_ASSERT((expected) != (actual))

#define TEST_ASSERT_EQUAL_INT(expected, actual) \
    TEST_ASSERT_EQUAL((UNITY_INT)(expected), (UNITY_INT)(actual))

#define TEST_ASSERT_EQUAL_UINT(expected, actual) \
    UnityAssertEqualNumber((UNITY_INT)(expected), (UNITY_INT)(actual), __LINE__)

#define TEST_ASSERT_EQUAL_HEX(expected, actual) \
    UnityAssertEqualNumber((UNITY_INT)(expected), (UNITY_INT)(actual), __LINE__)

#define TEST_ASSERT_EQUAL_PTR(expected, actual) \
    UnityAssertEqualNumber((UNITY_PTR_TO_INT)(expected), (UNITY_PTR_TO_INT)(actual), __LINE__)

#define TEST_ASSERT_EQUAL_STRING(expected, actual) \
    UnityAssertEqualString((const char*)(expected), (const char*)(actual), __LINE__)

#define TEST_ASSERT_EQUAL_MEMORY(expected, actual, length) \
    UnityAssertEqualMemory((const void*)(expected), (const void*)(actual), (UNITY_UINT32)(length), 1, __LINE__)

#define TEST_ASSERT_INT_WITHIN(delta, expected, actual) \
    UnityAssertIntsWithin((UNITY_INT)(delta), (UNITY_INT)(expected), (UNITY_INT)(actual), __LINE__)

#define TEST_ASSERT_BITS(mask, expected, actual) \
    UnityAssertBits((UNITY_INT)(mask), (UNITY_INT)(expected), (UNITY_INT)(actual), __LINE__)

/* Array Assertions */
#define TEST_ASSERT_EQUAL_INT_ARRAY(expected, actual, length) \
    UnityAssertEqualIntArray((const UNITY_INT*)(expected), (const UNITY_INT*)(actual), (UNITY_UINT32)(length), __LINE__)

#define TEST_ASSERT_EQUAL_UINT_ARRAY(expected, actual, length) \
    UnityAssertEqualIntArray((const UNITY_INT*)(expected), (const UNITY_INT*)(actual), (UNITY_UINT32)(length), __LINE__)

#define TEST_ASSERT_EQUAL_HEX_ARRAY(expected, actual, length) \
    UnityAssertEqualIntArray((const UNITY_INT*)(expected), (const UNITY_INT*)(actual), (UNITY_UINT32)(length), __LINE__)

/* Message Assertions */
#define TEST_ASSERT_MESSAGE(condition, message) \
    if (!(condition)) { TEST_FAIL_MESSAGE(message); }

#define TEST_ASSERT_EQUAL_MESSAGE(expected, actual, message) \
    if ((expected) != (actual)) { TEST_FAIL_MESSAGE(message); }

/* Test Runner */
#define RUN_TEST(test_func) \
    do { \
        Unity.Test.name = #test_func; \
        Unity.Test.line_number = 0; \
        Unity.status = UNITY_PASSED; \
        UnityExecTests++; \
        test_func(); \
        if (Unity.status == UNITY_PASSED) { \
            UnityTestsToRun++; \
            printf("  [PASS] %s\n", Unity.Test.name); \
        } else if (Unity.status == UNITY_FAILED) { \
            UnityTestsFailures++; \
            printf("  [FAIL] %s\n", Unity.Test.name); \
        } else if (Unity.status == UNITY_IGNORED) { \
            printf("  [IGNR] %s\n", Unity.Test.name); \
        } \
    } while (0)

/* Test Setup and Teardown (weak) */
void setUp(void) __attribute__((weak));
void tearDown(void) __attribute__((weak));

/* Unity Internal Functions */
void UnityAssertEqualNumber(UNITY_INT expected, UNITY_INT actual, const int line);
void UnityAssertEqualString(const char *expected, const char *actual, const int line);
void UnityAssertEqualMemory(const void *expected, const void *actual, UNITY_UINT32 length, UNITY_UINT32 num_elements, const int line);
void UnityAssertEqualIntArray(const UNITY_INT *expected, const UNITY_INT *actual, UNITY_UINT32 num_elements, const int line);
void UnityAssertIntsWithin(UNITY_INT delta, UNITY_INT expected, UNITY_INT actual, const int line);
void UnityAssertBits(UNITY_INT mask, UNITY_INT expected, UNITY_INT actual, const int line);
int UNITY_END(void);

/* Unity Pointer Support */
#if UNITY_POINTER_WIDTH == 32
typedef UNITY_INT32 UNITY_PTR_TO_INT;
#elif UNITY_POINTER_WIDTH == 16
typedef UNITY_INT16 UNITY_PTR_TO_INT;
#else
typedef UNITY_INT UNITY_PTR_TO_INT;
#endif

#endif /* _UNITY_H */
