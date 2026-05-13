/**
 * @file xy_at_client.h
 * @brief AT Command Client Implementation for XinYi Framework
 * @version 1.0.0
 * @date 2025-10-27
 *
 * @note This implementation uses XY OSAL for cross-platform compatibility
 */

#ifndef _XY_AT_CLIENT_H_
#define _XY_AT_CLIENT_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Configuration ==================== */

#ifndef XY_AT_CLIENT_NUM_MAX
#define XY_AT_CLIENT_NUM_MAX 1 ///< Maximum number of AT clients
#endif

#ifndef XY_AT_CMD_MAX_LEN
#define XY_AT_CMD_MAX_LEN 256 ///< Maximum AT command length
#endif

#ifndef XY_AT_RESP_MAX_LEN
#define XY_AT_RESP_MAX_LEN 1024 ///< Maximum response buffer length
#endif

#ifndef XY_AT_RECV_LINE_MAX_LEN
#define XY_AT_RECV_LINE_MAX_LEN 256 ///< Maximum line length
#endif

#ifndef XY_AT_DEFAULT_TIMEOUT
#define XY_AT_DEFAULT_TIMEOUT 5000 ///< Default timeout (ms)
#endif

#ifndef XY_AT_MAX_RETRY
#define XY_AT_MAX_RETRY 3 ///< Maximum retry count
#endif

#ifndef XY_AT_URC_TABLE_MAX
#define XY_AT_URC_TABLE_MAX 16 ///< Maximum URC handlers
#endif

/* ==================== Status and Result Codes ==================== */

/**
 * @brief AT client status
 */
typedef enum {
    XY_AT_STATUS_UNINITIALIZED = 0,
    XY_AT_STATUS_INITIALIZED,
    XY_AT_STATUS_IDLE,
    XY_AT_STATUS_BUSY,
    XY_AT_STATUS_DATA_MODE,
} xy_at_status_t;

/**
 * @brief AT response status
 */
typedef enum {
    XY_AT_RESP_OK        = 0,  ///< Response is OK
    XY_AT_RESP_ERROR     = -1, ///< Response is ERROR
    XY_AT_RESP_TIMEOUT   = -2, ///< Response timeout
    XY_AT_RESP_BUFF_FULL = -3, ///< Response buffer full
    XY_AT_RESP_INVALID   = -4, ///< Invalid response
} xy_at_resp_status_t;

/* ==================== Data Structures ==================== */

struct xy_at_client;

/**
 * @brief AT response structure
 */
typedef struct xy_at_response {
    char *buf;          ///< Response buffer
    size_t buf_size;    ///< Maximum buffer size
    size_t buf_len;     ///< Current buffer length
    size_t line_num;    ///< Expected line number (0 = wait for OK/ERROR)
    size_t line_counts; ///< Received line count
    uint32_t timeout;   ///< Response timeout (ms)
} xy_at_response_t;

/**
 * @brief URC (Unsolicited Result Code) handler
 */
typedef struct xy_at_urc {
    const char *prefix; ///< URC prefix (e.g., "+CREG:")
    const char *suffix; ///< URC suffix (optional)
    void (*func)(struct xy_at_client *client, const char *data, size_t size);
} xy_at_urc_t;

/**
 * @brief URC table
 */
typedef struct xy_at_urc_table {
    size_t urc_count;
    const xy_at_urc_t *urc;
} xy_at_urc_table_t;

/**
 * @brief AT client structure
 */
