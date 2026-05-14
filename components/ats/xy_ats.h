/**
 * @file xy_ats.h
 * @brief AT Command Server — bare-metal, main-loop driven.
 *
 * Usage:
 *   at_server_t srv;
 *   at_server_init(&srv, "debug");
 *   at_server_set_hal(&srv, NULL, uart_write);   // get_char unused; feed via feed_byte
 *   at_server_register_cmd(&srv, &cmd_version);
 *   at_server_start(&srv);
 *
 *   // In UART RX ISR (or DMA complete callback):
 *   at_server_feed_byte(&srv, received_char);
 */

#ifndef _ATS_H_
#define _ATS_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>

// Hash table for command mapping
#include "xy_ats_hash.h"

// Logging
#include "xy_log.h"

// Debug macro
#ifndef ATS_DBG
#define ATS_DBG(fmt, ...) XY_LOG_D("AT Server: " fmt, ##__VA_ARGS__)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Configuration ==================== */

#ifndef ATS_SERVER_RECV_BUF_SIZE
#define ATS_SERVER_RECV_BUF_SIZE 256 ///< Receive buffer size
#endif

#ifndef ATS_SERVER_SEND_BUF_SIZE
#define ATS_SERVER_SEND_BUF_SIZE 512 ///< Send buffer size
#endif

#ifndef ATS_CMD_NAME_MAX_LEN
#define ATS_CMD_NAME_MAX_LEN 16 ///< Maximum command name length
#endif

#ifndef ATS_CMD_TABLE_MAX
#define ATS_CMD_TABLE_MAX 32 ///< Maximum command table size
#endif

// Server configuration
#ifndef ATS_SERVER_ECHO_MODE
#define ATS_SERVER_ECHO_MODE true ///< Default echo mode
#endif

/* ==================== Result Codes ==================== */

/* Version information */
#define AT_VERSION       "1.0.0"
#define AT_VERSION_MAJOR 1
#define AT_VERSION_MINOR 0
#define AT_VERSION_PATCH 0

/* ==================== Response End Markers ==================== */

/**
 * @brief OK response string
 */
#ifndef AT_RESP_OK_STR
#define AT_RESP_OK_STR "OK"
#endif

/**
 * @brief ERROR response string
 */
#ifndef AT_RESP_ERROR_STR
#define AT_RESP_ERROR_STR "ERROR"
#endif

/**
 * @brief Line ending for commands
 */
#ifndef AT_CMD_LINE_END
#define AT_CMD_LINE_END "\r\n"
#endif

/**
 * @brief Command prefix
 */
#ifndef AT_CMD_PREFIX
#define AT_CMD_PREFIX "AT"
#endif


typedef enum {
    at_state_init,
    at_state_idle,
    at_state_sending,
    at_state_receiving,
    at_state_error,
    at_state_data_mode,
} at_state_t;

struct at_dev_attr {
    uint16_t recv_size;
    uint16_t send_size;
    void *user_data;
};
typedef struct at_dev_attr *at_dev_attr_t;

struct at_dev_ops {
    /** read from device ring buffer. */
    int (*read)(void *buffer, int len);
    /** write to device */
    int (*write)(const void *buffer, int len);
    /** cmd: power on, power off, enter sleep. */
    int (*control)(int cmd, void *args);
    /** dev ops attr */
    at_dev_attr_t attr;
};
typedef struct at_dev_ops *at_dev_ops_t;

/**
 * @brief AT command result
 */
typedef enum {
    ATS_RESULT_OK        = 0,  ///< Command executed successfully
    ATS_RESULT_FAIL      = -1, ///< Command execution failed
    ATS_RESULT_NULL      = -2, ///< No result to return
    ATS_RESULT_CMD_ERR   = -3, ///< Command format error
    ATS_RESULT_PARSE_ERR = -4, ///< Parameter parse error
} at_result_t;

/**
 * @brief AT command mode
 */
typedef enum {
    ATS_CMD_MODE_TEST,  ///< Test mode (AT+CMD=?)
    ATS_CMD_MODE_QUERY, ///< Query mode (AT+CMD?)
    ATS_CMD_MODE_SETUP, ///< Setup mode (AT+CMD=<params>)
    ATS_CMD_MODE_EXEC,  ///< Execute mode (AT+CMD)
} at_cmd_mode_t;

