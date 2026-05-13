#include "at.h"
#include "at_main.h"
#include "at_adapter.h"
#include "uvr.h"
#define AT_HANDLER "at_handler"
#define AT_SEND_RESPONSE "at_send_response"
#define AT_INPUT_COMMAND_HANDLER "at_input_command_handler"

static at_status_t at_init_hdlr_tbl_hash_value(at_cmd_hdlr_item_t *hdlr_table, uint16_t hdlr_number);
static uint16_t at_local_calculate_hash_value(uint8_t *at_name, uint32_t *hash_value1, uint32_t *hash_value2);

extern const at_cmd_hdlr_item_t g_at_cmd_items[];
extern uint32_t g_at_input_cmd_in_processing;
extern uint32_t g_at_local_startup_flag;
extern at_msg_type_t g_at_msg_type;

extern at_msgq_t g_at_cmd_response_msgq;

extern int at_tcp_send(char *data, int len);
/**
 * 清楚指令中多余的空格
 */
static at_status_t at_format_cmd(char *cmd)
{
    ;
}

static at_status_t at_check_end_valid_char(char c)
{
    if ((c == AT_CHAR_CR) || (c == AT_CHAR_LF) || (c == '\0'))
    {
        return AT_STATUS_OK;
    }
    return AT_STATUS_ERROR;
}

static at_status_t at_add_to_cmd_hdlr_tbls(at_cmd_hdlr_item_t *hdlr_table, uint16_t hdlr_number)
{

    if (g_at_registered_table_number < AT_MAX_GENERAL_TABLE_NUM)
    {
        g_at_cmd_hdlr_tables[g_at_registered_table_number].item_table = hdlr_table;
        g_at_cmd_hdlr_tables[g_at_registered_table_number].item_table_size = hdlr_number;
        g_at_registered_table_number++;
        return AT_STATUS_OK;
    }
    else
    {
        return AT_STATUS_ERROR;
    }
}

static at_status_t at_init_hdlr_tbl_hash_value(at_cmd_hdlr_item_t *hdlr_table, uint16_t hdlr_number)
{
    uint16_t i;
    uint8_t *cmd_ptr;
    for (i = 0; i < hdlr_number; i++)
    {
        cmd_ptr = (uint8_t *)hdlr_table[i].cmd_head;
        /* caculate hash value of AT command name */
        at_local_calculate_hash_value(cmd_ptr, &(hdlr_table[i].hash_value1), &(hdlr_table[i].hash_value2));
    }

    return AT_STATUS_OK;
}

static uint16_t at_local_calculate_hash_value(uint8_t *at_name, uint32_t *hash_value1, uint32_t *hash_value2)
{
    uint16_t i = 0, counter = 0;
    char ascii_char = 0;
    uint32_t value1 = 0;
    uint32_t value2 = 0;

    (*hash_value1) = AT_CHAR_INVALID_COMMAND_HASH_VALUE;
    (*hash_value2) = AT_CHAR_INVALID_COMMAND_HASH_VALUE;
    // if ((AT_CHAR_A == at_name[0] && AT_CHAR_T == at_name[1]) ||
    // (AT_CHAR_a == at_name[0] && AT_CHAR_t == at_name[1])) {
    if ((AT_CHAR_A == at_name[0] || AT_CHAR_a == at_name[0]) &&
        (AT_CHAR_T == at_name[1] || AT_CHAR_t == at_name[1]))
    {

        /* Only support 'AT+XXX' 'AT#XXX' 'AT%XXX' 'AT*XXX'*/
        if (AT_CHAR_PLUS == at_name[2] || AT_CHAR_POUND == at_name[2])
        {

            /* caculate hash value after ("AT+" or "AT#") until entering ('=' or '?' or CR/LF/NULL) */
            i = 3;
            counter = 3;
            while ((at_name[i] != AT_CHAR_EQUAL) && (at_name[i] != AT_CHAR_QUESTION_MARK) &&
                   (at_name[i] != AT_CHAR_CR) && (at_name[i] != AT_CHAR_LF) && (at_name[i] != '\0'))
            {

                if (AT_CHAR_IS_UPPER(at_name[i]))
                {
                    ascii_char = at_name[i] - AT_CHAR_A;
                }
                else if (AT_CHAR_IS_LOWER(at_name[i]))
                {
                    ascii_char = at_name[i] - AT_CHAR_a;
                }
                else if (AT_CHAR_IS_NUMBER(at_name[i]))
                {
                    ascii_char = at_name[i] - AT_CHAR_0;
                }
                if (counter < (AT_HASH_TABLE_SPAN + 3))
                {
                    value1 = value1 * (AT_HASH_TABLE_ROW + 1) + (ascii_char + 1); /* 0 ~ 4*/
                }
                else if (counter < AT_MAX_CMD_NAME_LEN + 3)
                {
                    value2 = value2 * (AT_HASH_TABLE_ROW + 1) + (ascii_char + 1); /* 5 ~ 9*/
                }

                counter++;
                i++;
            }
        }
    }

    (*hash_value1) = value1;
    (*hash_value2) = value2;
    return counter;
}

