/**
 * @file at_protocols.c
 * @brief 多协议实现
 */

#include "at_protocols.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* 协议私有数据 */
typedef struct {
    /* SSL/TLS状态 */
    struct {
        ssl_config_t config;
        bool initialized;
        uint8_t ssl_context_id;
    } ssl;

    /* MQTT状态 */
    struct {
        mqtt_config_t config;
        bool connected;
        uint8_t mqtt_client_id;
        uint16_t packet_id;
    } mqtt;

    /* FTP状态 */
    struct {
        ftp_config_t config;
        bool connected;
        uint8_t ftp_mode;
    } ftp;

    /* NTP状态 */
    struct {
        ntp_config_t config;
        time_t last_sync_time;
        bool time_synced;
    } ntp;

    /* Socket池 */
    socket_pool_t socket_pool;
} at_protocols_private_t;

/* ============ SSL/TLS实现 ============ */

/* SSL初始化 */
int at_ssl_init(at_device_t *dev, ssl_config_t *config)
{
    at_4g_private_t *priv = get_4g_private(dev);
    if (!priv) {
        return -1;
    }

    at_protocols_private_t *proto_priv = get_protocols_private(dev);
    if (!proto_priv) {
        /* 分配协议私有数据 */
        proto_priv =
            (at_protocols_private_t *)malloc(sizeof(at_protocols_private_t));
        if (!proto_priv) {
            return -1;
        }
        memset(proto_priv, 0, sizeof(at_protocols_private_t));
        set_protocols_private(dev, proto_priv);
    }

    /* 保存SSL配置 */
    if (config) {
        memcpy(&proto_priv->ssl.config, config, sizeof(ssl_config_t));
    } else {
        /* 使用默认配置 */
        memset(&proto_priv->ssl.config, 0, sizeof(ssl_config_t));
        proto_priv->ssl.config.ssl_version = 2; /* TLS 1.2 */
        proto_priv->ssl.config.verify_mode = 1; /* 验证服务器证书 */
    }

    char cmd[512];

    /* 配置SSL参数 */
    if (priv->type == MODULE_SIM7600) {
        /* SIM7600 SSL配置 */
        snprintf(cmd, sizeof(cmd), "AT+CSSLCFG=\"sslversion\",0,%d",
                 proto_priv->ssl.config.ssl_version);
        at_send_cmd(cmd, NULL, NULL, 2000);

        snprintf(cmd, sizeof(cmd), "AT+CSSLCFG=\"authmode\",0,%d",
                 proto_priv->ssl.config.auth_mode);
        at_send_cmd(cmd, NULL, NULL, 2000);

        if (proto_priv->ssl.config.verify_mode) {
            at_send_cmd("AT+CSSLCFG=\"verify\",0,1", NULL, NULL, 2000);
        }

        if (proto_priv->ssl.config.ca_cert) {
            /* 设置CA证书 */
            snprintf(cmd, sizeof(cmd), "AT+CSSLCFG=\"cacert\",0,\"%s\"",
                     proto_priv->ssl.config.ca_cert);
            at_send_cmd(cmd, NULL, NULL, 2000);
        }

        if (proto_priv->ssl.config.sni) {
            snprintf(cmd, sizeof(cmd), "AT+CSSLCFG=\"sni\",0,\"%s\"",
                     proto_priv->ssl.config.sni);
            at_send_cmd(cmd, NULL, NULL, 2000);
        }

    } else if (priv->type == MODULE_EC200) {
        /* EC200 SSL配置 */
        snprintf(cmd, sizeof(cmd), "AT+QSSLCFG=\"sslversion\",0,%d",
                 proto_priv->ssl.config.ssl_version);
        at_send_cmd(cmd, NULL, NULL, 2000);

        snprintf(cmd, sizeof(cmd), "AT+QSSLCFG=\"seclevel\",0,%d",
                 proto_priv->ssl.config.verify_mode);
        at_send_cmd(cmd, NULL, NULL, 2000);

        if (proto_priv->ssl.config.ca_cert) {
            /* 加载CA证书 */
            at_send_cmd("AT+QSSLCFG=\"cacert\",0,\"ca.crt\"", NULL, NULL, 2000);
        }
    }

    proto_priv->ssl.initialized    = true;
    proto_priv->ssl.ssl_context_id = 0;

    return 0;
}

