# XY OSAL Implementation Guide

## Overview

This document provides complete implementation guidance for the three OSAL backends: Bare-metal, RT-Thread, and FreeRTOS.

## Prerequisites

Before implementing, ensure:
1. `xy_os.h` uses standard types (`uint32_t`, `uint8_t`) or includes `xy_typedef.h` properly
2. `xy_tick.h` is available for bare-metal tick access
3. RT-Thread or FreeRTOS headers are in include path for respective backends

## Implementation Structure

Each backend should implement all functions declared in `xy_os.h`:

### File Organization

```
components/osal/
├── xy_os.h              # Public API (already exists)
├── baremetal/
│   └── xy_os_baremetal.c   # Bare-metal implementation
├── rt-thread/
│   └── xy_os_rtthread.c    # RT-Thread implementation
└── freertos/
    └── xy_os_freertos.c    # FreeRTOS implementation
```

---

## 1. Bare-metal Implementation

### Purpose
Minimal OSAL for systems without RTOS. Provides kernel control and delays only.

### Key Implementation Points

**Kernel Functions:**
```c
// State tracking
static xy_os_kernel_state_t s_kernel_state = XY_OS_KERNEL_INACTIVE;
static volatile uint32_t s_kernel_lock_count = 0;

xy_os_status_t xy_os_kernel_init(void) {
    s_kernel_state = XY_OS_KERNEL_READY;
    return XY_OS_OK;
}

xy_os_status_t xy_os_kernel_start(void) {
    s_kernel_state = XY_OS_KERNEL_RUNNING;
    return XY_OS_OK;
}

int32_t xy_os_kernel_lock(void) {
    uint32_t prev = s_kernel_lock_count++;
    if (s_kernel_lock_count == 1) {
        s_kernel_state = XY_OS_KERNEL_LOCKED;
        // TODO: Add platform-specific interrupt disable here
    }
    return prev;
}

uint32_t xy_os_kernel_get_tick_count(void) {
    return xy_tick_get(); // From components/kernel/xy_tick.h
}
```

**Delay Implementation:**
```c
xy_os_status_t xy_os_delay(uint32_t ticks) {
    uint32_t start = xy_tick_get();
    while ((xy_tick_get() - start) < ticks) {
        // Busy wait - could add WFI/sleep here
    }
    return XY_OS_OK;
}
```

**Thread/Sync Stubs:**
All thread, mutex, semaphore, etc. functions return `NULL` or `XY_OS_ERROR`.

---

## 2. RT-Thread Implementation

### Dependencies
```c
#include <rtthread.h>
```

### Priority Mapping
RT-Thread uses inverted priorities (0 = highest), XY OSAL uses normal (56 = highest):

```c
static rt_uint8_t xy_to_rt_priority(xy_os_priority_t xy_prio) {
    // Invert: XY 56 → RT 0, XY 0 → RT 56
    rt_uint8_t rt_prio = (RT_THREAD_PRIORITY_MAX - 1) - ((uint8_t)xy_prio);
    if (rt_prio >= RT_THREAD_PRIORITY_MAX) {
        rt_prio = RT_THREAD_PRIORITY_MAX - 1;
    }
    return rt_prio;
}
```

### Error Code Mapping
```c
static xy_os_status_t rt_err_to_xy(rt_err_t err) {
    switch (err) {
        case RT_EOK: return XY_OS_OK;
        case -RT_ETIMEOUT: return XY_OS_ERROR_TIMEOUT;
        case -RT_ENOMEM: return XY_OS_ERROR_NO_MEMORY;
        case -RT_EINVAL: return XY_OS_ERROR_PARAMETER;
        default: return XY_OS_ERROR;
    }
}
```

### Key Mappings

**Kernel:**
```c
uint32_t xy_os_kernel_get_tick_count(void) {
    return (uint32_t)rt_tick_get();
}

xy_os_status_t xy_os_delay(uint32_t ticks) {
    rt_thread_delay(ticks);
    return XY_OS_OK;
}
```

