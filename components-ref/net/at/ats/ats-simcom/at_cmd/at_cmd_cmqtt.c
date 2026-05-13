#include "uvr.h"

#include "simcom_mqtts_client.h"
extern void uvr_network_callback(int client, int cause);
// 开启MQTT服务----start
at_status_t at_cmd_hdlr_cmqttstart(at_parse_cmd_param_t *parse_cmd)
{
  at_response_t response = {{0}};
  int ret = 0;
  switch (parse_cmd->mode)
  {
  case AT_CMD_MODE_TESTING:
    response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
    break;

  case AT_CMD_MODE_ACTIVE:
    ret = sAPI_MqttStart(-1);
    at_log_d("ret == %d", ret);
    if (ret == SC_MQTT_RESULT_SUCCESS)
    {
      response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
      sprintf((char *)response.buf, "+CMQTTSTART:%d\r\n", ret);
    }
    else
    {
      uvr_log_e("MQTT start ERR,  ret = %d", ret);
      response.flag |= AT_RESPONSE_FLAG_APPEND_ERROR;
      sprintf((char *)response.buf, "ret = %d\r\n", ret);
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

// 关闭MQTT服务--stop
at_status_t at_cmd_hdlr_cmqttstop(at_parse_cmd_param_t *parse_cmd)
{
  at_response_t response = {{0}};
  int ret = 0;
  switch (parse_cmd->mode)
  {
  case AT_CMD_MODE_TESTING:
    response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
    break;

  case AT_CMD_MODE_ACTIVE:
    ret = sAPI_MqttStop();
    at_log_d("ret == %d", ret);
    if (ret == SC_MQTT_RESULT_SUCCESS)
    {
      response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
      sprintf((char *)response.buf, "+CMQTTSTOP:%d\r\n", ret);
    }
    else
    {
      response.flag |= AT_RESPONSE_FLAG_APPEND_ERROR;
      sprintf((char *)response.buf, "ret = %d\r\n", ret);
      uvr_log_e("MQTT stop ERR, ret = %d", ret);
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

// 获取MQTT客户端--accq
sMsgQRef at_accq_msgq;
// SCmqttReturnCode sAPI_MqttAccq(SCmqttOperationType commad_type, char *string_return, int client_index, char *clientID, int server_type, sMsgQRef msgQ_urc);
at_status_t at_cmd_hdlr_cmqttaccq(at_parse_cmd_param_t *parse_cmd)
{
  at_response_t response = {{0}};
  int ret = 0;
  char *param = NULL;
  int str_len = 0;

  int client_index = 0;
  char *clientID = " ";
  int server_type = 0;

  switch (parse_cmd->mode)
  {
  case AT_CMD_MODE_TESTING:
    strcpy((char *)response.buf, "+CMQTTACCQ: (0-1),(1-128)[,(0-1)]\r\n");
    response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
    break;

  case AT_CMD_MODE_EXECUTION:
    param = &parse_cmd->raw_data[parse_cmd->parse_pos];

    param = strtok(param, ",");
    client_index = atoi(param);
    at_log_d("client_index = %d", client_index);

    param = strtok(NULL, ",");
    str_len = strlen(param);
    // at_log_i("param len is %d, str is %s", str_len, param);
    clientID = malloc(str_len - 1);
    memset(clientID, 0, str_len - 1);
    memcpy(clientID, param + 1, str_len - 2);
    at_log_i("get server clientID%s", clientID);

    param = strtok(NULL, ",");
    server_type = atoi(param);
    at_log_d("server_type = %d", server_type);

    ret = sAPI_MqttAccq(0, NULL, client_index, clientID, server_type, at_accq_msgq);
    if (ret == SC_MQTT_RESULT_SUCCESS)
    {
      response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
    }
    else
    {
      response.flag |= AT_RESPONSE_FLAG_APPEND_ERROR;
      sprintf((char *)response.buf, "+CMQTTACCQ ERR, ret = %d\r\n", ret);
      at_log_d("ret = %d", ret);
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

// 释放客户端--rel
at_status_t at_cmd_hdlr_cmqttrel(at_parse_cmd_param_t *parse_cmd)
{
  at_response_t response = {{0}};
  int ret = 0;
  int client_index;
  char *param = NULL;

  switch (parse_cmd->mode)
  {
  case AT_CMD_MODE_TESTING:
    strcpy((char *)response.buf, "+CMQTTREL: (0-1)\r\n");
    response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
    break;

  case AT_CMD_MODE_READ:
    response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
    break;

  case AT_CMD_MODE_EXECUTION:
    param = &parse_cmd->raw_data[parse_cmd->parse_pos];
    client_index = atoi(param);
    at_log_d("client_index = %d", client_index);

    ret = sAPI_MqttRel(client_index);
    at_log_d("ret = %d", ret);
    if (ret == SC_MQTT_RESULT_SUCCESS)
    {
      response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
    }
    else
    {
      response.flag |= AT_RESPONSE_FLAG_APPEND_ERROR;
      sprintf((char *)response.buf, "+CMQTTREL ERR, ret = %d\r\n", ret);
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

// 连接到MQTT服务器--connect
at_status_t at_cmd_hdlr_cmqttconnect(at_parse_cmd_param_t *parse_cmd)
{
  at_response_t response = {{0}};
  int ret = 0;
  int client_index;
  char *server_addr;
  int keepalive_time;
  int clean_session;
  char *user_name = " ";
  char *pass_word = " ";
  char *param = NULL;
  int str_len = 0;

  switch (parse_cmd->mode)
  {
  case AT_CMD_MODE_TESTING:
    strcpy((char *)response.buf, "+CMQTTCONNECT: (0-1),(9-256),(1-64800),(0-1)\r\n");
    response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
    break;
  case AT_CMD_MODE_EXECUTION:
    param = &parse_cmd->raw_data[parse_cmd->parse_pos];

    param = strtok(param, ",");
    client_index = atoi(param);
    at_log_d("client_index = %d", client_index);

    param = strtok(NULL, ",");
    str_len = strlen(param);
    // at_log_i("param len is %d, str is %s", str_len, param);
    server_addr = malloc(str_len - 1);
    memset(server_addr, 0, str_len - 1);
    memcpy(server_addr, param + 1, str_len - 2);
    at_log_i("get server_addr%s", server_addr);

    param = strtok(NULL, ",");
    keepalive_time = atoi(param);
    at_log_d("keepalive_time = %d", keepalive_time);

    param = strtok(NULL, ",");
    clean_session = atoi(param);
    at_log_d("clean_session = %d", clean_session);

    param = strtok(NULL, ",");
    str_len = strlen(param);
    // at_log_i("param len is %d, str is %s", str_len, param);
    if (param == NULL)
    {
      user_name = NULL;
    }
    else
    {
      user_name = malloc(str_len - 1);
      memset(user_name, 0, str_len - 1);
      memcpy(user_name, param + 1, str_len - 2);
      at_log_i("get user_name %s", user_name);
    }

    param = strtok(NULL, ",");
    str_len = strlen(param);
    // at_log_i("param len is %d, str is %s", str_len, param);
    if (param == NULL)
    {
      pass_word = NULL;
    }
    else
    {
      pass_word = malloc(str_len - 1);
      memset(pass_word, 0, str_len - 1);
      memcpy(pass_word, param + 1, str_len - 2);
      at_log_i("get pass_word %s", pass_word);
    }

    sAPI_MqttConnLostCb(uvr_network_callback);
    ret = sAPI_MqttConnect(0, NULL, client_index, server_addr, keepalive_time, clean_session, user_name, pass_word);
    at_log_d("ret = %d", ret);
    if (ret == SC_MQTT_RESULT_SUCCESS)
    {
      response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
    }
    else
    {
      response.flag |= AT_RESPONSE_FLAG_APPEND_ERROR;
      at_log_e("connect to MQTT is error,ret == %d", ret);
    }
    sAPI_Free(server_addr);
    // free(server_addr);
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

// 输入发布信息的主题--topic
at_status_t at_cmd_hdlr_cmqtttopic(at_parse_cmd_param_t *parse_cmd)
{
  at_response_t response = {{0}};
  INT32 ret;
  char *param = NULL;
  int str_len = 0;

  // int comma_count = 0;

  int client_index = 0;
  char *topic_data;
  int topic_length = 0;

  switch (parse_cmd->mode)
  {

  case AT_CMD_MODE_TESTING:
    strcpy((char *)response.buf, "+CMQTTTOPIC: (0-1)[,(1-1024)],(1-1024)\r\n");
    response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
    break;
  case AT_CMD_MODE_EXECUTION:
    param = &parse_cmd->raw_data[parse_cmd->parse_pos];

    param = strtok(param, ",");
    client_index = atoi(param);
    at_log_d("client_index = %d", client_index);

    param = strtok(NULL, ",");
    str_len = strlen(param);
    topic_data = malloc(str_len - 1);
    memset(topic_data, 0, str_len - 1);
    memcpy(topic_data, param + 1, str_len - 2);
    at_log_i("get topic_data == %s", topic_data);

    param = strtok(NULL, ",");
    topic_length = atoi(param);
    at_log_d("topic_length = %d", topic_length);

    // topic_length = strlen(topic_data);
    ret = sAPI_MqttTopic(client_index, (char *)topic_data, topic_length);
    at_log_d("ret = %d", ret);
    if (ret == SC_MQTT_RESULT_SUCCESS)
    {
      sprintf((char *)response.buf, "+CMQTTTOPIC: %d,%s,%d\r\n", client_index, topic_data, topic_length);
      response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
    }
    else
    {
      sprintf((char *)response.buf, "ret = %ld\r\n", ret);
      response.flag |= AT_RESPONSE_FLAG_APPEND_ERROR;
      at_log_e("+CMQTTTOPIC ERR,ret == %d", ret);
    }
    sAPI_Free(topic_data);
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

// 输入发布信息--payload
at_status_t at_cmd_hdlr_cmqttpayload(at_parse_cmd_param_t *parse_cmd)
{
  at_response_t response = {{0}};
  INT32 ret;
  char *param = NULL;
  int str_len = 0;

  int client_index = 0;
  char *payload_data;
  int payload_length = 0;

  switch (parse_cmd->mode)
  {
  case AT_CMD_MODE_TESTING:
    strcpy((char *)response.buf, "+CMQTTPAYLOAD: (0-1)[,1-10240],(1-10240)");
    response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
    break;

  case AT_CMD_MODE_EXECUTION:
    param = &parse_cmd->raw_data[parse_cmd->parse_pos];

    param = strtok(param, ",");
    client_index = atoi(param);
    at_log_d("client_index = %d", client_index);

    param = strtok(NULL, ",");
    str_len = strlen(param);
    // at_log_i("param len is %d, str is %s", str_len, param);
    payload_data = malloc(str_len - 1);
    memset(payload_data, 0, str_len - 1);
    memcpy(payload_data, param + 1, str_len - 2);
    at_log_i("get payload_data%s", payload_data);

    param = strtok(NULL, ",");
    payload_length = atoi(param);
    at_log_d("payload_length = %d", payload_length);

    // ret = sAPI_MqttPayload(0, payload_data, payload_length);
    // ret = sAPI_MqttPayload(0, "aMsgsend123", strlen("aMsgsend123"));
    ret = sAPI_MqttPayload(client_index, (char *)payload_data, payload_length);

    if (ret == SC_MQTT_RESULT_SUCCESS)
    {
      sprintf((char *)response.buf, "+CMQTTPAYLOAD: %d,%s,%d\r\n", client_index, payload_data, payload_length);
      response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
    }
    else
    {
      response.flag |= AT_RESPONSE_FLAG_APPEND_ERROR;
      at_log_e("input is error,ret == %d", ret);
      sprintf((char *)response.buf, "+CMQTTPAYLOAD ERR,ret = %ld\r\n", ret);
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

// 将消息发布到服务器--pub
at_status_t at_cmd_hdlr_cmqttpub(at_parse_cmd_param_t *parse_cmd)
{
  at_response_t response = {{0}};
  int ret = 0;
  char *param = NULL;

  int client_index = 0;
  int qos = 0;
  int pub_timeout = 0;
  int ratained = 0;
  int dup = 0;

  switch (parse_cmd->mode)
  {
  case AT_CMD_MODE_TESTING:
    strcpy((char *)response.buf, "+CMQTTPUB: (0-1),(0-2),(60-180),(0-1),(0-1)\r\n");
    response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
    break;

  case AT_CMD_MODE_EXECUTION:
    param = &parse_cmd->raw_data[parse_cmd->parse_pos];

    param = strtok(param, ",");
    client_index = atoi(param);
    at_log_d("client_index = %d", client_index);

    param = strtok(NULL, ",");
    qos = atoi(param);
    at_log_d("qos = %d", qos);

    param = strtok(NULL, ",");
    pub_timeout = atoi(param);
    at_log_d("qos = %d", pub_timeout);

    ret = sAPI_MqttPub(client_index, qos, pub_timeout, ratained, dup);

    if (ret == SC_MQTT_RESULT_SUCCESS)
    {
      sprintf((char *)response.buf, "+CMQTTPUB: %d,0\r\n", client_index);
      response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
    }
    else
    {
      sprintf((char *)response.buf, "+CMQTTPUB ERR, ret= %d\r\n", ret);
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

// 输入订阅消息的主题--subtopic
at_status_t at_cmd_hdlr_cmqttsubtopic(at_parse_cmd_param_t *parse_cmd)
{
  at_response_t response = {{0}};
  int ret = 0;
  char *param = NULL;
  int str_len = 0;

  int client_index = 0;
  char *sub_topic_data;
  int sub_topic_length = 0;
  int qos = 0;

  switch (parse_cmd->mode)
  {
  case AT_CMD_MODE_TESTING:
    strcpy((char *)response.buf, "+CMQTTSUBTOPIC: (0-1),(1-1024),(0-2)\r\n");
    response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
    break;

  case AT_CMD_MODE_EXECUTION:
    param = &parse_cmd->raw_data[parse_cmd->parse_pos];

    param = strtok(param, ",");
    client_index = atoi(param);
    at_log_d("client_index = %d", client_index);

    param = strtok(NULL, ",");
    str_len = strlen(param);
    // at_log_i("param len is %d, str is %s", str_len, param);
    sub_topic_data = malloc(str_len - 1);
    memset(sub_topic_data, 0, str_len - 1);
    memcpy(sub_topic_data, param + 1, str_len - 2);
    at_log_i("get sub_topic_data %s", sub_topic_data);

    param = strtok(NULL, ",");
    sub_topic_length = atoi(param);
    at_log_d("sub_topic_length = %d", sub_topic_length);

    param = strtok(NULL, ",");
    qos = atoi(param);
    at_log_d("qos = %d", qos);

    ret = sAPI_MqttSubTopic(client_index, sub_topic_data, sub_topic_length, qos);

    if (ret == SC_MQTT_RESULT_SUCCESS)
    {
      sprintf((char *)response.buf, "+CMQTTSUBTOPIC: %d,%s,%d,%d\r\n", client_index, sub_topic_data, sub_topic_length, qos);
      response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
    }
    else
    {
      sprintf((char *)response.buf, "+CMQTTSUBTOPIC ERR, ret= %d\r\n", ret);
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

// 向服务器订阅一条消息--sub
at_status_t at_cmd_hdlr_cmqttsub(at_parse_cmd_param_t *parse_cmd)
{
  at_response_t response = {{0}};
  int ret = 0;
  char *param = NULL;
  int str_len = 0;

  int client_index = 0;
  char *topic_data;
  int topic_length = 0;
  int qos = 0;
  // int dup;

  switch (parse_cmd->mode)
  {
  case AT_CMD_MODE_TESTING:
    strcpy((char *)response.buf, "+CMQTTSUB: (0-1),(1-1024),(0-2),(0-1)\r\n");
    response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
    break;

  case AT_CMD_MODE_EXECUTION:
    param = &parse_cmd->raw_data[parse_cmd->parse_pos];

    param = strtok(param, ",");
    client_index = atoi(param);
    at_log_d("client_index = %d", client_index);

    param = strtok(NULL, ",");
    str_len = strlen(param);
    // at_log_i("param len is %d, str is %s", str_len, param);
    topic_data = malloc(str_len - 1);
    memset(topic_data, 0, str_len - 1);
    memcpy(topic_data, param + 1, str_len - 2);
    at_log_i("get topic_data%s", topic_data);

    param = strtok(NULL, ",");
    topic_length = atoi(param);
    at_log_d("sub_topic_length = %d", topic_length);

    param = strtok(NULL, ",");
    qos = atoi(param);
    at_log_d("qos = %d", qos);

    ret = sAPI_MqttSubTopic(client_index, topic_data, topic_length, qos);

    if (ret == SC_MQTT_RESULT_SUCCESS)
    {
      sprintf((char *)response.buf, "+CMQTTSUB: %d,%s,%d,%d\r\n", client_index, topic_data, topic_length, qos);
      response.flag |= AT_RESPONSE_FLAG_APPEND_OK;
    }
    else
    {
      sprintf((char *)response.buf, "+CMQTTSUB ERR, ret= %d\r\n", ret);
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

void at_mqtt_init(void)
{
  ///////////////消息队列////////////////////
  if (SC_SUCCESS != sAPI_MsgQCreate(&at_accq_msgq, "at_accq_msgq", sizeof(SIM_MSG_T), 4, SC_FIFO))
  {
    uvr_log_e("at accq msgq create err");
  }
  else
  {
    uvr_log_i("at accq magq create success");
  }
}
