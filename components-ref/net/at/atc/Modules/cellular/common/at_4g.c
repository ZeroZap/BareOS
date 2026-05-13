/**
 * @file at_4g.c
 * @brief 4G模块接口实现
 */

#include "at_4g.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* 模块命令集定义 */
typedef struct {
    module_type_t type;
    const char *name;
    const char *cmd_manufacturer;
    const char *cmd_model;
    const char *cmd_version;
    const char *cmd_imei;
    const char *cmd_iccid;
    const char *cmd_imsi;
    const char *cmd_signal;
    const char *cmd_network;
    const char *cmd_operator;
    const char *cmd_apn;
    const char *cmd_ip;
    const char *cmd_sms_send;
    const char *cmd_sms_read;
    const char *cmd_sms_delete;
    const char *cmd_tcp_connect;
    const char *cmd_tcp_send;
    const char *cmd_tcp_close;
    const char *cmd_udp_connect;
    const char *cmd_udp_send;
    const char *cmd_time;
} module_command_set_t;

/* SIM7600命令集 */
static const module_command_set_t sim7600_cmds = {
    MODULE_SIM7600, "SIM7600", "AT+CGMI", /* 制造商 */
    "AT+CGMM",                            /* 型号 */
    "AT+CGMR",                            /* 版本 */
    "AT+CGSN",                            /* IMEI */
    "AT+CCID",                            /* ICCID */
    "AT+CIMI",                            /* IMSI */
    "AT+CSQ",                             /* 信号强度 */
    "AT+CREG",                            /* 网络注册 */
    "AT+COPS",                            /* 运营商 */
    "AT+CGDCONT",                         /* APN设置 */
    "AT+CGPADDR",                         /* IP地址 */
    "AT+CMGS",                            /* 发送短信 */
    "AT+CMGR",                            /* 读取短信 */
    "AT+CMGD",                            /* 删除短信 */
    "AT+CIPSTART",                        /* TCP连接 */
    "AT+CIPSEND",                         /* TCP发送 */
    "AT+CIPCLOSE",                        /* TCP关闭 */
    "AT+CIPSTART",                        /* UDP连接 */
    "AT+CIPSEND",                         /* UDP发送 */
    "AT+CCLK",                            /* 网络时间 */
};

/* EC200命令集 */
static const module_command_set_t ec200_cmds = {
    MODULE_EC200, "EC200",      "AT+CGMI",    "AT+CGMM",   "AT+CGMR",
    "AT+CGSN",    "AT+CCID",    "AT+CIMI",    "AT+CSQ",    "AT+CREG",
    "AT+COPS",    "AT+CGDCONT", "AT+CGPADDR", "AT+CMGS",   "AT+CMGR",
    "AT+CMGD",    "AT+QIOPEN", /* Quectel专用 */
    "AT+QISEND",  "AT+QICLOSE", "AT+QIOPEN",  "AT+QISEND", "AT+CCLK",
};

/* 模块命令集表 */
static const module_command_set_t *module_cmd_sets[] = { &sim7600_cmds,
                                                         &ec200_cmds, NULL };

/* 4G模块私有数据 */
typedef struct {
    module_type_t type;
    const module_command_set_t *cmds;
    bool pdp_activated;
    uint8_t pdp_cid;
    char apn[64];
    char ip_addr[16];
    uint32_t data_tx_bytes;
    uint32_t data_rx_bytes;
} at_4g_private_t;

/* 响应处理器 */
static bool resp_handler_signal(at_device_t *dev, const char *resp_line,
                                void *user_data, at_resp_type_t *type);
static bool resp_handler_network(at_device_t *dev, const char *resp_line,
                                 void *user_data, at_resp_type_t *type);
static bool resp_handler_operator(at_device_t *dev, const char *resp_line,
                                  void *user_data, at_resp_type_t *type);
static bool resp_handler_iccid(at_device_t *dev, const char *resp_line,
                               void *user_data, at_resp_type_t *type);
static bool resp_handler_ip(at_device_t *dev, const char *resp_line,
                            void *user_data, at_resp_type_t *type);

/* URC处理器 */
static void urc_network_status(at_device_t *dev, const char *data,
                               void *user_data);
