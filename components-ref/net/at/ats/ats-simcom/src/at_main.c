#include "at.h"
#include "at_adapter.h"
#include "at_cmd.h"
#include "at_main.h"
#include "uvr.h"


at_msgq_t g_at_input_msgq= NULL;
at_msgq_t g_at_cmd_response_msgq= NULL;
uint32_t g_at_handlr_mutex = NULL;
uint32_t g_at_msq_semaphore = NULL;
uint32_t g_at_startup_semaphore = NULL;
sTimerRef *g_uart_timer_ref=NULL;
sTimerRef *g_bypass_timer_ref=NULL;
uint32_t g_bypass_timeout_flag = 0;

uint8_t g_at_uart_rx_buffer[AT_RX_BUFFER_SIZE];
uint32_t g_at_port=0;

enum AT_LOCAL_STARTUP_FLAG {
    AT_STARTUP_FLAG_DEINIT = 0,
    AT_STARTUP_FLAG_NORMAL = 1,
    AT_STARTUP_FLAG_SWITCHING = 2,
    AT_STARTUP_FLAG_INIT  = 3
};
/**
 * 0 - at deinit
 * 1 - at can work normal
 * 2 - in switching
 * 3 - just do local init
*/
uint32_t g_at_local_startup_flag = 0;



/**
 * 0 means we could receiving AT command
 * 1 means receiving valid AT command
 * 2 means parsing the AT command
 * 3 meanns handling the AT command response
*/
uint32_t g_at_input_cmd_in_processing = AT_CMD_PROCESSING_RECEIVE;

at_msg_t *g_at_input_data = NULL;
at_msg_t *g_at_cmd_response_data = NULL;

at_msg_type_t g_at_msg_type;


// extern at_msgq_t g_at_input_msgq;
// extern at_msgq_t g_at_cmd_response_msgq;
uint32_t g_at_registered_table_number = 0;
at_cmd_hdlr_table_t g_at_cmd_hdlr_tables[AT_MAX_GENERAL_TABLE_NUM] = {{0}};
extern uint32_t g_bypass_timeout_flag;



extern uint16_t at_local_calculate_hash_value(uint8_t *at_name,uint32_t *hash_value1,uint32_t *hash_value2);

at_status_t at_send_data(uint8_t *data, uint32_t data_len);

uint32_t at_port_read_data(uint32_t port, uint8_t *data, uint32_t data_len);

uint32_t at_port_send_data(uint32_t port, uint8_t *data, uint32_t data_len);

uint32_t at_uart_send(uint32_t port, uint8_t *data, uint32_t length);

uint32_t at_uart_read(uint32_t port, uint8_t *data, uint32_t length);



at_status_t at_send_data(uint8_t *data, uint32_t data_len)
{
    at_port_send_data(g_at_port, data, data_len);
}


uint32_t at_port_read_data(uint32_t port, uint8_t *data, uint32_t data_len)
{
     at_uart_read(port, data, data_len);
}


uint32_t at_port_send_data(uint32_t port, uint8_t *data, uint32_t data_len)
{
    at_uart_send(port, data, data_len);
}


uint32_t at_uart_send(uint32_t port, uint8_t *data, uint32_t length)
{
    if (SC_UART_RETURN_CODE_OK == sAPI_UsbVcomWrite(data, length)) {
        return length;
    } else {
        return 0;
    }
}


uint32_t at_uart_read(uint32_t port, uint8_t *data, uint32_t length)
{
   int i = length;

   return i;

}


void at_bypass_timeout(uint32_t timeout_flag)
{
    at_status_t status = AT_STATUS_OK;
    at_msg_t msg = {0,0,0,NULL};
    msg.id = AT_MSG_ID_SWITCH_TO_NORMAL;
    msg.data_len = 0;
    msg.port = SC_UART4;

    if (timeout_flag == 0) {  // set by uart idle
        g_bypass_timeout_flag = 1;
    } else if (timeout_flag == 1) { // set by bypass parsing
        g_bypass_timeout_flag = 2;
        status = at_msgq_send(g_at_input_msgq, &msg);
        at_log_i("g_at_input_msgq = AT_MSG_ID_SWITCH_TO_NORMAL");
    }
    // at_log_i("at send idle over 1S");
}

uint32_t at_bypass_timer_status(void)
{
    sTimerStatus timer_status;
    uint32_t expirations=0; //
    if (SC_SUCCESS == sAPI_TimerGetStatus(g_bypass_timer_ref, &timer_status)) {
            // at_log_i("get bypass_timer_status  %d, %d", timer_status.status, timer_status.expirations);
            expirations = timer_status.status;
    }

    if (expirations == 3) {
        return 0;
    } else {
        return 1;
    }

}

