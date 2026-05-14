/**
 * @file xy_btn.h
 * @brief Bare-metal button library — debounce, click, long-press, multi-click.
 *
 * No heap, no RTOS, no timer dependency.  The caller feeds the current GPIO
 * level and a timestamp each main-loop iteration; the library drives a state
 * machine and fires events via callback.
 *
 * ── Quick start ──────────────────────────────────────────────────────
 *
 *   static uint8_t  read_sos(void *arg) { return gpio_read(SOS_PIN); }
 *
 *   static void on_sos(xy_btn_t *btn, xy_btn_ev_t ev, uint8_t clicks, void *user) {
 *       if (ev == XY_BTN_EV_LONG_PRESS)
 *           start_distress();
 *   }
 *
 *   static xy_btn_t g_btn_sos;
 *
 *   // Init — active-low button, long-press = 3 s:
 *   xy_btn_init(&g_btn_sos, read_sos, NULL, 0, on_sos, NULL);
 *   xy_btn_config_t cfg = XY_BTN_CFG_DEFAULT;
 *   cfg.long_press_ms = 3000;
 *   xy_btn_set_config(&g_btn_sos, &cfg);
 *
 *   // Main loop:
 *   while (1) {
 *       xy_btn_process(&g_btn_sos, g_sys_tick_ms);
 *   }
 *
 * ── Multi-button convenience ──────────────────────────────────────────
 *
 *   xy_btn_register(&g_btn_sos);
 *   xy_btn_register(&g_btn_cancel);
 *
 *   while (1) {
 *       xy_btn_process_all(g_sys_tick_ms);
 *   }
 *
 * ── Events ────────────────────────────────────────────────────────────
 *
 *   EV_PRESSED     — debounced press (leading edge)
 *   EV_RELEASED    — debounced release
 *   EV_CLICK       — quick tap; clicks=1 (or N for multi-click)
 *   EV_LONG_PRESS  — held ≥ long_press_ms; fires once per hold
 *   EV_REPEAT      — auto-repeat pulse while held (if repeat_ms > 0)
 *   EV_MULTI_CLICK — ≥2 clicks in multi_click_gap_ms; clicks=count
 *
 * ── Timing notes ─────────────────────────────────────────────────────
 *
 *   All timing is in milliseconds.  Call xy_btn_process() at least every
 *   debounce_ms (default 20 ms) for accurate debounce.  Calling it more
 *   frequently (every main-loop tick) is fine and typical.
 *
 *   Long-press and click are mutually exclusive: if long_press fires,
 *   no EV_CLICK is sent for that press.
 *
 *   Multi-click detection (multi_click_max > 1) delays EV_CLICK by
 *   multi_click_gap_ms to collect all taps.
 */

#ifndef XY_BTN_H
#define XY_BTN_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Events ──────────────────────────────────────────────────────────── */

typedef uint8_t xy_btn_ev_t;

#define XY_BTN_EV_PRESSED     0x01u  /* debounced press (leading edge)        */
#define XY_BTN_EV_RELEASED    0x02u  /* debounced release                     */
#define XY_BTN_EV_CLICK       0x03u  /* quick tap (no long-press)             */
#define XY_BTN_EV_LONG_PRESS  0x04u  /* held ≥ long_press_ms                  */
#define XY_BTN_EV_REPEAT      0x05u  /* auto-repeat pulse while held          */
#define XY_BTN_EV_MULTI_CLICK 0x06u  /* 2+ rapid taps; clicks = tap count     */

/* ── Configuration ───────────────────────────────────────────────────── */

typedef struct {
    uint16_t debounce_ms;        /* input debounce window (default 20 ms)     */
    uint16_t long_press_ms;      /* hold time for EV_LONG_PRESS (default 1500)
                                  * Set 0 to disable long-press detection.    */
    uint16_t repeat_ms;          /* auto-repeat interval while held (default 0
                                  * = disabled).  Ignored when long_press fires. */
    uint8_t  multi_click_max;    /* max clicks to group; 1 = no multi-click   */
    uint16_t multi_click_gap_ms; /* max gap between taps (default 300 ms)     */
} xy_btn_config_t;

/** Default configuration — all sensible values for a PLB navigation button. */
#define XY_BTN_CFG_DEFAULT  {           \
    .debounce_ms        = 20u,          \
    .long_press_ms      = 1500u,        \
    .repeat_ms          = 0u,           \
    .multi_click_max    = 1u,           \
    .multi_click_gap_ms = 300u,         \
}

