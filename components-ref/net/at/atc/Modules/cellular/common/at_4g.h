/**
 * @file at_4g.h
 * @brief 4G模块专用AT命令接口
 */

#ifndef AT_4G_H
#define AT_4G_H

#include "at_client.h"
#include "at_urc.h"

/* 4G模块类型 */
typedef enum {
    MODULE_SIM7600 = 0,
    MODULE_EC200,
    MODULE_BG96,
    MODULE_ME909s,
    MODULE_UNKNOWN
} module_type_t;

/* 网络注册状态 */
typedef enum {
    NET_NOT_REGISTERED = 0,
    NET_REGISTERED_HOME,
    NET_SEARCHING,
    NET_REGISTRATION_DENIED,
    NET_UNKNOWN,
    NET_REGISTERED_ROAMING
} net_reg_status_t;

/* 网络模式 */
typedef enum {
    NET_MODE_NO_SERVICE = 0,
    NET_MODE_GSM,
    NET_MODE_EGSM,
    NET_MODE_WCDMA,
    NET_MODE_LTE,
    NET_MODE_NB_IOT,
    NET_MODE_5G
} net_mode_t;

/* SIM卡状态 */
typedef enum {
    SIM_UNKNOWN = 0,
    SIM_READY,
    SIM_PIN_REQUIRED,
    SIM_PUK_REQUIRED,
    SIM_NOT_INSERTED,
    SIM_ERROR
} sim_status_t;

/* 信号强度信息 */
typedef struct {
    int rssi; /* 信号强度 0-31, 99为未知 */
    int ber;  /* 误码率 0-7, 99为未知 */
    int rsrp; /* LTE参考信号接收功率 */
    int rsrq; /* LTE参考信号接收质量 */
    int sinr; /* 信噪比 */
} signal_info_t;

/* 网络信息 */
typedef struct {
    net_reg_status_t status;
    net_mode_t mode;
    char lac[8];    /* 位置区码 */
    char ci[16];    /* 小区ID */
    uint16_t arfcn; /* 绝对频点号 */
    uint8_t band;   /* 频段 */
} network_info_t;

/* SIM卡信息 */
typedef struct {
    sim_status_t status;
    char iccid[32]; /* ICCID */
    char imsi[32];  /* IMSI */
    char imei[32];  /* IMEI */
} sim_info_t;

/* PDP上下文信息 */
typedef struct {
    uint8_t cid;   /* 上下文ID */
    char apn[64];  /* APN */
    char ip[16];   /* IP地址 */
    uint8_t state; /* 状态 0-未激活 1-已激活 */
} pdp_context_t;

/* 短信信息 */
typedef struct {
    uint8_t index;      /* 短信索引 */
    char number[32];    /* 发送方号码 */
    char timestamp[32]; /* 时间戳 */
    char content[256];  /* 短信内容 */
} sms_info_t;

/* GPS信息（如果模块支持） */
typedef struct {
    double latitude;    /* 纬度 */
    double longitude;   /* 经度 */
    float altitude;     /* 海拔 */
    float speed;        /* 速度 */
    float course;       /* 航向 */
    uint8_t fix_mode;   /* 定位模式 */
    uint8_t satellites; /* 卫星数 */
    char time[20];      /* UTC时间 */
} gps_info_t;

/* TCP/UDP连接信息 */
typedef struct {
    uint8_t link_id;      /* 连接ID */
    char protocol[4];     /* 协议 TCP/UDP */
    char remote_ip[16];   /* 远程IP */
    uint16_t remote_port; /* 远程端口 */
    uint16_t local_port;  /* 本地端口 */
    uint8_t state;        /* 连接状态 */
} socket_info_t;

/* 4G模块接口函数 */

/**
 * @brief 初始化4G模块
 */
int at_4g_init(at_device_t *dev, module_type_t type);

/**
 * @brief 获取模块基本信息
 */
int at_4g_get_module_info(at_device_t *dev, char *manufacturer, char *model,
                          char *version, char *imei);

/**
 * @brief 检查SIM卡状态
 */
sim_status_t at_4g_check_sim(at_device_t *dev);

/**
 * @brief 获取ICCID
 */
