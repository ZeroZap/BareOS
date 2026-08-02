# BareOS Power Management

The PM component selects between tickless deep sleep, shallow sleep, and active
execution using HAL power locks, registered pending-work checks, and registered
timeout providers.

## Idle policy

`xy_pm_tickless_idle()` applies the following policy:

| Allowed mode | Behavior |
|---|---|
| STOP or deeper | Program the low-power timer and enter tickless STOP |
| SLEEP | Enter shallow sleep without stopping SysTick |
| RUN | Return without sleeping |

The shallow path is evaluated before the STOP minimum-residency check. A short
deadline such as 1-2 ms is still suitable for normal SLEEP because SysTick stays
active; `min_sleep_ms` only limits the deeper LPTIM/STOP path.

All registered sleep checks are repeated with interrupts masked immediately
before either sleep entry. This closes the normal check-to-sleep race with work
posted by an ISR.

The low-power timer is configured before entering the deep-sleep critical
section, with interrupts still enabled. Its clock-domain synchronization time is
therefore accounted by the normal system tick instead of being lost on every
STOP entry. PM then masks interrupts and repeats the allowed-mode, pending-work,
and expired-deadline checks. If state changed during timer setup, PM cancels the
timer and does not enter STOP.

## Statistics

`xy_pm_get_stats()` exposes:

| Field | Meaning |
|---|---|
| `idle_calls` | Calls to the idle policy |
| `sleep_count` | Successful LPTIM/tickless deep sleeps |
| `shallow_sleep_count` | Successful normal SLEEP entries |
| `abort_count` | Calls that could not enter either sleep mode |
| `last_planned_ms` | Last planned deep-sleep duration |
| `last_elapsed_ms` | Last measured deep-sleep duration |
| `total_deep_sleep_ms` | Cumulative measured deep-sleep residency |
| `max_deep_sleep_ms` | Longest measured deep-sleep residency |
| `early_wake_count` | Deep sleeps that ended before the programmed deadline |
| `uart_wake_count` | Sleeps ended with a UART wake source reported |
| `gpio_wake_count` | Sleeps ended with a GPIO wake source reported |
| `lptimer_wake_count` | Sleeps ended with an LPTIM wake source reported |
| `rtc_wake_count` | Sleeps ended with an RTC wake source reported |
| `wdg_wake_count` | Sleeps ended with a watchdog wake source reported |
| `unknown_wake_count` | Successful sleeps with no source reported |
| `last_wake_sources` | Bit mask of sources reported for the last successful sleep |

For shallow sleep, `last_planned_ms` and `last_elapsed_ms` remain zero because
SysTick continues to account for time. A growing shallow count with a stable
abort count is the expected result while a STOP lock is held.

Wake-capable interrupt handlers call `xy_pm_report_wake_sources()` with one or
more `XY_HAL_WAKE_*` bits. Reports are accepted only across sleep entry and
resume, so interrupts handled during normal execution are not counted. Every
asserted bit is counted independently; for example, pending UART and GPIO
interrupts after one resume increment both counters. PM also classifies a deep
sleep that reaches its programmed deadline as an LPTIM wake, because some HAL
ports consume the timer's pending interrupt while measuring elapsed time.
Statistics snapshots and resets run in an interrupt-safe critical section.

Normal Cortex-M SLEEP does not require an LPTIM. A configuration whose deepest
mode is SLEEP, or a deeper policy constrained by a STOP lock, can therefore use
`xy_pm_tickless_idle()` even when `xy_pm_is_tickless_available()` is false. The
LPTIM availability check applies only when PM is about to enter STOP or deeper.

## N32 validation

PLB-N32 board validation used a double-acquired STOP lock followed by two
releases. Before shallow fallback, the lock path busy-polled approximately
33,000 times per second. The first fallback reduced this to about 151 aborts per
second near short deadlines. Moving shallow selection ahead of
`min_sleep_ms` eliminated those remaining aborts:

```text
idle=985  sleep=3  shallow=982  abort=0  locks=0/0/2/0
idle=1972 sleep=3  shallow=1969 abort=0  locks=0/0/2/0
```

With the final STOP lock released, shallow sleeping stops and LPTIM-driven STOP2
resumes. PC coverage is provided by `project/Tiny-App/tests/test_pm_tickless.c`,
including a 2 ms deadline while the STOP lock is held.

The PLB-N32 lock sequence is validation-only and disabled by default. Build with
`PM_LOCK_TEST=y` to acquire the STOP lock twice at 3 seconds and release it at 6
and 9 seconds. `PM_LOG_INTERVAL_MS` controls the cumulative PM log interval and
defaults to 60000 ms for long-run images. The LPTIM hardware maximum still caps
each individual STOP2 residency, so the main loop wakes periodically to service
the watchdog even when the log deadline is much farther away.

## PLB-N32 long-run validation

The `V1.2-tickless-accounting` image was validated for one hour on PLB-N32 with
LSE at 32768 Hz and a 60-second PM log interval. LPTIM configuration runs with
interrupts enabled so its low-speed-domain synchronization time is accounted by
SysTick; PM masks interrupts and repeats all sleep checks immediately before
STOP2 entry.

Observed results:

| Metric | Result |
|---|---:|
| Continuous run time | 3600.507 s |
| BareOS tick delta | 3600.000 s |
| Successful STOP2 entries | 1800 |
| `idle_calls` / `sleep_count` | Always equal |
| `shallow_sleep_count` | 0 |
| `abort_count` | 0 |
| PM locks | `0/0/0/0` |
| Watchdog or low-power resets | 0 |
| 10-minute drift | 81 ms, about 135 ppm |
| 1-hour drift | 507 ms, about 141 ppm |

Before moving LPTIM setup outside the deep-sleep critical section, the same
test accumulated approximately 70 ms of drift per minute, or 1169 ppm. The
accounting change reduced average unaccounted time per STOP2 entry from about
2.26 ms to 0.282 ms and improved the measured drift by approximately 88%.

This closes the basic N32 tickless scope: LSE/LPTIM wakeup, STOP2 entry and clock
restoration, elapsed-tick compensation, STOP-lock shallow fallback, periodic
watchdog service, and one-hour stability. Applications that require an accurate
wall clock should use RTC/LSE time rather than treating the scheduler tick as a
calibrated real-time clock.
