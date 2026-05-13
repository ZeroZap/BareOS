/**
 * @file at_protocols_example.c
 * @brief 多协议使用示例
 */

#include "at_protocols.h"

/* HTTPS示例 */
void https_example(at_device_t *dev)
{
    printf("=== HTTPS Example ===\n");

    /* 1. 初始化SSL/TLS */
    ssl_config_t ssl_config = { .ssl_version = 2, /* TLS 1.2 */
                                .verify_mode = 1, /* 验证服务器证书 */
                                .auth_mode   = 0, /* 单向认证 */
                                .ca_cert = NULL,  /* 使用模块内置证书 */
                                .sni     = "api.github.com" };

    at_ssl_init(dev, &ssl_config);

    /* 2. 发送HTTPS GET请求 */
    char response[4096];
    int ret =
        at_https_get(dev, "https://api.github.com", response, sizeof(response));

    if (ret > 0) {
        printf("HTTPS GET successful, received %d bytes\n", ret);
        printf("Response: %.*s\n", 200, response); /* 只显示前200个字符 */
    } else {
        printf("HTTPS GET failed\n");
    }

    /* 3. 发送HTTPS POST请求 */
    const char *post_data = "{\"test\": \"data\"}";
    ret = at_https_post(dev, "https://httpbin.org/post", "application/json",
                        post_data, response, sizeof(response));

    if (ret > 0) {
        printf("HTTPS POST successful, received %d bytes\n", ret);
    }
}

/* MQTT完整示例 */
void mqtt_complete_example(at_device_t *dev)
{
    printf("=== MQTT Complete Example ===\n");

    /* 1. 初始化MQTT配置 */
    mqtt_config_t mqtt_config = { .client_id     = "4G_Device_001",
                                  .username      = "user",
                                  .password      = "pass",
                                  .will_topic    = "device/status",
                                  .will_message  = "offline",
                                  .will_qos      = 0,
                                  .will_retain   = 1,
                                  .keep_alive    = 60,
                                  .clean_session = 1,
                                  .timeout_ms    = 30000 };

    at_mqtt_init(dev, &mqtt_config);

    /* 2. 连接MQTT服务器 */
    int ret = at_mqtt_connect(dev, "broker.emqx.io", 1883);
    if (ret == 0) {
        printf("MQTT connected successfully\n");
    } else {
        printf("MQTT connection failed\n");
        return;
    }

    /* 3. 订阅主题 */
    ret = at_mqtt_subscribe(dev, "sensor/data", 0);
    if (ret == 0) {
        printf("Subscribed to sensor/data\n");
    }

    ret = at_mqtt_subscribe(dev, "device/control", 1);
    if (ret == 0) {
        printf("Subscribed to device/control with QoS 1\n");
    }

    /* 4. 发布消息 */
    char sensor_data[256];
    for (int i = 0; i < 5; i++) {
        float temperature = 25.0 + (rand() % 100) / 10.0;
        float humidity    = 50.0 + (rand() % 100) / 10.0;

        snprintf(sensor_data, sizeof(sensor_data),
                 "{\"temp\": %.1f, \"hum\": %.1f, \"time\": %ld}", temperature,
                 humidity, time(NULL));

        ret = at_mqtt_publish(dev, "sensor/data", sensor_data, 0, 0);
        if (ret == 0) {
            printf("Published: %s\n", sensor_data);
        }

        AT_DELAY_MS(5000); /* 每5秒发布一次 */
    }

    /* 5. 接收消息 */
    char topic[128], message[256];
    while (1) {
        ret = at_mqtt_receive(
            dev, topic, sizeof(topic), message, sizeof(message), 10000);
        if (ret > 0) {
            printf("Received message on topic [%s]: %s\n", topic, message);

            /* 处理控制命令 */
            if (strcmp(topic, "device/control") == 0) {
                if (strcmp(message, "REBOOT") == 0) {
                    printf("Reboot command received\n");
                    // 执行重启
                } else if (strcmp(message, "REPORT_STATUS") == 0) {
                    printf("Status report requested\n");
                    at_mqtt_publish(dev, "device/status", "online", 0, 1);
                }
            }
        }

        /* 检查退出条件 */
        if (time(NULL) % 60 == 0) { /* 每分钟检查一次 */
            break;
        }
    }

    /* 6. 断开连接 */
    at_mqtt_disconnect(dev);
    printf("MQTT disconnected\n");
}

/* FTP文件传输示例 */
void ftp_file_example(at_device_t *dev)
{
    printf("=== FTP File Transfer Example ===\n");

    /* 1. 配置FTP */
    ftp_config_t ftp_config = {
        .server      = "ftp.example.com",
        .port        = 21,
        .username    = "user",
        .password    = "password",
        .remote_path = "/uploads/",
        .local_path  = "/sd_card/",
        .mode        = 1 /* BINARY mode */
    };

    at_ftp_init(dev, &ftp_config);

    /* 2. 连接FTP服务器 */
    int ret = at_ftp_connect(dev);
    if (ret != 0) {
        printf("FTP connection failed\n");
        return;
    }
    printf("FTP connected\n");

    /* 3. 列出远程目录 */
    char file_list[2048];
    ret = at_ftp_list(dev, "/", file_list, sizeof(file_list));
    if (ret == 0) {
        printf("Remote directory listing:\n%s\n", file_list);
    }

    /* 4. 上传文件 */
    ret = at_ftp_upload(dev, "/sd_card/data.log", "/uploads/data.log");
    if (ret == 0) {
        printf("File uploaded successfully\n");
    }

    /* 5. 下载文件 */
    ret = at_ftp_download(dev, "/uploads/config.ini", "/sd_card/config.ini");
    if (ret == 0) {
        printf("File downloaded successfully\n");
    }

    /* 6. 创建目录 */
    ret = at_ftp_mkdir(dev, "/uploads/backup");
    if (ret == 0) {
        printf("Directory created\n");
    }

    /* 7. 断开连接 */
    at_ftp_disconnect(dev);
    printf("FTP disconnected\n");
}

