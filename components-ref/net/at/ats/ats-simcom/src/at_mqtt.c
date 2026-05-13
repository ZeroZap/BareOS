// #include "uvr.h"

// #define TEST_JSON "C:/atuopoll.json"
// #define CMD_SIZE 40

// sMsgQRef uvr_init_sub_rec_msgq; // 初始化mqtt sub监听
// sMsgQRef uvr_another_sub_rec_msgq;
// sMsgQRef uvr_uart_msgq;
// sMsgQRef uvr_mqtt_disconn_msgq;
// sMsgQRef uvr_mqtt_conn_msgq;
// sMsgQRef uvr_autopoll_stop_msgq;
// sMsgQRef uvr_sysreset_msgq;

// extern int mqtt_connect_flag;
// extern uint16_t CRC16(uint8_t *ptr, uint8_t len);
// extern uint8_t device_addr;

// char publish_topic[1024] = {" "};
// int publish_qos;
// int another_connect_flag = 0;
// int autopoll_flag = 0;
// int uart_data_flag = 0;
// int autopoll_layout = 0;
// int autopoll_overtime = 100;
// int autopoll_time = 10000;
// char reason[80] = "success";

// typedef struct
// {
//   int protocol;
//   int bind_port;
//   int heart;
//   char server_address[30];
//   int server_port;
//   char client_id[40];
//   char client_username[20];
//   char client_passwd[20];
//   char protocol_version[10];
//   int clean_session; //(1 持久会话,2离线自动销毁)
//   int keep_alive;
//   int pub_qos;
//   int sub_qos;
//   char pub_topic[40];
//   char sub_topic[40];
//   int will_flag;
//   int will_qos;
//   char will_topic[40];
//   char topic_content[50];
//   char reg_msg_type[30];
// } net_chan_t;

// char imei_get[20] = {0};
// unsigned int getimei(void)
// {
//   int ret = 0;
//   char imei_value[20] = {0};
//   ret = sAPI_SysGetImei(imei_value);
//   // uvr_log_i("imei_value : %s", imei_value);

//   memset(imei_get, 0, sizeof(imei_get));
//   memcpy(imei_get, imei_value, strlen(imei_value));
//   uvr_log_i("======imei_get: %s=====", imei_get);

//   return ret;
// }

// uint16_t hex_len = 0;
// char modbus_cmd[40] = {0};
// void string_to_hex(char *string)
// {
//   ////////////////////////string to hex////////////////////////////////////////
//   char *token;
//   long int val;
//   token = strtok(string, " ");
//   while (token != NULL)
//   {
//     val = strtol(token, NULL, 16);
//     modbus_cmd[hex_len] = val;
//     token = strtok(NULL, " ");
//     hex_len++;
//   }
// }

// /*
//  *1-->success  2-->fail
//  */
// // void return_json(int i, char *fail_reason)
// // {
// //     char success_json[200] = " ";
// //     char fail_json[1024] = " ";
// //     int ret;
// //     if (1 == i)
// //     {
// //         sprintf(success_json, "{\"code\":\"200\",\"msg\":\"success\",\"devid\":\"%s\",\"com\":\"10001\"}", imei_get);
// //         sAPI_MqttTopic(0, "rqiot/dtuissued/recinfo", 23);
// //         sAPI_MqttPayload(0, success_json, strlen(success_json));
// //     }
// //     else if (2 == i)
// //     {
// //         sprintf(fail_json, "{\"code\":\"-99\",\"msg\":\"%s\",\"devid\":\"%s\",\"com\":\"10001\"}", fail_reason, imei_get);
// //         sAPI_MqttTopic(0, "rqiot/dtuissued/recinfo", 23);
// //         sAPI_MqttPayload(0, fail_json, strlen(fail_json));
// //     }
// //     ret = sAPI_MqttPub(0, 0, 60, 0, 0);
// //     if (ret == SC_MQTT_RESULT_SUCCESS)
// //     {
// //         uvr_log_i("MQTT return json publish success,msg is %s!", fail_reason);
// //     }
// //     else
// //     {
// //         uvr_log_e("MQTT return json publish failure! ret = %d", ret);
// //     }
// // }

// // void uvr_network_callback(int index, int cause)
// // {
// //     if (mqtt_connect_flag == 0)
// //     {
// //         SC_STATUS status = SC_SUCCESS;
// //         SIM_MSG_T send_data = {SIM_MSG_INIT, 0, -1, NULL};
// //         unsigned char card_index;
// //         sAPI_SysGetBindSim(&card_index);
// //         if (card_index)
// //             uvr_ui_update(SIM1_ONLINE, NULL);
// //         else
// //             uvr_ui_update(SIM0_ONLINE, NULL);
// //         sAPI_TaskSleep(5 * 200);
// //         uvr_log_e("mqtt offline,index:%d,cause:%d", index, cause);
// //         if (0 == index)
// //         {
// //             uvr_log_e("mqtt network error! uvr mqtt network callback! wait for 5S and then start to send disconnect msg!");
// //             // send_data.arg1 = index;
// //             status = sAPI_MsgQSend(uvr_mqtt_disconn_msgq, &send_data);
// //             if (status != SC_SUCCESS)
// //             {
// //                 uvr_log_e("send uvr mqtt disconnect msg error!");
// //             }
// //         }
// //         else if (1 == index)
// //         {
// //             uvr_log_e("another mqtt server connect disconnected!");
// //             another_connect_flag = 0;
// //             sprintf(reason, "another mqtt server connect disconnected");
// //             return_json(2, reason);
// //             memset(reason, 0, sizeof(reason));
// //         }
// //     }
// // }

// // void uvr_mqtt_init(void)
// // {
// //     int ret;
// //     int mqtt_status = 0;
// //     int greg = 0;
// //     char uvr_sub_topic[100];
// //     uvr_log_i("start to mqtt init! please wait for mqtt network ready!");
// //     //////////////////check network/////////////////////////////
// //     while (1)
// //     {
// //         sAPI_NetworkGetCgreg(&greg);
// //         // network ->> OK
// //         if (1 == greg || 5 == greg)
// //         {
// //             uvr_log_i("MQTT NETWORK STATUS IS NORMAL");
// //             break;
// //         }
// //         // network ->> ERROE,sleep 10s
// //         else
// //         {
// //             uvr_log_e("MQTT NETWORK STATUS IS [%d]", greg);
// //             sAPI_TaskSleep(10 * 200);
// //         }
// //     }
// //     while (1)
// //     {
// //         //////////////////////////start mqtt connect/////////////////////////////
// //         ret = sAPI_MqttStart(-1);
// //         if (ret == SC_MQTT_RESULT_SUCCESS)
// //         {
// //             uvr_log_i("MQTT start success!");
// //         }
// //         else
// //         {
// //             uvr_log_e("MQTT start failure! ret = %d", ret);
// //         }
// //         ret = sAPI_MqttAccq(0, NULL, 0, imei_get, 0, uvr_init_sub_rec_msgq);
// //         if (ret == SC_MQTT_RESULT_SUCCESS)
// //         {
// //             uvr_log_i("MQTT Accq success!");
// //         }
// //         else
// //         {
// //             uvr_log_e("MQTT Accq failure! ret = %d", ret);
// //         }