/* ==================== AT Command Structure ==================== */

/**
 * @brief AT command structure
 */
typedef struct at_cmd {
    char name[ATS_CMD_NAME_MAX_LEN]; ///< Command name (e.g., "AT+CMD")
    const char *args_expr;           ///< Argument expression (optional)

    // Command handlers
    at_result_t (*test)(void);  ///< Test mode handler (AT+CMD=?)
    at_result_t (*query)(void); ///< Query mode handler (AT+CMD?)
    at_result_t (*setup)(
        const char *args);     ///< Setup mode handler (AT+CMD=<args>)
    at_result_t (*exec)(void); ///< Execute mode handler (AT+CMD)
} at_cmd_t;

/**
 * @brief AT server status
 */
typedef enum {
    ATS_SERVER_STATUS_UNINITIALIZED = 0,
    ATS_SERVER_STATUS_INITIALIZED,
    ATS_SERVER_STATUS_RUNNING,
} at_server_status_t;

struct ats_cmd {
    char name[ATS_CMD_NAME_MAX_LEN];
    char *args_expr;
    int (*test)(void);
    int (*query)(void);
    int (*setup)(const char *args);
    int (*exec)(void);
};

struct ats {
    at_dev_ops_t device;
    at_state_t status;
    uint8_t echo_en;
    uint16_t cur_recv_len;
};

/**
 * @brief AT server structure
 */
typedef struct at_server {
    const char *name;          ///< Server name
    at_server_status_t status; ///< Server status
    bool echo_mode;            ///< Echo mode enabled

    // Command mapping (hash table instead of linear array)
    ats_hash_table_t cmd_table; ///< Hash table for command lookup

    // HAL interface
    int (*get_char)(char *ch, uint32_t timeout);  ///< Get character from device
    size_t (*send)(const char *data, size_t len); ///< Send data to device

    // Buffers
    char send_buf[ATS_SERVER_SEND_BUF_SIZE];
    char recv_buf[ATS_SERVER_RECV_BUF_SIZE];
    size_t recv_len;

    bool parser_running;

    // Statistics
    uint32_t cmd_processed;
    uint32_t cmd_ok;
    uint32_t cmd_error;
} at_server_t;

// Compatibility typedef
typedef at_server_t ats_t;

/* ==================== Server Management ==================== */

/**
 * @brief Create AT server
 * @param name Server name
 * @return Pointer to server or NULL on error
 */
at_server_t *at_server_create(const char *name);

/**
 * @brief Initialize AT server
 * @param server Pointer to server structure
 * @param name Server name
 * @return 0 on success, -1 on error
 */
int at_server_init(at_server_t *server, const char *name);

/**
 * @brief Delete AT server
 * @param server Pointer to server
 */
void at_server_delete(at_server_t *server);

/**
 * @brief Set AT server HAL interface
 * @param server Pointer to server
 * @param get_char Get character function
 * @param send Send function
 * @return 0 on success, -1 on error
 */
int at_server_set_hal(at_server_t *server,
                      int (*get_char)(char *ch, uint32_t timeout),
                      size_t (*send)(const char *data, size_t len));

/**
 * @brief Start AT server
 * @param server Pointer to server
 * @return 0 on success, -1 on error
 */
int at_server_start(at_server_t *server);

/**
 * @brief Stop AT server
 * @param server Pointer to server
 * @return 0 on success, -1 on error
 */
int at_server_stop(at_server_t *server);

/* ==================== Command Registration ==================== */

/**
 * @brief Register AT command
 * @param server Pointer to server
 * @param cmd Pointer to command structure
 * @return 0 on success, -1 on error
 */
int at_server_register_cmd(at_server_t *server, const at_cmd_t *cmd);

/**
 * @brief Unregister AT command
 * @param server Pointer to server
 * @param name Command name
 * @return 0 on success, -1 on error
 */
int at_server_unregister_cmd(at_server_t *server, const char *name);

/**
 * @brief Macro to define and register AT command
 * @param _name Command name
 * @param _args Argument expression
 * @param _test Test mode handler
 * @param _query Query mode handler
 * @param _setup Setup mode handler
 * @param _exec Execute mode handler
 */
