#ifndef XY_CELL_DRV_H
#define XY_CELL_DRV_H

#include "xy_cell.h"
#include "xy_rb.h"

/* Single shared operation struct — bare-metal, one op at a time */
typedef struct {
    volatile bool done;
    at_resp_code  code;
    union {
        struct { const char *apn; }
            pdp;
        struct { int id; const char *proto; const char *host;
                 uint16_t port; bool tls; }
            sock_open;
        struct { int id; const void *data; uint16_t len; }
            sock_send;
        struct { int id; }
            sock_close;
        struct { int cid; const char *host; uint16_t port;
                 const char *client_id; const char *user;
                 const char *pass; bool tls; }
            mqtt_conn;
        struct { int cid; const char *topic;
                 const void *payload; uint16_t len; uint8_t qos; }
            mqtt_pub;
        struct { int cid; const char *topic; uint8_t qos; }
            mqtt_sub;
        struct { int cid; }
            mqtt_disc;
        struct { const char *number; const char *text; }
            sms;
    };
} xy_cell_op_t;

typedef struct {
    const char        *name;
    at_work_t          pdp_activate;
    at_work_t          pdp_deactivate;
    at_work_t          sock_open;
    at_work_t          sock_send;
    at_work_t          sock_close;
    at_work_t          mqtt_connect;
    at_work_t          mqtt_publish;
    at_work_t          mqtt_subscribe;
    at_work_t          mqtt_disconnect;
    const urc_item_t  *urc_tbl;
    int                urc_cnt;
} xy_cell_drv_t;

typedef struct {
    bool    open;
    bool    tls;
    uint8_t mem[XY_CELL_SOCK_RXBUF];
    xy_rb_t rx;
} xy_cell_sock_t;

/* Globals defined in xy_cell.c */
extern xy_cell_op_t   g_cell_op;
extern xy_cell_sock_t g_cell_socks[XY_CELL_SOCK_MAX];
extern xy_cell_t      g_cell;

/* Shared helpers defined in xy_cell.c */
int  xy_cell_urc_cmt(at_urc_info_t *info);
void cell_parse_ip(const char *buf, char *ip, int iplen);
void xy_cell_mqtt_deliver(int cid, const char *topic, uint16_t topiclen,
                          const void *payload, uint16_t payloadlen);

/* Driver vtables */
extern const xy_cell_drv_t xy_cell_drv_sim76;
extern const xy_cell_drv_t xy_cell_drv_ec2x;
extern const xy_cell_drv_t xy_cell_drv_ml302;
extern const xy_cell_drv_t xy_cell_drv_fibocom;

/* Work-completion helpers — aliased to the shared at_chat.h definitions. */
#define OP_OK(op, env)   AT_OP_OK((op), (env))
#define OP_ERR(op, env)  AT_OP_ERR((op), (env))

#endif /* XY_CELL_DRV_H */
