/**
 * @file xy_io.h
 * @brief IO abstraction: LEDs and buttons.
 *
 * BSP integration:
 *   Implement xy_io_led_hw_set() and xy_io_btn_hw_read() in the BSP layer.
 *   Weak stubs (no-ops) are provided in xy_io.c for platforms that don't
 *   need hardware I/O.
 */

#ifndef XY_IO_H
#define XY_IO_H

#include <stdint.h>
#include "xy_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ── LED IDs ─────────────────────────────────────────────────────────── */

typedef enum {
    XY_LED_STATUS = 0,   /* Green status / heartbeat LED   */
    XY_LED_ALARM,        /* Red alarm / distress LED       */
    XY_LED_GPS,          /* Blue GPS-fix indicator LED     */
    XY_LED_COUNT
} xy_led_id_t;

/* ── Button IDs ──────────────────────────────────────────────────────── */

typedef enum {
    XY_BTN_ACTIVATE = 0, /* Distress activation button     */
    XY_BTN_TEST,         /* Self-test / enter-config button*/
    XY_BTN_COUNT
} xy_btn_id_t;

/* ── API ─────────────────────────────────────────────────────────────── */

/** Initialise all outputs to off. */
void xy_io_init(void);

/** Set LED @id on or off. */
void xy_io_led_set(xy_led_id_t id, bool on);

/** Toggle LED @id. */
void xy_io_led_toggle(xy_led_id_t id);

/** Return current LED state (software shadow). */
bool xy_io_led_get(xy_led_id_t id);

/** Return true if button @id is currently pressed. */
bool xy_io_btn_pressed(xy_btn_id_t id);

/* ── BSP hooks (implement in BSP; weak stubs in xy_io.c) ─────────────── */

void xy_io_led_hw_set(xy_led_id_t id, bool on);
bool xy_io_btn_hw_read(xy_btn_id_t id);

#ifdef __cplusplus
}
#endif

#endif /* XY_IO_H */
