
#include "simcom_tcpip.h"
#include "uvr.h"

#define TCPIP_CID 1
typedef struct _tcpip_config
{
    int overtime;
    char *ip;
    uint16_t port;
    int channel;

} tcpip_config;

tcpip_config g_tcpip_config = {
    100,
    NULL,
    1883,
    1,
};

sMsgQRef tcpip_deactive_msgq = NULL;
int g_tcpip_pdp_status = 0;

extern at_status_t at_change_data_mode(uint32_t mode);
extern int at_tcpip_open(char *server_ipstr, uint16_t server_port);

int tcp_init(int channel)
{
    return 0;
}

int tcp_deinit(void)
{
    return 0;
}

int tcpip_open_exec(at_parse_cmd_param_t *parse_cmd, at_response_t *response)
{
    char *param = NULL;
    int at_tcp_pdp_id = 0;
    char *ip_addr = NULL;
    uint16_t port = 0;
    int str_len = 0;
    int ret = -1;

    // no need
    // if (strstr(parse_cmd->raw_data, "AT+CIPOPEN=") == NULL) {
    //     return -1;
    // }

    // get params: all
    param = &parse_cmd->raw_data[parse_cmd->parse_pos];

    // get param: pdp id
    param = strtok(param, ",");
    at_tcp_pdp_id = atoi(param);
    if ((at_tcp_pdp_id < 0) || (at_tcp_pdp_id > 9))
    {
        goto out;
    }
    at_log_d("at_tcp_pdp_id is %d", at_tcp_pdp_id);

    // get param: ipaddr
    param = strtok(NULL, ",");
    if (NULL == param)
    {
        at_log_e("Can not found server ip address");
        goto out;
    }
    str_len = strlen(param);
    at_log_i("param len is %d, str is %s", str_len, param);
    ip_addr = malloc(str_len - 1);
    memset(ip_addr, 0, str_len - 1);
    memcpy(ip_addr, param + 1, str_len - 2);
    at_log_i("get server ip addr:%s", ip_addr);

    param = strtok(NULL, ",");
    if (NULL == param)
    {
        at_log_e("Can not found server ip port");
        goto out;
    }

    port = atoi(param);
    at_log_i("get server port:%d", port);

    if (AT_STATUS_OK == at_tcpip_open(ip_addr, port))
    {
        at_change_data_mode(AT_DATA_BYPASS);
    }

    // get others params
    ret = 0;

out:
    if (NULL != ip_addr)
    {
        at_free(ip_addr);
        ip_addr = NULL;
    }
    return ret;
}

int tcpip_open_read(at_parse_cmd_param_t *parse_cmd, at_response_t *response)
{
    return 0;
}

int tcpip_close_exec(at_parse_cmd_param_t *parse_cmd, at_response_t *response)
{
    return 0;
}

int tcpip_close_read(at_parse_cmd_param_t *parse_cmd, at_response_t *response)
{
    return 0;
}

at_status_t at_cmd_tcpip_open(at_parse_cmd_param_t *parse_cmd)
{
    at_response_t response = {{0}};
    // int ret=0;
    // int urc_status = 0;
    switch (parse_cmd->mode)
    {

    case AT_CMD_MODE_TESTING:
        strcpy((char *)response.buf, "+CIPOPEN:(0-9),(IP),(0-65535) \r\n");
        response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
        break;
    case AT_CMD_MODE_READ:
        tcpip_open_read(parse_cmd, &response);
        break;
    case AT_CMD_MODE_EXECUTION:
        if (AT_STATUS_OK == tcpip_open_exec(parse_cmd, &response))
        {
            response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
        }
        else
        {
            response.flag |= AT_RESPONSE_FLAG_APPEND_ERROR;
        }
        break;
    default:
        response.flag |= AT_RESPONSE_FLAG_APPEND_ERROR;
        break;
    }

    if (1)
    {
        response.flag |= AT_RESPONSE_FLAG_AUTO_APPEND_LF_CR;
    }
    response.len = strlen((char *)response.buf);
    at_send_response(&response);
    return AT_STATUS_OK;
}

at_status_t at_cmd_tcpip_close(at_parse_cmd_param_t *parse_cmd)
{
    return AT_STATUS_OK;
}

// at_status_t at_cmd_tcpip_send(at_parse_cmd_param_t* parse_cmd)
// {
//     return AT_STATUS_OK;
// }
