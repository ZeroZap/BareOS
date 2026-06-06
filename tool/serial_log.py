#!/usr/bin/env python3
"""Command-line serial log capture helper for automated tests."""

from __future__ import annotations

import argparse
import re
import sys
import time
from pathlib import Path

try:
    import serial
    from serial.tools import list_ports
except ImportError:  # pragma: no cover - runtime environment check
    serial = None
    list_ports = None


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Capture serial logs for BareOS board automation."
    )
    parser.add_argument(
        "--list",
        action="store_true",
        help="list available serial ports and exit",
    )
    parser.add_argument("--port", help="serial port, for example COM8 or /dev/ttyUSB0")
    parser.add_argument("--baud", type=int, default=115200, help="baud rate, default: 115200")
    parser.add_argument("--timeout", type=float, default=10.0, help="capture seconds, default: 10")
    parser.add_argument("--idle-timeout", type=float, default=0.0, help="stop after N seconds without data; 0 disables")
    parser.add_argument("--output", help="write captured log to file")
    parser.add_argument("--append", action="store_true", help="append to output file instead of overwriting")
    parser.add_argument("--expect", action="append", default=[], help="regex that must appear in captured log; can repeat")
    parser.add_argument("--fail-on", action="append", default=[], help="regex that fails the run if present; can repeat")
    parser.add_argument("--hex", action="store_true", help="echo captured bytes as hex instead of text")
    parser.add_argument("--no-echo", action="store_true", help="do not echo serial data to stdout")
    return parser.parse_args()


def require_pyserial() -> None:
    if serial is None:
        print("pyserial is required. Install with: python -m pip install pyserial", file=sys.stderr)
        raise SystemExit(2)


def list_serial_ports() -> int:
    require_pyserial()
    ports = list(list_ports.comports())
    if not ports:
        print("No serial ports found")
        return 1
    for port in ports:
        desc = port.description or ""
        hwid = port.hwid or ""
        print(f"{port.device}\t{desc}\t{hwid}")
    return 0


def capture(args: argparse.Namespace) -> tuple[int, str]:
    require_pyserial()

    end_time = time.monotonic() + args.timeout
    last_data_time = time.monotonic()
    chunks: list[bytes] = []

    with serial.Serial(args.port, args.baud, timeout=0.1) as ser:
        ser.reset_input_buffer()
        while time.monotonic() < end_time:
            data = ser.read(4096)
            if data:
                chunks.append(data)
                last_data_time = time.monotonic()
                if not args.no_echo:
                    if args.hex:
                        print(data.hex(" "))
                    else:
                        sys.stdout.buffer.write(data)
                        sys.stdout.buffer.flush()
            elif args.idle_timeout > 0 and (time.monotonic() - last_data_time) >= args.idle_timeout:
                break

    raw = b"".join(chunks)
    text = raw.decode("utf-8", errors="replace")

    if args.output:
        path = Path(args.output)
        mode = "a" if args.append else "w"
        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open(mode, encoding="utf-8", newline="") as fp:
            fp.write(text)

    for pattern in args.fail_on:
        if re.search(pattern, text, re.MULTILINE):
            print(f"fail-on matched: {pattern}", file=sys.stderr)
            return 1, text

    for pattern in args.expect:
        if not re.search(pattern, text, re.MULTILINE):
            print(f"expect not found: {pattern}", file=sys.stderr)
            return 1, text

    return 0, text


def main() -> int:
    args = parse_args()
    if args.list:
        return list_serial_ports()
    if not args.port:
        print("--port is required unless --list is used", file=sys.stderr)
        return 2
    code, _ = capture(args)
    return code


if __name__ == "__main__":
    raise SystemExit(main())
