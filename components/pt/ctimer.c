/**
 * @file ctimer.c
 * @brief Callback timer implementation.
 */

#include "ctimer.h"
#include <stddef.h>

/* ── Active list ─────────────────────────────────────────────────────── */

static struct ctimer *s_head;

static void list_add(struct ctimer *ct)
{
    /* Remove first to avoid duplicates */
    struct ctimer **pp = &s_head;
    while (*pp) {
        if (*pp == ct) { *pp = ct->next; break; }
        pp = &(*pp)->next;
    }
    ct->next = s_head;
    s_head   = ct;
}

static void list_remove(struct ctimer *ct)
{
    struct ctimer **pp = &s_head;
    while (*pp) {
        if (*pp == ct) { *pp = ct->next; ct->next = NULL; return; }
        pp = &(*pp)->next;
    }
}

/* ── Public API ─────────────────────────────────────────────────────── */

void ctimer_set(struct ctimer *ct, uint32_t interval_ms,
                void (*f)(void *), void *ptr)
{
    ct->f   = f;
    ct->ptr = ptr;
    etimer_set(&ct->et, interval_ms);
    list_add(ct);
}

void ctimer_reset(struct ctimer *ct)
{
    etimer_restart(&ct->et);   /* restart from previous expiry — no drift */
    list_add(ct);
}

void ctimer_restart(struct ctimer *ct)
{
    etimer_reset(&ct->et);     /* restart from now */
    list_add(ct);
}

void ctimer_stop(struct ctimer *ct)
{
    etimer_stop(&ct->et);
    list_remove(ct);
}

bool ctimer_expired(const struct ctimer *ct)
{
    return etimer_expired(&ct->et);
}

int ctimer_run(void)
{
    int fired = 0;
    struct ctimer *ct = s_head;

    while (ct) {
        struct ctimer *next = ct->next;
        if (etimer_expired(&ct->et)) {
            list_remove(ct);       /* remove before callback (cb may re-arm) */
            if (ct->f) {
                ct->f(ct->ptr);
                fired++;
            }
        }
        ct = next;
    }
    return fired;
}
