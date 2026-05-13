/**
 * @file xy_os_cmsis_rtx.c
 * @brief XY OSAL CMSIS-RTX Implementation
 * @version 2.0
 * @date 2026-02-28
 */

#include "../inc/xy_os.h"

#if defined(STM32U5) || defined(STM32U5xx)

#include "cmsis_os2.h"
#include <string.h>

/* Context structures for CMSIS-RTX */
typedef struct {
    osThreadId_t thread_id;
    osThreadAttr_t attr;
    xy_os_callback_t callback;
    void *arg;
    uint8_t initialized;
} thread_ctx_t;

typedef struct {
    osMutexId_t mutex_id;
    osMutexAttr_t attr;
    uint8_t initialized;
} mutex_ctx_t;

typedef struct {
    osSemaphoreId_t sem_id;
    uint32_t max_count;
    uint32_t initial_count;
    uint8_t initialized;
} semaphore_ctx_t;

typedef struct {
    osTimerId_t timer_id;
    osTimerAttr_t attr;
    xy_os_callback_t callback;
    void *arg;
    uint8_t initialized;
} timer_ctx_t;

/* Maximum instances */
#define MAX_THREADS 16
#define MAX_MUTEXES 8
#define MAX_SEMAPHORES 8
#define MAX_TIMERS 8

static thread_ctx_t g_thread_ctx[MAX_THREADS];
static mutex_ctx_t g_mutex_ctx[MAX_MUTEXES];
static semaphore_ctx_t g_semaphore_ctx[MAX_SEMAPHORES];
static timer_ctx_t g_timer_ctx[MAX_TIMERS];

/**
 * @brief Find thread context by handle
 */
static thread_ctx_t *find_thread_ctx(osThreadId_t id)
{
    for (size_t i = 0; i < MAX_THREADS; i++) {
        if (g_thread_ctx[i].thread_id == id) {
            return &g_thread_ctx[i];
        }
    }
    return NULL;
}

/**
 * @brief Allocate new thread context
 */
static thread_ctx_t *alloc_thread_ctx(void)
{
    for (size_t i = 0; i < MAX_THREADS; i++) {
        if (g_thread_ctx[i].thread_id == NULL) {
            return &g_thread_ctx[i];
        }
    }
    return NULL;
}

/**
 * @brief Convert XY priority to CMSIS-RTX
 */
static osPriority_t xy_to_cmsis_priority(xy_os_priority_t priority)
{
    // Convert XinYi priority (0-56) to CMSIS priority (osPriorityLow-osPriorityISR)
    if (priority <= 14) {
        return osPriorityLow;
    } else if (priority <= 28) {
        return osPriorityBelowNormal;
    } else if (priority <= 42) {
        return osPriorityNormal;
    } else if (priority <= 50) {
        return osPriorityAboveNormal;
    } else {
        return osPriorityHigh;
    }
}

/**
 * @brief Convert CMSIS-RTX priority to XY
 */
static xy_os_priority_t cmsis_to_xy_priority(osPriority_t priority)
{
    switch (priority) {
    case osPriorityLow:
        return 7;
    case osPriorityBelowNormal:
        return 21;
    case osPriorityNormal:
        return 35;
    case osPriorityAboveNormal:
        return 46;
    case osPriorityHigh:
        return 54;
    default:
        return 28; // Default to normal
    }
}

/* Kernel Functions */

xy_os_error_t xy_os_kernel_init(void)
{
    // CMSIS-RTOS2 kernel init is usually handled by RTX5 init
    return XY_OS_OK;
}

xy_os_error_t xy_os_kernel_get_info(xy_os_version_t *version, char *id_buf,
                                    uint32_t id_size)
{
    if (version) {
        version->api    = osKernelGetVersion().api;
        version->kernel = osKernelGetVersion().kernel;
    }
    if (id_buf && id_size > 0) {
        const char *kernel_id = "CMSIS-RTX5";
        strncpy(id_buf, kernel_id, id_size - 1);
        id_buf[id_size - 1] = '\0';
    }
    return XY_OS_OK;
}