**Thread:**
```c
xy_os_thread_id_t xy_os_thread_new(xy_os_thread_func_t func, void *argument,
                                   const xy_os_thread_attr_t *attr) {
    const char *name = (attr && attr->name) ? attr->name : "thread";
    uint32_t stack_size = (attr && attr->stack_size) ? attr->stack_size : 1024;
    rt_uint8_t priority = attr ? xy_to_rt_priority(attr->priority) : 16;

    rt_thread_t thread = rt_thread_create(name, func, argument,
                                          stack_size, priority, 10);
    if (thread) {
        rt_thread_startup(thread);
    }
    return (xy_os_thread_id_t)thread;
}

xy_os_status_t xy_os_thread_terminate(xy_os_thread_id_t thread_id) {
    rt_thread_t thread = (rt_thread_t)thread_id;
    rt_err_t err = rt_thread_delete(thread);
    return rt_err_to_xy(err);
}
```

**Mutex:**
```c
xy_os_mutex_id_t xy_os_mutex_new(const xy_os_mutex_attr_t *attr) {
    const char *name = (attr && attr->name) ? attr->name : "mutex";
    rt_uint8_t flag = (attr && (attr->attr_bits & XY_OS_MUTEX_PRIO_INHERIT))
                      ? RT_IPC_FLAG_PRIO : RT_IPC_FLAG_FIFO;
    return (xy_os_mutex_id_t)rt_mutex_create(name, flag);
}

xy_os_status_t xy_os_mutex_acquire(xy_os_mutex_id_t mutex_id, uint32_t timeout) {
    rt_mutex_t mutex = (rt_mutex_t)mutex_id;
    rt_err_t err = rt_mutex_take(mutex, timeout);
    return rt_err_to_xy(err);
}

xy_os_status_t xy_os_mutex_release(xy_os_mutex_id_t mutex_id) {
    rt_mutex_t mutex = (rt_mutex_t)mutex_id;
    rt_err_t err = rt_mutex_release(mutex);
    return rt_err_to_xy(err);
}
```

**Semaphore:**
```c
xy_os_semaphore_id_t xy_os_semaphore_new(uint32_t max_count, uint32_t initial_count,
                                         const xy_os_semaphore_attr_t *attr) {
    const char *name = (attr && attr->name) ? attr->name : "sem";
    return (xy_os_semaphore_id_t)rt_sem_create(name, initial_count, RT_IPC_FLAG_FIFO);
}

xy_os_status_t xy_os_semaphore_acquire(xy_os_semaphore_id_t semaphore_id, uint32_t timeout) {
    rt_sem_t sem = (rt_sem_t)semaphore_id;
    rt_err_t err = rt_sem_take(sem, timeout);
    return rt_err_to_xy(err);
}
```

**Event Flags:**
```c
xy_os_event_flags_id_t xy_os_event_flags_new(const xy_os_event_flags_attr_t *attr) {
    const char *name = (attr && attr->name) ? attr->name : "event";
    return (xy_os_event_flags_id_t)rt_event_create(name, RT_IPC_FLAG_FIFO);
}

uint32_t xy_os_event_flags_wait(xy_os_event_flags_id_t ef_id, uint32_t flags,
                                uint32_t options, uint32_t timeout) {
    rt_event_t event = (rt_event_t)ef_id;
    rt_uint8_t opt = (options & XY_OS_FLAGS_WAIT_ALL) ? RT_EVENT_FLAG_AND : RT_EVENT_FLAG_OR;
    if (!(options & XY_OS_FLAGS_NO_CLEAR)) {
        opt |= RT_EVENT_FLAG_CLEAR;
    }

    rt_uint32_t recved;
    rt_err_t err = rt_event_recv(event, flags, opt, timeout, &recved);
    return (err == RT_EOK) ? recved : 0x80000000;
}
```

**Message Queue:**
```c
xy_os_msgqueue_id_t xy_os_msgqueue_new(uint32_t msg_count, uint32_t msg_size,
                                       const xy_os_msgqueue_attr_t *attr) {
    const char *name = (attr && attr->name) ? attr->name : "mq";
    return (xy_os_msgqueue_id_t)rt_mq_create(name, msg_size, msg_count, RT_IPC_FLAG_FIFO);
}

xy_os_status_t xy_os_msgqueue_put(xy_os_msgqueue_id_t mq_id, const void *msg_ptr,
                                  uint8_t msg_prio, uint32_t timeout) {
    rt_mq_t mq = (rt_mq_t)mq_id;
    // RT-Thread MQ doesn't support priority
    rt_err_t err = rt_mq_send_wait(mq, msg_ptr, mq->msg_size, timeout);
    return rt_err_to_xy(err);
}
```