/* SSL连接 */
int at_ssl_connect(at_device_t *dev, uint8_t link_id, const char *host,
                   uint16_t port)
{
    at_4g_private_t *priv = get_4g_private(dev);
    if (!priv) {
        return -1;
    }

    char cmd[256];
    at_resp_type_t resp;

    if (priv->type == MODULE_SIM7600) {
        /* SIM7600 SSL连接 */
        snprintf(cmd, sizeof(cmd), "AT+CCHSTART");
        resp = at_send_cmd(cmd, NULL, NULL, 5000);
        if (resp != AT_RESP_OK) {
            return -1;
        }

        snprintf(
            cmd, sizeof(cmd), "AT+CCHOPEN=%d,\"%s\",%d,0", link_id, host, port);
        resp = at_send_cmd(cmd, NULL, NULL, 30000);

    } else if (priv->type == MODULE_EC200) {
        /* EC200 SSL连接 */
        snprintf(cmd, sizeof(cmd), "AT+QSSLOPEN=0,%d,\"%s\",%d,0,0", link_id,
                 host, port);
        resp = at_send_cmd(cmd, NULL, NULL, 30000);
    }

    if (resp == AT_RESP_OK) {
        /* 等待连接建立 */
        for (int i = 0; i < 30; i++) {
            if (at_send_cmd("AT+CCHSTATUS", NULL, NULL, 1000) == AT_RESP_OK) {
                break;
            }
            AT_DELAY_MS(1000);
        }
        return 0;
    }

    return -1;
}

/* HTTPS GET请求 */
int at_https_get(at_device_t *dev, const char *url, char *response,
                 size_t max_len)
{
    at_4g_private_t *priv = get_4g_private(dev);
    if (!priv || !url) {
        return -1;
    }

    char cmd[512];
    at_resp_type_t resp;

    /* 提取主机名和路径 */
    char host[128] = { 0 };
    char path[256] = "/";
    if (parse_url(url, host, sizeof(host), path, sizeof(path)) != 0) {
        return -1;
    }

    /* 确定端口 */
    uint16_t port = 443;

    if (priv->type == MODULE_SIM7600) {
        /* 初始化HTTP */
        at_send_cmd("AT+CHTTPSSTART", NULL, NULL, 3000);

        /* 设置URL */
        snprintf(cmd, sizeof(cmd), "AT+CHTTPSOPSE=\"%s\",%d", host, port);
        resp = at_send_cmd(cmd, NULL, NULL, 5000);
        if (resp != AT_RESP_OK) {
            return -1;
        }

        /* 发送GET请求 */
        snprintf(cmd, sizeof(cmd), "AT+CHTTPSSEND=%d", (int)strlen(path) + 16);
        resp = at_send_cmd(cmd, NULL, NULL, 2000);
        if (resp != AT_RESP_CUSTOM) { /* 期待">"提示符 */
            return -1;
        }

        /* 发送HTTP头 */
        char http_request[512];
        snprintf(http_request, sizeof(http_request),
                 "GET %s HTTP/1.1\r\n"
                 "Host: %s\r\n"
                 "Connection: close\r\n"
                 "\r\n",
                 path, host);

        resp = at_send_cmd(http_request, NULL, NULL, 10000);

        /* 读取响应 */
        if (resp == AT_RESP_OK) {
            at_send_cmd("AT+CHTTPSSEND?,,1024", NULL, response, 10000);
        }

        /* 停止HTTP */
        at_send_cmd("AT+CHTTPSTOP", NULL, NULL, 3000);

    } else if (priv->type == MODULE_EC200) {
        /* EC200 HTTPS请求 */
        snprintf(cmd, sizeof(cmd), "AT+QHTTPGET=\"%s\"", url);
        resp = at_send_cmd(cmd, NULL, NULL, 30000);

        if (resp == AT_RESP_OK) {
            /* 等待数据就绪 */
            for (int i = 0; i < 30; i++) {
                resp = at_send_cmd("AT+QHTTPREAD", NULL, NULL, 5000);
                if (resp == AT_RESP_CUSTOM) {
                    /* 开始读取数据 */
                    // 这里需要继续读取响应体
                    break;
                }
                AT_DELAY_MS(1000);
            }
        }
    }

    return (resp == AT_RESP_OK) ? strlen(response) : -1;
}

/* ============ MQTT高级功能 ============ */

