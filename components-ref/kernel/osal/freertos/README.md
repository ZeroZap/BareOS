# XY OSAL FreeRTOS Backend

## Overview
Complete FreeRTOS implementation of XY OSAL providing full multitasking support.

## Features
✅ Kernel, Tasks, Mutex, Semaphore, Event Groups, Message Queue, Software Timers
⚠️ Memory Pool (stub), Thread Join (not supported)

## Priority Mapping
Direct mapping: XY 0 (lowest) → FreeRTOS 0, capped at `configMAX_PRIORITIES - 1`

## FreeRTOSConfig.h Requirements
```c
#define configUSE_MUTEXES                1
#define configUSE_RECURSIVE_MUTEXES      1
#define configUSE_COUNTING_SEMAPHORES    1
#define configUSE_TIMERS                 1
#define configUSE_TASK_NOTIFICATIONS     1
#define configUSE_EVENT_GROUPS           1
```

## Usage Example
```c
xy_os_thread_attr_t attr = {
    .name = "Task",
    .stack_size = 512 * sizeof(StackType_t),
    .priority = XY_OS_PRIORITY_NORMAL
};
xy_os_thread_id_t task = xy_os_thread_new(my_task_func, NULL, &attr);
```

## Status
Version 1.0.0 | Production Ready | Tested with FreeRTOS 10.4.6