/* NTP时间同步示例 */
void ntp_time_sync_example(at_device_t *dev)
{
    printf("=== NTP Time Synchronization Example ===\n");

    /* 1. 初始化NTP */
    ntp_config_t ntp_config = {
        .server        = "cn.pool.ntp.org",
        .port          = 123,
        .timezone      = 8, /* 东八区 */
        .auto_sync     = true,
        .sync_interval = 3600 /* 每小时同步一次 */
    };

    at_ntp_init(dev, &ntp_config);

    /* 2. 手动同步时间 */
    int ret = at_ntp_sync(dev);
    if (ret == 0) {
        printf("NTP time sync successful\n");
    }

    /* 3. 获取网络时间 */
    struct tm timeinfo;
    ret = at_ntp_get_time(dev, &timeinfo);
    if (ret == 0) {
        printf("Network time: %04d-%02d-%02d %02d:%02d:%02d\n",
               timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
               timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    }

    /* 4. 启用自动同步 */
    at_ntp_enable_auto_sync(dev, 3600); /* 每小时自动同步 */
}

/* DNS和Ping诊断示例 */
void network_diagnostic_example(at_device_t *dev)
{
    printf("=== Network Diagnostic Example ===\n");

    /* 1. 设置DNS服务器 */
    at_dns_set_servers(dev, "8.8.8.8", "8.8.4.4");
    printf("DNS servers set\n");

    /* 2. 域名解析 */
    char ip[16];
    int ret = at_dns_resolve(dev, "www.baidu.com", ip, sizeof(ip));
    if (ret == 0) {
        printf("Resolved www.baidu.com to %s\n", ip);
    }

    /* 3. Ping测试 */
    printf("Pinging www.baidu.com...\n");
    ret = at_ping(dev, "www.baidu.com", 5000, 4);

    if (ret > 0) {
        printf("Ping successful, %d packets received\n", ret);
    } else {
        printf("Ping failed\n");
    }

    /* 4. Ping多个目标 */
    const char *targets[] = { "8.8.8.8", "114.114.114.114", "www.qq.com",
                              NULL };

    for (int i = 0; targets[i] != NULL; i++) {
        printf("\nPinging %s...\n", targets[i]);
        at_ping(dev, targets[i], 3000, 2);
    }
}

/* 邮件发送示例 */
void email_send_example(at_device_t *dev)
{
    printf("=== Email Send Example ===\n");

    int ret = at_smtp_send(dev, "smtp.163.com", 25, /* SMTP服务器和端口 */
                           "your_email@163.com", "your_password", /* 认证信息 */
                           "your_email@163.com",    /* 发件人 */
                           "recipient@example.com", /* 收件人 */
                           "Test from 4G Module",   /* 主题 */
                           "This is a test email sent from 4G module.\r\n"
                           "Time: " __DATE__ " " __TIME__); /* 正文 */

    if (ret == 0) {
        printf("Email sent successfully\n");
    } else {
        printf("Email send failed\n");
    }
}

/* WebSocket实时通信示例 */
void websocket_example(at_device_t *dev)
{
    printf("=== WebSocket Example ===\n");

    /* 连接WebSocket服务器 */
    int ret = at_websocket_connect(dev, 0, "ws://echo.websocket.org", NULL);
    if (ret == 0) {
        printf("WebSocket connected\n");

        /* 发送消息 */
        const char *message = "Hello WebSocket!";
        ret                 = at_websocket_send(
            dev, 0, (uint8_t *)message, strlen(message), 1); /* text frame */
        if (ret == 0) {
            printf("Message sent: %s\n", message);
        }

        /* 接收回显消息 */
        uint8_t buffer[256];
        ret = at_websocket_receive(dev, 0, buffer, sizeof(buffer), 5000);
        if (ret > 0) {
            buffer[ret] = '\0';
            printf("Received echo: %s\n", buffer);
        }
    }
}

/* 多协议综合应用 - 物联网网关 */
void iot_gateway_example(at_device_t *dev)
{
    printf("=== IoT Gateway Multi-Protocol Example ===\n");

    /* 1. 初始化所有协议 */
    at_ssl_init(dev, NULL);  /* HTTPS */
    at_mqtt_init(dev, NULL); /* MQTT */
    at_ntp_init(dev, NULL);  /* NTP */

    /* 2. 时间同步 */
    at_ntp_sync(dev);

    /* 3. 连接云平台 */
    int ret = at_mqtt_connect(dev, "iot-cloud.example.com", 8883);
    if (ret != 0) {
        /* 如果MQTT失败，尝试HTTPS */
        printf("MQTT connection failed, trying HTTPS...\n");

        char https_response[1024];
        at_https_post(dev, "https://iot-cloud.example.com/api/report",
                      "application/json", "{\"status\":\"online\"}",
                      https_response, sizeof(https_response));
    }

    /* 4. 定期上报数据 */
    while (1) {
        /* 采集传感器数据 */
        float sensor_data[5];
        collect_sensor_data(sensor_data);

        /* 构建JSON数据 */
        char json_data[512];
        struct tm timeinfo;
        at_ntp_get_time(dev, &timeinfo);

        snprintf(json_data, sizeof(json_data),
                 "{\"timestamp\":\"%04d-%02d-%02dT%02d:%02d:%02d\","
                 "\"temperature\":%.1f,\"humidity\":%.1f,"
                 "\"pressure\":%.1f,\"pm25\":%.1f,\"co2\":%.1f}",
                 timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                 timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,
                 sensor_data[0], sensor_data[1], sensor_data[2], sensor_data[3],
                 sensor_data[4]);

        /* 通过MQTT上报 */
        at_mqtt_publish(dev, "sensor/data", json_data, 0, 0);

        /* 同时通过HTTPS备份 */
        char backup_url[256];
        snprintf(backup_url, sizeof(backup_url),
                 "https://backup-server.example.com/api/sensor");
        char response[512];
        at_https_post(dev, backup_url, "application/json", json_data, response,
                      sizeof(response));

        /* 如果是整点，发送邮件报告 */
        if (timeinfo.tm_min == 0) {
            char email_body[1024];
            snprintf(email_body, sizeof(email_body),
                     "Hourly report:\n"
                     "Time: %02d:00\n"
                     "Temperature: %.1f°C\n"
                     "Humidity: %.1f%%\n"
                     "Data sent successfully.",
                     timeinfo.tm_hour, sensor_data[0], sensor_data[1]);

            at_smtp_send(dev, "smtp.example.com", 587, "gateway@example.com",
                         "password", "gateway@example.com", "admin@example.com",
                         "Hourly IoT Gateway Report", email_body);
        }

        /* 每小时同步一次时间 */
        if (timeinfo.tm_min == 0 && timeinfo.tm_sec < 10) {
            at_ntp_sync(dev);
        }

        /* 等待10分钟 */
        AT_DELAY_MS(600000);
    }
}

/* 远程固件升级示例 */
void remote_firmware_update(at_device_t *dev)
{
    printf("=== Remote Firmware Update Example ===\n");

    /* 1. 检查新版本 */
    char version_check_url[256];
    snprintf(version_check_url, sizeof(version_check_url),
             "https://update-server.example.com/api/check?device_id=%s",
             get_device_id());

    char version_response[256];
    at_https_get(
        dev, version_check_url, version_response, sizeof(version_response));

    /* 解析响应，检查是否有新版本 */
    char latest_version[32];
    char firmware_url[256];
    if (parse_version_response(version_response, latest_version, firmware_url,
                               sizeof(firmware_url))) {
        printf("New firmware available: %s\n", latest_version);

        /* 2. 通过FTP下载固件 */
        at_ftp_init(dev, NULL);
        at_ftp_connect(dev);

        char local_file[64];
        snprintf(local_file, sizeof(local_file), "/update/firmware_%s.bin",
                 latest_version);

        if (at_ftp_download(dev, firmware_url, local_file) == 0) {
            printf("Firmware downloaded: %s\n", local_file);

            /* 3. 验证固件完整性 */
            if (verify_firmware(local_file)) {
                printf("Firmware verified, ready to update\n");

                /* 4. 发送更新确认 */
                at_mqtt_publish(
                    dev, "device/update/status",
                    "{\"status\":\"downloaded\",\"version\":\"1.0.1\"}", 1, 0);

                /* 5. 执行更新 */
                perform_firmware_update(local_file);
            }
        }

        at_ftp_disconnect(dev);
    } else {
        printf("Firmware is up to date\n");
    }
}

/* 网络质量监测系统 */
void network_quality_monitor(at_device_t *dev)
{
    printf("=== Network Quality Monitor ===\n");

    /* 监测目标列表 */
    typedef struct {
        const char *name;
        const char *host;
        int priority;
    } monitor_target_t;

    monitor_target_t targets[] = { { "DNS Primary", "8.8.8.8", 1 },
                                   { "DNS Secondary", "8.8.4.4", 2 },
                                   { "Gateway", "192.168.1.1", 1 },
                                   { "Cloud Service", "api.example.com", 1 },
                                   { "NTP Server", "cn.pool.ntp.org", 2 },
                                   { NULL, NULL, 0 } };

    /* 定期监测 */
    while (1) {
        printf("\n=== Network Quality Report ===\n");
        printf("Time: %ld\n", time(NULL));

        for (int i = 0; targets[i].name != NULL; i++) {
            printf("\nChecking %s (%s):\n", targets[i].name, targets[i].host);

            /* 解析域名（如果是域名） */
            char ip[16] = { 0 };
            if (strchr(targets[i].host, '.') && !isdigit(targets[i].host[0])) {
                if (at_dns_resolve(dev, targets[i].host, ip, sizeof(ip)) == 0) {
                    printf("  Resolved to: %s\n", ip);
                } else {
                    printf("  DNS resolution failed\n");
                    continue;
                }
            } else {
                strcpy(ip, targets[i].host);
            }

            /* Ping测试 */
            printf("  Ping test: ");
            int received = at_ping(dev, ip, 3000, 3);

            if (received > 0) {
                printf("OK (%d/3 packets)\n", received);

                /* 如果优先级高，进行详细测试 */
                if (targets[i].priority == 1) {
                    /* TCP连接测试 */
                    printf("  TCP connection test: ");
                    if (at_4g_tcp_connect(dev, 0, ip, 80) == 0) {
                        printf("OK\n");
                        at_4g_tcp_close(dev, 0);
                    } else {
                        printf("FAILED\n");
                    }
                }
            } else {
                printf("FAILED\n");
            }
        }

        /* 生成报告并通过邮件发送 */
        /* 每6小时发送一次报告 */
        if (time(NULL) % 21600 == 0) { /* 6小时 = 21600秒 */
            send_network_report_email(dev);
        }

        /* 等待15分钟 */
        AT_DELAY_MS(900000);
    }
}
}

