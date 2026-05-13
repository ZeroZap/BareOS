/* =========================================================================
    Unity - A Test Framework for C
    Implementation
   ========================================================================= */

#include "unity.h"
#include <string.h>

/* Unity Globals */
UNITY_TEST_T Unity = { UNITY_PASSED, NULL, NULL, 0 };
int UnityExecTests = 0;
int UnityTestsToRun = 0;
int UnityTestsFailures = 0;
int UnityTestsIgnored = 0;

/* Weak setup/teardown */
void setUp(void) __attribute__((weak));
void tearDown(void) __attribute__((weak));

void setUp(void) { }
void tearDown(void) { }

/* Assert Functions */
void UnityAssertEqualNumber(UNITY_INT expected, UNITY_INT actual, const int line)
{
    if (expected != actual) {
        printf("    Expected: %ld, Actual: %ld\n", (long)expected, (long)actual);
        TEST_FAIL();
    }
}

void UnityAssertEqualString(const char *expected, const char *actual, const int line)
{
    if (expected == NULL && actual == NULL) {
        return;
    }
    if (expected == NULL || actual == NULL) {
        printf("    String mismatch: one is NULL\n");
        TEST_FAIL();
    }
    if (strcmp(expected, actual) != 0) {
        printf("    Expected: \"%s\", Actual: \"%s\"\n", expected, actual);
        TEST_FAIL();
    }
}

void UnityAssertEqualMemory(const void *expected, const void *actual, 
                            UNITY_UINT32 length, UNITY_UINT32 num_elements, const int line)
{
    UNITY_UINT32 i;
    const uint8_t *exp = (const uint8_t *)expected;
    const uint8_t *act = (const uint8_t *)actual;
    
    for (i = 0; i < length * num_elements; i++) {
        if (exp[i] != act[i]) {
            printf("    Memory mismatch at byte %lu\n", (unsigned long)i);
            TEST_FAIL();
        }
    }
}

void UnityAssertEqualIntArray(const UNITY_INT *expected, const UNITY_INT *actual, 
                              UNITY_UINT32 num_elements, const int line)
{
    UNITY_UINT32 i;
    
    for (i = 0; i < num_elements; i++) {
        if (expected[i] != actual[i]) {
            printf("    Array mismatch at index %lu: Expected %ld, Actual %ld\n", 
                   (unsigned long)i, (long)expected[i], (long)actual[i]);
            TEST_FAIL();
        }
    }
}

void UnityAssertIntsWithin(UNITY_INT delta, UNITY_INT expected, UNITY_INT actual, const int line)
{
    UNITY_INT diff = actual - expected;
    if (diff < 0) diff = -diff;
    
    if (diff > delta) {
        printf("    Value %ld not within %ld of %ld\n", (long)actual, (long)delta, (long)expected);
        TEST_FAIL();
    }
}

void UnityAssertBits(UNITY_INT mask, UNITY_INT expected, UNITY_INT actual, const int line)
{
    if ((mask & expected) != (mask & actual)) {
        printf("    Bits mismatch: Expected 0x%lX, Actual 0x%lX (mask 0x%lX)\n", 
               (unsigned long)expected, (unsigned long)actual, (unsigned long)mask);
        TEST_FAIL();
    }
}

int UNITY_END(void)
{
    printf("\n");
    printf("=== Unity Test Summary ===\n");
    printf("Tests Executed: %d\n", UnityExecTests);
    printf("Tests Passed:   %d\n", UnityTestsToRun);
    printf("Tests Failed:   %d\n", UnityTestsFailures);
    printf("Tests Ignored:  %d\n", UnityTestsIgnored);
    printf("========================\n");
    
    return (UnityTestsFailures == 0) ? 0 : 1;
}