static at_status_t at_local_parse_cmd_name(at_parse_cmd_param_ex_t *parse_cmd)
{
    at_status_t ret = AT_STATUS_ERROR;
    uint32_t name_len = 0;
    uint8_t *at_name = parse_cmd->string_ptr;

    switch (at_name[2])
    {
    case AT_CHAR_PLUS:
        at_log_i("ext cmd, cal hash value");
        name_len = at_local_calculate_hash_value(at_name, &(parse_cmd->hash_value1), &(parse_cmd->hash_value2));
        if (name_len == 0)
            ret = AT_STATUS_INVALID_CMD;
        break;

    default: // basic cmd like: ATE, ATI...
        name_len = 2;
        at_log_i("basic cmd");
        while (name_len < parse_cmd->string_len)
        {
            if ((at_name[name_len] == AT_CHAR_CR) || (at_name[name_len] == AT_CHAR_LF || (at_name[name_len] == '\0')))
            {
                break;
            }
            name_len++;
        }
        ret = AT_STATUS_OK;
        break;
    }
    parse_cmd->name_len = name_len;
    parse_cmd->parse_pos = name_len;

    return ret;
}

static at_status_t at_local_parse_cmd_mode(at_parse_cmd_param_ex_t *parse_cmd)
{
    at_cmd_mode_t mode = AT_CMD_MODE_INVALID;
    uint16_t index = parse_cmd->name_len;
    char *str_ptr = (char *)parse_cmd->string_ptr;

    if (str_ptr[index] == AT_CHAR_QUESTION_MARK)
    {
        index++;
        if (at_check_end_valid_char(str_ptr[index]) == AT_STATUS_OK)
        {
            mode = AT_CMD_MODE_READ;
        }
    }
    else if (str_ptr[index] == AT_CHAR_EQUAL)
    {
        index++;
        if (str_ptr[index] == AT_CHAR_QUESTION_MARK)
        {
            index++;
            if (at_check_end_valid_char(str_ptr[index]) == AT_STATUS_OK)
            {
                mode = AT_CMD_MODE_TESTING;
            }
        }
        else
        {
            mode = AT_CMD_MODE_EXECUTION;
        }
    }
    else if (at_check_end_valid_char(str_ptr[index]) == AT_STATUS_OK)
    {
        mode = AT_CMD_MODE_ACTIVE;
    }
    parse_cmd->parse_pos = index;
    parse_cmd->mode = mode;
    return AT_STATUS_OK;
}

