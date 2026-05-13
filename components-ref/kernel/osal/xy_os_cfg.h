/**
 * @file xy_os_cfg.h
 * @brief XinYi OSAL Configuration - Kernel Selection and Feature Configuration
 * @version 1.0.0
 * @date 2025-12-20
 *
 * This file provides compile-time configuration for the XY OSAL (Operating
 * System Abstraction Layer), including kernel backend selection and feature
 * toggles.
 */

#ifndef _XY_OS_CFG_H_
#define _XY_OS_CFG_H_

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Kernel Backend Selection ==================== */

/**
 * @brief Select the RTOS backend for XY OSAL
 *
 * Only ONE of the following should be defined:
 * - XY_OS_BACKEND_BAREMETAL : No RTOS, minimal functionality
 * - XY_OS_BACKEND_FREERTOS  : FreeRTOS backend
 * - XY_OS_BACKEND_RTTHREAD  : RT-Thread backend
 *
 * Default: BAREMETAL (if none specified)
 */

/* Uncomment ONE of the following backend options: */

#define XY_OS_BACKEND_BAREMETAL      /**< Bare-metal backend (no RTOS) */
/* #define XY_OS_BACKEND_FREERTOS */ /**< FreeRTOS backend */
/* #define XY_OS_BACKEND_RTTHREAD */ /**< RT-Thread backend */

/* Validate backend selection */
#if defined(XY_OS_BACKEND_BAREMETAL) + defined(XY_OS_BACKEND_FREERTOS) \
        + defined(XY_OS_BACKEND_RTTHREAD)                              \
    > 1
#error "Only one XY_OS_BACKEND_xxx can be defined!"
#endif

#if !defined(XY_OS_BACKEND_BAREMETAL) && !defined(XY_OS_BACKEND_FREERTOS) \
    && !defined(XY_OS_BACKEND_RTTHREAD)
#warning "No backend defined, defaulting to BAREMETAL"
#define XY_OS_BACKEND_BAREMETAL
#endif

/* ==================== Feature Configuration ==================== */

/**
 * @brief Enable/Disable OSAL features
 *
 * These macros control which OSAL features are compiled in.
 * Disabling unused features can reduce code size.
 */

/* Thread Management Features */
#ifndef XY_OS_FEATURE_THREAD
#define XY_OS_FEATURE_THREAD \
    1 /**< Enable thread management (0=disable, 1=enable) */
#endif

#ifndef XY_OS_FEATURE_THREAD_FLAGS
#define XY_OS_FEATURE_THREAD_FLAGS \
    1 /**< Enable thread flags (0=disable, 1=enable) */
#endif

/* Synchronization Primitives */
#ifndef XY_OS_FEATURE_MUTEX
#define XY_OS_FEATURE_MUTEX 1 /**< Enable mutex support (0=disable, 1=enable) \
                               */
#endif

#ifndef XY_OS_FEATURE_SEMAPHORE
#define XY_OS_FEATURE_SEMAPHORE \
    1 /**< Enable semaphore support (0=disable, 1=enable) */
#endif

#ifndef XY_OS_FEATURE_EVENT_FLAGS
#define XY_OS_FEATURE_EVENT_FLAGS \
    1 /**< Enable event flags (0=disable, 1=enable) */
#endif

/* Inter-Process Communication */
#ifndef XY_OS_FEATURE_MESSAGE_QUEUE
#define XY_OS_FEATURE_MESSAGE_QUEUE \
    1 /**< Enable message queues (0=disable, 1=enable) */
#endif

/* Memory Management */
#ifndef XY_OS_FEATURE_MEMORY_POOL
#define XY_OS_FEATURE_MEMORY_POOL \
    1 /**< Enable memory pools (0=disable, 1=enable) */
#endif

/* Timer Support */
#ifndef XY_OS_FEATURE_TIMER
#define XY_OS_FEATURE_TIMER \
    1 /**< Enable software timers (0=disable, 1=enable) */
#endif

