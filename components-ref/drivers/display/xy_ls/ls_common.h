#ifndef _LS_COMMON_H_
#define _LS_COMMON_H_
#define ls_free free
#define ls_malloc malloc
#define ls_mem_set memset
#define CODE

#ifndef NULL
#define NULL
#endif

typedef signed char ls_int8_t;
typedef unsigned char ls_uint8_t;
typedef signed int ls_int16_t;
typedef unsigned int ls_uint16_t;
typedef signed long ls_int32_t;
typedef unsigned long ls_uint32_t;
typedef unsigned int ls_size_t;
typedef signed int ls_ssize_t;

#endif