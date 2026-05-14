#ifndef _XY_RB_H_
#define _XY_RB_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct xy_rb {
    uint8_t *buffer_ptr;
    uint32_t read_mirror : 1;
    uint32_t read_index : 31;
    uint32_t write_mirror : 1;
    uint32_t write_index : 31;
    int32_t buffer_size;
} xy_rb_t;

// Function declarations
void xy_rb_init(xy_rb_t *rb, uint8_t *pool, int32_t size);
void xy_rb_reset(xy_rb_t *rb);
size_t xy_rb_data_len(xy_rb_t *rb);
size_t xy_rb_space_len(xy_rb_t *rb);
size_t xy_rb_put(xy_rb_t *rb, const uint8_t *ptr, uint32_t length);
size_t xy_rb_put_force(xy_rb_t *rb, const uint8_t *ptr, uint32_t length);
size_t xy_rb_get(xy_rb_t *rb, uint8_t *ptr, uint32_t length);
size_t xy_rb_peek(xy_rb_t *rb, uint8_t **ptr);
size_t xy_rb_putchar(xy_rb_t *rb, const uint8_t ch);
size_t xy_rb_putchar_force(xy_rb_t *rb, const uint8_t ch);
size_t xy_rb_getchar(xy_rb_t *rb, uint8_t *ch);
xy_rb_t *xy_rb_create(uint32_t length);
void xy_rb_destroy(xy_rb_t *rb);

#ifdef __cplusplus
}
#endif

#endif