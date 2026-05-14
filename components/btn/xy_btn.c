/**
 * @file xy_btn.c
 * @brief Button state machine — debounce, click, long-press, multi-click.
 *
 * State transitions (pressed = gpio == active_level after debounce):
 *
 *   IDLE ──raw press──► PRESSING ──debounce OK──► PRESSED ──raw release──► RELEASING
 *                          │                         │                          │
 *                      bounced back              long_press / repeat        debounce OK
 *                          │                         │                          │
 *                         IDLE                   (stay)               WAIT_MULTI or IDLE
 *
 *   WAIT_MULTI ──raw press──► PRESSING (another tap)
 *              ──gap timeout──► fire CLICK / MULTI_CLICK ──► IDLE
 */

#include "xy_btn.h"
#include <string.h>

/* ── Registry ────────────────────────────────────────────────────────── */

static xy_btn_t *s_head;

/* ── Init ────────────────────────────────────────────────────────────── */

void xy_btn_init(xy_btn_t      *btn,
                 xy_btn_read_fn read_fn,
                 void          *read_arg,
                 uint8_t        active_level,
                 xy_btn_ev_fn   cb,
                 void          *user)
{
    if (!btn) return;
    memset(btn, 0, sizeof(*btn));

    btn->read_fn  = read_fn;
    btn->read_arg = read_arg;
    btn->active   = active_level;
    btn->cb       = cb;
    btn->user     = user;

    /* Apply defaults */
    btn->cfg.debounce_ms        = 20u;
    btn->cfg.long_press_ms      = 1500u;
    btn->cfg.repeat_ms          = 0u;
    btn->cfg.multi_click_max    = 1u;
    btn->cfg.multi_click_gap_ms = 300u;
}

void xy_btn_set_config(xy_btn_t *btn, const xy_btn_config_t *cfg)
{
    if (!btn || !cfg) return;
    btn->cfg = *cfg;
}

/* ── Internal helpers ────────────────────────────────────────────────── */

static inline void _fire(xy_btn_t *btn, xy_btn_ev_t ev, uint8_t clicks)
{
    if (btn->cb) btn->cb(btn, ev, clicks, btn->user);
}

/* Deliver accumulated clicks and reset to IDLE. */
static void _deliver_clicks(xy_btn_t *btn)
{
    if (btn->click_cnt == 1u) {
        _fire(btn, XY_BTN_EV_CLICK, 1u);
    } else if (btn->click_cnt >= 2u) {
        _fire(btn, XY_BTN_EV_MULTI_CLICK, btn->click_cnt);
    }
    btn->click_cnt = 0u;
    btn->state     = _XY_BTN_IDLE;
}

/* ── Process ─────────────────────────────────────────────────────────── */

void xy_btn_process(xy_btn_t *btn, uint32_t now_ms)
{
    if (!btn || !btn->read_fn) return;

    uint8_t raw     = btn->read_fn(btn->read_arg);
    uint8_t pressed = (raw == btn->active);   /* 1 = physically pressed */
    uint32_t elapsed;

    switch (btn->state) {

    case _XY_BTN_IDLE:
        if (pressed) {
            btn->state     = _XY_BTN_PRESSING;
            btn->t_edge_ms = now_ms;
        }
        break;

    case _XY_BTN_PRESSING:
        if (!pressed) {
            /* Bounced back — ignore */
            btn->state = _XY_BTN_IDLE;
            break;
        }
        elapsed = now_ms - btn->t_edge_ms;
        if (elapsed >= btn->cfg.debounce_ms) {
            btn->state       = _XY_BTN_PRESSED;
            btn->t_press_ms  = now_ms;
            btn->long_fired  = false;
            btn->repeat_due  = (btn->cfg.repeat_ms > 0u)
                               ? now_ms + btn->cfg.repeat_ms : 0u;
            _fire(btn, XY_BTN_EV_PRESSED, 0u);
        }
        break;

    case _XY_BTN_PRESSED:
        if (!pressed) {
            btn->state     = _XY_BTN_RELEASING;
            btn->t_edge_ms = now_ms;
            break;
        }
        elapsed = now_ms - btn->t_press_ms;

        /* Long-press (fires once per hold) */
        if (!btn->long_fired &&
            btn->cfg.long_press_ms > 0u &&
            elapsed >= btn->cfg.long_press_ms) {
            btn->long_fired = true;
            _fire(btn, XY_BTN_EV_LONG_PRESS, 0u);
        }

        /* Auto-repeat (only when long-press has not fired) */
        if (!btn->long_fired &&
            btn->cfg.repeat_ms > 0u &&
            btn->repeat_due != 0u &&
            (int32_t)(now_ms - btn->repeat_due) >= 0) {
            btn->repeat_due += btn->cfg.repeat_ms;
            _fire(btn, XY_BTN_EV_REPEAT, 0u);
        }
        break;

    case _XY_BTN_RELEASING:
        if (pressed) {
            /* Bounced back — return to PRESSED (timer continues) */
            btn->state = _XY_BTN_PRESSED;
            break;
        }
        elapsed = now_ms - btn->t_edge_ms;
        if (elapsed >= btn->cfg.debounce_ms) {
            _fire(btn, XY_BTN_EV_RELEASED, 0u);

            if (!btn->long_fired) {
                btn->click_cnt++;
                if (btn->cfg.multi_click_max <= 1u) {
                    /* Deliver immediately — no multi-click grouping */
                    _deliver_clicks(btn);
                } else {
                    /* Wait for possible follow-up tap */
                    btn->state     = _XY_BTN_WAIT_MULTI;
                    btn->t_edge_ms = now_ms;
                }
            } else {
                /* Long-press ended: no click */
                btn->click_cnt = 0u;
                btn->state     = _XY_BTN_IDLE;
            }
        }
        break;

    case _XY_BTN_WAIT_MULTI:
        if (pressed) {
            /* Another tap — restart debounce */
            if (btn->click_cnt < btn->cfg.multi_click_max)
                btn->state = _XY_BTN_PRESSING;
            else
                btn->state = _XY_BTN_PRESSING;  /* at max: still accept, deliver later */
            btn->t_edge_ms = now_ms;
            break;
        }
        elapsed = now_ms - btn->t_edge_ms;
        if (elapsed >= btn->cfg.multi_click_gap_ms) {
            _deliver_clicks(btn);
        }
        break;
    }
}

/* ── State queries ───────────────────────────────────────────────────── */

bool xy_btn_is_pressed(const xy_btn_t *btn)
{
    return btn && (btn->state == _XY_BTN_PRESSED || btn->state == _XY_BTN_RELEASING);
}

uint32_t xy_btn_press_duration_ms(const xy_btn_t *btn, uint32_t now_ms)
{
    if (!btn) return 0u;
    if (btn->state == _XY_BTN_PRESSED || btn->state == _XY_BTN_RELEASING)
        return now_ms - btn->t_press_ms;
    return 0u;
}

/* ── Registry ────────────────────────────────────────────────────────── */

void xy_btn_register(xy_btn_t *btn)
{
    if (!btn) return;
    btn->next = s_head;
    s_head    = btn;
}

void xy_btn_process_all(uint32_t now_ms)
{
    for (xy_btn_t *b = s_head; b; b = b->next)
        xy_btn_process(b, now_ms);
}
