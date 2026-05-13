/**
 * @file xy_pipe.c
 * @brief IPC Pipe Implementation
 * @version 1.0.0
 * @date 2026-02-28
 */

#include "xy_pipe.h"
#include <string.h>

/* ==================== Helper Macros ==================== */

#define INCREMENT_HEAD(pipe) \
    do {                     \
        (pipe)->head++;      \
        if ((pipe)->head >= (pipe)->size) { \
            (pipe)->head = 0; \
        }                    \
    } while (0)

#define INCREMENT_TAIL(pipe) \
    do {                     \
        (pipe)->tail++;      \
        if ((pipe)->tail >= (pipe)->size) { \
            (pipe)->tail = 0; \
        }                    \
    } while (0)

/* ==================== Implementation ==================== */

int xy_pipe_init(xy_pipe_t *pipe, const char *name, uint8_t *buffer, size_t size)
{
    if (!pipe || !buffer || size == 0) {
        return XY_PIPE_INVALID_PARAM;
    }

    memset(pipe, 0, sizeof(*pipe));

    if (name) {
        strncpy(pipe->name, name, sizeof(pipe->name) - 1);
        pipe->name[sizeof(pipe->name) - 1] = '\0';
    }

    pipe->buffer = buffer;
    pipe->size = size;
    pipe->head = 0;
    pipe->tail = 0;
    pipe->count = 0;
    pipe->full = false;

    return XY_PIPE_OK;
}

int xy_pipe_deinit(xy_pipe_t *pipe)
{
    if (!pipe) {
        return XY_PIPE_INVALID_PARAM;
    }

    pipe->buffer = NULL;
    pipe->size = 0;
    pipe->head = 0;
    pipe->tail = 0;
    pipe->count = 0;
    pipe->full = false;

    return XY_PIPE_OK;
}

int xy_pipe_write(xy_pipe_t *pipe, const uint8_t *data, size_t len)
{
    size_t written = 0;

    if (!pipe || !data || len == 0) {
        return XY_PIPE_INVALID_PARAM;
    }

    while (written < len) {
        if (pipe->full) {
            /* Buffer is full, return bytes written so far */
            return written > 0 ? (int)written : XY_PIPE_BUFFER_FULL;
        }

        pipe->buffer[pipe->head] = data[written];
        INCREMENT_HEAD(pipe);
        written++;

        if (pipe->head == pipe->tail) {
            pipe->full = true;
        }
        pipe->count++;
    }

    return (int)written;
}

int xy_pipe_read(xy_pipe_t *pipe, uint8_t *data, size_t len)
{
    size_t read_count = 0;

    if (!pipe || !data || len == 0) {
        return XY_PIPE_INVALID_PARAM;
    }

    while (read_count < len) {
        if (pipe->count == 0 && !pipe->full) {
            /* Buffer is empty, return bytes read so far */
            return read_count > 0 ? (int)read_count : XY_PIPE_BUFFER_EMPTY;
        }

        data[read_count] = pipe->buffer[pipe->tail];
        INCREMENT_TAIL(pipe);
        read_count++;
        pipe->count--;

        if (pipe->count == 0) {
            pipe->full = false;
        }
    }

    return (int)read_count;
}

int xy_pipe_peek(xy_pipe_t *pipe, uint8_t *data, size_t len)
{
    size_t peek_count = 0;
    size_t temp_tail;

    if (!pipe || !data || len == 0) {
        return XY_PIPE_INVALID_PARAM;
    }

    temp_tail = pipe->tail;

    while (peek_count < len && peek_count < pipe->count) {
        data[peek_count] = pipe->buffer[temp_tail];
        temp_tail++;
        if (temp_tail >= pipe->size) {
            temp_tail = 0;
        }
        peek_count++;
    }

    return (int)peek_count;
}

size_t xy_pipe_available(xy_pipe_t *pipe)
{
    if (!pipe) {
        return 0;
    }

    return pipe->count;
}

bool xy_pipe_is_empty(xy_pipe_t *pipe)
{
    if (!pipe) {
        return true;
    }

    return (pipe->count == 0 && !pipe->full);
}

bool xy_pipe_is_full(xy_pipe_t *pipe)
{
    if (!pipe) {
        return false;
    }

    return pipe->full;
}

int xy_pipe_clear(xy_pipe_t *pipe)
{
    if (!pipe) {
        return XY_PIPE_INVALID_PARAM;
    }

    pipe->head = 0;
    pipe->tail = 0;
    pipe->count = 0;
    pipe->full = false;

    return XY_PIPE_OK;
}