/* MQTT初始化 */
int at_mqtt_init(at_device_t *dev, mqtt_config_t *config)
{
    at_4g_private_t *priv = get_4g_private(dev);
    if (!priv) {
        return -1;
    }

    at_protocols_private_t *proto_priv = get_protocols_private(dev);
    if (!proto_priv) {
        proto_priv =
            (at_protocols_private_t *)malloc(sizeof(at_protocols_private_t));
        if (!proto_priv) {
            return -1;
        }
        memset(proto_priv, 0, sizeof(at_protocols_private_t));
        set_protocols_private(dev, proto_priv);
    }

    /* 保存MQTT配置 */
    if (config) {
        memcpy(&proto_priv->mqtt.config, config, sizeof(mqtt_config_t));
    } else {
        /* 默认配置 */
        memset(&proto_priv->mqtt.config, 0, sizeof(mqtt_config_t));
        proto_priv->mqtt.config.keep_alive    = 60;
        proto_priv->mqtt.config.clean_session = 1;
        proto_priv->mqtt.config.timeout_ms    = 30000;
    }

    proto_priv->mqtt.packet_id = 1;

    return 0;
}

/* MQTT连接 */
int at_mqtt_connect(at_device_t *dev, const char *host, uint16_t port)
{
    at_4g_private_t *priv = get_4g_private(dev);
    if (!priv || !host) {
        return -1;
    }

    at_protocols_private_t *proto_priv = get_protocols_private(dev);
    if (!proto_priv) {
        return -1;
    }

    char cmd[512];
    at_resp_type_t resp;

    if (priv->type == MODULE_SIM7600) {
        /* SIM7600 MQTT连接 */
        snprintf(cmd, sizeof(cmd), "AT+CMQTTSTART");
        resp = at_send_cmd(cmd, NULL, NULL, 5000);
        if (resp != AT_RESP_OK) {
            return -1;
        }

        /* 设置连接参数 */
        snprintf(cmd, sizeof(cmd), "AT+CMQTTACCQ=0,\"%s\"",
                 proto_priv->mqtt.config.client_id);
        resp = at_send_cmd(cmd, NULL, NULL, 2000);
        if (resp != AT_RESP_OK) {
            return -1;
        }

        /* 设置服务器地址 */
        snprintf(
            cmd, sizeof(cmd),
            "AT+CMQTTCONNECT=0,\"tcp://%s:%d\",%d,%d,\"%s\",\"%s\"", host, port,
            proto_priv->mqtt.config.keep_alive,
            proto_priv->mqtt.config.clean_session,
            proto_priv->mqtt.config.username ? proto_priv->mqtt.config.username
                                             : "",
            proto_priv->mqtt.config.password ? proto_priv->mqtt.config.password
                                             : "");
        resp = at_send_cmd(cmd, NULL, NULL, 30000);

    } else if (priv->type == MODULE_EC200) {
        /* EC200 MQTT连接 */
        snprintf(cmd, sizeof(cmd), "AT+QMTOPEN=0,\"%s\",%d", host, port);
        resp = at_send_cmd(cmd, NULL, NULL, 30000);
        if (resp != AT_RESP_OK) {
            return -1;
        }

        snprintf(
            cmd, sizeof(cmd), "AT+QMTCONN=0,\"%s\",\"%s\",\"%s\"",
            proto_priv->mqtt.config.client_id,
            proto_priv->mqtt.config.username ? proto_priv->mqtt.config.username
                                             : "",
            proto_priv->mqtt.config.password ? proto_priv->mqtt.config.password
                                             : "");
        resp = at_send_cmd(cmd, NULL, NULL, 30000);
    }

    if (resp == AT_RESP_OK) {
        proto_priv->mqtt.connected = true;

        /* 如果有遗嘱消息，设置遗嘱 */
        if (proto_priv->mqtt.config.will_topic) {
            at_mqtt_set_will(dev, proto_priv->mqtt.config.will_topic,
                             proto_priv->mqtt.config.will_message,
                             proto_priv->mqtt.config.will_qos,
                             proto_priv->mqtt.config.will_retain);
        }

        return 0;
    }

    return -1;
}