xy_os_kernel_state_t xy_os_kernel_get_state(void)
{
    switch (osKernelGetState()) {
    case osKernelInactive:
        return XY_OS_KERNEL_INACTIVE;
    case osKernelReady:
        return XY_OS_KERNEL_READY;
    case osKernelRunning:
        return XY_OS_KERNEL_RUNNING;
    case osKernelLocked:
        return XY_OS_KERNEL_LOCKED;
    case osKernelSuspended:
        return XY_OS_KERNEL_SUSPENDED;
    default:
        return XY_OS_KERNEL_ERROR;
    }
}

xy_os_error_t xy_os_kernel_start(void)
{
    if (osKernelInitialize() != osOK) {
        return XY_OS_ERROR_FAIL;
    }
    
    if (osKernelStart() != osOK) {
        return XY_OS_ERROR_FAIL;
    }
    
    return XY_OS_OK;
}

int32_t xy_os_kernel_lock(void)
{
    return (int32_t)osKernelLock();
}

int32_t xy_os_kernel_unlock(void)
{
    return (int32_t)osKernelUnlock();
}

int32_t xy_os_kernel_restore_lock(int32_t lock)
{
    return (int32_t)osKernelRestoreLock(lock);
}

uint32_t xy_os_kernel_get_tick_count(void)
{
    return osKernelGetTickCount();
}

uint32_t xy_os_kernel_get_tick_freq(void)
{
    return osKernelGetTickFreq();
}

uint32_t xy_os_kernel_get_sys_timer_count(void)
{
    return osKernelGetTickCount(); // Use same as tick count
}

uint32_t xy_os_kernel_get_sys_timer_freq(void)
{
    return osKernelGetTickFreq();
}

/* Thread Functions */

xy_os_thread_id_t xy_os_thread_new(xy_os_thread_func_t func, void *argument,
                                   const xy_os_thread_attr_t *attr)
{
    if (!func) {
        return NULL;
    }

    osThreadAttr_t thread_attr = { 0 };
    if (attr) {
        thread_attr.name = attr->name;
        thread_attr.attr_bits = attr->attr_bits;
        thread_attr.cb_mem = attr->cb_mem;
        thread_attr.cb_size = attr->cb_size;
        thread_attr.stack_mem = attr->stack_mem;
        thread_attr.stack_size = attr->stack_size;
        thread_attr.priority = xy_to_cmsis_priority(attr->priority);
        thread_attr.tz_module = attr->tz_module;
        thread_attr.reserved = attr->reserved;
    } else {
        thread_attr.priority = osPriorityNormal;
        thread_attr.stack_size = 1024; // Default stack size
    }

    osThreadId_t thread_id = osThreadNew((osThreadFunc_t)func, argument, &thread_attr);
    if (thread_id == NULL) {
        return NULL;
    }

    // Allocate context
    thread_ctx_t *ctx = alloc_thread_ctx();
    if (ctx) {
        ctx->thread_id = thread_id;
        ctx->attr = thread_attr;
        ctx->initialized = 1;
    }

    return (xy_os_thread_id_t)thread_id;
}

const char *xy_os_thread_get_name(xy_os_thread_id_t thread_id)
{
    return osThreadGetName((osThreadId_t)thread_id);
}

xy_os_thread_id_t xy_os_thread_get_id(void)
{
    return (xy_os_thread_id_t)osThreadGetId();
}

xy_os_thread_state_t xy_os_thread_get_state(xy_os_thread_id_t thread_id)
{
    switch (osThreadGetState((osThreadId_t)thread_id)) {
    case osThreadInactive:
        return XY_OS_THREAD_INACTIVE;
    case osThreadReady:
        return XY_OS_THREAD_READY;
    case osThreadRunning:
        return XY_OS_THREAD_RUNNING;
    case osThreadBlocked:
        return XY_OS_THREAD_BLOCKED;
    case osThreadTerminated:
        return XY_OS_THREAD_TERMINATED;
    default:
        return XY_OS_THREAD_ERROR;
    }
}

uint32_t xy_os_thread_get_stack_size(xy_os_thread_id_t thread_id)
{
    return osThreadGetStackSize((osThreadId_t)thread_id);
}

