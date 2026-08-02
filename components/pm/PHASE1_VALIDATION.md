# PM Phase 1 Validation

## Scope

PM Phase 1 is complete for the BareOS N32 RUN, SLEEP, and STOP2 path. The
validated scope includes:

- reference-counted CPU, SLEEP, STOP, and STANDBY locks;
- shallow SLEEP fallback while a STOP lock is held;
- LPTIM-driven STOP2 and logical tick compensation;
- pending-work checks, timeout providers, and wake hooks;
- GPIO early wake followed by a temporary shallow receive window;
- wake statistics for UART, GPIO, LPTIM, RTC, watchdog, and unknown sources;
- cumulative, maximum, and early deep-sleep residency diagnostics.

STANDBY, SHUTDOWN, native LPUART wakeup, and hardware-derived wake-reason
collection are outside Phase 1.

## Automated validation

| Target | Result |
|---|---|
| `pm_tickless_tests` | Passed |
| BareOS regression suite | 25 tests, 0 failures |
| `bareos_pc` | Built successfully |
| PLB-N32 ARM image | Built and linked successfully |

The PM unit tests cover full and early deep sleep, shallow sleep without an
LPTIM, STOP-lock fallback, lock reference counting, sleep-entry races, timeout
aggregation, wake hooks, wake-source accounting, and statistics reset.

## N32 board evidence

The one-hour LPTIM validation measured:

| Metric | Result |
|---|---:|
| Successful STOP2 entries | 1800 |
| BareOS tick delta | 3600.000 s |
| Continuous wall time | 3600.507 s |
| One-hour drift | 507 ms, about 141 ppm |
| Abort count | 0 |
| Unexpected reset | 0 |

The `V1.8-pm-residency`, security-counter 31 image subsequently measured after
eight minutes:

```text
tick=480213 sleep=240 shallow=0 abort=0
total=479369 max=1999 early=0 lptim=240
locks=0/0/0/0
```

This is about 99.82 percent measured STOP2 residency with an average deep-sleep
interval of 1997.4 ms.

PB5/EXTI5 validation demonstrated:

```text
STOP2 -> GPIO early wake -> one-second SLEEP window
      -> UART5 receives subsequent bytes -> STOP lock released -> STOP2
```

The wake byte may be lost because UART5 is not an LPUART. The test intentionally
uses the UART RX pin as a GPIO wake source and validates only subsequent UART
operation. EXTI5 is masked during the receive window so UART data transitions do
not create an interrupt storm.

## Integration contract

| Requirement | PM interface |
|---|---|
| Pending foreground work | `xy_pm_register_sleep_check()` |
| Next deadline | `xy_pm_register_timeout()` |
| Post-sleep compensation | `xy_pm_register_wake_hook()` |
| Prevent STOP temporarily | `xy_pm_acquire_lock(XY_HAL_PM_LOCK_STOP)` |
| Prevent all sleep | CPU or SLEEP lock |
| ISR wake diagnosis | `xy_pm_report_wake_sources()` |

Components must not execute WFI or STOP directly. Lock acquire/release calls
must be paired, and ordinary UARTs must not be assumed to preserve an
asynchronous first byte through STOP2.

## Deferred PM work

- unify the PM deep-sleep sequence with `xy_hal_tickless_enter()`;
- obtain actual wake causes from HAL hardware flags;
- classify abort reasons;
- reject duplicate callback registration;
- use 64-bit or rollover-aware long-term residency totals;
- validate RTC, watchdog, STANDBY, and SHUTDOWN on hardware.