void at_bypass_timer_reload(uint32_t timer, uint32_t flag)
{
    sTimerStatus timer_status;
    if (timer == 1) {
        sAPI_TimerGetStatus(g_bypass_timer_ref, &timer_status);
        // at_log_i("get timer status  %d, %d", timer_status); //这样居然也OK，，

        if (timer_status.status == SC_ENABLED) {
            if (sAPI_TimerStop(g_bypass_timer_ref) != SC_SUCCESS) {
                at_log_e("stop bypass timer failed");
            }
        }
        // g_bypass_timeout_flag = 1;
        // if( sAPI_TimerStart(g_bypass_timer_ref, 200, 0, at_bypass_timeout, &g_bypass_timeout_flag) != SC_SUCCESS ){
        if (sAPI_TimerStart(g_bypass_timer_ref, 200, 0, at_bypass_timeout, flag) != SC_SUCCESS) {

            at_log_e("bypass timer start error");
        }
    } else {
        at_log_e("couldn't found timer%d", timer);
    }

}

#define UART_READY_TO_READ                  0x80000000
#define UART_READ_OVER_HALF_OF_THRESHOLD    0x40000000
void at_uart_timeout(uint32_t timeout_data)
{
    // sTimerStatus timer_status;
    at_status_t status = AT_STATUS_OK;
    uint32_t *data = timeout_data;
    uint16_t read_length =  ((*data)&0xffff);
    at_msg_t msg = {0,0,0,NULL};
    // at_log_i("Uart Receive Timeout!, read_length:%d",read_length);
    *data = UART_READY_TO_READ;

    if (read_length > 0) {
        // mutex lock
        msg.id = AT_MSG_ID_READ_CMD;
        msg.data_len = read_length;
        msg.port = g_at_port;

        msg.data = at_malloc(msg.data_len+1);
        memset(msg.data, 0, msg.data_len+1);
        memcpy(msg.data, g_at_uart_rx_buffer, read_length);

        status = at_msgq_send(g_at_input_msgq, &msg);
        if(status != AT_STATUS_OK)
        {
            free(msg.data);
            at_log_e("send uvr_msg error,status = [%d]",status);
        }
        /* !!!! can't give here*/
        // at_semaphore_give(g_at_msq_semaphore);
        // mutex unlock
    }
}


void at_uart_recv_cb(int len, void *para)
{
    sTimerStatus timer_status;
    // SC_STATUS sc_status;

    /**higher 16bits is flag, lower 16bits is read data length*/
    static uint32_t timeout_data=UART_READY_TO_READ;
    static uint16_t read_index = 0;
    uint16_t read_len=0;

    if (timeout_data & UART_READY_TO_READ)
    {
        at_log_i("Uart Start to Read...");
        read_index = 0;
        memset(g_at_uart_rx_buffer, 0, AT_UART_RX_FIFO_THRESHOLD_SIZE);
        timeout_data = 0;
    }

    read_len = sAPI_UsbVcomRead((UINT8 *)&g_at_uart_rx_buffer[read_index], AT_UART_RX_FIFO_THRESHOLD_SIZE);
    read_index += read_len;

    if (read_index > AT_RX_BUFFER_SIZE) {
        at_log_e("Uart Read Size %d Over Threshold Size %d", read_index, AT_RX_BUFFER_SIZE);
        read_index = 0;
    } else if (read_index >= (AT_RX_BUFFER_SIZE>>1) ) {
        timeout_data |= UART_READ_OVER_HALF_OF_THRESHOLD;
    }

    timeout_data = ((timeout_data & 0xffff000) | read_index);


    /** Reload timeout setting */
    sAPI_TimerGetStatus(g_uart_timer_ref, &timer_status);
    // at_log_i("get timer status  %d, %d", timer_status); //这样居然也OK，，
    // at_log_i("get timer status  %d, %d", timer_status.status, timer_status.expirations);

    if (timer_status.status == SC_ENABLED) {
        if (sAPI_TimerStop(g_uart_timer_ref) !=  SC_SUCCESS) {
            at_log_e("stop uart timer failed");
        }
    }

    if( sAPI_TimerStart(g_uart_timer_ref, 2, 0, at_uart_timeout, &timeout_data)!= SC_SUCCESS ){

        at_log_e("uart timer start error");
    }


    // /** Reload timeout setting */
    // sAPI_TimerGetStatus(g_bypass_timer_ref, &timer_status);
    // // at_log_i("get timer status  %d, %d", timer_status); //这样居然也OK，，

    // if (timer_status.status == SC_ENABLED) {
    //     if (sAPI_TimerStop(g_bypass_timer_ref) !=  SC_SUCCESS) {
    //         at_log_e("stop bypass timer failed");
    //     }
    // }

    // // g_bypass_timeout_flag = 1;
    // // if( sAPI_TimerStart(g_bypass_timer_ref, 200, 0, at_bypass_timeout, &g_bypass_timeout_flag) != SC_SUCCESS ){
    // if( sAPI_TimerStart(g_bypass_timer_ref, 200, 0, at_bypass_timeout, 0) != SC_SUCCESS ){

    //     at_log_e("bypass timer start error");
    // }

    return;
}


