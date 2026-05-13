/**
 * @file xy_os.h
 * @brief XinYi OS Abstraction Layer (OSAL) - CMSIS-RTOS2 Compatible Interface
 * @note This header provides a generic RTOS interface based on CMSIS-RTOS2 API
 * @version 1.0.0
 * @date 2025-10-27
 */

#ifndef _XY_OS_H_
#define _XY_OS_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Version Information ==================== */
#define XY_OSAL_VERSION_MAJOR 1U
#define XY_OSAL_VERSION_MINOR 0U
#define XY_OSAL_VERSION_PATCH 0U

/* ==================== Common Definitions ==================== */

/**
 * @brief Status code values returned by OSAL functions
 */
typedef enum {
    XY_OS_OK                = 0,  ///< Operation completed successfully
    XY_OS_ERROR             = -1, ///< Unspecified RTOS error
    XY_OS_ERROR_TIMEOUT     = -2, ///< Operation not completed within timeout
    XY_OS_ERROR_RESOURCE    = -3, ///< Resource not available
    XY_OS_ERROR_PARAMETER   = -4, ///< Parameter error
    XY_OS_ERROR_NO_MEMORY   = -5, ///< System is out of memory
    XY_OS_ERROR_ISR         = -6, ///< Not allowed in ISR context
    XY_OS_ERROR_INVALID_OBJ = -7, ///< Invalid object
    XY_OS_ERROR_NOT_INITIALIZED = -8, ///< OS not initialized
    XY_OS_RESERVED              = 0x7FFFFFFF
} xy_os_status_t;

/**
 * @brief Timeout special values
 */
#define XY_OS_WAIT_FOREVER 0xFFFFFFFFU ///< Wait forever timeout value
#define XY_OS_NO_WAIT      0x0U        ///< Do not wait timeout value

/**
 * @brief Type for OS handles (opaque pointers)
 */
typedef void *xy_os_handle_t;


/* ==================== Kernel Control ==================== */

/**
 * @brief Kernel state values
 */
typedef enum {
    XY_OS_KERNEL_INACTIVE  = 0,  ///< Kernel is inactive
    XY_OS_KERNEL_READY     = 1,  ///< Kernel is ready
    XY_OS_KERNEL_RUNNING   = 2,  ///< Kernel is running
    XY_OS_KERNEL_LOCKED    = 3,  ///< Kernel is locked
    XY_OS_KERNEL_SUSPENDED = 4,  ///< Kernel is suspended
    XY_OS_KERNEL_ERROR     = -1, ///< Kernel error
    XY_OS_KERNEL_RESERVED  = 0x7FFFFFFF
} xy_os_kernel_state_t;

/**
 * @brief Kernel version information
 */
typedef struct {
    uint32_t api;    ///< API version
    uint32_t kernel; ///< Kernel version
} xy_os_version_t;

/**
 * @brief Initialize the RTOS kernel
 * @return Status code
 */
xy_os_status_t xy_os_kernel_init(void);

/**
 * @brief Get RTOS kernel information
 * @param[out] version Pointer to version structure
 * @param[out] id_buf Buffer for kernel ID string
 * @param[in] id_size Size of kernel ID buffer
 * @return Status code
 */
xy_os_status_t xy_os_kernel_get_info(xy_os_version_t *version, char *id_buf,
                                     uint32_t id_size);

/**
 * @brief Get the current RTOS kernel state
 * @return Current kernel state
 */
xy_os_kernel_state_t xy_os_kernel_get_state(void);

/**
 * @brief Start the RTOS kernel scheduler
 * @return Status code
 */
xy_os_status_t xy_os_kernel_start(void);

/**
 * @brief Lock the RTOS kernel scheduler
 * @return Previous lock state (lock count)
 */
int32_t xy_os_kernel_lock(void);

/**
 * @brief Unlock the RTOS kernel scheduler
 * @return Previous lock state (lock count)
 */
int32_t xy_os_kernel_unlock(void);

/**
 * @brief Restore the RTOS kernel scheduler lock state
 * @param[in] lock Previous lock state to restore
 * @return New lock state
 */
int32_t xy_os_kernel_restore_lock(int32_t lock);