static void urc_sms_notification(at_device_t *dev, const char *data,
                                 void *user_data);
static void urc_socket_data(at_device_t *dev, const char *data,
                            void *user_data);
static void urc_socket_closed(at_device_t *dev, const char *data,
                              void *user_data);

/* 工具函数 */
static at_4g_private_t *get_4g_private(at_device_t *dev);
static const module_command_set_t *detect_module_type(at_device_t *dev);
static int send_at_command(at_device_t *dev, const char *cmd,
                           at_resp_handler_t handler, void *user_data,
                           uint32_t timeout);

/* 初始化4G模块 */
int at_4g_init(at_device_t *dev, module_type_t type)
{
    if (!dev) {
        return -1;
    }

    /* 分配私有数据 */
    at_4g_private_t *priv = (at_4g_private_t *)malloc(sizeof(at_4g_private_t));
    if (!priv) {
        return -1;
    }

    memset(priv, 0, sizeof(at_4g_private_t));

    /* 如果未指定类型，则自动检测 */
    if (type == MODULE_UNKNOWN) {
        priv->type = at_4g_detect_module(dev);
        priv->cmds = detect_module_type(dev);
    } else {
        priv->type = type;
        /* 根据类型选择命令集 */
        for (int i = 0; module_cmd_sets[i] != NULL; i++) {
            if (module_cmd_sets[i]->type == type) {
                priv->cmds = module_cmd_sets[i];
                break;
            }
        }
    }

    if (!priv->cmds) {
        free(priv);
        return -1;
    }

    /* 将私有数据挂接到设备 */
    dev->user_data = priv;

    /* 注册URC处理器 */
    at_4g_register_urc_handlers(dev);

    /* 发送基本AT命令测试 */
    at_send_cmd("AT", NULL, NULL, 1000);

    /* 关闭回显 */
    at_send_cmd("ATE0", NULL, NULL, 1000);

    printf("4G Module %s initialized\n", priv->cmds->name);
    return 0;
}

/* 获取模块基本信息 */
int at_4g_get_module_info(at_device_t *dev, char *manufacturer, char *model,
                          char *version, char *imei)
{
    at_4g_private_t *priv = get_4g_private(dev);
    if (!priv || !priv->cmds) {
        return -1;
    }

    int ret = 0;
    char response[256];

    /* 获取制造商 */
    if (manufacturer) {
        memset(response, 0, sizeof(response));
        if (send_at_command(
                dev, priv->cmds->cmd_manufacturer, NULL, response, 2000)
            == AT_RESP_OK) {
            strncpy(manufacturer, response, 63);
        } else {
            ret = -1;
        }
    }

    /* 获取型号 */
    if (model) {
        memset(response, 0, sizeof(response));
        if (send_at_command(dev, priv->cmds->cmd_model, NULL, response, 2000)
            == AT_RESP_OK) {
            strncpy(model, response, 63);
        } else {
            ret = -1;
        }
    }

    /* 获取版本 */
    if (version) {
        memset(response, 0, sizeof(response));
        if (send_at_command(dev, priv->cmds->cmd_version, NULL, response, 2000)
            == AT_RESP_OK) {
            strncpy(version, response, 63);
        } else {
            ret = -1;
        }
    }

    /* 获取IMEI */
    if (imei) {
        memset(response, 0, sizeof(response));
        if (send_at_command(dev, priv->cmds->cmd_imei, NULL, response, 2000)
            == AT_RESP_OK) {
            strncpy(imei, response, 31);
        } else {
            ret = -1;
        }
    }

    return ret;
}

/* 检查SIM卡状态 */
sim_status_t at_4g_check_sim(at_device_t *dev)
{
    char response[128];

    if (send_at_command(dev, "AT+CPIN?", NULL, response, 3000) != AT_RESP_OK) {
        return SIM_ERROR;
    }

    if (strstr(response, "READY")) {
        return SIM_READY;
    } else if (strstr(response, "SIM PIN")) {
        return SIM_PIN_REQUIRED;
    } else if (strstr(response, "SIM PUK")) {
        return SIM_PUK_REQUIRED;
    } else if (strstr(response, "NOT INSERTED")) {
        return SIM_NOT_INSERTED;
    }

    return SIM_UNKNOWN;
}

