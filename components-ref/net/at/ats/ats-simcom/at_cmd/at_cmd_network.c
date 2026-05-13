#include "uvr.h"

at_status_t at_cmd_hdlr_csq(at_parse_cmd_param_t* parse_cmd)
{
    at_response_t response = { {0} };
    at_status_t ret;
    uint8_t csq;
    switch (parse_cmd->mode)
    {
    case  AT_CMD_MODE_READ:
        response.flag |= AT_RESPONSE_FLAG_APPEND_ERROR;
        break;
    case AT_CMD_MODE_ACTIVE:
        ret = sAPI_NetworkGetCsq(&csq);
        if (ret == AT_STATUS_OK) {
            sprintf((char *)response.buf, "+CSQ: %d,99\r\n",csq);
            response.flag |= AT_RESPONSE_FLAG_APPEND_OK;

        } else {
            response.flag |= AT_RESPONSE_FLAG_APPEND_ERROR;
        }
        break;
    case AT_CMD_MODE_EXECUTION:
        response.flag |= AT_RESPONSE_FLAG_APPEND_ERROR;
        break;
    case AT_CMD_MODE_TESTING:
        strcpy((char *)response.buf, "+CSQ: (0-31,99),(0-7,99)");
        break;
    default:
        response.flag |= AT_RESPONSE_FLAG_APPEND_ERROR;
        break;
    }

    if (1) {
        response.flag |= AT_RESPONSE_FLAG_AUTO_APPEND_LF_CR;
    }
    response.len = strlen((char*)response.buf);
    at_send_response(&response);
    return AT_STATUS_OK;
}

at_status_t at_cmd_hdlr_creg(at_parse_cmd_param_t* parse_cmd)
{
    at_response_t response = { {0} };
    at_status_t ret;
    int creg;
    int urc_status = 0;
    switch (parse_cmd->mode)
    {
    case  AT_CMD_MODE_READ:
        ret = sAPI_NetworkGetCreg(&creg);
        if (ret == AT_STATUS_OK) {
            sprintf((char *)response.buf, "+CREG: %d, %d\r\n", urc_status, creg);
            response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
        } else {
            response.flag |= AT_RESPONSE_FLAG_APPEND_ERROR;
        }
        break;
    case AT_CMD_MODE_ACTIVE:
        response.flag |= AT_RESPONSE_FLAG_APPEND_OK;

        break;
    case AT_CMD_MODE_EXECUTION:
        response.flag |= AT_RESPONSE_FLAG_APPEND_ERROR;
        break;
    case AT_CMD_MODE_TESTING:
        strcpy((char *)response.buf, "+CREG: (0-2)");
        response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
        break;
    default:
        response.flag |= AT_RESPONSE_FLAG_APPEND_ERROR;
        break;
    }

    if (1) {
        response.flag |= AT_RESPONSE_FLAG_AUTO_APPEND_LF_CR;
    }
    response.len = strlen((char*)response.buf);
    at_send_response(&response);
    return AT_STATUS_OK;
}

at_status_t at_cmd_hdlr_cgreg(at_parse_cmd_param_t* parse_cmd)
{
    at_response_t response = { {0} };
    at_status_t ret;
    int cgreg;
    int urc_status = 0;
    switch (parse_cmd->mode)
    {
    case  AT_CMD_MODE_READ:
        ret = sAPI_NetworkGetCgreg(&cgreg);
        if (ret == AT_STATUS_OK) {
            sprintf((char *)response.buf, "+CGREG: %d, %d\r\n", urc_status, cgreg);
            response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
        } else {
            response.flag |= AT_RESPONSE_FLAG_APPEND_ERROR;
        }
        break;
    case AT_CMD_MODE_ACTIVE:
        response.flag |= AT_RESPONSE_FLAG_APPEND_OK;

        break;
    case AT_CMD_MODE_EXECUTION:
        response.flag |= AT_RESPONSE_FLAG_APPEND_ERROR;
        break;
    case AT_CMD_MODE_TESTING:
        strcpy((char *)response.buf, "+CGREG: (0-2)");
        response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
        break;
    default:
        response.flag |= AT_RESPONSE_FLAG_APPEND_ERROR;
        break;
    }

    if (1) {
        response.flag |= AT_RESPONSE_FLAG_AUTO_APPEND_LF_CR;
    }
    response.len = strlen((char*)response.buf);
    at_send_response(&response);
    return AT_STATUS_OK;
}