/**
 * @brief Get the RTOS kernel tick count
 * @return RTOS kernel current tick count
 */
uint32_t xy_os_kernel_get_tick_count(void);

/**
 * @brief Get the RTOS kernel tick frequency (Hz)
 * @return Frequency of the kernel tick in Hz
 */
uint32_t xy_os_kernel_get_tick_freq(void);

/**
 * @brief Get the RTOS kernel system timer count
 * @return RTOS kernel current system timer count
 */
uint32_t xy_os_kernel_get_sys_timer_count(void);

/**
 * @brief Get the RTOS kernel system timer frequency
 * @return Frequency of the system timer in Hz
 */
uint32_t xy_os_kernel_get_sys_timer_freq(void);


/* ==================== Thread Management ==================== */

/**
 * @brief Thread ID type
 */
typedef xy_os_handle_t xy_os_thread_id_t;

/**
 * @brief Thread function type
 */
typedef void (*xy_os_thread_func_t)(void *argument);

/**
 * @brief Thread priority values
 */
typedef enum {
    XY_OS_PRIORITY_NONE          = 0, ///< No priority (not initialized)
    XY_OS_PRIORITY_IDLE          = 1, ///< Idle priority
    XY_OS_PRIORITY_LOW           = 8, ///< Low priority
    XY_OS_PRIORITY_LOW1          = 8 + 1,
    XY_OS_PRIORITY_LOW2          = 8 + 2,
    XY_OS_PRIORITY_LOW3          = 8 + 3,
    XY_OS_PRIORITY_LOW4          = 8 + 4,
    XY_OS_PRIORITY_LOW5          = 8 + 5,
    XY_OS_PRIORITY_LOW6          = 8 + 6,
    XY_OS_PRIORITY_LOW7          = 8 + 7,
    XY_OS_PRIORITY_BELOW_NORMAL  = 16, ///< Below normal priority
    XY_OS_PRIORITY_BELOW_NORMAL1 = 16 + 1,
    XY_OS_PRIORITY_BELOW_NORMAL2 = 16 + 2,
    XY_OS_PRIORITY_BELOW_NORMAL3 = 16 + 3,
    XY_OS_PRIORITY_BELOW_NORMAL4 = 16 + 4,
    XY_OS_PRIORITY_BELOW_NORMAL5 = 16 + 5,
    XY_OS_PRIORITY_BELOW_NORMAL6 = 16 + 6,
    XY_OS_PRIORITY_BELOW_NORMAL7 = 16 + 7,
    XY_OS_PRIORITY_NORMAL        = 24, ///< Normal priority
    XY_OS_PRIORITY_NORMAL1       = 24 + 1,
    XY_OS_PRIORITY_NORMAL2       = 24 + 2,
    XY_OS_PRIORITY_NORMAL3       = 24 + 3,
    XY_OS_PRIORITY_NORMAL4       = 24 + 4,
    XY_OS_PRIORITY_NORMAL5       = 24 + 5,
    XY_OS_PRIORITY_NORMAL6       = 24 + 6,
    XY_OS_PRIORITY_NORMAL7       = 24 + 7,
    XY_OS_PRIORITY_ABOVE_NORMAL  = 32, ///< Above normal priority
    XY_OS_PRIORITY_ABOVE_NORMAL1 = 32 + 1,
    XY_OS_PRIORITY_ABOVE_NORMAL2 = 32 + 2,
    XY_OS_PRIORITY_ABOVE_NORMAL3 = 32 + 3,
    XY_OS_PRIORITY_ABOVE_NORMAL4 = 32 + 4,
    XY_OS_PRIORITY_ABOVE_NORMAL5 = 32 + 5,
    XY_OS_PRIORITY_ABOVE_NORMAL6 = 32 + 6,
    XY_OS_PRIORITY_ABOVE_NORMAL7 = 32 + 7,
    XY_OS_PRIORITY_HIGH          = 40, ///< High priority
    XY_OS_PRIORITY_HIGH1         = 40 + 1,
    XY_OS_PRIORITY_HIGH2         = 40 + 2,
    XY_OS_PRIORITY_HIGH3         = 40 + 3,
    XY_OS_PRIORITY_HIGH4         = 40 + 4,
    XY_OS_PRIORITY_HIGH5         = 40 + 5,
    XY_OS_PRIORITY_HIGH6         = 40 + 6,
    XY_OS_PRIORITY_HIGH7         = 40 + 7,
    XY_OS_PRIORITY_REALTIME      = 48, ///< Realtime priority
    XY_OS_PRIORITY_REALTIME1     = 48 + 1,
    XY_OS_PRIORITY_REALTIME2     = 48 + 2,
    XY_OS_PRIORITY_REALTIME3     = 48 + 3,
    XY_OS_PRIORITY_REALTIME4     = 48 + 4,
    XY_OS_PRIORITY_REALTIME5     = 48 + 5,
    XY_OS_PRIORITY_REALTIME6     = 48 + 6,
    XY_OS_PRIORITY_REALTIME7     = 48 + 7,
    XY_OS_PRIORITY_ISR           = 56, ///< ISR priority
    XY_OS_PRIORITY_ERROR         = -1, ///< Error priority
    XY_OS_PRIORITY_RESERVED      = 0x7FFFFFFF
} xy_os_priority_t;