/* MQTT发布 */
int at_mqtt_publish(at_device_t *dev, const char *topic, const char *message,
                    uint8_t qos, uint8_t retain)
{
    at_4g_private_t *priv = get_4g_private(dev);
    if (!priv || !topic || !message) {
        return -1;
    }

    at_protocols_private_t *proto_priv = get_protocols_private(dev);
    if (!proto_priv || !proto_priv->mqtt.connected) {
        return -1;
    }

    char cmd[1024];
    at_resp_type_t resp;

    if (priv->type == MODULE_SIM7600) {
        /* SIM7600 MQTT发布 */
        snprintf(cmd, sizeof(cmd), "AT+CMQTTTOPIC=0,%d", (int)strlen(topic));
        resp = at_send_cmd(cmd, NULL, NULL, 2000);
        if (resp != AT_RESP_CUSTOM) {
            return -1;
        }

        /* 发送主题 */
        resp = at_send_cmd(topic, NULL, NULL, 2000);
        if (resp != AT_RESP_OK) {
            return -1;
        }

        snprintf(
            cmd, sizeof(cmd), "AT+CMQTTPAYLOAD=0,%d", (int)strlen(message));
        resp = at_send_cmd(cmd, NULL, NULL, 2000);
        if (resp != AT_RESP_CUSTOM) {
            return -1;
        }

        /* 发送消息 */
        resp = at_send_cmd(message, NULL, NULL, 2000);
        if (resp != AT_RESP_OK) {
            return -1;
        }

        snprintf(cmd, sizeof(cmd), "AT+CMQTTPUB=0,%d,%d", qos,
                 proto_priv->mqtt.packet_id++);
        resp = at_send_cmd(cmd, NULL, NULL, 5000);

    } else if (priv->type == MODULE_EC200) {
        /* EC200 MQTT发布 */
        snprintf(cmd, sizeof(cmd), "AT+QMTPUB=0,0,%d,%d,\"%s\",\"%s\"",
                 proto_priv->mqtt.packet_id++, qos, topic, message);
        resp = at_send_cmd(cmd, NULL, NULL, 5000);
    }

    if (resp == AT_RESP_OK) {
        /* 如果是QoS 1或2，等待PUBACK/PUBCOMP */
        if (qos > 0) {
            // 这里需要处理QoS确认
        }
        return 0;
    }

    return -1;
}

/* MQTT订阅 */
int at_mqtt_subscribe(at_device_t *dev, const char *topic, uint8_t qos)
{
    at_4g_private_t *priv = get_4g_private(dev);
    if (!priv || !topic) {
        return -1;
    }

    at_protocols_private_t *proto_priv = get_protocols_private(dev);
    if (!proto_priv || !proto_priv->mqtt.connected) {
        return -1;
    }

    char cmd[512];
    at_resp_type_t resp;

    if (priv->type == MODULE_SIM7600) {
        /* SIM7600 MQTT订阅 */
        snprintf(cmd, sizeof(cmd), "AT+CMQTTSUBTOPIC=0,%d,%d",
                 (int)strlen(topic), qos);
        resp = at_send_cmd(cmd, NULL, NULL, 2000);
        if (resp != AT_RESP_CUSTOM) {
            return -1;
        }

        resp = at_send_cmd(topic, NULL, NULL, 2000);
        if (resp != AT_RESP_OK) {
            return -1;
        }

        snprintf(
            cmd, sizeof(cmd), "AT+CMQTTSUB=0,%d", proto_priv->mqtt.packet_id++);
        resp = at_send_cmd(cmd, NULL, NULL, 5000);

    } else if (priv->type == MODULE_EC200) {
        /* EC200 MQTT订阅 */
        snprintf(cmd, sizeof(cmd), "AT+QMTSUB=0,%d,\"%s\",%d",
                 proto_priv->mqtt.packet_id++, topic, qos);
        resp = at_send_cmd(cmd, NULL, NULL, 5000);
    }

    return (resp == AT_RESP_OK) ? 0 : -1;
}

/* ============ FTP客户端功能 ============ */

/* FTP初始化 */
int at_ftp_init(at_device_t *dev, ftp_config_t *config)
{
    at_protocols_private_t *proto_priv = get_protocols_private(dev);
    if (!proto_priv) {
        proto_priv =
            (at_protocols_private_t *)malloc(sizeof(at_protocols_private_t));
        if (!proto_priv) {
            return -1;
        }
        memset(proto_priv, 0, sizeof(at_protocols_private_t));
        set_protocols_private(dev, proto_priv);
    }

    if (config) {
        memcpy(&proto_priv->ftp.config, config, sizeof(ftp_config_t));
    }

    return 0;
}