/* 获取信号强度 */
int at_4g_get_signal(at_device_t *dev, signal_info_t *signal)
{
    if (!signal) {
        return -1;
    }

    at_4g_private_t *priv = get_4g_private(dev);
    if (!priv || !priv->cmds) {
        return -1;
    }

    signal_info_t info = { 0 };

    at_resp_type_t resp = send_at_command(
        dev, priv->cmds->cmd_signal, resp_handler_signal, &info, 2000);

    if (resp == AT_RESP_OK) {
        memcpy(signal, &info, sizeof(signal_info_t));
        return 0;
    }

    return -1;
}

/* 获取网络注册状态 */
int at_4g_get_network_status(at_device_t *dev, network_info_t *netinfo)
{
    if (!netinfo) {
        return -1;
    }

    at_4g_private_t *priv = get_4g_private(dev);
    if (!priv || !priv->cmds) {
        return -1;
    }

    network_info_t info = { 0 };

    at_resp_type_t resp = send_at_command(
        dev, priv->cmds->cmd_network, resp_handler_network, &info, 3000);

    if (resp == AT_RESP_OK) {
        memcpy(netinfo, &info, sizeof(network_info_t));
        return 0;
    }

    return -1;
}

/* 激活PDP上下文 */
int at_4g_activate_pdp(at_device_t *dev, const char *apn, const char *user,
                       const char *password)
{
    at_4g_private_t *priv = get_4g_private(dev);
    if (!priv || !priv->cmds) {
        return -1;
    }

    char cmd[128];

    /* 先设置APN */
    if (priv->type == MODULE_SIM7600) {
        snprintf(cmd, sizeof(cmd), "AT+CGDCONT=1,\"IP\",\"%s\"", apn);
    } else if (priv->type == MODULE_EC200) {
        snprintf(cmd, sizeof(cmd), "AT+QICSGP=1,1,\"%s\",\"%s\",\"%s\",1", apn,
                 user ? user : "", password ? password : "");
    }

    if (send_at_command(dev, cmd, NULL, NULL, 3000) != AT_RESP_OK) {
        return -1;
    }

    /* 激活PDP上下文 */
    if (priv->type == MODULE_SIM7600) {
        strcpy(cmd, "AT+CGACT=1,1");
    } else if (priv->type == MODULE_EC200) {
        strcpy(cmd, "AT+QIACT=1");
    }

    if (send_at_command(dev, cmd, NULL, NULL, 10000) != AT_RESP_OK) {
        return -1;
    }

    /* 保存APN信息 */
    strncpy(priv->apn, apn, sizeof(priv->apn) - 1);
    priv->pdp_activated = true;
    priv->pdp_cid       = 1;

    return 0;
}

/* 获取IP地址 */
int at_4g_get_ip_address(at_device_t *dev, char *ip, size_t len)
{
    at_4g_private_t *priv = get_4g_private(dev);
    if (!priv || !priv->cmds || !ip) {
        return -1;
    }

    char response[64];

    if (send_at_command(
            dev, priv->cmds->cmd_ip, resp_handler_ip, response, 3000)
        != AT_RESP_OK) {
        return -1;
    }

    strncpy(ip, response, len - 1);
    ip[len - 1] = '\0';

    /* 保存到私有数据 */
    strncpy(priv->ip_addr, response, sizeof(priv->ip_addr) - 1);

    return 0;
}

/* 发送短信 */
int at_4g_send_sms(at_device_t *dev, const char *number, const char *message)
{
    at_4g_private_t *priv = get_4g_private(dev);
    if (!priv || !priv->cmds || !number || !message) {
        return -1;
    }

    char cmd[64];
    char final_cmd[512];

    /* 设置短信文本模式 */
    if (send_at_command(dev, "AT+CMGF=1", NULL, NULL, 1000) != AT_RESP_OK) {
        return -1;
    }

    /* 构建发送命令 */
    snprintf(cmd, sizeof(cmd), "AT+CMGS=\"%s\"", number);

    /* 先发送AT+CMGS命令 */
    at_resp_type_t resp = send_at_command(dev, cmd, NULL, NULL, 3000);
    if (resp != AT_RESP_CUSTOM) { /* 期待">"提示符 */
        return -1;
    }

    /* 发送短信内容 */
    snprintf(final_cmd, sizeof(final_cmd), "%s%c", message, 0x1A);
    resp = at_send_cmd(final_cmd, NULL, NULL, 10000);

    return (resp == AT_RESP_OK) ? 0 : -1;
}

