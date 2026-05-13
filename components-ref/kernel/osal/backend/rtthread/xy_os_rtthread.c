/**
 * @file xy_os_rtthread.c
 * @brief XY OSAL RT-Thread Implementation
 * @version 2.0
 * @date 2026-02-28
 */

#include "../inc/xy_os.h"

#if defined(STM32U5) || defined(STM32U5xx)

#include "rtthread.h"
#include <string.h>

/* RT-Thread context structures */
typedef struct {
    rt_thread_t thread;
    rt_uint8_t initialized;
    xy_os_callback_t callback;
    void *arg;
} thread_ctx_t;

typedef struct {
    rt_mutex_t mutex;
    rt_uint8_t initialized;
} mutex_ctx_t;

typedef struct {
    rt_sem_t sem;
    rt_uint8_t initialized;
} sem_ctx_t;

typedef struct {
    rt_timer_t timer;
    rt_uint8_t initialized;
    xy_os_callback_t callback;
    void *arg;
} timer_ctx_t;

/* Maximum instances */
#define MAX_THREADS 16
#define MAX_MUTEXES 8
#define MAX_SEMAPHORES 8
#define MAX_TIMERS 8

static thread_ctx_t g_thread_ctx[MAX_THREADS];
static mutex_ctx_t g_mutex_ctx[MAX_MUTEXES];
static sem_ctx_t g_sem_ctx[MAX_SEMAPHORES];
static timer_ctx_t g_timer_ctx[MAX_TIMERS];

/**
 * @brief Find thread context by handle
 */
