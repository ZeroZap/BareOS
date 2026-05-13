/**
 * @file xy_rb.c
 * @brief Implementation of ring buffer functions for XY platform
 */

#include "xy_rb.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

/**
 * @brief Initialize a ring buffer
 */
void xy_rb_init(xy_rb_t *rb, uint8_t *pool, int32_t size)
{
    if (!rb || !pool || size <= 0)
        return;

    /* initialize read and write index */
    rb->read_mirror = rb->read_index = 0;
    rb->write_mirror = rb->write_index = 0;

    /* set buffer pool and size */
    rb->buffer_ptr  = pool;
    rb->buffer_size = size;
}

/**
 * @brief Reset the ring buffer to empty state
 */
void xy_rb_reset(xy_rb_t *rb)
{
    if (!rb)
        return;

    rb->read_mirror = rb->read_index = 0;
    rb->write_mirror = rb->write_index = 0;
}

/**
 * @brief Get the size of data in ring buffer
 */
size_t xy_rb_data_len(xy_rb_t *rb)
{
    if (!rb)
        return 0;

    uint32_t read_index, write_index;

    read_index  = rb->read_index;
    write_index = rb->write_index;

    if (rb->read_mirror == rb->write_mirror) {
        /* same mirror, ring buffer is empty or full */
        if (read_index > write_index)
            return rb->buffer_size - read_index + write_index;
        else
            return write_index - read_index;
    } else {
        /* different mirror, ring buffer is neither empty nor full */
        return rb->buffer_size - read_index + write_index;
    }
}

/**
 * @brief Get the available space in ring buffer
 */
size_t xy_rb_space_len(xy_rb_t *rb)
{
    if (!rb)
        return 0;

    return rb->buffer_size - xy_rb_data_len(rb);
}

/**
 * @brief Put data into the ring buffer
 */
size_t xy_rb_put(xy_rb_t *rb, const uint8_t *ptr, uint32_t length)
{
    if (!rb || !ptr || length == 0)
        return 0;

    uint32_t size;

    /* available space in ring buffer */
    size = xy_rb_space_len(rb);

    /* no space */
    if (size == 0)
        return 0;

    /* drop some data */
    if (size < length)
        length = size;

    if (rb->buffer_size - rb->write_index > length) {
        /* copy all of data */
        memcpy(&rb->buffer_ptr[rb->write_index], ptr, length);

        /* this should not cause overflow because there is enough space for
         * length of data in current mirror */
        rb->write_index += length;
        return length;
    } else {
        /* copy first part */
        size = rb->buffer_size - rb->write_index;
        memcpy(&rb->buffer_ptr[rb->write_index], ptr, size);

        /* copy second part */
        memcpy(rb->buffer_ptr, &ptr[size], length - size);

        /* we are going into the other mirror */
        rb->write_mirror = ~rb->write_mirror;
        rb->write_index  = length - size;

        return length;
    }
}

/**
 * @brief Put a character into the ring buffer
 */
size_t xy_rb_putchar(xy_rb_t *rb, const uint8_t ch)
{
    if (!rb)
        return 0;

    /* whether has enough space */
    if (xy_rb_space_len(rb) == 0)
        return 0;

    rb->buffer_ptr[rb->write_index] = ch;

    /* flip mirror if necessary */
    if (rb->write_index == rb->buffer_size - 1) {
        rb->write_mirror = ~rb->write_mirror;
        rb->write_index  = 0;
    } else {
        rb->write_index++;
    }

    return 1;
}

/**
 * @brief Force put a character into the ring buffer, overwriting if necessary
 */
size_t xy_rb_putchar_force(xy_rb_t *rb, const uint8_t ch)
{
    if (!rb)
        return 0;

    /* buffer is full, discard one character */
    if (xy_rb_space_len(rb) == 0) {
        /* flip mirror if necessary */
        if (rb->read_index == rb->buffer_size - 1) {
            rb->read_mirror = ~rb->read_mirror;
            rb->read_index  = 0;
        } else {
            rb->read_index++;
        }
    }

    rb->buffer_ptr[rb->write_index] = ch;

    /* flip mirror if necessary */
    if (rb->write_index == rb->buffer_size - 1) {
        rb->write_mirror = ~rb->write_mirror;
        rb->write_index  = 0;
    } else {
        rb->write_index++;
    }

    return 1;
}