/* 建立TCP连接 */
int at_4g_tcp_connect(at_device_t *dev, uint8_t link_id, const char *host,
                      uint16_t port)
{
    at_4g_private_t *priv = get_4g_private(dev);
    if (!priv || !priv->cmds || !host) {
        return -1;
    }

    char cmd[128];

    if (priv->type == MODULE_SIM7600) {
        snprintf(cmd, sizeof(cmd), "AT+CIPSTART=%d,\"TCP\",\"%s\",%d", link_id,
                 host, port);
    } else if (priv->type == MODULE_EC200) {
        snprintf(cmd, sizeof(cmd), "AT+QIOPEN=1,%d,\"TCP\",\"%s\",%d,0,1",
                 link_id, host, port);
    }

    at_resp_type_t resp = send_at_command(dev, cmd, NULL, NULL, 30000);

    if (resp == AT_RESP_OK) {
        /* 等待连接建立 */
        for (int i = 0; i < 30; i++) {
            if (send_at_command(dev, "AT+CIPSTATUS", NULL, NULL, 1000)
                == AT_RESP_OK) {
                // 检查状态
                break;
            }
            AT_DELAY_MS(1000);
        }
        return 0;
    }

    return -1;
}

/* 发送TCP数据 */
int at_4g_tcp_send(at_device_t *dev, uint8_t link_id, const uint8_t *data,
                   uint32_t len)
{
    at_4g_private_t *priv = get_4g_private(dev);
    if (!priv || !priv->cmds || !data || len == 0) {
        return -1;
    }

    char cmd[64];
    char send_cmd[256];

    if (priv->type == MODULE_SIM7600) {
        snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%d,%d", link_id, len);

        /* 发送数据长度命令 */
        if (send_at_command(dev, cmd, NULL, NULL, 1000) != AT_RESP_CUSTOM) {
            return -1;
        }

        /* 发送实际数据 */
        memcpy(send_cmd, data, len);
        send_cmd[len] = '\0';
        if (at_send_cmd(send_cmd, NULL, NULL, 10000) != AT_RESP_OK) {
            return -1;
        }

    } else if (priv->type == MODULE_EC200) {
        snprintf(cmd, sizeof(cmd), "AT+QISEND=%d,%d", link_id, len);

        if (send_at_command(dev, cmd, NULL, NULL, 1000) != AT_RESP_CUSTOM) {
            return -1;
        }

        memcpy(send_cmd, data, len);
        send_cmd[len] = '\0';
        if (at_send_cmd(send_cmd, NULL, NULL, 10000) != AT_RESP_OK) {
            return -1;
        }
    }

    /* 更新发送字节统计 */
    priv->data_tx_bytes += len;

    return len;
}

/* 响应处理器实现 */
static bool resp_handler_signal(at_device_t *dev, const char *resp_line,
                                void *user_data, at_resp_type_t *type)
{
    signal_info_t *info = (signal_info_t *)user_data;

    if (strstr(resp_line, "+CSQ:")) {
        int rssi = 0, ber = 0;
        if (sscanf(resp_line, "+CSQ: %d,%d", &rssi, &ber) == 2) {
            info->rssi = rssi;
            info->ber  = ber;
        }
        return false;
    }

    if (strstr(resp_line, "OK")) {
        *type = AT_RESP_OK;
        return true;
    }

    return false;
}

static bool resp_handler_network(at_device_t *dev, const char *resp_line,
                                 void *user_data, at_resp_type_t *type)
{
    network_info_t *info = (network_info_t *)user_data;

    if (strstr(resp_line, "+CREG:")) {
        int n, stat;
        if (sscanf(resp_line, "+CREG: %d,%d", &n, &stat) == 2) {
            info->status = (net_reg_status_t)stat;
        }

        /* 如果还有LAC和CI信息 */
        char lac[8], ci[16];
        if (sscanf(
                resp_line, "+CREG: %*d,%*d,\"%7[^\"]\",\"%15[^\"]\"", lac, ci)
            == 2) {
            strncpy(info->lac, lac, sizeof(info->lac) - 1);
            strncpy(info->ci, ci, sizeof(info->ci) - 1);
        }
        return false;
    }

    if (strstr(resp_line, "OK")) {
        *type = AT_RESP_OK;
        return true;
    }

    return false;
}

