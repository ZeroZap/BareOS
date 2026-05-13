#include "at_cmd.h"

extern at_status_t at_change_data_mode(uint32_t mode);
at_status_t at_cmd_hdlr_ato(at_parse_cmd_param_t *parse_cmd)
{

    at_response_t response = {{0}};

    at_change_data_mode(AT_DATA_BYPASS);
    response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
    response.flag |=AT_RESPONSE_FLAG_QUOTED_WITH_LF_CR;
    response.len = strlen((char*) response.buf);
    at_send_response(&response);

    return AT_STATUS_OK;
}