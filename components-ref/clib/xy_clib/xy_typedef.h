#ifndef _XY_TYPEDEF_H_
#define _XY_TYPEDEF_H_
#include <stdint.h>
#include <stddef.h>

/* Define ssize_t for platforms without it */
#ifndef ssize_t
typedef intptr_t ssize_t;
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
