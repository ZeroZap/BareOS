/**
 * @file at_4g_example.c
 * @brief 4G模块使用示例
 */

#include "at_4g.h"
#include <stdio.h>

/* 回调函数示例 */
static void on_network_changed(net_reg_status_t status)
{
    printf("Network status changed: %d\n", status);

    switch (status) {
    case NET_REGISTERED_HOME:
        printf("Registered on home network\n");
        break;
    case NET_REGISTERED_ROAMING:
        printf("Registered on roaming network\n");
        break;
    case NET_NOT_REGISTERED:
        printf("Not registered\n");
        break;
    default:
        break;
    }
}

static void on_new_sms(const char *number, const char *message)
{
    printf("New SMS from %s: %s\n", number, message);
}

static void on_socket_data(uint8_t link_id, const uint8_t *data, uint32_t len)
{
    printf("Received %d bytes from link %d: %.*s\n", len, link_id, len, data);
}

/* 完整的4G模块应用示例 */
void at_4g_complete_example(void)
{
    at_device_t *dev;
    int ret;

    /* 1. 创建AT设备 */
    dev = at_device_register(
        "4G_MODULE", uart_read_byte, uart_write_data, get_system_tick, NULL);

    if (!dev) {
        printf("Failed to register device\n");
        return;
    }

    /* 2. 初始化4G模块 */
    ret = at_4g_init(dev, MODULE_UNKNOWN); /* 自动检测模块类型 */
    if (ret != 0) {
        printf("Failed to initialize 4G module\n");
        return;
    }

    /* 3. 获取模块信息 */
    char manufacturer[64], model[64], version[64], imei[32];
    at_4g_get_module_info(dev, manufacturer, model, version, imei);
    printf("Module: %s %s, Version: %s, IMEI: %s\n", manufacturer, model,
           version, imei);

    /* 4. 检查SIM卡 */
    sim_status_t sim_status = at_4g_check_sim(dev);
    if (sim_status != SIM_READY) {
        printf("SIM card not ready: %d\n", sim_status);
        return;
    }
    printf("SIM card ready\n");

    /* 5. 获取ICCID和IMSI */
    char iccid[32], imsi[32];
    at_4g_get_iccid(dev, iccid, sizeof(iccid));
    at_4g_get_imsi(dev, imsi, sizeof(imsi));
    printf("ICCID: %s, IMSI: %s\n", iccid, imsi);

    /* 6. 获取网络状态 */
    network_info_t netinfo;
    while (1) {
        ret = at_4g_get_network_status(dev, &netinfo);
        if (ret == 0 && netinfo.status >= NET_REGISTERED_HOME) {
            printf(
                "Network registered: LAC=%s, CI=%s\n", netinfo.lac, netinfo.ci);
            break;
        }
        printf("Waiting for network registration...\n");
        AT_DELAY_MS(5000);
    }

    /* 7. 获取信号强度 */
    signal_info_t signal;
    at_4g_get_signal(dev, &signal);
    printf("Signal: RSSI=%d, BER=%d\n", signal.rssi, signal.ber);

    /* 8. 获取运营商 */
    char operator_name[64];
    at_4g_get_operator(dev, operator_name, sizeof(operator_name));
    printf("Operator: %s\n", operator_name);

    /* 9. 激活PDP上下文 */
    ret = at_4g_activate_pdp(dev, "CMNET", "", "");
    if (ret != 0) {
        printf("Failed to activate PDP context\n");
        return;
    }
    printf("PDP context activated\n");

    /* 10. 获取IP地址 */
    char ip[16];
    at_4g_get_ip_address(dev, ip, sizeof(ip));
    printf("IP Address: %s\n", ip);

    /* 11. 建立TCP连接 */
    ret = at_4g_tcp_connect(dev, 0, "www.example.com", 80);
    if (ret != 0) {
        printf("Failed to connect TCP\n");
    } else {
        printf("TCP connected\n");

        /* 12. 发送HTTP GET请求 */
        const char *http_request =
            "GET / HTTP/1.1\r\n"
            "Host: www.example.com\r\n"
            "Connection: close\r\n"
            "\r\n";

        ret = at_4g_tcp_send(
            dev, 0, (uint8_t *)http_request, strlen(http_request));
        printf("Sent %d bytes\n", ret);

        /* 13. 接收数据（需要通过URC处理） */
        AT_DELAY_MS(5000);

        /* 14. 关闭连接 */
        at_4g_tcp_close(dev, 0);
    }

    /* 15. 发送短信 */
    ret = at_4g_send_sms(dev, "13800138000", "Hello from 4G module!");
    if (ret == 0) {
        printf("SMS sent successfully\n");
    }

    /* 16. 使用HTTP客户端 */
    printf("\nUsing HTTP client:\n");
    at_4g_http_init(dev);

    char http_response[2048];
    ret = at_4g_http_get(
        dev, "http://httpbin.org/get", http_response, sizeof(http_response));
    if (ret > 0) {
        printf("HTTP Response (%d bytes):\n%.200s...\n", ret, http_response);
    }

    /* 17. 使用MQTT客户端 */
    printf("\nUsing MQTT client:\n");
    ret = at_4g_mqtt_connect(
        dev, "mqtt.eclipse.org", 1883, "4G_Client", NULL, NULL);
    if (ret == 0) {
        printf("MQTT connected\n");

        at_4g_mqtt_publish(dev, "test/topic", "Hello MQTT!", 0);

        at_4g_mqtt_disconnect(dev);
    }

    /* 18. 获取网络时间 */
    char time_str[32];
    at_4g_get_network_time(dev, time_str, sizeof(time_str));
    printf("Network time: %s\n", time_str);

    /* 19. 获取数据使用量 */
    uint32_t tx_bytes = 0, rx_bytes = 0;
    at_4g_get_data_usage(dev, &tx_bytes, &rx_bytes);
    printf("Data usage: TX=%lu bytes, RX=%lu bytes\n", tx_bytes, rx_bytes);

    /* 20. 主循环处理URC */
    printf("\nEntering main loop (press Ctrl+C to exit)\n");

    while (1) {
        /* 轮询处理URC */
        at_urc_poll(dev);

        /* 定期检查信号强度 */
        static uint32_t last_check = 0;
        if (get_system_tick() - last_check > 60000) { /* 每分钟检查 */
            signal_info_t sig;
            if (at_4g_get_signal(dev, &sig) == 0) {
                printf("Current signal: RSSI=%d\n", sig.rssi);
            }
            last_check = get_system_tick();
        }

        AT_DELAY_MS(100);
    }
}