// //         sAPI_MqttConnLostCb(uvr_network_callback);subscribe
// //         ret = sAPI_MqttConnect(0, NULL, 0, "tcp://testmq.iotcmp.net:1883", 60, 1, "test", "test123456");
// //         if (ret == SC_MQTT_RESULT_SUCCESS)
// //         {
// //             uvr_log_i("MQTT connect success!");
// //         }
// //         else
// //         {
// //             uvr_log_e("MQTT connect failure! ret = %d", ret);
// //         }
// //         ////////////////////////subscribe////////////////////////////////////
// //         sprintf(uvr_sub_topic, "rqiot/dtuissued/%s", imei_get);
// //         ret = sAPI_MqttSubTopic(0, uvr_sub_topic, strlen(uvr_sub_topic), 0);
// //         ret = sAPI_MqttSub(0, NULL, 0, 0, 0);
// //         if (ret == SC_MQTT_RESULT_SUCCESS)
// //         {
// //             uvr_log_i("MQTT topic subscribe success!");
// //             uvr_log_i("MQTT connect to server is OK!");
// //             //////////////////////read sysreset json///////////////////
// //             SCFILE *file_root_json = sAPI_fopen("C:/root_json", "rb");
// //             if (NULL == file_root_json)
// //             {
// //                 uvr_log_e("json file \"C:/root_json\" can't find");
// //             }
// //             char *buf = UVR_MALLOC(1024);
// //             if (NULL == buf)
// //             {
// //                 uvr_log_e("UVR_MALLOC buf failed");
// //             }
// //             memset(buf, 0, 1024);
// //             unsigned long json_file_len = sAPI_fread(buf, 1, 2048, file_root_json);
// //             sAPI_fclose(file_root_json);
// //             if (json_file_len)
// //             {
// //                 cJSON *sys_reset_json = cJSON_Parse(buf);
// //                 sAPI_Free(buf);
// //                 if (sys_reset_json != NULL)
// //                 {
// //                     uvr_log_i("sys_reset_json is not NULL! wait for 3S and start to param reset!");
// //                     sAPI_TaskSleep(3 * 200);
// //                     uvr_cjson_isnotnull(sys_reset_json);
// //                 }
// //                 cJSON_Delete(sys_reset_json);
// //             }
// //             break;
// //         }
// //         else
// //         {
// //             uvr_log_e("MQTT topic subscribe failure! ret = %d", ret);
// //         }
// //         memset(uvr_sub_topic, 0, sizeof(uvr_sub_topic));
// //         mqtt_status++;
// //         if (mqtt_status != 1)
// //         {
// //             // ret = sAPI_MqttDisConnect(0, NULL, 0, 60);
// //             // if (ret != SC_MQTT_RESULT_SUCCESS)
// //             // {
// //             //     uvr_log_e("mqtt disconnect failure! ret = %d", ret);
// //             // }
// //             ret = sAPI_MqttRel(0);
// //             if (ret != SC_MQTT_RESULT_SUCCESS)
// //             {
// //                 uvr_log_e("mqtt rel failure! ret = %d", ret);
// //             }
// //             uvr_log_e("mqtt init fail,please wait for 3S");
// //             sAPI_TaskSleep(3 * 200);
// //         }
// //     }
// // }

// // void uvr_mqtt_init_conn_task(void *ptr)
// // {
// //     SIM_MSG_T uvr_mqtt_disconn = {SIM_MSG_INIT, 0, -1, NULL};
// //     SCmqttResultType ret;
// //     // int index = 0;
// //     ///////////start to connect to mqtt//////////////////////
// //     uvr_mqtt_init();
// //     while (1)
// //     {
// //         ////////////wait for disconnect message///////////////

// //         sAPI_MsgQRecv(uvr_mqtt_disconn_msgq, &uvr_mqtt_disconn, SC_SUSPEND);
// //         uvr_log_d("uvr mqtt disconect msgq received a message, please wait for reconnect to mqtt!");
// //         ret = sAPI_MqttRel(0);
// //         if (ret != SC_MQTT_RESULT_SUCCESS)
// //         {
// //             uvr_log_e("mqtt rel failure! ret = %d", ret);
// //         }
// //         uvr_mqtt_init();
// //         sAPI_Free(uvr_mqtt_disconn.arg3);
// //     }
// // }

// void uvr_cjson_uartparam(cJSON *uartparam)
// {
//   ////////////////////////get set_uart_data//////////////////////////////////////
//   cJSON *cjsonstatus = cJSON_GetArrayItem(uartparam, 0);
//   int status = cjsonstatus->valueint;
//   int ret;
//   if (status == 1)
//   {
//     cJSON *cjsonbaudrate = cJSON_GetArrayItem(uartparam, 1);
//     int baudrate = cjsonbaudrate->valueint;

//     cJSON *cjsondatabits = cJSON_GetArrayItem(uartparam, 2);
//     int databits = cjsondatabits->valueint;

//     cJSON *cjsonparitybits = cJSON_GetArrayItem(uartparam, 3);
//     int paritybits = cjsonparitybits->valueint;

//     cJSON *cjsonstopbits = cJSON_GetArrayItem(uartparam, 4);
//     int stopbits = cjsonstopbits->valueint;

//     ///////////////change to right data///////////////////////
//     if (paritybits == 1)
//     {
//       paritybits = 2;
//     }
//     else if (paritybits == 2)
//     {
//       paritybits = 1;
//     }

//     if (stopbits == 1)
//     {
//       stopbits = 0;
//     }
//     else if (stopbits == 2)
//     {
//       stopbits = 1;
//     }
//     else if (stopbits == 0)
//     {
//       stopbits = 3; // 防止输入为0还是执行停止位1
//     }
//     ////////////////////////set uart//////////////////////////////////
//     ret = uvr_set_usart(4, baudrate, databits, paritybits, stopbits);
//     // if (SC_FAIL == ret)
//     // {
//     // sprintf(reason, "uart set fail");
//     // sAPI_TaskSleep(30);
//     // return_json(2, reason);
//     // memset(reason, 0, sizeof(reason));
//     // }
//   }
// }

// void uvr_cjson_netchan(cJSON *netchan)
// {
//   net_chan_t *chan_t = NULL;
//   chan_t = UVR_MALLOC(sizeof(net_chan_t));
//   SC_STATUS status = SC_SUCCESS;
//   SIM_MSG_T send_data = {SIM_MSG_INIT, 0, -1, NULL};
//   uint8_t parm_err_flag = 0;
//   cJSON *netchanarr = cJSON_GetArrayItem(netchan, 0);
//   int netarr_size = cJSON_GetArraySize(netchanarr);
//   if (netarr_size != 0)
//   {
//     cJSON *cjsona = cJSON_GetArrayItem(netchanarr, 0);
//     int a = cjsona->valueint;
//     chan_t->protocol = a;

//     cJSON *cjsonb = cJSON_GetArrayItem(netchanarr, 1);
//     int b = cjsonb->valueint;
//     chan_t->bind_port = b;

