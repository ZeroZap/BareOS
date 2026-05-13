#include "uvr.h"
#include "simcom_rtc.h"

extern struct _uvr_data g_uvr_data_t;

// 请求序列号标识--cgsn
at_status_t at_cmd_hdlr_cgsn(at_parse_cmd_param_t *parse_cmd)
{
	at_response_t response = {{0}};
	at_status_t ret;
	char imei[50] = {0};
	switch (parse_cmd->mode)
	{
	case AT_CMD_MODE_TESTING:
		response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
		break;

	case AT_CMD_MODE_ACTIVE:
		ret = sAPI_SysGetImei(imei);
		if (ret == AT_STATUS_OK)
		{
			sprintf((char *)response.buf, "%s\r\n", imei);
			response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
		}
		else
		{
			response.flag |= AT_RESPONSE_FLAG_APPEND_ERROR;
		}
		break;

		// case AT_CMD_MODE_EXECUTION:
		// 	response.flag |= AT_RESPONSE_FLAG_APPEND_ERROR;
		// 	break;

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
// 实时时间--cclk
sMsgQRef ntpUIResp_msgq;
at_status_t at_cmd_hdlr_cclk(at_parse_cmd_param_t *parse_cmd)
{
	at_response_t response = {{0}};
	// at_status_t ret;
	// sMsgQRef htpUIResp_msgq;
	// sTimeval tv = {0, 0};
	INT32 ret;

	SCsysTime_t currUtcTime;
	char buff[220] = {0};
	// char *resp = NULL;
	SIM_MSG_T ntp_result = {SC_SRV_NONE, -1, 0, NULL};

	switch (parse_cmd->mode)
	{
	case AT_CMD_MODE_READ:

		// 消息队列ntpUIResp_msgq
		if (NULL == ntpUIResp_msgq)
		{
			SC_STATUS status;
			status = sAPI_MsgQCreate(&ntpUIResp_msgq, "htpUIResp_msgq", sizeof(SIM_MSG_T), 4, SC_FIFO);
			if (SC_SUCCESS != status)
			{
				uvr_log_i("[CNTP]msgQ create fail");
				// resp = "\r\nNTP Update Fail!\r\n";
				// sAPI_UartWrite(SC_UART, (UINT8 *)resp, strlen(resp));
				break;
			}
		}
		memset(&currUtcTime, 0, sizeof(currUtcTime));

		// 获取本地时间
		sAPI_GetSysLocalTime(&currUtcTime);
		uvr_log_i("[CNTP] sAPI_GetSysLocalTime %d/%d/%d,%d:%d:%d %d", currUtcTime.tm_year, currUtcTime.tm_mon,
							currUtcTime.tm_mday,
							currUtcTime.tm_hour, currUtcTime.tm_min, currUtcTime.tm_sec, currUtcTime.tm_wday);

		// 调用sAPI_NtpUpdate函数更新NTP时间，设置超时时间为32秒
		ret = sAPI_NtpUpdate(SC_NTP_OP_SET, "ntp3.aliyun.com", 32, NULL); // Unavailable addr may cause long time suspend
		uvr_log_i("ret[%d]", ret);

		// 获取更新后的时间，并将结果输出到buff中
		ret = sAPI_NtpUpdate(SC_NTP_OP_GET, buff, 0, NULL);
		uvr_log_i("ret[%d] buff[%s]", ret, buff);

		// 发送一个请求到NTP服务器，并通过消息队列ntpUIResp_msgq接收相应结果
		ret = sAPI_NtpUpdate(SC_NTP_OP_EXC, NULL, 0, ntpUIResp_msgq);
		uvr_log_i(" ret[%d] ", ret);

		// 循环获取消息队列中的消息，如果接收到的信息不是NTP服务的相应信息，则继续等待直到接收到正确的消息。
		do
		{
			ret = sAPI_MsgQRecv(ntpUIResp_msgq, &ntp_result, SC_SUSPEND);
			uvr_log_i("msg rec ret = %d", ret);

			if (SC_SRV_NTP != ntp_result.msg_id) // wrong msg received
			{
				at_log_d("[CNTP] ntp_result.msg_id =[%d]", ntp_result.msg_id);
				ntp_result.msg_id = SC_SRV_NONE; // para reset
				ntp_result.arg1 = -1;
				ntp_result.arg3 = NULL;
				continue;
			}

			// 如果接收到的NTP服务的响应码arg1为SC_NTP_OK，则表示更新成功，将提示信息输出到终端窗口
			if (SC_NTP_OK == ntp_result.arg1) // it means update succeed
			{
				sAPI_Debug("[CNTP] successfully update time! ");
				// PrintfResp("\r\nNTP Update Time Successful!\r\n");
				break;
			}
			// 否则将提示更新失败信息
			else
			{
				sAPI_Debug("[CNTP] failed to update time! result code: %d", ntp_result.arg1);
				// PrintfResp("\r\nNTP Update Time Failed!\r\n");
				break;
			}

		} while (1);
		// 打印出更新后的本地时间。
		memset(&currUtcTime, 0, sizeof(currUtcTime));
		sAPI_GetSysLocalTime(&currUtcTime);
		memset(&ntp_result, 0, sizeof(ntp_result));
		if (ret == AT_STATUS_OK)
		{
			sprintf((char *)response.buf, "+CCLK: %d/%d/%d,%d:%d:%d %d \r\n", currUtcTime.tm_year, currUtcTime.tm_mon,
							currUtcTime.tm_mday,
							currUtcTime.tm_hour, currUtcTime.tm_min, currUtcTime.tm_sec, currUtcTime.tm_wday);
			uvr_log_i("%s", response.buf);
			response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
			sAPI_Free(ntp_result.arg3);
		}
		else
		{
			response.flag |= AT_RESPONSE_FLAG_APPEND_ERROR;
		}
		break;

	case AT_CMD_MODE_TESTING:
		response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
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
// 重启--creset
at_status_t at_cmd_hdlr_creset(at_parse_cmd_param_t *parse_cmd)
{
	at_response_t response = {{0}};

	switch (parse_cmd->mode)
	{
	case AT_CMD_MODE_TESTING:
		response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
		break;

	case AT_CMD_MODE_ACTIVE:
		sAPI_SysReset();
		response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
		break;

		// case AT_CMD_MODE_EXECUTION:
		// 	response.flag |= AT_RESPONSE_FLAG_APPEND_ERROR;
		// 	break;

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

// 读取ICCID--ciccid
at_status_t at_cmd_hdlr_ciccid(at_parse_cmd_param_t *parse_cmd)
{
	at_response_t response = {{0}};
	at_status_t ret;
	// SC_simcard_err_e ret;
	char iccid[32] = {0};

	switch (parse_cmd->mode)
	{
	case AT_CMD_MODE_ACTIVE:
		ret = sAPI_SysGetIccid(iccid);
		if (ret == AT_STATUS_OK)
		{
			// sAPI_Debug("+ICCID:%s\r\n", iccid);
			sprintf((char *)response.buf, "+CICCID: %s\r\n", iccid);
			response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
		}
		else
		{
			response.flag |= AT_RESPONSE_FLAG_APPEND_ERROR;
		}
		break;

	case AT_CMD_MODE_TESTING:
		response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
		break;

	case AT_CMD_MODE_EXECUTION:
		response.flag |= AT_RESPONSE_FLAG_APPEND_ERROR;
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

// 查询UE系统信息--cpsi
at_status_t at_cmd_hdlr_cpsi(at_parse_cmd_param_t *parse_cmd)
{
	at_response_t response = {{0}};
	at_status_t ret;
	// SC_simcard_err_e ret;
	SCcpsiParm Scpsi;

	switch (parse_cmd->mode)
	{
	case AT_CMD_MODE_READ:
		ret = sAPI_NetworkGetCpsi(&Scpsi);
		if (ret == AT_STATUS_OK)
		{
			// sprintf((char *)response.buf, "+CPSI: \r\n %s,%s,%d,%d,%s,%s,%d\r\n",
			// 				Scpsi.networkmode, Scpsi.Mnc_Mcc, Scpsi.LAC, Scpsi.CellID, Scpsi.GSMBandStr, Scpsi.LTEBandStr, Scpsi.TAC);
			sprintf((char *)response.buf, "+CPSI: \r\n %s,%s,%d,%d,%s,%s,%d,%d,%d,%d,%d,%d\r\n",
							Scpsi.networkmode, Scpsi.Mnc_Mcc, Scpsi.LAC, Scpsi.CellID, Scpsi.GSMBandStr, Scpsi.LTEBandStr, Scpsi.TAC,
							Scpsi.Rsrp, Scpsi.RXLEV, Scpsi.TA, Scpsi.SINR, Scpsi.Rssi);
			response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
		}
		else
		{
			response.flag |= AT_RESPONSE_FLAG_APPEND_ERROR;
		}
		break;

	case AT_CMD_MODE_TESTING:
		response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
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

// 从SIM卡中获取服务提供商的名称--cspn
at_status_t at_cmd_hdlr_cspn(at_parse_cmd_param_t *parse_cmd)
{
	at_response_t response = {{0}};
	at_status_t ret;
	// SC_simcard_err_e ret;
	Hplmn_st HplmnValue;

	switch (parse_cmd->mode)
	{
	case AT_CMD_MODE_READ:
		ret = sAPI_SysGetHplmn(&HplmnValue);

		int i = 0;
		if (ret == AT_STATUS_OK)
		{
			if (HplmnValue.mnc != NULL)
			{
				i = 1;
			}
			else
				i = 0;

			// sprintf((char *)response.buf, "+CSPN: \"%s\",%s\r\n", HplmnValue.spn, HplmnValue.mnc);
			sprintf((char *)response.buf, "+CSPN: \"%s\",%d\r\n", HplmnValue.spn, i);
			response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
		}
		else
		{
			response.flag |= AT_RESPONSE_FLAG_APPEND_ERROR;
		}
		break;

	case AT_CMD_MODE_TESTING:
		response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
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

// pin---cpin
at_status_t at_cmd_hdlr_cpin(at_parse_cmd_param_t *parse_cmd)
{
	at_response_t response = {{0}};
	at_status_t ret;

	// UINT8 cpin = 0;
	uint8_t cpin = 0;

	char *cpin1 = NULL;

	// 0 : READY 1 : PIN 2 : PUK 3 : BLK 4 : REMV 5 : CRASH 6 : NOINSRT 7 : UNKN

	switch (parse_cmd->mode)
	{
	case AT_CMD_MODE_READ:
		ret = sAPI_SimcardPinGet(&cpin);
		if (cpin == 0)
		{
			cpin1 = "READY";
		}
		if (cpin == 1)
		{
			cpin1 = "PIN";
		}
		if (cpin == 2)
		{
			cpin1 = "PUK";
		}
		if (cpin == 3)
		{
			cpin1 = "BLK";
		}
		if (cpin == 4)
		{
			cpin1 = "REMV";
		}
		if (cpin == 5)
		{
			cpin1 = "CRASH";
		}
		if (cpin == 6)
		{
			cpin1 = "NOINSRT";
		}
		if (cpin == 7)
		{
			cpin1 = "UNKN";
		}
		if (ret == AT_STATUS_OK)
		{
			sprintf((char *)response.buf, "+CPIN:%s\r\n", cpin1);
			response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
		}
		else
		{
			response.flag |= AT_RESPONSE_FLAG_APPEND_ERROR;
		}
		break;

	case AT_CMD_MODE_TESTING:
		response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
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

// 设置指定GPIO--cgdrt
at_status_t at_cmd_hdlr_cgdrt(at_parse_cmd_param_t *parse_cmd)
{
	at_response_t response = {{0}};
	at_status_t ret;

	unsigned int gpio;
	unsigned int direction;
	// unsigned int direction1;

	char *param = NULL;

	switch (parse_cmd->mode)
	{

	case AT_CMD_MODE_TESTING:
		strcpy((char *)response.buf, "+CGDRT: (1,2,3,6,12,14,16,18,22,41,43,63,77),(0-1)\r\n");
		response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
		break;

	case AT_CMD_MODE_EXECUTION:
		// sAPI_GpioGetDirection(direction1);
		param = &parse_cmd->raw_data[parse_cmd->parse_pos];
		// get param: gpio
		param = strtok(param, ",");
		gpio = atoi(param);
		// at_log_d("modeVal is %d", modeVal);

		// get param: direction
		param = strtok(NULL, ",");
		direction = atoi(param);

		if (param == NULL)
		{
			ret = sAPI_GpioSetDirection(gpio, direction);
			if (ret == AT_STATUS_OK)
			{
				sprintf((char *)response.buf, "+CGDRT: %d,%d\r\n", gpio, direction);
				response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
			}
			else
			{
				response.flag |= AT_RESPONSE_FLAG_APPEND_ERROR;
			}
		}

		else
		{
			ret = sAPI_GpioSetDirection(gpio, direction);
			if (ret == AT_STATUS_OK)
			{
				response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
			}
			else
			{
				response.flag |= AT_RESPONSE_FLAG_APPEND_ERROR;
			}
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

// 设置指定GPIO的值--cgsetv
at_status_t at_cmd_hdlr_cgsetv(at_parse_cmd_param_t *parse_cmd)
{
	at_response_t response = {{0}};
	at_status_t ret;

	unsigned int gpio, value = 0;
	char *param = NULL;

	switch (parse_cmd->mode)
	{

	case AT_CMD_MODE_TESTING:
		strcpy((char *)response.buf, "+CGSETV: (1,2,3,6,12,14,16,18,22,41,43,63,77),(0-1)\r\n");
		response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
		break;

	case AT_CMD_MODE_EXECUTION:
		param = &parse_cmd->raw_data[parse_cmd->parse_pos];
		// get param: gpio
		param = strtok(param, ",");
		gpio = atoi(param);
		// at_log_d("gpio is %d", gpio);

		// get param: direction
		param = strtok(NULL, ",");
		value = atoi(param);
		// at_log_d("value is %d", value);

		ret = sAPI_GpioSetValue(gpio, value);
		at_log_d("gpio,value ==== %d,%d", gpio, value);
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

// 获取指定的GPIO的值--cggetv
at_status_t at_cmd_hdlr_cggetv(at_parse_cmd_param_t *parse_cmd)
{
	at_response_t response = {{0}};
	at_status_t ret;

	unsigned int gpio = 0;
	char *param = NULL;

	switch (parse_cmd->mode)
	{

	case AT_CMD_MODE_TESTING:
		strcpy((char *)response.buf, "+CGGETV: (1,2,3,6,12,14,16,18,22,41,43,63,77)\r\n");
		response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
		break;

	case AT_CMD_MODE_EXECUTION:
		param = &parse_cmd->raw_data[parse_cmd->parse_pos];
		// get param: gpio
		// param = strtok(param, ",");
		gpio = atoi(param);
		// at_log_d("gpio is %d", gpio);

		ret = sAPI_GpioGetValue(gpio);

		if (ret == AT_STATUS_OK)
		{
			sprintf((char *)response.buf, "+CGGETV: %d,0\r\n", gpio);
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

// 读取ADC的值--cadc
at_status_t at_cmd_hdlr_cadc(at_parse_cmd_param_t *parse_cmd)
{
	at_response_t response = {{0}};
	UINT16 ret;

	int channel, adc_value1, adc_mv_value1;
	char *param = NULL;

	switch (parse_cmd->mode)
	{

	case AT_CMD_MODE_TESTING:
		strcpy((char *)response.buf, "+CADC:(0,2)\r\n");
		response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
		break;

	case AT_CMD_MODE_READ:
		sprintf((char *)response.buf, "+CADC:%d\r\n", sAPI_ReadAdc(1));
		response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
		break;

	case AT_CMD_MODE_EXECUTION:
		param = &parse_cmd->raw_data[parse_cmd->parse_pos];
		channel = atoi(param);

		if (channel == 0 || channel == 2)
		{
			ret = 0;
		}
		else
			ret = 1;
		if (ret == AT_STATUS_OK)
		{
			adc_mv_value1 = sAPI_ReadAdc(1);
			if (channel == 0)
			{

				uvr_log_i(" adc_mv_value1=%d", adc_mv_value1);
				adc_value1 = (adc_mv_value1 * 4096) / 1200;
				uvr_log_i("vcc_adc_value1(2x)=%d", adc_value1);
				sprintf((char *)response.buf, "+CADC: %d\r\n", adc_value1); // 十进制数
																																		// response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
			}
			else
			{
				//  adc_mv_value1 = sAPI_ReadAdc(1);
				uvr_log_i("adc_mv_value1=%d", adc_mv_value1);
				sprintf((char *)response.buf, "+CADC: %d\r\n", adc_mv_value1); // 十进制数
			}

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

// cadc2
at_status_t at_cmd_hdlr_cadc2(at_parse_cmd_param_t *parse_cmd)
{
	at_response_t response = {{0}};
	UINT16 ret;

	char *param = NULL;
	int channel, adc_value1, adc_mv_value1;

	switch (parse_cmd->mode)
	{

	case AT_CMD_MODE_TESTING:
		strcpy((char *)response.buf, "+CADC2:(0,2)\r\n");
		response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
		break;

	case AT_CMD_MODE_READ:
		sprintf((char *)response.buf, "+CADC2:%d\r\n", sAPI_ReadAdc(2));
		response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
		break;

	case AT_CMD_MODE_EXECUTION:
		param = &parse_cmd->raw_data[parse_cmd->parse_pos];
		channel = atoi(param);
		// at_log_d("channel=%d", channel);

		if (channel == 0 || channel == 2)
		{
			ret = 0;
		}
		else
			ret = 1;

		if (ret == AT_STATUS_OK)
		{
			adc_mv_value1 = sAPI_ReadAdc(2);
			if (channel == 0)
			{

				uvr_log_i(" adc_mv_value1=%d", adc_mv_value1);
				adc_value1 = (adc_mv_value1 * 4096) / 1200;
				uvr_log_i("vcc_adc_value1(2x)=%d", adc_value1);
				sprintf((char *)response.buf, "+CADC2: %d\r\n", adc_value1); // 十进制数
																																		 // response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
			}
			else
			{
				//  adc_mv_value1 = sAPI_ReadAdc(1);
				uvr_log_i("adc_mv_value1=%d", adc_mv_value1);
				sprintf((char *)response.buf, "+CADC2: %d\r\n", adc_mv_value1); // 十进制数
			}
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

// sn
char sn[20] = "0000000000000000";
at_status_t at_cmd_hdlr_sn(at_parse_cmd_param_t *parse_cmd)
{
	at_response_t response = {{0}};
	UINT16 ret;
	int str_len = 0;
	char *param = NULL;

	switch (parse_cmd->mode)
	{
	case AT_CMD_MODE_TESTING:
		strcpy((char *)response.buf, "+SN:(0000000000000000)\r\n"); // 16位
		response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
		break;
	case AT_CMD_MODE_READ:

		sprintf(sn, "%s", g_uvr_data_t.sn);
		at_log_d("sn=%s", sn);
		sprintf((char *)response.buf, "+SN: %s\r", sn);
		response.flag |= AT_RESPONSE_FLAG_APPEND_OK;

		break;
	case AT_CMD_MODE_EXECUTION:
		// sn = atoi(param);
		// at_log_d("channel=%d", channel);

		param = &parse_cmd->raw_data[parse_cmd->parse_pos];
		str_len = strlen(param);
		at_log_d("str_len=%d", str_len);
		at_log_d("sn=%s", param);

		if (str_len == 18)
		{
			ret = 0;
		}
		else
			ret = 1;

		if (ret == AT_STATUS_OK)
		{
			sprintf(sn, "%s", param);
			uvr_log_i("sn = %s", sn);
			sprintf(g_uvr_data_t.sn, "%s", sn);
			uvr_data_update();
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

// setuart
at_status_t at_cmd_hdlr_setuart(at_parse_cmd_param_t *parse_cmd)
{
	at_response_t response = {{0}};
	UINT16 ret;
	int portnumber, databits, paritybit, stopbits;
	uint32_t baudrate;
	char *param = NULL;

	switch (parse_cmd->mode)
	{
	case AT_CMD_MODE_TESTING:
		strcpy((char *)response.buf, "+SETUART:portnumber(1,6)\r\nbaudrate(300,600,1200,2400,3600,4800,9600,19200,38400,57600,115200,230100,460800,921600,1842000,3684000)\r\ndatabits(5-8)\r\nparitybit(0,1,2)\r\nstopbits(0,1)\r\n"); // 16位
		response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
		break;
	case AT_CMD_MODE_EXECUTION:

		param = &parse_cmd->raw_data[parse_cmd->parse_pos];
		// at_log_d("param=%s", param);

		// get param: portnumber
		param = strtok(param, ",");
		portnumber = atoi(param);
		at_log_d("portnumber is %d", portnumber);

		// get param: baudrate
		param = strtok(NULL, ",");
		baudrate = atoi(param);
		at_log_d("baudrate is %ld", baudrate);

		// get param: databits
		param = strtok(NULL, ",");
		databits = atoi(param);
		at_log_d("databits is %d", databits);

		// get param: paritybit
		param = strtok(NULL, ",");
		paritybit = atoi(param);
		at_log_d("paritybit is %d", paritybit);

		// get param: stopbits
		param = strtok(NULL, ",");
		stopbits = atoi(param);
		at_log_d("stopbits is %d", stopbits);

		ret = uvr_set_usart(portnumber, baudrate, databits, paritybit, stopbits);
		at_log_d("ret is %d", ret);

		if (ret == AT_STATUS_OK)
		{
			sprintf((char *)response.buf, "+setuart: %d,%ld,%d,%d,%d\r\n", portnumber, baudrate, databits, paritybit, stopbits);
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

// simid
char sim_uuid;
at_status_t at_cmd_hdlr_simid(at_parse_cmd_param_t *parse_cmd)
{
	at_response_t response = {{0}};
	// UINT16 ret;
	// int str_len = 0;
	// char *param = NULL;

	switch (parse_cmd->mode)
	{
	case AT_CMD_MODE_TESTING:
		strcpy((char *)response.buf, "+SIMID:(1,2)\r\n");
		response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
		break;
	case AT_CMD_MODE_READ:

		sprintf((char *)response.buf, "+SIMID1: %s\r\n +SIMID2: %s\r\n", &g_uvr_data_t.sim_uuid[0][0], &g_uvr_data_t.sim_uuid[1][0]);

		response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
		uvr_data_update();

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

// 设置电话功能--cfun
at_status_t at_cmd_hdlr_cfun(at_parse_cmd_param_t *parse_cmd)
{
	at_response_t response = {{0}};
	at_status_t ret;
	UINT8 cfun = 0;

	char *param = NULL;

	switch (parse_cmd->mode)
	{
	case AT_CMD_MODE_READ:
		ret = sAPI_NetworkGetCfun(&cfun);
		if (ret == AT_STATUS_OK)
		{
			sprintf((char *)response.buf, "+CFUN: %d\r\n", cfun);
			response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
		}
		else
		{
			response.flag |= AT_RESPONSE_FLAG_APPEND_ERROR;
		}
		break;

	case AT_CMD_MODE_TESTING:
		strcpy((char *)response.buf, "+CFUN: (0-1,4-7),(0-1)\r\n");
		response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
		break;

		// case AT_CMD_MODE_ACTIVE:
		// 	response.flag |= AT_RESPONSE_FLAG_APPEND_OK;

		// 	break;

	case AT_CMD_MODE_EXECUTION:
		param = &parse_cmd->raw_data[parse_cmd->parse_pos];
		int func = atoi(param);
		ret = sAPI_NetworkSetCfun(func);
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

/*
// 定义PDP上下文----CGDCONT
at_status_t at_cmd_hdlr_cgdcont(at_parse_cmd_param_t *parse_cmd)
{
	at_response_t response = {{0}};
	at_status_t ret;
	SCApnParm cgdcont;

	switch (parse_cmd->mode)
	{
	case AT_CMD_MODE_TESTING:
		ret = sAPI_NetworkGetCgdcont(&cgdcont);
		if (ret == AT_STATUS_OK)
		{
			sprintf((char *)response.buf, "+CGDCONT:(0-15),\r\n");
			response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
		}
		else
		{
			response.flag |= AT_RESPONSE_FLAG_APPEND_ERROR;
		}
		break;

	case AT_CMD_MODE_READ:
		response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
		break;

	case AT_CMD_MODE_ACTIVE:
		response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
		break;
	case AT_CMD_MODE_EXECUTION:
		response.flag |= AT_RESPONSE_FLAG_APPEND_ERROR;
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
*/