uint32_t xy_os_thread_get_stack_space(xy_os_thread_id_t thread_id)
{
    return osThreadGetStackSpace((osThreadId_t)thread_id);
}

xy_os_error_t xy_os_thread_set_priority(xy_os_thread_id_t thread_id,
                                        xy_os_priority_t priority)
{
    osStatus_t status = osThreadSetPriority((osThreadId_t)thread_id,
                                            xy_to_cmsis_priority(priority));
    return (status == osOK) ? XY_OS_OK : XY_OS_ERROR_FAIL;
}

xy_os_priority_t xy_os_thread_get_priority(xy_os_thread_id_t thread_id)
{
    osPriority_t cmsis_prio = osThreadGetPriority((osThreadId_t)thread_id);
    return cmsis_to_xy_priority(cmsis_prio);
}

xy_os_error_t xy_os_thread_yield(void)
{
    return (osThreadYield() == osOK) ? XY_OS_OK : XY_OS_ERROR_FAIL;
}

xy_os_error_t xy_os_thread_suspend(xy_os_thread_id_t thread_id)
{
    osStatus_t status = osThreadSuspend((osThreadId_t)thread_id);
    return (status == osOK) ? XY_OS_OK : XY_OS_ERROR_FAIL;
}

xy_os_error_t xy_os_thread_resume(xy_os_thread_id_t thread_id)
{
    osStatus_t status = osThreadResume((osThreadId_t)thread_id);
    return (status == osOK) ? XY_OS_OK : XY_OS_ERROR_FAIL;
}

xy_os_error_t xy_os_thread_detach(xy_os_thread_id_t thread_id)
{
    // CMSIS-RTOS2 doesn't have detach concept, return OK
    XY_UNUSED(thread_id);
    return XY_OS_OK;
}

xy_os_error_t xy_os_thread_join(xy_os_thread_id_t thread_id)
{
    // CMSIS-RTOS2 doesn't have join concept, return OK
    XY_UNUSED(thread_id);
    return XY_OS_OK;
}

void xy_os_thread_exit(void)
{
    osThreadExit();
}

xy_os_error_t xy_os_thread_terminate(xy_os_thread_id_t thread_id)
{
    osStatus_t status = osThreadTerminate((osThreadId_t)thread_id);
    return (status == osOK) ? XY_OS_OK : XY_OS_ERROR_FAIL;
}

uint32_t xy_os_thread_get_count(void)
{
    // CMSIS-RTOS2 doesn't provide thread count, return 0
    return 0;
}

uint32_t xy_os_thread_enumerate(xy_os_thread_id_t *thread_array, uint32_t array_items)
{
    // CMSIS-RTOS2 doesn't provide thread enumeration, return 0
    XY_UNUSED(thread_array);
    XY_UNUSED(array_items);
    return 0;
}

/* Thread Flags Functions */

uint32_t xy_os_thread_flags_set(xy_os_thread_id_t thread_id, uint32_t flags)
{
    uint32_t result = osThreadFlagsSet((osThreadId_t)thread_id, flags);
    return (result & 0x80000000) ? 0x80000000 : result;
}

uint32_t xy_os_thread_flags_clear(uint32_t flags)
{
    xy_os_thread_id_t current_thread = xy_os_thread_get_id();
    uint32_t result = osThreadFlagsClear((osThreadId_t)current_thread, flags);
    return (result & 0x80000000) ? 0x80000000 : result;
}

uint32_t xy_os_thread_flags_get(void)
{
    xy_os_thread_id_t current_thread = xy_os_thread_get_id();
    return osThreadFlagsGet((osThreadId_t)current_thread);
}

uint32_t xy_os_thread_flags_wait(uint32_t flags, uint32_t options, uint32_t timeout)
{
    uint32_t opt = 0;
    if (options & XY_OS_FLAGS_WAIT_ALL) opt |= osFlagsWaitAll;
    if (options & XY_OS_FLAGS_NO_CLEAR) opt |= osFlagsNoClear;
    
    uint32_t result = osThreadFlagsWait(flags, opt, timeout);
    return (result & 0x80000000) ? 0x80000000 : result;
}

/* Delay Functions */