static at_status_t at_input_cmd_hdlr(at_msg_t *input_data)
{
    at_status_t ret = AT_STATUS_ERROR;
    at_parse_cmd_param_ex_t parse_cmd[1];
    at_cmd_hdlr_item_t *handler_item = NULL;
    // uint16_t index = 0;
    uint32_t item_table_size;
    int i;
    int j;

    if (NULL == input_data)
    {
        at_log_e("input buf is null");
    }

    parse_cmd->string_ptr = (uint8_t *)input_data->data;
    parse_cmd->string_len = input_data->data_len;
    parse_cmd->hash_value1 = 0;
    parse_cmd->hash_value2 = 0;
    // parse_cmd->mode = g_at_msg_type;
    // parse_cmd->mode = AT_MSG_TYPE_CMD;

    // 将命令中含有字母的统一大写处理
    for (i = 0; i < parse_cmd->string_len; i++)
    {
        if (AT_CHAR_EQUAL == parse_cmd->string_ptr[i])
        {
            break;
        }
        // 数据中，=之前的字母统一大写处理，这个数据命令最长
        if (parse_cmd->string_ptr[i] >= 'a' && parse_cmd->string_ptr[i] < 'z')
        {
            parse_cmd->string_ptr[i] -= 32; // a-A=32
        }
    }

    at_local_parse_cmd_name(parse_cmd);
    at_local_parse_cmd_mode(parse_cmd);

    if (g_at_input_cmd_in_processing == AT_CMD_PROCESSING_PARSING)
    {
        g_at_input_cmd_in_processing = AT_CMD_PROCESSING_RESPONSE;
    }

    // Dispatch Handler
    for (i = 0; i < g_at_registered_table_number; i++)
    {
        item_table_size = g_at_cmd_hdlr_tables[i].item_table_size;

        if (ret == AT_STATUS_OK)
        {
            break;
        }

        for (j = 0; j < item_table_size; j++)
        {
            handler_item = &(g_at_cmd_hdlr_tables[i].item_table[j]);
            if (parse_cmd->hash_value1 == handler_item->hash_value1 && parse_cmd->hash_value2 == handler_item->hash_value2 &&
                !strncmp((char *)parse_cmd->string_ptr, handler_item->cmd_head, parse_cmd->name_len))
            {
                ret = AT_STATUS_OK;
                break;
            }
            handler_item = NULL;
        }
    }

    at_log_i("Parse cmd name len:%d, parse pos:%d , parse mode %d", parse_cmd->name_len, parse_cmd->parse_pos, parse_cmd->mode);
    /* executing AT command handler */
    if ((NULL != handler_item) && (ret == AT_STATUS_OK))
    {
        at_log_i("find at cmd %s", handler_item->cmd_head);
        handler_item->cmd_hdlr((at_parse_cmd_param_t *)parse_cmd);
    }
    else
    {
        at_log_e("find command handler fail \r\n", 0);
    }
}

static at_status_t at_input_cmd_parse(at_msg_t *input_data)
{
    at_status_t ret = AT_STATUS_ERROR;
    uint32_t length = 0;
    uint32_t find_valid_cmd = 0;
    char *data = (char *)input_data->data;
    at_log_i("parse data %s", data);

    g_at_input_cmd_in_processing = AT_CMD_PROCESSING_VALID;
    if (1 >= input_data->data_len)
    {
        at_log_e("command too short");
        ret = AT_STATUS_INVALID_CMD;
        at_response_error();
        goto out;
    }

    if (((data[0] != 'a') && (data[0] != 'A')) && ((data[1] != 't') && (data[1] != 'T')))
    {
        ret = AT_STATUS_INVALID_CMD;
        at_response_error();
        goto out;
    }

    for (length = (input_data->data_len - 1); length > 1; length--)
    {
        if ('\0' == data[length] ||
            AT_CHAR_CR == data[length] ||
            AT_CHAR_LF == data[length])
        {
            find_valid_cmd = 1;
            break;
        }
    }

    at_log_i("read data len is %d, cmd len is %d", input_data->data_len, length);

    if ((length > 3) && find_valid_cmd)
    {
        g_at_input_cmd_in_processing = AT_CMD_PROCESSING_PARSING;
        at_input_cmd_hdlr(input_data);
    }
    else if ((length == 3) && find_valid_cmd)
    { // if length == 3 , JUST AT<CR><LF> \ AT<CR> \AT<LF> AT
        g_at_input_cmd_in_processing = AT_CMD_PROCESSING_RESPONSE;
        at_response_ok();
    }
    else
    {
        at_response_error();
    }
    ret = AT_STATUS_OK;

out:
    return ret;
}

static at_status_t at_bypass_tcpip_parse(at_msg_id_t *input_data)
{

    return AT_STATUS_OK;
}

static at_status_t at_input_bypass_parse(at_msg_t *input_data)
{
    at_status_t ret = AT_STATUS_ERROR;
    uint16_t reload_flag = 0;
    char *data_str = (char *)input_data->data;
    if (input_data->id == AT_MSG_ID_READ_CMD)
    {
        if ((input_data->data_len == 3) && (at_bypass_timer_status() == 0))
        {
            if (strncmp(input_data->data, "+++", 4) == 0)
            {
                if (at_check_end_valid_char(data_str[3]) == AT_STATUS_OK)
                {
                    /** Reload timeout setting */
                    reload_flag = 1;
                    goto out;
                }
            }
        }
        // switch bypass status, tcp bypass, other bypass
        // tcp bypass, add cipsend=xxxx cmd
        ret = AT_STATUS_OK;
        at_tcp_send(input_data->data, input_data->data_len);
    }
    else if (input_data->id == AT_MSG_ID_SWITCH_TO_NORMAL)
    {
        at_change_data_mode(AT_MSG_TYPE_CMD);
        at_log_i("swich back to normal");
        at_send_data("\r\nOK\r\n", sizeof("\r\nOK\r\n"));
        ret = AT_STATUS_OK;
        goto out;
    }

out:
    at_log_i("at_bypass_timer_reload %d", reload_flag);
    at_bypass_timer_reload(1, reload_flag);
    return ret;
}