/**
 * @brief Thread state values
 */
typedef enum {
    XY_OS_THREAD_INACTIVE =
        0, ///< Thread not created or terminated with all resources released
    XY_OS_THREAD_READY      = 1,  ///< Thread is ready to run
    XY_OS_THREAD_RUNNING    = 2,  ///< Thread is running
    XY_OS_THREAD_BLOCKED    = 3,  ///< Thread is blocked
    XY_OS_THREAD_TERMINATED = 4,  ///< Thread is terminated
    XY_OS_THREAD_ERROR      = -1, ///< Thread error
    XY_OS_THREAD_RESERVED   = 0x7FFFFFFF
} xy_os_thread_state_t;

/**
 * @brief Thread attributes flags
 */
#define XY_OS_THREAD_JOINABLE 0x00000001U ///< Thread created in joinable mode
#define XY_OS_THREAD_DETACHED \
    0x00000000U ///< Thread created in detached mode (default)

/**
 * @brief Thread attributes structure
 */
typedef struct {
    const char *name;          ///< Thread name
    uint32_t attr_bits;        ///< Attribute bits (XY_OS_THREAD_xxx flags)
    void *cb_mem;              ///< Control block memory
    uint32_t cb_size;          ///< Control block size
    void *stack_mem;           ///< Stack memory
    uint32_t stack_size;       ///< Stack size
    xy_os_priority_t priority; ///< Thread priority
    uint32_t tz_module;        ///< TrustZone module ID
    uint32_t reserved;         ///< Reserved
} xy_os_thread_attr_t;

/**
 * @brief Create a thread and add it to active threads
 * @param[in] func Thread function
 * @param[in] argument Pointer passed to the thread function
 * @param[in] attr Thread attributes (NULL for default)
 * @return Thread ID or NULL on error
 */
xy_os_thread_id_t xy_os_thread_new(xy_os_thread_func_t func, void *argument,
                                   const xy_os_thread_attr_t *attr);

/**
 * @brief Get name of a thread
 * @param[in] thread_id Thread ID or NULL for current thread
 * @return Name string or NULL on error
 */
const char *xy_os_thread_get_name(xy_os_thread_id_t thread_id);

/**
 * @brief Return the thread ID of the current running thread
 * @return Thread ID or NULL on error
 */
xy_os_thread_id_t xy_os_thread_get_id(void);

/**
 * @brief Get current state of a thread
 * @param[in] thread_id Thread ID
 * @return Thread state
 */
xy_os_thread_state_t xy_os_thread_get_state(xy_os_thread_id_t thread_id);

/**
 * @brief Get stack size of a thread
 * @param[in] thread_id Thread ID
 * @return Stack size in bytes
 */
uint32_t xy_os_thread_get_stack_size(xy_os_thread_id_t thread_id);

/**
 * @brief Get available stack space of a thread
 * @param[in] thread_id Thread ID
 * @return Available stack space in bytes
 */
uint32_t xy_os_thread_get_stack_space(xy_os_thread_id_t thread_id);

