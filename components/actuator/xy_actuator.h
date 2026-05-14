/**
 * @file xy_actuator.h
 * @brief Bare-metal actuator abstraction — output-side complement to xy_sensor.
 *
 * Provides a unified API for any hardware output: LED, buzzer, relay, PWM,
 * generic GPIO.  Each actuator is described by a static device object plus a
 * const driver vtable; no heap is required.
 *
 * An integrated pattern player allows timed sequences (blink, beep, SOS morse)
 * driven by ctimer callbacks, requiring only ctimer_run() in the main loop.
 *
 * ── Quick start ──────────────────────────────────────────────────────
 *
 *   // 1. Implement the driver (in bsp_led.c):
 *   static int led_set(xy_actuator_device_t *dev,
 *                      xy_actuator_channel_t ch, int32_t val) {
 *       if (ch == XY_ACTUATOR_CHAN_STATE) gpio_write(LED_PIN, val);
 *       return 0;
 *   }
 *   static const xy_actuator_driver_api_t led_api = { .set = led_set };
 *   xy_actuator_device_t g_led = { "led_status", XY_ACTUATOR_LED, &led_api };
 *
 *   // 2. Register at startup:
 *   xy_actuator_register(&g_led);
 *   xy_actuator_init(&g_led);
 *
 *   // 3. Direct control:
 *   xy_actuator_on(&g_led);
 *   xy_actuator_off(&g_led);
 *   xy_actuator_set(&g_led, XY_ACTUATOR_CHAN_DUTY, 75);  // 75% PWM
 *
 *   // 4. Pattern playback (requires ctimer_run() in main loop):
 *   static xy_actuator_pattern_t pat;
 *   xy_actuator_pattern_start(&pat, &g_led, XY_ACTUATOR_CHAN_STATE,
 *                             XY_ACTUATOR_STEPS_BLINK_SLOW,
 *                             XY_ACTUATOR_STEPS_BLINK_SLOW_N, -1);
 *
 * ── Channel values ────────────────────────────────────────────────────
 *
 *   CHAN_STATE:      0 = off,   1 = on
 *   CHAN_INTENSITY:  0..100  (percent, e.g. LED brightness)
 *   CHAN_FREQUENCY:  Hz      (buzzer tone, PWM frequency)
 *   CHAN_DUTY:       0..100  (percent duty cycle)
 *   CHAN_DIRECTION:  0 = forward, 1 = reverse (motor)
 *
 * ── Pre-defined patterns ──────────────────────────────────────────────
 *
 *   XY_ACTUATOR_STEPS_BLINK_SLOW   500 ms on / 500 ms off
 *   XY_ACTUATOR_STEPS_BLINK_FAST   100 ms on / 100 ms off
 *   XY_ACTUATOR_STEPS_BEEP_ONCE    200 ms on / 800 ms off (1-s period)
 *   XY_ACTUATOR_STEPS_SOS          SOS morse code with 700 ms word gap
 */

#ifndef XY_ACTUATOR_H
#define XY_ACTUATOR_H

#include "ctimer.h"   /* struct ctimer, ctimer_set/stop */
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Actuator types ──────────────────────────────────────────────────── */

typedef uint8_t xy_actuator_type_t;

#define XY_ACTUATOR_GPIO     0x00u   /* generic digital output           */
#define XY_ACTUATOR_LED      0x01u   /* LED (on/off or PWM brightness)   */
#define XY_ACTUATOR_BUZZER   0x02u   /* piezo / magnetic buzzer          */
#define XY_ACTUATOR_RELAY    0x03u   /* relay or solid-state switch      */
#define XY_ACTUATOR_PWM      0x04u   /* generic PWM output               */
#define XY_ACTUATOR_MOTOR    0x05u   /* DC motor or servo                */

/* ── Output channels ─────────────────────────────────────────────────── */

typedef uint8_t xy_actuator_channel_t;

#define XY_ACTUATOR_CHAN_STATE      0x00u   /* 0 = off,      1 = on         */
#define XY_ACTUATOR_CHAN_INTENSITY  0x01u   /* 0..100 percent               */
#define XY_ACTUATOR_CHAN_FREQUENCY  0x02u   /* Hz                           */
#define XY_ACTUATOR_CHAN_DUTY       0x03u   /* 0..100 percent duty cycle    */
#define XY_ACTUATOR_CHAN_DIRECTION  0x04u   /* 0 = forward, 1 = reverse     */

