#!/usr/bin/env python3
"""CLI entry point for XY SecBoot host tools."""

from __future__ import annotations

import argparse
import hmac
import hashlib
import json
import struct
import sys
from pathlib import Path

from .constants import APP_IMAGE_ADDR_N32, PRODUCT_ID_N32, SUITE_MARKET, UART_DEFAULT_PAYLOAD, MANIFEST_SIGNED_LEN
from .crc import crc32
from .manifest import Manifest
from .package import SecbootPackage, align_image
from .portgen import generate_port, write_sample_config
from .uart import FlashInterrupted, SecbootUartClient, available_ports, require_pyserial


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


def refresh_manifest_auth(manifest: Manifest, hmac_key: bytes | None) -> None:
    manifest.header_crc32 = 0
    if hmac_key is not None:
        manifest.signature = b"\x00" * 128
        tag = hmac.new(hmac_key, manifest.pack()[:MANIFEST_SIGNED_LEN], hashlib.sha256).digest()
        manifest.signature = tag.ljust(128, b"\x00")
    manifest.header_crc32 = crc32(manifest.pack()[:-4])


def cmd_fault_package(args: argparse.Namespace) -> int:
    package = SecbootPackage.read(args.package)
    hmac_key = read_key(args.hmac_key)

    if args.fault == "bad-image":
        if not package.image:
            raise ValueError("image is empty")
        image = bytearray(package.image)
        image[min(args.offset, len(image) - 1)] ^= 0x01
        package.image = bytes(image)
    elif args.fault == "bad-hash":
        image_hash = bytearray(package.manifest.image_hash)
        image_hash[0] ^= 0x01
        package.manifest.image_hash = bytes(image_hash)
        refresh_manifest_auth(package.manifest, hmac_key)
    elif args.fault == "bad-signature":
        signature = bytearray(package.manifest.signature)
        signature[0] ^= 0x01
        package.manifest.signature = bytes(signature)
        package.manifest.header_crc32 = crc32(package.manifest.pack()[:-4])
    elif args.fault == "bad-manifest-crc":
        package.manifest.header_crc32 ^= 0x00000001
    elif args.fault == "bad-product":
        package.manifest.product_id = args.value
        refresh_manifest_auth(package.manifest, hmac_key)
    elif args.fault == "bad-entry":
        package.manifest.entry_addr = args.value
        refresh_manifest_auth(package.manifest, hmac_key)
    elif args.fault == "old-counter":
        package.manifest.security_counter = args.value
        refresh_manifest_auth(package.manifest, hmac_key)
    elif args.fault == "bad-package-crc":
        data = bytearray(package.pack())
        stored = struct.unpack_from("<I", data, 20)[0]
        struct.pack_into("<I", data, 20, stored ^ 0x00000001)
        Path(args.output).parent.mkdir(parents=True, exist_ok=True)
        Path(args.output).write_bytes(data)
        print(f"wrote {args.output}")
        return 0
    else:  # pragma: no cover - argparse choices keep this unreachable
        raise ValueError(f"unsupported fault {args.fault}")

    package.write(args.output)
    print(f"wrote {args.output}")
    print(json.dumps(SecbootPackage.read(args.output).summary(), indent=2, ensure_ascii=False))
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
        client.recover_bootloader(args.recover_ms)
        caps = client.hello()
        print(f"CAPS payload={caps.payload.hex()}")
        try:
            ack = client.flash_package(
                package,
                args.payload,
                args.retries,
                progress=progress,
                interrupt_after_manifest=args.interrupt_after_manifest,
                interrupt_at_offset=args.interrupt_at_offset,
            )
        except FlashInterrupted as exc:
            print()
            print(str(exc))
            return 2
        print()
        print(f"END {ack.describe()}")
        if args.reset:
            print(client.reset().describe())
    return 0


def cmd_probe_transport(args: argparse.Namespace) -> int:
    package = SecbootPackage.read(args.package)
    with SecbootUartClient(args.port, args.baud, args.timeout_ms / 1000.0, args.session_id) as client:
        client.recover_bootloader(args.recover_ms)
        caps = client.hello()
        print(f"CAPS payload={caps.payload.hex()}")
        acks = client.probe_transport_fault(package, args.fault, args.payload, args.retries)
        for index, ack in enumerate(acks):
            print(f"ACK[{index}] {ack.describe()}")
    return 0


