#!/usr/bin/env python3
"""Serial AT server simulator for BareOS AT client validation."""

from __future__ import annotations

import argparse
import re
import sys
import time
from dataclasses import dataclass, field
from typing import Callable, Pattern

try:
    import serial
    from serial.tools import list_ports
except ImportError:  # pragma: no cover - runtime environment check
    serial = None
    list_ports = None


WriteFn = Callable[[bytes], None]
LogFn = Callable[[str], None]


def decode_escaped(value: str) -> bytes:
    """Decode CLI text such as ``+URC\r\n`` without changing non-ASCII text."""
    return value.encode("utf-8").decode("unicode_escape").encode("latin1")


def format_bytes(data: bytes) -> str:
    text = data.decode("ascii", errors="backslashreplace")
    return text.replace("\r", "\\r").replace("\n", "\\n")


@dataclass
class SimulatorConfig:
    profile: str = "ec2x"
    echo: bool = False
    response_delay_ms: int = 0
    fragment_size: int = 0
    fragment_delay_ms: int = 0
    drop_commands: list[Pattern[str]] = field(default_factory=list)
    error_commands: list[Pattern[str]] = field(default_factory=list)


class ATServerSimulator:
    """Incremental AT command and prompt-data protocol simulator."""

    def __init__(
        self,
        write: WriteFn,
        config: SimulatorConfig | None = None,
        log: LogFn | None = None,
        sleep: Callable[[float], None] = time.sleep,
    ) -> None:
        self.write = write
        self.config = config or SimulatorConfig()
        self.log = log or (lambda _message: None)
        self.sleep = sleep
        self.command_buffer = bytearray()
        self.raw_buffer = bytearray()
        self.raw_remaining = 0
        self.raw_success = b""
        self.skip_lf = False
        self.commands: list[str] = []
        self.payloads: list[bytes] = []

    def feed(self, data: bytes) -> None:
        """Consume arbitrary serial chunks, including command and binary data."""
        offset = 0
        while offset < len(data):
            if self.skip_lf:
                self.skip_lf = False
                if data[offset] == 0x0A:
                    offset += 1
                    continue

            if self.raw_remaining:
                count = min(self.raw_remaining, len(data) - offset)
                self.raw_buffer.extend(data[offset : offset + count])
                self.raw_remaining -= count
                offset += count
                if self.raw_remaining == 0:
                    payload = bytes(self.raw_buffer)
                    self.payloads.append(payload)
                    self.log(f"RX DATA ({len(payload)}): {payload.hex(' ')}")
                    self.raw_buffer.clear()
                    self._respond(self.raw_success)
                continue

            byte = data[offset]
            offset += 1
            if byte in (0x0D, 0x0A):
                if self.command_buffer:
                    raw_command = bytes(self.command_buffer)
                    self.command_buffer.clear()
                    self.skip_lf = byte == 0x0D
                    self._handle_command(raw_command.decode("ascii", errors="replace"))
            else:
                self.command_buffer.append(byte)

    def inject(self, data: bytes) -> None:
        self.log(f"TX URC: {format_bytes(data)}")
        self._write_fragmented(data)

    def _matches(self, patterns: list[Pattern[str]], command: str) -> bool:
        return any(pattern.search(command) for pattern in patterns)

    def _handle_command(self, command: str) -> None:
        upper = command.upper()
        at_offset = upper.find("AT")
        if at_offset > 0:
            noise = command[:at_offset]
            command = command[at_offset:]
            upper = command.upper()
            self.log(f"RX NOISE: {noise.encode('ascii', errors='backslashreplace')!r}")
            self.log(f"RX RESYNC CMD: {command}")

        if not upper.startswith("AT"):
            self.log(f"RX CMD: {command}")
            self.log("TX: <ignored non-AT input>")
            return

        self.commands.append(command)
        self.log(f"RX CMD: {command}")

        if self.config.echo:
            self._respond(command.encode("ascii", errors="replace") + b"\r\n")

        if self._matches(self.config.drop_commands, command):
            self.log("TX: <dropped>")
            return
        if self._matches(self.config.error_commands, command):
            self._respond(b"\r\nERROR\r\n")
            return

        if upper == "ATE0":
            self.config.echo = False
            self._respond(b"\r\nOK\r\n")
        elif upper == "ATE1":
            self._respond(b"\r\nOK\r\n")
            self.config.echo = True
        elif upper in {
            "AT",
            "AT+CMEE=2",
            "AT+CMGF=1",
            "AT+CNMI=2,2,0,0,0",
            "AT+CWMODE=1",
            "AT+CIPMUX=1",
        }:
            self._respond(b"\r\nOK\r\n")
        elif upper == "AT+CSQ":
            self._respond(b"\r\n+CSQ: 18,0\r\n\r\nOK\r\n")
        elif upper == "AT+CEREG?":
            self._respond(b"\r\n+CEREG: 0,1\r\n\r\nOK\r\n")
        elif upper == "AT+CREG?":
            self._respond(b"\r\n+CREG: 0,1\r\n\r\nOK\r\n")
        elif upper == "AT+CIMI":
            self._respond(b"\r\n460001234567890\r\n\r\nOK\r\n")
        elif upper in {"AT+CCID", "AT+ICCID"}:
            self._respond(b"\r\n89860012345678901234\r\n\r\nOK\r\n")
        elif match := re.fullmatch(r'AT\+QIOPEN=\d+,(\d+),.+', command, re.IGNORECASE):
            link = int(match.group(1))
            self._respond(b"\r\nOK\r\n")
            self._respond(f'\r\n+QIOPEN: {link},0\r\n'.encode("ascii"))
        elif match := re.fullmatch(r"AT\+QISEND=(\d+),(\d+)", command, re.IGNORECASE):
            self._start_raw(int(match.group(2)), b"\r\nSEND OK\r\n")
        elif match := re.fullmatch(r"AT\+CIPSEND=(\d+),(\d+)", command, re.IGNORECASE):
            self._start_raw(int(match.group(2)), b"\r\nSEND OK\r\n")
        elif upper.startswith("AT+QICLOSE=") or upper.startswith("AT+CIPCLOSE="):
            self._respond(b"\r\nOK\r\n")
        elif upper.startswith("AT+CWJAP=") or upper.startswith("AT+CIPSTART="):
            self._respond(b"\r\nOK\r\n")
        elif upper == "AT+SIMURC=RECV":
            self._respond(b"\r\nOK\r\n")
            self.inject(self.receive_urc(0, b"HELLO"))
        elif upper == "AT+SIMURC=CLOSED":
            self._respond(b"\r\nOK\r\n")
            self.inject(b'\r\n+QIURC: "closed",0\r\n')
        else:
            self._respond(b"\r\nERROR\r\n")

    def _start_raw(self, length: int, success: bytes) -> None:
        self.raw_remaining = length
        self.raw_success = success
        self.raw_buffer.clear()
        self._respond(b">")
        if length == 0:
            self.payloads.append(b"")
            self._respond(success)

    def receive_urc(self, link: int, payload: bytes) -> bytes:
        if self.config.profile == "sim76":
            return f"\r\n+RECEIVE,{link},{len(payload)}:".encode("ascii") + payload
        if self.config.profile == "esp_at":
            return f"\r\n+IPD,{link},{len(payload)}:".encode("ascii") + payload
        return (
            f'\r\n+QIURC: "recv",{link},{len(payload)}\n'.encode("ascii")
            + payload
            + b"\r\n"
        )

    def _respond(self, data: bytes) -> None:
        if self.config.response_delay_ms:
            self.sleep(self.config.response_delay_ms / 1000.0)
        self.log(f"TX: {format_bytes(data)}")
        self._write_fragmented(data)

    def _write_fragmented(self, data: bytes) -> None:
        size = self.config.fragment_size
        if size <= 0:
            self.write(data)
            return
        for offset in range(0, len(data), size):
            self.write(data[offset : offset + size])
            if self.config.fragment_delay_ms and offset + size < len(data):
                self.sleep(self.config.fragment_delay_ms / 1000.0)


