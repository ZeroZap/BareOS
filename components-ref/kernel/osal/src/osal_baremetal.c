/**
 * @file xy_os_baremetal.c
 * @brief XY OSAL Bare-metal Implementation - Minimal RTOS stub
 * @version 1.0.0
 */

#include "../xy_os.h"
#include "../../misc/xy_tick.h"
#include "../../misc/xy_timer_sw.h"
#include <string.h>

/**
 * @brief Platform-specific interrupt control
 * 
 * Supports:
 * - ARM Cortex-M (M0/M0+/M3/M4/M7/M33)
 * - RISC-V (with CSR access)
 * - x86/x64 (PC/Testing - no interrupt control)
 * - Generic fallback
 */

#if defined(XY_OS_DISABLE_ASM) || defined(__x86_64__) || defined(_M_X64) || \
    defined(__i386__) || defined(_M_IX86)
    /* 
     * PC/x86 - No interrupt control needed 
     * Used for testing and simulation
     */
    static __inline void __disable_irq_global(void) {}
    static __inline void __enable_irq_global(void) {}
    static __inline uint32_t __get_PRIMASK_global(void) { return 0; }
    
    #define XY_OS_BAREMETAL_PLATFORM "x86/x64 (No IRQ Control)"

#elif defined(__ARMCC_VERSION) || (defined(__GNUC__) && defined(__ARM_ARCH))
    /* 
     * ARM Cortex-M interrupt disable/enable
     * Uses PRIMASK register to disable all IRQ exceptions
     */
    #include <stdint.h>
    
    static __inline void __disable_irq_global(void) {
        __asm volatile ("cpsid i" : : : "memory");
    }
    
    static __inline void __enable_irq_global(void) {
        __asm volatile ("cpsie i" : : : "memory");
    }
    
    static __inline uint32_t __get_PRIMASK_global(void) {
        uint32_t result;
        __asm volatile ("MRS %0, primask" : "=r" (result) );
        return result;
    }
    
    #define XY_OS_BAREMETAL_PLATFORM "ARM Cortex-M (PRIMASK)"

#elif defined(__ICCARM__)
    /* 
     * IAR ARM interrupt control 
     * Uses intrinsic functions
     */
    #include <intrinsics.h>
    #define __disable_irq_global __disable_interrupt
    #define __enable_irq_global __enable_interrupt
    #define __get_PRIMASK_global __get_interrupt_state
    
    #define XY_OS_BAREMETAL_PLATFORM "ARM Cortex-M (IAR)"

#elif defined(__riscv) || defined(__riscv__)
    /* 
     * RISC-V interrupt control
     * Uses mstatus CSR to control MIE (Machine Interrupt Enable) bit
     * 
     * Note: Requires machine mode (M-mode) access
     * For user mode (U-mode), use supervisor calls or platform-specific API
     */
    #include <stdint.h>
    
    /* RISC-V mstatus register MIE bit position */
    #define XY_OS_RISCV_MSTATUS_MIE_BIT  (1 << 3)
    
    static __inline void __disable_irq_global(void) {
        #if defined(__GNUC__) || defined(__clang__)
            __asm volatile ("csrc mstatus, %0" :: "r"(XY_OS_RISCV_MSTATUS_MIE_BIT));
        #else
            /* Generic CSR write */
            uint32_t mstatus;
            __asm volatile ("csrr %0, mstatus" : "=r"(mstatus));
            mstatus &= ~XY_OS_RISCV_MSTATUS_MIE_BIT;
            __asm volatile ("csrw mstatus, %0" :: "r"(mstatus));
        #endif
    }
    
    static __inline void __enable_irq_global(void) {
        #if defined(__GNUC__) || defined(__clang__)
            __asm volatile ("csrs mstatus, %0" :: "r"(XY_OS_RISCV_MSTATUS_MIE_BIT));
        #else
            /* Generic CSR write */
            uint32_t mstatus;
            __asm volatile ("csrr %0, mstatus" : "=r"(mstatus));
            mstatus |= XY_OS_RISCV_MSTATUS_MIE_BIT;
            __asm volatile ("csrw mstatus, %0" :: "r"(mstatus));
        #endif
    }
    
    static __inline uint32_t __get_PRIMASK_global(void) {
        uint32_t mstatus;
        __asm volatile ("csrr %0, mstatus" : "=r"(mstatus));
        return (mstatus & XY_OS_RISCV_MSTATUS_MIE_BIT) ? 0 : 1;
    }
    
    #define XY_OS_BAREMETAL_PLATFORM "RISC-V (mstatus.MIE)"

