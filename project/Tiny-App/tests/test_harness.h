/*
 * test_harness.h — minimal bare-metal C assertion macros for regression tests.
 *
 * Usage:
 *   TEST_CASE("descriptive name") {
 *       TEST_EQ(actual, expected);
 *       TEST_TRUE(condition);
 *   }
 *
 * Runs against host libc (PC build). Exit code = number of failures.
 */
#ifndef TEST_HARNESS_H
#define TEST_HARNESS_H

#include <stdio.h>
#include <string.h>

extern int g_test_failures;
extern int g_test_count;
extern const char *g_test_current_name;
extern const char *g_test_filter;   /* NULL = run all; else substring match */
extern int g_test_list_only;        /* 1 = print names, do not execute */

/* TEST_CASE skips execution unless name matches the filter (or no filter set).
 * In --list mode it prints the name and skips the body. */
#define TEST_CASE(name)                                                        \
    for (g_test_current_name = (name);                                         \
         g_test_current_name != NULL;                                          \
         g_test_current_name = NULL)                                           \
        if (g_test_list_only) {                                                \
            fprintf(stdout, "%s\n", g_test_current_name);                      \
            continue;                                                          \
        } else if (g_test_filter == NULL                                       \
                || strstr(g_test_current_name, g_test_filter) != NULL)         \
            for (int _once = (g_test_count++,                                  \
                              fprintf(stderr, "[ RUN  ] %s\n",                 \
                                      g_test_current_name), 1);                \
                 _once; _once = 0)

#define TEST_FAIL_(fmt, ...)                                                   \
    do {                                                                       \
        g_test_failures++;                                                     \
        fprintf(stderr, "[ FAIL ] %s -- %s:%d: " fmt "\n",                     \
                g_test_current_name, __FILE__, __LINE__, ##__VA_ARGS__);       \
    } while (0)

#define TEST_TRUE(cond)                                                        \
    do { if (!(cond)) TEST_FAIL_("expected true: %s", #cond); } while (0)

#define TEST_FALSE(cond)                                                       \
    do { if ((cond))  TEST_FAIL_("expected false: %s", #cond); } while (0)

#define TEST_EQ(a, b)                                                          \
    do {                                                                       \
        long long _a = (long long)(a), _b = (long long)(b);                    \
        if (_a != _b)                                                          \
            TEST_FAIL_("%s == %s (got %lld, want %lld)", #a, #b, _a, _b);      \
    } while (0)

#define TEST_NEQ(a, b)                                                         \
    do {                                                                       \
        long long _a = (long long)(a), _b = (long long)(b);                    \
        if (_a == _b)                                                          \
            TEST_FAIL_("%s != %s (both %lld)", #a, #b, _a);                    \
    } while (0)

#define TEST_MEM_EQ(a, b, n)                                                   \
    do {                                                                       \
        if (memcmp((a), (b), (n)) != 0)                                        \
            TEST_FAIL_("memory mismatch: %s vs %s (%d bytes)", #a, #b, (int)(n)); \
    } while (0)

#define TEST_STR_EQ(a, b)                                                      \
    do {                                                                       \
        if (strcmp((a), (b)) != 0)                                             \
            TEST_FAIL_("string mismatch: \"%s\" vs \"%s\"", (a), (b));         \
    } while (0)

#endif /* TEST_HARNESS_H */
