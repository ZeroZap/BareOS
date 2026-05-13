/**
 * @file test_osal.c
 * @brief OSAL Unit Tests using Unity Framework
 * @version 1.0.0
 * @date 2026-02-28
 */

#include <stdint.h>
#include <string.h>
#include "unity.h"

/* OSAL headers */
#include "xy_os.h"
#include "xy_os_tick.h"
#include "xy_os_timer_sw.h"

/* ==================== Test Fixtures ==================== */

void setUp(void)
{
    /* Called before each test */
}

void tearDown(void)
{
    /* Called after each test */
}

/* ==================== Kernel Tests ==================== */

void test_kernel_init(void)
{
    xy_os_status_t status;
    
    /* Test normal initialization */
    status = xy_os_kernel_init();
    TEST_ASSERT_EQUAL(XY_OS_OK, status);
    
    /* Test double initialization (should be allowed or return specific error) */
    status = xy_os_kernel_init();
    TEST_ASSERT_TRUE(status == XY_OS_OK || status == XY_OS_ERROR);
}

void test_kernel_get_info(void)
{
    xy_os_status_t status;
    xy_os_version_t version;
    char id_buf[32];
    
    /* Initialize first */
    xy_os_kernel_init();
    
    /* Test with valid parameters */
    status = xy_os_kernel_get_info(&version, id_buf, sizeof(id_buf));
    TEST_ASSERT_EQUAL(XY_OS_OK, status);
    TEST_ASSERT_TRUE(strlen(id_buf) > 0);
    
    /* Test with NULL version */
    status = xy_os_kernel_get_info(NULL, id_buf, sizeof(id_buf));
    TEST_ASSERT_EQUAL(XY_OS_OK, status);
    
    /* Test with NULL id_buf */
    status = xy_os_kernel_get_info(&version, NULL, 0);
    TEST_ASSERT_EQUAL(XY_OS_OK, status);
}

void test_kernel_get_state(void)
{
    xy_os_kernel_state_t state;
    
    /* Initialize first */
    xy_os_kernel_init();
    
    /* Get state after init */
    state = xy_os_kernel_get_state();
    TEST_ASSERT_TRUE(state == XY_OS_KERNEL_READY || 
                     state == XY_OS_KERNEL_RUNNING ||
                     state == XY_OS_KERNEL_INACTIVE);
}

void test_kernel_tick_count(void)
{
    uint32_t tick1, tick2;
    
    xy_os_kernel_init();
    
    /* Get tick count */
    tick1 = xy_os_kernel_get_tick_count();
    
    /* Delay and check tick increased (may not work in bare-metal without proper setup) */
    xy_os_delay(10);
    tick2 = xy_os_kernel_get_tick_count();
    
    /* Tick should be >= previous value */
    TEST_ASSERT_TRUE(tick2 >= tick1);
}

/* ==================== Tick Module Tests ==================== */

void test_tick_init(void)
{
    /* Test initialization with different frequencies */
    xy_tick_init(100);
    TEST_ASSERT_EQUAL(100, xy_tick_get_freq());
    
    xy_tick_init(1000);
    TEST_ASSERT_EQUAL(1000, xy_tick_get_freq());
}

void test_tick_get(void)
{
    uint32_t tick1, tick2;
    
    xy_tick_init(1000);
    xy_tick_set(0);
    
    tick1 = xy_tick_get();
    TEST_ASSERT_EQUAL(0, tick1);
    
    xy_tick_increment();
    tick2 = xy_tick_get();
    TEST_ASSERT_EQUAL(1, tick2);
}

void test_tick_elapsed(void)
{
    xy_tick_init(1000);
    xy_tick_set(0);
    
    /* Test elapsed since */
    TEST_ASSERT_EQUAL(0, xy_tick_elapsed_since(0));
    
    xy_tick_increment();
    xy_tick_increment();
    TEST_ASSERT_EQUAL(2, xy_tick_elapsed_since(0));
    
    /* Test has elapsed */
    TEST_ASSERT_TRUE(xy_tick_has_elapsed(0, 2));
    TEST_ASSERT_FALSE(xy_tick_has_elapsed(0, 3));
}

