# XY OSAL Quick Start Guide

## 1. Choose Your Backend

| Backend | When to Use |
|---------|-------------|
| **Bare-metal** | No RTOS needed, simple app, minimal code |
| **FreeRTOS** | Industry standard, wide support, mature ecosystem |
| **RT-Thread** | Chinese RTOS, rich features, good documentation |

## 2. Add to Build

### Makefile
```makefile
SRC_FILES += components/osal/baremetal/xy_os_baremetal.c
INCLUDE_DIRS += components/osal
```

### CMake
```cmake
target_sources(${PROJECT_NAME} PRIVATE
    components/osal/baremetal/xy_os_baremetal.c
)
target_include_directories(${PROJECT_NAME} PUBLIC
    components/osal
)
```

## 3. Basic Usage

```c
#include "components/osal/xy_os.h"

int main(void) {
    // Initialize
    xy_os_kernel_init();
    xy_os_kernel_start();

    while (1) {
        // Main loop
        xy_os_delay(1000);  // 1 second delay
    }
}
```

## 4. With RTOS (FreeRTOS/RT-Thread)

```c
#include "components/osal/xy_os.h"

void task1(void *arg) {
    while (1) {
        // Task work
        xy_os_delay(500);
    }
}

int main(void) {
    xy_os_kernel_init();

    // Create task
    xy_os_thread_attr_t attr = {
        .name = "Task1",
        .stack_size = 1024,
        .priority = XY_OS_PRIORITY_NORMAL
    };
    xy_os_thread_new(task1, NULL, &attr);

    // Start scheduler
    xy_os_kernel_start();

    // Never returns
    while (1);
}
```

## 5. Using Synchronization

### Mutex
```c
xy_os_mutex_id_t mutex = xy_os_mutex_new(NULL);

xy_os_mutex_acquire(mutex, XY_OS_WAIT_FOREVER);
// Critical section
xy_os_mutex_release(mutex);
```

### Semaphore
```c
xy_os_semaphore_id_t sem = xy_os_semaphore_new(1, 0, NULL);

// Producer
xy_os_semaphore_release(sem);

// Consumer
xy_os_semaphore_acquire(sem, XY_OS_WAIT_FOREVER);
```

### Message Queue
```c
xy_os_msgqueue_id_t queue = xy_os_msgqueue_new(10, sizeof(uint32_t), NULL);

// Send
uint32_t msg = 42;
xy_os_msgqueue_put(queue, &msg, 0, 100);

// Receive
uint32_t rx_msg;
xy_os_msgqueue_get(queue, &rx_msg, NULL, XY_OS_WAIT_FOREVER);
```

## That's It

Same code runs on bare-metal, FreeRTOS, or RT-Thread. Just switch the backend .c file in your build.

See [IMPLEMENTATION_GUIDE.md](IMPLEMENTATION_GUIDE.md) for complete API reference.
