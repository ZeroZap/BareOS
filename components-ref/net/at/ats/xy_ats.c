/**
 * @file xy_ats.c
 * @brief AT Command Server Implementation
 * @version 1.0.0
 * @date 2026-03-26
 */

#include "xy_ats.h"
#include <stdint.h>
#include <string.h>

/**
 * @brief Create AT server instance
 * @param name Server name
 * @return Pointer to server instance or NULL on failure
 */
at_server_t *at_server_create(const char *name)
{
    static at_server_t server_instance;
    at_server_t *server = &server_instance;
    
    if (!server) {
        return NULL;
    }
    
    memset(server, 0, sizeof(at_server_t));
    server->name = name;
    server->status = ATS_SERVER_STATUS_INITIALIZED;
    server->echo_mode = ATS_SERVER_ECHO_MODE;
    server->recv_len = 0;
    server->parser_running = false;
    server->cmd_processed = 0;
    server->cmd_ok = 0;
    server->cmd_error = 0;
    
    // Initialize hash table
    ats_hash_init(&server->cmd_table);
    
    return server;
}

/**
 * @brief Delete AT server instance
 * @param server Pointer to server
 */
void at_server_delete(at_server_t *server)
{
    if (server) {
        at_server_stop(server);
    }
}

/**
 * @brief Set HAL interface for server
 * @param server Pointer to server
 * @param get_char Get character function
 * @param send Send data function
 * @return 0 on success, -1 on failure
 */
int at_server_set_hal(at_server_t *server, 
                      int (*get_char)(char *ch, uint32_t timeout),
                      size_t (*send)(const char *data, size_t len))
{
    if (!server) {
        return -1;
    }
    
    server->get_char = get_char;
    server->send = send;
    
    return 0;
}

/**
 * @brief Register AT command
 * @param server Pointer to server
 * @param cmd Command definition
 * @return 0 on success, -1 on failure
 */
int at_server_register_cmd(at_server_t *server, const at_cmd_t *cmd)
{
    if (!server || !cmd) {
        return -1;
    }
    
    // Add command to hash table
    if (!ats_hash_insert(&server->cmd_table, cmd->name, (struct at_cmd *)cmd)) {
        return -1;
    }
    
    ATS_DBG("Registered command: %s\n", cmd->name);
    return 0;
}

/**
 * @brief Find command by name
 * @param server Pointer to server
 * @param name Command name
 * @return Pointer to command or NULL if not found
 */
at_cmd_t *at_server_find_cmd(at_server_t *server, const char *name)
{
    if (!server || !name) {
        return NULL;
    }
    
    struct at_cmd *cmd = ats_hash_find(&server->cmd_table, name);
    return (at_cmd_t *)cmd;
}

/**
 * @brief Start AT server
 * @param server Pointer to server
 * @return 0 on success, -1 on failure
 */
int at_server_start(at_server_t *server)
{
    if (!server) {
        return -1;
    }
    
    if (server->status == ATS_SERVER_STATUS_RUNNING) {
        return 0; // Already running
    }
    
    server->status = ATS_SERVER_STATUS_RUNNING;
    server->parser_running = true;
    
    ATS_DBG("AT Server started: %s\n", server->name);
    return 0;
}

/**
 * @brief Stop AT server
 * @param server Pointer to server
 * @return 0 on success, -1 on failure
 */
int at_server_stop(at_server_t *server)
{
    if (!server) {
        return -1;
    }
    
    server->parser_running = false;
    server->status = ATS_SERVER_STATUS_INITIALIZED;
    
    ATS_DBG("AT Server stopped: %s\n", server->name);
    return 0;
}

/**
 * @brief Process AT command
 * @param server Pointer to server
 * @param cmd_line Command line to process
 * @return 0 on success, -1 on failure
 */
