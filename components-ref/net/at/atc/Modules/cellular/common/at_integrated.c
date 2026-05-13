/**
 * @file at_integrated.c
 * @brief 集成示例 - 完整的AT客户端使用
 */

#include "at_client.h"
#include "at_urc.h"
#include "at_enhanced.h"

/* 全局状态管理 */
typedef struct {
    bool gsm_ready;
    bool network_registered;
    bool gps_fixed;
    int signal_strength;
    int battery_level;
} system_status_t;

static system_status_t g_status;

/* URC处理器 */
static void urc_system_status_handler(at_device_t *dev, const char *data,
                                      void *user_data)
{
    system_status_t *status = (system_status_t *)user_data;

    if (strstr(data, "+CPIN: READY")) {
        status->gsm_ready = true;
        printf("GSM SIM card ready\n");
    } else if (strstr(data, "+CREG: 1")) {
        status->network_registered = true;
        printf("Network registered\n");
    } else if (strstr(data, "+CSQ:")) {
        int rssi = 0;
        sscanf(data, "+CSQ: %d", &rssi);
        status->signal_strength = rssi;
        printf("Signal strength: %d\n", rssi);
    } else if (strstr(data, "+CBC:")) {
        int level = 0;
        sscanf(data, "+CBC: %*d,%d", &level);
        status->battery_level = level;
        printf("Battery level: %d%%\n", level);
    }
}

/* 主应用示例 */
void at_client_application(void)
{
    /* 1. 初始化AT客户端 */
    at_client_t *client = at_client_init();

    /* 2. 注册设备 */
    at_device_t *gsm =
        at_device_register("SIM800", sim800_read_byte, sim800_write_data,
                           get_system_tick, &sim800_priv);

    at_device_t *gps = at_device_register(
        "AT6558", gps_read_byte, gps_write_data, get_system_tick, &gps_priv);

    /* 3. 初始化URC管理器 */
    at_urc_init(gsm);
    at_urc_init(gps);

    /* 4. 注册URC处理器 */
    at_urc_register(
        gsm, "+CPIN:", URC_MATCH_PREFIX, urc_system_status_handler, &g_status);
    at_urc_register(
        gsm, "+CREG:", URC_MATCH_PREFIX, urc_system_status_handler, &g_status);
    at_urc_register(
        gsm, "+CSQ:", URC_MATCH_PREFIX, urc_system_status_handler, &g_status);
    at_urc_register(
        gsm, "+CBC:", URC_MATCH_PREFIX, urc_system_status_handler, &g_status);
    at_urc_register(
        gsm, "+CMTI:", URC_MATCH_PREFIX, urc_sms_notification_handler, NULL);

    /* 5. 启动URC处理任务（RTOS） */
#ifdef USE_RTOS
    xTaskCreate(urc_monitor_task, "URC_Monitor", 4096, client, 1, NULL);
#endif

    /* 6. 执行初始化序列 */
    at_cmd_context_t init_seq[] = {
        { "AT", "OK", 1000, NULL, NULL },
        { "ATE0", "OK", 1000, NULL, NULL },      /* 关闭回显 */
        { "AT+CMGF=1", "OK", 1000, NULL, NULL }, /* 设置短信文本模式 */
        { "AT+CNMI=2,1", "OK", 1000, NULL, NULL }, /* 新短信指示 */
    };

    at_execute_sequence(gsm, init_seq, sizeof(init_seq) / sizeof(init_seq[0]));

    /* 7. 主循环 */
    while (1) {
        /* 裸机环境轮询URC */
#ifndef USE_RTOS
        at_urc_poll(gsm);
        at_urc_poll(gps);
#endif

        /* 定期检查状态 */
        static uint32_t last_check = 0;
        if (get_system_tick() - last_check > 60000) { /* 每分钟检查 */
            at_send_cmd("AT+CSQ", NULL, NULL, 1000);
            last_check = get_system_tick();
        }

        delay_ms(10);
    }
}

/* 集成示例：物联网设备 */
void iot_device_example(void)
{
    /* 初始化所有组件 */
    at_device_t *lte    = at_lte_init();
    at_device_t *wifi   = at_wifi_init();
    at_device_t *sensor = at_sensor_init();

    /* 动态URC处理器表 */
    static const struct {
        at_device_t *dev;
        const char *pattern;
        urc_callback_t handler;
    } urc_config[] = {
        { lte, "+CEREG:", lte_network_handler },
        { lte, "+CME ERROR:", lte_error_handler },
        { wifi, "+CWLAP:", wifi_ap_handler },
        { wifi, "+CWJAP:", wifi_connect_handler },
        { sensor, "+DATA:", sensor_data_handler },
    };

    /* 注册所有URC处理器 */
    for (int i = 0; i < sizeof(urc_config) / sizeof(urc_config[0]); i++) {
        at_urc_register(urc_config[i].dev, urc_config[i].pattern,
                        URC_MATCH_PREFIX, urc_config[i].handler, NULL);
    }

    /* 自动URC应答配置 */
    at_set_urc_auto_response(lte, "RING", "ATH"); /* 自动挂断来电 */
    at_set_urc_auto_response(
        wifi, "WIFI DISCONNECTED", "AT+CWJAP"); /* 自动重连 */
}