#include "at_cmd.h"

#define EXTERN_AT_CMD_HANDLER(hdl) extern at_status_t hdl(at_parse_cmd_param_t *parse_cmd);

/** Basic  */
EXTERN_AT_CMD_HANDLER(at_cmd_hdlr_module_info)
EXTERN_AT_CMD_HANDLER(at_cmd_hdlr_ato)

/** Network */
EXTERN_AT_CMD_HANDLER(at_cmd_hdlr_csq)
EXTERN_AT_CMD_HANDLER(at_cmd_hdlr_creg)
EXTERN_AT_CMD_HANDLER(at_cmd_hdlr_cereg)
EXTERN_AT_CMD_HANDLER(at_cmd_hdlr_cgreg)

/** TCPIP */
EXTERN_AT_CMD_HANDLER(at_cmd_tcpip_open)
EXTERN_AT_CMD_HANDLER(at_cmd_tcpip_close)

/*Requrst*/
EXTERN_AT_CMD_HANDLER(at_cmd_hdlr_cgsn)
EXTERN_AT_CMD_HANDLER(at_cmd_hdlr_cclk)
EXTERN_AT_CMD_HANDLER(at_cmd_hdlr_creset)
EXTERN_AT_CMD_HANDLER(at_cmd_hdlr_ciccid)
EXTERN_AT_CMD_HANDLER(at_cmd_hdlr_cpsi)
EXTERN_AT_CMD_HANDLER(at_cmd_hdlr_cspn)
EXTERN_AT_CMD_HANDLER(at_cmd_hdlr_cops)
// EXTERN_AT_CMD_HANDLER(at_cmd_hdlr_cgdcont)
EXTERN_AT_CMD_HANDLER(at_cmd_hdlr_cfun)
EXTERN_AT_CMD_HANDLER(at_cmd_hdlr_cpin)
EXTERN_AT_CMD_HANDLER(at_cmd_hdlr_cgdrt)
EXTERN_AT_CMD_HANDLER(at_cmd_hdlr_cgsetv)
EXTERN_AT_CMD_HANDLER(at_cmd_hdlr_cggetv)
EXTERN_AT_CMD_HANDLER(at_cmd_hdlr_cadc)
EXTERN_AT_CMD_HANDLER(at_cmd_hdlr_cadc2)
EXTERN_AT_CMD_HANDLER(at_cmd_hdlr_sn)
EXTERN_AT_CMD_HANDLER(at_cmd_hdlr_setuart)
EXTERN_AT_CMD_HANDLER(at_cmd_hdlr_simid)

/*MQTT*/
EXTERN_AT_CMD_HANDLER(at_cmd_hdlr_cmqttstart)
EXTERN_AT_CMD_HANDLER(at_cmd_hdlr_cmqttstop)
EXTERN_AT_CMD_HANDLER(at_cmd_hdlr_cmqttaccq)
EXTERN_AT_CMD_HANDLER(at_cmd_hdlr_cmqttrel)
EXTERN_AT_CMD_HANDLER(at_cmd_hdlr_cmqttconnect)
EXTERN_AT_CMD_HANDLER(at_cmd_hdlr_cmqtttopic)
EXTERN_AT_CMD_HANDLER(at_cmd_hdlr_cmqttpayload)
EXTERN_AT_CMD_HANDLER(at_cmd_hdlr_cmqttpub)
EXTERN_AT_CMD_HANDLER(at_cmd_hdlr_cmqttsubtopic)
EXTERN_AT_CMD_HANDLER(at_cmd_hdlr_cmqttsub)

/**
 * AT Command Table
 *
 */
const at_cmd_hdlr_item_t g_at_cmd_items[] = {
    // Basic Command Table
    {"ATI", at_cmd_hdlr_module_info, 0, 0},
    {"ATO", at_cmd_hdlr_ato, 0, 0},
    {"AT+CSQ", at_cmd_hdlr_csq, 0, 0},
    {"AT+CREG", at_cmd_hdlr_creg, 190805, 0},
    {"AT+CGREG", at_cmd_hdlr_cgreg, 6665701, 0},
    {"AT+CEREG", at_cmd_hdlr_cereg, 6555957, 0},
    {"AT+CIPOPEN", at_cmd_tcpip_open, 29495730, 204},
    {"AT+CIPCLOSE", at_cmd_tcpip_close, 29495270, 22387},
    {"AT+CGSN", at_cmd_hdlr_cgsn, 175460, 0},
    {"AT+CCLK", at_cmd_hdlr_cclk, 169415, 0},
    {"AT+CRESET", at_cmd_hdlr_creset, 7251051, 20},
    {"AT+CICCID", at_cmd_hdlr_ciccid, 6753711, 4},
    {"AT+CPSI", at_cmd_hdlr_cpsi, 188451, 0},
    {"AT+CSPN", at_cmd_hdlr_cspn, 192674, 0},
    {"AT+COPS", at_cmd_hdlr_cops, 186903, 0},
    // {"AT+CGDCONT", at_cmd_hdlr_cgdcont, 6645417, 552},
    {"AT+CFUN", at_cmd_hdlr_cfun, 174092, 0},
    {"AT+CPIN", at_cmd_hdlr_cpin, 188076, 0},
    {"AT+CGDRT", at_cmd_hdlr_cgdrt, 6645992, 0},
    {"AT+CGSETV", at_cmd_hdlr_cgsetv, 6667158, 22},
    {"AT+CGGETV", at_cmd_hdlr_cggetv, 6649830, 22},
    {"AT+CADC", at_cmd_hdlr_cadc, 166215, 0},
    {"AT+CADC2", at_cmd_hdlr_cadc2, 6316191, 0},
    {"AT+SN", at_cmd_hdlr_sn, 736, 0},
    {"AT+SETUART", at_cmd_hdlr_setuart, 39921623, 704},
    {"AT+SIMID", at_cmd_hdlr_simid, 40130550, 0},

    {"AT+CMQTTSTART", at_cmd_hdlr_cmqttstart, 6994072, 40717172},
    {"AT+CMQTTSTOP", at_cmd_hdlr_cmqttstop, 6994072, 1072034},
    {"AT+CMQTTACCQ", at_cmd_hdlr_cmqttaccq, 6994072, 59335},
    {"AT+CMQTTREL", at_cmd_hdlr_cmqttrel, 6994072, 26194},
    {"AT+CMQTTCONNECT", at_cmd_hdlr_cmqttconnect, 6994072, 7099241},
    {"AT+CMQTTTOPIC", at_cmd_hdlr_cmqtttopic, 6994072, 42549249},
    {"AT+CMQTTPAYLOAD", at_cmd_hdlr_cmqttpayload, 6994072, 33453619},
    {"AT+CMQTTPUB", at_cmd_hdlr_cmqttpub, 6994072, 23904},
    {"AT+CMQTTSUBTOPIC", at_cmd_hdlr_cmqttsubtopic, 6994072, 40773559},
    {"AT+CMQTTSUB", at_cmd_hdlr_cmqttsub, 6994072, 28236}
	
    //{"AT+CPARAMALL", at_cmd_param_set_all, 40130550, 0}
};

at_status_t at_cmd_init(void)
{
    at_status_t ret = AT_STATUS_REGISTRATION_FAILURE;

    ret = at_register_handler(g_at_cmd_items, sizeof(g_at_cmd_items) / sizeof(at_cmd_hdlr_item_t));

    if (ret == AT_STATUS_OK)
    {
        at_log_i("at cmd init register success!");
    }
    else
    {
        at_log_e("at cmd init register success!");
    }

    return ret;
}