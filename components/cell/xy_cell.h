#ifndef XY_CELL_H
#define XY_CELL_H

#include "xy_typedef.h"
#include "at_chat.h"

#define XY_CELL_SOCK_MAX    6
#define XY_CELL_SOCK_RXBUF  512

typedef enum {
    CELL_MDM_SIM76XX = 0,
    CELL_MDM_EC2X,
    CELL_MDM_ML302,
    CELL_MDM_FIBOCOM,  /* Fibocom L610 (UNISOC Cat.1) */
} xy_cell_mdm_t;

typedef enum {
    CELL_ST_OFF = 0,
    CELL_ST_INIT,        /* AT responds, echo off */
    CELL_ST_SIM_OK,      /* SIM present, IMSI read */
    CELL_ST_REGISTERED,  /* registered (CREG/CEREG stat=1 or 5) */
    CELL_ST_ONLINE,      /* PDP active, IP assigned */
} xy_cell_state_t;

typedef struct {
    at_obj_t        *at;
    xy_cell_state_t  state;
    int              rssi_dbm;   /* -113..–51 dBm, or 0 if unknown */
    char             imsi[16];   /* 15 digits + NUL */
    char             iccid[21];  /* 20 digits + NUL */
    char             ip[16];     /* dotted-decimal */
} xy_cell_t;

typedef void (*xy_cell_mqtt_recv_cb_t)(int cid, const char *topic,
                                        const void *payload, uint16_t len);
typedef void (*xy_cell_sms_recv_cb_t)(const char *number, const char *text);

/* Lifecycle */
void             xy_cell_init(at_obj_t *at, xy_cell_mdm_t mdm);
xy_cell_state_t  xy_cell_get_state(void);
const xy_cell_t *xy_cell_get_info(void);

/* Async operation poll helpers */
bool xy_cell_op_done(void);
bool xy_cell_op_ok(void);

/* Core — standard 3GPP TS 27.007 */
bool xy_cell_start_modem_init(void);
bool xy_cell_start_check_sim(void);
bool xy_cell_start_update_signal(void);
bool xy_cell_start_check_reg(void);
bool xy_cell_start_pdp_activate(const char *apn);
bool xy_cell_start_pdp_deactivate(void);

/* Socket — driver-specific */
bool xy_cell_start_sock_open(int id, const char *proto,
                              const char *host, uint16_t port);
bool xy_cell_start_sock_send(int id, const void *data, uint16_t len);
int  xy_cell_sock_recv(int id, void *buf, uint16_t maxlen);
void xy_cell_start_sock_close(int id);
bool xy_cell_sock_is_open(int id);

/* Module MQTT — driver-specific (returns false if driver has no MQTT) */
bool xy_cell_start_mqtt_connect(int cid, const char *host, uint16_t port,
                                 const char *client_id,
                                 const char *user, const char *pass, bool tls);
bool xy_cell_start_mqtt_disconnect(int cid);
bool xy_cell_start_mqtt_publish(int cid, const char *topic,
                                 const void *payload, uint16_t len, uint8_t qos);
bool xy_cell_start_mqtt_subscribe(int cid, const char *topic, uint8_t qos);
void xy_cell_mqtt_set_recv_cb(xy_cell_mqtt_recv_cb_t cb);

/* SMS */
bool xy_cell_start_sms_send(const char *number, const char *text);
void xy_cell_sms_set_recv_cb(xy_cell_sms_recv_cb_t cb);

#endif /* XY_CELL_H */
