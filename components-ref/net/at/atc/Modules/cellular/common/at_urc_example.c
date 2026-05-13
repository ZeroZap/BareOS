/**
 * @file at_urc_example.c
 * @brief URC使用示例
 */

#include "at_client.h"
#include "at_urc.h"

/* 示例：网络状态URC处理器 */
static void urc_network_status_handler(at_device_t *dev, const char *data,
                                       void *user_data)
{
    int status = 0;
    sscanf(data, "+CREG: %*d,%d", &status);

    printf("Network status changed: %d\n", status);

    /* 触发事件通知 */
    if (user_data) {
        int *network_event = (int *)user_data;
        *network_event     = status;
    }
}

/* 示例：短信通知URC处理器 */
static void urc_sms_notification_handler(at_device_t *dev, const char *data,
                                         void *user_data)
{
    printf("New SMS received!\n");

    /* 可以在这里触发短信读取 */
    at_send_cmd("AT+CMGR=1", NULL, NULL, 1000);
}

/* 示例：来电通知URC处理器 */
static void urc_call_notification_handler(at_device_t *dev, const char *data,
                                          void *user_data)
{
    char number[32];

    if (strstr(data, "RING")) {
        printf("Incoming call\n");
    } else if (sscanf(data, "+CLIP: \"%31[^\"]\"", number) == 1) {
        printf("Call from: %s\n", number);
    }
}

/* 示例：GPS数据URC处理器 */
static void urc_gps_data_handler(at_device_t *dev, const char *data,
                                 void *user_data)
{
    double lat = 0, lon = 0;
    int fix = 0;

    if (sscanf(data, "+GPS: %lf,%lf,%d", &lat, &lon, &fix) == 3) {
        printf("GPS: Lat=%f, Lon=%f, Fix=%d\n", lat, lon, fix);
    }
}

/* 示例：多设备URC管理 */
void urc_multi_device_example(void)
{
    at_client_t *client = at_client_init();

    /* 创建设备1 (GSM模块) */
    at_device_t *gsm = at_device_register(
        "GSM", gsm_read_byte, gsm_write_data, get_system_tick, gsm_priv_data);

    /* 创建设备2 (GPS模块) */
    at_device_t *gps = at_device_register(
        "GPS", gps_read_byte, gps_write_data, get_system_tick, gps_priv_data);

    /* 初始化URC管理器 */
    at_urc_init(gsm);
    at_urc_init(gps);

    /* 动态注册URC处理器 */
    int network_urc_id =
        at_urc_register(gsm, "+CREG:", URC_MATCH_PREFIX,
                        urc_network_status_handler, &network_status);

    int sms_urc_id = at_urc_register(
        gsm, "+CMTI:", URC_MATCH_PREFIX, urc_sms_notification_handler, NULL);

    int call_urc_id = at_urc_register(
        gsm, "RING", URC_MATCH_CONTAIN, urc_call_notification_handler, NULL);

    int gps_urc_id = at_urc_register(
        gps, "+GPS:", URC_MATCH_PREFIX, urc_gps_data_handler, NULL);

    /* 启用/禁用特定URC处理器 */
    at_urc_set_enable(gsm, sms_urc_id, false); /* 暂时禁用短信通知 */

    /* 在RTOS中创建URC处理任务 */
#ifdef USE_RTOS
    xTaskCreate(at_urc_task, "URC_GSM", 2048, gsm, 2, NULL);
    xTaskCreate(at_urc_task, "URC_GPS", 2048, gps, 2, NULL);
#endif

    /* 在裸机中轮询处理URC */
#ifndef USE_RTOS
    while (1) {
        at_urc_poll(gsm);
        at_urc_poll(gps);
        delay_ms(10);
    }
#endif
}

/* 示例：条件URC处理器 - 只在特定状态下处理 */
static bool gsm_powered_on = false;
static int power_on_urc_id = -1;

static void urc_power_on_handler(at_device_t *dev, const char *data,
                                 void *user_data)
{
    if (strstr(data, "RDY")) {
        gsm_powered_on = true;
        printf("GSM module powered on\n");

        /* 启用其他URC处理器 */
        at_urc_set_enable(dev, network_urc_id, true);
        at_urc_set_enable(dev, sms_urc_id, true);
    }
}

void conditional_urc_example(at_device_t *dev)
{
    /* 首先只注册电源状态URC */
    power_on_urc_id = at_urc_register(
        dev, "RDY", URC_MATCH_EXACT, urc_power_on_handler, NULL);

    /* 注册其他URC但先禁用 */
    network_urc_id = at_urc_register(
        dev, "+CREG:", URC_MATCH_PREFIX, urc_network_status_handler, NULL);
    at_urc_set_enable(dev, network_urc_id, false);

    /* 发送开机命令 */
    at_send_cmd("AT+CFUN=1", NULL, NULL, 3000);
}

/* 示例：数据解析URC处理器 */
typedef struct {
    float temperature;
    float humidity;
    uint32_t timestamp;
} sensor_data_t;

static void urc_sensor_data_handler(at_device_t *dev, const char *data,
                                    void *user_data)
{
    sensor_data_t *sensor = (sensor_data_t *)user_data;

    if (sensor) {
        sscanf(data, "+SENSOR: T=%f,H=%f,TIME=%lu", &sensor->temperature,
               &sensor->humidity, &sensor->timestamp);

        printf("Sensor: Temp=%.2f, Hum=%.2f\n", sensor->temperature,
               sensor->humidity);
    }
}

/* 示例：批量注册URC处理器 */
void batch_urc_registration(at_device_t *dev)
{
    /* URC处理器表 */
    typedef struct {
        const char *pattern;
        urc_match_type_t type;
        urc_callback_t callback;
    } urc_table_t;

    static sensor_data_t sensor_data;

    urc_table_t urc_table[] = {
        { "+CSQ:", URC_MATCH_PREFIX, urc_signal_strength_handler },
        { "+CCLK:", URC_MATCH_PREFIX, urc_time_handler },
        { "+CBC:", URC_MATCH_PREFIX, urc_battery_handler },
        { "+SENSOR:", URC_MATCH_PREFIX, urc_sensor_data_handler },
        { "ALARM", URC_MATCH_CONTAIN, urc_alarm_handler },
        { NULL, URC_MATCH_PREFIX, NULL }
    };

    /* 批量注册 */
    for (int i = 0; urc_table[i].pattern != NULL; i++) {
        at_urc_register(dev, urc_table[i].pattern, urc_table[i].type,
                        urc_table[i].callback, &sensor_data);
    }
}

/* 示例：URC过滤 - 只处理特定数据 */
static void urc_filtered_handler(at_device_t *dev, const char *data,
                                 void *user_data)
{
    int rssi = 0;

    if (sscanf(data, "+CSQ: %d", &rssi) == 1) {
        if (rssi > 10 && rssi < 30) {
            printf("Good signal strength: %d\n", rssi);
        } else {
            printf("Weak signal strength: %d\n", rssi);
        }
    }
}