/* 智能家居控制系统 */
void smart_home_control(at_device_t *dev)
{
    printf("=== Smart Home Control System ===\n");

    /* 1. 初始化所有协议 */
    at_mqtt_init(dev, NULL);
    at_ssl_init(dev, NULL);

    /* 2. 连接家庭云平台 */
    mqtt_config_t mqtt_cfg = { .client_id     = "SmartHome_Gateway_001",
                               .username      = "home_user",
                               .password      = "home_pass",
                               .keep_alive    = 120,
                               .clean_session = 1 };
    at_mqtt_init(dev, &mqtt_cfg);

    if (at_mqtt_connect(dev, "home.iot-cloud.com", 8883) != 0) {
        printf("MQTT connection failed, using HTTPS fallback\n");
        // HTTPS连接作为备选
    }

    /* 3. 订阅控制主题 */
    at_mqtt_subscribe(dev, "home/livingroom/light/control", 1);
    at_mqtt_subscribe(dev, "home/bedroom/aircon/control", 1);
    at_mqtt_subscribe(dev, "home/curtain/control", 1);
    at_mqtt_subscribe(dev, "home/security/alarm", 2); /* QoS 2 for security */
    at_mqtt_subscribe(dev, "home/gateway/command", 1);

    /* 4. 主控制循环 */
    char topic[128], message[256];
    while (1) {
        /* 接收控制命令 */
        if (at_mqtt_receive(
                dev, topic, sizeof(topic), message, sizeof(message), 1000)
            > 0) {

            printf("Control command: [%s] %s\n", topic, message);

            /* 解析并执行控制命令 */
            if (strcmp(topic, "home/livingroom/light/control") == 0) {
                control_light("livingroom", message);

                /* 反馈状态 */
                char status_msg[64];
                snprintf(status_msg, sizeof(status_msg),
                         "{\"device\":\"livingroom_light\",\"status\":\"%s\"}",
                         message);
                at_mqtt_publish(
                    dev, "home/livingroom/light/status", status_msg, 1, 0);

            } else if (strcmp(topic, "home/bedroom/aircon/control") == 0) {
                control_aircon("bedroom", message);

            } else if (strcmp(topic, "home/gateway/command") == 0) {
                if (strcmp(message, "REBOOT") == 0) {
                    printf("Gateway reboot command received\n");
                    at_mqtt_publish(dev, "home/gateway/status",
                                    "{\"status\":\"rebooting\"}", 1, 1);
                    AT_DELAY_MS(1000);
                    system_reboot();

                } else if (strcmp(message, "UPDATE_TIME") == 0) {
                    at_ntp_sync(dev);

                } else if (strcmp(message, "DIAGNOSTIC") == 0) {
                    perform_diagnostic(dev);
                }
            }
        }

        /* 定期上报传感器数据 */
        static uint32_t last_report = 0;
        uint32_t current_time       = get_system_tick();
        if (current_time - last_report > 30000) { /* 每30秒 */

            /* 采集所有传感器数据 */
            float temperature    = read_temperature();
            float humidity       = read_humidity();
            bool motion_detected = check_motion();
            bool door_open       = check_door();
            bool window_open     = check_window();

            /* 构建JSON报告 */
            char sensor_report[512];
            snprintf(sensor_report, sizeof(sensor_report),
                     "{\"timestamp\":%lu,"
                     "\"temperature\":%.1f,"
                     "\"humidity\":%.1f,"
                     "\"motion\":%s,"
                     "\"door\":%s,"
                     "\"window\":%s,"
                     "\"battery\":%d}",
                     time(NULL), temperature, humidity,
                     motion_detected ? "true" : "false",
                     door_open ? "open" : "closed",
                     window_open ? "open" : "closed", get_battery_level());

            /* 发布到MQTT */
            at_mqtt_publish(dev, "home/sensors/data", sensor_report, 0, 0);

            /* 如果有异常，发送邮件报警 */
            if (temperature > 35.0 || temperature < 5.0) {
                char alert_email[256];
                snprintf(alert_email, sizeof(alert_email),
                         "Temperature alert: %.1f°C", temperature);
                at_smtp_send(dev, "smtp.gmail.com", 587,
                             "smarthome@example.com", "password",
                             "smarthome@example.com", "owner@example.com",
                             "Temperature Alert", alert_email);
            }

            last_report = current_time;
        }

        /* 检查MQTT连接状态，必要时重连 */
        if (at_mqtt_get_status(dev) != 0) { /* 连接断开 */
            printf("MQTT disconnected, reconnecting...\n");
            at_mqtt_connect(dev, "home.iot-cloud.com", 8883);
        }

        AT_DELAY_MS(100);
    }
}