**Memory Pool:**
```c
xy_os_mempool_id_t xy_os_mempool_new(uint32_t block_count, uint32_t block_size,
                                     const xy_os_mempool_attr_t *attr) {
    const char *name = (attr && attr->name) ? attr->name : "pool";
    return (xy_os_mempool_id_t)rt_mp_create(name, block_count, block_size);
}

void *xy_os_mempool_alloc(xy_os_mempool_id_t mp_id, uint32_t timeout) {
    rt_mp_t mp = (rt_mp_t)mp_id;
    return rt_mp_alloc(mp, timeout);
}
```

**Timer:**
```c
xy_os_timer_id_t xy_os_timer_new(xy_os_timer_func_t func, xy_os_timer_type_t type,
                                 void *argument, const xy_os_timer_attr_t *attr) {
    const char *name = (attr && attr->name) ? attr->name : "timer";
    rt_uint8_t flag = (type == XY_OS_TIMER_PERIODIC)
                      ? (RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_SOFT_TIMER)
                      : (RT_TIMER_FLAG_ONE_SHOT | RT_TIMER_FLAG_SOFT_TIMER);

    return (xy_os_timer_id_t)rt_timer_create(name, func, argument, 10, flag);
}

xy_os_status_t xy_os_timer_start(xy_os_timer_id_t timer_id, uint32_t ticks) {
    rt_timer_t timer = (rt_timer_t)timer_id;
    rt_timer_control(timer, RT_TIMER_CTRL_SET_TIME, &ticks);
    rt_err_t err = rt_timer_start(timer);
    return rt_err_to_xy(err);
}
```

---

## 3. FreeRTOS Implementation

### Dependencies
```c
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "event_groups.h"
#include "timers.h"
```

### Priority Mapping
FreeRTOS: 0 = lowest, higher number = higher priority (same as XY OSAL)

```c
static UBaseType_t xy_to_freertos_priority(xy_os_priority_t xy_prio) {
    // Map XY 0-56 to FreeRTOS 0-configMAX_PRIORITIES
    UBaseType_t prio = (UBaseType_t)xy_prio;
    if (prio >= configMAX_PRIORITIES) {
        prio = configMAX_PRIORITIES - 1;
    }
    return prio;
}
```

### Error Mapping
```c
static xy_os_status_t pdstatus_to_xy(BaseType_t status) {
    return (status == pdPASS) ? XY_OS_OK : XY_OS_ERROR;
}
```

### Key Mappings

**Kernel:**
```c
uint32_t xy_os_kernel_get_tick_count(void) {
    return (uint32_t)xTaskGetTickCount();
}

xy_os_status_t xy_os_delay(uint32_t ticks) {
    vTaskDelay((TickType_t)ticks);
    return XY_OS_OK;
}

int32_t xy_os_kernel_lock(void) {
    vTaskSuspendAll();
    return 0;
}

int32_t xy_os_kernel_unlock(void) {
    xTaskResumeAll();
    return 0;
}
```

**Thread (Task):**
```c
xy_os_thread_id_t xy_os_thread_new(xy_os_thread_func_t func, void *argument,
                                   const xy_os_thread_attr_t *attr) {
    const char *name = (attr && attr->name) ? attr->name : "task";
    uint16_t stack_size = (attr && attr->stack_size) ? (attr->stack_size / sizeof(StackType_t)) : 256;
    UBaseType_t priority = attr ? xy_to_freertos_priority(attr->priority) : tskIDLE_PRIORITY + 1;

    TaskHandle_t handle;
    BaseType_t result = xTaskCreate(func, name, stack_size, argument, priority, &handle);
    return (result == pdPASS) ? (xy_os_thread_id_t)handle : NULL;
}

xy_os_status_t xy_os_thread_terminate(xy_os_thread_id_t thread_id) {
    TaskHandle_t handle = (TaskHandle_t)thread_id;
    vTaskDelete(handle);
    return XY_OS_OK;
}

xy_os_status_t xy_os_thread_yield(void) {
    taskYIELD();
    return XY_OS_OK;
}
```