/* ── Status codes ────────────────────────────────────────────────────── */

typedef enum {
    XY_ACTUATOR_OK = 0,
    XY_ACTUATOR_ERROR,
    XY_ACTUATOR_ERROR_INVALID_PARAM,
    XY_ACTUATOR_ERROR_NOT_SUPPORTED,
    XY_ACTUATOR_ERROR_NOT_INITIALIZED,
    XY_ACTUATOR_ERROR_BUSY,
} xy_actuator_status_t;

/* ── Forward declaration ─────────────────────────────────────────────── */

struct xy_actuator_device;

/* ── Driver vtable ───────────────────────────────────────────────────── */

/**
 * Driver vtable — implement in your BSP driver file.
 * Only @set is mandatory; others may be NULL.
 */
typedef struct {
    /** One-time hardware initialisation. */
    int (*init)(struct xy_actuator_device *dev);

    /** Release hardware resources. */
    int (*deinit)(struct xy_actuator_device *dev);

    /**
     * Write a channel value.
     * @param ch   XY_ACTUATOR_CHAN_*
     * @param val  New value.
     * @return 0 on success, negative on error.
     */
    int (*set)(struct xy_actuator_device *dev,
               xy_actuator_channel_t ch, int32_t val);

    /**
     * Read the current channel value.
     * @param val  Output pointer.
     * @return 0 on success, negative on error.
     */
    int (*get)(struct xy_actuator_device *dev,
               xy_actuator_channel_t ch, int32_t *val);
} xy_actuator_driver_api_t;

/* ── Device object ───────────────────────────────────────────────────── */

/**
 * Statically allocate one of these per physical actuator.
 * Initialise name, type, and api; zero the rest.
 *
 *   xy_actuator_device_t g_led = { "led_status", XY_ACTUATOR_LED, &led_api };
 */
typedef struct xy_actuator_device {
    const char                       *name;         /* unique identifier           */
    xy_actuator_type_t                type;
    const xy_actuator_driver_api_t   *api;
    void                             *data;          /* driver private state        */
    bool                              initialized;

    struct xy_actuator_device        *next;          /* registry linked list        */
} xy_actuator_device_t;

/* ── Pattern player ──────────────────────────────────────────────────── */

/**
 * One step in a timed output sequence.
 *   { .value = 1, .duration_ms = 500 }  — output high for 500 ms
 *   { .value = 0, .duration_ms = 500 }  — output low  for 500 ms
 */
typedef struct {
    int32_t  value;        /* channel value to apply at start of this step */
    uint32_t duration_ms;  /* how long to hold the value                   */
} xy_actuator_step_t;

/**
 * Pattern player state.  Allocate one per simultaneous pattern instance.
 * Do not read or modify fields directly.
 */
typedef struct {
    xy_actuator_device_t      *dev;
    xy_actuator_channel_t      channel;
    const xy_actuator_step_t  *steps;
    uint8_t                    count;       /* total steps in sequence      */
    uint8_t                    pos;         /* current step index           */
    int8_t                     plays_left;  /* -1=infinite, 0=done, N=N more */
    struct ctimer              ct;
} xy_actuator_pattern_t;

/* ── Pre-defined step tables ─────────────────────────────────────────── */

/** 500 ms on / 500 ms off */
extern const xy_actuator_step_t xy_actuator_steps_blink_slow[2];
#define XY_ACTUATOR_STEPS_BLINK_SLOW    xy_actuator_steps_blink_slow
#define XY_ACTUATOR_STEPS_BLINK_SLOW_N  2u

/** 100 ms on / 100 ms off */
extern const xy_actuator_step_t xy_actuator_steps_blink_fast[2];
#define XY_ACTUATOR_STEPS_BLINK_FAST    xy_actuator_steps_blink_fast
#define XY_ACTUATOR_STEPS_BLINK_FAST_N  2u

/** 200 ms on / 800 ms off (1-second period, one beep per second) */
extern const xy_actuator_step_t xy_actuator_steps_beep_once[2];
#define XY_ACTUATOR_STEPS_BEEP_ONCE     xy_actuator_steps_beep_once
#define XY_ACTUATOR_STEPS_BEEP_ONCE_N   2u