/* 车辆追踪与监控系统 */
void vehicle_tracking_system(at_device_t *dev)
{
    printf("=== Vehicle Tracking System ===\n");

    /* 初始化所有需要的协议 */
    at_ssl_init(dev, NULL);  /* 用于HTTPS上报 */
    at_mqtt_init(dev, NULL); /* 用于实时位置推送 */
    at_ntp_init(dev, NULL);  /* 时间同步 */

    /* GPS初始化（如果模块支持） */
#ifdef AT_4G_WITH_GPS
    at_4g_gps_power(dev, true);
#endif

    /* 连接追踪平台 */
    mqtt_config_t mqtt_cfg = { .client_id    = get_vehicle_id(),
                               .username     = "tracking_user",
                               .password     = "tracking_pass",
                               .keep_alive   = 60,
                               .will_topic   = "vehicle/status",
                               .will_message = "offline",
                               .will_qos     = 1,
                               .will_retain  = 1 };
    at_mqtt_init(dev, &mqtt_cfg);

    at_mqtt_connect(dev, "tracking.server.com", 8883);

    /* 主追踪循环 */
    while (1) {
#ifdef AT_4G_WITH_GPS
        /* 获取GPS位置 */
        gps_info_t gps;
        if (at_4g_get_gps_info(dev, &gps) == 0) {

            /* 构建位置报告 */
            char location_report[512];
            snprintf(location_report, sizeof(location_report),
                     "{\"vehicle_id\":\"%s\","
                     "\"timestamp\":\"%s\","
                     "\"latitude\":%.6f,"
                     "\"longitude\":%.6f,"
                     "\"speed\":%.1f,"
                     "\"heading\":%.1f,"
                     "\"altitude\":%.1f,"
                     "\"satellites\":%d,"
                     "\"battery\":%d,"
                     "\"ignition\":%s}",
                     get_vehicle_id(), gps.time, gps.latitude, gps.longitude,
                     gps.speed, gps.course, gps.altitude, gps.satellites,
                     get_battery_level(), is_ignition_on() ? "on" : "off");

            /* 通过MQTT实时推送 */
            at_mqtt_publish(dev, "vehicle/location", location_report, 0, 0);

            /* 通过HTTPS批量上报（存储到数据库） */
            static int report_count = 0;
            report_count++;

            if (report_count >= 10) { /* 每10个点批量上报一次 */
                char batch_report[2048];
                // 这里应该是一个位置点数组
                at_https_post(dev, "https://tracking.server.com/api/locations",
                              "application/json", batch_report, NULL, 0);
                report_count = 0;
            }

            /* 超速报警 */
            if (gps.speed > 120.0) { /* 120 km/h */
                char speed_alert[256];
                snprintf(speed_alert, sizeof(speed_alert),
                         "Vehicle %s speeding: %.1f km/h at %s",
                         get_vehicle_id(), gps.speed, gps.time);

                /* 发送短信报警 */
                at_4g_send_sms(dev, "13800138000", speed_alert);

                /* 发送邮件报警 */
                at_smtp_send(dev, "smtp.example.com", 587,
                             "alerts@tracking.com", "password",
                             "alerts@tracking.com", "fleetmanager@example.com",
                             "Speed Alert", speed_alert);
            }

            /* 地理围栏检查 */
            if (check_geofence(gps.latitude, gps.longitude)) {
                char geofence_alert[256];
                snprintf(geofence_alert, sizeof(geofence_alert),
                         "Vehicle %s entered restricted area at %s",
                         get_vehicle_id(), gps.time);
                at_mqtt_publish(
                    dev, "vehicle/alerts/geofence", geofence_alert, 1, 0);
            }
        }
#endif

        /* 检查车辆状态 */
        check_vehicle_status(dev);

        /* 接收控制命令 */
        char topic[128], message[256];
        if (at_mqtt_receive(
                dev, topic, sizeof(topic), message, sizeof(message), 1000)
            > 0) {

            if (strcmp(topic, "vehicle/control") == 0) {
                if (strcmp(message, "LOCK") == 0) {
                    control_vehicle_lock(true);
                } else if (strcmp(message, "UNLOCK") == 0) {
                    control_vehicle_lock(false);
                } else if (strcmp(message, "ENGINE_START") == 0) {
                    remote_engine_start();
                } else if (strcmp(message, "ENGINE_STOP") == 0) {
                    remote_engine_stop();
                } else if (strcmp(message, "TRACE_ON") == 0) {
                    set_tracking_interval(10); /* 10秒上报间隔 */
                } else if (strcmp(message, "TRACE_OFF") == 0) {
                    set_tracking_interval(60); /* 60秒上报间隔 */
                }
            }
        }

        /* 根据车辆状态调整上报频率 */
        uint32_t interval =
            is_ignition_on() ? 10000 : 30000; /* 点火时10秒，熄火时30秒 */
        AT_DELAY_MS(interval);
    }
}

