#ifndef _XY_TYPEDEF_H_
#define _XY_TYPEDEF_H_
#include <stdint.h>

/* ── size_t ──────────────────────────────────────────────────────────
 * Derived from stdint.h (allowed) without pulling in stddef.h.
 * GCC/Clang expose __SIZE_TYPE__ as a compiler built-in; fall back to
 * uintptr_t which has the same width as a pointer on all platforms.
 */
#ifndef __size_t
#define __size_t
#  ifdef __SIZE_TYPE__
     typedef __SIZE_TYPE__ size_t;
#  else
     typedef uintptr_t     size_t;
#  endif
#endif

/* ── ssize_t ─────────────────────────────────────────────────────────── */
#ifndef ssize_t
typedef intptr_t ssize_t;
#endif

/* ── NULL ────────────────────────────────────────────────────────────── */
#ifndef NULL
#  ifdef __cplusplus
#    define NULL 0
#  else
#    define NULL ((void *)0)
#  endif
#endif

/* ── bool / true / false (C99 _Bool without stdbool.h) ──────────────── */
#ifndef __cplusplus
#  ifndef bool
#    define bool  _Bool
#    define true  1
#    define false 0
#  endif
#endif

/* ── offsetof (compiler built-in, no stddef.h needed) ───────────────── */
#ifndef offsetof
#  ifdef __builtin_offsetof
#    define offsetof(type, member)  __builtin_offsetof(type, member)
#  else
#    define offsetof(type, member)  ((size_t)&((type *)0)->member)
#  endif
#endif

/* ── Integer limits (avoids limits.h) ───────────────────────────────── */
#ifndef CHAR_MIN
#  define CHAR_MIN  (-128)
#  define CHAR_MAX  127
#  define UCHAR_MAX 255U
#endif
#ifndef INT_MAX
#  define INT_MAX   0x7FFFFFFF
#  define INT_MIN   (-INT_MAX - 1)
#  define UINT_MAX  0xFFFFFFFFU
#endif
#ifndef LONG_MAX
#  ifdef __LONG_MAX__
#    define LONG_MAX  __LONG_MAX__
#  elif defined(__LP64__) || defined(_LP64)
#    define LONG_MAX  0x7FFFFFFFFFFFFFFFL
#  else
#    define LONG_MAX  0x7FFFFFFFL
#  endif
#  define LONG_MIN  (-LONG_MAX - 1L)
#endif
#ifdef __cplusplus
extern "C" {
#endif

#ifndef XY_NULL
#define XY_NULL ((void *)0)
#endif

#define XY_TRUE  1
#define XY_FALSE 0


#define XY_U8_MAX  0xFF
#define XY_I8_MAX  0x7F
#define XY_U16_MAX 0xFFFF
#define XY_I16_MAX 0x7FFF
#define XY_U32_MAX 0xFFFFFFFF
#define XY_I32_MAX 0x7FFFFFFF
#define XY_U64_MAX 0xFFFFFFFFFFFFFFFF
#define XY_I64_MIN 0x80000000000000000L
#define XY_I64_MAX 0x7FFFFFFFFFFFFFFF

#define xy_success 0

typedef uint8_t xy_u8;
typedef uint8_t xy_uint8_t;
typedef int8_t xy_i8;
typedef int8_t xy_int8_t;
typedef uint16_t xy_u16;
typedef uint16_t xy_uint16_t;
typedef int16_t xy_i16;
typedef int16_t xy_int16_t;
typedef uint32_t xy_u32;
typedef uint32_t xy_uint32_t;
typedef int32_t xy_i32;
typedef int32_t xy_int32_t;
typedef uint64_t xy_u64;
typedef uint64_t xy_uint64_t;
typedef int64_t xy_i64;
typedef int64_t xy_int64_t;
typedef uint8_t xy_bool;
typedef size_t xy_size_t;
typedef ssize_t xy_ssize_t;
typedef xy_ssize_t xy_base_t;
typedef xy_size_t xy_ubase_t;

#ifdef __cplusplus
}
#endif

#endif