/**
 * @brief Change priority of a thread
 * @param[in] thread_id Thread ID
 * @param[in] priority New priority
 * @return Status code
 */
xy_os_status_t xy_os_thread_set_priority(xy_os_thread_id_t thread_id,
                                         xy_os_priority_t priority);

/**
 * @brief Get current priority of a thread
 * @param[in] thread_id Thread ID
 * @return Priority value
 */
xy_os_priority_t xy_os_thread_get_priority(xy_os_thread_id_t thread_id);

/**
 * @brief Pass control to next thread that is in READY state
 * @return Status code
 */
xy_os_status_t xy_os_thread_yield(void);

/**
 * @brief Suspend execution of a thread
 * @param[in] thread_id Thread ID
 * @return Status code
 */
xy_os_status_t xy_os_thread_suspend(xy_os_thread_id_t thread_id);

/**
 * @brief Resume execution of a thread
 * @param[in] thread_id Thread ID
 * @return Status code
 */
xy_os_status_t xy_os_thread_resume(xy_os_thread_id_t thread_id);

/**
 * @brief Detach a thread (storage can be reclaimed when thread terminates)
 * @param[in] thread_id Thread ID
 * @return Status code
 */
xy_os_status_t xy_os_thread_detach(xy_os_thread_id_t thread_id);

/**
 * @brief Wait for specified thread to terminate
 * @param[in] thread_id Thread ID
 * @return Status code
 */
xy_os_status_t xy_os_thread_join(xy_os_thread_id_t thread_id);

/**
 * @brief Terminate execution of current running thread
 */
void xy_os_thread_exit(void) __attribute__((noreturn));

/**
 * @brief Terminate execution of a thread
 * @param[in] thread_id Thread ID
 * @return Status code
 */
xy_os_status_t xy_os_thread_terminate(xy_os_thread_id_t thread_id);

/**
 * @brief Get number of active threads
 * @return Number of active threads
 */
uint32_t xy_os_thread_get_count(void);

/**
 * @brief Enumerate active threads
 * @param[out] thread_array Pointer to array for thread IDs
 * @param[in] array_items Maximum number of items in array
 * @return Number of enumerated threads
 */
uint32_t xy_os_thread_enumerate(xy_os_thread_id_t *thread_array,
                                uint32_t array_items);


/* ==================== Thread Flags ==================== */

/**
 * @brief Set the specified thread flags of a thread
 * @param[in] thread_id Thread ID
 * @param[in] flags Flags to set
 * @return Flags after setting or error code
 */
uint32_t xy_os_thread_flags_set(xy_os_thread_id_t thread_id, uint32_t flags);

/**
 * @brief Clear the specified thread flags of current running thread
 * @param[in] flags Flags to clear
 * @return Flags before clearing or error code
 */
uint32_t xy_os_thread_flags_clear(uint32_t flags);

/**
 * @brief Get the current thread flags of current running thread
 * @return Current thread flags
 */
uint32_t xy_os_thread_flags_get(void);

/**
 * @brief Wait for one or more thread flags of the current running thread to
 * become signaled
 * @param[in] flags Flags to wait for
 * @param[in] options Flags options (or/and, clear)
 * @param[in] timeout Timeout value or 0 (no wait) or XY_OS_WAIT_FOREVER
 * @return Flags before clearing or error code
 */
uint32_t xy_os_thread_flags_wait(uint32_t flags, uint32_t options,
                                 uint32_t timeout);

/* Thread flags options */
#define XY_OS_FLAGS_WAIT_ANY 0x00000000U ///< Wait for any flag (default)
#define XY_OS_FLAGS_WAIT_ALL 0x00000001U ///< Wait for all flags
#define XY_OS_FLAGS_NO_CLEAR 0x00000002U ///< Do not clear flags


/* ==================== Generic Wait Functions ==================== */

/**
 * @brief Wait for Timeout (Time Delay)
 * @param[in] ticks Time ticks value
 * @return Status code
 */
xy_os_status_t xy_os_delay(uint32_t ticks);

/**
 * @brief Wait until specified time
 * @param[in] ticks Absolute time in ticks
 * @return Status code
 */
xy_os_status_t xy_os_delay_until(uint32_t ticks);


/* ==================== Timer Management ==================== */

