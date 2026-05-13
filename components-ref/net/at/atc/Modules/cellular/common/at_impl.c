/**
 * @file at_impl.c
 * @brief AT客户端实现示例 - 基于ringbuffer
 */

#include "at_client.h"
#include "ringbuffer.h" /* 假设有ringbuffer库 */

/* 设备私有数据 */
typedef struct {
    ringbuffer_t *rx_rb; /* 接收ringbuffer */
    ringbuffer_t *tx_rb; /* 发送ringbuffer(可选) */
    void *uart_handle;   /* UART句柄 */
} at_uart_priv_t;

/* 从ringbuffer读取一个字节 */
static int uart_read_byte(at_device_t *dev)
{
    at_uart_priv_t *priv = (at_uart_priv_t *)dev->user_data;

    if (!priv || !priv->rx_rb) {
        return -1;
    }

    uint8_t data;
    if (ringbuffer_get(priv->rx_rb, &data, 1) == 1) {
        return data;
    }

    return -1; /* 无数据 */
}

/* 写入数据到UART */
static void uart_write_data(at_device_t *dev, const uint8_t *data, uint32_t len)
{
    at_uart_priv_t *priv = (at_uart_priv_t *)dev->user_data;

    if (!priv || !data || len == 0) {
        return;
    }

    /* 实际写入UART */
    // uart_send(priv->uart_handle, data, len);

    /* 示例：直接打印 */
    printf("AT TX: %.*s\n", len, data);
}

/* 获取系统tick */
static uint32_t get_system_tick(void)
{
#ifdef USE_RTOS
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
#else
    /* 裸机系统：需要用户实现 */
    // return HAL_GetTick();
    static uint32_t tick = 0;
    return tick++;
#endif
}

/* 示例：解析+CSQ响应 */
static bool csq_resp_handler(at_device_t *dev, const char *resp_line,
                             void *user_data, at_resp_type_t *type)
{
    (void)dev;

    if (strstr(resp_line, "+CSQ:")) {
        int rssi = 0, ber = 0;
        sscanf(resp_line, "+CSQ: %d,%d", &rssi, &ber);

        if (user_data) {
            int *result = (int *)user_data;
            result[0]   = rssi;
            result[1]   = ber;
        }

        *type = AT_RESP_CUSTOM;
        return false; /* 继续等待OK/ERROR */
    }

    if (strstr(resp_line, "OK")) {
        *type = AT_RESP_OK;
        return true;
    } else if (strstr(resp_line, "ERROR")) {
        *type = AT_RESP_ERROR;
        return true;
    }

    return false;
}

/* 使用示例 */
void at_client_example(void)
{
    /* 1. 初始化客户端 */
    at_client_init();

    /* 2. 创建设备私有数据 */
    at_uart_priv_t uart1_priv = {
        .rx_rb       = ringbuffer_create(1024),
        .uart_handle = NULL,
    };

    /* 3. 注册设备 */
    at_device_t *dev1 = at_device_register(
        "UART1", uart_read_byte, uart_write_data, get_system_tick, &uart1_priv);

    /* 4. 发送AT命令 */
    at_resp_type_t resp;

    /* 简单命令 */
    resp = at_send_cmd("AT", NULL, NULL, 1000);
    if (resp == AT_RESP_OK) {
        printf("AT OK\n");
    }

    /* 带数据解析的命令 */
    int csq_result[2] = { 0 };
    resp = at_send_cmd("AT+CSQ", csq_resp_handler, csq_result, 2000);
    if (resp == AT_RESP_OK) {
        printf("RSSI: %d, BER: %d\n", csq_result[0], csq_result[1]);
    }

    /* 5. 发送原始数据 */
    at_send_raw(dev1, (uint8_t *)"DATA", 4);

    /* 6. 多设备示例 */
    at_uart_priv_t uart2_priv = { /* ... */ };
    at_device_t *dev2         = at_device_register(
        "UART2", uart_read_byte, uart_write_data, get_system_tick, &uart2_priv);

    /* 在设备间切换使用 */
    at_set_default_device(dev1);
    at_send_cmd("AT", NULL, NULL, 1000);

    at_set_default_device(dev2);
    at_send_cmd("AT", NULL, NULL, 1000);
}

/* UART接收中断中填充ringbuffer */
void uart_rx_isr(uint8_t data)
{
    static ringbuffer_t *active_rb = NULL;

    if (active_rb) {
        ringbuffer_put(active_rb, &data, 1);
    }
}