## 简介
这是at 框架库
- atc 是 at client
- ats 是 at sever
- at 是公共的定义

当前已经参考下列的相关文档做了些操作
rt-thread 的 at 组件 及 https://gitee.com/moluo-tech/AT-Command
及 at-nos.md, COMPLETE_IMPLEMENTATION.md, IMPLEMENTATION_SUMMARY.md XY_README.md

现在需要集齐上述的文档重新 设计一个 AT CLIENT 和 AT Server 框架


## 软件库

- ATC
- ATS

支持不同的接口涉及，可以是 UART, SPI 或者虚拟的都可以

## 移植准备
- ring buffer 的读接口：ATC


## 接口

### 结构体

```c
struct atc_device {
    void (*lock)(void);
    void (*unlock)(void);
    int (*read)(void *data, int num);
    int (*write)(const void *buf, uint16_t len);
    void (*error)(atc_response *resp);
    void (*debug)(const char *fmt, ...);
    void (*enter_sleep)(void);
    void (*exit_sleep)(void);
    uint16_t rx_buf_size;
    uint16_t urc_buf_size;
}

struct atc_attr {
    const char *prefix;
    const char *suffix;

    void *user_data;
}

struct atc{
    atc_device *dev;

    void *user_data;
}

```
### 内核接口
统一采用 xy_osal_xx 接口实现

### 底层接口

- lock 如果是单线程运行，直接 NULL 即可了
- read_bytes(uint16_t n) //from buffer
- write_bytes(uint8_t bytes, uint16_t n) // from buffer
- flush_read_buff(void);
- flush_read_bus(void);
- atc_tick_inc(void); // 放置到一个 tick 的地方
- atc_tick(void)
- atc_process(atc_obj);
- atc_send(atc_hdlr, call back, timeout)
- atc_send_multi(atc_hdlr, timeout)
- atc_contain()
- atc_config(atc_obj)
- atc_enter_sleep(atc_obj)
- atc_exit_sleep(atc_obj)
- atc_urc_add(atc_obj)
- atc_urc_remove(atc_obj)
-

```c
void at_device_process(void) {
    static uint32_t at_device_tick = 0;
    if(atc_get_tick() - at_device_tick > 5) {
        at_obj_process(&at_obj);
    }
}
```

### AT Client API

| Function | Description |
|----------|-------------|
| `xy_at_client_create()` | Create AT client |
| `xy_at_client_init()` | Initialize client |
| `xy_at_client_delete()` | Delete client |
| `xy_at_client_set_hal()` | Set HAL interface |
| `xy_at_exec_cmd()` | Execute AT command |
| `xy_at_create_resp()` | Create response object |
| `xy_at_delete_resp()` | Delete response object |
| `xy_at_resp_get_line()` | Get response line by index |
| `xy_at_resp_parse_line_args()` | Parse response arguments |
| `xy_at_set_urc_table()` | Set URC handler table |
| `xy_at_client_enter_data_mode()` | Enter transparent mode |
| `xy_at_client_exit_data_mode()` | Exit transparent mode |


### AT Server API

| Function | Description |
|----------|-------------|
| `xy_at_server_create()` | Create AT server |
| `xy_at_server_init()` | Initialize server |
| `xy_at_server_delete()` | Delete server |
| `xy_at_server_set_hal()` | Set HAL interface |
| `xy_at_server_start()` | Start server |
| `xy_at_server_stop()` | Stop server |
| `xy_at_server_register_cmd()` | Register command |
| `xy_at_server_printf()` | Send formatted response |
| `xy_at_server_printfln()` | Send response with newline |
| `xy_at_server_print_result()` | Send result code |
| `xy_at_parse_args()` | Parse command arguments |


### 应用接口



### 广域网模块接口
- atc_ng_power(on, off)

### WIFI 模块接口
- atc_wf_xxx

### Eth 模块接口
- atc_eth_xx

### Sensor 模块接口

## Thread Model

### AT Client Threads

```
┌──────────────┐     ┌──────────────┐
│ Application  │────▶│ Command Send │
│   Thread     │     │   (Blocking) │
└──────────────┘     └──────────────┘
                            │
                            ▼
                     ┌──────────────┐
                     │ Parser Thread│
                     │ (Background) │
                     └──────────────┘
                            │
                            ▼
                     ┌──────────────┐
                     │ URC Handlers │
                     └──────────────┘
```

### AT Server Threads

```
┌──────────────┐
│ Parser Thread│
│ (Background) │
└──────────────┘
       │
       ▼
┌──────────────┐
│ Command      │
│ Dispatcher   │
└──────────────┘
       │
       ▼
┌──────────────┐
│ Handler      │
│ Functions    │
└──────────────┘
```

## Porting Guide

### 1. Implement HAL Functions

```c
// For AT Client
int (*get_char)(char *ch, uint32_t timeout);
size_t (*send)(const char *data, size_t len);
size_t (*recv)(char *data, size_t len);

// For AT Server
int (*get_char)(char *ch, uint32_t timeout);
size_t (*send)(const char *data, size_t len);
```

### 2. Configure XY OSAL

Ensure XY OSAL is properly configured for your platform (FreeRTOS, RT-Thread, or bare-metal).

### 3. Adjust Buffer Sizes

Modify configuration defines based on your memory constraints and use case.

## Performance

### Memory Footprint

| Component | RAM (Default Config) |
|-----------|---------------------|
| AT Client | ~1.5 KB |
| AT Server | ~1.0 KB |
| Response Buffer | Configurable |
| Command Table | ~1.5 KB (32 commands) |

### Throughput

- **Command Rate**: Up to 100 commands/sec
- **Data Mode**: Limited by UART speed
- **Response Time**: < 10ms (typical)

## Troubleshooting

### Q: Client commands timeout
- Check UART baud rate
- Verify HAL get_char function
- Increase timeout value
- Check UART RX buffer

### Q: Server not responding
- Verify parser thread is running
- Check HAL get_char implementation
- Enable echo mode for debugging
- Check command registration

### Q: URC not triggered
- Verify URC prefix matches exactly
- Check parser thread priority
- Ensure URC table is set before commands

### Q: Response buffer full
- Increase XY_AT_RESP_MAX_LEN
- Use line_num parameter in xy_at_create_resp()
- Parse responses incrementally

## License

Same as XinYi project

## Version

- **Version**: 1.0.0
- **Date**: 2025-10-27
- **Status**: Ready for Implementation