/* 物联网应用示例 */
void iot_4g_application(void)
{
    at_device_t *dev;

    /* 初始化 */
    dev = at_device_register(
        "IoT_4G", uart_read_byte, uart_write_data, get_system_tick, NULL);
    at_4g_init(dev, MODULE_UNKNOWN);

    /* 配置APN */
    at_4g_activate_pdp(dev, "nbiot", "", ""); /* NB-IoT专用APN */

    /* 物联网专用URC处理器 */
    at_urc_register(dev, "+NSONMI:", URC_MATCH_PREFIX, urc_nbiot_data, dev);
    at_urc_register(dev, "+NNMI:", URC_MATCH_PREFIX, urc_nbiot_data, dev);

    /* 定期上报数据 */
    while (1) {
        /* 读取传感器数据 */
        float temperature = read_temperature();
        float humidity    = read_humidity();

        /* 构建JSON数据 */
        char json_data[256];
        snprintf(json_data, sizeof(json_data),
                 "{\"temp\":%.1f,\"hum\":%.1f,\"ts\":%lu}", temperature,
                 humidity, (unsigned long)time(NULL));

        /* 通过MQTT上报 */
        at_4g_mqtt_publish(dev, "iot/sensor/data", json_data, 0);

        /* 或者通过HTTP上报 */
        char url[256];
        snprintf(url, sizeof(url),
                 "http://iot.server.com/api/data?temp=%.1f&hum=%.1f",
                 temperature, humidity);
        at_4g_http_get(dev, url, NULL, 0);

        /* 休眠一段时间 */
        AT_DELAY_MS(60000); /* 每分钟上报一次 */
    }
}

/* 车载追踪器示例 */
void vehicle_tracker_example(void)
{
    at_device_t *dev = at_device_register("Tracker", NULL, NULL, NULL, NULL);
    at_4g_init(dev, MODULE_SIM7600);

#ifdef AT_4G_WITH_GPS
    /* 启用GPS */
    at_4g_gps_power(dev, true);

    /* 主循环 */
    while (1) {
        gps_info_t gps;

        /* 获取GPS位置 */
        if (at_4g_get_gps_info(dev, &gps) == 0) {
            printf("Position: %.6f, %.6f, Speed: %.1f km/h\n", gps.latitude,
                   gps.longitude, gps.speed);

            /* 上报到服务器 */
            char report[512];
            snprintf(
                report, sizeof(report),
                "{\"lat\":%.6f,\"lon\":%.6f,\"speed\":%.1f,\"time\":\"%s\"}",
                gps.latitude, gps.longitude, gps.speed, gps.time);

            at_4g_tcp_send(dev, 0, (uint8_t *)report, strlen(report));
        }

        /* 检查是否有控制命令 */
        // ...

        AT_DELAY_MS(10000); /* 每10秒上报一次 */
    }
#endif
}