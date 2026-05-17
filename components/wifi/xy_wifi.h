#ifndef XY_WIFI_H
#define XY_WIFI_H

#include "xy_typedef.h"
#include "at_chat.h"
#include "xy_rb.h"

/* Maximum concurrent TCP/UDP connections */
#define XY_WIFI_SOCK_MAX    4
/* Per-socket receive ring buffer size (bytes) */
#define XY_WIFI_SOCK_RXBUF  512

typedef enum {
    WIFI_ST_OFF = 0,
    WIFI_ST_INIT,        /* AT responds, station mode set */
    WIFI_ST_CONNECTED,   /* Associated to AP, IP assigned */
} xy_wifi_state_t;

typedef struct {
    at_obj_t        *at;
    xy_wifi_state_t  state;
    char             ip[16];    /* dotted-decimal */
    char             ssid[33];  /* current AP SSID */
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

/* Station management */
bool xy_wifi_start_init(void);                              /* ATE0 + CWMODE + CIPMUX */
bool xy_wifi_start_connect(const char *ssid, const char *pass);
bool xy_wifi_start_disconnect(void);

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
