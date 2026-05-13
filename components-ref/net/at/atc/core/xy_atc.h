#ifndef _XY_AT_H_
#define _XY_ATC_H_

#define ATC_PRINT_RAW_CMD
#define ATC_MAX_RESP_LEN        1024
#define ATC_MAX_CMD_LEN         1024
#define ATC_MAX_RETRY           5
#define ATC_MAX_DEFAULT_TIMEOUT 5000

#define ATC_MAX_CLIENT 1

#define ATC_END_SIGN

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

struct atc_client;

typedef void *atc_mutex_t;
typedef void *atc_semaphore_t;

enum atc_error { atc_error_ok = 0, atc_error = -1, atc_error_invalid_cmd };
typedef enum atc_error atc_error_t;

enum atc_status {
    atc_status_uninitialized = 0,
    atc_status_initialized,
    atc_status_cli,
    atc_status_idle,
    atc_status_busy
};
typedef enum atc_status atc_status_t;

enum {
    atc_resp_status_ok        = 0,
    atc_resp_status_error     = -1,
    atc_resp_status_timeout   = -2,
    atc_resp_status_buff_full = -3,
} atc_resp_status;
typedef enum atc_resp_status atc_resp_status_t;

struct atc_response {
    char *buf;       /** response buff */
    size_t buf_size; /** max buffer size */
    size_t buf_len;  /** current buffer len */
    // 0: wait for OK or ERROR response , >0  wait for target line
    uint8_t target_line; // !0 wait target line
    uint8_t line_counts;
    uint16_t resp_len;
    uint32_t timeout; /** the maximum reponse time. */
};

typedef struct atc_response *atc_response_t;

struct atc_urc {
    const char *cmd_prefix;
    const char *cmd_suffix;
    void (*func)(struct atc_client *client, const char *data, size_t size);
};
typedef struct atc_urc *atc_urc_t;

struct atc_urc_table {
    size_t urc_size;
    const struct atc_urc *urc;
};
typedef struct atc_urc_table atc_urc_table_t;

struct atc_client {

    atc_status_t status;
    char end_sign;

    int (*get_char)(char *ch, uint32_t timeout);
    size_t (*send)(const char *data, size_t len);

    // 用于 URC 后面的数据读取
    size_t (*recv)(char *data, size_t len);

    char *send_buf;
    size_t send_bufsz;
    /** 最后命令的长度 */
    size_t last_cmd_len;

    /**当前获取行buf*/
    char *recv_line_buf;
    /** 当前行数 */
    size_t recv_line_len;
    /** 当前行最大长度 */
    size_t recv_bufsz;
    int rx_notice;

    // send cmd lock 住， send cmd 释放
    uint32_t mutex_lock;

    // send cmd 时清除，parse 到加操作
    uint32_t sem_notice;
    atc_resp_status_t resp_status;
    // 当前的 resp
    atc_response_t resp;

    struct atc_urc_table *urc_table;
    size_t urc_table_size;
};
typedef struct atc_client *atc_client_t;

/**
= 0 返回 OK，小于其他，需要几个 bit 去表示 client 有些地方并未初始化
*/
int atc_register(atc_client_t client);
int atc_init(atc_client_t client);
int atc_add_urc_hlr(atc_client_t client, struct atc_urc_table *urc_table,
                    size_t size);

atc_client_t atc_get_client(uint8_t client_index);

/* AT client send commands to AT server and waiter response */
int atc_cmd(atc_client_t client, atc_response_t resp, const char *cmd_expr,
            ...);
int atc_cmd_format(atc_client_t client, atc_response_t resp, const char *format,
                   const char *cmd_expr, ...);

int atc_wait_connect(atc_client_t client, uint32_t timeout);
int atc_send(atc_client_t client, const char *buff, uint16_t size);
int atc_recv(atc_client_t client, char *buff, uint16_t size, uint32_t timeout);

/* ATC response object create and delete */
atc_response_t atc_create_resp(uint16_t buf_size, uint16_t line_num,
                               uint32_t timeout);
void atc_delete_resp(atc_response_t resp);
atc_response_t atc_resp_set_info(atc_response_t resp, uint16_t buf_size,
                                 uint16_t line_num, uint32_t timeout);

/* AT response line buffer get and parse response buffer arguments */
const char *atc_resp_get_line(atc_response_t resp, size_t resp_line);
const char *atc_resp_get_line_by_kw(atc_response_t resp, const char *keyword);
int atc_resp_parse_line_args(atc_response_t resp, size_t resp_line,
                             const char *resp_expr, ...);
int atc_resp_parse_line_args_by_kw(atc_response_t resp, const char *keyword,
                                   const char *resp_expr, ...);

#endif