/* Delay Functions */
#ifndef XY_OS_FEATURE_DELAY
#define XY_OS_FEATURE_DELAY \
    1 /**< Enable delay functions (0=disable, 1=enable) */
#endif

/* ==================== Kernel Configuration Parameters ==================== */

/**
 * @brief Default stack size for threads (in bytes)
 */
#ifndef XY_OS_DEFAULT_STACK_SIZE
#if defined(XY_OS_BACKEND_FREERTOS)
#define XY_OS_DEFAULT_STACK_SIZE 2048
#elif defined(XY_OS_BACKEND_RTTHREAD)
#define XY_OS_DEFAULT_STACK_SIZE 2048
#else
#define XY_OS_DEFAULT_STACK_SIZE 1024
#endif
#endif

/**
 * @brief Default thread priority
 * Range: 0 (lowest) to 56 (highest)
 */
#ifndef XY_OS_DEFAULT_PRIORITY
#define XY_OS_DEFAULT_PRIORITY 24 /**< Normal priority */
#endif

/**
 * @brief System tick frequency (Hz)
 * Only applicable for bare-metal backend
 */
#ifndef XY_OS_TICK_FREQ
#define XY_OS_TICK_FREQ 1000 /**< 1ms tick period */
#endif

/**
 * @brief Maximum number of priority levels
 */
#ifndef XY_OS_MAX_PRIORITY_LEVELS
#define XY_OS_MAX_PRIORITY_LEVELS 57 /**< 0-56 priority range */
#endif

/* ==================== Timeout Configuration ==================== */

/**
 * @brief Default timeout for blocking operations (in ticks)
 * Set to XY_OS_WAIT_FOREVER for infinite wait
 */
#ifndef XY_OS_DEFAULT_TIMEOUT
#define XY_OS_DEFAULT_TIMEOUT 5000 /**< 5 seconds at 1kHz tick */
#endif

/* ==================== Debug and Safety Configuration ==================== */

/**
 * @brief Enable parameter validation checks
 * Disable in production to reduce code size and improve performance
 */
#ifndef XY_OS_PARAM_CHECK
#define XY_OS_PARAM_CHECK 1 /**< Enable parameter checks */
#endif

/**
 * @brief Enable ISR context detection
 * Helps prevent API misuse from interrupt handlers
 */
#ifndef XY_OS_ISR_CHECK
#define XY_OS_ISR_CHECK 1 /**< Enable ISR context check */
#endif

/**
 * @brief Enable NULL pointer checks
 */
#ifndef XY_OS_NULL_CHECK
#define XY_OS_NULL_CHECK 1 /**< Enable NULL pointer checks */
#endif

/**
 * @brief Enable OSAL statistics collection
 * Useful for debugging and performance analysis
 */
#ifndef XY_OS_ENABLE_STATS
#define XY_OS_ENABLE_STATS 0 /**< Disable statistics by default */
#endif

/* ==================== Backend-Specific Configuration ==================== */

#ifdef XY_OS_BACKEND_FREERTOS
/**
 * @brief FreeRTOS-specific configurations
 */
#ifndef XY_OS_FREERTOS_USE_TICKLESS
#define XY_OS_FREERTOS_USE_TICKLESS 0 /**< Enable tickless idle mode */
#endif

#ifndef XY_OS_FREERTOS_HEAP_TYPE
#define XY_OS_FREERTOS_HEAP_TYPE 4 /**< Heap scheme (1-5) */
#endif
#endif

#ifdef XY_OS_BACKEND_RTTHREAD
/**
 * @brief RT-Thread-specific configurations
 */
#ifndef XY_OS_RTTHREAD_HOOK_ENABLE
#define XY_OS_RTTHREAD_HOOK_ENABLE 0 /**< Enable RT-Thread hooks */
#endif

#ifndef XY_OS_RTTHREAD_CONSOLE_ENABLE
#define XY_OS_RTTHREAD_CONSOLE_ENABLE 1 /**< Enable console output */
#endif
#endif