/* FTP连接 */
int at_ftp_connect(at_device_t *dev)
{
    at_4g_private_t *priv = get_4g_private(dev);
    if (!priv) {
        return -1;
    }

    at_protocols_private_t *proto_priv = get_protocols_private(dev);
    if (!proto_priv) {
        return -1;
    }

    char cmd[256];
    at_resp_type_t resp;

    if (priv->type == MODULE_SIM7600) {
        /* SIM7600 FTP连接 */
        /* 设置FTP模式 */
        snprintf(
            cmd, sizeof(cmd), "AT+CFTPMODE=%d", proto_priv->ftp.config.mode);
        at_send_cmd(cmd, NULL, NULL, 2000);

        /* 设置FTP类型 */
        at_send_cmd("AT+CFTPTYPE=A", NULL, NULL, 2000);

        /* 连接FTP服务器 */
        snprintf(
            cmd, sizeof(cmd), "AT+CFTPSLOGIN=\"%s\",%d,\"%s\",\"%s\"",
            proto_priv->ftp.config.server,
            proto_priv->ftp.config.port ? proto_priv->ftp.config.port : 21,
            proto_priv->ftp.config.username ? proto_priv->ftp.config.username
                                            : "anonymous",
            proto_priv->ftp.config.password ? proto_priv->ftp.config.password
                                            : "");
        resp = at_send_cmd(cmd, NULL, NULL, 30000);

    } else if (priv->type == MODULE_EC200) {
        /* EC200 FTP连接 */
        snprintf(
            cmd, sizeof(cmd), "AT+QFTPOPEN=\"%s\",%d,\"%s\",\"%s\"",
            proto_priv->ftp.config.server,
            proto_priv->ftp.config.port ? proto_priv->ftp.config.port : 21,
            proto_priv->ftp.config.username ? proto_priv->ftp.config.username
                                            : "anonymous",
            proto_priv->ftp.config.password ? proto_priv->ftp.config.password
                                            : "");
        resp = at_send_cmd(cmd, NULL, NULL, 30000);
    }

    if (resp == AT_RESP_OK) {
        proto_priv->ftp.connected = true;
        return 0;
    }

    return -1;
}

/* FTP上传文件 */
int at_ftp_upload(at_device_t *dev, const char *local_file,
                  const char *remote_file)
{
    at_4g_private_t *priv = get_4g_private(dev);
    if (!priv || !local_file || !remote_file) {
        return -1;
    }

    at_protocols_private_t *proto_priv = get_protocols_private(dev);
    if (!proto_priv || !proto_priv->ftp.connected) {
        return -1;
    }

    char cmd[512];
    at_resp_type_t resp;

    /* 获取文件大小 */
    FILE *fp = fopen(local_file, "rb");
    if (!fp) {
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    fclose(fp);

    if (priv->type == MODULE_SIM7600) {
        /* SIM7600 FTP上传 */
        /* 设置本地文件 */
        snprintf(cmd, sizeof(cmd), "AT+CFTPLOCAL=\"%s\"", local_file);
        resp = at_send_cmd(cmd, NULL, NULL, 2000);
        if (resp != AT_RESP_OK) {
            return -1;
        }

        /* 设置远程文件 */
        snprintf(cmd, sizeof(cmd), "AT+CFTPREMOTE=\"%s\"", remote_file);
        resp = at_send_cmd(cmd, NULL, NULL, 2000);
        if (resp != AT_RESP_OK) {
            return -1;
        }

        /* 设置文件大小 */
        snprintf(cmd, sizeof(cmd), "AT+CFTPSIZE=%ld", file_size);
        resp = at_send_cmd(cmd, NULL, NULL, 2000);
        if (resp != AT_RESP_OK) {
            return -1;
        }

        /* 开始上传 */
        resp = at_send_cmd("AT+CFTPPUT=1", NULL, NULL, 60000);

    } else if (priv->type == MODULE_EC200) {
        /* EC200 FTP上传 */
        snprintf(cmd, sizeof(cmd), "AT+QFTPPUT=\"%s\",\"%s\"", remote_file,
                 local_file);
        resp = at_send_cmd(cmd, NULL, NULL, 60000);
    }

    return (resp == AT_RESP_OK) ? 0 : -1;
}

/* ============ NTP时间同步 ============ */

/* NTP初始化 */
int at_ntp_init(at_device_t *dev, ntp_config_t *config)
{
    at_protocols_private_t *proto_priv = get_protocols_private(dev);
    if (!proto_priv) {
        proto_priv =
            (at_protocols_private_t *)malloc(sizeof(at_protocols_private_t));
        if (!proto_priv) {
            return -1;
        }
        memset(proto_priv, 0, sizeof(at_protocols_private_t));
        set_protocols_private(dev, proto_priv);
    }

    if (config) {
        memcpy(&proto_priv->ntp.config, config, sizeof(ntp_config_t));
    } else {
        /* 默认配置 */
        memset(&proto_priv->ntp.config, 0, sizeof(ntp_config_t));
        proto_priv->ntp.config.server        = "cn.pool.ntp.org";
        proto_priv->ntp.config.port          = 123;
        proto_priv->ntp.config.timezone      = 8; /* 东八区 */
        proto_priv->ntp.config.auto_sync     = true;
        proto_priv->ntp.config.sync_interval = 3600; /* 1小时 */
    }

    /* 设置时区 */
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "AT+CTZR=%d", proto_priv->ntp.config.timezone);
    at_send_cmd(cmd, NULL, NULL, 2000);

    return 0;
}