static bool resp_handler_ip(at_device_t *dev, const char *resp_line,
                            void *user_data, at_resp_type_t *type)
{
    char *response = (char *)user_data;

    if (strstr(resp_line, "+CGPADDR:")) {
        int cid;
        char ip[16];
        if (sscanf(resp_line, "+CGPADDR: %d,\"%15[^\"]\"", &cid, ip) == 2) {
            strncpy(response, ip, 15);
        }
        return false;
    }

    if (strstr(resp_line, "OK")) {
        *type = AT_RESP_OK;
        return true;
    }

    return false;
}

/* 注册4G模块通用URC处理器 */
int at_4g_register_urc_handlers(at_device_t *dev)
{
    /* 网络状态变化 */
    at_urc_register(dev, "+CREG:", URC_MATCH_PREFIX, urc_network_status, dev);

    /* 短信通知 */
    at_urc_register(dev, "+CMTI:", URC_MATCH_PREFIX, urc_sms_notification, dev);

    /* TCP/UDP数据接收 */
    at_urc_register(dev, "+IPD", URC_MATCH_PREFIX, urc_socket_data, dev);
    at_urc_register(dev, "+RECEIVE", URC_MATCH_PREFIX, urc_socket_data, dev);

    /* 连接关闭 */
    at_urc_register(dev, "CLOSED", URC_MATCH_CONTAIN, urc_socket_closed, dev);

    /* 其他常用URC */
    at_urc_register(dev, "+CSCON:", URC_MATCH_PREFIX, urc_network_status, dev);
    at_urc_register(
        dev, "+PSBEARER:", URC_MATCH_PREFIX, urc_network_status, dev);
    at_urc_register(dev, "+CTZV:", URC_MATCH_PREFIX, urc_network_status, dev);

    return 0;
}

/* URC处理器实现 */
static void urc_network_status(at_device_t *dev, const char *data,
                               void *user_data)
{
    printf("Network URC: %s\n", data);

    /* 可以在这里触发网络状态更新 */
    network_info_t netinfo;
    if (at_4g_get_network_status(dev, &netinfo) == 0) {
        // 通知应用层网络状态变化
    }
}

static void urc_sms_notification(at_device_t *dev, const char *data,
                                 void *user_data)
{
    printf("SMS URC: %s\n", data);

    /* 解析短信索引并读取 */
    int index = 0;
    if (sscanf(data, "+CMTI: \"SM\",%d", &index) == 1) {
        // 可以在这里触发短信读取
        sms_info_t sms;
        if (at_4g_read_sms(dev, index, &sms) == 0) {
            printf("New SMS from %s: %s\n", sms.number, sms.content);
        }
    }
}

static void urc_socket_data(at_device_t *dev, const char *data, void *user_data)
{
    printf("Socket Data URC: %s\n", data);

    /* 解析数据长度和连接ID */
    int link_id, len;
    if (sscanf(data, "+IPD,%d,%d:", &link_id, &len) == 2) {
        // 读取后续的数据
        // 注意：这里需要从串口读取len字节的数据
    }
}

/* 工具函数 */
static at_4g_private_t *get_4g_private(at_device_t *dev)
{
    return (at_4g_private_t *)dev->user_data;
}

static const module_command_set_t *detect_module_type(at_device_t *dev)
{
    char response[128];

    /* 尝试获取制造商信息 */
    if (send_at_command(dev, "AT+CGMI", NULL, response, 2000) == AT_RESP_OK) {
        if (strstr(response, "SIMCOM")) {
            return &sim7600_cmds;
        } else if (strstr(response, "Quectel")) {
            return &ec200_cmds;
        }
    }

    return &sim7600_cmds; /* 默认返回SIM7600 */
}

static int send_at_command(at_device_t *dev, const char *cmd,
                           at_resp_handler_t handler, void *user_data,
                           uint32_t timeout)
{
    return at_send_command(dev, cmd, handler, user_data, timeout);
}