/**
 * @brief Force put data into the ring buffer, overwriting old data if necessary
 */
size_t xy_rb_put_force(xy_rb_t *rb, const uint8_t *ptr, uint32_t length)
{
    if (!rb || !ptr || length == 0)
        return 0;

    uint32_t space_length;

    space_length = xy_rb_space_len(rb);

    /* if there is not enough space, discard old data */
    if (length > space_length) {
        uint32_t data_length;
        data_length = length - space_length;

        if (data_length < rb->buffer_size) {
            if (rb->read_index + data_length < rb->buffer_size) {
                /* move read index */
                rb->read_index += data_length;
            } else {
                /* move read index with mirror change */
                rb->read_mirror = ~rb->read_mirror;
                rb->read_index =
                    data_length - (rb->buffer_size - rb->read_index);
            }
        } else {
            /* data length > buffer size, empty the buffer and put only the last
             * buffer_size bytes */
            ptr             = &ptr[length - rb->buffer_size];
            length          = rb->buffer_size;
            rb->read_mirror = ~rb->read_mirror;
            rb->read_index  = rb->write_index;
        }
    }

    return xy_rb_put(rb, ptr, length);
}

/**
 * @brief Get data from the ring buffer
 */
size_t xy_rb_get(xy_rb_t *rb, uint8_t *ptr, uint32_t length)
{
    if (!rb || !ptr || length == 0)
        return 0;

    uint32_t size;

    /* available data in ring buffer */
    size = xy_rb_data_len(rb);

    /* no data */
    if (size == 0)
        return 0;

    /* drop some data */
    if (size < length)
        length = size;

    if (rb->buffer_size - rb->read_index > length) {
        /* copy all of data */
        memcpy(ptr, &rb->buffer_ptr[rb->read_index], length);

        /* this should not cause overflow because there is enough space for
         * length of data in current mirror */
        rb->read_index += length;
        return length;
    } else {
        /* copy first part */
        size = rb->buffer_size - rb->read_index;
        memcpy(ptr, &rb->buffer_ptr[rb->read_index], size);

        /* copy second part */
        memcpy(&ptr[size], rb->buffer_ptr, length - size);

        /* we are going into the other mirror */
        rb->read_mirror = ~rb->read_mirror;
        rb->read_index  = length - size;

        return length;
    }
}

/**
 * @brief Peek data from the ring buffer without changing read index
 */
size_t xy_rb_peek(xy_rb_t *rb, uint8_t **ptr)
{
    if (!rb || !ptr)
        return 0;

    *ptr = &rb->buffer_ptr[rb->read_index];

    if (rb->read_index > rb->write_index) {
        if (rb->read_mirror == rb->write_mirror)
            return rb->buffer_size - rb->read_index;
        else
            return rb->buffer_size - rb->read_index;
    } else {
        if (rb->read_mirror == rb->write_mirror)
            return rb->write_index - rb->read_index;
        else
            return rb->buffer_size - rb->read_index;
    }
}

/**
 * @brief Get a character from the ring buffer
 */
size_t xy_rb_getchar(xy_rb_t *rb, uint8_t *ch)
{
    if (!rb || !ch)
        return 0;

    /* ring buffer is empty */
    if (!xy_rb_data_len(rb))
        return 0;

    /* put character */
    *ch = rb->buffer_ptr[rb->read_index];

    if (rb->read_index == rb->buffer_size - 1) {
        rb->read_mirror = ~rb->read_mirror;
        rb->read_index  = 0;
    } else {
        rb->read_index++;
    }

    return 1;
}

/**
 * @brief Create a ring buffer object
 */
xy_rb_t *xy_rb_create(uint32_t length)
{
    if (length == 0)
        return NULL;

    xy_rb_t *rb = (xy_rb_t *)malloc(sizeof(xy_rb_t));
    if (!rb)
        return NULL;

    uint8_t *pool = (uint8_t *)malloc(length);
    if (!pool) {
        free(rb);
        return NULL;
    }

    xy_rb_init(rb, pool, length);

    return rb;
}

/**
 * @brief Destroy a ring buffer object
 */
void xy_rb_destroy(xy_rb_t *rb)
{
    if (!rb)
        return;

    if (rb->buffer_ptr)
        free(rb->buffer_ptr);

    free(rb);
}