/* 工业物联网数据采集系统 */
void industrial_iot_collector(at_device_t *dev)
{
    printf("=== Industrial IoT Data Collector ===\n");

    /* 初始化协议 */
    at_mqtt_init(dev, NULL); /* 实时数据推送 */
    at_ftp_init(dev, NULL);  /* 历史数据备份 */
    at_ssl_init(dev, NULL);  /* 安全上报 */

    /* 配置FTP用于数据备份 */
    ftp_config_t ftp_cfg = {
        .server      = "backup.industrial.com",
        .port        = 21,
        .username    = "iiot_user",
        .password    = "iiot_pass",
        .remote_path = "/data_logs/",
        .mode        = 1 /* BINARY模式 */
    };
    at_ftp_init(dev, &ftp_cfg);

    /* 数据采集和上报循环 */
    char data_buffer[1024];
    uint32_t data_count = 0;
    time_t start_time   = time(NULL);

    while (1) {
        /* 采集工业设备数据 */
        float process_data[20]; /* 假设有20个工艺参数 */
        collect_process_data(process_data);

        /* 构建数据记录 */
        char data_record[256];
        struct tm timeinfo;
        at_ntp_get_time(dev, &timeinfo);

        snprintf(data_record, sizeof(data_record),
                 "%04d-%02d-%02d %02d:%02d:%02d,"
                 "%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,"
                 "%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",
                 timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                 timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,
                 process_data[0], process_data[1], process_data[2],
                 process_data[3], process_data[4], process_data[5],
                 process_data[6], process_data[7], process_data[8],
                 process_data[9], process_data[10], process_data[11],
                 process_data[12], process_data[13], process_data[14],
                 process_data[15], process_data[16], process_data[17],
                 process_data[18], process_data[19]);

        /* 添加到缓冲区 */
        strcat(data_buffer, data_record);
        data_count++;

        /* 实时数据通过MQTT推送（关键参数） */
        char realtime_data[256];
        snprintf(realtime_data, sizeof(realtime_data),
                 "{\"timestamp\":\"%02d:%02d:%02d\","
                 "\"temp\":%.1f,\"pressure\":%.1f,\"flow\":%.1f,"
                 "\"speed\":%.1f,\"current\":%.1f}",
                 timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,
                 process_data[0], process_data[1], process_data[2],
                 process_data[3], process_data[4]);

        at_mqtt_publish(dev, "iiot/process/realtime", realtime_data, 0, 0);

        /* 报警检查 */
        if (process_data[0] > 100.0) { /* 温度过高 */
            char alarm_msg[128];
            snprintf(alarm_msg, sizeof(alarm_msg),
                     "Temperature alarm: %.1f°C at %02d:%02d", process_data[0],
                     timeinfo.tm_hour, timeinfo.tm_min);

            /* 发送短信报警 */
            at_4g_send_sms(dev, "13900139000", alarm_msg);

            /* 发布MQTT报警 */
            at_mqtt_publish(dev, "iiot/alarms/temperature", alarm_msg, 1,
                            1); /* QoS 1, retained */
        }

        /* 每100条记录或每小时上传一次数据到FTP */
        if (data_count >= 100 || (time(NULL) - start_time) >= 3600) {
            if (data_count > 0) {
                /* 生成文件名 */
                char filename[64];
                snprintf(filename, sizeof(filename),
                         "data_%04d%02d%02d_%02d%02d.csv",
                         timeinfo.tm_year + 1900, timeinfo.tm_mon + 1,
                         timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min);

                /* 保存到本地文件 */
                FILE *fp = fopen(filename, "w");
                if (fp) {
                    fwrite(data_buffer, 1, strlen(data_buffer), fp);
                    fclose(fp);

                    /* 上传到FTP服务器 */
                    at_ftp_connect(dev);
                    at_ftp_upload(dev, filename, filename);
                    at_ftp_disconnect(dev);

                    /* 同时通过HTTPS上传到云平台 */
                    at_https_post(dev, "https://iiot.cloud.com/api/upload",
                                  "text/csv", data_buffer, NULL, 0);

                    /* 清空缓冲区 */
                    memset(data_buffer, 0, sizeof(data_buffer));
                    data_count = 0;
                }
            }
            start_time = time(NULL);
        }

        /* 等待1秒采集间隔 */
        AT_DELAY_MS(1000);
    }
}

