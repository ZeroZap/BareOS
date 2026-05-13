#ifndef _AT_MAIN_H_
#define _AT_MAIN_H_
#include "at.h"
#include "uvr.h"
/****************** AT Setting *******************/

// Queue Define
#define AT_QUEUE_ITEM_SIZE  (1)
#define AT_LOCAL_QUEUE_LENGTH (5)

// General Table
#define AT_MAX_GNENERAL_TABLE_NUM    (20)
#define AT_HASH_TABLE_ROW            (37)
#define AT_HASH_TABLE_SPAN           (5)
#define AT_MAX_CMD_NAME_LEN          (2*AT_HASH_TABLE_SPAN)
#define AT_MAX_CMD_HEAD_LEN          (AT_MAX_CMD_NAME_LEN+3)

/**UART Related*/
#define AT_UART_RX_FIFO_THRESHOLD_SIZE    (32)
#define AT_UART_TX_FIFO_THRESHOLD_SIZE    (1024)


typedef enum {
    AT_MSG_ID_READ_CMD = 2000,
    AT_MSG_ID_RESPOSE_CMD,
    AT_MSG_ID_SWITCH_TO_NORMAL,
    AT_MSG_ID_SWITCH_TO_BYPASS,
    AT_MSG_ID_MAX
}at_msg_id_t;


typedef struct {
    uint32_t id;
    int32_t port;
    int32_t data_len;
    void *data;
}at_msg_t;


typedef struct{
    char input_buf[AT_RX_BUFFER_SIZE];
    uint16_t input_len;
    uint32_t flag;
}at_input_cmd_msg_t;


typedef struct {
    /* the beginning structure need to be the same with at_parse_cmd_param_t (at.h)*/
    uint8_t               *string_ptr;
    uint32_t              string_len;
    uint32_t              name_len;      /* AT command name length. ex. In "AT+EXAMPLE=1,2,3", name_len = 10 (not include = symbol) */
    uint32_t              parse_pos;     /* parse_pos means the length after detecting AT command mode */
    at_cmd_mode_t mode;

    uint32_t              hash_value1;
    uint32_t              hash_value2;

} at_parse_cmd_param_ex_t;

enum AT_DATA_MODE {
    AT_DATA_CMD = 0,
    AT_DATA_BYPASS
};

typedef enum  {
    AT_MSG_TYPE_CMD,
    AT_MSG_TYPE_BYPASS_DATA,
}at_msg_type_t;

enum AT_CMD_PROCESSING {
    AT_CMD_PROCESSING_RECEIVE = 0,
    AT_CMD_PROCESSING_VALID = 1,
    AT_CMD_PROCESSING_PARSING = 2,
    AT_CMD_PROCESSING_RESPONSE = 3,
    AT_CMD_PROCESSING_BYPASS = 4
};

#define AT_MAX_GENERAL_TABLE_NUM 10
#define AT_MAX_INPUT_MSGQ_NUM 12
#define AT_MAX_RESPONSE_MSGQ_NUM 12

extern at_cmd_hdlr_table_t g_at_cmd_hdlr_tables[];
extern uint32_t g_at_registered_table_number;



uint32_t at_uart_send(uint32_t port, uint8_t *data, uint32_t length);
void at_bypass_timer_reload(uint32_t timer, uint32_t flag);
uint32_t at_bypass_timer_status(void);

void uvr_at_task_create(unsigned char priority);

#endif