#elif defined(__ARC__)
    /* 
     * Synopsys DesignWare ARC interrupt control
     */
    #include <arc/arc_exception.h>
    #define __disable_irq_global _interrupt_disable
    #define __enable_irq_global _interrupt_enable
    #define __get_PRIMASK_global _interrupt_status
    
    #define XY_OS_BAREMETAL_PLATFORM "ARC (DesignWare)"

#else
    /* 
     * Generic fallback - no interrupt control
     * Use with caution - not safe for ISR contexts
     */
    static __inline void __disable_irq_global(void) {}
    static __inline void __enable_irq_global(void) {}
    static __inline uint32_t __get_PRIMASK_global(void) { return 0; }
    
    #define XY_OS_BAREMETAL_PLATFORM "Generic (No IRQ Control)"
#endif

/* ==================== Kernel State ==================== */

static volatile uint32_t s_lock_count = 0;
static xy_os_kernel_state_t s_state   = XY_OS_KERNEL_INACTIVE;

/**
 * @brief Get bare-metal platform information
 * @return Platform description string
 */
const char *xy_os_baremetal_get_platform(void)
{
    #if defined(XY_OS_BAREMETAL_PLATFORM)
        return XY_OS_BAREMETAL_PLATFORM;
    #else
        return "Unknown";
    #endif
}

// Kernel functions
xy_os_status_t xy_os_kernel_init(void)
{
    s_state = XY_OS_KERNEL_READY;
    return XY_OS_OK;
}

xy_os_status_t xy_os_kernel_get_info(xy_os_version_t *version, char *id_buf,
                                     uint32_t id_size)
{
    if (version) {
        version->api    = (XY_OSAL_VERSION_MAJOR << 16) | XY_OSAL_VERSION_MINOR;
        version->kernel = 0x00010000;
    }
    if (id_buf && id_size > 0) {
        /* Include platform information in kernel ID */
        int written = snprintf(id_buf, id_size, "Baremetal (%s)", 
                               xy_os_baremetal_get_platform());
        if (written < 0 || (uint32_t)written >= id_size) {
            /* Fallback if buffer too small */
            strncpy(id_buf, "Baremetal", id_size - 1);
            id_buf[id_size - 1] = '\0';
        }
    }
    return XY_OS_OK;
}

xy_os_kernel_state_t xy_os_kernel_get_state(void)
{
    return s_state;
}
xy_os_status_t xy_os_kernel_start(void)
{
    s_state = XY_OS_KERNEL_RUNNING;
    return XY_OS_OK;
}

/**
 * @brief Lock kernel (disable interrupts)
 * @return Previous lock count
 * 
 * Implementation:
 * - ARM Cortex-M: Uses PRIMASK to disable IRQ
 * - Other platforms: Counter-based (no hardware interrupt control)
 */
int32_t xy_os_kernel_lock(void)
{
    uint32_t prev_primask = __get_PRIMASK_global();
    
    /* Disable global interrupts on first lock */
    if (s_lock_count == 0) {
        __disable_irq_global();
        s_state = XY_OS_KERNEL_LOCKED;
    }
    
    s_lock_count++;
    
    /* Return previous interrupt state for restore */
    return (int32_t)prev_primask;
}

/**
 * @brief Unlock kernel (enable interrupts)
 * @return Current lock count
 */
int32_t xy_os_kernel_unlock(void)
{
    if (s_lock_count > 0) {
        s_lock_count--;
    }
    
    /* Enable interrupts when lock count reaches 0 */
    if (s_lock_count == 0) {
        s_state = XY_OS_KERNEL_RUNNING;
        __enable_irq_global();
    }
    
    return (int32_t)s_lock_count;
}

/**
 * @brief Restore previous lock state
 * @param lock Previous lock state (from xy_os_kernel_lock)
 * @return Previous lock count
 */