/* NTP时间同步 */
int at_ntp_sync(at_device_t *dev)
{
    at_4g_private_t *priv = get_4g_private(dev);
    if (!priv) {
        return -1;
    }

    at_protocols_private_t *proto_priv = get_protocols_private(dev);
    if (!proto_priv) {
        return -1;
    }

    char cmd[256];
    at_resp_type_t resp;

    if (priv->type == MODULE_SIM7600) {
        /* SIM7600 NTP同步 */
        snprintf(cmd, sizeof(cmd), "AT+CNTP=\"%s\",%d",
                 proto_priv->ntp.config.server, proto_priv->ntp.config.port);
        resp = at_send_cmd(cmd, NULL, NULL, 5000);
        if (resp != AT_RESP_OK) {
            return -1;
        }

        /* 开始同步 */
        resp = at_send_cmd("AT+CNTP", NULL, NULL, 10000);

    } else if (priv->type == MODULE_EC200) {
        /* EC200 NTP同步 */
        snprintf(cmd, sizeof(cmd), "AT+QNTP=\"%s\",%d",
                 proto_priv->ntp.config.server, proto_priv->ntp.config.port);
        resp = at_send_cmd(cmd, NULL, NULL, 10000);
    }

    if (resp == AT_RESP_OK) {
        proto_priv->ntp.last_sync_time = time(NULL);
        proto_priv->ntp.time_synced    = true;

        /* 同步后立即读取时间 */
        at_ntp_get_time(dev, NULL);

        return 0;
    }

    return -1;
}

/* 获取NTP时间 */
int at_ntp_get_time(at_device_t *dev, struct tm *timeinfo)
{
    char response[128];

    /* 获取网络时间 */
    at_resp_type_t resp = at_send_cmd("AT+CCLK?", NULL, response, 2000);
    if (resp == AT_RESP_OK) {
        /* 解析时间格式: "+CCLK: \"YY/MM/DD,HH:MM:SS+TZ\"" */
        int year, month, day, hour, minute, second, tz;
        if (sscanf(response, "+CCLK: \"%d/%d/%d,%d:%d:%d+%d\"", &year, &month,
                   &day, &hour, &minute, &second, &tz)
            == 7) {

            if (timeinfo) {
                memset(timeinfo, 0, sizeof(struct tm));
                timeinfo->tm_year =
                    year + 100; /* 年份从1900开始，2000年对应100 */
                timeinfo->tm_mon  = month - 1;
                timeinfo->tm_mday = day;
                timeinfo->tm_hour = hour;
                timeinfo->tm_min  = minute;
                timeinfo->tm_sec  = second;
            }

            return 0;
        }
    }

    return -1;
}

/* ============ DNS域名解析 ============ */

/* DNS解析 */
int at_dns_resolve(at_device_t *dev, const char *hostname, char *ip,
                   size_t ip_len)
{
    if (!dev || !hostname || !ip) {
        return -1;
    }

    char cmd[128];
    char response[256];

    snprintf(cmd, sizeof(cmd), "AT+CDNSGIP=\"%s\"", hostname);
    at_resp_type_t resp = at_send_cmd(cmd, NULL, response, 10000);

    if (resp == AT_RESP_OK) {
        /* 解析响应: "+CDNSGIP: 1,<hostname>,<ip1>,<ip2>..." */
        char *ip_start = strstr(response, hostname);
        if (ip_start) {
            ip_start += strlen(hostname) + 1; /* 跳过逗号 */

            /* 提取第一个IP地址 */
            char *comma = strchr(ip_start, ',');
            if (comma) {
                size_t len = comma - ip_start;
                if (len < ip_len) {
                    strncpy(ip, ip_start, len);
                    ip[len] = '\0';
                    return 0;
                }
            } else {
                /* 只有一个IP */
                strncpy(ip, ip_start, ip_len - 1);
                ip[ip_len - 1] = '\0';
                return 0;
            }
        }
    }

    return -1;
}

