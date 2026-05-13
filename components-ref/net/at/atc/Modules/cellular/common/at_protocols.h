/**
 * @file at_protocols.h
 * @brief 多协议支持接口
 */

#ifndef AT_PROTOCOLS_H
#define AT_PROTOCOLS_H

#include "at_client.h"
#include "at_4g.h"

/* SSL/TLS配置 */
typedef struct {
    uint8_t ssl_version; /* 0: TLS 1.0, 1: TLS 1.1, 2: TLS 1.2 */
    uint8_t verify_mode; /* 0:不验证, 1:验证服务器证书 */
    uint8_t auth_mode;   /* 0:单向认证, 1:双向认证 */
    char *ca_cert;       /* CA证书 */
    char *client_cert;   /* 客户端证书 */
    char *client_key;    /* 客户端私钥 */
    char *sni;           /* 服务器名称指示 */
} ssl_config_t;

/* MQTT配置 */
typedef struct {
    char *client_id;
    char *username;
    char *password;
    char *will_topic;
    char *will_message;
    uint8_t will_qos;
    uint8_t will_retain;
    uint16_t keep_alive;
    uint8_t clean_session;
    uint32_t timeout_ms;
} mqtt_config_t;

/* FTP配置 */
typedef struct {
    char *server;
    uint16_t port;
    char *username;
    char *password;
    char *remote_path;
    char *local_path;
    uint8_t mode; /* 0:ASCII, 1:BINARY */
} ftp_config_t;

/* NTP配置 */
typedef struct {
    char *server;
    uint16_t port;
    int8_t timezone;        /* 时区，如8表示东八区 */
    bool auto_sync;         /* 自动同步 */
    uint32_t sync_interval; /* 同步间隔(秒) */
} ntp_config_t;

/* DNS配置 */
typedef struct {
    char *primary_dns;
    char *secondary_dns;
    uint32_t cache_timeout;
} dns_config_t;

/* 多路Socket管理 */
typedef struct {
    uint8_t max_sockets;  /* 最大socket数量 */
    uint8_t used_sockets; /* 已用socket数量 */
    struct {
        uint8_t id;
        uint8_t protocol; /* 0:TCP, 1:UDP, 2:SSL */
        uint8_t state;    /* 0:关闭, 1:连接中, 2:已连接 */
        uint32_t tx_bytes;
        uint32_t rx_bytes;
    } sockets[6]; /* 通常4G模块支持最多6个socket */
} socket_pool_t;

/* API函数声明 */

/* ============ HTTPS/SSL功能 ============ */
/**
 * @brief 初始化SSL/TLS配置
 */
int at_ssl_init(at_device_t *dev, ssl_config_t *config);

/**
 * @brief 建立SSL连接
 */
int at_ssl_connect(at_device_t *dev, uint8_t link_id, const char *host,
                   uint16_t port);

/**
 * @brief 发送SSL数据
 */
int at_ssl_send(at_device_t *dev, uint8_t link_id, const uint8_t *data,
                uint32_t len);

/**
 * @brief 关闭SSL连接
 */
int at_ssl_close(at_device_t *dev, uint8_t link_id);

/**
 * @brief 获取SSL证书信息
 */
int at_ssl_get_cert_info(at_device_t *dev, char *issuer, char *subject,
                         char *expiry_date);

/**
 * @brief HTTPS GET请求
 */
int at_https_get(at_device_t *dev, const char *url, char *response,
                 size_t max_len);

/**
 * @brief HTTPS POST请求
 */
int at_https_post(at_device_t *dev, const char *url, const char *content_type,
                  const char *data, char *response, size_t max_len);

/* ============ MQTT高级功能 ============ */
/**
 * @brief 初始化MQTT客户端
 */
int at_mqtt_init(at_device_t *dev, mqtt_config_t *config);

/**
 * @brief 连接MQTT服务器
 */
int at_mqtt_connect(at_device_t *dev, const char *host, uint16_t port);

/**
 * @brief 发布消息
 */
int at_mqtt_publish(at_device_t *dev, const char *topic, const char *message,
                    uint8_t qos, uint8_t retain);

/**
 * @brief 订阅主题
 */
int at_mqtt_subscribe(at_device_t *dev, const char *topic, uint8_t qos);

/**
 * @brief 取消订阅
 */
int at_mqtt_unsubscribe(at_device_t *dev, const char *topic);

/**
 * @brief 接收MQTT消息（非阻塞）
 */