xy_os_error_t xy_os_delay(uint32_t ticks)
{
    osStatus_t status = osDelay(ticks);
    return (status == osOK) ? XY_OS_OK : XY_OS_ERROR_FAIL;
}

xy_os_error_t xy_os_delay_until(uint32_t ticks)
{
    uint32_t current = osKernelGetTickCount();
    if (ticks > current) {
        uint32_t delay_ticks = ticks - current;
        return xy_os_delay(delay_ticks);
    }
    return XY_OS_OK;
}

/* Timer Functions */

xy_os_timer_id_t xy_os_timer_new(xy_os_timer_func_t func, xy_os_timer_type_t type,
                                 void *argument, const xy_os_timer_attr_t *attr)
{
    osTimerType_t cmsis_type = (type == XY_OS_TIMER_PERIODIC) ? 
                               osTimerPeriodic : osTimerOnce;
    
    osTimerAttr_t timer_attr = { 0 };
    if (attr) {
        timer_attr.name = attr->name;
        timer_attr.attr_bits = attr->attr_bits;
        timer_attr.cb_mem = attr->cb_mem;
        timer_attr.cb_size = attr->cb_size;
    }
    
    osTimerId_t timer_id = osTimerNew((osTimerFunc_t)func, cmsis_type, argument, &timer_attr);
    if (timer_id == NULL) {
        return NULL;
    }
    
    // Find available context
    for (size_t i = 0; i < MAX_TIMERS; i++) {
        if (g_timer_ctx[i].timer_id == NULL) {
            g_timer_ctx[i].timer_id = timer_id;
            g_timer_ctx[i].attr = timer_attr;
            g_timer_ctx[i].callback = NULL;
            g_timer_ctx[i].arg = argument;
            g_timer_ctx[i].initialized = 1;
            return (xy_os_timer_id_t)timer_id;
        }
    }
    
    osTimerDelete(timer_id);
    return NULL;
}

const char *xy_os_timer_get_name(xy_os_timer_id_t timer_id)
{
    return osTimerGetName((osTimerId_t)timer_id);
}

xy_os_error_t xy_os_timer_start(xy_os_timer_id_t timer_id, uint32_t ticks)
{
    osStatus_t status = osTimerStart((osTimerId_t)timer_id, ticks);
    return (status == osOK) ? XY_OS_OK : XY_OS_ERROR_FAIL;
}

xy_os_error_t xy_os_timer_stop(xy_os_timer_id_t timer_id)
{
    osStatus_t status = osTimerStop((osTimerId_t)timer_id);
    return (status == osOK) ? XY_OS_OK : XY_OS_ERROR_FAIL;
}

uint32_t xy_os_timer_is_running(xy_os_timer_id_t timer_id)
{
    return osTimerIsRunning((osTimerId_t)timer_id);
}

xy_os_error_t xy_os_timer_delete(xy_os_timer_id_t timer_id)
{
    osStatus_t status = osTimerDelete((osTimerId_t)timer_id);
    if (status == osOK) {
        // Clean up context
        for (size_t i = 0; i < MAX_TIMERS; i++) {
            if (g_timer_ctx[i].timer_id == (osTimerId_t)timer_id) {
                g_timer_ctx[i].initialized = 0;
                g_timer_ctx[i].timer_id = NULL;
                break;
            }
        }
    }
    return (status == osOK) ? XY_OS_OK : XY_OS_ERROR_FAIL;
}

/* Event Flags Functions */

xy_os_event_flags_id_t xy_os_event_flags_new(const xy_os_event_flags_attr_t *attr)
{
    osEventFlagsAttr_t event_attr = { 0 };
    if (attr) {
        event_attr.name = attr->name;
        event_attr.attr_bits = attr->attr_bits;
        event_attr.cb_mem = attr->cb_mem;
        event_attr.cb_size = attr->cb_size;
    }
    
    osEventFlagsId_t event_id = osEventFlagsNew(&event_attr);
    return (xy_os_event_flags_id_t)event_id;
}

const char *xy_os_event_flags_get_name(xy_os_event_flags_id_t ef_id)
{
    return osEventFlagsGetName((osEventFlagsId_t)ef_id);
}