/* ============ Ping网络诊断 ============ */

/* Ping测试 */
int at_ping(at_device_t *dev, const char *host, uint32_t timeout_ms,
            uint8_t count)
{
    char cmd[128];
    char response[512];

    snprintf(cmd, sizeof(cmd), "AT+PING=\"%s\",%lu,%d", host, timeout_ms / 1000,
             count);

    at_resp_type_t resp = at_send_cmd(cmd, NULL, response, timeout_ms + 5000);

    if (resp == AT_RESP_OK) {
        /* 解析Ping结果 */
        printf("Ping results:\n%s\n", response);

        /* 提取统计信息 */
        int sent = 0, received = 0, lost = 0;
        int min_time = 0, max_time = 0, avg_time = 0;

        char *ptr = response;
        while ((ptr = strstr(ptr, "+PING:")) != NULL) {
            int seq, ttl, time;
            if (sscanf(ptr, "+PING: %d,%d,%d", &seq, &ttl, &time) == 3) {
                received++;
                if (min_time == 0 || time < min_time)
                    min_time = time;
                if (time > max_time)
                    max_time = time;
                avg_time += time;
            }
            ptr++;
        }

        if (received > 0) {
            avg_time /= received;
        }
        sent = count;
        lost = sent - received;

        printf("Sent: %d, Received: %d, Lost: %d (%.1f%%)\n", sent, received,
               lost, (lost * 100.0) / sent);
        printf(
            "Min: %dms, Max: %dms, Avg: %dms\n", min_time, max_time, avg_time);

        return received;
    }

    return -1;
}

/* ============ 邮件发送(SMTP) ============ */

/* 发送邮件 */
int at_smtp_send(at_device_t *dev, const char *server, uint16_t port,
                 const char *username, const char *password, const char *from,
                 const char *to, const char *subject, const char *body)
{
    at_4g_private_t *priv = get_4g_private(dev);
    if (!priv) {
        return -1;
    }

    char cmd[512];
    at_resp_type_t resp;

    if (priv->type == MODULE_SIM7600) {
        /* SIM7600邮件发送 */
        /* 设置邮件服务器 */
        snprintf(cmd, sizeof(cmd), "AT+ESMTPSCFG=\"server\",\"%s\",%d", server,
                 port);
        resp = at_send_cmd(cmd, NULL, NULL, 2000);
        if (resp != AT_RESP_OK) {
            return -1;
        }

        /* 设置认证信息 */
        snprintf(cmd, sizeof(cmd), "AT+ESMTPSCFG=\"auth\",\"%s\",\"%s\"",
                 username, password);
        resp = at_send_cmd(cmd, NULL, NULL, 2000);
        if (resp != AT_RESP_OK) {
            return -1;
        }

        /* 设置发件人 */
        snprintf(cmd, sizeof(cmd), "AT+ESMTPSFROM=\"%s\"", from);
        resp = at_send_cmd(cmd, NULL, NULL, 2000);
        if (resp != AT_RESP_OK) {
            return -1;
        }

        /* 设置收件人 */
        snprintf(cmd, sizeof(cmd), "AT+ESMTPSRCPT=0,0,\"%s\"", to);
        resp = at_send_cmd(cmd, NULL, NULL, 2000);
        if (resp != AT_RESP_OK) {
            return -1;
        }

        /* 设置邮件内容 */
        snprintf(cmd, sizeof(cmd), "AT+ESMTPSSUB=\"%s\"", subject);
        resp = at_send_cmd(cmd, NULL, NULL, 2000);
        if (resp != AT_RESP_OK) {
            return -1;
        }

        /* 发送邮件 */
        resp = at_send_cmd("AT+ESMTPSEND", NULL, NULL, 30000);

        /* 输入邮件正文 */
        if (resp == AT_RESP_CUSTOM) { /* 期待">"提示符 */
            char mail_body[2048];
            snprintf(mail_body, sizeof(mail_body), "%s\r\n.\r\n",
                     body); /* 以"."结束 */
            resp = at_send_cmd(mail_body, NULL, NULL, 30000);
        }

    } else if (priv->type == MODULE_EC200) {
        /* EC200邮件发送 */
        // EC200通常不支持直接的邮件发送，需要自己实现SMTP协议
        return at_smtp_send_raw(
            dev, server, port, username, password, from, to, subject, body);
    }

    return (resp == AT_RESP_OK) ? 0 : -1;
}