def cmd_gui(args: argparse.Namespace) -> int:
    from .gui import main as gui_main

    return gui_main()


def cmd_portgen(args: argparse.Namespace) -> int:
    if args.sample_config:
        write_sample_config(Path(args.sample_config), args.force)
        print(f"wrote sample config {args.sample_config}")
        return 0

    paths = generate_port(
        Path(args.config) if args.config else None,
        Path(args.output),
        args.force,
    )
    for path in paths:
        print(f"wrote {path}")
    return 0


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

    fault = sub.add_parser("fault-package", help="build a mutated .sbp for V1 fault injection")
    fault.add_argument("--package", required=True, help="input .sbp")
    fault.add_argument("--output", "-o", required=True, help="output mutated .sbp")
    fault.add_argument(
        "--fault",
        required=True,
        choices=[
            "bad-image",
            "bad-hash",
            "bad-signature",
            "bad-manifest-crc",
            "bad-product",
            "bad-entry",
            "old-counter",
            "bad-package-crc",
        ],
    )
    fault.add_argument("--value", type=parse_int, default=0, help="replacement value for field faults")
    fault.add_argument("--offset", type=parse_int, default=0, help="image byte offset for bad-image")
    fault.add_argument("--hmac-key", help="key used to re-authenticate field mutations when needed")
    fault.set_defaults(func=cmd_fault_package)

    ports = sub.add_parser("ports", help="list serial ports")
    ports.set_defaults(func=cmd_ports)

    flash = sub.add_parser("flash", help="flash an .sbp over UART v1")
    flash.add_argument("--port", required=True)
    flash.add_argument("--baud", type=int, default=115200)
    flash.add_argument("--package", required=True)
    flash.add_argument("--payload", type=int, default=UART_DEFAULT_PAYLOAD)
    flash.add_argument("--timeout-ms", type=int, default=1000)
    flash.add_argument("--retries", type=int, default=10)
    flash.add_argument("--recover-ms", type=int, default=0, help="send '?' before HELLO to keep bootloader in recovery")
    flash.add_argument("--session-id", type=parse_int, default=0)
    flash.add_argument("--reset", action="store_true", help="send RESET after successful END")
    flash.add_argument("--interrupt-after-manifest", action="store_true", help="stop after MANIFEST ACK without sending DATA")
    flash.add_argument("--interrupt-at-offset", type=parse_int, help="stop after DATA reaches this image offset")
    flash.set_defaults(func=cmd_flash)

    probe = sub.add_parser("probe-transport", help="probe UART DATA fault handling without sending END")
    probe.add_argument("--port", required=True)
    probe.add_argument("--baud", type=int, default=115200)
    probe.add_argument("--package", required=True)
    probe.add_argument("--payload", type=int, default=UART_DEFAULT_PAYLOAD)
    probe.add_argument("--timeout-ms", type=int, default=1000)
    probe.add_argument("--retries", type=int, default=10)
    probe.add_argument("--recover-ms", type=int, default=0, help="send '?' before HELLO to keep bootloader in recovery")
    probe.add_argument("--session-id", type=parse_int, default=0)
    probe.add_argument("--fault", required=True, choices=["duplicate-data", "bad-seq", "bad-offset"])
    probe.set_defaults(func=cmd_probe_transport)

    gui = sub.add_parser("gui", help="launch Tkinter GUI")
    gui.set_defaults(func=cmd_gui)

    portgen = sub.add_parser("portgen", help="generate a SecBoot V1 MCU porting skeleton")
    portgen.add_argument("--config", help="JSON porting config; defaults to a sample N32-like layout")
    portgen.add_argument("--output", "-o", default="build/secboot_port", help="output directory")
    portgen.add_argument("--sample-config", help="write a sample JSON config and exit")
    portgen.add_argument("--force", action="store_true", help="overwrite existing generated files")
    portgen.set_defaults(func=cmd_portgen)
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
