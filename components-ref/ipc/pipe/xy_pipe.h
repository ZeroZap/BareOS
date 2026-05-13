/**
 * @file xy_pipe.h
 * @brief IPC Pipe - Inter-Process Communication Pipe
 * @version 1.0.0
 * @date 2026-02-28
 */

#ifndef XY_PIPE_H
#define XY_PIPE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Configuration ==================== */

#ifndef XY_PIPE_BUFFER_SIZE
#define XY_PIPE_BUFFER_SIZE 256
#endif

#ifndef XY_PIPE_MAX_PIPES
#define XY_PIPE_MAX_PIPES 16
#endif

/* ==================== Error Codes ==================== */

#define XY_PIPE_OK              0
#define XY_PIPE_ERROR           (-1)
#define XY_PIPE_INVALID_PARAM   (-2)
#define XY_PIPE_BUFFER_FULL     (-3)
#define XY_PIPE_BUFFER_EMPTY    (-4)
#define XY_PIPE_NOT_FOUND       (-5)

/* ==================== Data Structures ==================== */

/**
 * @brief Pipe structure
 */
typedef struct {
    char name[32];
    uint8_t *buffer;
    size_t size;
    size_t head;
    size_t tail;
    size_t count;
    bool full;
    uint32_t flags;
} xy_pipe_t;

/* ==================== Pipe Operations ==================== */

/**
 * @brief Initialize a pipe
 * @param pipe Pipe handle
 * @param name Pipe name
 * @param buffer Buffer pointer
 * @param size Buffer size
 * @return XY_PIPE_OK on success
 */
int xy_pipe_init(xy_pipe_t *pipe, const char *name, uint8_t *buffer, size_t size);

/**
 * @brief Deinitialize a pipe
 * @param pipe Pipe handle
 * @return XY_PIPE_OK on success
 */
int xy_pipe_deinit(xy_pipe_t *pipe);

/**
 * @brief Write data to pipe
 * @param pipe Pipe handle
 * @param data Data to write
 * @param len Data length
 * @return Number of bytes written, or negative error code
 */
int xy_pipe_write(xy_pipe_t *pipe, const uint8_t *data, size_t len);

/**
 * @brief Read data from pipe
 * @param pipe Pipe handle
 * @param data Buffer to read into
 * @param len Buffer size
 * @return Number of bytes read, or negative error code
 */
int xy_pipe_read(xy_pipe_t *pipe, uint8_t *data, size_t len);

/**
 * @brief Peek data from pipe without removing
 * @param pipe Pipe handle
 * @param data Buffer to peek into
 * @param len Buffer size
 * @return Number of bytes peeked
 */
int xy_pipe_peek(xy_pipe_t *pipe, uint8_t *data, size_t len);

/**
 * @brief Get available bytes in pipe
 * @param pipe Pipe handle
 * @return Number of bytes available
 */
size_t xy_pipe_available(xy_pipe_t *pipe);

/**
 * @brief Check if pipe is empty
 * @param pipe Pipe handle
 * @return true if empty
 */
bool xy_pipe_is_empty(xy_pipe_t *pipe);

/**
 * @brief Check if pipe is full
 * @param pipe Pipe handle
 * @return true if full
 */
bool xy_pipe_is_full(xy_pipe_t *pipe);

/**
 * @brief Clear pipe contents
 * @param pipe Pipe handle
 * @return XY_PIPE_OK on success
 */
int xy_pipe_clear(xy_pipe_t *pipe);

#ifdef __cplusplus
}
#endif

#endif /* XY_PIPE_H */