int at_mqtt_receive(at_device_t *dev, char *topic, size_t topic_len,
                    char *message, size_t msg_len, uint32_t timeout_ms);

/**
 * @brief 断开MQTT连接
 */
int at_mqtt_disconnect(at_device_t *dev);

/**
 * @brief 获取MQTT连接状态
 */
int at_mqtt_get_status(at_device_t *dev);

/* ============ FTP客户端功能 ============ */
/**
 * @brief 初始化FTP客户端
 */
int at_ftp_init(at_device_t *dev, ftp_config_t *config);

/**
 * @brief 连接FTP服务器
 */
int at_ftp_connect(at_device_t *dev);

/**
 * @brief 上传文件
 */
int at_ftp_upload(at_device_t *dev, const char *local_file,
                  const char *remote_file);

/**
 * @brief 下载文件
 */
int at_ftp_download(at_device_t *dev, const char *remote_file,
                    const char *local_file);

/**
 * @brief 列出目录
 */
int at_ftp_list(at_device_t *dev, const char *path, char *list, size_t max_len);

/**
 * @brief 删除文件
 */
int at_ftp_delete(at_device_t *dev, const char *remote_file);

/**
 * @brief 创建目录
 */
int at_ftp_mkdir(at_device_t *dev, const char *dir);

/**
 * @brief 断开FTP连接
 */
int at_ftp_disconnect(at_device_t *dev);

/* ============ NTP时间同步 ============ */
/**
 * @brief 初始化NTP客户端
 */
int at_ntp_init(at_device_t *dev, ntp_config_t *config);

/**
 * @brief 同步网络时间
 */
int at_ntp_sync(at_device_t *dev);

/**
 * @brief 获取网络时间
 */
int at_ntp_get_time(at_device_t *dev, struct tm *timeinfo);

/**
 * @brief 设置时区
 */
int at_ntp_set_timezone(at_device_t *dev, int8_t timezone);

/**
 * @brief 启用自动时间同步
 */
int at_ntp_enable_auto_sync(at_device_t *dev, uint32_t interval_sec);

/* ============ DNS域名解析 ============ */
/**
 * @brief 设置DNS服务器
 */
int at_dns_set_servers(at_device_t *dev, const char *primary,
                       const char *secondary);

/**
 * @brief 域名解析
 */
int at_dns_resolve(at_device_t *dev, const char *hostname, char *ip,
                   size_t ip_len);

/**
 * @brief 清空DNS缓存
 */
int at_dns_clear_cache(at_device_t *dev);

/* ============ 多路Socket管理 ============ */
/**
 * @brief 初始化Socket池
 */
int at_socket_pool_init(at_device_t *dev, uint8_t max_sockets);

/**
 * @brief 申请Socket
 */
int at_socket_alloc(at_device_t *dev, uint8_t protocol);

/**
 * @brief 释放Socket
 */
int at_socket_free(at_device_t *dev, uint8_t socket_id);

/**
 * @brief 获取Socket状态
 */
int at_socket_get_status(at_device_t *dev, uint8_t socket_id);

/**
 * @brief 获取Socket统计信息
 */
int at_socket_get_stats(at_device_t *dev, uint8_t socket_id, uint32_t *tx_bytes,
                        uint32_t *rx_bytes);

/* ============ PING网络诊断 ============ */
/**
 * @brief Ping测试
 */
int at_ping(at_device_t *dev, const char *host, uint32_t timeout_ms,
            uint8_t count);

/* ============ 邮件发送(SMTP) ============ */
/**
 * @brief 发送邮件
 */
int at_smtp_send(at_device_t *dev, const char *server, uint16_t port,
                 const char *username, const char *password, const char *from,
                 const char *to, const char *subject, const char *body);

/* ============ WebSocket ============ */
/**
 * @brief WebSocket连接
 */
int at_websocket_connect(at_device_t *dev, uint8_t link_id, const char *url,
                         const char *protocol);

/**
 * @brief WebSocket发送
 */
int at_websocket_send(at_device_t *dev, uint8_t link_id, const uint8_t *data,
                      uint32_t len, uint8_t opcode); /* 1:text, 2:binary */

/**
 * @brief WebSocket接收
 */
int at_websocket_receive(at_device_t *dev, uint8_t link_id, uint8_t *data,
                         uint32_t max_len, uint32_t timeout_ms);

#endif /* AT_PROTOCOLS_H */