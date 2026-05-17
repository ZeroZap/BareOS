/**
 * @file app_timer.c
 * @brief Demonstrates all four Contiki timer types on the PC build.
 *
 *  etimer  — 1 s periodic, drives the timer_process wake-up.
 *  ctimer  — 2 s periodic callback (independent of any process).
 *  stimer  — 10 s seconds-granularity expiry check (polled inside etimer).
 *  rtimer  — 500 ms real-time callback (polled via pc_rtimer_poll in main loop).
 */

#include "app_timer.h"
#include "etimer.h"
#include "ctimer.h"
#include "stimer.h"
#include "rtimer.h"
#include "xy_log.h"
#include "tiny_cmd.h"

extern volatile unsigned int g_sys_tick_ms;

/* ── ctimer (2 s callback) ────────────────────────────────────────────── */

static struct ctimer s_ct;

static void on_ctimer(void *p)
{
    (void)p;
    xy_log_i("[ctimer] 2s fired @ %u ms", g_sys_tick_ms);
    ctimer_reset(&s_ct);   /* re-arm for another 2 s */
}

/* ── rtimer (500 ms callback) ────────────────────────────────────────── */

static struct rtimer s_rt;

static void on_rtimer(struct rtimer *t, void *p)
{
    (void)p;
    xy_log_d("[rtimer] 500ms fired @ %u ms  tick=%u",
             g_sys_tick_ms, (unsigned int)rtimer_now());
    rtimer_set(t, rtimer_now() + RTIMER_SECOND / 2, on_rtimer, NULL);
}

/* ── timer_process (etimer 1s + stimer 10s) ─────────────────────────── */

PROCESS(timer_process, "TimerDemo");

PROCESS_THREAD(timer_process, ev, data)
{
    static struct etimer s_et;
    static struct stimer s_st;

    PROCESS_BEGIN();

    /* Arm ctimer and rtimer once (they self-re-arm in their callbacks). */
    ctimer_set(&s_ct, 2000u, on_ctimer, NULL);
    rtimer_set(&s_rt, rtimer_now() + RTIMER_SECOND / 2, on_rtimer, NULL);
    stimer_set(&s_st, 10u);   /* 10 second interval */

    xy_log_i("[timer] started: etimer=1s  ctimer=2s  rtimer=500ms  stimer=10s");

    while (1) {
        etimer_set(&s_et, 1000u);
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&s_et));

        xy_log_d("[etimer] 1s tick @ %u ms", g_sys_tick_ms);

        if (stimer_expired(&s_st)) {
            xy_log_i("[stimer] 10s elapsed @ %u ms", g_sys_tick_ms);
            stimer_reset(&s_st);
        }
    }

    PROCESS_END();
}

/* ── Shell command: timers ────────────────────────────────────────────── */

static int cmd_timers(tiny_cmd_t *sh, int argc, char *argv[])
{
    (void)argc; (void)argv;
    tiny_cmd_printf(sh, "tick_ms   = %u\r\n", g_sys_tick_ms);
    tiny_cmd_printf(sh, "rtimer    = %u ticks (%u us)\r\n",
                    (unsigned int)rtimer_now(),
                    (unsigned int)(rtimer_now() / (RTIMER_SECOND / 1000000UL)));
    tiny_cmd_printf(sh, "stimer_s  = %u s\r\n", (unsigned int)stimer_now_s());
    tiny_cmd_printf(sh, "ctimer    = %s\r\n",
                    ctimer_expired(&s_ct) ? "expired" : "running");
    return 0;
}

static const tiny_cmd_entry_t s_timer_cmds[] = {
    { "timers", "Show timer states", cmd_timers },
};

void app_timer_register_cmds(void)
{
    tiny_cmd_register(s_timer_cmds,
                      sizeof(s_timer_cmds) / sizeof(s_timer_cmds[0]));
}
