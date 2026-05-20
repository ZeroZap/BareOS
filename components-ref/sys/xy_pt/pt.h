/**
 * @file pt.h
 * @brief Protothread — stackless cooperative coroutines for bare-metal MCU.
 *
 * Design: Duff's Device trick (switch + case __LINE__) lets a C function
 * suspend and resume without a dedicated stack frame.
 *
 * Usage — single PT:
 *   static struct pt s_pt;
 *   PT_THREAD(my_task(struct pt *pt)) {
 *       PT_BEGIN(pt);
 *       PT_WAIT_UNTIL(pt, condition);
 *       PT_END(pt);
 *   }
 *   // in main loop:
 *   PT_SCHEDULE(my_task(&s_pt));
 *
 * Usage — sub-thread (PT_SPAWN):
 *   PT_SPAWN(pt, &sub_pt, child_task(&sub_pt));
 */

#ifndef XY_PT_H
#define XY_PT_H

#include <stdint.h>

/* ────────────────────────────────────────────────
 * Local continuation type
 * ──────────────────────────────────────────────── */

typedef uint16_t pt_lc_t;  /* supports functions up to 65535 source lines */

struct pt {
    pt_lc_t lc;
};

/* ────────────────────────────────────────────────
 * Return values
 * ──────────────────────────────────────────────── */

#define PT_WAITING  0   /* still running, condition not met yet   */
#define PT_YIELDED  1   /* voluntarily yielded for one iteration  */
#define PT_ENDED    2   /* reached PT_END                         */
#define PT_EXITED   3   /* PT_EXIT was called                     */

/* ────────────────────────────────────────────────
 * Core macros
 * ──────────────────────────────────────────────── */

/** Declare a PT function signature. */
#define PT_THREAD(name_args)   int name_args

/** Initialize (or reset) a protothread. */
#define PT_INIT(pt)    ((pt)->lc = 0)

/** Begin the protothread body. Must be the first statement. */
#define PT_BEGIN(pt)   { switch ((pt)->lc) { case 0:

/**
 * Block until cond is true.
 * The function returns PT_WAITING each time cond is false.
 */
#define PT_WAIT_UNTIL(pt, cond)                         \
    do {                                                \
        (pt)->lc = (pt_lc_t)__LINE__;                  \
        case __LINE__:                                  \
        if (!(cond)) { return PT_WAITING; }             \
    } while (0)

/** Block while cond is true (inverse of PT_WAIT_UNTIL). */
#define PT_WAIT_WHILE(pt, cond)  PT_WAIT_UNTIL(pt, !(cond))

/**
 * Yield once: suspend now, resume on the next call.
 * Useful to give other tasks a chance to run.
 */
#define PT_YIELD(pt)                                    \
    do {                                                \
        (pt)->lc = (pt_lc_t)__LINE__;                  \
        return PT_YIELDED;                              \
        case __LINE__: ;                                \
    } while (0)

/** Yield while cond is true. */
#define PT_YIELD_UNTIL(pt, cond)                        \
    do {                                                \
        (pt)->lc = (pt_lc_t)__LINE__;                  \
        return PT_YIELDED;                              \
        case __LINE__:                                  \
        if (!(cond)) { return PT_YIELDED; }             \
    } while (0)

/**
 * Spawn a child protothread and block until it finishes (PT_ENDED).
 * child_pt must be a separate struct pt from the parent.
 *
 *   PT_SPAWN(pt, &sub_pt, child_task(&sub_pt));
 */
#define PT_SPAWN(pt, child_pt, child_call)              \
    do {                                                \
        PT_INIT(child_pt);                              \
        PT_WAIT_UNTIL(pt, (child_call) >= PT_ENDED);   \
    } while (0)

/** End the protothread. Resets lc so it can be restarted. */
#define PT_END(pt)                                      \
    } (pt)->lc = 0; return PT_ENDED; }

/** Exit the protothread early (from inside any nested block). */
#define PT_EXIT(pt)                                     \
    do { (pt)->lc = 0; return PT_EXITED; } while (0)

/* ────────────────────────────────────────────────
 * Schedule helper
 * ──────────────────────────────────────────────── */

/**
 * Call a PT function and return non-zero if it is still running.
 *   PT_SCHEDULE(my_task(&s_pt));
 */
#define PT_SCHEDULE(f)  ((f) < PT_ENDED)

/* ────────────────────────────────────────────────
 * State query
 * ──────────────────────────────────────────────── */

/** True if the protothread has not started or has finished. */
#define PT_IS_IDLE(pt)      ((pt)->lc == 0)

/** True if the protothread is suspended mid-execution. */
#define PT_IS_RUNNING(pt)   ((pt)->lc != 0)

#endif /* XY_PT_H */
