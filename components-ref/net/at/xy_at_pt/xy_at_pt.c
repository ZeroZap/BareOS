/**
 * @file xy_at_pt.c
 */

#include "xy_at_pt.h"
#include "sys/xy_pt/etimer.h"
#include <string.h>
#include <stddef.h>

/* ── helpers ────────────────────────────────────────────────────────── */

static uint16_t rx_available(const xy_at_client_t *c)
{
    return (c->rx_head - c->rx_tail) & (XY_AT_PT_RX_BUF_SIZE - 1u);
}

static uint8_t rx_read(xy_at_client_t *c)
{
    uint8_t b = c->rx_buf[c->rx_tail & (XY_AT_PT_RX_BUF_SIZE - 1u)];
    c->rx_tail++;
    return b;
}

/* Remove trailing \r and \n from a null-terminated line. */
static void strip_crlf(char *line)
{
    int len = (int)strlen(line);
    while (len > 0 && (line[len - 1] == '\r' || line[len - 1] == '\n')) {
        line[--len] = '\0';
    }
}

/* ── Lifecycle ──────────────────────────────────────────────────────── */

void xy_at_pt_init(xy_at_client_t *c, const char *name,
                   xy_at_uart_send_fn uart_send)
{
    memset(c, 0, sizeof(*c));
    c->name       = name;
    c->uart_send  = uart_send;
}

int xy_at_pt_urc_register(xy_at_client_t *c, const char *prefix,
                           xy_at_urc_cb_t cb, void *arg)
{
    if (c->urc_count >= XY_AT_PT_URC_MAX) {
        return -1;
    }
    c->urcs[c->urc_count].prefix = prefix;
    c->urcs[c->urc_count].cb     = cb;
    c->urcs[c->urc_count].arg    = arg;
    c->urc_count++;
    return 0;
}

/* ── ISR interface ──────────────────────────────────────────────────── */

void xy_at_pt_isr_feed(xy_at_client_t *c, uint8_t byte)
{
    /* Simple ring-buffer write; drop on overflow (head wraps into tail). */
    uint16_t next = (c->rx_head + 1u) & (XY_AT_PT_RX_BUF_SIZE - 1u);
    if (next != (c->rx_tail & (XY_AT_PT_RX_BUF_SIZE - 1u))) {
        c->rx_buf[c->rx_head & (XY_AT_PT_RX_BUF_SIZE - 1u)] = byte;
        c->rx_head++;
    }
}

/* ── Line dispatcher (called when a complete line is assembled) ─────── */

static void dispatch_line(xy_at_client_t *c, char *line)
{
    strip_crlf(line);
    if (line[0] == '\0') {
        return;  /* empty line — ignore */
    }

    if (c->busy) {
        /* We are inside a command exchange. */
        if (c->expect_ok && strcmp(line, c->expect_ok) == 0) {
            c->last_result = XY_AT_OK;
            c->busy        = false;
            return;
        }
        if (c->expect_err && strcmp(line, c->expect_err) == 0) {
            c->last_result = XY_AT_ERROR;
            c->busy        = false;
            return;
        }
        /* Intermediate data line — capture if prefix matches. */
        if (c->data_prefix &&
            strncmp(line, c->data_prefix, strlen(c->data_prefix)) == 0) {
            strncpy(c->last_line, line, XY_AT_PT_LINE_MAX - 1);
            c->last_line[XY_AT_PT_LINE_MAX - 1] = '\0';
        }
        return;
    }

    /* Not in a command — check URC table. */
    for (uint8_t i = 0; i < c->urc_count; i++) {
        if (c->urcs[i].prefix &&
            strncmp(line, c->urcs[i].prefix, strlen(c->urcs[i].prefix)) == 0) {
            if (c->urcs[i].cb) {
                c->urcs[i].cb(line, c->urcs[i].arg);
            }
            return;
        }
    }
}

/* ── Main-loop poll ─────────────────────────────────────────────────── */

void xy_at_pt_poll(xy_at_client_t *c)
{
    while (rx_available(c)) {
        uint8_t b = rx_read(c);

        if (b == '\n') {
            /* End of line — dispatch and reset. */
            c->line_buf[c->line_len] = '\0';
            dispatch_line(c, c->line_buf);
            c->line_len = 0;
        } else if (b != '\r') {
            if (c->line_len < XY_AT_PT_LINE_MAX - 1) {
                c->line_buf[c->line_len++] = (char)b;
            }
        }
    }
}

/* ── PT command execution ───────────────────────────────────────────── */

PT_THREAD(xy_at_exec(struct pt      *pt,
                     xy_at_client_t *c,
                     xy_at_req_t    *req,
                     const char     *cmd,
                     const char     *data_prefix,
                     const char     *expect_ok,
                     const char     *expect_err,
                     uint32_t        timeout_ms))
{
    static struct etimer s_timer;  /* NOTE: static — one per PT context */

    PT_BEGIN(pt);

    /* Wait until the client is free (no concurrent command). */
    PT_WAIT_UNTIL(pt, !c->busy);

    /* Arm the client for this exchange. */
    c->busy         = true;
    c->expect_ok    = expect_ok;
    c->expect_err   = expect_err;
    c->data_prefix  = data_prefix;
    c->last_result  = XY_AT_TIMEOUT;
    c->last_line[0] = '\0';
    req->result     = XY_AT_TIMEOUT;
    req->response[0]= '\0';

    /* Send command string + CRLF. */
    c->uart_send((const uint8_t *)cmd, (uint16_t)strlen(cmd));
    c->uart_send((const uint8_t *)"\r\n", 2);

    /* Arm timeout timer. */
    etimer_set(&s_timer, timeout_ms);

    /* Suspend until response received or timeout. */
    PT_WAIT_UNTIL(pt, !c->busy || etimer_expired(&s_timer));

    /* Collect result. */
    req->result = c->last_result;
    if (data_prefix && req->result == XY_AT_OK) {
        strncpy(req->response, c->last_line, XY_AT_PT_LINE_MAX - 1);
        req->response[XY_AT_PT_LINE_MAX - 1] = '\0';
    }

    /* Release the client. */
    c->busy        = false;
    c->expect_ok   = NULL;
    c->expect_err  = NULL;
    c->data_prefix = NULL;
    etimer_stop(&s_timer);

    PT_END(pt);
}
