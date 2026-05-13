/**
 * @file osal_example.c
 * @brief XY OSAL Example Usage
 * @version 1.0.0
 * @date 2026-02-28
 */

#include "xy_os.h"
#include <stdio.h>

/* ==================== Kernel Info Example ==================== */

static void example_kernel_info(void)
{
    xy_os_version_t version;
    char id_buf[32];

    xy_os_kernel_get_info(&version, id_buf, sizeof(id_buf));

    printf("OS Kernel: %s\n", id_buf);
    printf("API Version: %d.%d.%d\n",
           (version.api >> 16) & 0xFF,
           (version.api >> 8) & 0xFF,
           version.api & 0xFF);
}

/* ==================== Delay Example ==================== */

static void example_delay(void)
{
    uint32_t start = xy_os_kernel_get_tick_count();

    printf("Delaying 1000 ticks...\n");
    xy_os_delay(1000);

    uint32_t elapsed = xy_os_kernel_get_tick_count() - start;
    printf("Elapsed: %lu ticks\n", elapsed);
}

/* ==================== Thread Example (RTOS only) ==================== */

#if !defined(XY_OS_BACKEND_BAREMETAL)

static void worker_thread(void *arg)
{
    const char *name = (const char *)arg;

    printf("[Thread %s] Started\n", name);

    while (1) {
        printf("[Thread %s] Running...\n", name);
        xy_os_delay(500);
    }
}

static void example_thread(void)
{
    xy_os_thread_attr_t attr = {
        .name = "worker",
        .priority = XY_OS_PRIORITY_NORMAL,
        .stack_size = 2048,
    };

    xy_os_thread_id_t thread = xy_os_thread_new(worker_thread, (void *)"T1", &attr);

    if (thread) {
        printf("Thread created successfully\n");

        // Get thread info
        printf("Thread name: %s\n", xy_os_thread_get_name(thread));
        printf("Thread state: %d\n", xy_os_thread_get_state(thread));
    } else {
        printf("Failed to create thread\n");
    }
}

#endif /* !XY_OS_BACKEND_BAREMETAL */

/* ==================== Mutex Example ==================== */

#if !defined(XY_OS_BACKEND_BAREMETAL)

static xy_os_mutex_id_t g_mutex;
static int g_shared_counter = 0;

static void mutex_thread(void *arg)
{
    (void)arg;

    for (int i = 0; i < 5; i++) {
        if (xy_os_mutex_acquire(g_mutex, XY_OS_WAIT_FOREVER) == XY_OS_OK) {
            g_shared_counter++;
            printf("Counter: %d\n", g_shared_counter);
            xy_os_mutex_release(g_mutex);
        }
        xy_os_delay(100);
    }
}

static void example_mutex(void)
{
    g_mutex = xy_os_mutex_new(NULL);

    if (!g_mutex) {
        printf("Failed to create mutex\n");
        return;
    }

    printf("Mutex created\n");

    // Create threads that share the counter
    xy_os_thread_attr_t attr = {
        .priority = XY_OS_PRIORITY_NORMAL,
        .stack_size = 1024,
    };

    xy_os_thread_new(mutex_thread, (void *)"T1", &attr);
    xy_os_thread_new(mutex_thread, (void *)"T2", &attr);
}

#endif /* !XY_OS_BACKEND_BAREMETAL */

/* ==================== Semaphore Example ==================== */

#if !defined(XY_OS_BACKEND_BAREMETAL)

static xy_os_semaphore_id_t g_sem;

static void sem_producer(void *arg)
{
    (void)arg;

    for (int i = 0; i < 5; i++) {
        xy_os_delay(200);
        xy_os_semaphore_release(g_sem);
        printf("Produced item %d\n", i);
    }
}

static void sem_consumer(void *arg)
{
    (void)arg;

    for (int i = 0; i < 5; i++) {
        if (xy_os_semaphore_acquire(g_sem, XY_OS_WAIT_FOREVER) == XY_OS_OK) {
            printf("Consumed item %d\n", i);
        }
    }
}

static void example_semaphore(void)
{
    // Create semaphore with max 10, initial 0
    g_sem = xy_os_semaphore_new(10, 0, NULL);

    if (!g_sem) {
        printf("Failed to create semaphore\n");
        return;
    }

    printf("Semaphore created\n");

    xy_os_thread_attr_t attr = {
        .priority = XY_OS_PRIORITY_NORMAL,
        .stack_size = 1024,
    };

    xy_os_thread_new(sem_producer, NULL, &attr);
    xy_os_thread_new(sem_consumer, NULL, &attr);
}