#ifdef XY_OS_BACKEND_BAREMETAL
/**
 * @brief Bare-metal specific configurations
 */
#ifndef XY_OS_BAREMETAL_USE_SYSTICK
#define XY_OS_BAREMETAL_USE_SYSTICK 1 /**< Use SysTick for timing */
#endif

#ifndef XY_OS_BAREMETAL_MAX_LOCKS
#define XY_OS_BAREMETAL_MAX_LOCKS 8 /**< Max nested lock depth */
#endif
#endif

/* ==================== Size Limits ==================== */

/**
 * @brief Maximum name length for OS objects (threads, mutexes, etc.)
 */
#ifndef XY_OS_MAX_NAME_LENGTH
#define XY_OS_MAX_NAME_LENGTH 16 /**< Maximum object name length */
#endif

/**
 * @brief Maximum number of threads that can be enumerated
 */
#ifndef XY_OS_MAX_THREAD_COUNT
#define XY_OS_MAX_THREAD_COUNT 32 /**< Maximum enumerable threads */
#endif

/* ==================== Compiler and Platform Configuration ====================
 */

/**
 * @brief Platform-specific optimizations
 */
#if defined(__GNUC__)
#define XY_OS_WEAK      __attribute__((weak))
#define XY_OS_INLINE    static inline __attribute__((always_inline))
#define XY_OS_NO_RETURN __attribute__((noreturn))
#elif defined(__CC_ARM) || defined(__ARMCC_VERSION)
#define XY_OS_WEAK      __weak
#define XY_OS_INLINE    static __inline
#define XY_OS_NO_RETURN __attribute__((noreturn))
#elif defined(_MSC_VER)
#define XY_OS_WEAK
#define XY_OS_INLINE    static __inline
#define XY_OS_NO_RETURN __declspec(noreturn)
#else
#define XY_OS_WEAK
#define XY_OS_INLINE static inline
#define XY_OS_NO_RETURN
#endif

/* ==================== Version and Build Information ==================== */

/**
 * @brief OSAL configuration version
 */
#define XY_OS_CFG_VERSION_MAJOR 1
#define XY_OS_CFG_VERSION_MINOR 0
#define XY_OS_CFG_VERSION_PATCH 0

/**
 * @brief Build timestamp (populated by build system)
 */
#ifndef XY_OS_BUILD_DATE
#define XY_OS_BUILD_DATE __DATE__
#endif

#ifndef XY_OS_BUILD_TIME
#define XY_OS_BUILD_TIME __TIME__
#endif

/* ==================== Configuration Summary ==================== */

/**
 * @brief Print configuration summary at compile time
 * Useful for verifying build configuration
 */
#if defined(XY_OS_BACKEND_BAREMETAL)
#pragma message("XY_OSAL: Using BAREMETAL backend")
#elif defined(XY_OS_BACKEND_FREERTOS)
#pragma message("XY_OSAL: Using FREERTOS backend")
#elif defined(XY_OS_BACKEND_RTTHREAD)
#pragma message("XY_OSAL: Using RT-THREAD backend")
#endif

/* ==================== Feature Validation ==================== */

/* Validate feature dependencies */
#if XY_OS_FEATURE_THREAD_FLAGS && !XY_OS_FEATURE_THREAD
#warning "XY_OS_FEATURE_THREAD_FLAGS requires XY_OS_FEATURE_THREAD"
#undef XY_OS_FEATURE_THREAD_FLAGS
#define XY_OS_FEATURE_THREAD_FLAGS 0
#endif

/* Bare-metal backend cannot support threading features */
#ifdef XY_OS_BACKEND_BAREMETAL
#if XY_OS_FEATURE_THREAD
#pragma message( \
    "Warning: BAREMETAL backend does not support threading features")
#endif
#if XY_OS_FEATURE_MUTEX || XY_OS_FEATURE_SEMAPHORE || XY_OS_FEATURE_EVENT_FLAGS
#pragma message( \
    "Warning: BAREMETAL backend has limited synchronization support")
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif /* _XY_OS_CFG_H_ */
