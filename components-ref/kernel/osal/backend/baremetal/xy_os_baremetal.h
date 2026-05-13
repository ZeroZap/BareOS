/**
 * @file xy_os_baremetal.h
 * @brief XY OSAL Bare-metal Backend Configuration and Documentation
 * @version 1.0.0
 * @date 2026-03-15
 * 
 * @section overview 概述
 * Bare-metal 后端提供无 RTOS 环境下的 OSAL 实现，适用于：
 * - 资源受限的 MCU (Flash < 32KB, RAM < 8KB)
 * - 简单应用无需多任务调度
 * - 性能关键代码需要确定性执行
 * 
 * @section platform_support 支持的平台
 * 
 * ### ARM Cortex-M (M0/M0+/M3/M4/M7/M33)
 * - ✅ 完整的临界区保护 (PRIMASK)
 * - ✅ 硬件中断禁用/启用
 * - ✅ 编译器支持：ARMCC, GCC, IAR
 * 
 * ### RISC-V
 * - ✅ 临界区保护 (mstatus 寄存器)
 * - ✅ 硬件中断禁用/启用
 * - ✅ 编译器支持：GCC, Clang
 * 
 * ### x86/x64 (PC/测试)
 * - ⚠️ 无硬件中断控制 (仅计数器模式)
 * - ✅ 适用于单元测试和仿真
 * 
 * @section usage 使用示例
 * 
 * ```c
 * #include "xy_os.h"
 * 
 * int main(void)
 * {
 *     // 初始化 OSAL (Bare-metal 模式)
 *     xy_os_kernel_init();
 *     
 *     // 进入临界区
 *     int32_t lock = xy_os_kernel_lock();
 *     
 *     // 临界区代码
 *     critical_section();
 *     
 *     // 退出临界区
 *     xy_os_kernel_unlock();
 *     
 *     // 或使用恢复模式
 *     lock = xy_os_kernel_lock();
 *     // ... 临界区代码 ...
 *     xy_os_kernel_restore_lock(lock);
 *     
 *     return 0;
 * }
 * ```
 * 
 * @section critical_section 临界区实现
 * 
 * ### ARM Cortex-M
 * ```c
 * // 使用 PRIMASK 寄存器禁用所有 IRQ
 * __disable_irq_global();  // cpsid i
 * __enable_irq_global();   // cpsie i
 * ```
 * 
 * ### RISC-V
 * ```c
 * // 使用 mstatus.MIE 位禁用中断
 * uint32_t mstatus = __get_MSTATUS();
 * __clear_MIE();  // 清除 MIE 位
 * // ... 临界区代码 ...
 * __set_MSTATUS(mstatus);  // 恢复
 * ```
 * 
 * @section limitations 限制
 * - ❌ 不支持多线程 (所有线程 API 返回错误)
 * - ❌ 不支持信号量/互斥量 (返回错误)
 * - ❌ 不支持消息队列 (返回错误)
 * - ✅ 支持软件定时器 (基于 xy_timer_sw)
 * - ✅ 支持事件标志 ( stub 实现)
 * 
 * @section performance 性能特性
 * - 中断禁用延迟：< 10 个周期 (ARM Cortex-M)
 * - 时间精度：取决于 xy_tick 实现 (通常 1ms)
 * - 内存占用：~500 字节 (仅内核状态)
 * 
 * @note 在 Bare-metal 模式下，所有"线程"实际都在主循环中运行
 * @note 适用于 cooperative multitasking 或无多任务应用
 */

#ifndef _XY_OS_BAREMETAL_H_
#define _XY_OS_BAREMETAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Platform Detection ==================== */

/* ARM Cortex-M */
#if defined(__ARM_ARCH) || defined(__ARM_ARCH_6M__) || defined(__ARM_ARCH_7M__) || \
    defined(__ARM_ARCH_7EM__) || defined(__ARM_ARCH_8M_BASE__) || \
    defined(__ARM_ARCH_8M_MAIN__)
    #define XY_OS_BAREMETAL_ARM_CM 1
#endif

/* RISC-V */
#if defined(__riscv) || defined(__riscv__)
    #define XY_OS_BAREMETAL_RISCV 1
#endif

/* x86/x64 */
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #define XY_OS_BAREMETAL_X86 1
#endif

/* ==================== Interrupt Control Macros ==================== */

/**
 * @brief Disable global interrupts (platform-specific)
 * @note Use xy_os_kernel_lock() instead for portability
 */
#define XY_OS_BM_DISABLE_IRQ() __disable_irq_global()

/**
 * @brief Enable global interrupts (platform-specific)
 * @note Use xy_os_kernel_unlock() instead for portability
 */
#define XY_OS_BM_ENABLE_IRQ() __enable_irq_global()

/**
 * @brief Get current interrupt state (platform-specific)
 * @return 0 if interrupts enabled, non-zero if disabled
 */
#define XY_OS_BM_GET_IRQ_STATE() __get_PRIMASK_global()

/* ==================== Configuration ==================== */

/**
 * @brief Maximum number of software timers in Bare-metal mode
 * @note Can be overridden in xy_os_cfg.h
 */
#ifndef XY_OS_BAREMETAL_MAX_TIMERS
    #define XY_OS_BAREMETAL_MAX_TIMERS  8
#endif

/**
 * @brief Enable RISC-V specific interrupt control
 * @note Only used on RISC-V platforms
 */
#ifndef XY_OS_BAREMETAL_RISCV_USE_MSTATUS
    #define XY_OS_BAREMETAL_RISCV_USE_MSTATUS  1
#endif

#ifdef __cplusplus
}
#endif

#endif /* _XY_OS_BAREMETAL_H_ */