/* 远程医疗监护系统 */
void remote_medical_monitoring(at_device_t *dev)
{
    printf("=== Remote Medical Monitoring System ===\n");

    /* 初始化安全连接 */
    ssl_config_t ssl_cfg = { .ssl_version = 2, /* TLS 1.2 */
                             .verify_mode = 1, /* 强制证书验证 */
                             .auth_mode   = 1, /* 双向认证 */
                             .ca_cert     = "medical_ca.crt",
                             .client_cert = "device_client.crt",
                             .client_key  = "device_client.key" };
    at_ssl_init(dev, &ssl_cfg);

    /* 主监控循环 */
    while (1) {
        /* 采集生理数据 */
        float heart_rate         = read_heart_rate();
        float blood_pressure_sys = read_blood_pressure_sys();
        float blood_pressure_dia = read_blood_pressure_dia();
        float blood_oxygen       = read_blood_oxygen();
        float body_temperature   = read_body_temperature();

        /* 构建医疗数据包 */
        char medical_data[512];
        struct tm timeinfo;
        at_ntp_get_time(dev, &timeinfo);

        snprintf(medical_data, sizeof(medical_data),
                 "{\"patient_id\":\"%s\","
                 "\"timestamp\":\"%04d-%02d-%02dT%02d:%02d:%02d\","
                 "\"heart_rate\":%.0f,"
                 "\"blood_pressure\":\"%.0f/%.0f\","
                 "\"blood_oxygen\":%.1f,"
                 "\"body_temperature\":%.1f,"
                 "\"ecg_data\":\"%s\"}",
                 get_patient_id(), timeinfo.tm_year + 1900, timeinfo.tm_mon + 1,
                 timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min,
                 timeinfo.tm_sec, heart_rate, blood_pressure_sys,
                 blood_pressure_dia, blood_oxygen, body_temperature,
                 get_ecg_data_sample()); /* 简化的ECG数据 */

        /* 通过HTTPS安全上报 */
        int ret = at_https_post(dev, "https://medical-cloud.com/api/vitals",
                                "application/json", medical_data, NULL, 0);

        if (ret < 0) {
            /* 如果HTTPS失败，尝试MQTT（低安全级别） */
            printf("HTTPS failed, trying MQTT fallback\n");
            at_mqtt_publish(dev, "medical/patient/vitals", medical_data, 2,
                            0); /* QoS 2保证送达 */
        }

        /* 紧急情况处理 */
        if (heart_rate < 50 || heart_rate > 150 || blood_pressure_sys > 180
            || blood_pressure_sys < 90 || blood_oxygen < 90.0) {

            char emergency_alert[256];
            snprintf(emergency_alert, sizeof(emergency_alert),
                     "EMERGENCY! Patient %s abnormal vitals: "
                     "HR=%.0f, BP=%.0f/%.0f, SpO2=%.1f",
                     get_patient_id(), heart_rate, blood_pressure_sys,
                     blood_pressure_dia, blood_oxygen);

            /* 发送紧急短信 */
            at_4g_send_sms(dev, "120", emergency_alert);
            at_4g_send_sms(dev, get_emergency_contact(), emergency_alert);

            /* 发送紧急邮件 */
            at_smtp_send(dev, "smtp.gmail.com", 587,
                         "medical-alert@hospital.com", "password",
                         "medical-alert@hospital.com", "doctor@hospital.com",
                         "MEDICAL EMERGENCY ALERT", emergency_alert);

            /* 自动拨打紧急电话（如果模块支持） */
            at_send_cmd("ATD120;", NULL, NULL, 30000);
        }

        /* 接收医生指令 */
        char topic[128], message[256];
        if (at_mqtt_receive(
                dev, topic, sizeof(topic), message, sizeof(message), 1000)
            > 0) {

            if (strcmp(topic, "medical/patient/instruction") == 0) {
                printf("Doctor instruction: %s\n", message);

                /* 执行指令，如调整设备参数等 */
                execute_medical_instruction(message);

                /* 确认接收 */
                char ack_msg[128];
                snprintf(ack_msg, sizeof(ack_msg),
                         "{\"patient_id\":\"%s\","
                         "\"instruction\":\"%s\","
                         "\"status\":\"executed\"}",
                         get_patient_id(), message);

                at_mqtt_publish(dev, "medical/patient/ack", ack_msg, 1, 0);
            }
        }

        /* 每5分钟上报一次 */
        AT_DELAY_MS(300000);
    }
}