#define ATS_CMD_EXPORT(_name, _args, _test, _query, _setup, _exec) \
    static const at_cmd_t __at_cmd_##_name = {                     \
        .name      = #_name,                                       \
        .args_expr = _args,                                        \
        .test      = _test,                                        \
        .query     = _query,                                       \
        .setup     = _setup,                                       \
        .exec      = _exec,                                        \
    }

/**
 * @brief Feed one received byte into the server (call from UART RX ISR or main loop).
 * Accumulates bytes until CR/LF, then dispatches the complete command line.
 */
void at_server_feed_byte(at_server_t *server, char ch);

/**
 * @brief Process AT command
 * @param server Pointer to server
 * @param cmd_line Command line to process
 * @return 0 on success, -1 on failure
 */
int at_server_process_command(at_server_t *server, const char *cmd_line);

/**
 * @brief Find command by name
 * @param server Pointer to server
 * @param name Command name
 * @return Pointer to command or NULL if not found
 */
at_cmd_t *at_server_find_cmd(at_server_t *server, const char *name);

/* ==================== Response Functions ==================== */

/**
 * @brief Send formatted response
 * @param server Pointer to server
 * @param format Format string
 * @param ... Variable arguments
 * @return Number of bytes sent
 */
int at_server_printf(at_server_t *server, const char *format, ...);

/**
 * @brief Send formatted response with newline
 * @param server Pointer to server
 * @param format Format string
 * @param ... Variable arguments
 * @return Number of bytes sent
 */
int at_server_printfln(at_server_t *server, const char *format, ...);

/**
 * @brief Send result code
 * @param server Pointer to server
 * @param result Result code
 * @return Number of bytes sent
 */
int at_server_print_result(at_server_t *server, at_result_t result);

/**
 * @brief Send raw data
 * @param server Pointer to server
 * @param data Data buffer
 * @param len Data length
 * @return Number of bytes sent
 */
size_t at_server_send(at_server_t *server, const char *data, size_t len);

/**
 * @brief Receive data with timeout
 * @param server Pointer to server
 * @param data Data buffer
 * @param len Maximum length
 * @param timeout Timeout in milliseconds
 * @return Number of bytes received
 */
size_t at_server_recv(at_server_t *server, char *data, size_t len,
                      uint32_t timeout);

/* ==================== Parameter Parsing ==================== */

/**
 * @brief Parse command arguments
 * @param args Argument string
 * @param format Format string (e.g., "%d,%s")
 * @param ... Variable arguments
 * @return Number of parsed arguments
 */
int at_parse_args(const char *args, const char *format, ...);

/**
 * @brief Parse integer parameter
 * @param args Argument string
 * @param value Output: parsed value
 * @return 0 on success, -1 on error
 */
int at_parse_int(const char *args, int *value);

/**
 * @brief Parse string parameter
 * @param args Argument string
 * @param value Output: string buffer
 * @param max_len Maximum buffer length
 * @return 0 on success, -1 on error
 */
int at_parse_string(const char *args, char *value, size_t max_len);

/**
 * @brief Parse hexadecimal parameter
 * @param args Argument string
 * @param value Output: parsed value
 * @return 0 on success, -1 on error
 */
int at_parse_hex(const char *args, uint32_t *value);

/* ==================== Echo Mode ==================== */

/**
 * @brief Set echo mode
 * @param server Pointer to server
 * @param enable true to enable, false to disable
 */
void at_server_set_echo(at_server_t *server, bool enable);

/**
 * @brief Get echo mode
 * @param server Pointer to server
 * @return true if enabled, false otherwise
 */
bool at_server_get_echo(at_server_t *server);

/* ==================== Utility Functions ==================== */

/**
 * @brief Get server statistics
 * @param server Pointer to server
 * @param cmd_processed Output: processed count
 * @param cmd_ok Output: OK count
 * @param cmd_error Output: error count
 */
void at_server_get_stats(at_server_t *server, uint32_t *cmd_processed,
                         uint32_t *cmd_ok, uint32_t *cmd_error);

/**
 * @brief Reset server statistics
 * @param server Pointer to server
 */
void at_server_reset_stats(at_server_t *server);

/**
 * @brief Get server by name
 * @param name Server name
 * @return Pointer to server or NULL
 */
at_server_t *at_server_get_by_name(const char *name);

#ifdef __cplusplus
}
#endif

#endif /* _ATSS_H_ */