int at_server_process_command(at_server_t *server, const char *cmd_line)
{
    if (!server || !cmd_line) {
        return -1;
    }
    
    // Skip "AT" prefix
    const char *cmd_name = cmd_line;
    if (strncmp(cmd_line, "AT", 2) == 0) {
        cmd_name = cmd_line + 2;
    }
    
    // Skip "+" if present
    if (*cmd_name == '+') {
        cmd_name++;
    }
    
    // Create a copy for modification
    char lookup_name[ATS_CMD_NAME_MAX_LEN];
    strncpy(lookup_name, cmd_name, sizeof(lookup_name) - 1);
    lookup_name[sizeof(lookup_name) - 1] = '\0';
    
    // Remove query (?) suffix
    size_t len = strlen(lookup_name);
    if (len > 0 && lookup_name[len - 1] == '?') {
        lookup_name[len - 1] = '\0';
    }
    
    // Remove setup (=) suffix and everything after it
    char *equals_pos = strchr(lookup_name, '=');
    if (equals_pos) {
        *equals_pos = '\0';
    }
    
    // Find command in hash table
    at_cmd_t *cmd = at_server_find_cmd(server, lookup_name);
    if (!cmd) {
        if (server->send) {
            server->send("ERROR\r\n", 7);
        }
        server->cmd_error++;
        return -1;
    }
    
    server->cmd_processed++;
    
    // Parse command mode and execute
    const char *args = NULL;
    at_cmd_mode_t mode = ATS_CMD_MODE_EXEC;
    
    // Check for test mode (AT+CMD=?)
    if (strstr(cmd_name, "=?") != NULL) {
        mode = ATS_CMD_MODE_TEST;
    }
    // Check for query mode (AT+CMD?)
    else if (cmd_name[strlen(cmd_name)-1] == '?') {
        mode = ATS_CMD_MODE_QUERY;
    }
    // Look for = sign for setup mode
    else {
        char *equals = strchr((char*)cmd_name, '=');
        if (equals) {
            mode = ATS_CMD_MODE_SETUP;
            args = equals + 1;
        }
    }
    
    at_result_t result = ATS_RESULT_FAIL;
    
    switch (mode) {
    case ATS_CMD_MODE_TEST:
        if (cmd->test) {
            result = cmd->test();
        }
        break;
        
    case ATS_CMD_MODE_QUERY:
        if (cmd->query) {
            result = cmd->query();
        }
        break;
        
    case ATS_CMD_MODE_SETUP:
        if (cmd->setup) {
            result = cmd->setup(args);
        }
        break;
        
    case ATS_CMD_MODE_EXEC:
        if (cmd->exec) {
            result = cmd->exec();
        }
        break;
    }
    
    // Send result
    at_server_print_result(server, result);
    
    return 0;
}

/**
 * @brief Get server statistics
 * @param server Pointer to server
 * @param cmd_processed Pointer to store processed command count
 * @param cmd_ok Pointer to store OK command count  
 * @param cmd_error Pointer to store error command count
 */
void at_server_get_stats(at_server_t *server, uint32_t *cmd_processed,
                         uint32_t *cmd_ok, uint32_t *cmd_error)
{
    if (server) {
        if (cmd_processed) {
            *cmd_processed = server->cmd_processed;
        }
        if (cmd_ok) {
            *cmd_ok = server->cmd_ok;
        }
        if (cmd_error) {
            *cmd_error = server->cmd_error;
        }
    }
}

/**
 * @brief Reset server statistics
 * @param server Pointer to server
 */
void at_server_reset_stats(at_server_t *server)
{
    if (server) {
        server->cmd_processed = 0;
        server->cmd_ok = 0;
        server->cmd_error = 0;
    }
}

/**
 * @brief Get server by name
 * @param name Server name
 * @return Pointer to server or NULL
 */
at_server_t *at_server_get_by_name(const char *name)
{
    // Simple implementation - returns static instance if name matches
    static at_server_t server_instance;
    
    if (name && server_instance.name && 
        strcmp(name, server_instance.name) == 0) {
        return &server_instance;
    }
    
    return NULL;
}

/**
 * @brief Print formatted data to AT device
 * @param server Pointer to server
 * @param fmt Format string
 * @param ... Variable arguments
 * @return Number of bytes sent
 */