//     cJSON *cjsonc = cJSON_GetArrayItem(netchanarr, 2);
//     int c = cjsonc->valueint;
//     chan_t->heart = c;

//     cJSON *cjsond = cJSON_GetArrayItem(netchanarr, 3);
//     char *d = cjsond->valuestring;
//     strcpy(chan_t->server_address, d);
//     if ((chan_t->server_address == NULL) || (strcmp(chan_t->server_address, "") == 0))
//     {
//       parm_err_flag = 1;
//     }

//     cJSON *cjsone = cJSON_GetArrayItem(netchanarr, 4);
//     int e = cjsone->valueint;
//     chan_t->server_port = e;

//     cJSON *cjsonf = cJSON_GetArrayItem(netchanarr, 5);
//     char *f = cjsonf->valuestring;
//     strcpy(chan_t->client_id, f);
//     if ((chan_t->client_id == NULL) || (strcmp(chan_t->client_id, "") == 0))
//     {
//       parm_err_flag = 1;
//     }

//     cJSON *cjsong = cJSON_GetArrayItem(netchanarr, 6);
//     char *g = cjsong->valuestring;
//     strcpy(chan_t->client_username, g);
//     if ((chan_t->client_username == NULL) || (strcmp(chan_t->client_username, "") == 0))
//     {
//       parm_err_flag = 1;
//     }

//     cJSON *cjsonh = cJSON_GetArrayItem(netchanarr, 7);
//     char *h = cjsonh->valuestring;
//     strcpy(chan_t->client_passwd, h);
//     if ((chan_t->client_passwd == NULL) || (strcmp(chan_t->client_passwd, "") == 0))
//     {
//       parm_err_flag = 1;
//     }

//     cJSON *cjsoni = cJSON_GetArrayItem(netchanarr, 8);
//     char *i = cjsoni->valuestring;
//     strcpy(chan_t->protocol_version, i);
//     if ((chan_t->protocol_version == NULL) || (strcmp(chan_t->protocol_version, "") == 0))
//     {
//       parm_err_flag = 1;
//     }

//     cJSON *cjsonj = cJSON_GetArrayItem(netchanarr, 9);
//     int j = cjsonj->valueint;
//     chan_t->clean_session = j;
//     if (j == 2)
//     {
//       j = 0;
//     }

//     cJSON *cjsonk = cJSON_GetArrayItem(netchanarr, 10);
//     int k = cjsonk->valueint;
//     chan_t->keep_alive = k;

//     cJSON *cjsonl = cJSON_GetArrayItem(netchanarr, 11);
//     int l = cjsonl->valueint;
//     chan_t->sub_qos = l;

//     cJSON *cjsonm = cJSON_GetArrayItem(netchanarr, 12);
//     int m = cjsonm->valueint;
//     chan_t->pub_qos = m;
//     publish_qos = m;

//     cJSON *cjsonn = cJSON_GetArrayItem(netchanarr, 13);
//     char *n = cjsonn->valuestring;
//     strcpy(chan_t->sub_topic, n);
//     if ((chan_t->sub_topic == NULL) || (strcmp(chan_t->sub_topic, "") == 0))
//     {
//       parm_err_flag = 1;
//     }

//     cJSON *cjsono = cJSON_GetArrayItem(netchanarr, 14);
//     char *o = cjsono->valuestring;
//     strcpy(chan_t->pub_topic, o);
//     memcpy(publish_topic, o, strlen(o));
//     if ((chan_t->pub_topic == NULL) || (strcmp(chan_t->pub_topic, "") == 0))
//     {
//       parm_err_flag = 1;
//     }

//     cJSON *cjsonp = cJSON_GetArrayItem(netchanarr, 15);
//     int p = cjsonp->valueint;
//     chan_t->will_flag = p;

//     if (p == 1)
//     {
//       cJSON *cjsonq = cJSON_GetArrayItem(netchanarr, 16);
//       int q = cjsonq->valueint;
//       chan_t->will_qos = q;
//       uvr_log_i("%d\n", chan_t->will_qos);

//       cJSON *cjsonr = cJSON_GetArrayItem(netchanarr, 17);
//       char *r = cjsonr->valuestring;
//       strcpy(chan_t->will_topic, r);
//       if ((chan_t->will_topic == NULL) || (strcmp(chan_t->will_topic, "") == 0))
//       {
//         parm_err_flag = 1;
//       }

//       cJSON *cjsons = cJSON_GetArrayItem(netchanarr, 18);
//       char *s = cjsons->valuestring;
//       strcpy(chan_t->topic_content, s);
//       if ((chan_t->topic_content == NULL) || (strcmp(chan_t->topic_content, "") == 0))
//       {
//         parm_err_flag = 1;
//       }

//       cJSON *cjsont = cJSON_GetArrayItem(netchanarr, 19);
//       char *t = cjsont->valuestring;
//       strcpy(chan_t->reg_msg_type, t);
//       if ((chan_t->reg_msg_type == NULL) || (strcmp(chan_t->reg_msg_type, "") == 0))
//       {
//         parm_err_flag = 1;
//       }
//     }
//     if (parm_err_flag != 1)
//     {
//       send_data.arg3 = (void *)chan_t;
//       status = sAPI_MsgQSend(uvr_mqtt_conn_msgq, &send_data);
//       if (status != SC_SUCCESS)
//       {
//         sAPI_Free(send_data.arg3);
//         uvr_log_e("send uvr another mqtt connect msg error,status = [%d]", status);
//       }
//     }
//     else if (parm_err_flag == 1)
//     {
//       uvr_log_e("netchan param is NULL");
//       // sprintf(reason, "netchan param is NULL");
//       // sAPI_TaskSleep(30);
//       // return_json(2, reason);
//       // memset(reason, 0, sizeof(reason));
//       parm_err_flag = 0;
//     }
//   }
//   else
//   {
//     uvr_log_e("autoarr is NULL");
//   }
// }

// void uvr_send_sensor_publish(void)
// {
//   ////////////////////////等待设备回传消息并进行CRC校验////////////////////////////////////////////////
//   uint8_t Crc16_check1;
//   uint8_t Crc16_check2;
//   char uvr_uart4_Data[50]; // 接收到的数据
//   int uvr_uart4Datalen;
//   uint16_t Crc16_check = 0;
//   SC_STATUS status;
//   SIM_MSG_T option_msg = {0, 0, 0, NULL};
//   int ret;
//   char send_data[50];
//   char *send_to_server_data;
//   send_to_server_data = UVR_MALLOC(50);

//   status = sAPI_MsgQRecv(uvr_uart_msgq, &option_msg, autopoll_overtime / 5);
//   if (SC_TIMEOUT == status)
//   {
//     // sprintf(reason, "sensor timeout");
//     // sAPI_TaskSleep(10);
//     // return_json(2, reason);
//     // memset(reason, 0, sizeof(reason));
//     sAPI_TaskSleep(30);
//     uart_data_flag = 1;
//   }
//   if (SC_SUCCESS == status)
//   {
//     if (SRV_UART != option_msg.msg_id)
//     {
//       uvr_log_e("uart msg_id is error!!,option_msg.msg_id=%d", option_msg.msg_id);
//       uart_data_flag = 1;
//     }
//     else if (SRV_UART == option_msg.msg_id)
//     {
//       uart_data_flag = 0;
//       memcpy(uvr_uart4_Data, option_msg.arg3, option_msg.arg2);
//       uvr_uart4Datalen = option_msg.arg2;
//       sAPI_Free(option_msg.arg3);