uint32_t xy_os_event_flags_set(xy_os_event_flags_id_t ef_id, uint32_t flags)
{
    uint32_t result = osEventFlagsSet((osEventFlagsId_t)ef_id, flags);
    return (result & 0x80000000) ? 0x80000000 : result;
}

uint32_t xy_os_event_flags_clear(xy_os_event_flags_id_t ef_id, uint32_t flags)
{
    uint32_t result = osEventFlagsClear((osEventFlagsId_t)ef_id, flags);
    return (result & 0x80000000) ? 0x80000000 : result;
}

uint32_t xy_os_event_flags_get(xy_os_event_flags_id_t ef_id)
{
    return osEventFlagsGet((osEventFlagsId_t)ef_id);
}

uint32_t xy_os_event_flags_wait(xy_os_event_flags_id_t ef_id, uint32_t flags,
                                uint32_t options, uint32_t timeout)
{
    uint32_t opt = 0;
    if (options & XY_OS_FLAGS_WAIT_ALL) opt |= osFlagsWaitAll;
    if (options & XY_OS_FLAGS_NO_CLEAR) opt |= osFlagsNoClear;
    
    uint32_t result = osEventFlagsWait((osEventFlagsId_t)ef_id, flags, opt, timeout);
    return (result & 0x80000000) ? 0x80000000 : result;
}

xy_os_error_t xy_os_event_flags_delete(xy_os_event_flags_id_t ef_id)
{
    osStatus_t status = osEventFlagsDelete((osEventFlagsId_t)ef_id);
    return (status == osOK) ? XY_OS_OK : XY_OS_ERROR_FAIL;
}

/* Mutex Functions */

xy_os_mutex_id_t xy_os_mutex_new(const xy_os_mutex_attr_t *attr)
{
    osMutexAttr_t mutex_attr = { 0 };
    if (attr) {
        mutex_attr.name = attr->name;
        mutex_attr.attr_bits = attr->attr_bits;
        mutex_attr.cb_mem = attr->cb_mem;
        mutex_attr.cb_size = attr->cb_size;
    } else {
        mutex_attr.attr_bits = osMutexPrioInherit; // Default priority inheritance
    }
    
    osMutexId_t mutex_id = osMutexNew(&mutex_attr);
    return (xy_os_mutex_id_t)mutex_id;
}

const char *xy_os_mutex_get_name(xy_os_mutex_id_t mutex_id)
{
    return osMutexGetName((osMutexId_t)mutex_id);
}

xy_os_error_t xy_os_mutex_acquire(xy_os_mutex_id_t mutex_id, uint32_t timeout)
{
    osStatus_t status = osMutexAcquire((osMutexId_t)mutex_id, timeout);
    return (status == osOK) ? XY_OS_OK : XY_OS_ERROR_FAIL;
}

xy_os_error_t xy_os_mutex_release(xy_os_mutex_id_t mutex_id)
{
    osStatus_t status = osMutexRelease((osMutexId_t)mutex_id);
    return (status == osOK) ? XY_OS_OK : XY_OS_ERROR_FAIL;
}

xy_os_thread_id_t xy_os_mutex_get_owner(xy_os_mutex_id_t mutex_id)
{
    osThreadId_t owner = osMutexGetOwner((osMutexId_t)mutex_id);
    return (xy_os_thread_id_t)owner;
}

xy_os_error_t xy_os_mutex_delete(xy_os_mutex_id_t mutex_id)
{
    osStatus_t status = osMutexDelete((osMutexId_t)mutex_id);
    return (status == osOK) ? XY_OS_OK : XY_OS_ERROR_FAIL;
}

/* Semaphore Functions */

