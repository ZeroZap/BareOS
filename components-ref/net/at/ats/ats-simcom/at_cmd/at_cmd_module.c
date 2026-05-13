#include "at_cmd.h"

#define STRINGIFY2(x) #x
// 上述定义如果参数是一个宏，参数不会展开，所以需要这个宏做中间转换
#define STRINGIFY(x) STRINGIFY2(x)

#define SW_MAJOR_VERSION   0
#define SW_MINOR_VERSION1  1
#define SW_MINOR_VERSION2  2
#define SW_MINOR_VERSION3  3
#define PRODUCT_VERSION STRINGIFY(SW_MAJOR_VERSION.SW_MINOR_VERSION1.SW_MINOR_VERSION2.SW_MINOR_VERSION3)
at_status_t at_cmd_hdlr_module_info(at_parse_cmd_param_t *parse_cmd)
{
    at_response_t response = {{0}};

    // at_log_i("welcome ati");
    switch (parse_cmd->mode)
    {
    case AT_CMD_MODE_ACTIVE:
        at_log_i("UVR ATI");
        strcpy((char *)response.buf, "+ATI: UVR D62554\r\n" "Ver"PRODUCT_VERSION "\r\n"  __DATE__ " " __TIME__ "\r\n");
        // response.flag  |= AT_RESPONSE_FLAG_QUOTED_WITH_LF_CR;
        // response.flag  |= AT_RESPONSE_FLAG_AUTO_APPEND_LF_CR;
        response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
        // response.flag |= AT_RESPONSE_FLAG_APPEND_ERROR;
        // response.flag |= AT_RESPONSE_FLAG_AUTO_APPEND_LF_CR;
        response.flag |=AT_RESPONSE_FLAG_QUOTED_WITH_LF_CR;
        response.len = strlen((char*) response.buf);
        at_send_response(&response);
        break;
    default:
        break;
    }
    return AT_STATUS_OK;
}