//       Crc16_check = CRC16((uint8_t *)uvr_uart4_Data, uvr_uart4Datalen - 2);
//       Crc16_check1 = Crc16_check & 0xFF;
//       Crc16_check2 = (Crc16_check >> 8) & 0xFF;

//       if ((Crc16_check1 != uvr_uart4_Data[uvr_uart4Datalen - 2]) | (Crc16_check2 != uvr_uart4_Data[uvr_uart4Datalen - 1]))
//       {
//         uvr_log_e("sensor data CRC check error!");
//         // sprintf(reason, "sensor data CRC check error");
//         // sAPI_TaskSleep(30);
//         // return_json(2, reason);
//         // memset(reason, 0, sizeof(reason));
//       }
//       else
//       {
//         uvr_log_d("sensor data CRC check success!");
//         ////////////////////////////校验成功往上发/////////////////////////////
//         for (int j = 0; j < uvr_uart4Datalen; j++)
//         {
//           sprintf(send_to_server_data + j * 3, " %02X", uvr_uart4_Data[j]);
//         }
//         sprintf(send_data, "{\"data\":\"%s\"}", send_to_server_data);
//         uvr_log_d("send_to_server is %s", send_to_server_data);
//         sAPI_Free(send_to_server_data);
//         ////////////////////////////////////////////设备数据上报给字符串////////////////////////////////////
//         ret = sAPI_MqttTopic(1, publish_topic, strlen(publish_topic));

//         ret = sAPI_MqttPayload(1, send_data, strlen(send_data));

//         ret = sAPI_MqttPub(1, publish_qos, 60, 0, 0);
//         if (ret == SC_MQTT_RESULT_SUCCESS)
//         {
//           uvr_log_i("publish topic is %s", publish_topic);
//           uvr_log_i("MQTT sensor data Publish success!");
//         }
//         else
//         {
//           uvr_log_e("MQTT sensor data publish failure! ret = %d", ret);
//         }
//         memset(uvr_uart4_Data, 0, uvr_uart4Datalen);
//       }
//     }
//   }
// }

// void uvr_cjson_autopoll(cJSON *uvr_autopoll)
// {
//   cJSON *autopoll = cJSON_GetArrayItem(uvr_autopoll, 0);
//   int cjson_size = cJSON_GetArraySize(autopoll);

//   cJSON *auto_json = cJSON_CreateArray();

//   SCFILE *fp = sAPI_fopen(TEST_JSON, "wb");
//   if (NULL == fp)
//   {
//     uvr_log_e("fp(sAPI_fopen TEST_JSON) is NULL!");
//   }
//   if (cjson_size != 0)
//   {
//     if (cjson_size > 4 && cjson_size < 44)
//     {
//       autopoll_flag = 1;
//       cJSON *cjson_autopoll_len = cJSON_GetArrayItem(autopoll, 0);
//       int autopoll_status = cjson_autopoll_len->valueint;
//       if (1 == autopoll_status)
//       {
//         cJSON *cjson_autopoll_overtime = cJSON_GetArrayItem(autopoll, 1);
//         autopoll_overtime = cjson_autopoll_overtime->valueint;

//         cJSON *cjson_autopoll_time = cJSON_GetArrayItem(autopoll, 2);
//         autopoll_time = cjson_autopoll_time->valueint;

//         cJSON *cjson_autopoll_layout = cJSON_GetArrayItem(autopoll, 3);
//         autopoll_layout = cjson_autopoll_layout->valueint;

//         if (autopoll_layout == 2)
//         {
//           for (int l = 0; l < cjson_size - 4; l++)
//           {
//             cJSON *cjson_cmd = cJSON_GetArrayItem(autopoll, l + 4);
//             char *cmd = cjson_cmd->valuestring;
//             uvr_log_d("receive cmd[%d] is %s", l, cmd);
//             cJSON_AddStringToObject(auto_json, "cmd", cmd);
//           }
//           // 将json结构格式化到缓冲区
//           char *buf = cJSON_Print(auto_json);
//           uvr_log_d("write to the file buf is %s\n", buf);

//           // 打开文件写入json内容
//           sAPI_fwrite(buf, 1, strlen(buf), fp);
//           uvr_log_i("File write autopoll json success");
//           sAPI_Free(buf);
//           sAPI_fclose(fp);
//           // 释放json结构所占用的内存
//           sAPI_TaskSleep(10);
//           cJSON_Delete(auto_json);
//         }
//         else
//         {
//           uvr_log_e("autopoll_layout is not HEX");
//           // sprintf(reason, "autopoll type is not HEX");
//           // return_json(2, reason);
//           // memset(reason, 0, sizeof(reason));
//         }
//       }
//     }
//     else
//     {
//       autopoll_flag = 0;
//       uvr_log_e("autopoll cmd is null");
//     }
//   }
//   else
//   {
//     autopoll_flag = 0;
//     uvr_log_e("autopoll cmd is null");
//   }
// }

// void uvr_cjson_isnotnull(cJSON *root)
// {
//   SC_STATUS status = SC_SUCCESS;
//   cJSON *uartparam = cJSON_GetObjectItem(root, "uartparam");
//   if (uartparam != NULL)
//   {
//     uvr_cjson_uartparam(uartparam); // set if not empty
//   }

//   ///////////////////////////////////////////////////////////
//   // cJSON *networktime_json = cJSON_GetObjectItem(root, "networktime");
//   // int networktime = networktime_json->valueint;
//   // if (networktime != NULL)
//   // {
//   //     uvr_log_i("networktime");
//   // }
//   // cJSON *log_json = cJSON_GetObjectItem(root, "log");
//   // char *log = log_json->valueint;
//   // if (log != NULL)
//   // {
//   //     uvr_log_i("log");
//   // }
//   // cJSON *ota_json = cJSON_GetObjectItem(root, "ota");
//   // int ota = ota_json->valueint;
//   // if (ota != NULL)
//   // {
//   //     uvr_log_i("ota");
//   // }
//   // cJSON *nrestarttime_json = cJSON_GetObjectItem(root, "nrestarttime");
//   // char *nrestarttime = nrestarttime_json->valuestring;
//   // if (nrestarttime != NULL)
//   // {
//   //     uvr_log_i("nrestarttime");
//   // }
//   cJSON *reboottime_json = cJSON_GetObjectItem(root, "reboottime");
//   if (reboottime_json != NULL)
//   {
//     char *reboottime = reboottime_json->valuestring;
//     unsigned int num = atoi(reboottime);
//     uvr_log_i("reboottime=%d", num);
//     SIM_MSG_T send_reboottime = {SIM_MSG_INIT, 0, -1, NULL};