/* ============ WebSocket ============ */

/* WebSocket连接 */
int at_websocket_connect(at_device_t *dev, uint8_t link_id, const char *url,
                         const char *protocol)
{
    /* 目前大多数4G模块不直接支持WebSocket，
       需要自己实现WebSocket协议在TCP之上 */

    /* 先建立TCP连接 */
    char host[128], path[256];
    uint16_t port = 80;

    if (parse_url(url, host, sizeof(host), path, sizeof(path)) != 0) {
        return -1;
    }

    /* 如果是wss://，使用SSL连接 */
    if (strstr(url, "wss://")) {
        port = 443;
        at_ssl_connect(dev, link_id, host, port);
    } else {
        at_4g_tcp_connect(dev, link_id, host, port);
    }

    /* 发送WebSocket握手请求 */
    char handshake[1024];
    char key[32];
    generate_websocket_key(key, sizeof(key));

    snprintf(handshake, sizeof(handshake),
             "GET %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Upgrade: websocket\r\n"
             "Connection: Upgrade\r\n"
             "Sec-WebSocket-Key: %s\r\n"
             "Sec-WebSocket-Version: 13\r\n",
             path, host, key);

    if (protocol) {
        char protocol_header[128];
        snprintf(protocol_header, sizeof(protocol_header),
                 "Sec-WebSocket-Protocol: %s\r\n", protocol);
        strcat(handshake, protocol_header);
    }

    strcat(handshake, "\r\n");

    /* 发送握手请求 */
    at_4g_tcp_send(dev, link_id, (uint8_t *)handshake, strlen(handshake));

    /* 接收并验证握手响应 */
    char response[1024];
    // 这里需要接收HTTP响应并验证

    return 0;
}

/* 工具函数 */
static int parse_url(const char *url, char *host, size_t host_len, char *path,
                     size_t path_len)
{
    if (!url || !host || !path) {
        return -1;
    }

    const char *p = url;

    /* 跳过协议头 */
    if (strncmp(p, "http://", 7) == 0) {
        p += 7;
    } else if (strncmp(p, "https://", 8) == 0) {
        p += 8;
    } else if (strncmp(p, "ws://", 5) == 0) {
        p += 5;
    } else if (strncmp(p, "wss://", 6) == 0) {
        p += 6;
    }

    /* 提取主机名 */
    const char *slash = strchr(p, '/');
    if (slash) {
        size_t hostlen = slash - p;
        if (hostlen < host_len) {
            strncpy(host, p, hostlen);
            host[hostlen] = '\0';
        } else {
            return -1;
        }

        /* 提取路径 */
        if (strlen(slash) < path_len) {
            strcpy(path, slash);
        } else {
            return -1;
        }
    } else {
        /* 没有路径部分 */
        if (strlen(p) < host_len) {
            strcpy(host, p);
        } else {
            return -1;
        }
        strcpy(path, "/");
    }

    return 0;
}

static void generate_websocket_key(char *key, size_t key_len)
{
    const char *base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    if (key_len < 25)
        return; /* 需要至少24个字符 + 结束符 */

    /* 生成16字节的随机数据 */
    uint8_t random[16];
    for (int i = 0; i < 16; i++) {
        random[i] = rand() & 0xFF;
    }

    /* Base64编码 */
    int i = 0, j = 0;
    uint8_t char_array_3[3];
    uint8_t char_array_4[4];

    for (int k = 0; k < 16; k++) {
        char_array_3[i++] = random[k];
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4)
                              + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2)
                              + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; i < 4; i++) {
                key[j++] = base64_chars[char_array_4[i]];
            }
            i = 0;
        }
    }

    if (i) {
        for (int k = i; k < 3; k++) {
            char_array_3[k] = '\0';
        }

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] =
            ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] =
            ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (int k = 0; k < i + 1; k++) {
            key[j++] = base64_chars[char_array_4[k]];
        }

        while (i++ < 3) {
            key[j++] = '=';
        }
    }

    key[j] = '\0';
}