/* ── GPIO read callback ───────────────────────────────────────────────── */

/**
 * Return the raw GPIO level: 1 = high, 0 = low.
 * Called once per xy_btn_process() invocation.
 */
typedef uint8_t (*xy_btn_read_fn)(void *arg);

/* ── Event callback ──────────────────────────────────────────────────── */

struct xy_btn;

/**
 * @param btn    The button that fired the event.
 * @param ev     XY_BTN_EV_* constant.
 * @param clicks Click count — valid for EV_CLICK and EV_MULTI_CLICK only.
 * @param user   User context passed to xy_btn_init().
 */
typedef void (*xy_btn_ev_fn)(struct xy_btn *btn,
                              xy_btn_ev_t    ev,
                              uint8_t        clicks,
                              void          *user);

/* ── Button object ───────────────────────────────────────────────────── */

typedef enum {
    _XY_BTN_IDLE = 0,
    _XY_BTN_PRESSING,     /* debouncing press edge                        */
    _XY_BTN_PRESSED,      /* clean press; timing long-press / repeat      */
    _XY_BTN_RELEASING,    /* debouncing release edge                      */
    _XY_BTN_WAIT_MULTI,   /* released; collecting further clicks          */
} _xy_btn_state_t;

typedef struct xy_btn {
    /* Configuration */
    xy_btn_config_t  cfg;
    xy_btn_read_fn   read_fn;
    void            *read_arg;
    uint8_t          active;      /* GPIO level when button is pressed      */
    xy_btn_ev_fn     cb;
    void            *user;

    /* Internal state — do not access directly */
    _xy_btn_state_t  state;
    uint32_t         t_edge_ms;   /* timestamp of last state-machine edge   */
    uint32_t         t_press_ms;  /* timestamp of last debounced press      */
    uint32_t         repeat_due;  /* next repeat deadline (abs ms)          */
    bool             long_fired;  /* long-press already sent this hold      */
    uint8_t          click_cnt;   /* taps accumulated for multi-click       */

    struct xy_btn   *next;        /* registry linked list                   */
} xy_btn_t;

/* ── API ─────────────────────────────────────────────────────────────── */

/**
 * Initialise a button with default configuration.
 *
 * @param btn          Button object (caller-allocated).
 * @param read_fn      GPIO read callback (returns 0 or 1).
 * @param read_arg     Forwarded to read_fn as the arg parameter.
 * @param active_level GPIO level when the button is physically pressed (0 or 1).
 * @param cb           Event callback; may be NULL (poll via xy_btn_is_pressed).
 * @param user         User context forwarded to cb.
 */
void xy_btn_init(xy_btn_t      *btn,
                 xy_btn_read_fn read_fn,
                 void          *read_arg,
                 uint8_t        active_level,
                 xy_btn_ev_fn   cb,
                 void          *user);

/**
 * Override configuration after xy_btn_init().
 * Changes take effect from the next xy_btn_process() call.
 */
void xy_btn_set_config(xy_btn_t *btn, const xy_btn_config_t *cfg);

/**
 * Process one button.  Call each main-loop iteration.
 *
 * @param btn     The button to process.
 * @param now_ms  Current system time in milliseconds (e.g. g_sys_tick_ms).
 */
void xy_btn_process(xy_btn_t *btn, uint32_t now_ms);

/**
 * Return true when the button is in a clean debounced-pressed state.
 * Safe to call without a callback for polling-style code.
 */
bool xy_btn_is_pressed(const xy_btn_t *btn);

/**
 * Return how many milliseconds the button has been held since the last
 * debounced press.  0 when not pressed.  Useful for progress bars.
 */
uint32_t xy_btn_press_duration_ms(const xy_btn_t *btn, uint32_t now_ms);

/* ── Multi-button registry ───────────────────────────────────────────── */

/**
 * Register a button into the global linked list.
 * After registration, xy_btn_process_all() handles it automatically.
 */
void xy_btn_register(xy_btn_t *btn);

/**
 * Process all registered buttons in one call.
 * @param now_ms  Current system time (e.g. g_sys_tick_ms).
 */
void xy_btn_process_all(uint32_t now_ms);

#ifdef __cplusplus
}
#endif

#endif /* XY_BTN_H */