//     send_reboottime.arg2 = num;
//     status = sAPI_MsgQSend(uvr_sysreset_msgq, &send_reboottime);
//     if (status != SC_SUCCESS)
//     {
//       sAPI_Free(send_reboottime.arg3);
//       uvr_log_e("send uvr reboottime msg error,status = [%d]", status);
//     }
//   }
//   else
//   {
//     uvr_log_e("reboottime is NULL!");
//   }
//   // cJSON *spreboottime_json = cJSON_GetObjectItem(root, "spreboottime");
//   // char *spreboottime = spreboottime_json->valuestring;
//   // if (spreboottime != NULL)
//   // {
//   //     uvr_log_i("spreboottime=%s", spreboottime);
//   //     unsigned int num = atoi(spreboottime);
//   //     uvr_log_i("num=%d", num);
//   // }
//   // cJSON *nwreboottime_json = cJSON_GetObjectItem(root, "nwreboottime");
//   // char *nwreboottime = nwreboottime_json->valuestring;
//   // if (nwreboottime != NULL)
//   // {
//   //     uvr_log_i("nwreboottime");
//   //     unsigned int num = atoi(nwreboottime);
//   //     uvr_log_i("num=%d", num);
//   // }
//   // cJSON *remotecmd_json = cJSON_GetObjectItem(root, "remotecmd");
//   // char *remotecmd = remotecmd_json->valuestring;
//   // if (remotecmd != NULL)
//   // {
//   //     uvr_log_i("remotecmd");
//   // }
//   // cJSON *ntptime_json = cJSON_GetObjectItem(root, "ntptime");
//   // int ntptime = ntptime_json->valueint;
//   // if (ntptime != NULL)
//   // {
//   //     uvr_log_i("ntptime");
//   // }
//   // cJSON *cjsontask = cJSON_GetObjectItem(root, "task");
//   // cJSON *task = cJSON_GetArrayItem(cjsontask, 0);
//   // if (task != NULL)
//   // {
//   //     uvr_log_i("task");
//   // }
//   /////////////////////////////////////////////////////
//   cJSON *netchan = cJSON_GetObjectItem(root, "netchan");
//   if (netchan != NULL)
//   {
//     uvr_log_d("netchan");
//     uvr_cjson_netchan(netchan); // set if not empty.
//   }
//   else
//   {
//     uvr_log_d("netchan is NULL!");
//   }
//   cJSON *autopoll = cJSON_GetObjectItem(root, "autopoll");
//   if (autopoll != NULL)
//   {
//     uvr_log_d("autopoll");
//     uvr_cjson_autopoll(autopoll); // set if not empty
//   }
//   // sAPI_TaskSleep(20);
//   // sprintf(reason, "success");
//   // return_json(1, reason); // set ok return success json
//   // memset(reason, 0, sizeof(reason));
// }

// // #define JSON_MEM_DEBUG_PRINT
// #ifdef JSON_MEM_DEBUG_PRINT
// #define dbg_printf uvr_log_i
// #else
// #define dbg_printf
// #endif
// static int dbg_malloc_time = 0;
// static long long dbg_malloc_size = 0;
// void *agent_malloc(size_t size)
// {
//   void *mem = UVR_MALLOC(size);
//   if (mem != NULL)
//   {
//     dbg_malloc_time++;
//     dbg_malloc_size += size;
//     dbg_printf("cJSON_malloc OK time=%d, size=%d\r\n", dbg_malloc_time, dbg_malloc_size);
//   }
//   else
//   {
//     dbg_printf("cJSON_malloc failed,  size=%d\r\n", size);
//   }
//   return mem;
// }

// void agent_free(void *mem)
// {
//   sAPI_Free(mem);
//   if (dbg_malloc_time)
//     dbg_malloc_time--;
//   dbg_printf("cJSON sAPI_Free time =%d\r\n", dbg_malloc_time);
// }

// int agent_free_left(void)
// {
//   return dbg_malloc_time;
// }

// cJSON_Hooks json_hooks;
// void uvr_mqtt_receive_task(void *ptr)
// {
//   json_hooks.malloc_fn = agent_malloc;
//   json_hooks.free_fn = agent_free;
//   cJSON_InitHooks(&json_hooks);
//   cJSON *root;
//   char uvr_check_sub_topic[100] = {0};
//   uint8_t rec_times_flag = 0;
//   //////////////////////////////////Wait for my subscription to receive a message//////////////////////////////////////////////////
//   while (1)
//   {
//     SIM_MSG_T msgq_data_recv = {SIM_MSG_INIT, 0, -1, NULL};
//     SCmqttData *sub_data = NULL;
//     sAPI_MsgQRecv(uvr_init_sub_rec_msgq, &msgq_data_recv, SC_SUSPEND);
//     if ((SC_SRV_MQTT != msgq_data_recv.msg_id) || (0 != msgq_data_recv.arg1) || (NULL == msgq_data_recv.arg3)) // wrong msg received
//       continue;
//     rec_times_flag++;
//     uvr_log_i("rec_times_flag = %d", rec_times_flag);
//     sub_data = (SCmqttData *)(msgq_data_recv.arg3);

//     uvr_log_i("mqtt receive-----index: [%d]; tpoic_len: [%d]; tpoic: [%s]", sub_data->client_index, sub_data->topic_len, sub_data->topic_P);

//     uvr_log_i("mqtt receive-----payload_len: [%d]", sub_data->payload_len);

//     uvr_log_i("mqtt receive-----mqtt payload is %s", sub_data->payload_P);

//     //////////////////////////////receive string_data to json/////////////////////////////////
//     root = cJSON_Parse(sub_data->payload_P); // 将JSON字符串转换成JSON结构体
//     ///////////////////////////////printf root json///////////////////////////////////////////
//     char *cjson_str = cJSON_Print(root);
//     if (root == NULL) // 判断转换是否成功
//     {
//       uvr_log_e("mqtt receive payload root json error...\r\n");
//     }
//     else
//     {
//       uvr_log_i("%s\n", cjson_str); // 打包成功调用cJSON_Print打印输出
//     }

//     /////////////////////////////save root json to file////////////////////////////////////////
//     SCFILE *file_root_json = sAPI_fopen("C:/root_json", "wb");
//     uint32_t write_len = strlen(cjson_str);
//     uint32_t actual_write_len = 0;
//     if (NULL == file_root_json)
//     {
//       uvr_log_e("open root_json fail!");
//     }
//     else
//     {
//       // 打开文件写入json内容
//       actual_write_len = sAPI_fwrite(cjson_str, 1, write_len, file_root_json);
//       uvr_log_i("File write root json success");
//     }
//     uvr_log_i("root json write len: %d,act len: %d", write_len, actual_write_len);
//     sAPI_fclose(file_root_json);

