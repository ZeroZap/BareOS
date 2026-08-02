#!/usr/bin/env python3
"""Automate SecBoot-N32 + PLB-N32 app build/package/flash flow."""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SECBOOT_MAKE_DIR = ROOT / "project" / "Sec-boot-N32" / "Makefile"
PLB_MAKE_DIR = ROOT / "project" / "PLB -N32" / "Makefile"
PLB_PACKAGE = PLB_MAKE_DIR / "build" / "PLB.sbp"
PLB_BIN = PLB_MAKE_DIR / "build" / "PLB.bin"
DEV_HMAC_KEY = ROOT / "tool" / "xy_secboot" / "dev_hmac_key.txt"


def run(cmd: list[str], cwd: Path, *, dry_run: bool = False) -> None:
    print(f"[{cwd}] {' '.join(cmd)}")
    if dry_run:
        return
    subprocess.run(cmd, cwd=cwd, check=True)


def make_cmd(target: str | None = None, gcc_path: str | None = None) -> list[str]:
    cmd = ["make"]
    if target:
        cmd.append(target)
    if gcc_path:
        cmd.append(f"GCC_PATH={gcc_path}")
    return cmd


def plb_make_cmd(target: str | None = None,
                  gcc_path: str | None = None,
                  hmac_key: Path | None = None,
                  security_counter: int | None = None,
                  image_version: int | None = None,
                  at_selftest: bool = False) -> list[str]:
    cmd = make_cmd(target, gcc_path)
    if hmac_key is not None:
        cmd.append(f"SECBOOT_HMAC_KEY={hmac_key}")
    if security_counter is not None:
        cmd.append(f"SECBOOT_SECURITY_COUNTER={security_counter}")
    if image_version is not None:
        cmd.append(f"SECBOOT_IMAGE_VERSION={image_version}")
    if at_selftest:
        cmd.append("AT_SELFTEST=y")
    return cmd


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Build and verify PLB-N32 as SecBoot app")
    parser.add_argument("--gcc-path", help="optional ARM GCC bin path passed to make")
    parser.add_argument("--clean", action="store_true", help="run make clean before each build")
    parser.add_argument("--flash-boot", action="store_true", help="flash SecBoot-N32 bootloader with make flash")
    parser.add_argument("--flash-app-uart", action="store_true", help="flash packaged PLB app through UART5")
    parser.add_argument("--port", help="UART5 serial port for --flash-app-uart")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--payload", type=int, default=256)
    parser.add_argument("--timeout-ms", type=int, default=1000)
    parser.add_argument("--retries", type=int, default=10)
    parser.add_argument("--hmac-key", type=Path, default=DEV_HMAC_KEY,
                        help="development HMAC key for PLB app package")
    parser.add_argument("--security-counter", type=int, default=1,
                        help="package anti-rollback counter, must exceed the device counter")
    parser.add_argument("--image-version", type=int, default=1)
    parser.add_argument("--at-selftest", action="store_true",
                        help="build PLB app with the UART5 AT self-test enabled")
    parser.add_argument("--capture-log", help="optional UART4 log port to capture after flashing")
    parser.add_argument("--log-seconds", type=float, default=5.0)
    parser.add_argument("--dry-run", action="store_true", help="print commands without executing")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    try:
        if args.clean:
            run(make_cmd("clean", args.gcc_path), SECBOOT_MAKE_DIR, dry_run=args.dry_run)
        run(make_cmd(None, args.gcc_path), SECBOOT_MAKE_DIR, dry_run=args.dry_run)

        if args.flash_boot:
            run(make_cmd("flash", args.gcc_path), SECBOOT_MAKE_DIR, dry_run=args.dry_run)

        if args.clean:
            run(make_cmd("clean", args.gcc_path), PLB_MAKE_DIR, dry_run=args.dry_run)
        if not args.dry_run and not args.hmac_key.exists():
            raise FileNotFoundError(args.hmac_key)
        run(plb_make_cmd("inspect-package", args.gcc_path, args.hmac_key,
                         args.security_counter, args.image_version,
                         args.at_selftest),
            PLB_MAKE_DIR,
            dry_run=args.dry_run)

        if not args.dry_run and not PLB_PACKAGE.exists():
            raise FileNotFoundError(PLB_PACKAGE)
        if not args.dry_run:
            print(f"PLB app binary: {PLB_BIN}")
            print(f"PLB app package: {PLB_PACKAGE}")

        if args.flash_app_uart:
            if not args.port:
                raise ValueError("--port is required for --flash-app-uart")
            run(
                [
                    sys.executable,
                    str(ROOT / "tool" / "xy_secboot" / "xy_secboot.py"),
                    "flash",
                    "--port",
                    args.port,
                    "--baud",
                    str(args.baud),
                    "--package",
                    str(PLB_PACKAGE),
                    "--payload",
                    str(args.payload),
                    "--timeout-ms",
                    str(args.timeout_ms),
                    "--retries",
                    str(args.retries),
                ],
                ROOT,
                dry_run=args.dry_run,
            )

        if args.capture_log:
            run(
                [
                    sys.executable,
                    str(ROOT / "tool" / "serial_log.py"),
                    "--port",
                    args.capture_log,
                    "--baud",
                    str(args.baud),
                    "--timeout",
                    str(args.log_seconds),
                ],
                ROOT,
                dry_run=args.dry_run,
            )
    except subprocess.CalledProcessError as exc:
        return exc.returncode
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
