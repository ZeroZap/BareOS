/* comp_port.h
 *
 * Unified compiler/architecture portability layer for:
 * - Keil ARMCC5
 * - Keil ARMCLANG (ARM Compiler 6)
 * - IAR
 * - GCC/Clang (ARM / RISC-V / PC x86/x64)
 *
 * Features:
 * - Compiler detection: GCC/Clang/ARMCLANG/ARMCC5/IAR
 * - Arch detection: ARM / RISC-V / x86
 * - Attributes:
 * WEAK/USED/UNUSED/NORETURN/PACKED/ALIGNED/SECTION/ALWAYS_INLINE/NOINLINE
 * - IAR pragma helpers: IAR_PRAGMA/IAR_LOC/IAR_ALIGN + NOINIT
 * - Basic helpers: ARRAY_SIZE/STATIC_ASSERT
 * - Bare-metal-ish helpers: irq_enable/irq_disable + mem_barrier_full
 *
 * Notes:
 * - SECTION/NOINIT/RAMFUNC still require linker config (ld/sct/icf).
 * - irq_enable/disable are no-ops on PC/x86 and unknown architectures.
 */

#ifndef __XY_COMP_H__
#define __XY_COMP_H__

#ifdef __cplusplus
extern "C" {
#endif

/*==========================================================
  1) Compiler detection
==========================================================*/
#if defined(__IAR_SYSTEMS_ICC__)
#define COMPILER_IAR 1

#elif defined(__CC_ARM) && !defined(__ARMCC_VERSION)
/* Keil MDK: ARMCC 5.x */
#define COMPILER_ARMCC5 1

#elif defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6000000)
/* Keil MDK: ARM Compiler 6 (armclang) */
#define COMPILER_ARMCLANG 1

#elif defined(__clang__)
#define COMPILER_CLANG 1

#elif defined(__GNUC__)
#define COMPILER_GCC 1

#else
#warning "Unknown compiler"
#endif


/*==========================================================
  2) Architecture detection (ARM / RISC-V / x86)
==========================================================*/
#if defined(__riscv) || defined(__riscv_xlen)
#define ARCH_RISCV 1
#elif defined(__arm__) || defined(__thumb__) || defined(__ARM_ARCH) \
    || defined(__ARM_ARCH_ISA_THUMB)
#define ARCH_ARM 1
#elif defined(__i386__) || defined(__x86_64__)
#define ARCH_X86 1
#else
#define ARCH_UNKNOWN 1
#endif


/*==========================================================
  3) Attribute/keyword abstraction
==========================================================*/
#if defined(COMPILER_GCC) || defined(COMPILER_CLANG) \
    || defined(COMPILER_ARMCLANG)

#define XY_WEAK          __attribute__((weak))
#define XY_USED          __attribute__((used))
#define XY_UNUSED        __attribute__((unused))
#define XY_NORETURN      __attribute__((noreturn))
#define XY_PACKED        __attribute__((packed))
#define XY_ALIGNED(n)    __attribute__((aligned(n)))
#define XY_SECTION(name) __attribute__((section(name)))
#define XY_ALWAYS_INLINE __attribute__((always_inline)) inline
#define XY_NOINLINE      __attribute__((noinline))

#define XY_LIKELY(x)   __builtin_expect(!!(x), 1)
#define XY_UNLIKELY(x) __builtin_expect(!!(x), 0)

#define XY_COMPILER_BARRIER() __asm__ volatile("" ::: "memory")

#elif defined(COMPILER_ARMCC5)

#define XY_WEAK          __weak
#define XY_USED          __attribute__((used))
#define XY_UNUSED        __attribute__((unused))
#define XY_NORETURN      __attribute__((noreturn))
#define XY_PACKED        __packed
#define XY_ALIGNED(n)    __align(n)
#define XY_SECTION(name) __attribute__((section(name)))
#define XY_ALWAYS_INLINE __forceinline
#define XY_NOINLINE      __attribute__((noinline))

#define XY_LIKELY(x)   (x)
#define XY_UNLIKELY(x) (x)

#define XY_COMPILER_BARRIER() __asm volatile("" ::: "memory")

#elif defined(COMPILER_IAR)

/* IAR notes:
 * - USED: __root keeps symbol.
 * - SECTION(name): works in some contexts as: var @ "SEC";
 *   For robust placement, prefer IAR_LOC("SEC") or @.
 */
#define XY_WEAK          __weak
#define XY_USED          __root
#define XY_UNUSED        /* no direct equivalent; use (void)x */
#define XY_NORETURN      __noreturn
#define XY_PACKED        __packed

/* IAR alignment/section are usually handled via pragmas.
 * Keep ALIGNED/SECTION minimal; provide pragma helpers below.
 */
#define XY_ALIGNED(n)    /* prefer IAR_ALIGN(n) on objects */
#define XY_SECTION(name) @name

#define XY_ALWAYS_INLINE inline
#define XY_NOINLINE

#define XY_LIKELY(x)   (x)
#define XY_UNLIKELY(x) (x)

#define XY_COMPILER_BARRIER() \
    do {                      \
    } while (0)

#else

/* Fallback (best-effort) */
#define XY_WEAK
#define XY_USED
#define XY_UNUSED
#define XY_NORETURN
#define XY_PACKED
#define XY_ALIGNED(n)
#define XY_SECTION(name)
#define XY_ALWAYS_INLINE inline
#define XY_NOINLINE
#define XY_LIKELY(x)   (x)
#define XY_UNLIKELY(x) (x)
#define XY_COMPILER_BARRIER() \
    do {                      \
    } while (0)

#endif


/*==========================================================
  4) IAR pragma helpers (macro-friendly)
==========================================================*/
#if defined(COMPILER_IAR)
#define XY_IAR_PRAGMA(x) _Pragma(#x)
/* Example: IAR_LOC(".noinit") __no_init uint32_t x; */
#define XY_IAR_LOC(sec_literal) IAR_PRAGMA(location = sec_literal)
/* Example: IAR_ALIGN(32) uint8_t buf[64]; */
#define XY_IAR_ALIGN(n) IAR_PRAGMA(data_alignment = n)
#define XY_NOINIT       __no_init
#else
#define XY_IAR_PRAGMA(x)
#define XY_IAR_LOC(sec_literal)
#define XY_IAR_ALIGN(n)
#define XY_NOINIT
#endif


/*==========================================================
  5) Common helpers
==========================================================*/
#ifndef ARRAY_SIZE
#define XY_ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

#ifndef STATIC_ASSERT
#if defined(__cplusplus)
#define XY_STATIC_ASSERT(cond, msg) static_assert(cond, msg)
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#define XY_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
#else
#define XY_STATIC_ASSERT(cond, msg) \
    typedef char static_assertion_##__LINE__[(cond) ? 1 : -1]
#endif
#endif


/*==========================================================
  6) IRQ enable/disable (bare-metal oriented)
  - ARM: assumes Cortex-M (cpsid/cpsie).
  - RISC-V: assumes M-mode, uses mstatus.MIE.
  - x86/PC: no-op (so host builds compile).
==========================================================*/
static ALWAYS_INLINE void irq_disable(void)
{
#if defined(ARCH_ARM)
    __asm__ volatile("cpsid i" ::: "memory");
#elif defined(ARCH_RISCV)
    __asm__ volatile("csrc mstatus, %0" ::"r"(0x8) : "memory"); /* clear MIE */
#else
    (void)0; /* host/unknown */
#endif
}

static ALWAYS_INLINE void irq_enable(void)
{
#if defined(ARCH_ARM)
    __asm__ volatile("cpsie i" ::: "memory");
#elif defined(ARCH_RISCV)
    __asm__ volatile("csrs mstatus, %0" ::"r"(0x8) : "memory"); /* set MIE */
#else
    (void)0; /* host/unknown */
#endif
}


/*==========================================================
  7) Memory barrier (coarse, cross-arch)
==========================================================*/
static ALWAYS_INLINE void mem_barrier_full(void)
{
#if defined(ARCH_ARM)
    __asm__ volatile("dmb 0xF" ::: "memory");
#elif defined(ARCH_RISCV)
    __asm__ volatile("fence iorw, iorw" ::: "memory");
#elif (defined(COMPILER_GCC) || defined(COMPILER_CLANG) \
       || defined(COMPILER_ARMCLANG))
    /* Works on PC GCC/Clang as well */
    __sync_synchronize();
#else
    COMPILER_BARRIER();
#endif
}


/*==========================================================
  8) Section placement convenience macros (practical)
  Provide unified wrappers for typical cases:
  - NOINIT data
  - RAMFUNC (run from RAM)
==========================================================*/

/* NOINIT variable:
 * - GCC/ARMCLANG: use SECTION(".noinit")
 * - IAR: use IAR_LOC(".noinit") NOINIT
 *
 * Usage:
 *   NOINIT_VAR(".noinit", uint32_t g_boot_flag;);
 */
#if defined(COMPILER_IAR)
#define XY_NOINIT_VAR(sec_literal, decl) IAR_LOC(sec_literal) NOINIT decl
#else
#define XY_NOINIT_VAR(sec_literal, decl) SECTION(sec_literal) decl
#endif

/* RAMFUNC:
 * NOTE: you still must configure linker (ld/sct/icf) to copy/locate properly.
 * Usage:
 *   RAMFUNC void flash_prog(void) { ... }
 */
#if defined(COMPILER_IAR)
#define XY_RAMFUNC __ramfunc
#else
#define XY_RAMFUNC SECTION(".ramfunc")
#endif


#ifdef __cplusplus
}
#endif

#endif