def compile_patterns(values: list[str]) -> list[Pattern[str]]:
    try:
        return [re.compile(value, re.IGNORECASE) for value in values]
    except re.error as exc:
        raise ValueError(f"invalid command regex: {exc}") from exc


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Simulate an AT server over a serial port.")
    parser.add_argument("--list", action="store_true", help="list serial ports and exit")
    parser.add_argument("--port", help="serial port, for example COM8 or /dev/ttyUSB0")
    parser.add_argument("--baud", type=int, default=115200, help="baud rate, default: 115200")
    parser.add_argument(
        "--profile",
        choices=("ec2x", "sim76", "esp_at"),
        default="ec2x",
        help="binary receive URC format, default: ec2x",
    )
    parser.add_argument("--echo", action="store_true", help="echo commands like a modem")
    parser.add_argument("--response-delay-ms", type=int, default=0, help="delay every response")
    parser.add_argument("--fragment-size", type=int, default=0, help="split TX into N-byte writes")
    parser.add_argument("--fragment-delay-ms", type=int, default=0, help="delay between fragments")
    parser.add_argument("--drop-command", action="append", default=[], metavar="REGEX", help="drop matching responses")
    parser.add_argument("--error-command", action="append", default=[], metavar="REGEX", help="reply ERROR to matches")
    parser.add_argument("--urc", action="append", default=[], help=r"periodic URC text; supports \r and \n")
    parser.add_argument("--urc-interval", type=float, default=0.0, help="seconds between periodic URCs")
    parser.add_argument("--duration", type=float, default=0.0, help="stop after N seconds; 0 runs until Ctrl-C")
    parser.add_argument("--expect-command", action="append", default=[], metavar="REGEX", help="required received command")
    return parser.parse_args()