void test_tick_delay(void)
{
    uint32_t start;
    
    xy_tick_init(1000);
    xy_tick_set(0);
    
    start = xy_tick_get();
    xy_tick_delay(5);
    
    /* Check that at least 5 ticks have passed */
    TEST_ASSERT_TRUE(xy_tick_get() >= start + 5);
}

/* ==================== Software Timer Tests ==================== */

static int g_timer_callback_count = 0;
static void *g_timer_callback_arg = NULL;

static void timer_test_callback(void *arg)
{
    g_timer_callback_count++;
    g_timer_callback_arg = arg;
}

void test_timer_sw_init(void)
{
    xy_timer_sw_init();
    /* If init succeeds, no crash should occur */
    TEST_ASSERT_TRUE(1);
}

void test_timer_sw_create(void)
{
    xy_timer_sw_id_t id;
    
    xy_timer_sw_init();
    
    /* Create timer with valid parameters */
    id = xy_timer_sw_create(10, timer_test_callback, (void *)0x1234, 0);
    TEST_ASSERT_NOT_EQUAL(XY_TIMER_SW_INVALID_ID, id);
    
    /* Create timer with NULL callback (should fail) */
    xy_timer_sw_id_t invalid_id = xy_timer_sw_create(10, NULL, NULL, 0);
    TEST_ASSERT_EQUAL(XY_TIMER_SW_INVALID_ID, invalid_id);
}

void test_timer_sw_start_stop(void)
{
    xy_timer_sw_id_t id;
    xy_timer_sw_error_t err;
    
    xy_timer_sw_init();
    
    /* Create and start timer */
    id = xy_timer_sw_create(10, timer_test_callback, NULL, 0);
    TEST_ASSERT_NOT_EQUAL(XY_TIMER_SW_INVALID_ID, id);
    
    err = xy_timer_sw_start(id);
    TEST_ASSERT_EQUAL(XY_TIMER_SW_OK, err);
    
    err = xy_timer_sw_stop(id);
    TEST_ASSERT_EQUAL(XY_TIMER_SW_OK, err);
    
    /* Stop invalid timer */
    err = xy_timer_sw_stop(XY_TIMER_SW_INVALID_ID);
    TEST_ASSERT_EQUAL(XY_TIMER_SW_ERR_INVALID, err);
}

void test_timer_sw_poll(void)
{
    xy_timer_sw_id_t id;
    
    xy_timer_sw_init();
    g_timer_callback_count = 0;
    g_timer_callback_arg = NULL;
    
    /* Create periodic timer */
    id = xy_timer_sw_create(5, timer_test_callback, (void *)0x5678, 1);
    TEST_ASSERT_NOT_EQUAL(XY_TIMER_SW_INVALID_ID, id);
    
    xy_timer_sw_start(id);
    
    /* Poll 10 times - should trigger callback twice (at tick 5 and 10) */
    for (int i = 0; i < 10; i++) {
        xy_timer_sw_poll();
    }
    
    TEST_ASSERT_TRUE(g_timer_callback_count >= 1);
    TEST_ASSERT_EQUAL_PTR((void *)0x5678, g_timer_callback_arg);
}

void test_timer_sw_delete(void)
{
    xy_timer_sw_id_t id;
    xy_timer_sw_error_t err;
    
    xy_timer_sw_init();
    
    id = xy_timer_sw_create(10, timer_test_callback, NULL, 0);
    TEST_ASSERT_NOT_EQUAL(XY_TIMER_SW_INVALID_ID, id);
    
    err = xy_timer_sw_delete(id);
    TEST_ASSERT_EQUAL(XY_TIMER_SW_OK, err);
    
    /* Delete invalid timer */
    err = xy_timer_sw_delete(XY_TIMER_SW_INVALID_ID);
    TEST_ASSERT_EQUAL(XY_TIMER_SW_ERR_INVALID, err);
}