/* 检测模块类型 */
module_type_t at_4g_detect_module(at_device_t *dev)
{
    char manufacturer[64] = { 0 };

    if (at_4g_get_module_info(dev, manufacturer, NULL, NULL, NULL) == 0) {
        if (strstr(manufacturer, "SIMCOM")) {
            return MODULE_SIM7600;
        } else if (strstr(manufacturer, "Quectel")) {
            return MODULE_EC200;
        }
    }

    return MODULE_UNKNOWN;
}
```


    c

``` c
/**
 * @file at_4g_advanced.c
 * @brief 4G模块高级功能实现
 */

#include "at_4g.h"

    /* HTTP客户端实现 */
    typedef struct {
    uint8_t context_id;
    uint8_t http_client_id;
    bool initialized;
} http_client_t;

/* MQTT客户端实现 */
typedef struct {
    uint8_t client_id;
    bool connected;
    char client_name[64];
    uint16_t keep_alive;
} mqtt_client_t;

/* HTTP功能 */
int at_4g_http_init(at_device_t *dev)
{
    at_4g_private_t *priv = get_4g_private(dev);
    if (!priv) {
        return -1;
    }

    /* SIM7600 HTTP初始化 */
    if (priv->type == MODULE_SIM7600) {
        /* 设置HTTP参数 */
        if (send_at_command(dev, "AT+HTTPINIT", NULL, NULL, 3000)
            != AT_RESP_OK) {
            return -1;
        }

        if (send_at_command(dev, "AT+HTTPPARA=\"CID\",1", NULL, NULL, 3000)
            != AT_RESP_OK) {
            return -1;
        }

    } else if (priv->type == MODULE_EC200) {
        /* EC200 HTTP初始化 */
        if (send_at_command(
                dev, "AT+QHTTPCFG=\"contextid\",1", NULL, NULL, 3000)
            != AT_RESP_OK) {
            return -1;
        }
    }

    return 0;
}

int at_4g_http_get(at_device_t *dev, const char *url, char *response,
                   size_t max_len)
{
    at_4g_private_t *priv = get_4g_private(dev);
    if (!priv || !url) {
        return -1;
    }

    char cmd[256];

    if (priv->type == MODULE_SIM7600) {
        /* 设置URL */
        snprintf(cmd, sizeof(cmd), "AT+HTTPPARA=\"URL\",\"%s\"", url);
        if (send_at_command(dev, cmd, NULL, NULL, 3000) != AT_RESP_OK) {
            return -1;
        }

        /* 执行GET请求 */
        if (send_at_command(dev, "AT+HTTPACTION=0", NULL, NULL, 15000)
            != AT_RESP_OK) {
            return -1;
        }

        /* 读取响应 */
        if (send_at_command(dev, "AT+HTTPREAD", NULL, response, 10000)
            != AT_RESP_OK) {
            return -1;
        }

    } else if (priv->type == MODULE_EC200) {
        /* EC200 GET请求 */
        snprintf(cmd, sizeof(cmd), "AT+QHTTPGET=\"%s\"", url);
        if (send_at_command(dev, cmd, NULL, NULL, 30000) != AT_RESP_OK) {
            return -1;
        }

        /* 等待数据就绪 */
        for (int i = 0; i < 30; i++) {
            if (send_at_command(dev, "AT+QHTTPREAD", NULL, NULL, 1000)
                == AT_RESP_CUSTOM) {
                // 读取数据
                break;
            }
            AT_DELAY_MS(1000);
        }
    }

    return strlen(response);
}