/* 农业物联网监控系统 */
void agriculture_iot_monitor(at_device_t *dev)
{
    printf("=== Agriculture IoT Monitoring System ===\n");

    /* 初始化需要的协议 */
    at_mqtt_init(dev, NULL); /* 实时数据 */
    at_ftp_init(dev, NULL);  /* 图片上传 */

    /* 订阅控制主题 */
    at_mqtt_subscribe(dev, "farm/irrigation/control", 1);
    at_mqtt_subscribe(dev, "farm/lighting/control", 1);
    at_mqtt_subscribe(dev, "farm/ventilation/control", 1);

    /* 数据采集和控制系统 */
    while (1) {
        /* 采集环境数据 */
        float soil_moisture[6]; /* 6个土壤湿度传感器 */
        float air_temperature  = read_air_temperature();
        float air_humidity     = read_air_humidity();
        float soil_temperature = read_soil_temperature();
        float light_intensity  = read_light_intensity();
        float co2_level        = read_co2_level();

        for (int i = 0; i < 6; i++) {
            soil_moisture[i] = read_soil_moisture(i);
        }

        /* 构建农业数据报告 */
        char farm_data[1024];
        struct tm timeinfo;
        at_ntp_get_time(dev, &timeinfo);

        snprintf(farm_data, sizeof(farm_data),
                 "{\"farm_id\":\"%s\","
                 "\"timestamp\":\"%04d-%02d-%02d %02d:%02d\","
                 "\"air_temp\":%.1f,"
                 "\"air_humidity\":%.1f,"
                 "\"soil_temp\":%.1f,"
                 "\"light\":%.1f,"
                 "\"co2\":%.1f,"
                 "\"soil_moisture\":[%.1f,%.1f,%.1f,%.1f,%.1f,%.1f]}",
                 get_farm_id(), timeinfo.tm_year + 1900, timeinfo.tm_mon + 1,
                 timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min,
                 air_temperature, air_humidity, soil_temperature,
                 light_intensity, co2_level, soil_moisture[0], soil_moisture[1],
                 soil_moisture[2], soil_moisture[3], soil_moisture[4],
                 soil_moisture[5]);

        /* 发布MQTT数据 */
        at_mqtt_publish(dev, "farm/environment/data", farm_data, 0, 0);

        /* 自动灌溉控制 */
        float avg_moisture = 0;
        for (int i = 0; i < 6; i++)
            avg_moisture += soil_moisture[i];
        avg_moisture /= 6;

        if (avg_moisture < 30.0) { /* 土壤干燥 */
            printf("Soil dry (%.1f%%), starting irrigation\n", avg_moisture);
            control_irrigation(true);

            char irrigation_event[128];
            snprintf(irrigation_event, sizeof(irrigation_event),
                     "{\"farm_id\":\"%s\","
                     "\"event\":\"irrigation_start\","
                     "\"moisture\":%.1f}",
                     get_farm_id(), avg_moisture);

            at_mqtt_publish(
                dev, "farm/events/irrigation", irrigation_event, 1, 0);

        } else if (avg_moisture > 70.0) { /* 土壤过湿 */
            printf("Soil wet (%.1f%%), stopping irrigation\n", avg_moisture);
            control_irrigation(false);
        }

        /* 光照控制 */
        if (light_intensity < 1000.0 && timeinfo.tm_hour >= 18
            || timeinfo.tm_hour < 6) {
            control_grow_lights(true);
        } else {
            control_grow_lights(false);
        }

        /* 通风控制 */
        if (air_temperature > 30.0 || air_humidity > 80.0
            || co2_level > 2000.0) {
            control_ventilation(true);
        } else {
            control_ventilation(false);
        }

        /* 每天定时拍摄作物生长图片并上传 */
        if (timeinfo.tm_hour == 12 && timeinfo.tm_min == 0) { /* 中午12点 */
            take_crop_photo("/sd_card/crop_photo.jpg");

            /* 上传到FTP服务器 */
            at_ftp_connect(dev);

            char photo_name[64];
            snprintf(photo_name, sizeof(photo_name), "crop_%04d%02d%02d.jpg",
                     timeinfo.tm_year + 1900, timeinfo.tm_mon + 1,
                     timeinfo.tm_mday);

            at_ftp_upload(dev, "/sd_card/crop_photo.jpg", photo_name);
            at_ftp_disconnect(dev);

            /* 发送图片URL通知 */
            char photo_notice[256];
            snprintf(photo_notice, sizeof(photo_notice),
                     "Daily crop photo uploaded: ftp://farm.server.com/%s",
                     photo_name);

            at_smtp_send(dev, "smtp.example.com", 587, "farm@example.com",
                         "password", "farm@example.com", "farmer@example.com",
                         "Daily Crop Photo", photo_notice);
        }

        /* 接收远程控制命令 */
        char topic[128], message[256];
        if (at_mqtt_receive(
                dev, topic, sizeof(topic), message, sizeof(message), 1000)
            > 0) {

            if (strcmp(topic, "farm/irrigation/control") == 0) {
                if (strcmp(message, "ON") == 0) {
                    control_irrigation(true);
                } else if (strcmp(message, "OFF") == 0) {
                    control_irrigation(false);
                } else if (strcmp(message, "SCHEDULE") == 0) {
                    set_irrigation_schedule(message);
                }

            } else if (strcmp(topic, "farm/lighting/control") == 0) {
                control_grow_lights(strcmp(message, "ON") == 0);

            } else if (strcmp(topic, "farm/ventilation/control") == 0) {
                control_ventilation(strcmp(message, "ON") == 0);
            }
        }

        /* 每10分钟采集一次 */
        AT_DELAY_MS(600000);
    }
}

/* 统一协议管理器 - 简化多协议使用 */
typedef struct {
    at_device_t *dev;
    bool https_enabled;
    bool mqtt_enabled;
    bool ftp_enabled;
    bool ntp_enabled;
    bool ssl_enabled;

    /* 连接状态 */
    struct {
        bool mqtt_connected;
        bool ftp_connected;
        time_t last_ntp_sync;
    } status;

    /* 配置 */
    struct {
        char *mqtt_broker;
        uint16_t mqtt_port;
        char *ftp_server;
        char *ntp_server;
    } config;
} protocol_manager_t;

/* 创建协议管理器 */
protocol_manager_t *create_protocol_manager(at_device_t *dev)
{
    protocol_manager_t *mgr =
        (protocol_manager_t *)malloc(sizeof(protocol_manager_t));
    if (!mgr)
        return NULL;

    memset(mgr, 0, sizeof(protocol_manager_t));
    mgr->dev = dev;

    /* 默认启用所有协议 */
    mgr->https_enabled = true;
    mgr->mqtt_enabled  = true;
    mgr->ftp_enabled   = true;
    mgr->ntp_enabled   = true;
    mgr->ssl_enabled   = true;

    /* 默认配置 */
    mgr->config.mqtt_broker = "broker.example.com";
    mgr->config.mqtt_port   = 1883;
    mgr->config.ftp_server  = "ftp.example.com";
    mgr->config.ntp_server  = "pool.ntp.org";

    return mgr;
}

