## Overview



## ATCI acronyms definition



## AT Command Types

Generic AT commands has four types:

| Type            | Command Format            | Description                                                  | Response                            |
| --------------- | ------------------------- | ------------------------------------------------------------ | ----------------------------------- |
| Test Command    | `AT+<CMD>?<\r><\n>`       | Query the Set Commands’ internal parameters and their range of values. | `+<CMD>:<values><\r><\n>OK<\r><\n>` |
| Query Command   | `AT+<CMD>=?<\r><\n>`      | Return the current value of parameters.                      | `+<CMD>:<values><\r><\n>OK<\r><\n>` |
| Set Command     | `AT+<CMD>=<…>[…]<\r><\n>` | Set the value of user-defined parameters in commands, and run these commands. | `OK<\r><\n>`                        |
| Execute Command | `AT+<CMD><\r><\n>`        | Run commands with no user-defined parameters.                | `OK<\r><\n>`                        |

- Not all AT commands support all of the four types mentioned above.
- Currently, only strings and integer numbers are supported as input parameters in AT commands.
- Angle brackets < > designate parameters that can not be omitted.
- Square brackets [ ] designate optional parameters that can be omitted. The default value of the parameter will be used instead when you omit it.



## ATCI architecture



## Folder structure



## How to use this module

- Step1. Call `atci_init() ` at `atci_main.c`

  - `atci_local_init()`
    - init g_atci_handler_semaphore
    - init g_atci_input_command_queue
    - init g_atci_response_command_queue
    - init g_atci_data_cached_queue
    - init g_atci_local_startup_flag = 3
    - init g_atci_processing_semaphore 获取互斥量 （在RX 中断中获取）
  - ret = atci_uart_init((hal_uart_port_t)port)
    - `hal_uart_set_dma(&dma_config) `
    - `hal_uart_register_callback(port, atci_uart_irq, NULL);`
      - atci_uart_irq 在at main 中
  - `atci_init_int()`

- Step 2. Call `atci_register_handler()`to register the AT command handler.

  - 各个功能处理注册

- Step 3. Call `task_create()` to create a task that take name “atci task”

  - atci_input_command_handler
  - atci_local_parse_input_at_cmd
    - skip AT*
    - atci_local_parse_extend_cmd(parse_cmd)
    - atci_local_parse_find_mode(parse_cmd)  `or`  atci_local_parse_basic_cmd(parse_cmd);
  - atci_local_dispatch_cm4_general_handler(parse_cmd); 才进入到各自的处理了。。
    - handler_item = &(g_atci_cm4_general_hdlr_tables[i].item_table[j]);  查早指令对应的hash值是否相同，OK的就是代表有该指令啦！那哪里初始化这hash值呢？前面找找
    - 有对应的hash值，那就可以啦

- Step 4. After the above steps， send AT command through port(such as UART) on master (such as PC). On the target side, the atci module receives data and calls the corresponding registered command handler to provide a response.

  

### Sample code

``` c
// command table
atci_cmd_hdlr_item_t atcmd_table[] {
    {"AT+TEST",   atci_cmd_hdlr_test,  0, 0}
}

*atci_status_t atci_cmd_hdlr_test(atci_parse_cmd_param_t *parse_cmd)
{
    int read_value = 0;
    atci_response_t resonse = {0};
    atci_response_t urc_data = {0};
    char *param = NULL;
    int  param1_val = -1;

    resonse.response_flag = 0; // Command execution is complete.

    switch (parse_cmd->mode) {
        case ATCI_CMD_MODE_TESTING:    // Input data. AT+TEST=?
            printf("AT Test OK.\n");
            strcpy(resonse.response_buf, "+TEST:(0,1)\r\nOK\r\n");
            resonse.response_len = strlen(resonse.response_buf);
            atci_send_response(&resonse);
            break;

        case ATCI_CMD_MODE_READ:    // Input data. AT+TEST?
            printf("AT Read done.\n");
            sprintf(resonse.response_buf, "+TEST:%d\r\n", read_value);
            resonse.response_len = strlen(resonse.response_buf);
            // The ATCI appends 'OK' at the end of the response buffer.
            resonse.response_flag |= ATCI_RESPONSE_FLAG_APPEND_OK;
            atci_send_response(&resonse);
            break;

        case ATCI_CMD_MODE_ACTIVE:  // Input data. AT+TEST
            printf("AT Active OK.\n");
            // Assume the active mode is invalid and returns an error message.
            strcpy(resonse.response_buf, "ERROR\r\n");
            resonse.response_len = strlen(resonse.response_buf);
            atci_send_response(&resonse);
            break;

        case ATCI_CMD_MODE_EXECUTION: // Input data. AT+TEST=<p1>  the handler need to parse the parameters.
            printf("AT Executing...\r\n");
            //Parsing the parameters.
            param = strtok(parse_cmd->string_ptr, ",\n\r");
            param = strtok(parse_cmd->string_ptr, "AT+TEST=");
            param1_val = atoi(param);

            if (param != NULL && (param1_val == 0 || param1_val == 1)) {
                // Valid parameter, update the data and return "OK".
                resonse.response_len = 0;
                // // The ATCI appends 'OK' at the end of the response buffer.
                resonse.response_flag |= ATCI_RESPONSE_FLAG_APPEND_OK;
            } else {
                // Invalide parameter, return "ERROR".
                resonse.response_len = 0;
                // // The ATCI appends 'ERROR' at the end of the response buffer.
                resonse.response_flag |= ATCI_RESPONSE_FLAG_APPEND_ERROR;
            };

            atci_send_response(&resonse);
            param = NULL;

            // Send the URC data.
            sprintf(urc_data.response_buf, "+TEST:urc_data\r\n");
            urc_data.response_len = strlen(urc_data.response_buf);
            urc_data.response_flag |= ATCI_RESPONSE_FLAG_URC_FORMAT;
            atci_send_response(&urc_data);

            break;

        default:
            strcpy(resonse.response_buf, "ERROR\r\n");
            resonse.response_len = strlen(resonse.response_buf);
            atci_send_response(&resonse);
            break;
    }
    return ATCI_STATUS_OK;
}

// Register the AT CMD handler.
void at_cmd_init(void)
{
    atci_status_t ret = ATCI_STATUS_REGISTRATION_FAILURE;

    ret = atci_register_handler(atcmd_table, sizeof(atcmd_table) / sizeof(atci_cmd_hdlr_item_t));
    if (ret == ATCI_STATUS_OK) {
        printf("at_cmd_init register success\r\n");
    } else {
        printf("at_cmd_init register fail\r\n");
    }
}

// ATCI task main function.
void atci_def_task(void *param)
{
    while (1) {
        atci_processing();
    }
}

void app_main(void)
{
    // Initialize the ATCI module and set the UART port.
    atci_status_t ret = ATCI_STATUS_ERROR;
    ret = atci_init(HAL_UART_1);
    if (ret == ATCI_STATUS_OK) {
        // Register the AT CMD handler.
        at_cmd_init();
        // Create a task for the ATCI.
        xTaskCreate( atci_def_task, "ATCI", 1024, NULL, 3, NULL );
    }
}
```



## Functions

