# Tool

Command-line helpers for BareOS board automation.

## Serial Log Capture

`serial_log.py` captures UART logs and returns a test-friendly exit code.

Install dependency:

```sh
python -m pip install pyserial
```

List ports:

```sh
python tool/serial_log.py --list
```

Capture UART4 debug log at 115200 baud for 10 seconds:

```sh
python tool/serial_log.py --port COM8 --baud 115200 --timeout 10 --output logs/uart4.log
```

Use regex checks for automation:

```sh
python tool/serial_log.py --port COM8 --timeout 5 --expect "PLB" --fail-on "HardFault|ASSERT|ERROR"
```

Exit codes:

- `0`: capture completed and checks passed.
- `1`: expected pattern missing, fail pattern matched, or no serial port found for `--list`.
- `2`: usage/dependency error.
