/**
 * @file xy_stdlib.h
 * @brief stdlib.h replacement — no standard library dependency.
 *
 * Provides atoi/atol/strtol/strtoul/abs implemented in xy_stdio.c, plus
 * standard-name compatibility macros so existing code that calls atoi(),
 * strtoul() etc. works without any source-level rename.
 *
 * Dynamic memory (malloc/free) is intentionally NOT here — use xy_mem.h
 * and xy_malloc()/xy_free() directly for explicit pool-based allocation.
 */

#ifndef XY_STDLIB_H
#define XY_STDLIB_H

#include "xy_stdio.h"   /* declares xy_atoi, xy_atol, xy_atoll, xy_strtol,
                           xy_strtoul — all implemented in xy_stdio.c      */
#include "xy_typedef.h" /* size_t, bool, NULL */

#ifdef __cplusplus
extern "C" {
#endif

/* ── abs / labs ──────────────────────────────────────────────────────── */

#define xy_abs(x)   ((int)  ((x) <  0  ? -(x) :  (x)))
#define xy_labs(x)  ((long) ((x) <  0L ? -(x) :  (x)))

/* ── Standard-name compatibility macros ─────────────────────────────── */

#define atoi(s)           xy_atoi(s)
#define atol(s)           xy_atol(s)
#define atoll(s)          xy_atoll(s)
#define strtol(s, e, b)   xy_strtol((s), (e), (b))
#define strtoul(s, e, b)  xy_strtoul((s), (e), (b))
#define abs(x)            xy_abs(x)
#define labs(x)           xy_labs(x)

#ifdef __cplusplus
}
#endif

#endif /* XY_STDLIB_H */