/**
 * SOS morse code: · · · — — — · · ·
 * Dot=100 ms, Dash=300 ms, intra-letter gap=100 ms,
 * inter-letter gap=300 ms, inter-word gap=700 ms.
 * 18 steps, ~3.5 s per cycle.
 */
extern const xy_actuator_step_t xy_actuator_steps_sos[18];
#define XY_ACTUATOR_STEPS_SOS    xy_actuator_steps_sos
#define XY_ACTUATOR_STEPS_SOS_N  18u

/* ── Core API ────────────────────────────────────────────────────────── */

/**
 * Register a device with the global registry.
 * Call once per device before xy_actuator_init().
 * @return XY_ACTUATOR_OK or XY_ACTUATOR_ERROR_INVALID_PARAM.
 */
xy_actuator_status_t xy_actuator_register(xy_actuator_device_t *dev);

/**
 * Initialise a device (calls api->init if present).
 * @return XY_ACTUATOR_OK on success.
 */
xy_actuator_status_t xy_actuator_init(xy_actuator_device_t *dev);

/**
 * De-initialise a device (calls api->deinit if present).
 */
xy_actuator_status_t xy_actuator_deinit(xy_actuator_device_t *dev);

/**
 * Write a value to the specified channel.
 * @param ch   XY_ACTUATOR_CHAN_* constant.
 * @param val  Value to set.
 */
xy_actuator_status_t xy_actuator_set(xy_actuator_device_t *dev,
                                     xy_actuator_channel_t ch, int32_t val);

/**
 * Read the current value of a channel.
 * @param val  Output pointer; unchanged on error.
 */
xy_actuator_status_t xy_actuator_get(xy_actuator_device_t *dev,
                                     xy_actuator_channel_t ch, int32_t *val);

/** Convenience: turn the actuator on  (CHAN_STATE = 1). */
xy_actuator_status_t xy_actuator_on(xy_actuator_device_t *dev);

/** Convenience: turn the actuator off (CHAN_STATE = 0). */
xy_actuator_status_t xy_actuator_off(xy_actuator_device_t *dev);

/**
 * Toggle CHAN_STATE.  Reads the current state via api->get (if implemented)
 * and inverts it; falls back to internal tracking if get is NULL.
 */
xy_actuator_status_t xy_actuator_toggle(xy_actuator_device_t *dev);

/* ── Device lookup ───────────────────────────────────────────────────── */

/** Find a registered device by name.  Returns NULL if not found. */
xy_actuator_device_t *xy_actuator_device_get(const char *name);

/** Find a registered device by type and index (0-based).  Returns NULL. */
xy_actuator_device_t *xy_actuator_device_get_by_type(xy_actuator_type_t type,
                                                      uint8_t index);

/** Iterate all registered devices.  Return false from @cb to stop early. */
void xy_actuator_device_foreach(bool (*cb)(xy_actuator_device_t *, void *),
                                void *user);

/** Return the number of registered devices. */
uint8_t xy_actuator_device_count(void);

/* ── Pattern player API ──────────────────────────────────────────────── */

/**
 * Start a timed output sequence.
 *
 * Applies steps[0].value immediately, then steps through the remaining
 * steps using ctimer callbacks.  Requires ctimer_run() in the main loop.
 *
 * @param pat       Pattern state (caller-allocated, must stay valid).
 * @param dev       Target actuator device.
 * @param channel   Channel to drive (typically XY_ACTUATOR_CHAN_STATE).
 * @param steps     Array of { value, duration_ms } steps.
 * @param count     Number of steps.
 * @param plays     -1 = loop forever; N > 0 = play N times then stop.
 */
void xy_actuator_pattern_start(xy_actuator_pattern_t    *pat,
                               xy_actuator_device_t     *dev,
                               xy_actuator_channel_t     channel,
                               const xy_actuator_step_t *steps,
                               uint8_t                   count,
                               int8_t                    plays);

/**
 * Stop the pattern and set CHAN_STATE = 0.
 * Safe to call when no pattern is running.
 */
void xy_actuator_pattern_stop(xy_actuator_pattern_t *pat);

/**
 * Return true if a pattern is currently running (ctimer not expired).
 */
bool xy_actuator_pattern_active(const xy_actuator_pattern_t *pat);

#ifdef __cplusplus
}
#endif

#endif /* XY_ACTUATOR_H */