#endif /* !XY_OS_BACKEND_BAREMETAL */

/* ==================== Timer Example ==================== */

static int g_timer_count = 0;

static void timer_callback(void *arg)
{
    (void)arg;
    g_timer_count++;
    printf("Timer expired! Count: %d\n", g_timer_count);

    if (g_timer_count >= 5) {
        printf("Stopping timer\n");
    }
}

static void example_timer(void)
{
    xy_os_timer_attr_t attr = {
        .name = "my_timer",
    };

    xy_os_timer_id_t timer = xy_os_timer_new(
        timer_callback,
        XY_OS_TIMER_PERIODIC,
        NULL,
        &attr
    );

    if (!timer) {
        printf("Failed to create timer\n");
        return;
    }

    printf("Timer created\n");

    xy_os_timer_start(timer, 500);  // 500ms period

    // Let it run for 3 seconds
    xy_os_delay(3000);

    xy_os_timer_stop(timer);
    printf("Timer stopped\n");

    xy_os_timer_delete(timer);
}

/* ==================== Message Queue Example ==================== */

#if !defined(XY_OS_BACKEND_BAREMETAL)

static xy_os_msgqueue_id_t g_queue;

static void queue_sender(void *arg)
{
    (void)arg;

    for (int i = 0; i < 5; i++) {
        int msg = i * 100;
        if (xy_os_msgqueue_put(g_queue, &msg, 0, XY_OS_WAIT_FOREVER) == XY_OS_OK) {
            printf("Sent: %d\n", msg);
        }
        xy_os_delay(150);
    }
}

static void queue_receiver(void *arg)
{
    (void)arg;

    for (int i = 0; i < 5; i++) {
        int msg;
        if (xy_os_msgqueue_get(g_queue, &msg, NULL, XY_OS_WAIT_FOREVER) == XY_OS_OK) {
            printf("Received: %d\n", msg);
        }
    }
}

static void example_message_queue(void)
{
    // Create queue with 10 messages, 4 bytes each
    g_queue = xy_os_msgqueue_new(10, sizeof(int), NULL);

    if (!g_queue) {
        printf("Failed to create queue\n");
        return;
    }

    printf("Queue created (capacity: %lu, msg_size: %lu)\n",
           xy_os_msgqueue_get_capacity(g_queue),
           xy_os_msgqueue_get_msg_size(g_queue));

    xy_os_thread_attr_t attr = {
        .priority = XY_OS_PRIORITY_NORMAL,
        .stack_size = 1024,
    };

    xy_os_thread_new(queue_sender, NULL, &attr);
    xy_os_thread_new(queue_receiver, NULL, &attr);
}

#endif /* !XY_OS_BACKEND_BAREMETAL */

/* ==================== Main Example ==================== */

void osal_example_main(void)
{
    printf("\n=== XY OSAL Example ===\n\n");

    // Initialize kernel
    xy_os_kernel_init();
    printf("Kernel initialized\n");

    // Show kernel info
    example_kernel_info();

    // Test delay
    example_delay();

#if !defined(XY_OS_BACKEND_BAREMETAL)
    // Test timer (works on bare-metal too)
    printf("\n=== Timer Example ===\n");
    example_timer();

    // Test thread
    printf("\n=== Thread Example ===\n");
    example_thread();

    // Test mutex
    printf("\n=== Mutex Example ===\n");
    example_mutex();

    // Test semaphore
    printf("\n=== Semaphore Example ===\n");
    example_semaphore();

    // Test message queue
    printf("\n=== Message Queue Example ===\n");
    example_message_queue();
#endif

    // Start kernel (for RTOS backends)
#if !defined(XY_OS_BACKEND_BAREMETAL)
    printf("\nStarting kernel scheduler...\n");
    xy_os_kernel_start();
#else
    printf("\nBare-metal mode: running main loop\n");

    // Poll software timers
    while (1) {
        xy_timer_sw_poll();
        xy_os_delay(100);
    }
#endif
}

/* ==================== Bare-metal Main ==================== */

#ifdef XY_OS_BACKEND_BAREMETAL

int main(void)
{
    // Initialize tick timer (call from SysTick ISR)
    xy_tick_init(1000);  // 1kHz

    // Initialize software timers
    xy_timer_sw_init();

    // Run example
    osal_example_main();

    return 0;
}

#endif /* XY_OS_BACKEND_BAREMETAL */