int32_t xy_os_kernel_restore_lock(int32_t lock)
{
    uint32_t prev = s_lock_count;
    
    /* Restore interrupt state based on lock parameter */
    if (lock == 0) {
        /* Previous state was unlocked */
        s_lock_count = 0;
        s_state = XY_OS_KERNEL_RUNNING;
        __enable_irq_global();
    } else {
        /* Previous state was locked */
        s_lock_count = 1;
        s_state = XY_OS_KERNEL_LOCKED;
        /* Interrupts already disabled */
    }
    
    return (int32_t)prev;
}

uint32_t xy_os_kernel_get_tick_count(void)
{
    return xy_tick_get();
}
uint32_t xy_os_kernel_get_tick_freq(void)
{
    return 1000;
}
uint32_t xy_os_kernel_get_sys_timer_count(void)
{
    return xy_tick_get();
}
uint32_t xy_os_kernel_get_sys_timer_freq(void)
{
    return 1000;
}

// Delay functions
xy_os_status_t xy_os_delay(uint32_t ticks)
{
    uint32_t start = xy_tick_get();
    while ((xy_tick_get() - start) < ticks)
        ;
    return XY_OS_OK;
}

xy_os_status_t xy_os_delay_until(uint32_t ticks)
{
    uint32_t now = xy_tick_get();
    return (ticks > now) ? xy_os_delay(ticks - now) : XY_OS_OK;
}

// Thread stubs
xy_os_thread_id_t xy_os_thread_new(xy_os_thread_func_t f, void *arg,
                                   const xy_os_thread_attr_t *attr)
{
    return NULL;
}
const char *xy_os_thread_get_name(xy_os_thread_id_t id)
{
    return "main";
}
xy_os_thread_id_t xy_os_thread_get_id(void)
{
    return (void *)0x1;
}
xy_os_thread_state_t xy_os_thread_get_state(xy_os_thread_id_t id)
{
    return XY_OS_THREAD_RUNNING;
}
uint32_t xy_os_thread_get_stack_size(xy_os_thread_id_t id)
{
    return 0;
}
uint32_t xy_os_thread_get_stack_space(xy_os_thread_id_t id)
{
    return 0;
}
xy_os_status_t xy_os_thread_set_priority(xy_os_thread_id_t id,
                                         xy_os_priority_t p)
{
    return XY_OS_ERROR;
}
xy_os_priority_t xy_os_thread_get_priority(xy_os_thread_id_t id)
{
    return XY_OS_PRIORITY_NORMAL;
}
xy_os_status_t xy_os_thread_yield(void)
{
    return XY_OS_OK;
}
xy_os_status_t xy_os_thread_suspend(xy_os_thread_id_t id)
{
    return XY_OS_ERROR;
}
xy_os_status_t xy_os_thread_resume(xy_os_thread_id_t id)
{
    return XY_OS_ERROR;
}
xy_os_status_t xy_os_thread_detach(xy_os_thread_id_t id)
{
    return XY_OS_ERROR;
}
xy_os_status_t xy_os_thread_join(xy_os_thread_id_t id)
{
    return XY_OS_ERROR;
}
void xy_os_thread_exit(void)
{
    while (1)
        ;
}
xy_os_status_t xy_os_thread_terminate(xy_os_thread_id_t id)
{
    return XY_OS_ERROR;
}
uint32_t xy_os_thread_get_count(void)
{
    return 1;
}
uint32_t xy_os_thread_enumerate(xy_os_thread_id_t *arr, uint32_t n)
{
    if (arr && n > 0) {
        arr[0] = xy_os_thread_get_id();
        return 1;
    }
    return 0;
}

// Thread flags stubs
uint32_t xy_os_thread_flags_set(xy_os_thread_id_t id, uint32_t f)
{
    return 0x80000000;
}
uint32_t xy_os_thread_flags_clear(uint32_t f)
{
    return 0;
}
uint32_t xy_os_thread_flags_get(void)
{
    return 0;
}
uint32_t xy_os_thread_flags_wait(uint32_t f, uint32_t opt, uint32_t to)
{
    return 0x80000000;
}

// All other primitives return NULL/error

/* Software Timer Implementation using xy_timer_sw module */
typedef struct {
    xy_timer_sw_id_t sw_timer_id;
    xy_os_timer_func_t callback;
    void *arg;
    xy_os_timer_type_t type;
    char name[16];
} baremetal_timer_ctx_t;