/**
 * @brief Timer ID type
 */
typedef xy_os_handle_t xy_os_timer_id_t;

/**
 * @brief Timer function type
 */
typedef void (*xy_os_timer_func_t)(void *argument);

/**
 * @brief Timer type values
 */
typedef enum {
    XY_OS_TIMER_ONCE     = 0, ///< One-shot timer
    XY_OS_TIMER_PERIODIC = 1  ///< Repeating timer
} xy_os_timer_type_t;

/**
 * @brief Timer attributes structure
 */
typedef struct {
    const char *name;   ///< Timer name
    uint32_t attr_bits; ///< Attribute bits
    void *cb_mem;       ///< Control block memory
    uint32_t cb_size;   ///< Control block size
} xy_os_timer_attr_t;

/**
 * @brief Create and Initialize a timer
 * @param[in] func Timer callback function
 * @param[in] type Timer type (once or periodic)
 * @param[in] argument Argument to the timer callback function
 * @param[in] attr Timer attributes (NULL for default)
 * @return Timer ID or NULL on error
 */
xy_os_timer_id_t xy_os_timer_new(xy_os_timer_func_t func,
                                 xy_os_timer_type_t type, void *argument,
                                 const xy_os_timer_attr_t *attr);

/**
 * @brief Get name of a timer
 * @param[in] timer_id Timer ID
 * @return Name string or NULL on error
 */
const char *xy_os_timer_get_name(xy_os_timer_id_t timer_id);

/**
 * @brief Start or restart a timer
 * @param[in] timer_id Timer ID
 * @param[in] ticks Time ticks value
 * @return Status code
 */
xy_os_status_t xy_os_timer_start(xy_os_timer_id_t timer_id, uint32_t ticks);

/**
 * @brief Stop a timer
 * @param[in] timer_id Timer ID
 * @return Status code
 */
xy_os_status_t xy_os_timer_stop(xy_os_timer_id_t timer_id);

/**
 * @brief Check if a timer is running
 * @param[in] timer_id Timer ID
 * @return 1 if running, 0 if not running
 */
uint32_t xy_os_timer_is_running(xy_os_timer_id_t timer_id);

/**
 * @brief Delete a timer
 * @param[in] timer_id Timer ID
 * @return Status code
 */
xy_os_status_t xy_os_timer_delete(xy_os_timer_id_t timer_id);


/* ==================== Event Flags ==================== */

/**
 * @brief Event flags ID type
 */
typedef xy_os_handle_t xy_os_event_flags_id_t;

/**
 * @brief Event flags attributes structure
 */
typedef struct {
    const char *name;   ///< Event flags name
    uint32_t attr_bits; ///< Attribute bits
    void *cb_mem;       ///< Control block memory
    uint32_t cb_size;   ///< Control block size
} xy_os_event_flags_attr_t;

/**
 * @brief Create and Initialize an Event Flags object
 * @param[in] attr Event flags attributes (NULL for default)
 * @return Event flags ID or NULL on error
 */
xy_os_event_flags_id_t
xy_os_event_flags_new(const xy_os_event_flags_attr_t *attr);

/**
 * @brief Get name of an Event Flags object
 * @param[in] ef_id Event flags ID
 * @return Name string or NULL on error
 */
const char *xy_os_event_flags_get_name(xy_os_event_flags_id_t ef_id);

/**
 * @brief Set the specified Event Flags
 * @param[in] ef_id Event flags ID
 * @param[in] flags Flags to set
 * @return Event flags after setting or error code
 */
uint32_t xy_os_event_flags_set(xy_os_event_flags_id_t ef_id, uint32_t flags);

/**
 * @brief Clear the specified Event Flags
 * @param[in] ef_id Event flags ID
 * @param[in] flags Flags to clear
 * @return Event flags before clearing or error code
 */
uint32_t xy_os_event_flags_clear(xy_os_event_flags_id_t ef_id, uint32_t flags);

/**
 * @brief Get the current Event Flags
 * @param[in] ef_id Event flags ID
 * @return Current event flags
 */
uint32_t xy_os_event_flags_get(xy_os_event_flags_id_t ef_id);