/* 初始化所有协议 */
int init_all_protocols(protocol_manager_t *mgr)
{
    int ret = 0;

    if (mgr->ssl_enabled) {
        ssl_config_t ssl_cfg = { 0 };
        ssl_cfg.ssl_version  = 2;
        ssl_cfg.verify_mode  = 1;
        ret |= at_ssl_init(mgr->dev, &ssl_cfg);
    }

    if (mgr->mqtt_enabled) {
        mqtt_config_t mqtt_cfg = { 0 };
        mqtt_cfg.client_id     = "MultiProtocol_Client";
        mqtt_cfg.keep_alive    = 60;
        mqtt_cfg.clean_session = 1;
        ret |= at_mqtt_init(mgr->dev, &mqtt_cfg);

        /* 连接MQTT服务器 */
        if (at_mqtt_connect(
                mgr->dev, mgr->config.mqtt_broker, mgr->config.mqtt_port)
            == 0) {
            mgr->status.mqtt_connected = true;
        }
    }

    if (mgr->ftp_enabled) {
        ftp_config_t ftp_cfg = { 0 };
        ftp_cfg.server       = mgr->config.ftp_server;
        ftp_cfg.port         = 21;
        ftp_cfg.username     = "anonymous";
        ftp_cfg.password     = "";
        ret |= at_ftp_init(mgr->dev, &ftp_cfg);
    }

    if (mgr->ntp_enabled) {
        ntp_config_t ntp_cfg  = { 0 };
        ntp_cfg.server        = mgr->config.ntp_server;
        ntp_cfg.timezone      = 8;
        ntp_cfg.auto_sync     = true;
        ntp_cfg.sync_interval = 3600;
        ret |= at_ntp_init(mgr->dev, &ntp_cfg);

        /* 立即同步一次时间 */
        at_ntp_sync(mgr->dev);
        mgr->status.last_ntp_sync = time(NULL);
    }

    return ret;
}

/* 智能数据上报 - 根据网络质量选择最佳协议 */
int smart_data_report(protocol_manager_t *mgr, const char *data_path,
                      const char *data, size_t data_len)
{
    /* 检查网络质量 */
    signal_info_t signal;
    at_4g_get_signal(mgr->dev, &signal);

    /* 根据信号强度和数据大小选择协议 */
    if (signal.rssi >= 20) {   /* 信号好 */
        if (data_len < 1024) { /* 小数据用MQTT */
            if (mgr->status.mqtt_connected) {
                return at_mqtt_publish(mgr->dev, data_path, data, 0, 0);
            } else {
                /* MQTT不可用，用HTTPS */
                char url[256];
                snprintf(
                    url, sizeof(url), "https://api.example.com%s", data_path);
                return at_https_post(
                    mgr->dev, url, "application/json", data, NULL, 0);
            }
        } else { /* 大数据用FTP */
            /* 保存到临时文件 */
            char temp_file[64];
            snprintf(
                temp_file, sizeof(temp_file), "/tmp/data_%ld.json", time(NULL));

            FILE *fp = fopen(temp_file, "w");
            if (fp) {
                fwrite(data, 1, data_len, fp);
                fclose(fp);

                at_ftp_connect(mgr->dev);
                int ret = at_ftp_upload(
                    mgr->dev, temp_file, strrchr(data_path, '/') + 1);
                at_ftp_disconnect(mgr->dev);

                remove(temp_file);
                return ret;
            }
        }
    } else { /* 信号差 */
        /* 使用UDP发送（如果有）或者存储到本地，等信号好再发 */
        printf("Signal poor (RSSI=%d), storing data locally\n", signal.rssi);
        store_data_locally(data_path, data, data_len);
        return 0; /* 返回成功，但实际是存储到本地 */
    }

    return -1;
}

/* 协议健康检查 */
int check_protocol_health(protocol_manager_t *mgr)
{
    int issues = 0;

    /* 检查MQTT连接 */
    if (mgr->mqtt_enabled && !mgr->status.mqtt_connected) {
        printf("MQTT disconnected, reconnecting...\n");
        if (at_mqtt_connect(
                mgr->dev, mgr->config.mqtt_broker, mgr->config.mqtt_port)
            == 0) {
            mgr->status.mqtt_connected = true;
        } else {
            issues |= 0x01;
        }
    }

    /* 检查时间同步 */
    if (mgr->ntp_enabled) {
        time_t now = time(NULL);
        if (now - mgr->status.last_ntp_sync > 86400) { /* 超过24小时 */
            printf("NTP sync outdated, resyncing...\n");
            if (at_ntp_sync(mgr->dev) == 0) {
                mgr->status.last_ntp_sync = now;
            } else {
                issues |= 0x02;
            }
        }
    }

    /* 检查网络连接 */
    char test_ip[16];
    if (at_dns_resolve(mgr->dev, "www.baidu.com", test_ip, sizeof(test_ip))
        != 0) {
        printf("DNS resolution failed\n");
        issues |= 0x04;
    }

    return issues; /* 返回问题位图 */
}

/* 示例：使用统一协议管理器 */
void unified_protocol_example(at_device_t *dev)
{
    printf("=== Unified Protocol Manager Example ===\n");

    /* 创建协议管理器 */
    protocol_manager_t *mgr = create_protocol_manager(dev);

    /* 初始化所有协议 */
    init_all_protocols(mgr);

    /* 主应用循环 */
    for (int i = 0; i < 100; i++) {
        /* 生成模拟数据 */
        char sensor_data[256];
        snprintf(sensor_data, sizeof(sensor_data),
                 "{\"id\":%d,\"value\":%.2f,\"timestamp\":%ld}", i,
                 25.0 + (rand() % 100) / 10.0, time(NULL));

        /* 智能上报数据 */
        smart_data_report(
            mgr, "/sensors/temperature", sensor_data, strlen(sensor_data));

        /* 定期健康检查 */
        if (i % 10 == 0) {
            int issues = check_protocol_health(mgr);
            if (issues) {
                printf("Protocol issues detected: 0x%02X\n", issues);

                /* 如果有严重问题，发送报警 */
                if (issues & 0x04) { /* 网络连接问题 */
                    char alert[128];
                    snprintf(alert, sizeof(alert),
                             "Network connectivity issue detected");
                    at_4g_send_sms(dev, "13800138000", alert);
                }
            }
        }

        AT_DELAY_MS(5000);
    }

    /* 清理 */
    free(mgr);
}