static thread_ctx_t *find_thread_ctx(rt_thread_t thread)
{
    for (size_t i = 0; i < MAX_THREADS; i++) {
        if (g_thread_ctx[i].thread == thread) {
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
        if (g_thread_ctx[i].thread == NULL) {
            return &g_thread_ctx[i];
        }
    }
    return NULL;
}

/**
 * @brief Convert XY priority to RT-Thread
 */
static rt_uint8_t xy_to_rt_priority(xy_os_priority_t priority)
{
    // RT-Thread uses 0 as highest priority, 31 as lowest (for 32 priority levels)
    // XY uses 0 as lowest, 56 as highest
    rt_uint8_t max_priority = RT_THREAD_PRIORITY_MAX;
    if (max_priority > 56) max_priority = 56;
    
    return (rt_uint8_t)(max_priority - (rt_uint8_t)priority);
}

/**
 * @brief Convert RT-Thread priority to XY
 */
static xy_os_priority_t rt_to_xy_priority(rt_uint8_t priority)
{
    rt_uint8_t max_priority = RT_THREAD_PRIORITY_MAX;
    if (max_priority > 56) max_priority = 56;
    
    return (xy_os_priority_t)(max_priority - priority);
}

/* Kernel Functions */

xy_os_error_t xy_os_kernel_init(void)
{
    // RT-Thread kernel is initialized by rtthread_startup()
    return XY_OS_OK;
}

xy_os_error_t xy_os_kernel_get_info(xy_os_version_t *version, char *id_buf,
                                    uint32_t id_size)
{
    if (version) {
        version->api    = 0x20000; // 2.0.0
        version->kernel = RT_VERSION;
    }
    if (id_buf && id_size > 0) {
        rt_strncpy(id_buf, "RT-Thread", id_size - 1);
        id_buf[id_size - 1] = '\0';
    }
    return XY_OS_OK;
}

xy_os_kernel_state_t xy_os_kernel_get_state(void)
{
    return XY_OS_KERNEL_RUNNING; // RT-Thread is always running after startup
}

xy_os_error_t xy_os_kernel_start(void)
{
    // RT-Thread kernel start is handled by rtthread_startup()
    return XY_OS_OK;
}

int32_t xy_os_kernel_lock(void)
{
    rt_enter_critical();
    return 0;
}

int32_t xy_os_kernel_unlock(void)
{
    rt_exit_critical();
    return 0;
}

int32_t xy_os_kernel_restore_lock(int32_t lock)
{
    XY_UNUSED(lock);
    rt_exit_critical();
    return 0;
}

uint32_t xy_os_kernel_get_tick_count(void)
{
    return (uint32_t)rt_tick_get();
}

uint32_t xy_os_kernel_get_tick_freq(void)
{
    return RT_TICK_PER_SECOND;
}

uint32_t xy_os_kernel_get_sys_timer_count(void)
{
    return (uint32_t)rt_tick_get();
}

uint32_t xy_os_kernel_get_sys_timer_freq(void)
{
    return RT_TICK_PER_SECOND;
}

/* Thread Functions */

xy_os_thread_id_t xy_os_thread_new(xy_os_thread_func_t func, void *argument,
                                   const xy_os_thread_attr_t *attr)
{
    if (!func) {
        return NULL;
    }

    rt_thread_t thread = RT_NULL;
    rt_err_t result;
    
    const char *name = "thread";
    rt_uint32_t stack_size = 1024;
    rt_uint8_t priority = 20;
    rt_uint32_t tick = 10;

    if (attr) {
        if (attr->name) name = attr->name;
        if (attr->stack_size > 0) stack_size = attr->stack_size;
        if (attr->priority <= 56) priority = xy_to_rt_priority(attr->priority);
        if (attr->tick > 0) tick = attr->tick;
    }

    thread = rt_thread_create(name, 
                              (void(*)(void *))func, 
                              argument,
                              stack_size,
                              priority,
                              tick);

    if (thread == RT_NULL) {
        return NULL;
    }

    result = rt_thread_startup(thread);
    if (result != RT_EOK) {
        rt_thread_delete(thread);
        return NULL;
    }

    // Store context
    thread_ctx_t *ctx = alloc_thread_ctx();
    if (ctx) {
        ctx->thread = thread;
        ctx->callback = NULL;
        ctx->arg = argument;
        ctx->initialized = 1;
    }

    return (xy_os_thread_id_t)thread;
}

const char *xy_os_thread_get_name(xy_os_thread_id_t thread_id)
{
    rt_thread_t thread = (rt_thread_t)thread_id;
    if (thread) {
        return thread->name;
    }
    return NULL;
}

xy_os_thread_id_t xy_os_thread_get_id(void)
{
    return (xy_os_thread_id_t)rt_thread_self();
}

xy_os_thread_state_t xy_os_thread_get_state(xy_os_thread_id_t thread_id)
{
    rt_thread_t thread = (rt_thread_t)thread_id;
    if (!thread) return XY_OS_THREAD_ERROR;

    switch (thread->stat) {
    case RT_THREAD_READY:
        return XY_OS_THREAD_READY;
    case RT_THREAD_SUSPEND:
        return XY_OS_THREAD_BLOCKED;
    case RT_THREAD_RUNNING:
        return XY_OS_THREAD_RUNNING;
    case RT_THREAD_CLOSE:
    case RT_THREAD_INIT:
        return XY_OS_THREAD_INACTIVE;
    default:
        return XY_OS_THREAD_ERROR;
    }
}

uint32_t xy_os_thread_get_stack_size(xy_os_thread_id_t thread_id)
{
    rt_thread_t thread = (rt_thread_t)thread_id;
    if (!thread || !thread->stack_addr) return 0;
    
    return thread->stack_size;
}

uint32_t xy_os_thread_get_stack_space(xy_os_thread_id_t thread_id)
{
    rt_thread_t thread = (rt_thread_t)thread_id;
    if (!thread || !thread->stack_addr) return 0;
    
    rt_uint8_t *top_ptr = (rt_uint8_t *)thread->stack_addr;
    rt_uint32_t used = 0;
    
    // Count used stack space (approximate)
    while (used < thread->stack_size && top_ptr[used] == 0xFF) {
        used++;
    }
    
    return thread->stack_size - used;
}

xy_os_error_t xy_os_thread_set_priority(xy_os_thread_id_t thread_id,
                                        xy_os_priority_t priority)
{
    rt_thread_t thread = (rt_thread_t)thread_id;
    if (!thread) return XY_OS_ERROR_INVALID_PARAM;

    rt_uint8_t rt_priority = xy_to_rt_priority(priority);
    rt_err_t result = rt_thread_control(thread, RT_THREAD_CTRL_CHANGE_PRIORITY, &rt_priority);
    
    return (result == RT_EOK) ? XY_OS_OK : XY_OS_ERROR_FAIL;
}

xy_os_priority_t xy_os_thread_get_priority(xy_os_thread_id_t thread_id)
{
    rt_thread_t thread = (rt_thread_t)thread_id;
    if (!thread) return -1;

    return rt_to_xy_priority(thread->current_priority);
}

xy_os_error_t xy_os_thread_yield(void)
{
    rt_thread_yield();
    return XY_OS_OK;
}

xy_os_error_t xy_os_thread_suspend(xy_os_thread_id_t thread_id)
{
    rt_thread_t thread = (rt_thread_t)thread_id;
    if (!thread) return XY_OS_ERROR_INVALID_PARAM;

    rt_err_t result = rt_thread_suspend(thread);
    if (result == RT_EOK) {
        rt_schedule();
    }
    
    return (result == RT_EOK) ? XY_OS_OK : XY_OS_ERROR_FAIL;
}

xy_os_error_t xy_os_thread_resume(xy_os_thread_id_t thread_id)
{
    rt_thread_t thread = (rt_thread_t)thread_id;
    if (!thread) return XY_OS_ERROR_INVALID_PARAM;

    rt_err_t result = rt_thread_resume(thread);
    if (result == RT_EOK) {
        rt_schedule();
    }
    
    return (result == RT_EOK) ? XY_OS_OK : XY_OS_ERROR_FAIL;
}

xy_os_error_t xy_os_thread_detach(xy_os_thread_id_t thread_id)
{
    rt_thread_t thread = (rt_thread_t)thread_id;
    if (!thread) return XY_OS_ERROR_INVALID_PARAM;

    rt_err_t result = rt_thread_detach(thread);
    return (result == RT_EOK) ? XY_OS_OK : XY_OS_ERROR_FAIL;
}

xy_os_error_t xy_os_thread_join(xy_os_thread_id_t thread_id)
{
    // RT-Thread doesn't have thread join concept
    XY_UNUSED(thread_id);
    return XY_OS_OK;
}

void xy_os_thread_exit(void)
{
    rt_thread_exit();
}

xy_os_error_t xy_os_thread_terminate(xy_os_thread_id_t thread_id)
{
    rt_thread_t thread = (rt_thread_t)thread_id;
    if (!thread) return XY_OS_ERROR_INVALID_PARAM;

    rt_err_t result = rt_thread_delete(thread);
    return (result == RT_EOK) ? XY_OS_OK : XY_OS_ERROR_FAIL;
}

uint32_t xy_os_thread_get_count(void)
{
    return rt_thread_get_count();
}

uint32_t xy_os_thread_enumerate(xy_os_thread_id_t *thread_array, uint32_t array_items)
{
    if (!thread_array || array_items == 0) return 0;

    rt_thread_t *threads = (rt_thread_t *)thread_array;
    rt_uint32_t count = rt_thread_get_count();
    
    if (count > array_items) count = array_items;

    // RT-Thread doesn't directly expose thread enumeration
    // This is a simplified implementation
    for (uint32_t i = 0; i < count; i++) {
        threads[i] = RT_NULL; // Not implemented in this simplified version
    }

    return count;
}

/* Thread Flags Functions */
// RT-Thread doesn't have native thread flags, use event groups instead

uint32_t xy_os_thread_flags_set(xy_os_thread_id_t thread_id, uint32_t flags)
{
    // Use RT-Thread event for similar functionality
    XY_UNUSED(thread_id);
    XY_UNUSED(flags);
    return 0x80000000; // Not supported
}

uint32_t xy_os_thread_flags_clear(uint32_t flags)
{
    XY_UNUSED(flags);
    return 0;
}

uint32_t xy_os_thread_flags_get(void)
{
    return 0;
}

uint32_t xy_os_thread_flags_wait(uint32_t flags, uint32_t options, uint32_t timeout)
{
    XY_UNUSED(flags);
    XY_UNUSED(options);
    XY_UNUSED(timeout);
    return 0x80000000; // Not supported
}

/* Delay Functions */

xy_os_error_t xy_os_delay(uint32_t ticks)
{
    rt_thread_delay(ticks);
    return XY_OS_OK;
}

xy_os_error_t xy_os_delay_until(uint32_t ticks)
{
    rt_tick_t current = rt_tick_get();
    if (ticks > current) {
        rt_thread_delay(ticks - current);
    }
    return XY_OS_OK;
}

/* Timer Functions */

static void timer_callback(void *parameter)
{
    timer_ctx_t *ctx = (timer_ctx_t *)parameter;
    if (ctx && ctx->callback) {
        ctx->callback(ctx->arg);
    }
}

xy_os_timer_id_t xy_os_timer_new(xy_os_timer_func_t func, xy_os_timer_type_t type,
                                 void *argument, const xy_os_timer_attr_t *attr)
{
    if (!func) return NULL;

    const char *name = "timer";
    if (attr && attr->name) {
        name = attr->name;
    }

    rt_timer_t timer = rt_timer_create(name,
                                       (void(*)(void *))timer_callback,
                                       argument,
                                       1000, // Will be set later
                                       RT_TIMER_FLAG_ONE_SHOT | RT_TIMER_FLAG_SOFT_TIMER);

    if (!timer) return NULL;

    // Configure timer type
    rt_uint8_t flag = (type == XY_OS_TIMER_PERIODIC) ? 
                      RT_TIMER_FLAG_PERIODIC : RT_TIMER_FLAG_ONE_SHOT;
    rt_timer_control(timer, RT_TIMER_CTRL_SET_FLAG, &flag);

    // Store context
    for (size_t i = 0; i < MAX_TIMERS; i++) {
        if (g_timer_ctx[i].timer == NULL) {
            g_timer_ctx[i].timer = timer;
            g_timer_ctx[i].callback = func;
            g_timer_ctx[i].arg = argument;
            g_timer_ctx[i].initialized = 1;
            return (xy_os_timer_id_t)timer;
        }
    }

    rt_timer_delete(timer);
    return NULL;
}

const char *xy_os_timer_get_name(xy_os_timer_id_t timer_id)
{
    rt_timer_t timer = (rt_timer_t)timer_id;
    if (timer) {
        return timer->parent.name;
    }
    return NULL;
}

xy_os_error_t xy_os_timer_start(xy_os_timer_id_t timer_id, uint32_t ticks)
{
    rt_timer_t timer = (rt_timer_t)timer_id;
    if (!timer) return XY_OS_ERROR_INVALID_PARAM;

    // Set timeout
    rt_timer_control(timer, RT_TIMER_CTRL_SET_TIME, &ticks);

    rt_err_t result = rt_timer_start(timer);
    return (result == RT_EOK) ? XY_OS_OK : XY_OS_ERROR_FAIL;
}

xy_os_error_t xy_os_timer_stop(xy_os_timer_id_t timer_id)
{
    rt_timer_t timer = (rt_timer_t)timer_id;
    if (!timer) return XY_OS_ERROR_INVALID_PARAM;

    rt_err_t result = rt_timer_stop(timer);
    return (result == RT_EOK) ? XY_OS_OK : XY_OS_ERROR_FAIL;
}

uint32_t xy_os_timer_is_running(xy_os_timer_id_t timer_id)
{
    rt_timer_t timer = (rt_timer_t)timer_id;
    if (!timer) return 0;

    return (timer->parent.flag & RT_OBJECT_INIT) ? 1 : 0;
}

xy_os_error_t xy_os_timer_delete(xy_os_timer_id_t timer_id)
{
    rt_timer_t timer = (rt_timer_t)timer_id;
    if (!timer) return XY_OS_ERROR_INVALID_PARAM;

    // Clean up context
    for (size_t i = 0; i < MAX_TIMERS; i++) {
        if (g_timer_ctx[i].timer == timer) {
            g_timer_ctx[i].initialized = 0;
            g_timer_ctx[i].timer = NULL;
            break;
        }
    }

    rt_err_t result = rt_timer_delete(timer);
    return (result == RT_EOK) ? XY_OS_OK : XY_OS_ERROR_FAIL;
}

/* Event Flags Functions */
// Use RT-Thread event group

xy_os_event_flags_id_t xy_os_event_flags_new(const xy_os_event_flags_attr_t *attr)
{
    const char *name = "event";
    rt_uint8_t flag = RT_IPC_FLAG_PRIO;

    if (attr && attr->name) {
        name = attr->name;
    }

    rt_event_t event = rt_event_create(name, flag);
    return (xy_os_event_flags_id_t)event;
}

const char *xy_os_event_flags_get_name(xy_os_event_flags_id_t ef_id)
{
    rt_event_t event = (rt_event_t)ef_id;
    if (event) {
        return event->parent.parent.name;
    }
    return NULL;
}

uint32_t xy_os_event_flags_set(xy_os_event_flags_id_t ef_id, uint32_t flags)
{
    rt_event_t event = (rt_event_t)ef_id;
    if (!event) return 0x80000000;

    rt_err_t result = rt_event_send(event, flags);
    return (result == RT_EOK) ? flags : 0x80000000;
}

uint32_t xy_os_event_flags_clear(xy_os_event_flags_id_t ef_id, uint32_t flags)
{
    // RT-Thread event doesn't have clear function, return current flags
    rt_event_t event = (rt_event_t)ef_id;
    if (!event) return 0x80000000;

    rt_uint32_t current;
    rt_err_t result = rt_event_control(event, RT_EVENT_CTRL_GET, &current);
    if (result == RT_EOK) {
        current &= ~flags;
        return current;
    }
    return 0x80000000;
}

uint32_t xy_os_event_flags_get(xy_os_event_flags_id_t ef_id)
{
    rt_event_t event = (rt_event_t)ef_id;
    if (!event) return 0x80000000;

    rt_uint32_t current;
    rt_err_t result = rt_event_control(event, RT_EVENT_CTRL_GET, &current);
    return (result == RT_EOK) ? current : 0x80000000;
}

uint32_t xy_os_event_flags_wait(xy_os_event_flags_id_t ef_id, uint32_t flags,
                                uint32_t options, uint32_t timeout)
{
    rt_event_t event = (rt_event_t)ef_id;
    if (!event) return 0x80000000;

    rt_uint32_t recved;
    rt_uint8_t opt = RT_EVENT_FLAG_AND;
    if (options & XY_OS_FLAGS_WAIT_ALL) {
        opt = RT_EVENT_FLAG_AND;
    } else {
        opt = RT_EVENT_FLAG_OR;
    }

    if (options & XY_OS_FLAGS_NO_CLEAR) {
        opt |= RT_EVENT_FLAG_CLEAR; // Actually means "clear", so if NO_CLEAR, don't set flag
    }

    rt_err_t result = rt_event_recv(event, flags, opt, timeout, &recved);
    return (result == RT_EOK) ? recved : 0x80000000;
}

xy_os_error_t xy_os_event_flags_delete(xy_os_event_flags_id_t ef_id)
{
    rt_event_t event = (rt_event_t)ef_id;
    if (!event) return XY_OS_ERROR_INVALID_PARAM;

    rt_err_t result = rt_event_delete(event);
    return (result == RT_EOK) ? XY_OS_OK : XY_OS_ERROR_FAIL;
}

/* Mutex Functions */

xy_os_mutex_id_t xy_os_mutex_new(const xy_os_mutex_attr_t *attr)
{
    const char *name = "mutex";
    rt_uint8_t flag = RT_IPC_FLAG_PRIO;

    if (attr && attr->name) {
        name = attr->name;
    }

    rt_mutex_t mutex = rt_mutex_create(name, flag);
    if (!mutex) return NULL;

    // Store context
    for (size_t i = 0; i < MAX_MUTEXES; i++) {
        if (g_mutex_ctx[i].mutex == NULL) {
            g_mutex_ctx[i].mutex = mutex;
            g_mutex_ctx[i].initialized = 1;
            return (xy_os_mutex_id_t)mutex;
        }
    }

    rt_mutex_delete(mutex);
    return NULL;
}

const char *xy_os_mutex_get_name(xy_os_mutex_id_t mutex_id)
{
    rt_mutex_t mutex = (rt_mutex_t)mutex_id;
    if (mutex) {
        return mutex->parent.parent.name;
    }
    return NULL;
}

xy_os_error_t xy_os_mutex_acquire(xy_os_mutex_id_t mutex_id, uint32_t timeout)
{
    rt_mutex_t mutex = (rt_mutex_t)mutex_id;
    if (!mutex) return XY_OS_ERROR_INVALID_PARAM;

    rt_err_t result = (timeout == XY_OS_WAIT_FOREVER) ?
                      rt_mutex_take(mutex, RT_WAITING_FOREVER) :
                      rt_mutex_take(mutex, timeout);

    return (result == RT_EOK) ? XY_OS_OK : XY_OS_ERROR_FAIL;
}

xy_os_error_t xy_os_mutex_release(xy_os_mutex_id_t mutex_id)
{
    rt_mutex_t mutex = (rt_mutex_t)mutex_id;
    if (!mutex) return XY_OS_ERROR_INVALID_PARAM;

    rt_err_t result = rt_mutex_release(mutex);
    return (result == RT_EOK) ? XY_OS_OK : XY_OS_ERROR_FAIL;
}

xy_os_thread_id_t xy_os_mutex_get_owner(xy_os_mutex_id_t mutex_id)
{
    rt_mutex_t mutex = (rt_mutex_t)mutex_id;
    if (!mutex) return NULL;

    return (xy_os_thread_id_t)mutex->owner;
}

xy_os_error_t xy_os_mutex_delete(xy_os_mutex_id_t mutex_id)
{
    rt_mutex_t mutex = (rt_mutex_t)mutex_id;
    if (!mutex) return XY_OS_ERROR_INVALID_PARAM;

    // Clean up context
    for (size_t i = 0; i < MAX_MUTEXES; i++) {
        if (g_mutex_ctx[i].mutex == mutex) {
            g_mutex_ctx[i].initialized = 0;
            g_mutex_ctx[i].mutex = NULL;
            break;
        }
    }

    rt_err_t result = rt_mutex_delete(mutex);
    return (result == RT_EOK) ? XY_OS_OK : XY_OS_ERROR_FAIL;
}

/* Semaphore Functions */

xy_os_semaphore_id_t xy_os_semaphore_new(uint32_t max_count, uint32_t initial_count,
                                         const xy_os_semaphore_attr_t *attr)
{
    const char *name = "sem";
    rt_uint8_t flag = RT_IPC_FLAG_PRIO;

    if (attr && attr->name) {
        name = attr->name;
    }

    // RT-Thread semaphore doesn't have max_count, just use initial_count
    rt_sem_t sem = rt_sem_create(name, initial_count, flag);
    if (!sem) return NULL;

    // Store context
    for (size_t i = 0; i < MAX_SEMAPHORES; i++) {
        if (g_sem_ctx[i].sem == NULL) {
            g_sem_ctx[i].sem = sem;
            g_sem_ctx[i].initialized = 1;
            return (xy_os_semaphore_id_t)sem;
        }
    }

    rt_sem_delete(sem);
    return NULL;
}

const char *xy_os_semaphore_get_name(xy_os_semaphore_id_t semaphore_id)
{
    rt_sem_t sem = (rt_sem_t)semaphore_id;
    if (sem) {
        return sem->parent.parent.name;
    }
    return NULL;
}

xy_os_error_t xy_os_semaphore_acquire(xy_os_semaphore_id_t semaphore_id, uint32_t timeout)
{
    rt_sem_t sem = (rt_sem_t)semaphore_id;
    if (!sem) return XY_OS_ERROR_INVALID_PARAM;

    rt_err_t result = (timeout == XY_OS_WAIT_FOREVER) ?
                      rt_sem_take(sem, RT_WAITING_FOREVER) :
                      rt_sem_take(sem, timeout);

    return (result == RT_EOK) ? XY_OS_OK : XY_OS_ERROR_FAIL;
}

xy_os_error_t xy_os_semaphore_release(xy_os_semaphore_id_t semaphore_id)
{
    rt_sem_t sem = (rt_sem_t)semaphore_id;
    if (!sem) return XY_OS_ERROR_INVALID_PARAM;

    rt_err_t result = rt_sem_release(sem);
    return (result == RT_EOK) ? XY_OS_OK : XY_OS_ERROR_FAIL;
}

uint32_t xy_os_semaphore_get_count(xy_os_semaphore_id_t semaphore_id)
{
    rt_sem_t sem = (rt_sem_t)semaphore_id;
    if (!sem) return 0;

    return sem->value;
}

xy_os_error_t xy_os_semaphore_delete(xy_os_semaphore_id_t semaphore_id)
{
    rt_sem_t sem = (rt_sem_t)semaphore_id;
    if (!sem) return XY_OS_ERROR_INVALID_PARAM;

    // Clean up context
    for (size_t i = 0; i < MAX_SEMAPHORES; i++) {
        if (g_sem_ctx[i].sem == sem) {
            g_sem_ctx[i].initialized = 0;
            g_sem_ctx[i].sem = NULL;
            break;
        }
    }

    rt_err_t result = rt_sem_delete(sem);
    return (result == RT_EOK) ? XY_OS_OK : XY_OS_ERROR_FAIL;
}

/* Memory Pool Functions */

xy_os_mempool_id_t xy_os_mempool_new(uint32_t block_count, uint32_t block_size,
                                     const xy_os_mempool_attr_t *attr)
{
    const char *name = "memp";
    rt_uint8_t flag = RT_IPC_FLAG_PRIO;

    if (attr && attr->name) {
        name = attr->name;
    }

    // Calculate total memory size needed
    rt_uint32_t total_size = block_count * block_size;
    rt_uint8_t *pool = (rt_uint8_t *)rt_malloc(total_size);
    if (!pool) return NULL;

    rt_mp_t mp = rt_mp_create(name, block_count, block_size);
    if (!mp) {
        rt_free(pool);
        return NULL;
    }

    return (xy_os_mempool_id_t)mp;
}

const char *xy_os_mempool_get_name(xy_os_mempool_id_t mp_id)
{
    rt_mp_t mp = (rt_mp_t)mp_id;
    if (mp) {
        return mp->parent.name;
    }
    return NULL;
}

void *xy_os_mempool_alloc(xy_os_mempool_id_t mp_id, uint32_t timeout)
{
    rt_mp_t mp = (rt_mp_t)mp_id;
    if (!mp) return NULL;

    void *block = RT_NULL;
    if (timeout == XY_OS_WAIT_FOREVER) {
        block = rt_mp_alloc(mp, RT_WAITING_FOREVER);
    } else {
        block = rt_mp_alloc(mp, timeout);
    }

    return block;
}

xy_os_error_t xy_os_mempool_free(xy_os_mempool_id_t mp_id, void *block)
{
    rt_mp_t mp = (rt_mp_t)mp_id;
    if (!mp || !block) return XY_OS_ERROR_INVALID_PARAM;

    rt_err_t result = rt_mp_free(mp, block);
    return (result == RT_EOK) ? XY_OS_OK : XY_OS_ERROR_FAIL;
}

uint32_t xy_os_mempool_get_capacity(xy_os_mempool_id_t mp_id)
{
    rt_mp_t mp = (rt_mp_t)mp_id;
    if (!mp) return 0;

    // RT-Thread doesn't expose capacity directly, return stored value
    XY_UNUSED(mp);
    return 0; // Not available
}

uint32_t xy_os_mempool_get_block_size(xy_os_mempool_id_t mp_id)
{
    rt_mp_t mp = (rt_mp_t)mp_id;
    if (!mp) return 0;

    // RT-Thread doesn't expose block size directly
    XY_UNUSED(mp);
    return 0; // Not available
}

uint32_t xy_os_mempool_get_count(xy_os_mempool_id_t mp_id)
{
    rt_mp_t mp = (rt_mp_t)mp_id;
    if (!mp) return 0;

    // RT-Thread doesn't expose count directly
    XY_UNUSED(mp);
    return 0; // Not available
}

uint32_t xy_os_mempool_get_space(xy_os_mempool_id_t mp_id)
{
    rt_mp_t mp = (rt_mp_t)mp_id;
    if (!mp) return 0;

    // RT-Thread doesn't expose space directly
    XY_UNUSED(mp);
    return 0; // Not available
}

xy_os_error_t xy_os_mempool_delete(xy_os_mempool_id_t mp_id)
{
    rt_mp_t mp = (rt_mp_t)mp_id;
    if (!mp) return XY_OS_ERROR_INVALID_PARAM;

    rt_err_t result = rt_mp_delete(mp);
    return (result == RT_EOK) ? XY_OS_OK : XY_OS_ERROR_FAIL;
}

/* Message Queue Functions */

xy_os_msgqueue_id_t xy_os_msgqueue_new(uint32_t msg_count, uint32_t msg_size,
                                       const xy_os_msgqueue_attr_t *attr)
{
    const char *name = "msgq";
    rt_uint8_t flag = RT_IPC_FLAG_PRIO;

    if (attr && attr->name) {
        name = attr->name;
    }

    rt_mq_t mq = rt_mq_create(name, msg_size, msg_count, flag);
    return (xy_os_msgqueue_id_t)mq;
}

const char *xy_os_msgqueue_get_name(xy_os_msgqueue_id_t mq_id)
{
    rt_mq_t mq = (rt_mq_t)mq_id;
    if (mq) {
        return mq->parent.parent.name;
    }
    return NULL;
}

xy_os_error_t xy_os_msgqueue_put(xy_os_msgqueue_id_t mq_id,
                                 const void *msg_ptr, uint8_t msg_prio,
                                 uint32_t timeout)
{
    rt_mq_t mq = (rt_mq_t)mq_id;
    if (!mq || !msg_ptr) return XY_OS_ERROR_INVALID_PARAM;

    rt_err_t result = (timeout == XY_OS_WAIT_FOREVER) ?
                      rt_mq_send(mq, (void *)msg_ptr, strlen((char *)msg_ptr) + 1) :
                      rt_mq_send_wait(mq, (void *)msg_ptr, strlen((char *)msg_ptr) + 1, timeout);

    return (result == RT_EOK) ? XY_OS_OK : XY_OS_ERROR_FAIL;
}

xy_os_error_t xy_os_msgqueue_get(xy_os_msgqueue_id_t mq_id, void *msg_ptr,
                                 uint8_t *msg_prio, uint32_t timeout)
{
    rt_mq_t mq = (rt_mq_t)mq_id;
    if (!mq || !msg_ptr) return XY_OS_ERROR_INVALID_PARAM;

    rt_size_t size = 0;
    rt_err_t result = (timeout == XY_OS_WAIT_FOREVER) ?
                      rt_mq_recv(mq, msg_ptr, mq->msg_size, RT_WAITING_FOREVER, &size) :
                      rt_mq_recv(mq, msg_ptr, mq->msg_size, timeout, &size);

    XY_UNUSED(msg_prio); // RT-Thread doesn't support message priority

    return (result == RT_EOK) ? XY_OS_OK : XY_OS_ERROR_FAIL;
}

uint32_t xy_os_msgqueue_get_capacity(xy_os_msgqueue_id_t mq_id)
{
    rt_mq_t mq = (rt_mq_t)mq_id;
    if (!mq) return 0;

    return mq->max_msgs;
}

uint32_t xy_os_msgqueue_get_msg_size(xy_os_msgqueue_id_t mq_id)
{
    rt_mq_t mq = (rt_mq_t)mq_id;
    if (!mq) return 0;

    return mq->msg_size;
}

uint32_t xy_os_msgqueue_get_count(xy_os_msgqueue_id_t mq_id)
{
    rt_mq_t mq = (rt_mq_t)mq_id;
    if (!mq) return 0;

    return mq->entry;
}

uint32_t xy_os_msgqueue_get_space(xy_os_msgqueue_id_t mq_id)
{
    rt_mq_t mq = (rt_mq_t)mq_id;
    if (!mq) return 0;

    return mq->max_msgs - mq->entry;
}

xy_os_error_t xy_os_msgqueue_reset(xy_os_msgqueue_id_t mq_id)
{
    rt_mq_t mq = (rt_mq_t)mq_id;
    if (!mq) return XY_OS_ERROR_INVALID_PARAM;

    // Clear all messages in queue
    rt_err_t result = rt_mq_control(mq, RT_MQ_CTRL_RESET, RT_NULL);
    return (result == RT_EOK) ? XY_OS_OK : XY_OS_ERROR_FAIL;
}

xy_os_error_t xy_os_msgqueue_delete(xy_os_msgqueue_id_t mq_id)
{
    rt_mq_t mq = (rt_mq_t)mq_id;
    if (!mq) return XY_OS_ERROR_INVALID_PARAM;

    rt_err_t result = rt_mq_delete(mq);
    return (result == RT_EOK) ? XY_OS_OK : XY_OS_ERROR_FAIL;
}

#endif /* STM32U5 || STM32U5xx */