/**
 * @brief Wait for one or more Event Flags to become signaled
 * @param[in] ef_id Event flags ID
 * @param[in] flags Flags to wait for
 * @param[in] options Flags options (or/and, clear)
 * @param[in] timeout Timeout value or 0 (no wait) or XY_OS_WAIT_FOREVER
 * @return Event flags before clearing or error code
 */
uint32_t xy_os_event_flags_wait(xy_os_event_flags_id_t ef_id, uint32_t flags,
                                uint32_t options, uint32_t timeout);

/**
 * @brief Delete an Event Flags object
 * @param[in] ef_id Event flags ID
 * @return Status code
 */
xy_os_status_t xy_os_event_flags_delete(xy_os_event_flags_id_t ef_id);


/* ==================== Mutex Management ==================== */

/**
 * @brief Mutex ID type
 */
typedef xy_os_handle_t xy_os_mutex_id_t;

/**
 * @brief Mutex attributes flags
 */
#define XY_OS_MUTEX_RECURSIVE    0x00000001U ///< Recursive mutex
#define XY_OS_MUTEX_PRIO_INHERIT 0x00000002U ///< Priority inherit protocol
#define XY_OS_MUTEX_ROBUST       0x00000008U ///< Robust mutex

/**
 * @brief Mutex attributes structure
 */
typedef struct {
    const char *name;   ///< Mutex name
    uint32_t attr_bits; ///< Attribute bits (XY_OS_MUTEX_xxx flags)
    void *cb_mem;       ///< Control block memory
    uint32_t cb_size;   ///< Control block size
} xy_os_mutex_attr_t;

/**
 * @brief Create and Initialize a Mutex object
 * @param[in] attr Mutex attributes (NULL for default)
 * @return Mutex ID or NULL on error
 */
xy_os_mutex_id_t xy_os_mutex_new(const xy_os_mutex_attr_t *attr);

/**
 * @brief Get name of a Mutex object
 * @param[in] mutex_id Mutex ID
 * @return Name string or NULL on error
 */
const char *xy_os_mutex_get_name(xy_os_mutex_id_t mutex_id);

/**
 * @brief Acquire a Mutex or timeout if it is locked
 * @param[in] mutex_id Mutex ID
 * @param[in] timeout Timeout value or 0 (no wait) or XY_OS_WAIT_FOREVER
 * @return Status code
 */
xy_os_status_t xy_os_mutex_acquire(xy_os_mutex_id_t mutex_id, uint32_t timeout);

/**
 * @brief Release a Mutex that was acquired
 * @param[in] mutex_id Mutex ID
 * @return Status code
 */
xy_os_status_t xy_os_mutex_release(xy_os_mutex_id_t mutex_id);

/**
 * @brief Get Thread which owns a Mutex object
 * @param[in] mutex_id Mutex ID
 * @return Thread ID or NULL
 */
xy_os_thread_id_t xy_os_mutex_get_owner(xy_os_mutex_id_t mutex_id);

/**
 * @brief Delete a Mutex object
 * @param[in] mutex_id Mutex ID
 * @return Status code
 */
xy_os_status_t xy_os_mutex_delete(xy_os_mutex_id_t mutex_id);


/* ==================== Semaphore Management ==================== */

/**
 * @brief Semaphore ID type
 */
typedef xy_os_handle_t xy_os_semaphore_id_t;

/**
 * @brief Semaphore attributes structure
 */
typedef struct {
    const char *name;   ///< Semaphore name
    uint32_t attr_bits; ///< Attribute bits
    void *cb_mem;       ///< Control block memory
    uint32_t cb_size;   ///< Control block size
} xy_os_semaphore_attr_t;

/**
 * @brief Create and Initialize a Semaphore object
 * @param[in] max_count Maximum number of available tokens
 * @param[in] initial_count Initial number of available tokens
 * @param[in] attr Semaphore attributes (NULL for default)
 * @return Semaphore ID or NULL on error
 */
xy_os_semaphore_id_t xy_os_semaphore_new(uint32_t max_count,
                                         uint32_t initial_count,
                                         const xy_os_semaphore_attr_t *attr);

/**
 * @brief Get name of a Semaphore object
 * @param[in] semaphore_id Semaphore ID
 * @return Name string or NULL on error
 */