//     sAPI_TaskSleep(50);
//     sAPI_Free(cjson_str);
//     /////////////////////////////////////////////////////////////////////////////
//     memset(uvr_check_sub_topic, 0, sizeof(uvr_check_sub_topic));
//     sprintf(uvr_check_sub_topic, "rqiot/dtuissued/%s", imei_get);
//     uvr_log_i("uvr_check_sub is %s", uvr_check_sub_topic);
//     if (strcmp(sub_data->topic_P, uvr_check_sub_topic) == 0)
//     {
//       /////////////////获取json文件中的devid/////////////////
//       cJSON *devid_json = cJSON_GetObjectItem(root, "devid");
//       if (devid_json != NULL)
//       {
//         char devid[20] = {0};
//         sprintf(devid, "%s", devid_json->valuestring);
//         uvr_log_d("devid=%s", devid);
//         if (strcmp(devid, imei_get) == 0) ////////////判断是否为本机的参数
//         {
//           if (rec_times_flag == 1) ///////如果是第一次接收参数,检验密码
//           {
//             cJSON *pass_json = cJSON_GetObjectItem(root, "password");
//             if (pass_json != NULL)
//             {
//               char *parm_password = pass_json->valuestring;
//               uvr_log_d("password = %s", parm_password);
//               if (strcmp(parm_password, "123456") == 0)
//               {
//                 uvr_log_i("password correct!");
//                 uvr_cjson_isnotnull(root);
//               }
//               else
//               {
//                 uvr_log_e("password error!");
//                 // sprintf(reason, "is first time set,but param password is err");
//                 // return_json(2, reason);
//                 // memset(reason, 0, sizeof(reason));
//                 rec_times_flag = 0;
//               }
//             }
//             else
//             {
//               uvr_log_e("is first time set,but param password is err!");
//               // sprintf(reason, "is first time set,but param password is err");
//               // return_json(2, reason);
//               // memset(reason, 0, sizeof(reason));
//             }
//           }
//           else
//           {
//             uvr_cjson_isnotnull(root);
//           }
//         }
//         else
//         {
//           uvr_log_e("devid is not mine");
//           // sprintf(reason, "devid is not mine");
//           // return_json(2, reason);
//           // memset(reason, 0, sizeof(reason));
//         }

//         sAPI_Free(msgq_data_recv.arg3);
//         cJSON_Delete(root);
//         uvr_log_i("Check json UVR_MALLOC time left %d", agent_free_left());
//       }
//     }
//   }
// }

// void uvr_mqtt_connect_another_server(void *ptr)
// {
//   SIM_MSG_T msgq_mqtt_connect_data_recv = {SIM_MSG_INIT, 0, -1, NULL};
//   net_chan_t *recv_conn_msg = NULL;
//   SCmqttResultType ret;
//   char host[1024];
//   unsigned char card_index;
//   while (1)
//   {
//     sAPI_MsgQRecv(uvr_mqtt_conn_msgq, &msgq_mqtt_connect_data_recv, SC_SUSPEND);
//     if (msgq_mqtt_connect_data_recv.arg3 == NULL)
//     {
//       uvr_log_e("msgq_mqtt_connect_data_recv is NULL!,another mqtt server connect is break!");
//       continue;
//     }
//     recv_conn_msg = (net_chan_t *)(msgq_mqtt_connect_data_recv.arg3);

//     sAPI_SysGetBindSim(&card_index);
//     if (card_index)
//       uvr_ui_update(SIM1_ONLINE, NULL);
//     else
//       uvr_ui_update(SIM0_ONLINE, NULL);
//     //////////////////////////start mqtt connect/////////////////////////////
//     if (1 == recv_conn_msg->protocol)
//     {
//       uvr_log_i("uvr mqtt connect another server start, If it fails, please wait for the second connection!");
//       ret = sAPI_MqttStart(-1);
//       if (ret == SC_MQTT_RESULT_SUCCESS)
//       {
//         uvr_log_i("MQTT start success!");
//       }
//       else
//       {
//         uvr_log_e("MQTT start failure! ret = %d", ret);
//       }
//       if (another_connect_flag == 1)
//       {
//         ret = sAPI_MqttDisConnect(0, NULL, 1, 60);
//         if (ret != SC_MQTT_RESULT_SUCCESS)
//         {
//           uvr_log_e("another mqtt disconnect failure! ret = %d", ret);
//         }
//         ret = sAPI_MqttRel(1);
//         if (ret != SC_MQTT_RESULT_SUCCESS)
//         {
//           uvr_log_e("another mqtt rel failure! ret = %d", ret);
//         }
//       }
//       for (int i = 0; i < 2; i++)
//       {
//         another_connect_flag = 0;
//         ret = sAPI_MqttAccq(0, NULL, 1, recv_conn_msg->client_id, 0, uvr_another_sub_rec_msgq); // connect client_index is 1
//         if (ret == SC_MQTT_RESULT_SUCCESS)
//         {
//           uvr_log_i("another mqtt accq success!");
//         }
//         else
//         {
//           uvr_log_e("another mqtt accq failure! ret = %d", ret);
//         }
//         sprintf(host, "tcp://%s:%d", recv_conn_msg->server_address, recv_conn_msg->server_port);
//         // sAPI_MqttConnLostCb(uvr_network_callback);
//         ret = sAPI_MqttConnect(0, NULL, 1, host, recv_conn_msg->heart, recv_conn_msg->clean_session, recv_conn_msg->client_username, recv_conn_msg->client_passwd);
//         memset(host, 0, sizeof(host));
//         if (ret == SC_MQTT_RESULT_SUCCESS)
//         {
//           uvr_log_i("another mqtt connect success!");
//         }
//         else if (ret == SC_MQTT_RESULT_INVALID_PARAMETER)
//         {
//           uvr_log_e("invalid parameter!");
//           // sprintf(reason, "netchan is invalid parameter");
//           // return_json(2, reason);
//           // memset(reason, 0, sizeof(reason));
//           break;
//         }
//         else
//         {
//           uvr_log_e("another mqtt connect failure! ret = %d,please wait for 1S to reconnect!", ret);
//           // sprintf(reason, "another mqtt server connect fail,ret = %d", ret);
//           // return_json(2, reason);
//           // memset(reason, 0, sizeof(reason));
//           // sAPI_TaskSleep(1 * 200);
//           continue;
//         }
//         ////////////////////////subscribe////////////////////////////////////
//         ret = sAPI_MqttSubTopic(1, recv_conn_msg->sub_topic, strlen(recv_conn_msg->sub_topic), recv_conn_msg->sub_qos);
//         ret = sAPI_MqttSub(1, NULL, 0, 0, 0);
//         if (ret == SC_MQTT_RESULT_SUCCESS)
//         {
//           uvr_log_i("another mqtt topic subscribe success!");
//           another_connect_flag = 1;

//           if (card_index)
//             uvr_ui_update(SIM1_CONNECT_READY, NULL);
//           else
//             uvr_ui_update(SIM0_CONNECT_READY, NULL);
//           break;
//         }
//         else
//         {
//           uvr_log_e("another mqtt topic subscribe failure! ret = %d,please wait for 1S to reconnect", ret);
//           sAPI_TaskSleep(1 * 200);
//         }
//       }
//       uvr_log_i("another mqtt server connect task break!");
//     }
//     sAPI_Free(msgq_mqtt_connect_data_recv.arg3);
//   }
// }

