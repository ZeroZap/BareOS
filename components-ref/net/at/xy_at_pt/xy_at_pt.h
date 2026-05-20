/**
 * @file xy_at_pt.h
 * @brief AT command client with Protothread integration.
 *
 * Wraps the raw AT-Command-V2 framework so that AT exchanges can be
 * written as sequential Protothread code instead of callback spaghetti.
 *
 * ─── Typical usage inside a PT ──────────────────────────────────────
 *
 *   #include "sys/xy_pt/pt.h"
 *   #include "net/at/xy_at_pt/xy_at_pt.h"
 *
 *   static xy_at_client_t  g_at_4g;
 *   static struct pt        s_pt_4g;
 *
 *   // UART ISR feeds bytes into the client:
 *   void UART1_IRQHandler(void) {
 *       xy_at_pt_isr_feed(&g_at_4g, uart_rx_byte());
 *   }
 *
 *   PT_THREAD(task_4g(struct pt *pt))
 *   {
 *       static struct pt   sub_pt;
 *       static xy_at_req_t req;
 *
 *       PT_BEGIN(pt);
 *
 *       // ① Handshake
 *       PT_SPAWN(pt, &sub_pt,
 *           xy_at_exec(&sub_pt, &g_at_4g, &req,
 *                      "AT", NULL, "OK", "ERROR", 1000));
 *       if (req.result != XY_AT_OK) { PT_EXIT(pt); }
 *
 *       // ② Query signal quality
 *       PT_SPAWN(pt, &sub_pt,
 *           xy_at_exec(&sub_pt, &g_at_4g, &req,
 *                      "AT+CSQ", "+CSQ", "OK", "ERROR", 2000));
 *       if (req.result == XY_AT_OK) {
 *           int rssi;
 *           sscanf(req.response, "+CSQ: %d", &rssi);
 *       }
 *
 *       PT_END(pt);
 *   }
 *
 * ─── URC (unsolicited result code) ──────────────────────────────────
 *
 *   // Register once during init:
 *   xy_at_pt_urc_register(&g_at_4g, "+QMTRECV",  on_mqtt_recv,  NULL);
 *   xy_at_pt_urc_register(&g_at_4g, "+QMTSTAT",  on_mqtt_stat,  NULL);
 *
 *   // Callback fires in main-loop context (not ISR):
 *   static void on_mqtt_recv(const char *line, void *arg) {
 *       xy_broker_post(&msg);  // forward to broker
 *   }
 */

#ifndef XY_AT_PT_H
#define XY_AT_PT_H

#include <stdint.h>
#include <stdbool.h>
#include "sys/xy_pt/pt.h"

/* ── Configuration ──────────────────────────────────────────────────── */

#ifndef XY_AT_PT_LINE_MAX
#define XY_AT_PT_LINE_MAX       256   /* max chars in one AT response line */
#endif

#ifndef XY_AT_PT_RX_BUF_SIZE
#define XY_AT_PT_RX_BUF_SIZE    512   /* ISR ring buffer size               */
#endif

#ifndef XY_AT_PT_URC_MAX
#define XY_AT_PT_URC_MAX        8     /* max registered URC prefixes        */
#endif

/* ── Result codes ───────────────────────────────────────────────────── */

typedef enum {
    XY_AT_OK        =  0,
    XY_AT_ERROR     = -1,
    XY_AT_TIMEOUT   = -2,
    XY_AT_BUSY      = -3,  /* client already executing a command */
} xy_at_result_t;

/* ── Request descriptor (output from xy_at_exec) ──────────────────── */

typedef struct {
    xy_at_result_t  result;
    char            response[XY_AT_PT_LINE_MAX];  /* intermediate data line */
} xy_at_req_t;

/* ── URC callback type ──────────────────────────────────────────────── */

typedef void (*xy_at_urc_cb_t)(const char *line, void *arg);

/* ── URC entry ──────────────────────────────────────────────────────── */

typedef struct {
    const char    *prefix;
    xy_at_urc_cb_t cb;
    void          *arg;
} xy_at_urc_t;