const char *xy_os_semaphore_get_name(xy_os_semaphore_id_t semaphore_id);

/**
 * @brief Acquire a Semaphore token or timeout if no tokens are available
 * @param[in] semaphore_id Semaphore ID
 * @param[in] timeout Timeout value or 0 (no wait) or XY_OS_WAIT_FOREVER
 * @return Status code
 */
xy_os_status_t xy_os_semaphore_acquire(xy_os_semaphore_id_t semaphore_id,
                                       uint32_t timeout);

/**
 * @brief Release a Semaphore token
 * @param[in] semaphore_id Semaphore ID
 * @return Status code
 */
xy_os_status_t xy_os_semaphore_release(xy_os_semaphore_id_t semaphore_id);

/**
 * @brief Get current Semaphore token count
 * @param[in] semaphore_id Semaphore ID
 * @return Number of available tokens
 */
uint32_t xy_os_semaphore_get_count(xy_os_semaphore_id_t semaphore_id);

/**
 * @brief Delete a Semaphore object
 * @param[in] semaphore_id Semaphore ID
 * @return Status code
 */
xy_os_status_t xy_os_semaphore_delete(xy_os_semaphore_id_t semaphore_id);


/* ==================== Memory Pool Management ==================== */

/**
 * @brief Memory Pool ID type
 */
typedef xy_os_handle_t xy_os_mempool_id_t;

/**
 * @brief Memory Pool attributes structure
 */
typedef struct {
    const char *name;   ///< Memory pool name
    uint32_t attr_bits; ///< Attribute bits
    void *cb_mem;       ///< Control block memory
    uint32_t cb_size;   ///< Control block size
    void *mp_mem;       ///< Memory pool memory
    uint32_t mp_size;   ///< Memory pool size
} xy_os_mempool_attr_t;

/**
 * @brief Create and Initialize a Memory Pool object
 * @param[in] block_count Maximum number of memory blocks in pool
 * @param[in] block_size Memory block size in bytes
 * @param[in] attr Memory pool attributes (NULL for default)
 * @return Memory pool ID or NULL on error
 */
xy_os_mempool_id_t xy_os_mempool_new(uint32_t block_count, uint32_t block_size,
                                     const xy_os_mempool_attr_t *attr);

/**
 * @brief Get name of a Memory Pool object
 * @param[in] mp_id Memory pool ID
 * @return Name string or NULL on error
 */
const char *xy_os_mempool_get_name(xy_os_mempool_id_t mp_id);

/**
 * @brief Allocate a memory block from a Memory Pool
 * @param[in] mp_id Memory pool ID
 * @param[in] timeout Timeout value or 0 (no wait) or XY_OS_WAIT_FOREVER
 * @return Address of allocated memory block or NULL on error
 */
void *xy_os_mempool_alloc(xy_os_mempool_id_t mp_id, uint32_t timeout);

/**
 * @brief Return an allocated memory block back to a Memory Pool
 * @param[in] mp_id Memory pool ID
 * @param[in] block Address of memory block to free
 * @return Status code
 */
xy_os_status_t xy_os_mempool_free(xy_os_mempool_id_t mp_id, void *block);

/**
 * @brief Get maximum number of memory blocks in a Memory Pool
 * @param[in] mp_id Memory pool ID
 * @return Maximum number of memory blocks
 */
uint32_t xy_os_mempool_get_capacity(xy_os_mempool_id_t mp_id);

/**
 * @brief Get memory block size in a Memory Pool
 * @param[in] mp_id Memory pool ID
 * @return Memory block size in bytes
 */
uint32_t xy_os_mempool_get_block_size(xy_os_mempool_id_t mp_id);

/**
 * @brief Get number of memory blocks used in a Memory Pool
 * @param[in] mp_id Memory pool ID
 * @return Number of memory blocks used
 */
uint32_t xy_os_mempool_get_count(xy_os_mempool_id_t mp_id);

/**
 * @brief Get number of memory blocks available in a Memory Pool
 * @param[in] mp_id Memory pool ID
 * @return Number of memory blocks available
 */
uint32_t xy_os_mempool_get_space(xy_os_mempool_id_t mp_id);