at_status_t at_cmd_hdlr_cereg(at_parse_cmd_param_t* parse_cmd)
{
    at_response_t response = { {0} };
    at_status_t ret;
    int cereg;
    int urc_status = 0;
    switch (parse_cmd->mode)
    {
    case  AT_CMD_MODE_READ:
        ret = sAPI_NetworkGetCgreg(&cereg);
        if (ret == AT_STATUS_OK) {
            sprintf((char *)response.buf, "+CEREG: %d, %d\r\n", urc_status, cereg);
            response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
        } else {
            response.flag |= AT_RESPONSE_FLAG_APPEND_ERROR;
        }
        break;
    case AT_CMD_MODE_ACTIVE:
        response.flag |= AT_RESPONSE_FLAG_APPEND_OK;

        break;
    case AT_CMD_MODE_EXECUTION:
        response.flag |= AT_RESPONSE_FLAG_APPEND_ERROR;
        break;
    case AT_CMD_MODE_TESTING:
        strcpy((char *)response.buf, "+CEREG: (0-2)");
        response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
        break;
    default:
        response.flag |= AT_RESPONSE_FLAG_APPEND_ERROR;
        break;
    }

    if (1) {
        response.flag |= AT_RESPONSE_FLAG_AUTO_APPEND_LF_CR;
    }
    response.len = strlen((char*)response.buf);
    at_send_response(&response);
    return AT_STATUS_OK;
}

// COPS
at_status_t at_cmd_hdlr_cops(at_parse_cmd_param_t *parse_cmd)
{
	at_response_t response = {{0}};
	at_status_t ret;
	char cops[100] = {0};
	int modeVal = 0;
	int formatVal = 0;
	int accTchVal = 0;
	char networkOperator[20] = " ";

	char *param = NULL;
	int str_len = 0;

	switch (parse_cmd->mode)
	{

	case AT_CMD_MODE_READ:
		// ret获取到的是0，cops获取的总是0
		ret = sAPI_NetworkGetCops(cops);
		at_log_d("cops=%s", cops);
		if (ret == AT_STATUS_OK)
		{
			sprintf((char *)response.buf, "%s\r\n", cops);
			// uvr_log_i("response.buf=%s", response.buf);
			response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
		}
		else
		{
			uvr_log_e("network get cops err,ret = %d", ret);
			response.flag |= AT_RESPONSE_FLAG_APPEND_ERROR;
		}
		break;

	case AT_CMD_MODE_TESTING:
		strcpy((char *)response.buf, "+COPS: (2,\"CHN-UNICOM\",\"UNICOM\",\"46001\",7),(1,\"CHN-UNICOM\",\"UNICOM\", \"46001\",2),(1,\"CHN-UNICOM\",\"UNICOM\",\"46001\",0),(3,\"CHINA MOBILE\",\"CMCC\",\"46000\",7),(3,\"CHN-CT\",\"CT\",\"46011\",7),(3,\"CHINA MOBILE\",\"CMCC\",\"46000\", 0),,(0,1,2,3,4),(0,1,2)\r\n");
		response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
		break;

	case AT_CMD_MODE_ACTIVE:
		response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
		break;

	case AT_CMD_MODE_EXECUTION:

		param = &parse_cmd->raw_data[parse_cmd->parse_pos];
		// at_log_d("param=%s", param);

		// get param: modeVal
		param = strtok(param, ",");
		modeVal = atoi(param);
		// at_log_d("modeVal is %d", modeVal);

		// get param: formatVal
		param = strtok(NULL, ",");
		formatVal = atoi(param);
		// at_log_d("formatVal is %d", formatVal);

		// get param: networkOperator
		param = strtok(NULL, ",");
		str_len = strlen(param);
		// at_log_i("param len is %d, str is %s", str_len, param);
		memset(networkOperator, 0, str_len - 1);
		memcpy(networkOperator, param + 1, str_len - 2);
		// at_log_i("get server networkOperator%s", networkOperator);

		// get param: accTchVal
		param = strtok(NULL, ",");
		accTchVal = atoi(param);
		// at_log_d("accTchVal is %d", accTchVal);

		ret = sAPI_NetworkSetCops(modeVal, formatVal, networkOperator, accTchVal);

		if (ret == AT_STATUS_OK)
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




// void _at_gdcont_execution(char *cmd_str, at_response_t *response)
// {

// }

// at_status_t at_cmd_hdlr_gdcont(at_parse_cmd_param_t* parse_cmd)
// {

// }

