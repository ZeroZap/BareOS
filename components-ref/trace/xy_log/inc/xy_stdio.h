/**
 * @file xy_stdio.h
 * @brief Standard I/O Wrapper
 */

#ifndef XY_STDIO_H
#define XY_STDIO_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* Wrapper functions */
#define xy_printf printf
#define xy_sprintf sprintf
#define xy_snprintf snprintf
#define xy_putchar putchar
#define xy_getchar getchar

#endif /* XY_STDIO_H */