typedef struct xy_at_client {
    const char *name;      ///< Client name
    xy_at_status_t status; ///< Current status
    char end_sign;         ///< End sign character

    // HAL interface
    int (*get_char)(char *ch, uint32_t timeout);  ///< Get character from device
    size_t (*send)(const char *data, size_t len); ///< Send data to device
    size_t (*recv)(char *data, size_t len);       ///< Receive data from device

    // Send buffer
    char *send_buf;
    size_t send_buf_size;
    size_t last_cmd_len;

    // Receive line buffer
    char *recv_line_buf;
    size_t recv_line_len;
    size_t recv_line_size;

    // OSAL primitives
    void *lock;        ///< Mutex (xy_os_mutex_id_t)
    void *rx_notice;   ///< Semaphore for RX notification
    void *resp_notice; ///< Semaphore for response notification

    // Response handling
    xy_at_response_t *resp;
    xy_at_resp_status_t resp_status;

    // URC handling
    xy_at_urc_table_t *urc_table;
    size_t urc_table_size;

    // Parser thread
    void *parser_thread; ///< Thread handle (xy_os_thread_id_t)
    bool parser_running;

    // Statistics
    uint32_t tx_count;
    uint32_t rx_count;
    uint32_t error_count;
    uint32_t timeout_count;
} xy_at_client_t;

/* ==================== Client Management ==================== */

/**
 * @brief Create AT client
 * @param name Client name
 * @param send_buf_size Send buffer size
 * @param recv_buf_size Receive buffer size
 * @return Pointer to client or NULL on error
 */
xy_at_client_t *xy_at_client_create(const char *name, size_t send_buf_size,
                                    size_t recv_buf_size);

/**
 * @brief Initialize AT client
 * @param client Pointer to client structure
 * @param name Client name
 * @return 0 on success, -1 on error
 */
int xy_at_client_init(xy_at_client_t *client, const char *name);

/**
 * @brief Delete AT client
 * @param client Pointer to client
 */
void xy_at_client_delete(xy_at_client_t *client);

/**
 * @brief Set AT client HAL interface
 * @param client Pointer to client
 * @param get_char Get character function
 * @param send Send function
 * @param recv Receive function
 * @return 0 on success, -1 on error
 */
int xy_at_client_set_hal(xy_at_client_t *client,
                         int (*get_char)(char *ch, uint32_t timeout),
                         size_t (*send)(const char *data, size_t len),
                         size_t (*recv)(char *data, size_t len));

/* ==================== Response Management ==================== */

/**
 * @brief Create response structure
 * @param buf_size Response buffer size
 * @param line_num Expected line number (0 = wait for OK/ERROR)
 * @param timeout Timeout in milliseconds
 * @return Pointer to response or NULL on error
 */
xy_at_response_t *xy_at_create_resp(size_t buf_size, size_t line_num,
                                    uint32_t timeout);

/**
 * @brief Delete response structure
 * @param resp Pointer to response
 */
void xy_at_delete_resp(xy_at_response_t *resp);

/**
 * @brief Get response line by index
 * @param resp Pointer to response
 * @param line_num Line number (0-based)
 * @return Pointer to line or NULL
 */
const char *xy_at_resp_get_line(xy_at_response_t *resp, size_t line_num);

/**
 * @brief Get response line by prefix
 * @param resp Pointer to response
 * @param prefix Line prefix
 * @return Pointer to line or NULL
 */
const char *xy_at_resp_get_line_by_prefix(xy_at_response_t *resp,
                                          const char *prefix);

/**
 * @brief Parse response arguments
 * @param line Response line
 * @param format Format string (e.g., "%d,%s")
 * @param ... Variable arguments
 * @return Number of parsed arguments
 */
int xy_at_resp_parse_line_args(const char *line, const char *format, ...);

/**
 * @brief Parse response arguments by keyword
 * @param line Response line
 * @param keyword Keyword to search
 * @param format Format string
 * @param ... Variable arguments
 * @return Number of parsed arguments
 */
int xy_at_resp_parse_line_args_by_kw(const char *line, const char *keyword,
                                     const char *format, ...);

/* ==================== Command Execution ==================== */

/**
 * @brief Execute AT command
 * @param client Pointer to client
 * @param resp Pointer to response (can be NULL)
 * @param cmd_expr Command expression (printf-style)
 * @param ... Variable arguments
 * @return Response status
 */
