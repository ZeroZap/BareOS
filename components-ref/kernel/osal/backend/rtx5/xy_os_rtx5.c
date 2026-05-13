/**
 * @file xy_os_rtx5.c
 * @brief XY OSAL ARM RTX5 (CMSIS-RTOS2) Backend Implementation
 * @version 1.0.0
 * @date 2026-03-15
 * 
 * @note ARM RTX5 is a CMSIS-RTOS2 compliant RTOS for ARM Cortex-M/RISC-V processors
 * @note This backend provides native integration with ARM RTX5 RTOS
 */

#include "../xy_os.h"
#include <string.h>

/* RTX5/CMSIS-RTOS2 headers */
#if defined(__ARMCC_VERSION) || defined(__GNUC__)
    #include "cmsis_os2.h"
#else
    #error "RTX5 backend requires CMSIS-RTOS2 compatible compiler"
#endif

/* ==================== Private Definitions ==================== */

#define XY_OS_RTX5_VERSION_MAJOR 1U
#define XY_OS_RTX5_VERSION_MINOR 0U
#define XY_OS_RTX5_VERSION_PATCH 0U

/* ==================== Kernel Control ==================== */

xy_os_status_t xy_os_kernel_init(void)
{
    /* RTX5 kernel is initialized by osKernelInitialize() in main() */
    /* This function is a stub for compatibility */
    return XY_OS_OK;
}

xy_os_status_t xy_os_kernel_get_info(xy_os_version_t *version, char *id_buf,
                                     uint32_t id_size)
{
    if (version) {
        version->api    = (XY_OSAL_VERSION_MAJOR << 16) | XY_OSAL_VERSION_MINOR;
        version->kernel = osKernelGetVersion();
    }
    
    if (id_buf && id_size > 0) {
        strncpy(id_buf, "ARM RTX5", id_size - 1);
        id_buf[id_size - 1] = '\0';
    }
    
    return XY_OS_OK;
}