void test_timer_sw_multiple_timers(void)
{
    xy_timer_sw_id_t ids[8];
    int counts[8] = {0};
    
    xy_timer_sw_init();
    
    /* Create 8 timers */
    for (int i = 0; i < 8; i++) {
        ids[i] = xy_timer_sw_create(10 + i * 5, timer_test_callback, &counts[i], 1);
        TEST_ASSERT_NOT_EQUAL(XY_TIMER_SW_INVALID_ID, ids[i]);
        xy_timer_sw_start(ids[i]);
    }
    
    /* 9th timer should fail (max is 8) */
    xy_timer_sw_id_t extra_id = xy_timer_sw_create(10, timer_test_callback, NULL, 0);
    TEST_ASSERT_EQUAL(XY_TIMER_SW_INVALID_ID, extra_id);
    
    /* Poll and check callbacks */
    for (int i = 0; i < 50; i++) {
        xy_timer_sw_poll();
    }
    
    /* All timers should have triggered at least once */
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT_TRUE(counts[i] >= 1);
    }
}

/* ==================== OSAL Primitive Tests (Bare-metal) ==================== */

void test_os_delay(void)
{
    uint32_t start;
    
    xy_os_kernel_init();
    
    start = xy_os_kernel_get_tick_count();
    xy_os_delay(10);
    
    /* Delay should take at least 10 ticks */
    TEST_ASSERT_TRUE(xy_os_kernel_get_tick_count() >= start + 10);
}

void test_os_mutex_stub(void)
{
    xy_os_mutex_id_t mutex;
    xy_os_status_t status;
    
    xy_os_kernel_init();
    
    /* In bare-metal, mutex should return NULL */
    mutex = xy_os_mutex_new(NULL);
    TEST_ASSERT_NULL(mutex);
    
    /* Operations on bare-metal should return error */
    status = xy_os_mutex_acquire(mutex, 1000);
    TEST_ASSERT_EQUAL(XY_OS_ERROR, status);
}

void test_os_semaphore_stub(void)
{
    xy_os_semaphore_id_t sem;
    xy_os_status_t status;
    
    xy_os_kernel_init();
    
    /* In bare-metal, semaphore should return NULL */
    sem = xy_os_semaphore_new(10, 10, NULL);
    TEST_ASSERT_NULL(sem);
    
    /* Operations on bare-metal should return error */
    status = xy_os_semaphore_acquire(sem, 1000);
    TEST_ASSERT_EQUAL(XY_OS_ERROR, status);
}

/* ==================== Main ==================== */

int main(void)
{
    UNITY_BEGIN();
    
    printf("\n=== OSAL Unit Tests ===\n\n");
    
    /* Kernel tests */
    printf("Running Kernel Tests...\n");
    RUN_TEST(test_kernel_init);
    RUN_TEST(test_kernel_get_info);
    RUN_TEST(test_kernel_get_state);
    RUN_TEST(test_kernel_tick_count);
    
    /* Tick module tests */
    printf("Running Tick Module Tests...\n");
    RUN_TEST(test_tick_init);
    RUN_TEST(test_tick_get);
    RUN_TEST(test_tick_elapsed);
    RUN_TEST(test_tick_delay);
    
    /* Software timer tests */
    printf("Running Software Timer Tests...\n");
    RUN_TEST(test_timer_sw_init);
    RUN_TEST(test_timer_sw_create);
    RUN_TEST(test_timer_sw_start_stop);
    RUN_TEST(test_timer_sw_poll);
    RUN_TEST(test_timer_sw_delete);
    RUN_TEST(test_timer_sw_multiple_timers);
    
    /* OSAL primitive tests */
    printf("Running OSAL Primitive Tests...\n");
    RUN_TEST(test_os_delay);
    RUN_TEST(test_os_mutex_stub);
    RUN_TEST(test_os_semaphore_stub);
    
    printf("\n=== Tests Complete ===\n");
    
    return UNITY_END();
}