def require_pyserial() -> None:
    if serial is None:
        print("pyserial is required. Install with: python -m pip install pyserial", file=sys.stderr)
        raise SystemExit(2)


def show_ports() -> int:
    require_pyserial()
    ports = list(list_ports.comports())
    if not ports:
        print("No serial ports found")
        return 1
    for port in ports:
        print(f"{port.device}\t{port.description or ''}\t{port.hwid or ''}")
    return 0


def run(args: argparse.Namespace) -> int:
    require_pyserial()
    try:
        config = SimulatorConfig(
            profile=args.profile,
            echo=args.echo,
            response_delay_ms=max(args.response_delay_ms, 0),
            fragment_size=max(args.fragment_size, 0),
            fragment_delay_ms=max(args.fragment_delay_ms, 0),
            drop_commands=compile_patterns(args.drop_command),
            error_commands=compile_patterns(args.error_command),
        )
        expected = compile_patterns(args.expect_command)
        urcs = [decode_escaped(value) for value in args.urc]
    except (ValueError, UnicodeError) as exc:
        print(str(exc), file=sys.stderr)
        return 2

    start = time.monotonic()
    next_urc = start + args.urc_interval if urcs and args.urc_interval > 0 else float("inf")
    urc_index = 0

    with serial.Serial(args.port, args.baud, timeout=0.05) as port:
        simulator = ATServerSimulator(port.write, config, lambda message: print(message, flush=True))
        print(f"AT server listening on {args.port} at {args.baud} baud ({args.profile})")
        try:
            while args.duration <= 0 or time.monotonic() - start < args.duration:
                data = port.read(4096)
                if data:
                    simulator.feed(data)
                now = time.monotonic()
                if now >= next_urc:
                    simulator.inject(urcs[urc_index % len(urcs)])
                    urc_index += 1
                    next_urc = now + args.urc_interval
        except KeyboardInterrupt:
            print("Stopped")

    missing = [pattern.pattern for pattern in expected if not any(pattern.search(cmd) for cmd in simulator.commands)]
    if missing:
        print(f"Missing expected commands: {', '.join(missing)}", file=sys.stderr)
        return 1
    print(f"Summary: {len(simulator.commands)} commands, {len(simulator.payloads)} payloads")
    return 0


def main() -> int:
    args = parse_args()
    if args.list:
        return show_ports()
    if not args.port:
        print("--port is required unless --list is used", file=sys.stderr)
        return 2
    return run(args)


if __name__ == "__main__":
    raise SystemExit(main())