xy_os_kernel_state_t xy_os_kernel_get_state(void)
{
    osKernelState_t state = osKernelGetState();
    
    switch (state) {
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

xy_os_status_t xy_os_kernel_start(void)
{
    osKernelStart();
    /* Should never return */
    return XY_OS_OK;
}

int32_t xy_os_kernel_lock(void)
{
    int32_t lock_state = osKernelLock();
    
    if (lock_state < 0) {
        return 0; /* Error */
    }
    
    return lock_state;
}

int32_t xy_os_kernel_unlock(void)
{
    int32_t lock_state = osKernelUnlock();
    
    if (lock_state < 0) {
        return 0; /* Error */
    }
    
    return lock_state;
}

int32_t xy_os_kernel_restore_lock(int32_t lock)
{
    /* RTX5 doesn't have direct restore, use lock/unlock based on state */
    if (lock > 0) {
        return osKernelLock();
    } else {
        return osKernelUnlock();
    }
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
    return osKernelGetSysTimerCount();
}

uint32_t xy_os_kernel_get_sys_timer_freq(void)
{
    return osKernelGetSysTimerFreq();
}


/* ==================== Thread Management ==================== */

xy_os_thread_t xy_os_thread_create(const char *name, 
                                   xy_os_thread_func_t func,
                                   void *argument,
                                   xy_os_thread_attr_t *attr)
{
    osThreadAttr_t thread_attr = {0};
    
    thread_attr.name = name;
    thread_attr.attr_bits = osThreadDetached; /* Default to detached */
    
    if (attr) {
        thread_attr.stack_size = attr->stack_size;
        thread_attr.priority = (osPriority_t)attr->priority;
        
        if (attr->stack_mem) {
            thread_attr.stack_mem = attr->stack_mem;
            thread_attr.attr_bits |= osThreadStackAllocated;
        }
        
        if (attr->cb_mem) {
            thread_attr.cb_mem = attr->cb_mem;
            thread_attr.attr_bits |= osThreadCBAllocated;
        }
    }
    
    osThreadId_t tid = osThreadNew((osThreadFunc_t)func, argument, &thread_attr);
    
    return (xy_os_thread_t)tid;
}

xy_os_status_t xy_os_thread_terminate(xy_os_thread_t thread)
{
    osStatus_t status = osThreadTerminate((osThreadId_t)thread);
    
    if (status == osOK) {
        return XY_OS_OK;
    } else if (status == osErrorParameter) {
        return XY_OS_ERROR_INVALID_OBJ;
    } else {
        return XY_OS_ERROR;
    }
}

xy_os_status_t xy_os_thread_exit(void)
{
    osThreadExit();
    /* Should never return */
    return XY_OS_OK;
}

xy_os_status_t xy_os_thread_delay(uint32_t ticks)
{
    osDelay(ticks);
    return XY_OS_OK;
}

xy_os_status_t xy_os_thread_delay_until(uint32_t ticks)
{
    osDelayUntil(ticks);
    return XY_OS_OK;
}

uint32_t xy_os_thread_get_id(void)
{
    return (uint32_t)(uintptr_t)osThreadGetId();
}

xy_os_thread_state_t xy_os_thread_get_state(xy_os_thread_t thread)
{
    osThreadState_t state = osThreadGetState((osThreadId_t)thread);
    
    switch (state) {
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
        case osThreadError:
            return XY_OS_THREAD_ERROR;
        default:
            return XY_OS_THREAD_ERROR;
    }
}

const char *xy_os_thread_get_name(xy_os_thread_t thread)
{
    return osThreadGetName((osThreadId_t)thread);
}

xy_os_status_t xy_os_thread_set_name(xy_os_thread_t thread, const char *name)
{
    osStatus_t status = osThreadSetName((osThreadId_t)thread, name);
    
    if (status == osOK) {
        return XY_OS_OK;
    } else {
        return XY_OS_ERROR;
    }
}

uint32_t xy_os_thread_get_stack_space(xy_os_thread_t thread)
{
    return osThreadGetStackSpace((osThreadId_t)thread);
}

xy_os_status_t xy_os_thread_set_priority(xy_os_thread_t thread, 
                                         xy_os_priority_t priority)
{
    osStatus_t status = osThreadSetPriority((osThreadId_t)thread, 
                                            (osPriority_t)priority);
    
    if (status == osOK) {
        return XY_OS_OK;
    } else {
        return XY_OS_ERROR;
    }
}

xy_os_priority_t xy_os_thread_get_priority(xy_os_thread_t thread)
{
    return (xy_os_priority_t)osThreadGetPriority((osThreadId_t)thread);
}

xy_os_status_t xy_os_thread_yield(void)
{
    osThreadYield();
    return XY_OS_OK;
}

xy_os_status_t xy_os_thread_suspend(xy_os_thread_t thread)
{
    osStatus_t status = osThreadSuspend((osThreadId_t)thread);
    
    if (status == osOK) {
        return XY_OS_OK;
    } else {
        return XY_OS_ERROR;
    }
}

xy_os_status_t xy_os_thread_resume(xy_os_thread_t thread)
{
    osStatus_t status = osThreadResume((osThreadId_t)thread);
    
    if (status == osOK) {
        return XY_OS_OK;
    } else {
        return XY_OS_ERROR;
    }
}


/* ==================== Mutex Management ==================== */

xy_os_mutex_t xy_os_mutex_create(xy_os_mutex_attr_t *attr)
{
    osMutexAttr_t mutex_attr = {0};
    
    if (attr) {
        mutex_attr.name = attr->name;
        
        if (attr->cb_mem) {
            mutex_attr.cb_mem = attr->cb_mem;
            mutex_attr.attr_bits |= osMutexCBAllocated;
        }
    }
    
    osMutexId_t mid = osMutexNew(&mutex_attr);
    
    return (xy_os_mutex_t)mid;
}

xy_os_status_t xy_os_mutex_acquire(xy_os_mutex_t mutex, uint32_t timeout)
{
    osStatus_t status = osMutexAcquire((osMutexId_t)mutex, timeout);
    
    if (status == osOK) {
        return XY_OS_OK;
    } else if (status == osErrorTimeout) {
        return XY_OS_ERROR_TIMEOUT;
    } else if (status == osErrorParameter) {
        return XY_OS_ERROR_INVALID_OBJ;
    } else if (status == osErrorResource) {
        return XY_OS_ERROR_RESOURCE;
    } else {
        return XY_OS_ERROR;
    }
}

xy_os_status_t xy_os_mutex_release(xy_os_mutex_t mutex)
{
    osStatus_t status = osMutexRelease((osMutexId_t)mutex);
    
    if (status == osOK) {
        return XY_OS_OK;
    } else {
        return XY_OS_ERROR;
    }
}

xy_os_status_t xy_os_mutex_delete(xy_os_mutex_t mutex)
{
    osStatus_t status = osMutexDelete((osMutexId_t)mutex);
    
    if (status == osOK) {
        return XY_OS_OK;
    } else {
        return XY_OS_ERROR;
    }
}

xy_os_thread_t xy_os_mutex_get_owner(xy_os_mutex_t mutex)
{
    return (xy_os_thread_t)osMutexGetOwner((osMutexId_t)mutex);
}


/* ==================== Semaphore Management ==================== */

xy_os_semaphore_t xy_os_semaphore_create(uint32_t max_count, 
                                         uint32_t initial_count,
                                         xy_os_semaphore_attr_t *attr)
{
    osSemaphoreAttr_t sem_attr = {0};
    
    if (attr) {
        sem_attr.name = attr->name;
        
        if (attr->cb_mem) {
            sem_attr.cb_mem = attr->cb_mem;
            sem_attr.attr_bits |= osSemaphoreCBAllocated;
        }
    }
    
    osSemaphoreId_t sid = osSemaphoreNew(max_count, initial_count, &sem_attr);
    
    return (xy_os_semaphore_t)sid;
}

xy_os_status_t xy_os_semaphore_acquire(xy_os_semaphore_t semaphore, 
                                       uint32_t timeout)
{
    osStatus_t status = osSemaphoreAcquire((osSemaphoreId_t)semaphore, timeout);
    
    if (status == osOK) {
        return XY_OS_OK;
    } else if (status == osErrorTimeout) {
        return XY_OS_ERROR_TIMEOUT;
    } else if (status == osErrorParameter) {
        return XY_OS_ERROR_INVALID_OBJ;
    } else if (status == osErrorResource) {
        return XY_OS_ERROR_RESOURCE;
    } else {
        return XY_OS_ERROR;
    }
}

xy_os_status_t xy_os_semaphore_release(xy_os_semaphore_t semaphore)
{
    osStatus_t status = osSemaphoreRelease((osSemaphoreId_t)semaphore);
    
    if (status == osOK) {
        return XY_OS_OK;
    } else {
        return XY_OS_ERROR;
    }
}

xy_os_status_t xy_os_semaphore_delete(xy_os_semaphore_t semaphore)
{
    osStatus_t status = osSemaphoreDelete((osSemaphoreId_t)semaphore);
    
    if (status == osOK) {
        return XY_OS_OK;
    } else {
        return XY_OS_ERROR;
    }
}

uint32_t xy_os_semaphore_get_count(xy_os_semaphore_t semaphore)
{
    return osSemaphoreGetCount((osSemaphoreId_t)semaphore);
}


/* ==================== Event Flags Management ==================== */

xy_os_event_flags_t xy_os_event_flags_create(xy_os_event_flags_attr_t *attr)
{
    osEventFlagsAttr_t attr_rtx = {0};
    
    if (attr) {
        attr_rtx.name = attr->name;
        
        if (attr->cb_mem) {
            attr_rtx.cb_mem = attr->cb_mem;
            attr_rtx.attr_bits |= osEventFlagsCBAllocated;
        }
    }
    
    osEventFlagsId_t eid = osEventFlagsNew(&attr_rtx);
    
    return (xy_os_event_flags_t)eid;
}

xy_os_status_t xy_os_event_flags_set(xy_os_event_flags_t event_flags, 
                                     uint32_t flags,
                                     xy_os_event_flags_mode_t mode)
{
    uint32_t ret;
    
    if (mode == XY_OS_EVENT_FLAGS_OR) {
        ret = osEventFlagsSet((osEventFlagsId_t)event_flags, flags);
    } else {
        ret = osEventFlagsClear((osEventFlagsId_t)event_flags, flags);
    }
    
    if (ret & 0x80000000U) {
        return XY_OS_ERROR;
    }
    
    return XY_OS_OK;
}

uint32_t xy_os_event_flags_wait(xy_os_event_flags_t event_flags,
                                uint32_t flags,
                                xy_os_event_flags_mode_t mode,
                                uint32_t timeout)
{
    uint32_t ret;
    
    if (mode == XY_OS_EVENT_FLAGS_AND) {
        ret = osEventFlagsWait((osEventFlagsId_t)event_flags, flags, 
                               osFlagsWaitAll, timeout);
    } else {
        ret = osEventFlagsWait((osEventFlagsId_t)event_flags, flags, 
                               osFlagsWaitAny, timeout);
    }
    
    if (ret & 0x80000000U) {
        return 0; /* Error or timeout */
    }
    
    return ret;
}

xy_os_status_t xy_os_event_flags_clear(xy_os_event_flags_t event_flags, 
                                       uint32_t flags)
{
    uint32_t ret = osEventFlagsClear((osEventFlagsId_t)event_flags, flags);
    
    if (ret & 0x80000000U) {
        return XY_OS_ERROR;
    }
    
    return XY_OS_OK;
}

uint32_t xy_os_event_flags_get(xy_os_event_flags_t event_flags)
{
    return osEventFlagsGet((osEventFlagsId_t)event_flags);
}

xy_os_status_t xy_os_event_flags_delete(xy_os_event_flags_t event_flags)
{
    osStatus_t status = osEventFlagsDelete((osEventFlagsId_t)event_flags);
    
    if (status == osOK) {
        return XY_OS_OK;
    } else {
        return XY_OS_ERROR;
    }
}


/* ==================== Timer Management ==================== */

xy_os_timer_t xy_os_timer_create(const char *name,
                                 xy_os_timer_func_t func,
                                 void *argument,
                                 xy_os_timer_type_t type,
                                 xy_os_timer_attr_t *attr)
{
    osTimerAttr_t timer_attr = {0};
    osTimerType_t timer_type;
    
    timer_attr.name = name;
    
    if (type == XY_OS_TIMER_ONCE) {
        timer_type = osTimerOnce;
    } else {
        timer_type = osTimerPeriodic;
    }
    
    if (attr) {
        if (attr->cb_mem) {
            timer_attr.cb_mem = attr->cb_mem;
            timer_attr.attr_bits |= osTimerCBAllocated;
        }
    }
    
    osTimerId_t tid = osTimerNew((osTimerFunc_t)func, timer_type, 
                                 argument, &timer_attr);
    
    return (xy_os_timer_t)tid;
}

xy_os_status_t xy_os_timer_start(xy_os_timer_t timer, uint32_t ticks)
{
    osStatus_t status = osTimerStart((osTimerId_t)timer, ticks);
    
    if (status == osOK) {
        return XY_OS_OK;
    } else {
        return XY_OS_ERROR;
    }
}

xy_os_status_t xy_os_timer_stop(xy_os_timer_t timer)
{
    osStatus_t status = osTimerStop((osTimerId_t)timer);
    
    if (status == osOK) {
        return XY_OS_OK;
    } else {
        return XY_OS_ERROR;
    }
}

uint32_t xy_os_timer_is_running(xy_os_timer_t timer)
{
    return osTimerIsRunning((osTimerId_t)timer);
}

xy_os_status_t xy_os_timer_delete(xy_os_timer_t timer)
{
    osStatus_t status = osTimerDelete((osTimerId_t)timer);
    
    if (status == osOK) {
        return XY_OS_OK;
    } else {
        return XY_OS_ERROR;
    }
}


/* ==================== Memory Management ==================== */

xy_os_mempool_t xy_os_mempool_create(uint32_t block_count, 
                                     uint32_t block_size,
                                     xy_os_mempool_attr_t *attr)
{
    osMemoryPoolAttr_t pool_attr = {0};
    
    if (attr) {
        pool_attr.name = attr->name;
        
        if (attr->cb_mem) {
            pool_attr.cb_mem = attr->cb_mem;
            pool_attr.attr_bits |= osMemoryPoolCBAllocated;
        }
        
        if (attr->mp_mem) {
            pool_attr.mp_mem = attr->mp_mem;
            pool_attr.attr_bits |= osMemoryPoolBlockAllocated;
        }
    }
    
    osMemoryPoolId_t mid = osMemoryPoolNew(block_count, block_size, &pool_attr);
    
    return (xy_os_mempool_t)mid;
}

xy_os_status_t xy_os_mempool_alloc(xy_os_mempool_t pool, void **block, 
                                   uint32_t timeout)
{
    void *ptr = osMemoryPoolAlloc((osMemoryPoolId_t)pool, timeout);
    
    if (ptr) {
        *block = ptr;
        return XY_OS_OK;
    } else {
        return XY_OS_ERROR_TIMEOUT;
    }
}

xy_os_status_t xy_os_mempool_free(xy_os_mempool_t pool, void *block)
{
    osStatus_t status = osMemoryPoolFree((osMemoryPoolId_t)pool, block);
    
    if (status == osOK) {
        return XY_OS_OK;
    } else {
        return XY_OS_ERROR;
    }
}

uint32_t xy_os_mempool_get_capacity(xy_os_mempool_t pool)
{
    return osMemoryPoolGetCapacity((osMemoryPoolId_t)pool);
}

uint32_t xy_os_mempool_get_block_size(xy_os_mempool_t pool)
{
    return osMemoryPoolGetBlockSize((osMemoryPoolId_t)pool);
}

uint32_t xy_os_mempool_get_count(xy_os_mempool_t pool)
{
    return osMemoryPoolGetCount((osMemoryPoolId_t)pool);
}

uint32_t xy_os_mempool_get_space(xy_os_mempool_t pool)
{
    return osMemoryPoolGetSpace((osMemoryPoolId_t)pool);
}

xy_os_status_t xy_os_mempool_delete(xy_os_mempool_t pool)
{
    osStatus_t status = osMemoryPoolDelete((osMemoryPoolId_t)pool);
    
    if (status == osOK) {
        return XY_OS_OK;
    } else {
        return XY_OS_ERROR;
    }
}


/* ==================== Message Queue Management ==================== */

xy_os_msgqueue_t xy_os_msgqueue_create(uint32_t msg_count, 
                                       uint32_t msg_size,
                                       xy_os_msgqueue_attr_t *attr)
{
    osMessageQueueAttr_t mq_attr = {0};
    
    if (attr) {
        mq_attr.name = attr->name;
        
        if (attr->cb_mem) {
            mq_attr.cb_mem = attr->cb_mem;
            mq_attr.attr_bits |= osMessageQueueCBAllocated;
        }
        
        if (attr->mq_mem) {
            mq_attr.mq_mem = attr->mq_mem;
            mq_attr.attr_bits |= osMessageQueueBufferAllocated;
        }
    }
    
    osMessageQueueId_t mid = osMessageQueueNew(msg_count, msg_size, &mq_attr);
    
    return (xy_os_msgqueue_t)mid;
}

xy_os_status_t xy_os_msgqueue_put(xy_os_msgqueue_t queue, 
                                  const void *msg_ptr,
                                  uint32_t timeout)
{
    osStatus_t status = osMessageQueuePut((osMessageQueueId_t)queue, 
                                          msg_ptr, 0, timeout);
    
    if (status == osOK) {
        return XY_OS_OK;
    } else if (status == osErrorTimeout) {
        return XY_OS_ERROR_TIMEOUT;
    } else if (status == osErrorParameter) {
        return XY_OS_ERROR_INVALID_OBJ;
    } else if (status == osErrorResource) {
        return XY_OS_ERROR_RESOURCE;
    } else {
        return XY_OS_ERROR;
    }
}

xy_os_status_t xy_os_msgqueue_get(xy_os_msgqueue_t queue, 
                                  void *msg_ptr,
                                  uint8_t *msg_prio,
                                  uint32_t timeout)
{
    osStatus_t status;
    
    if (msg_prio) {
        status = osMessageQueueGet((osMessageQueueId_t)queue, msg_ptr, 
                                   msg_prio, timeout);
    } else {
        status = osMessageQueueGet((osMessageQueueId_t)queue, msg_ptr, 
                                   NULL, timeout);
    }
    
    if (status == osOK) {
        return XY_OS_OK;
    } else if (status == osErrorTimeout) {
        return XY_OS_ERROR_TIMEOUT;
    } else if (status == osErrorParameter) {
        return XY_OS_ERROR_INVALID_OBJ;
    } else if (status == osErrorResource) {
        return XY_OS_ERROR_RESOURCE;
    } else {
        return XY_OS_ERROR;
    }
}

xy_os_status_t xy_os_msgqueue_delete(xy_os_msgqueue_t queue)
{
    osStatus_t status = osMessageQueueDelete((osMessageQueueId_t)queue);
    
    if (status == osOK) {
        return XY_OS_OK;
    } else {
        return XY_OS_ERROR;
    }
}

uint32_t xy_os_msgqueue_get_capacity(xy_os_msgqueue_t queue)
{
    return osMessageQueueGetCapacity((osMessageQueueId_t)queue);
}

uint32_t xy_os_msgqueue_get_count(xy_os_msgqueue_t queue)
{
    return osMessageQueueGetCount((osMessageQueueId_t)queue);
}

uint32_t xy_os_msgqueue_get_space(xy_os_msgqueue_t queue)
{
    return osMessageQueueGetSpace((osMessageQueueId_t)queue);
}


/* ==================== ISR Context Functions ==================== */

xy_os_status_t xy_os_is_in_isr(void)
{
    /* RTX5: Check if in ISR context */
    #if defined(__ARMCC_VERSION)
        return (osKernelGetState() == osKernelRunning) ? 0 : 1;
    #else
        /* Generic implementation - may need platform-specific adjustment */
        return 0;
    #endif
}

xy_os_status_t xy_os_delay_isr(uint32_t ticks)
{
    (void)ticks;
    /* Cannot delay in ISR */
    return XY_OS_ERROR_ISR;
}

xy_os_status_t xy_os_thread_yield_isr(void)
{
    osThreadYield();
    return XY_OS_OK;
}


/* ==================== System Time Functions ==================== */

uint64_t xy_os_get_sys_time_us(void)
{
    uint32_t tick_freq = osKernelGetTickFreq();
    uint32_t tick_count = osKernelGetTickCount();
    
    if (tick_freq > 0) {
        return ((uint64_t)tick_count * 1000000ULL) / tick_freq;
    }
    
    return 0;
}

uint64_t xy_os_get_sys_time_ms(void)
{
    uint32_t tick_freq = osKernelGetTickFreq();
    uint32_t tick_count = osKernelGetTickCount();
    
    if (tick_freq > 0) {
        return ((uint64_t)tick_count * 1000ULL) / tick_freq;
    }
    
    return 0;
}


/* ==================== End of File ==================== */