static at_status_t at_process_response_flag(uint8_t *str_ptr, uint16_t *str_len_ptr, uint16_t str_max_len, uint32_t flag)
{
    uint16_t str_len = (*str_len_ptr);
    int32_t i;
    // at_log_i("str:%s, leng:%d, max_len:%d, flag:0X%2x", str_ptr, *str_len_ptr, str_max_len, flag);

    if (AT_RESPONSE_FLAG_AUTO_APPEND_LF_CR == (flag & AT_RESPONSE_FLAG_AUTO_APPEND_LF_CR))
    {
        if (str_len + 2 > str_max_len)
        {
            return AT_STATUS_ERROR;
        }
        /* APPEND BACK CR LF */
        str_ptr[str_len] = AT_CHAR_CR;
        str_ptr[str_len + 1] = AT_CHAR_LF;
        str_len += 2;
        // at_log_i("str:%s, leng:%d, max_len:%d, flag:0X%2x", str_ptr, str_len, str_max_len, flag);
    }

    if (AT_RESPONSE_FLAG_APPEND_OK == (flag & AT_RESPONSE_FLAG_APPEND_OK))
    {
        /* append 'O','K',CR,LF */
        if (str_len + 4 > str_max_len)
        {
            return AT_STATUS_ERROR;
        }
        str_ptr[str_len] = AT_CHAR_O;
        str_ptr[str_len + 1] = AT_CHAR_K;
        str_ptr[str_len + 2] = AT_CHAR_CR;
        str_ptr[str_len + 3] = AT_CHAR_LF;
        str_len += 4;
        // at_log_i("str:%s, leng:%d, max_len:%d, flag:0X%2x", str_ptr, *str_len_ptr, str_max_len, flag);
    }
    if (AT_RESPONSE_FLAG_APPEND_ERROR == (flag & AT_RESPONSE_FLAG_APPEND_ERROR))
    {
        /* append 'E','R','R','O','R',CR,LF */
        if (str_len + 7 > str_max_len)
        {
            return AT_STATUS_ERROR;
        }
        str_ptr[str_len] = AT_CHAR_E;
        str_ptr[str_len + 1] = AT_CHAR_R;
        str_ptr[str_len + 2] = AT_CHAR_R;
        str_ptr[str_len + 3] = AT_CHAR_O;
        str_ptr[str_len + 4] = AT_CHAR_R;
        str_ptr[str_len + 5] = AT_CHAR_CR;
        str_ptr[str_len + 6] = AT_CHAR_LF;
        str_len += 7;
        // at_log_i("str:%s, leng:%d, max_len:%d, flag:0X%2x", str_ptr, str_len, str_max_len, flag);
    }

    if (AT_RESPONSE_FLAG_QUOTED_WITH_LF_CR == (flag & AT_RESPONSE_FLAG_QUOTED_WITH_LF_CR))
    {
        if (str_len + 4 > str_max_len)
        {
            return AT_STATUS_ERROR;
        }

        /* Left shift 2 position */
        for (i = str_len - 1; i >= 0; i--)
        {
            str_ptr[i + 2] = str_ptr[i];
            // at_log_i("for i=%d", i);
        }
        /* APPEND FRONT CR LF */
        str_ptr[0] = AT_CHAR_CR;
        str_ptr[1] = AT_CHAR_LF;
        str_len += 2;
        /* APPEND BACK CR LF */
        str_ptr[str_len] = AT_CHAR_CR;
        str_ptr[str_len + 1] = AT_CHAR_LF;
        str_len += 2;
        // at_log_i("str:%s, leng:%d, max_len:%d, flag:0X%2x", str_ptr, str_len, str_max_len, flag);
    }

    /* add null terminal at end of string buffer */
    if (str_max_len > (str_len))
    {
        str_ptr[str_len] = AT_CHAR_END_OF_STRING;
    }

    (*str_len_ptr) = str_len;

    return AT_STATUS_OK;
}