at_status_t at_port_init(int port, void* cb_fun)
{
    at_status_t ret = AT_STATUS_ERROR;
    SCuartConfiguration uartConfig;
    g_at_port = port;
    uartConfig.BaudRate  =  SC_UART_BAUD_115200;
    uartConfig.DataBits  =  SC_UART_WORD_LEN_8;
    uartConfig.ParityBit =  SC_UART_NO_PARITY_BITS;
    uartConfig.StopBits  =  SC_UART_ONE_STOP_BIT;
    if(sAPI_UartSetConfig(port, &uartConfig) == SC_UART_RETURN_CODE_ERROR)
    {
        at_log_i("Configure UART failure!!");
        return ret;
    }

    if (sAPI_TimerCreate(&g_uart_timer_ref) != SC_SUCCESS)    {
        at_log_i("Create uart timer failure!!");
        return ret;
    }

    if (sAPI_TimerCreate(&g_bypass_timer_ref) != SC_SUCCESS)    {
        at_log_i("Create bypass timer failure!!");
        return ret;
    }

    sAPI_TimerStart(g_bypass_timer_ref, 200, 0, at_bypass_timeout, 0);

    // sAPI_UartRegisterCallback(port,  cb_fun);
    sAPI_UsbVcomRegisterCallbackEX(at_uart_recv_cb, (void *)"at_uart_recv_cb");

    return AT_STATUS_OK;
}


at_status_t at_port_deinit(void)
{

}


at_status_t at_local_init(void)
{
    at_status_t ret = AT_STATUS_ERROR;

    if (g_at_local_startup_flag != AT_STARTUP_FLAG_DEINIT) {
        return AT_STATUS_OK;
    }


    g_at_input_msgq = at_msgq_create("at_cmd_input_msg", sizeof(at_msg_t), AT_MAX_INPUT_MSGQ_NUM);
    if (NULL == g_at_input_msgq)
    {
        at_log_i("at_cmd_input_msg failed");
        return ret;
    }

    g_at_cmd_response_msgq = at_msgq_create("at_cmd_response_msg", sizeof(at_msg_t), AT_MAX_RESPONSE_MSGQ_NUM);
    if (NULL == g_at_cmd_response_msgq)
    {
        at_log_i("g_at_cmd_response_msgq failed");
        return ret;
    }

    g_at_msq_semaphore = at_semaphore_create(10);
    if (NULL == g_at_msq_semaphore)
    {
        at_log_i("g_at_msq_semaphore failed");
        return ret;
    }

    g_at_startup_semaphore = at_semaphore_create(10);
    if (NULL == g_at_startup_semaphore)
    {
        at_log_i("g_at_startup_semaphore failed");
        return ret;
    }
    g_at_input_cmd_in_processing = AT_CMD_PROCESSING_RECEIVE;
    g_at_local_startup_flag = AT_STARTUP_FLAG_INIT;

    return AT_STATUS_OK;

}

at_status_t at_local_deinit(void)
{

}

at_status_t at_init(void)
{
    at_status_t ret = AT_STATUS_ERROR;

    at_cmd_init();

    // 变量初始化
    ret = at_local_init();

    if (ret != AT_STATUS_OK)
    {
        return ret;
    }

    // 接口初始化
    ret = at_port_init(SC_UART4, at_uart_recv_cb);

    if (ret != AT_STATUS_OK)
    {
        return ret;
    }

    g_at_local_startup_flag = AT_STARTUP_FLAG_NORMAL;

    return 0;
}


// at_status_t at_local_parse_basic_cmd(at_parse_cmd_param_ex_t *parse_cmd)
// {
//     uint32_t name_len = 2;   /* AT command begins "AT" */
//     uint8_t  *string_ptr = parse_cmd->string_ptr;
//     uint32_t length = parse_cmd->string_len;

//     while (name_len < length) {
//         if ((string_ptr[name_len]  == AT_CHAR_CR) || (string_ptr[name_len]  == AT_CHAR_LF || (string_ptr[name_len]  == '\0'))) {
//             break;
//         }
//         name_len++;
//     }
//     parse_cmd->name_len = name_len;
//     parse_cmd->parse_pos = name_len;
//     return AT_STATUS_OK;
// }

// at_status_t at_local_parse_extend_cmd(at_parse_cmd_param_ex_t *parse_cmd)
// {
//     uint32_t name_len = 0;
//     uint8_t *at_name = parse_cmd->string_ptr;

