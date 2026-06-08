#!/usr/bin/env python3
"""CLI entry point for XY SecBoot host tools."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

from .constants import APP_IMAGE_ADDR_N32, PRODUCT_ID_N32, SUITE_MARKET, UART_DEFAULT_PAYLOAD
from .manifest import Manifest
from .package import SecbootPackage, align_image
from .uart import SecbootUartClient, available_ports, require_pyserial


def parse_int(text: str) -> int:
    return int(text, 0)


def read_key(path: str | None) -> bytes | None:
    if not path:
        return None
    return Path(path).read_bytes().strip()


def cmd_pack(args: argparse.Namespace) -> int:
    raw_image = Path(args.input).read_bytes()
    image = align_image(raw_image)
    manifest = Manifest.build(
        image,
        product_id=args.product_id,
        image_addr=args.image_addr,
        entry_addr=args.entry_addr,
        image_version=args.image_version,
        min_boot_version=args.min_boot_version,
        security_counter=args.security_counter,
        key_id=args.key_id.encode("utf-8"),
        hmac_key=read_key(args.hmac_key),
    )
    package = SecbootPackage.build(image, manifest, suite_id=args.suite_id)
    package.write(args.output)
    print(f"wrote {args.output}")
    if len(image) != len(raw_image):
        print(f"padded image from {len(raw_image)} to {len(image)} bytes for 4-byte Flash writes")
    print(json.dumps(package.summary(), indent=2, ensure_ascii=False))
    return 0


def cmd_inspect(args: argparse.Namespace) -> int:
    package = SecbootPackage.read(args.package)
    print(json.dumps(package.summary(), indent=2, ensure_ascii=False))
    return 0


def cmd_ports(args: argparse.Namespace) -> int:
    require_pyserial()
    ports = available_ports()
    if not ports:
        print("No serial ports found")
        return 1
    for port in ports:
        print(port)
    return 0


def cmd_flash(args: argparse.Namespace) -> int:
    package = SecbootPackage.read(args.package)

    def progress(done: int, total: int) -> None:
        percent = 100.0 if total == 0 else done * 100.0 / total
        print(f"\rDATA {done}/{total} {percent:5.1f}%", end="", flush=True)

    with SecbootUartClient(args.port, args.baud, args.timeout_ms / 1000.0, args.session_id) as client:
        caps = client.hello()
        print(f"CAPS payload={caps.payload.hex()}")
        ack = client.flash_package(package, args.payload, args.retries, progress=progress)
        print()
        print(f"END {ack.describe()}")
        if args.reset:
            print(client.reset().describe())
    return 0


def cmd_gui(args: argparse.Namespace) -> int:
    from .gui import main as gui_main

    return gui_main()


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="XY SecBoot host tool")
    sub = parser.add_subparsers(dest="command", required=True)

    pack = sub.add_parser("pack", help="build an .sbp package from app.bin")
    pack.add_argument("--input", "-i", required=True, help="input app .bin")
    pack.add_argument("--output", "-o", required=True, help="output .sbp")
    pack.add_argument("--product-id", type=parse_int, default=PRODUCT_ID_N32)
    pack.add_argument("--image-addr", type=parse_int, default=APP_IMAGE_ADDR_N32)
    pack.add_argument("--entry-addr", type=parse_int, default=APP_IMAGE_ADDR_N32)
    pack.add_argument("--image-version", type=parse_int, default=1)
    pack.add_argument("--min-boot-version", type=parse_int, default=0)
    pack.add_argument("--security-counter", type=parse_int, default=1)
    pack.add_argument("--suite-id", type=parse_int, default=SUITE_MARKET)
    pack.add_argument("--key-id", default="dev-key-01")
    pack.add_argument("--hmac-key", help="development HMAC key file; do not use production secrets")
    pack.set_defaults(func=cmd_pack)

    inspect = sub.add_parser("inspect", help="inspect an .sbp package")
    inspect.add_argument("package")
    inspect.set_defaults(func=cmd_inspect)

    ports = sub.add_parser("ports", help="list serial ports")
    ports.set_defaults(func=cmd_ports)

    flash = sub.add_parser("flash", help="flash an .sbp over UART v1")
    flash.add_argument("--port", required=True)
    flash.add_argument("--baud", type=int, default=115200)
    flash.add_argument("--package", required=True)
    flash.add_argument("--payload", type=int, default=UART_DEFAULT_PAYLOAD)
    flash.add_argument("--timeout-ms", type=int, default=1000)
    flash.add_argument("--retries", type=int, default=10)
    flash.add_argument("--session-id", type=parse_int, default=0)
    flash.add_argument("--reset", action="store_true", help="send RESET after successful END")
    flash.set_defaults(func=cmd_flash)

    gui = sub.add_parser("gui", help="launch Tkinter GUI")
    gui.set_defaults(func=cmd_gui)
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    try:
        return args.func(args)
    except Exception as exc:  # pragma: no cover - CLI boundary
        print(f"error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