/* MQTT功能 */
int at_4g_mqtt_connect(at_device_t *dev, const char *host, uint16_t port,
                       const char *client_id, const char *username,
                       const char *password)
{
    at_4g_private_t *priv = get_4g_private(dev);
    if (!priv || !host || !client_id) {
        return -1;
    }

    char cmd[256];

    if (priv->type == MODULE_SIM7600) {
        /* 设置MQTT参数 */
        snprintf(cmd, sizeof(cmd), "AT+MCONFIG=\"%s\",\"%s\",\"%s\"", client_id,
                 username ? username : "", password ? password : "");
        if (send_at_command(dev, cmd, NULL, NULL, 3000) != AT_RESP_OK) {
            return -1;
        }

        /* 连接MQTT服务器 */
        snprintf(cmd, sizeof(cmd), "AT+MCONNECT=\"%s\",%d,1", host, port);
        if (send_at_command(dev, cmd, NULL, NULL, 30000) != AT_RESP_OK) {
            return -1;
        }

    } else if (priv->type == MODULE_EC200) {
        /* EC200 MQTT连接 */
        snprintf(cmd, sizeof(cmd), "AT+QMTOPEN=0,\"%s\",%d", host, port);
        if (send_at_command(dev, cmd, NULL, NULL, 30000) != AT_RESP_OK) {
            return -1;
        }

        snprintf(cmd, sizeof(cmd), "AT+QMTCONN=0,\"%s\"", client_id);
        if (send_at_command(dev, cmd, NULL, NULL, 30000) != AT_RESP_OK) {
            return -1;
        }
    }

    return 0;
}

int at_4g_mqtt_publish(at_device_t *dev, const char *topic, const char *message,
                       uint8_t qos)
{
    at_4g_private_t *priv = get_4g_private(dev);
    if (!priv || !topic || !message) {
        return -1;
    }

    char cmd[512];

    if (priv->type == MODULE_SIM7600) {
        snprintf(cmd, sizeof(cmd), "AT+MPUB=\"%s\",%d,1,\"%s\"", topic, qos,
                 message);
    } else if (priv->type == MODULE_EC200) {
        snprintf(cmd, sizeof(cmd), "AT+QMTPUB=0,0,%d,0,\"%s\",\"%s\"", qos,
                 topic, message);
    }

    if (send_at_command(dev, cmd, NULL, NULL, 10000) != AT_RESP_OK) {
        return -1;
    }

    return 0;
}

/* GPS功能（如果模块支持） */
#ifdef AT_4G_WITH_GPS
int at_4g_gps_power(at_device_t *dev, bool enable)
{
    char cmd[32];

    if (enable) {
        strcpy(cmd, "AT+CGPS=1,1");
    } else {
        strcpy(cmd, "AT+CGPS=0");
    }

    if (send_at_command(dev, cmd, NULL, NULL, 3000) != AT_RESP_OK) {
        return -1;
    }

    return 0;
}

int at_4g_get_gps_info(at_device_t *dev, gps_info_t *gps)
{
    char response[256];

    /* 获取GPS数据 */
    if (send_at_command(dev, "AT+CGPSINFO", NULL, response, 3000)
        != AT_RESP_OK) {
        return -1;
    }

    /* 解析GPS数据格式：+CGPSINFO:
     * lat,lat_dir,lon,lon_dir,date,utc_time,alt,spd,cog,sat */
    char lat[16], lat_dir[2], lon[16], lon_dir[2];
    char date[16], time[16];
    float alt, spd, cog;
    int sat;

    if (sscanf(response,
               "+CGPSINFO: "
               "%15[^,],%1[^,],%15[^,],%1[^,],%15[^,],%15[^,],%f,%f,%f,%d",
               lat, lat_dir, lon, lon_dir, date, time, &alt, &spd, &cog, &sat)
        == 10) {

        /* 转换坐标格式 */
        gps->latitude   = convert_gps_coordinate(lat, lat_dir[0]);
        gps->longitude  = convert_gps_coordinate(lon, lon_dir[0]);
        gps->altitude   = alt;
        gps->speed      = spd;
        gps->course     = cog;
        gps->satellites = sat;
        snprintf(gps->time, sizeof(gps->time), "%s %s", date, time);

        return 0;
    }

    return -1;
}

static double convert_gps_coordinate(const char *coord, char dir)
{
    /* 将ddmm.mmmm格式转换为度 */
    double degrees = 0.0;
    double minutes = 0.0;
    int dd, mm;
    double mmmm;

    if (sscanf(coord, "%2d%2d%lf", &dd, &mm, &mmmm) == 3) {
        minutes = mm + mmmm / 10000.0;
        degrees = dd + minutes / 60.0;

        if (dir == 'S' || dir == 'W') {
            degrees = -degrees;
        }
    }

    return degrees;
}
#endif