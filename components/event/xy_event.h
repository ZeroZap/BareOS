/**
 * @file xy_event.h
 * @brief Lightweight bare-metal event system (Contiki-inspired).
 *
 * Events are posted to a bounded ring queue and dispatched in the main loop.
 * Posting is ISR-safe (atomic write to a ring slot).
 *
 * Usage:
 *   // Define event IDs (application owns the namespace above XY_EVT_USER_BASE)
 *   #define APP_EVT_SOS_PRESS   (XY_EVT_USER_BASE + 0)
 *   #define APP_EVT_GNSS_FIX    (XY_EVT_USER_BASE + 1)
 *
 *   // Subscribe once at startup (e.g. in app_init()):
 *   xy_event_subscribe(APP_EVT_SOS_PRESS, on_sos_press);
 *
 *   // Post from ISR or any task:
 *   xy_event_post(APP_EVT_SOS_PRESS, NULL);
 *
 *   // In main loop:
 *   xy_event_dispatch();
 *
 * Constraints:
 *   - Max subscribers per event: XY_EVENT_MAX_HANDLERS (default 4)
 *   - Queue depth:               XY_EVENT_QUEUE_DEPTH  (default 16, must be power of 2)
 *   - Event data pointer is NOT copied — caller must keep it alive until handled.
 */

#ifndef XY_EVENT_H
#define XY_EVENT_H

#include <stdint.h>
#include "xy_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ── Configuration ──────────────────────────────────────────────────── */

#ifndef XY_EVENT_QUEUE_DEPTH
#define XY_EVENT_QUEUE_DEPTH    16u   /* must be power of 2 */
#endif

#ifndef XY_EVENT_MAX_ID
#define XY_EVENT_MAX_ID         64u   /* total event IDs (0..63) */
#endif

#ifndef XY_EVENT_MAX_HANDLERS
#define XY_EVENT_MAX_HANDLERS   4u    /* max subscribers per event ID */
#endif

/* ── Event IDs ──────────────────────────────────────────────────────── */

typedef uint8_t xy_event_id_t;

#define XY_EVT_NONE             0x00u
#define XY_EVT_TIMER            0x01u   /* etimer expired              */
#define XY_EVT_INIT             0x02u   /* process initialisation      */
#define XY_EVT_POLL             0x03u   /* request a process poll      */
#define XY_EVT_EXIT             0x04u   /* process should exit         */
#define XY_EVT_CONTINUE         0x05u   /* resume a paused process     */

/* Application event IDs start here */
#define XY_EVT_USER_BASE        0x10u

/* ── Handler type ───────────────────────────────────────────────────── */

typedef void (*xy_event_handler_t)(xy_event_id_t ev, void *data);

/* ── API ─────────────────────────────────────────────────────────────── */

/**
 * Subscribe @handler to event @ev_id.
 * Multiple handlers may subscribe to the same event (up to XY_EVENT_MAX_HANDLERS).
 * Returns 0 on success, -1 if handler table is full.
 */
int xy_event_subscribe(xy_event_id_t ev_id, xy_event_handler_t handler);

/**
 * Unsubscribe @handler from event @ev_id.
 * Returns 0 on success, -1 if not found.
 */
int xy_event_unsubscribe(xy_event_id_t ev_id, xy_event_handler_t handler);

/**
 * Post event @ev_id with optional payload pointer @data.
 *
 * Single-producer / single-consumer: safe with one ISR posting while the
 * main loop dispatches. No critical section is taken.
 *
 * Multi-producer (more than one ISR posting concurrently): the BSP must
 * override xy_event_lock() / xy_event_unlock() to disable / restore IRQs
 * around the slot reservation. Default implementations are no-ops.
 *
 * Returns 0 on success, -1 if queue is full (event dropped).
 *
 * @note @data pointer is NOT copied; it must remain valid until
 *       xy_event_dispatch() calls the handler.
 */
int xy_event_post(xy_event_id_t ev_id, void *data);

/**
 * BSP-overridable critical-section hooks. Override (non-weak strong def)
 * to enable multi-producer post safety:
 *   void xy_event_lock(void)   { __disable_irq();   }
 *   void xy_event_unlock(void) { __enable_irq();    }
 * The defaults are no-ops, preserving single-producer behavior.
 */
void xy_event_lock(void);
void xy_event_unlock(void);

/**
 * Dispatch all pending events from the queue.
 * Call once per main loop iteration.
 * Returns number of events dispatched.
 */
int xy_event_dispatch(void);

/**
 * Return number of events currently waiting in the queue.
 */
uint32_t xy_event_pending(void);

/**
 * Flush all pending events without dispatching them.
 */
void xy_event_flush(void);

#ifdef __cplusplus
}
#endif

#endif /* XY_EVENT_H */
