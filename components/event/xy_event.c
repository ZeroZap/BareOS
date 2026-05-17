/**
 * @file xy_event.c
 * @brief Lightweight bare-metal event system.
 *
 * Ring queue uses power-of-2 masking so head/tail can be incremented freely
 * without wrapping arithmetic on every access.  The write (post) side only
 * updates s_head after the slot is filled; the read (dispatch) side only
 * advances s_tail after the slot is consumed — safe for one-producer /
 * one-consumer without disabling interrupts.
 *
 * For multi-producer (multiple ISRs posting), wrap xy_event_post() with a
 * critical section in the BSP (disable/enable interrupts around the call).
 */

#include "xy_event.h"
#include "xy_string.h"

/* ── Queue slot ─────────────────────────────────────────────────────── */

typedef struct {
    xy_event_id_t ev_id;
    void         *data;
} xy_event_slot_t;

/* ── Module state ───────────────────────────────────────────────────── */

static xy_event_slot_t   s_queue[XY_EVENT_QUEUE_DEPTH];
static volatile uint32_t s_head; /* next slot to write into */
static volatile uint32_t s_tail; /* next slot to read from  */

#define QUEUE_MASK  (XY_EVENT_QUEUE_DEPTH - 1u)

/* Compile-time check: XY_EVENT_QUEUE_DEPTH must be power of 2 */
typedef char _evt_pow2_check[(((XY_EVENT_QUEUE_DEPTH) & (XY_EVENT_QUEUE_DEPTH - 1u)) == 0) ? 1 : -1];

/* ── Subscriber table ───────────────────────────────────────────────── */

static xy_event_handler_t s_handlers[XY_EVENT_MAX_ID][XY_EVENT_MAX_HANDLERS];

/* ── BSP-overridable critical section (default: no-op) ──────────────── */

#if defined(__GNUC__)
__attribute__((weak)) void xy_event_lock(void)   { }
__attribute__((weak)) void xy_event_unlock(void) { }
#else
/* For non-GCC toolchains the BSP must provide strong definitions of these. */
void xy_event_lock(void);
void xy_event_unlock(void);
#endif

/* ── Public API ─────────────────────────────────────────────────────── */

int xy_event_subscribe(xy_event_id_t ev_id, xy_event_handler_t handler)
{
    if (ev_id >= XY_EVENT_MAX_ID || !handler) return -1;
    /* First pass: reject if already registered (avoid double-dispatch). */
    for (uint32_t i = 0; i < XY_EVENT_MAX_HANDLERS; i++) {
        if (s_handlers[ev_id][i] == handler) return 0;
    }
    /* Second pass: install in first empty slot. */
    for (uint32_t i = 0; i < XY_EVENT_MAX_HANDLERS; i++) {
        if (!s_handlers[ev_id][i]) {
            s_handlers[ev_id][i] = handler;
            return 0;
        }
    }
    return -1; /* table full */
}

int xy_event_unsubscribe(xy_event_id_t ev_id, xy_event_handler_t handler)
{
    if (ev_id >= XY_EVENT_MAX_ID || !handler) return -1;
    for (uint32_t i = 0; i < XY_EVENT_MAX_HANDLERS; i++) {
        if (s_handlers[ev_id][i] == handler) {
            s_handlers[ev_id][i] = NULL;
            return 0;
        }
    }
    return -1;
}

int xy_event_post(xy_event_id_t ev_id, void *data)
{
    int      rc = 0;
    uint32_t head;
    uint32_t next;

    xy_event_lock();          /* No-op unless BSP overrides for multi-producer. */
    head = s_head;
    next = head + 1u;
    if ((next - s_tail) > QUEUE_MASK) {
        rc = -1;              /* queue full */
    } else {
        s_queue[head & QUEUE_MASK].ev_id = ev_id;
        s_queue[head & QUEUE_MASK].data  = data;
        /* Publish: the increment makes the slot visible to the consumer */
        s_head = next;
    }
    xy_event_unlock();
    return rc;
}

int xy_event_dispatch(void)
{
    int dispatched = 0;

    while (s_tail != s_head) {
        xy_event_slot_t slot = s_queue[s_tail & QUEUE_MASK];
        s_tail++;
        dispatched++;

        if (slot.ev_id >= XY_EVENT_MAX_ID) continue;

        for (uint32_t i = 0; i < XY_EVENT_MAX_HANDLERS; i++) {
            if (s_handlers[slot.ev_id][i]) {
                s_handlers[slot.ev_id][i](slot.ev_id, slot.data);
            }
        }
    }
    return dispatched;
}

uint32_t xy_event_pending(void)
{
    return s_head - s_tail;
}

void xy_event_flush(void)
{
    s_tail = s_head;
}