int at_server_printf(at_server_t *server, const char *fmt, ...)
{
    if (!server || !fmt || !server->send) {
        return -1;
    }
    
    // Use send buffer
    char buf[ATS_SERVER_SEND_BUF_SIZE];
    va_list args;
    va_start(args, fmt);
    
    // Simple formatting - just copy for now
    // In production, use a proper vsnprintf implementation
    int len = 0;
    const char *p = fmt;
    char *out = buf;
    
    while (*p && len < (int)(sizeof(buf) - 1)) {
        if (*p == '%') {
            p++;
            if (*p == 'd' || *p == 'i') {
                int val = va_arg(args, int);
                // Simple int to string conversion
                char tmp[16];
                int i = 0;
                int n = val < 0 ? -val : val;
                do {
                    tmp[i++] = '0' + (n % 10);
                    n /= 10;
                } while (n > 0);
                if (val < 0) {
                    *out++ = '-';
                    len++;
                }
                while (i > 0) {
                    *out++ = tmp[--i];
                    len++;
                }
            } else if (*p == 's') {
                const char *s = va_arg(args, const char *);
                if (s) {
                    while (*s && len < (int)(sizeof(buf) - 1)) {
                        *out++ = *s++;
                        len++;
                    }
                }
            } else if (*p == 'x' || *p == 'X') {
                unsigned int val = va_arg(args, unsigned int);
                char tmp[16];
                int i = 0;
                do {
                    int d = val & 0xF;
                    tmp[i++] = d < 10 ? '0' + d : 'A' + (d - 10);
                    val >>= 4;
                } while (val > 0);
                while (i > 0) {
                    *out++ = tmp[--i];
                    len++;
                }
            }
            p++;
        } else {
            *out++ = *p++;
            len++;
        }
    }
    
    va_end(args);
    *out = '\0';
    
    return (int)server->send(buf, len);
}

/**
 * @brief Print formatted data to AT device with newline
 * @param server Pointer to server
 * @param fmt Format string
 * @param ... Variable arguments
 * @return Number of bytes sent
 */
int at_server_printfln(at_server_t *server, const char *fmt, ...)
{
    if (!server || !fmt || !server->send) {
        return -1;
    }
    
    // Use send buffer
    char buf[ATS_SERVER_SEND_BUF_SIZE];
    va_list args;
    va_start(args, fmt);
    
    // Simple formatting
    int len = 0;
    const char *p = fmt;
    char *out = buf;
    
    while (*p && len < (int)(sizeof(buf) - 3)) {  // -3 for \r\n\0
        if (*p == '%') {
            p++;
            if (*p == 'd' || *p == 'i') {
                int val = va_arg(args, int);
                char tmp[16];
                int i = 0;
                int n = val < 0 ? -val : val;
                do {
                    tmp[i++] = '0' + (n % 10);
                    n /= 10;
                } while (n > 0);
                if (val < 0) {
                    *out++ = '-';
                    len++;
                }
                while (i > 0) {
                    *out++ = tmp[--i];
                    len++;
                }
            } else if (*p == 's') {
                const char *s = va_arg(args, const char *);
                if (s) {
                    while (*s && len < (int)(sizeof(buf) - 3)) {
                        *out++ = *s++;
                        len++;
                    }
                }
            } else if (*p == 'x' || *p == 'X') {
                unsigned int val = va_arg(args, unsigned int);
                char tmp[16];
                int i = 0;
                do {
                    int d = val & 0xF;
                    tmp[i++] = d < 10 ? '0' + d : 'A' + (d - 10);
                    val >>= 4;
                } while (val > 0);
                while (i > 0) {
                    *out++ = tmp[--i];
                    len++;
                }
            }
            p++;
        } else {
            *out++ = *p++;
            len++;
        }
    }
    
    va_end(args);
    
    // Add line ending
    *out++ = '\r';
    *out++ = '\n';
    *out = '\0';
    len += 2;
    
    return (int)server->send(buf, len);
}

/**
 * @brief Send result response
 * @param server Pointer to server
 * @param result Result code
 * @return 0 on success, -1 on failure
 */
int at_server_print_result(at_server_t *server, at_result_t result)
{
    if (!server || !server->send) {
        return -1;
    }
    
    if (result == ATS_RESULT_OK) {
        server->send("OK\r\n", 4);
        server->cmd_ok++;
    } else if (result != ATS_RESULT_NULL) {
        server->send("ERROR\r\n", 7);
        server->cmd_error++;
    }
    
    return 0;
}