// void uvr_mqtt_another_server_recv(void *ptr)
// {
//   while (1)
//   {
//     SIM_MSG_T msgq_another_recv = {SIM_MSG_INIT, 0, -1, NULL};
//     SCmqttData *sub_data = NULL;
//     sAPI_MsgQRecv(uvr_another_sub_rec_msgq, &msgq_another_recv, SC_SUSPEND);
//     if ((SC_SRV_MQTT != msgq_another_recv.msg_id) || (0 != msgq_another_recv.arg1) || (NULL == msgq_another_recv.arg3)) // wrong msg received
//       continue;
//     sub_data = (SCmqttData *)(msgq_another_recv.arg3);

//     uvr_log_i("mqtt another receive-----index: [%d]; tpoic_len: [%d]; tpoic: [%s]", sub_data->client_index, sub_data->topic_len, sub_data->topic_P);

//     uvr_log_i("mqtt another receive-----payload_len: [%d]", sub_data->payload_len);

//     uvr_log_i("mqtt another receive-----mqtt another payload is %s", sub_data->payload_P);

//     //////////////////////////////receive string_data to json/////////////////////////////////
//     cJSON *user = cJSON_Parse(sub_data->payload_P); // 将JSON字符串转换成JSON结构体
//     char *cjson_str = cJSON_Print(user);
//     if (user == NULL) // 判断转换是否成功
//     {
//       uvr_log_e("user cjson error...\r\n");
//     }
//     else
//     {
//       uvr_log_i("%s\n", cjson_str); // 打包成功调用cJSON_Print打印输出
//     }
//     sAPI_TaskSleep(10);
//     sAPI_Free(cjson_str);
//     ////////////////////////////////////////////////////////////////////////////
//     if (NULL != sub_data->topic_P)
//     {
//       uvr_log_i("start to send another rec msg!");
//       cJSON *data = cJSON_GetObjectItem(user, "data");
//       SC_STATUS status = SC_SUCCESS;
//       SIM_MSG_T send_data = {SIM_MSG_INIT, 0, -1, NULL};
//       if (data != NULL)
//       {
//         char *cmd_str = NULL;
//         cmd_str = UVR_MALLOC(CMD_SIZE);
//         memset(cmd_str, 0, CMD_SIZE);

//         strcpy(cmd_str, data->valuestring);
//         send_data.arg1 = 1;
//         send_data.arg3 = cmd_str;
//         uvr_log_i("cmd_str is %s", cmd_str);
//         // uvr_log_i("send_data.arg3 is %s", send_data.arg3);
//         status = sAPI_MsgQSend(uvr_autopoll_stop_msgq, &send_data);
//         if (status != SC_SUCCESS)
//         {
//           sAPI_Free(send_data.arg3);
//           uvr_log_e("uvr_autopoll_stop send msg error");
//         }
//         else
//         {
//           uvr_log_i("send autopoll data msg is success");
//         }
//         sAPI_TaskSleep(10);
//       }
//       else
//       {
//         uvr_log_e("another mqtt rec data is null");
//       }
//     }
//     sAPI_TaskSleep(10);
//     sAPI_Free(msgq_another_recv.arg3);
//     cJSON_Delete(user);
//     uvr_log_i("cjson delete user json is OK!");
//   }
// }

// void uvr_autopoll_task(void *ptr)
// {
//   SC_Uart_Return_Code ret = SC_UART_RETURN_CODE_OK;
//   SIM_MSG_T msgq_autopoll_stop = {SIM_MSG_INIT, 0, -1, NULL};
//   char autopoll_stop_cmd[50];
//   int rw_length = 0;
//   // uint16_t crc_check = 0;
//   // uint8_t crc16_check1;
//   // uint8_t crc16_check2;
//   uint16_t modbus_regaddr = 0;
//   while (1)
//   {
//     sAPI_TaskSleep(100);
//     while (1)
//     {
//       sAPI_MsgQRecv(uvr_autopoll_stop_msgq, &msgq_autopoll_stop, 0);
//       if (1 == msgq_autopoll_stop.arg1)
//       {
//         uvr_log_i("=================start to stop autopoll cmd=======================");
//         strcpy(autopoll_stop_cmd, msgq_autopoll_stop.arg3);
//         uvr_log_i("autopoll_stop_cmd is %s", autopoll_stop_cmd);

//         string_to_hex(autopoll_stop_cmd);
//         ret = sAPI_UartWrite(4, (uint8_t *)modbus_cmd, hex_len);
//         if (ret == SC_UART_RETURN_CODE_OK)
//         {
//           uvr_log_i("write autopoll stop cmd success!");
//         }
//         modbus_regaddr = modbus_cmd[2] << 8;
//         modbus_regaddr |= modbus_cmd[3];
//         if ((modbus_cmd[0] == device_addr) || ((modbus_cmd[0] == 0xff) && (modbus_regaddr == 0x9999)))
//         {
//           uvr_log_i("start to senser analysis");
//           uvr_sensor_data_analysis((uint8_t *)modbus_cmd, hex_len);
//         }
//         else
//         {
//           // CRC校验
//           // crc_check = CRC16((uint8_t *)modbus_cmd, hex_len - 2);
//           // crc16_check1 = crc_check & 0xFF;
//           // crc16_check2 = (crc_check >> 8) & 0xFF;

//           // if ((crc16_check1 != modbus_cmd[hex_len - 2]) | (crc16_check2 != modbus_cmd[hex_len - 1]))
//           // {
//           //     uvr_log_i("modbus cmd CRC check error");
//           //     sprintf(reason, "your modbus cmd CRC check error");
//           //     sAPI_TaskSleep(30);
//           //     return_json(2, reason);
//           //     memset(reason, 0, sizeof(reason));
//           // }
//           // else
//           // {
//           uvr_send_sensor_publish();
//           // }
//         }
//         hex_len = 0;
//         memset(modbus_cmd, 0, hex_len);

//         uvr_log_i("autopoll stop is ok");
//         msgq_autopoll_stop.arg1 = 0;
//         uvr_log_i("clear autopoll stop cmd");
//         sAPI_TaskSleep(10);
//         sAPI_Free(msgq_autopoll_stop.arg3);
//       }
//       else
//       {
//         break;
//       }
//       sAPI_TaskSleep(500);
//     }
//     if (1 == another_connect_flag && autopoll_flag == 1 && autopoll_layout == 2)
//     {
//       uvr_log_i("======1 == another_connect_flag && autopoll_flag == 1,start to autopoll=======");
//       ///////////////////////////////////////////////////
//       SCFILE *fp = sAPI_fopen(TEST_JSON, "rb");
//       if (NULL == fp)
//       {
//         uvr_log_i("json file %s can't find", TEST_JSON);
//       }
//       char *buf = UVR_MALLOC(1024);
//       if (NULL == buf)
//       {
//         uvr_log_e("UVR_MALLOC buf failed");
//       }
//       memset(buf, 0, 1024);
//       rw_length = sAPI_fread(buf, 1, 2048, fp);
//       uvr_log_i("read autopoll buf is %s", buf);
//       sAPI_fclose(fp);
//       if (rw_length)
//       {
//         // uvr_log_i("read data length %d", rw_length);
//         cJSON *autojson = cJSON_Parse(buf);
//         sAPI_Free(buf);
//         if (autojson != NULL)
//         {
//           int poll_size = cJSON_GetArraySize(autojson);
//           for (int k = 0; k < poll_size; k++)
//           {
//             cJSON *uvr_red_cmd = cJSON_GetArrayItem(autojson, k);
//             char *uvr_cmd = uvr_red_cmd->valuestring;
//             string_to_hex(uvr_cmd);
//             ret = sAPI_UartWrite(4, (uint8_t *)modbus_cmd, hex_len);
//             sAPI_TaskSleep(30);
//             if (ret == SC_UART_RETURN_CODE_OK)
//             {
//               uvr_log_i("write modbus_cmd success!");
//             }
//             uvr_send_sensor_publish();
//             uvr_log_i("wait for autopoll time");
//             sAPI_TaskSleep(autopoll_time / 5);

