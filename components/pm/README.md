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

For shallow sleep, `last_planned_ms` and `last_elapsed_ms` remain zero because
SysTick continues to account for time. A growing shallow count with a stable
abort count is the expected result while a STOP lock is held.

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