at_status_t at_response_error(void)
{
    return at_send_data("\r\nERROR\r\n", sizeof("\r\nERROR\r\n"));
}

at_status_t at_response_ok(void)
{
    return at_send_data("\r\nOK\r\n", sizeof("\r\nOK\r\n"));
}

at_status_t at_send_response(at_response_t *response)
{
    at_status_t ret = AT_STATUS_ERROR;
    at_msg_t msgq;
    uint8_t *msg_data = NULL;
    uint16_t msg_num = 0;

    // if (g_at_local_startup_flag != AT_STARTUP_FLAG_NORMAL) {
    //     at_log_e("AT send response failed, at do not ready");
    //     return ret;
    // }
    msg_num = at_msgq_get_num(g_at_cmd_response_msgq);
    if (msg_num >= AT_MAX_RESPONSE_MSGQ_NUM)
    {
        at_process_response_flag((uint8_t *)response->buf, (uint16_t *)&response->len,
                                 AT_TX_BUFFER_SIZE, response->flag);
        if (AT_RESPONSE_FLAG_URC_FORMAT == (response->flag & AT_RESPONSE_FLAG_URC_FORMAT))
        {
            at_log_w("at send response atci response queue full(%d), drop this urc data!\r\n", 1, msg_num);
        }
        else
        {

            if (g_at_input_cmd_in_processing == AT_CMD_PROCESSING_PARSING)
            {
                g_at_input_cmd_in_processing = AT_CMD_PROCESSING_RESPONSE;
            }

            at_log_w("at send response atci response queue full(%d), drop this rsp data!\r\n", 1, msg_num);
        }
        return ret;
    }

    ret = at_process_response_flag((uint8_t *)response->buf, (uint16_t *)&response->len,
                                   AT_TX_BUFFER_SIZE, response->flag);

    if (ret == AT_STATUS_ERROR)
    {
        at_log_w("process flag error, at send response len(%d) flag(%x), too long, drop!\r\n", response->len, response->flag);
        return ret;
    }

    // msg_data = (uint8_t *)at_malloc(response->len+sizeof(response->len));
    msg_data = (uint8_t *)at_malloc(response->len);
    memset(msg_data, 0, response->len);
    memcpy((void *)(msg_data), (uint8_t *)response->buf, response->len);

    msgq.id = AT_MSG_ID_RESPOSE_CMD;
    msgq.data_len = response->len;
    msgq.data = msg_data;

    ret = at_msgq_send(g_at_cmd_response_msgq, &msgq);

    return ret;
}

at_status_t at_send_heavy_response(at_response_heavy_data_t *response)
{
    return AT_STATUS_OK;
}

at_status_t at_send_cmd(uint8_t *cmd)
{
    int auto_append = 1;
    if (auto_append)
    {
        ;
        ;
    }
}

at_status_t at_change_data_mode(uint32_t mode)
{
    at_log_i("at data mode%d change to mode%d", g_at_msg_type, mode);
    g_at_msg_type = mode;
    return AT_STATUS_OK;
}

at_status_t at_input_msg_parse(at_msg_t *input_data)
{
    at_status_t ret = AT_STATUS_ERROR;
    // multi cmd or switch mode handle...
    // g_at_msg_type = AT_MSG_TYPE_BYPASS_DATA;
    switch (g_at_msg_type)
    {
    case AT_MSG_TYPE_CMD:
        ret = at_input_cmd_parse(input_data);
        break;
    case AT_MSG_TYPE_BYPASS_DATA:
        ret = at_input_bypass_parse(input_data);
        break;
    default:

        break;
    }

out:
    g_at_input_cmd_in_processing = AT_CMD_PROCESSING_RECEIVE;
    return ret;
}

at_status_t at_register_handler(at_cmd_hdlr_item_t *hdlr_items, int32_t hdlr_number)
{
    at_status_t ret = AT_STATUS_REGISTRATION_FAILURE;
    if (1)
    {
        ret = at_init_hdlr_tbl_hash_value(hdlr_items, hdlr_number);

        ret = at_add_to_cmd_hdlr_tbls(hdlr_items, hdlr_number);
    }

    return ret;
}