**Mutex:**
```c
xy_os_mutex_id_t xy_os_mutex_new(const xy_os_mutex_attr_t *attr) {
    SemaphoreHandle_t mutex;
    if (attr && (attr->attr_bits & XY_OS_MUTEX_RECURSIVE)) {
        mutex = xSemaphoreCreateRecursiveMutex();
    } else {
        mutex = xSemaphoreCreateMutex();
    }
    return (xy_os_mutex_id_t)mutex;
}

xy_os_status_t xy_os_mutex_acquire(xy_os_mutex_id_t mutex_id, uint32_t timeout) {
    SemaphoreHandle_t mutex = (SemaphoreHandle_t)mutex_id;
    TickType_t ticks = (timeout == XY_OS_WAIT_FOREVER) ? portMAX_DELAY : (TickType_t)timeout;
    BaseType_t result = xSemaphoreTake(mutex, ticks);
    return pdstatus_to_xy(result);
}

xy_os_status_t xy_os_mutex_release(xy_os_mutex_id_t mutex_id) {
    SemaphoreHandle_t mutex = (SemaphoreHandle_t)mutex_id;
    BaseType_t result = xSemaphoreGive(mutex);
    return pdstatus_to_xy(result);
}
```

**Semaphore:**
```c
xy_os_semaphore_id_t xy_os_semaphore_new(uint32_t max_count, uint32_t initial_count,
                                         const xy_os_semaphore_attr_t *attr) {
    SemaphoreHandle_t sem;
    if (max_count == 1) {
        sem = xSemaphoreCreateBinary();
        if (sem && initial_count > 0) {
            xSemaphoreGive(sem);
        }
    } else {
        sem = xSemaphoreCreateCounting((UBaseType_t)max_count, (UBaseType_t)initial_count);
    }
    return (xy_os_semaphore_id_t)sem;
}

xy_os_status_t xy_os_semaphore_acquire(xy_os_semaphore_id_t semaphore_id, uint32_t timeout) {
    SemaphoreHandle_t sem = (SemaphoreHandle_t)semaphore_id;
    TickType_t ticks = (timeout == XY_OS_WAIT_FOREVER) ? portMAX_DELAY : (TickType_t)timeout;
    BaseType_t result = xSemaphoreTake(sem, ticks);
    return pdstatus_to_xy(result);
}
```

**Event Flags (Event Groups):**
```c
xy_os_event_flags_id_t xy_os_event_flags_new(const xy_os_event_flags_attr_t *attr) {
    return (xy_os_event_flags_id_t)xEventGroupCreate();
}

uint32_t xy_os_event_flags_set(xy_os_event_flags_id_t ef_id, uint32_t flags) {
    EventGroupHandle_t eg = (EventGroupHandle_t)ef_id;
    return (uint32_t)xEventGroupSetBits(eg, (EventBits_t)flags);
}

uint32_t xy_os_event_flags_wait(xy_os_event_flags_id_t ef_id, uint32_t flags,
                                uint32_t options, uint32_t timeout) {
    EventGroupHandle_t eg = (EventGroupHandle_t)ef_id;
    BaseType_t wait_all = (options & XY_OS_FLAGS_WAIT_ALL) ? pdTRUE : pdFALSE;
    BaseType_t clear = (options & XY_OS_FLAGS_NO_CLEAR) ? pdFALSE : pdTRUE;
    TickType_t ticks = (timeout == XY_OS_WAIT_FOREVER) ? portMAX_DELAY : (TickType_t)timeout;

    EventBits_t bits = xEventGroupWaitBits(eg, (EventBits_t)flags, clear, wait_all, ticks);
    return (uint32_t)bits;
}
```

**Message Queue:**
```c
xy_os_msgqueue_id_t xy_os_msgqueue_new(uint32_t msg_count, uint32_t msg_size,
                                       const xy_os_msgqueue_attr_t *attr) {
    return (xy_os_msgqueue_id_t)xQueueCreate((UBaseType_t)msg_count, (UBaseType_t)msg_size);
}

xy_os_status_t xy_os_msgqueue_put(xy_os_msgqueue_id_t mq_id, const void *msg_ptr,
                                  uint8_t msg_prio, uint32_t timeout) {
    QueueHandle_t queue = (QueueHandle_t)mq_id;
    TickType_t ticks = (timeout == XY_OS_WAIT_FOREVER) ? portMAX_DELAY : (TickType_t)timeout;
    // FreeRTOS queue doesn't support priority unless using priority queue
    BaseType_t result = xQueueSendToBack(queue, msg_ptr, ticks);
    return pdstatus_to_xy(result);
}

xy_os_status_t xy_os_msgqueue_get(xy_os_msgqueue_id_t mq_id, void *msg_ptr,
                                  uint8_t *msg_prio, uint32_t timeout) {
    QueueHandle_t queue = (QueueHandle_t)mq_id;
    TickType_t ticks = (timeout == XY_OS_WAIT_FOREVER) ? portMAX_DELAY : (TickType_t)timeout;
    if (msg_prio) *msg_prio = 0;
    BaseType_t result = xQueueReceive(queue, msg_ptr, ticks);
    return pdstatus_to_xy(result);
}
```