/* ── UART send function pointer (platform-supplied) ────────────────── */

typedef void (*xy_at_uart_send_fn)(const uint8_t *buf, uint16_t len);

/* ── Client handle ──────────────────────────────────────────────────── */

typedef struct {
    const char         *name;
    xy_at_uart_send_fn  uart_send;       /* platform TX function */

    /* ── ISR ring buffer (written from ISR, read from main loop) ── */
    uint8_t  rx_buf[XY_AT_PT_RX_BUF_SIZE];
    uint16_t rx_head;   /* write index (ISR) */
    uint16_t rx_tail;   /* read  index (main loop) */

    /* ── Line parser state ── */
    char     line_buf[XY_AT_PT_LINE_MAX];
    uint16_t line_len;

    /* ── Current command state ── */
    volatile bool       busy;
    const char         *expect_ok;
    const char         *expect_err;
    const char         *data_prefix;
    volatile xy_at_result_t last_result;
    char                last_line[XY_AT_PT_LINE_MAX];

    /* ── URC table ── */
    xy_at_urc_t  urcs[XY_AT_PT_URC_MAX];
    uint8_t      urc_count;
} xy_at_client_t;

/* ────────────────────────────────────────────────────────────────────
 * Lifecycle
 * ──────────────────────────────────────────────────────────────────── */

/** Initialize a client.  Must be called before any other function. */
void xy_at_pt_init(xy_at_client_t *c, const char *name,
                   xy_at_uart_send_fn uart_send);

/** Register a URC prefix and its callback.
 *  Callback fires in main-loop context when a line starting with
 *  prefix is received outside of a command exchange.             */
int  xy_at_pt_urc_register(xy_at_client_t *c, const char *prefix,
                            xy_at_urc_cb_t cb, void *arg);

/* ────────────────────────────────────────────────────────────────────
 * ISR interface — call from UART RX interrupt only
 * ──────────────────────────────────────────────────────────────────── */

/** Feed one received byte into the client's ring buffer. */
void xy_at_pt_isr_feed(xy_at_client_t *c, uint8_t byte);

/* ────────────────────────────────────────────────────────────────────
 * Main-loop interface — call once per main loop iteration
 * ──────────────────────────────────────────────────────────────────── */

/**
 * Process bytes from the ring buffer, parse complete lines, dispatch
 * URC callbacks and update command result.
 * Must be called from main-loop context (not ISR).
 */
void xy_at_pt_poll(xy_at_client_t *c);

/* ────────────────────────────────────────────────────────────────────
 * Protothread command execution
 * ──────────────────────────────────────────────────────────────────── */

/**
 * PT_THREAD: Send an AT command and wait (in PT sense) for the response.
 *
 * @param pt          Caller-supplied sub struct pt.
 * @param c           The AT client to use.
 * @param req         Output: result code + intermediate data line.
 * @param cmd         AT command string (without trailing "\r\n").
 * @param data_prefix Prefix of the intermediate data line to capture
 *                    (e.g. "+CSQ").  NULL = don't capture.
 * @param expect_ok   String that marks success (e.g. "OK").
 * @param expect_err  String that marks failure (e.g. "ERROR").
 * @param timeout_ms  Milliseconds before XY_AT_TIMEOUT is set.
 *
 * Example:
 *   PT_SPAWN(pt, &sub_pt,
 *       xy_at_exec(&sub_pt, &g_4g, &req,
 *                  "AT+CSQ", "+CSQ", "OK", "ERROR", 2000));
 */
PT_THREAD(xy_at_exec(struct pt      *pt,
                     xy_at_client_t *c,
                     xy_at_req_t    *req,
                     const char     *cmd,
                     const char     *data_prefix,
                     const char     *expect_ok,
                     const char     *expect_err,
                     uint32_t        timeout_ms));

/* ── Convenience macro: only care about OK/ERROR, no data line ──── */
#define XY_AT_EXEC_SIMPLE(pt, c, req, cmd, timeout_ms)          \
    xy_at_exec(pt, c, req, cmd, NULL, "OK", "ERROR", timeout_ms)

#endif /* XY_AT_PT_H */
