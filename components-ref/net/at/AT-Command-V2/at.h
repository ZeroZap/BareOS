#ifndef _SF_AT_H_
#define _SF_AT_H_
#include "sf.h"

#define AT_UART_TX_FIFO_BUFFER_SIZE 128
typedef enum {
    AT_STA_OK = 0,      /**Failed to register the AT command handler table*/
    AT_STA_REG_FAILURE, /**< An error occurred during the function call. */
    AT_STA_ERROR        /**< No error occurred during the function call. */
}AT_STA;

typedef enum {
    AT_CMD_MODE_READ,
    AT_CMD_MODE_ACTIVE,
    AT_CMD_MODE_EXE,
    AT_CMD_MODE_TESTING,
    AT_CMD_MODE_INVALID
}AT_CMD_MODE;

typedef enum {
    AT_RESPONSE_FLAG_AUTO_APPEND_LF_CR = 0X02,
    AT_RESPONSE_FLAG_URC_FORMAT = 0X10,
    AT_RESPONSE_FLAG_QUOTED_WITH_LF_CR = 0x20,
    AT_RESPONSE_FLAG_AUTO_APPEND_OK = 0x40,
    AT_RESPONSE_FLAG_APPEND_ERROR = 0x80
}AT_RESPONSE_FLAG;

struct at_response{
    sf_uint8_t buf[AT_UART_TX_FIFO_BUFFER_SIZE];
    sf_uint16_t len;
    sf_uint8_t flag;  /* For moe information, please refer to #AT_RESPONSE_FLAG*/
};
typedef struct at_response at_response_t;


struct at_response_heavy{
    sf_uint8_t *buf;
    sf_uint16_t len;
    sf_uint8_t flag;  /* For moe information, please refer to #AT_RESPONSE_FLAG*/
};
typedef struct at_response_heavy at_response_heavy_t;

struct at_parse_cmd_param{
    char *str_ptr;    /**< The input data buffer*/
    sf_uint32_t str_len; /**< The response data len*/
    sf_uint32_t name_len; /** */
};

struct at_cmd_list{
    sf_uint16_t interval;
    sf_int8_t   *cmd;
    sf_int8_t   *result;
    void        (*tx_func)(void *);
    void        (*rx_func)(void *);
    sf_uint8_t   flag;
};

sf_uint8_t at_ok(void *);


#endif