**Timer:**
```c
xy_os_timer_id_t xy_os_timer_new(xy_os_timer_func_t func, xy_os_timer_type_t type,
                                 void *argument, const xy_os_timer_attr_t *attr) {
    const char *name = (attr && attr->name) ? attr->name : "timer";
    UBaseType_t auto_reload = (type == XY_OS_TIMER_PERIODIC) ? pdTRUE : pdFALSE;

    return (xy_os_timer_id_t)xTimerCreate(name, 1, auto_reload, argument,
                                          (TimerCallbackFunction_t)func);
}

xy_os_status_t xy_os_timer_start(xy_os_timer_id_t timer_id, uint32_t ticks) {
    TimerHandle_t timer = (TimerHandle_t)timer_id;
    xTimerChangePeriod(timer, (TickType_t)ticks, 0);
    BaseType_t result = xTimerStart(timer, 0);
    return pdstatus_to_xy(result);
}
```

**Memory Pool (Emulated):**
FreeRTOS doesn't have native memory pools. Implement using heap allocation or create custom pool.

```c
// Simple wrapper using pvPortMalloc/vPortFree
void *xy_os_mempool_alloc(xy_os_mempool_id_t mp_id, uint32_t timeout) {
    // Custom implementation needed
    return pvPortMalloc(/* block_size */);
}
```

---

## Build Integration

### Makefile Example

```makefile
# Select backend
OSAL_BACKEND = baremetal  # or rt-thread or freertos

# Add source
SRC_FILES += components/osal/$(OSAL_BACKEND)/xy_os_$(OSAL_BACKEND).c

# Add includes
INCLUDE_DIRS += components/osal
INCLUDE_DIRS += components/kernel

# For RT-Thread
ifeq ($(OSAL_BACKEND),rt-thread)
    INCLUDE_DIRS += rt-thread/include
    CFLAGS += -DRT_USING_HOOK
endif

# For FreeRTOS
ifeq ($(OSAL_BACKEND),freertos)
    INCLUDE_DIRS += FreeRTOS/Source/include
    INCLUDE_DIRS += FreeRTOS/Source/portable/GCC/ARM_CM4F
endif
```

### CMake Example

```cmake
set(OSAL_BACKEND "baremetal" CACHE STRING "OSAL backend")

if(OSAL_BACKEND STREQUAL "baremetal")
    target_sources(${PROJECT_NAME} PRIVATE
        components/osal/baremetal/xy_os_baremetal.c
    )
elseif(OSAL_BACKEND STREQUAL "rt-thread")
    target_sources(${PROJECT_NAME} PRIVATE
        components/osal/rt-thread/xy_os_rtthread.c
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE rtthread)
elseif(OSAL_BACKEND STREQUAL "freertos")
    target_sources(${PROJECT_NAME} PRIVATE
        components/osal/freertos/xy_os_freertos.c
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE freertos_kernel)
endif()
```

---

## Testing

Each implementation should pass the same test suite:

```c
void test_osal(void) {
    // Kernel
    xy_os_kernel_init();
    xy_os_kernel_start();
    assert(xy_os_kernel_get_tick_count() >= 0);

    // Delay
    uint32_t start = xy_os_kernel_get_tick_count();
    xy_os_delay(100);
    assert((xy_os_kernel_get_tick_count() - start) >= 100);

    // Mutex
    xy_os_mutex_id_t mutex = xy_os_mutex_new(NULL);
    assert(mutex != NULL);
    assert(xy_os_mutex_acquire(mutex, 100) == XY_OS_OK);
    assert(xy_os_mutex_release(mutex) == XY_OS_OK);
    xy_os_mutex_delete(mutex);

    // Semaphore
    xy_os_semaphore_id_t sem = xy_os_semaphore_new(1, 0, NULL);
    assert(sem != NULL);
    xy_os_semaphore_release(sem);
    assert(xy_os_semaphore_acquire(sem, 100) == XY_OS_OK);
    xy_os_semaphore_delete(sem);

    // More tests...
}
```

---

## Summary

All three backends provide the same API from `xy_os.h`, allowing applications to be portable across bare-metal, RT-Thread, and FreeRTOS without code changes. Choose the backend based on system requirements and complexity.