xy_os_semaphore_id_t xy_os_semaphore_new(uint32_t max_count, uint32_t initial_count,
                                         const xy_os_semaphore_attr_t *attr)
{
    osSemaphoreAttr_t sem_attr = { 0 };
    if (attr) {
        sem_attr.name = attr->name;
        sem_attr.attr_bits = attr->attr_bits;
        sem_attr.cb_mem = attr->cb_mem;
        sem_attr.cb_size = attr->cb_size;
    }
    
    osSemaphoreId_t sem_id = osSemaphoreNew(max_count, initial_count, &sem_attr);
    if (sem_id == NULL) {
        return NULL;
    }
    
    // Find available context
    for (size_t i = 0; i < MAX_SEMAPHORES; i++) {
        if (g_semaphore_ctx[i].sem_id == NULL) {
            g_semaphore_ctx[i].sem_id = sem_id;
            g_semaphore_ctx[i].max_count = max_count;
            g_semaphore_ctx[i].initial_count = initial_count;
            g_semaphore_ctx[i].initialized = 1;
            return (xy_os_semaphore_id_t)sem_id;
        }
    }
    
    osSemaphoreDelete(sem_id);
    return NULL;
}

const char *xy_os_semaphore_get_name(xy_os_semaphore_id_t semaphore_id)
{
    return osSemaphoreGetName((osSemaphoreId_t)semaphore_id);
}

xy_os_error_t xy_os_semaphore_acquire(xy_os_semaphore_id_t semaphore_id, uint32_t timeout)
{
    osStatus_t status = osSemaphoreAcquire((osSemaphoreId_t)semaphore_id, timeout);
    return (status == osOK) ? XY_OS_OK : XY_OS_ERROR_FAIL;
}

xy_os_error_t xy_os_semaphore_release(xy_os_semaphore_id_t semaphore_id)
{
    osStatus_t status = osSemaphoreRelease((osSemaphoreId_t)semaphore_id);
    return (status == osOK) ? XY_OS_OK : XY_OS_ERROR_FAIL;
}

uint32_t xy_os_semaphore_get_count(xy_os_semaphore_id_t semaphore_id)
{
    return osSemaphoreGetCount((osSemaphoreId_t)semaphore_id);
}

xy_os_error_t xy_os_semaphore_delete(xy_os_semaphore_id_t semaphore_id)
{
    osStatus_t status = osSemaphoreDelete((osSemaphoreId_t)semaphore_id);
    if (status == osOK) {
        // Clean up context
        for (size_t i = 0; i < MAX_SEMAPHORES; i++) {
            if (g_semaphore_ctx[i].sem_id == (osSemaphoreId_t)semaphore_id) {
                g_semaphore_ctx[i].initialized = 0;
                g_semaphore_ctx[i].sem_id = NULL;
                break;
            }
        }
    }
    return (status == osOK) ? XY_OS_OK : XY_OS_ERROR_FAIL;
}

/* Memory Pool Functions */

xy_os_mempool_id_t xy_os_mempool_new(uint32_t block_count, uint32_t block_size,
                                     const xy_os_mempool_attr_t *attr)
{
    osMemoryPoolAttr_t mp_attr = { 0 };
    if (attr) {
        mp_attr.name = attr->name;
        mp_attr.attr_bits = attr->attr_bits;
        mp_attr.cb_mem = attr->cb_mem;
        mp_attr.cb_size = attr->cb_size;
        mp_attr.mp_mem = attr->mp_mem;
        mp_attr.mp_size = attr->mp_size;
    }
    
    osMemoryPoolId_t mp_id = osMemoryPoolNew(block_count, block_size, &mp_attr);
    return (xy_os_mempool_id_t)mp_id;
}

const char *xy_os_mempool_get_name(xy_os_mempool_id_t mp_id)
{
    return osMemoryPoolGetName((osMemoryPoolId_t)mp_id);
}

void *xy_os_mempool_alloc(xy_os_mempool_id_t mp_id, uint32_t timeout)
{
    return osMemoryPoolAlloc((osMemoryPoolId_t)mp_id, timeout);
}

xy_os_error_t xy_os_mempool_free(xy_os_mempool_id_t mp_id, void *block)
{
    osStatus_t status = osMemoryPoolFree((osMemoryPoolId_t)mp_id, block);
    return (status == osOK) ? XY_OS_OK : XY_OS_ERROR_FAIL;
}

uint32_t xy_os_mempool_get_capacity(xy_os_mempool_id_t mp_id)
{
    return osMemoryPoolGetCapacity((osMemoryPoolId_t)mp_id);
}