static baremetal_timer_ctx_t s_timers[8] = { 0 };

static void timer_sw_callback(void *arg)
{
    baremetal_timer_ctx_t *ctx = (baremetal_timer_ctx_t *)arg;
    if (ctx && ctx->callback) {
        ctx->callback(ctx->arg);
    }
}

xy_os_timer_id_t xy_os_timer_new(xy_os_timer_func_t f, xy_os_timer_type_t t,
                                 void *arg, const xy_os_timer_attr_t *attr)
{
    (void)arg;
    (void)attr;
    
    for (int i = 0; i < 8; i++) {
        if (!s_timers[i].sw_timer_id) {
            s_timers[i].callback = f;
            s_timers[i].type = t;
            s_timers[i].arg = arg;
            if (attr && attr->name) {
                strncpy(s_timers[i].name, attr->name, sizeof(s_timers[i].name) - 1);
            }
            return (xy_os_timer_id_t)(i + 1);
        }
    }
    return NULL;
}

const char *xy_os_timer_get_name(xy_os_timer_id_t id)
{
    if (!id || (uintptr_t)id > 8) {
        return NULL;
    }
    return s_timers[(uintptr_t)id - 1].name[0] ? s_timers[(uintptr_t)id - 1].name : "timer";
}

xy_os_status_t xy_os_timer_start(xy_os_timer_id_t id, uint32_t ticks)
{
    if (!id || (uintptr_t)id > 8) {
        return XY_OS_ERROR;
    }
    
    baremetal_timer_ctx_t *ctx = &s_timers[(uintptr_t)id - 1];
    uint8_t periodic = (ctx->type == XY_OS_TIMER_PERIODIC) ? 1 : 0;
    
    ctx->sw_timer_id = xy_timer_sw_create(ticks, timer_sw_callback, ctx, periodic);
    if (ctx->sw_timer_id == XY_TIMER_SW_INVALID_ID) {
        return XY_OS_ERROR;
    }
    
    return XY_OS_OK;
}

xy_os_status_t xy_os_timer_stop(xy_os_timer_id_t id)
{
    if (!id || (uintptr_t)id > 8) {
        return XY_OS_ERROR;
    }
    
    baremetal_timer_ctx_t *ctx = &s_timers[(uintptr_t)id - 1];
    if (ctx->sw_timer_id) {
        xy_timer_sw_stop(ctx->sw_timer_id);
        ctx->sw_timer_id = 0;
    }
    return XY_OS_OK;
}

uint32_t xy_os_timer_is_running(xy_os_timer_id_t id)
{
    if (!id || (uintptr_t)id > 8) {
        return 0;
    }
    return s_timers[(uintptr_t)id - 1].sw_timer_id ? 1 : 0;
}

xy_os_status_t xy_os_timer_delete(xy_os_timer_id_t id)
{
    if (!id || (uintptr_t)id > 8) {
        return XY_OS_ERROR;
    }
    
    baremetal_timer_ctx_t *ctx = &s_timers[(uintptr_t)id - 1];
    if (ctx->sw_timer_id) {
        xy_timer_sw_delete(ctx->sw_timer_id);
        ctx->sw_timer_id = 0;
    }
    ctx->callback = NULL;
    ctx->arg = NULL;
    return XY_OS_OK;
}

xy_os_event_flags_id_t
xy_os_event_flags_new(const xy_os_event_flags_attr_t *attr)
{
    return NULL;
}
const char *xy_os_event_flags_get_name(xy_os_event_flags_id_t id)
{
    return NULL;
}
uint32_t xy_os_event_flags_set(xy_os_event_flags_id_t id, uint32_t f)
{
    return 0x80000000;
}
uint32_t xy_os_event_flags_clear(xy_os_event_flags_id_t id, uint32_t f)
{
    return 0;
}
uint32_t xy_os_event_flags_get(xy_os_event_flags_id_t id)
{
    return 0;
}
uint32_t xy_os_event_flags_wait(xy_os_event_flags_id_t id, uint32_t f,
                                uint32_t opt, uint32_t to)
{
    return 0x80000000;
}
xy_os_status_t xy_os_event_flags_delete(xy_os_event_flags_id_t id)
{
    return XY_OS_ERROR;
}