//             memset(modbus_cmd, 0, hex_len);
//             hex_len = 0;
//           }
//         }
//         cJSON_Delete(autojson);
//       }
//     }
//   }
// }

// unsigned char uvr_mqtt_init_con_to_another_stack[1024 * 10] = {0};
// unsigned char uvr_mqtt_rec_to_another_stack[1024 * 10] = {0};
// static unsigned char uvr_mqtt_autopoll_task_stack[1024 * 10] = {0};
// static sTaskRef uvr_mqtt_init_con_another_task_handler = NULL;
// static sTaskRef uvr_mqtt_to_another_task_handler = NULL;
// static sTaskRef uvr_mqtt_autopoll_task_handler = NULL;

// void uvr_mqtt_task_create(unsigned char priority)
// {

//   ///////////////uart消息队列////////////////////
//   if (SC_SUCCESS != sAPI_MsgQCreate(&uvr_uart_msgq, "uvr_uart_msgq", sizeof(SIM_MSG_T), 12, SC_FIFO))
//   {
//     uvr_log_e("uart msgq create err");
//   }

//   ///////////////mqtt network err disconnect消息队列////////////////
//   if (SC_SUCCESS != sAPI_MsgQCreate(&uvr_mqtt_disconn_msgq, "uvr_mqtt_disconn_msgq", sizeof(SIM_MSG_T), 4, SC_FIFO))
//   {
//     uvr_log_e("uvr_mqtt disconnect msgq create err");
//   }

//   ///////////////rec消息队列////////////////////
//   if (SC_SUCCESS != sAPI_MsgQCreate(&uvr_init_sub_rec_msgq, "uvr_init_sub_rec_msgq", (sizeof(SIM_MSG_T)), 4, SC_FIFO))
//   {
//     uvr_log_e("uvr_init_sub_rec message queue create err!");
//   }

//   ///////////////another server connect消息队列////////////////////
//   if (SC_SUCCESS != sAPI_MsgQCreate(&uvr_mqtt_conn_msgq, "uvr_mqtt_conn_msgq", sizeof(SIM_MSG_T), 4, SC_FIFO))
//   {
//     uvr_log_e("uvr mqtt connect msgq create err");
//   } ///////////////rec another 消息队列////////////////////
//   if (SC_SUCCESS != sAPI_MsgQCreate(&uvr_another_sub_rec_msgq, "uvr_another_sub_rec_msgq", (sizeof(SIM_MSG_T)), 4, SC_FIFO))
//   {
//     uvr_log_e("uvr another sub rec message queue create err!");
//   }

//   ///////////////stop autopoll cmd消息队列////////////////////
//   if (SC_SUCCESS != sAPI_MsgQCreate(&uvr_autopoll_stop_msgq, "uvr_autopoll_stop_msgq", sizeof(SIM_MSG_T), 4, SC_FIFO))
//   {
//     uvr_log_e("uvr_mqtt stop autopoll msgq create err");
//   }
//   //////////////sysreset 消息队列//////////////////////////
//   if (SC_SUCCESS != sAPI_MsgQCreate(&uvr_sysreset_msgq, "uvr_sysreset_msgq", sizeof(SIM_MSG_T), 1, SC_FIFO))
//   {
//     uvr_log_e("uvr_mqtt sysreset msgq create err");
//   }
//   getimei();
//   ///////////////mqtt connect another task/////////////////////
//   if (SC_SUCCESS != sAPI_TaskCreate(&uvr_mqtt_init_con_another_task_handler,
//                                     uvr_mqtt_init_con_to_another_stack,
//                                     1024 * 10,
//                                     SC_DEFAULT_TASK_PRIORITY + 12,
//                                     (char *)"uvr mqtt init connect to another task",
//                                     uvr_mqtt_connect_another_server,
//                                     (void *)0))
//   {
//     uvr_log_e("UVR mqtt con to another server task create err!");
//   }
//   else
//   {
//     uvr_log_i("UVR mqtt con to another server task create succeed!");
//   }

//   ///////////////mqtt another server rec task/////////////////////
//   if (SC_SUCCESS != sAPI_TaskCreate(&uvr_mqtt_to_another_task_handler,
//                                     uvr_mqtt_rec_to_another_stack,
//                                     1024 * 10,
//                                     SC_DEFAULT_TASK_PRIORITY + 13,
//                                     (char *)"uvr mqtt another receive task",
//                                     uvr_mqtt_another_server_recv,
//                                     (void *)0))
//   {
//     uvr_log_e("UVR mqtt receive to another task create err!");
//   }
//   else
//   {
//     uvr_log_i("UVR mqtt receive to another task create succeed!");
//   }
//   // ///////////////mqtt autopoll task/////////////////////
//   if (SC_SUCCESS != sAPI_TaskCreate(&uvr_mqtt_autopoll_task_handler,
//                                     uvr_mqtt_autopoll_task_stack,
//                                     1024 * 10,
//                                     SC_DEFAULT_TASK_PRIORITY + 14,
//                                     (char *)"uvr autopoll task",
//                                     uvr_autopoll_task,
//                                     (void *)0))
//   {
//     uvr_log_e("UVR mqtt autopoll task create err!");
//   }
//   else
//   {
//     uvr_log_i("UVR mqtt autopoll task create succeed!");
//   }
// }

// void uvr_mqtt_monitor_task(void *ptr)
// {
//   while (1)
//   {
//     sAPI_TaskSleep(100);
//     if (mqtt_connect_flag == 1)
//     {
//       uvr_log_i("card is swapping! please wait for 5S!");
//       sAPI_TaskSleep(5 * 200);
//       uvr_mqtt_init();
//       mqtt_connect_flag = 0;
//       uvr_log_i("mqtt_connect_flag is 0");
//     }
//   }
// }

// static unsigned char uvr_mqtt_monitor_task_stack[1024 * 10] = {0};
// static sTaskRef uvr_mqtt_monitor_task_handler = NULL;
// void uvr_mqtt_monitor_task_create(unsigned char priority)
// {
//   if (SC_SUCCESS != sAPI_TaskCreate(&uvr_mqtt_monitor_task_handler,
//                                     uvr_mqtt_monitor_task_stack,
//                                     1024 * 10,
//                                     priority,
//                                     (char *)"uvr mqtt monitor task",
//                                     uvr_mqtt_monitor_task,
//                                     (void *)0))
//   {
//     uvr_log_e("UVR mqtt monitor task create err!");
//   }
//   else
//   {
//     uvr_log_i("UVR mqtt monitor task create succeed!");
//   }
// }