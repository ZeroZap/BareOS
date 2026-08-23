#ifndef XY_WIFI_H
#define XY_WIFI_H

#include "xy_typedef.h"
#include "at_chat.h"
#include "xy_rb.h"

/* Maximum concurrent TCP/UDP connections */
#define XY_WIFI_SOCK_MAX    4
/* Per-socket receive ring buffer size (bytes) */
#define XY_WIFI_SOCK_RXBUF  512
/* Maximum AP scan results retained in RAM */
#define XY_WIFI_SCAN_MAX    8

typedef enum {
    WIFI_ERR_OK = 0,
    WIFI_ERR_BUSY,
    WIFI_ERR_PARAM,
    WIFI_ERR_TIMEOUT,
    WIFI_ERR_AT_ERROR,
    WIFI_ERR_AUTH,
    WIFI_ERR_NO_AP,
    WIFI_ERR_CONNECT_FAIL,
    WIFI_ERR_DNS,
    WIFI_ERR_CLOSED,
    WIFI_ERR_UNSUPPORTED,
} xy_wifi_err_t;

typedef enum {
    WIFI_ST_OFF = 0,
    WIFI_ST_INIT,        /* AT responds, station mode set */
    WIFI_ST_CONNECTED,   /* Associated to AP, IP assigned */
} xy_wifi_state_t;

typedef enum {
    WIFI_AUTH_OPEN = 0,
    WIFI_AUTH_WEP,
    WIFI_AUTH_WPA_PSK,
    WIFI_AUTH_WPA2_PSK,
    WIFI_AUTH_WPA_WPA2_PSK,
    WIFI_AUTH_WPA2_ENTERPRISE,
    WIFI_AUTH_WPA3_PSK,
    WIFI_AUTH_WPA2_WPA3_PSK,
    WIFI_AUTH_UNKNOWN,
} xy_wifi_auth_t;

typedef struct {
    char ssid[33];
    char bssid[18];
    int  rssi_dbm;
    int  channel;
    xy_wifi_auth_t auth;
} xy_wifi_ap_t;

typedef struct {
    at_obj_t        *at;
    xy_wifi_state_t  state;
    xy_wifi_err_t    last_error;
    char             ip[16];      /* dotted-decimal */
    char             gateway[16];
    char             netmask[16];
    char             dns1[16];
    char             dns2[16];
    char             ssid[33];    /* current AP SSID */
    char             bssid[18];
    char             mac[18];
    int              rssi_dbm;
} xy_wifi_t;

typedef void (*xy_wifi_mqtt_recv_cb_t)(const char *topic,
                                        const void *payload, uint16_t len);

/* Lifecycle */
void             xy_wifi_init(at_obj_t *at);
xy_wifi_state_t  xy_wifi_get_state(void);
const xy_wifi_t *xy_wifi_get_info(void);

/* Async operation poll helpers */
bool xy_wifi_op_done(void);
bool xy_wifi_op_ok(void);
bool xy_wifi_busy(void);
xy_wifi_err_t xy_wifi_last_error(void);
const char *xy_wifi_last_error_str(void);
bool xy_wifi_is_connected(void);
bool xy_wifi_has_ip(void);

/* Station management */
bool xy_wifi_start_init(void);                              /* ATE0 + CWMODE + CIPMUX */
bool xy_wifi_start_connect(const char *ssid, const char *pass);
bool xy_wifi_start_disconnect(void);
bool xy_wifi_start_query_state(void);
bool xy_wifi_start_query_ip(void);
bool xy_wifi_start_query_mac(void);
bool xy_wifi_start_query_rssi(void);

/* AP scan */
bool xy_wifi_start_scan(void);
int xy_wifi_scan_count(void);
const xy_wifi_ap_t *xy_wifi_scan_result(int index);

/* Network interface configuration */
bool xy_wifi_start_set_dhcp(bool enable);
bool xy_wifi_start_set_static_ip(const char *ip,
                                 const char *gateway,
                                 const char *netmask);
bool xy_wifi_start_set_dns(const char *dns1, const char *dns2);

/* Socket (ESP-AT multi-connection mode) */
bool xy_wifi_start_sock_open(int id, const char *proto,
                              const char *host, uint16_t port);
bool xy_wifi_start_sock_send(int id, const void *data, uint16_t len);
int  xy_wifi_sock_recv(int id, void *buf, uint16_t maxlen);
void xy_wifi_start_sock_close(int id);
bool xy_wifi_sock_is_open(int id);

/* Module MQTT (AT+MQTTCONN / AT+MQTTPUB / AT+MQTTSUB)
 *
 * Note: ESP-AT firmware supports only one MQTT link (linkid 0). This API
 * intentionally omits the `cid` parameter that xy_cell's MQTT API exposes
 * for multi-connection cellular modems. Callers that need to target both
 * transports should either branch by transport or wrap behind their own
 * single-cid adapter. */
bool xy_wifi_start_mqtt_connect(const char *host, uint16_t port,
                                 const char *client_id,
                                 const char *user, const char *pass,
                                 bool tls);
bool xy_wifi_start_mqtt_disconnect(void);
bool xy_wifi_start_mqtt_publish(const char *topic,
                                 const void *payload, uint16_t len,
                                 uint8_t qos);
bool xy_wifi_start_mqtt_subscribe(const char *topic, uint8_t qos);
void xy_wifi_mqtt_set_recv_cb(xy_wifi_mqtt_recv_cb_t cb);

#endif /* XY_WIFI_H */