uint32_t xy_os_mempool_get_block_size(xy_os_mempool_id_t mp_id)
{
    return osMemoryPoolGetBlockSize((osMemoryPoolId_t)mp_id);
}

uint32_t xy_os_mempool_get_count(xy_os_mempool_id_t mp_id)
{
    return osMemoryPoolGetCount((osMemoryPoolId_t)mp_id);
}

uint32_t xy_os_mempool_get_space(xy_os_mempool_id_t mp_id)
{
    return osMemoryPoolGetSpace((osMemoryPoolId_t)mp_id);
}

xy_os_error_t xy_os_mempool_delete(xy_os_mempool_id_t mp_id)
{
    osStatus_t status = osMemoryPoolDelete((osMemoryPoolId_t)mp_id);
    return (status == osOK) ? XY_OS_OK : XY_OS_ERROR_FAIL;
}

/* Message Queue Functions */

xy_os_msgqueue_id_t xy_os_msgqueue_new(uint32_t msg_count, uint32_t msg_size,
                                       const xy_os_msgqueue_attr_t *attr)
{
    osMessageQueueAttr_t mq_attr = { 0 };
    if (attr) {
        mq_attr.name = attr->name;
        mq_attr.attr_bits = attr->attr_bits;
        mq_attr.cb_mem = attr->cb_mem;
        mq_attr.cb_size = attr->cb_size;
        mq_attr.mq_mem = attr->mq_mem;
        mq_attr.mq_size = attr->mq_size;
    }
    
    osMessageQueueId_t mq_id = osMessageQueueNew(msg_count, msg_size, &mq_attr);
    return (xy_os_msgqueue_id_t)mq_id;
}

const char *xy_os_msgqueue_get_name(xy_os_msgqueue_id_t mq_id)
{
    return osMessageQueueGetName((osMessageQueueId_t)mq_id);
}

xy_os_error_t xy_os_msgqueue_put(xy_os_msgqueue_id_t mq_id,
                                 const void *msg_ptr, uint8_t msg_prio,
                                 uint32_t timeout)
{
    osStatus_t status = osMessageQueuePut((osMessageQueueId_t)mq_id, msg_ptr,
                                          msg_prio, timeout);
    return (status == osOK) ? XY_OS_OK : XY_OS_ERROR_FAIL;
}

xy_os_error_t xy_os_msgqueue_get(xy_os_msgqueue_id_t mq_id, void *msg_ptr,
                                 uint8_t *msg_prio, uint32_t timeout)
{
    osStatus_t status = osMessageQueueGet((osMessageQueueId_t)mq_id, msg_ptr,
                                          msg_prio, timeout);
    return (status == osOK) ? XY_OS_OK : XY_OS_ERROR_FAIL;
}

uint32_t xy_os_msgqueue_get_capacity(xy_os_msgqueue_id_t mq_id)
{
    return osMessageQueueGetCapacity((osMessageQueueId_t)mq_id);
}

uint32_t xy_os_msgqueue_get_msg_size(xy_os_msgqueue_id_t mq_id)
{
    return osMessageQueueGetMsgSize((osMessageQueueId_t)mq_id);
}

uint32_t xy_os_msgqueue_get_count(xy_os_msgqueue_id_t mq_id)
{
    return osMessageQueueGetCount((osMessageQueueId_t)mq_id);
}

uint32_t xy_os_msgqueue_get_space(xy_os_msgqueue_id_t mq_id)
{
    return osMessageQueueGetSpace((osMessageQueueId_t)mq_id);
}

xy_os_error_t xy_os_msgqueue_reset(xy_os_msgqueue_id_t mq_id)
{
    osStatus_t status = osMessageQueueReset((osMessageQueueId_t)mq_id);
    return (status == osOK) ? XY_OS_OK : XY_OS_ERROR_FAIL;
}

xy_os_error_t xy_os_msgqueue_delete(xy_os_msgqueue_id_t mq_id)
{
    osStatus_t status = osMessageQueueDelete((osMessageQueueId_t)mq_id);
    return (status == osOK) ? XY_OS_OK : XY_OS_ERROR_FAIL;
}

#endif /* STM32U5 || STM32U5xx */