int at_4g_get_iccid(at_device_t *dev, char *iccid, size_t len);

/**
 * @brief 获取IMSI
 */
int at_4g_get_imsi(at_device_t *dev, char *imsi, size_t len);

/**
 * @brief 获取信号强度
 */
int at_4g_get_signal(at_device_t *dev, signal_info_t *signal);

/**
 * @brief 获取网络注册状态
 */
int at_4g_get_network_status(at_device_t *dev, network_info_t *netinfo);

/**
 * @brief 获取运营商信息
 */
int at_4g_get_operator(at_device_t *dev, char *operator_name, size_t len);

/**
 * @brief 激活PDP上下文
 */
int at_4g_activate_pdp(at_device_t *dev, const char *apn, const char *user,
                       const char *password);

/**
 * @brief 去激活PDP上下文
 */
int at_4g_deactivate_pdp(at_device_t *dev);

/**
 * @brief 获取IP地址
 */
int at_4g_get_ip_address(at_device_t *dev, char *ip, size_t len);

/**
 * @brief 发送短信
 */
int at_4g_send_sms(at_device_t *dev, const char *number, const char *message);

/**
 * @brief 读取短信
 */
int at_4g_read_sms(at_device_t *dev, uint8_t index, sms_info_t *sms);

/**
 * @brief 删除短信
 */
int at_4g_delete_sms(at_device_t *dev, uint8_t index);

/**
 * @brief 建立TCP连接
 */
int at_4g_tcp_connect(at_device_t *dev, uint8_t link_id, const char *host,
                      uint16_t port);

/**
 * @brief 关闭TCP连接
 */
int at_4g_tcp_close(at_device_t *dev, uint8_t link_id);

/**
 * @brief 发送TCP数据
 */
int at_4g_tcp_send(at_device_t *dev, uint8_t link_id, const uint8_t *data,
                   uint32_t len);

/**
 * @brief 建立UDP连接
 */
int at_4g_udp_connect(at_device_t *dev, uint8_t link_id, const char *host,
                      uint16_t port);

/**
 * @brief 发送UDP数据
 */
int at_4g_udp_send(at_device_t *dev, uint8_t link_id, const uint8_t *data,
                   uint32_t len);

/**
 * @brief 查询数据使用量
 */
int at_4g_get_data_usage(at_device_t *dev, uint32_t *tx_bytes,
                         uint32_t *rx_bytes);

/**
 * @brief 获取网络时间
 */
int at_4g_get_network_time(at_device_t *dev, char *time_str, size_t len);

/**
 * @brief 获取基站信息
 */
int at_4g_get_cell_info(at_device_t *dev, char *cell_info, size_t len);

/**
 * @brief 重启模块
 */
int at_4g_reboot(at_device_t *dev);

/**
 * @brief 设置模块低功耗模式
 */
int at_4g_set_low_power(at_device_t *dev, bool enable);

/**
 * @brief GPS功能（如果模块支持）
 */
#ifdef AT_4G_WITH_GPS
int at_4g_gps_power(at_device_t *dev, bool enable);
int at_4g_get_gps_info(at_device_t *dev, gps_info_t *gps);
#endif

/**
 * @brief HTTP客户端功能
 */
int at_4g_http_init(at_device_t *dev);
int at_4g_http_get(at_device_t *dev, const char *url, char *response,
                   size_t max_len);
int at_4g_http_post(at_device_t *dev, const char *url, const char *content_type,
                    const char *data, char *response, size_t max_len);

/**
 * @brief MQTT客户端功能
 */
int at_4g_mqtt_connect(at_device_t *dev, const char *host, uint16_t port,
                       const char *client_id, const char *username,
                       const char *password);
int at_4g_mqtt_publish(at_device_t *dev, const char *topic, const char *message,
                       uint8_t qos);
int at_4g_mqtt_subscribe(at_device_t *dev, const char *topic, uint8_t qos);
int at_4g_mqtt_disconnect(at_device_t *dev);

/**
 * @brief 注册4G模块通用URC处理器
 */
int at_4g_register_urc_handlers(at_device_t *dev);

/**
 * @brief 模块兼容性适配
 */
module_type_t at_4g_detect_module(at_device_t *dev);

#endif /* AT_4G_H */