xy_at_resp_status_t xy_at_exec_cmd(xy_at_client_t *client,
                                   xy_at_response_t *resp, const char *cmd_expr,
                                   ...);

/**
 * @brief Send AT command (formatted)
 * @param client Pointer to client
 * @param format Format string
 * @param ... Variable arguments
 * @return Number of bytes sent or -1 on error
 */
int xy_at_client_send(xy_at_client_t *client, const char *format, ...);

/**
 * @brief Wait for response with timeout
 * @param client Pointer to client
 * @param resp Pointer to response
 * @param timeout Timeout in milliseconds
 * @return Response status
 */
xy_at_resp_status_t xy_at_client_wait_resp(xy_at_client_t *client,
                                           xy_at_response_t *resp,
                                           uint32_t timeout);

/* ==================== URC Management ==================== */

/**
 * @brief Set URC table
 * @param client Pointer to client
 * @param urc_table Pointer to URC table
 * @param table_size Table size
 * @return 0 on success, -1 on error
 */
int xy_at_set_urc_table(xy_at_client_t *client, const xy_at_urc_t *urc_table,
                        size_t table_size);

/**
 * @brief Add single URC handler
 * @param client Pointer to client
 * @param prefix URC prefix
 * @param suffix URC suffix (can be NULL)
 * @param func Handler function
 * @return 0 on success, -1 on error
 */
int xy_at_add_urc_handler(xy_at_client_t *client, const char *prefix,
                          const char *suffix,
                          void (*func)(xy_at_client_t *client, const char *data,
                                       size_t size));

/* ==================== Data Mode ==================== */

/**
 * @brief Enter data mode (transparent transmission)
 * @param client Pointer to client
 * @return 0 on success, -1 on error
 */
int xy_at_client_enter_data_mode(xy_at_client_t *client);

/**
 * @brief Exit data mode
 * @param client Pointer to client
 * @return 0 on success, -1 on error
 */
int xy_at_client_exit_data_mode(xy_at_client_t *client);

/**
 * @brief Send data in data mode
 * @param client Pointer to client
 * @param data Data buffer
 * @param len Data length
 * @return Number of bytes sent or -1 on error
 */
int xy_at_client_send_data(xy_at_client_t *client, const uint8_t *data,
                           size_t len);

/**
 * @brief Receive data in data mode
 * @param client Pointer to client
 * @param data Data buffer
 * @param len Maximum length
 * @param timeout Timeout in milliseconds
 * @return Number of bytes received or -1 on error
 */
int xy_at_client_recv_data(xy_at_client_t *client, uint8_t *data, size_t len,
                           uint32_t timeout);

/* ==================== Utility Functions ==================== */

/**
 * @brief Get client statistics
 * @param client Pointer to client
 * @param tx_count Output: TX count
 * @param rx_count Output: RX count
 * @param error_count Output: Error count
 * @param timeout_count Output: Timeout count
 */
void xy_at_client_get_stats(xy_at_client_t *client, uint32_t *tx_count,
                            uint32_t *rx_count, uint32_t *error_count,
                            uint32_t *timeout_count);

/**
 * @brief Reset client statistics
 * @param client Pointer to client
 */
void xy_at_client_reset_stats(xy_at_client_t *client);

/**
 * @brief Wait for client to become idle
 * @param client Pointer to client
 * @param timeout Timeout in milliseconds
 * @return 0 if idle, -1 on timeout
 */
int xy_at_client_wait_idle(xy_at_client_t *client, uint32_t timeout);

/**
 * @brief Get client by name
 * @param name Client name
 * @return Pointer to client or NULL
 */
xy_at_client_t *xy_at_client_get_by_name(const char *name);

/**
 * @brief Get first available client
 * @return Pointer to client or NULL
 */
xy_at_client_t *xy_at_client_get_first(void);

#ifdef __cplusplus
}
#endif

#endif /* _XY_AT_CLIENT_H_ */
