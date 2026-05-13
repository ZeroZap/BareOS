#include "uvr.h"

// 重置--creset
at_status_t at_cmd_param_set_all(at_parse_cmd_param_t *parse_cmd)
{
  at_response_t response = {{0}};
	at_status_t ret;
	char *param = NULL;

  switch (parse_cmd->mode)
  {
  case AT_CMD_MODE_TESTING:
    response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
    break;

  case AT_CMD_MODE_EXECUTION:
  	param = &parse_cmd->raw_data[parse_cmd->parse_pos];
    uvr_log_i("%s",param);
    uvr_log_i("data len is %d",strlen(param));
     ////////////////////////////// string_data to json/////////////////////////////////
    cJSON *param_json = cJSON_Parse(param); // 将JSON字符串转换成JSON结构体    
    char *cjson_str = cJSON_Print(param_json);
    if (param_json == NULL) // 判断转换是否成功
    {
      uvr_log_e("user cjson error...\r\n");
    }
    else
    {
      uvr_log_i("%s\n", cjson_str); // 打包成功调用cJSON_Print打印输出
      uvr_cjson_isnotnull(param_json);
    }
    sAPI_TaskSleep(10);
    sAPI_Free(cjson_str);
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