/**
 * @brief Delete a Memory Pool object
 * @param[in] mp_id Memory pool ID
 * @return Status code
 */
xy_os_status_t xy_os_mempool_delete(xy_os_mempool_id_t mp_id);


/* ==================== Message Queue Management ==================== */

/**
 * @brief Message Queue ID type
 */
typedef xy_os_handle_t xy_os_msgqueue_id_t;

/**
 * @brief Message Queue attributes structure
 */
typedef struct {
    const char *name;   ///< Message queue name
    uint32_t attr_bits; ///< Attribute bits
    void *cb_mem;       ///< Control block memory
    uint32_t cb_size;   ///< Control block size
    void *mq_mem;       ///< Message queue memory
    uint32_t mq_size;   ///< Message queue size
} xy_os_msgqueue_attr_t;

/**
 * @brief Create and Initialize a Message Queue object
 * @param[in] msg_count Maximum number of messages in queue
 * @param[in] msg_size Maximum message size in bytes
 * @param[in] attr Message queue attributes (NULL for default)
 * @return Message queue ID or NULL on error
 */
xy_os_msgqueue_id_t xy_os_msgqueue_new(uint32_t msg_count, uint32_t msg_size,
                                       const xy_os_msgqueue_attr_t *attr);

/**
 * @brief Get name of a Message Queue object
 * @param[in] mq_id Message queue ID
 * @return Name string or NULL on error
 */
const char *xy_os_msgqueue_get_name(xy_os_msgqueue_id_t mq_id);

/**
 * @brief Put a message into a Message Queue
 * @param[in] mq_id Message queue ID
 * @param[in] msg_ptr Message pointer
 * @param[in] msg_prio Message priority
 * @param[in] timeout Timeout value or 0 (no wait) or XY_OS_WAIT_FOREVER
 * @return Status code
 */
xy_os_status_t xy_os_msgqueue_put(xy_os_msgqueue_id_t mq_id,
                                  const void *msg_ptr, uint8_t msg_prio,
                                  uint32_t timeout);

/**
 * @brief Get a message from a Message Queue or timeout if queue is empty
 * @param[in] mq_id Message queue ID
 * @param[out] msg_ptr Pointer to buffer for message
 * @param[out] msg_prio Pointer to buffer for message priority (NULL if not
 * needed)
 * @param[in] timeout Timeout value or 0 (no wait) or XY_OS_WAIT_FOREVER
 * @return Status code
 */
xy_os_status_t xy_os_msgqueue_get(xy_os_msgqueue_id_t mq_id, void *msg_ptr,
                                  uint8_t *msg_prio, uint32_t timeout);

/**
 * @brief Get maximum number of messages in a Message Queue
 * @param[in] mq_id Message queue ID
 * @return Maximum number of messages
 */
uint32_t xy_os_msgqueue_get_capacity(xy_os_msgqueue_id_t mq_id);

/**
 * @brief Get maximum message size in a Message Queue
 * @param[in] mq_id Message queue ID
 * @return Maximum message size in bytes
 */
uint32_t xy_os_msgqueue_get_msg_size(xy_os_msgqueue_id_t mq_id);

/**
 * @brief Get number of queued messages in a Message Queue
 * @param[in] mq_id Message queue ID
 * @return Number of queued messages
 */
uint32_t xy_os_msgqueue_get_count(xy_os_msgqueue_id_t mq_id);

/**
 * @brief Get number of available slots for messages in a Message Queue
 * @param[in] mq_id Message queue ID
 * @return Number of available slots
 */
uint32_t xy_os_msgqueue_get_space(xy_os_msgqueue_id_t mq_id);

/**
 * @brief Reset a Message Queue to initial empty state
 * @param[in] mq_id Message queue ID
 * @return Status code
 */
xy_os_status_t xy_os_msgqueue_reset(xy_os_msgqueue_id_t mq_id);

/**
 * @brief Delete a Message Queue object
 * @param[in] mq_id Message queue ID
 * @return Status code
 */
xy_os_status_t xy_os_msgqueue_delete(xy_os_msgqueue_id_t mq_id);


#ifdef __cplusplus
}
#endif

#endif /* _XY_OS_H_ */
