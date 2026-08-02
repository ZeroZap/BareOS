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

## AT Server Simulator

`at_server_sim/at_server_sim.py` emulates EC2X, SIM76, or ESP-AT command responses over a serial port for PLB-N32 AT client validation. It supports prompt/data transfers, URCs, fragmented responses, timeout injection, and error injection.

```sh
python -m pip install -r tool/at_server_sim/requirements.txt
python tool/at_server_sim/at_server_sim.py --port COM8 --baud 115200 --profile ec2x
```

See `tool/at_server_sim/README.md` for wiring and validation scenarios.