//     name_len = at_local_calculate_hash_value(at_name,&(parse_cmd->hash_value1),&(parse_cmd->hash_value2));
//     parse_cmd->name_len = name_len;
//     parse_cmd->parse_pos = name_len;
//     if (0 == name_len) {
//         return AT_STATUS_ERROR;
//     } else {
//         return AT_STATUS_OK;
//     }
// }


at_status_t at_input_bypass_parse(at_msg_t *input_data)
{

}



at_status_t at_read_data(uint32_t port, at_msg_t ** input_data)
{
    at_status_t ret = AT_STATUS_ERROR;
    // at_msg_t at_input_cmd_data;

    if (NULL == *input_data) {
        *input_data = malloc(sizeof(at_msg_t));
    }

    if (at_msgq_receive(g_at_input_msgq, *input_data, SC_NO_SUSPEND) == AT_STATUS_OK) {
        ret = AT_STATUS_OK;
    } else {
        if ((*input_data)->data != NULL) { at_free((*input_data)->data); }
    }

    if (g_at_input_cmd_in_processing != AT_CMD_PROCESSING_RECEIVE)
    {

        if ((*input_data)->data != NULL) { at_free((*input_data)->data); }
        at_log_e(" error processing, drop this cmd");
    }

    return ret;
}

at_status_t at_send_response_data(void)
{
    at_status_t ret = AT_STATUS_ERROR;

    // 获取队列消息
    // 发送大小
    return ret;
}


extern at_status_t at_input_msg_parse(at_msg_t *input_data);
void at_processing(void)
{
    at_status_t ret;
    uint32_t input_msg_num = 0;
    uint32_t response_msg_num = 0;
    uint32_t notify_msg_num = 0;

    while (1) {
        sAPI_TaskSleep(50);
        // if ((NULL != (void*)g_at_msq_semaphore) && (g_at_local_startup_flag != 0)) {
        //     at_log_i("Take at msq semaphore...");
        //     at_semaphore_take(g_at_msq_semaphore);
        // } else if ((NULL != (void*)g_at_startup_semaphore) && (g_at_local_startup_flag == 0)) {
        //     at_log_i("Take at startup semaphore...");
        //     at_semaphore_take(g_at_startup_semaphore);
        // }

        if (g_at_local_startup_flag == AT_STARTUP_FLAG_SWITCHING) {
            continue;
        }

        input_msg_num = at_msgq_get_num(g_at_input_msgq);
        if (input_msg_num > 0) {
           if(AT_STATUS_OK == at_read_data(0, &g_at_input_data)) {
                at_input_msg_parse(g_at_input_data);
                g_at_input_data->data_len = 0;
                g_at_input_data->id = AT_MSG_ID_MAX;

                if (NULL != g_at_input_data->data) {
                    at_free(g_at_input_data->data);
                    g_at_input_data->data = NULL;
                }

           }
        } else {
           // check if need swap back to normal
        }

        response_msg_num = at_msgq_get_num(g_at_cmd_response_msgq);
        if (response_msg_num >0) {
            if (g_at_cmd_response_data == NULL) {
                g_at_cmd_response_data = at_malloc(sizeof(at_msg_t));
            }
            if (at_msgq_receive(g_at_cmd_response_msgq, g_at_cmd_response_data, SC_NO_SUSPEND) == AT_STATUS_OK) {
                at_log_i("get response data %s", g_at_cmd_response_data->data);
                at_send_data(g_at_cmd_response_data->data, g_at_cmd_response_data->data_len);
                g_at_cmd_response_data->data_len = 0;
                g_at_cmd_response_data->id = AT_MSG_ID_MAX;
                at_free(g_at_cmd_response_data->data);
                g_at_cmd_response_data->data = NULL;
            }

        }

    }
}


void at_task(void)
{
    while (1)
    {
        at_processing();
    }
}

/**
  * @brief  Create urc task
  * @param  void
  * @note   There will be additional urc events when there is new request from market.
  * @retval void
  */
#define AT_TASK_STACK_SIZE 1024*10
#define AT_TASK_PRIORITY   150
static sTaskRef uvr_at_task_handler = NULL;
static uint8_t uvr_at_task_stack[AT_TASK_STACK_SIZE] = {0xA5};
extern void at_tcpip_task_init(void);
extern void at_mqtt_init(void);
void uvr_at_task_create(unsigned char priority)
{
    at_init();
    if (NULL != uvr_at_task_handler)
    {
        return;
    }
    at_tcpip_task_init();
    at_mqtt_init();
    if (sAPI_TaskCreate(&uvr_at_task_handler,
        uvr_at_task_stack,
        AT_TASK_STACK_SIZE,
        priority,
        (char*)"uvr at process" ,
        at_task,
        (void *)0) != SC_SUCCESS)
        {
            at_log_e("uvr urc task create error!\n");
        }
}