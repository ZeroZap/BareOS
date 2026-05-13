#ifndef _XY_rbl_H_
#define _XY_rbl_H_
#include <stdint.h>
typedef struct {
    uint8_t *buffer;
    volatile uint16_t head;
    volatile uint16_t tail;
    volatile uint16_t count;
    volatile uint16_t size;
} xy_rbl_t;

#define xy_rbl_full(rbl)  ((rbl)->count >= (rbl)->size)
#define xy_rbl_empty(rbl) ((rbl)->count == 0)

#define xy_rbl_put(rbl, ch)                                                     \
    do {                                                                        \
        if (!xy_rbl_full(rbl)) {                                                \
            (rbl)->buffer[(rbl)->tail] = (ch);                                  \
            (rbl)->tail                = ((rbl)->tail + 1) & ((rbl)->size - 1); \
            (rbl)->count++;                                                     \
        }                                                                       \
    } while (0)

#define xy_rbl_put_force(rbl, ch)                                           \
    do {                                                                    \
        (rbl)->buffer[(rbl)->tail] = (ch);                                  \
        (rbl)->tail                = ((rbl)->tail + 1) & ((rbl)->size - 1); \
        if ((rbl)->count < (rbl)->size)                                     \
            (rbl)->count++;                                                 \
        else                                                                \
            (rbl)->head = ((rbl)->head + 1) & ((rbl)->size - 1);            \
    } while (0)

#define xy_rbl_get(rbl)                                          \
    ({                                                           \
        uint8_t ch = 0;                                          \
        if (!xy_rbl_empty(rbl)) {                                \
            ch          = (rbl)->buffer[(rbl)->head];            \
            (rbl)->head = ((rbl)->head + 1) & ((rbl)->size - 1); \
            (rbl)->count--;                                      \
        }                                                        \
        ch;                                                      \
    })

#endif