xy_os_mutex_id_t xy_os_mutex_new(const xy_os_mutex_attr_t *attr)
{
    return NULL;
}
const char *xy_os_mutex_get_name(xy_os_mutex_id_t id)
{
    return NULL;
}
xy_os_status_t xy_os_mutex_acquire(xy_os_mutex_id_t id, uint32_t to)
{
    return XY_OS_ERROR;
}
xy_os_status_t xy_os_mutex_release(xy_os_mutex_id_t id)
{
    return XY_OS_ERROR;
}
xy_os_thread_id_t xy_os_mutex_get_owner(xy_os_mutex_id_t id)
{
    return NULL;
}
xy_os_status_t xy_os_mutex_delete(xy_os_mutex_id_t id)
{
    return XY_OS_ERROR;
}

xy_os_semaphore_id_t xy_os_semaphore_new(uint32_t max, uint32_t init,
                                         const xy_os_semaphore_attr_t *attr)
{
    return NULL;
}
const char *xy_os_semaphore_get_name(xy_os_semaphore_id_t id)
{
    return NULL;
}
xy_os_status_t xy_os_semaphore_acquire(xy_os_semaphore_id_t id, uint32_t to)
{
    return XY_OS_ERROR;
}
xy_os_status_t xy_os_semaphore_release(xy_os_semaphore_id_t id)
{
    return XY_OS_ERROR;
}
uint32_t xy_os_semaphore_get_count(xy_os_semaphore_id_t id)
{
    return 0;
}
xy_os_status_t xy_os_semaphore_delete(xy_os_semaphore_id_t id)
{
    return XY_OS_ERROR;
}

xy_os_mempool_id_t xy_os_mempool_new(uint32_t cnt, uint32_t sz,
                                     const xy_os_mempool_attr_t *attr)
{
    return NULL;
}
const char *xy_os_mempool_get_name(xy_os_mempool_id_t id)
{
    return NULL;
}
void *xy_os_mempool_alloc(xy_os_mempool_id_t id, uint32_t to)
{
    return NULL;
}
xy_os_status_t xy_os_mempool_free(xy_os_mempool_id_t id, void *blk)
{
    return XY_OS_ERROR;
}
uint32_t xy_os_mempool_get_capacity(xy_os_mempool_id_t id)
{
    return 0;
}
uint32_t xy_os_mempool_get_block_size(xy_os_mempool_id_t id)
{
    return 0;
}
uint32_t xy_os_mempool_get_count(xy_os_mempool_id_t id)
{
    return 0;
}
uint32_t xy_os_mempool_get_space(xy_os_mempool_id_t id)
{
    return 0;
}
xy_os_status_t xy_os_mempool_delete(xy_os_mempool_id_t id)
{
    return XY_OS_ERROR;
}

xy_os_msgqueue_id_t xy_os_msgqueue_new(uint32_t cnt, uint32_t sz,
                                       const xy_os_msgqueue_attr_t *attr)
{
    return NULL;
}
const char *xy_os_msgqueue_get_name(xy_os_msgqueue_id_t id)
{
    return NULL;
}
xy_os_status_t xy_os_msgqueue_put(xy_os_msgqueue_id_t id, const void *msg,
                                  uint8_t prio, uint32_t to)
{
    return XY_OS_ERROR;
}
xy_os_status_t xy_os_msgqueue_get(xy_os_msgqueue_id_t id, void *msg,
                                  uint8_t *prio, uint32_t to)
{
    return XY_OS_ERROR;
}
uint32_t xy_os_msgqueue_get_capacity(xy_os_msgqueue_id_t id)
{
    return 0;
}
uint32_t xy_os_msgqueue_get_msg_size(xy_os_msgqueue_id_t id)
{
    return 0;
}
uint32_t xy_os_msgqueue_get_count(xy_os_msgqueue_id_t id)
{
    return 0;
}
uint32_t xy_os_msgqueue_get_space(xy_os_msgqueue_id_t id)
{
    return 0;
}
xy_os_status_t xy_os_msgqueue_reset(xy_os_msgqueue_id_t id)
{
    return XY_OS_ERROR;
}
xy_os_status_t xy_os_msgqueue_delete(xy_os_msgqueue_id_t id)
{
    return XY_OS_ERROR;
}
