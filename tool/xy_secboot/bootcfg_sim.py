#!/usr/bin/env python3
"""Host-side simulation tests for SecBoot boot config A/B storage."""

from __future__ import annotations

import argparse
import struct
import zlib
from dataclasses import dataclass

BOOTCFG_MAGIC = 0x31434258  # 'XBC1'
BOOTCFG_FORMAT_VERSION = 1
BOOTCFG_HEADER = struct.Struct("<IHHIIIII")


@dataclass
class Header:
    magic: int
    format_version: int
    header_size: int
    seq: int
    seq_inv: int
    payload_len: int
    payload_crc32: int
    header_crc32: int


@dataclass
class LoadResult:
    copy: str
    seq: int
    payload: bytes


class SimFlash:
    def __init__(self, page_size: int = 2048) -> None:
        self.page_size = page_size
        self.data = bytearray(b"\xff" * (page_size * 2))

    def erase(self, addr: int, size: int) -> None:
        if addr % self.page_size != 0 or size % self.page_size != 0:
            raise ValueError("erase must be page aligned")
        self.data[addr : addr + size] = b"\xff" * size

    def write(self, addr: int, data: bytes) -> None:
        for offset, value in enumerate(data):
            old = self.data[addr + offset]
            if (old & value) != value:
                raise ValueError("Flash write attempted 0->1 bit transition")
            self.data[addr + offset] = value

    def read(self, addr: int, size: int) -> bytes:
        return bytes(self.data[addr : addr + size])


def crc32(data: bytes) -> int:
    return zlib.crc32(data) & 0xFFFFFFFF


def pack_header_without_crc(header: Header) -> bytes:
    return BOOTCFG_HEADER.pack(
        header.magic,
        header.format_version,
        header.header_size,
        header.seq,
        header.seq_inv,
        header.payload_len,
        header.payload_crc32,
        0,
    )


def make_header(seq: int, payload: bytes) -> bytes:
    header = Header(
        BOOTCFG_MAGIC,
        BOOTCFG_FORMAT_VERSION,
        BOOTCFG_HEADER.size,
        seq,
        (~seq) & 0xFFFFFFFF,
        len(payload),
        crc32(payload),
        0,
    )
    header.header_crc32 = crc32(pack_header_without_crc(header))
    return BOOTCFG_HEADER.pack(
        header.magic,
        header.format_version,
        header.header_size,
        header.seq,
        header.seq_inv,
        header.payload_len,
        header.payload_crc32,
        header.header_crc32,
    )


def unpack_header(data: bytes) -> Header:
    return Header(*BOOTCFG_HEADER.unpack(data[: BOOTCFG_HEADER.size]))


def read_copy(flash: SimFlash, addr: int, copy_size: int) -> tuple[int, bytes] | None:
    header = unpack_header(flash.read(addr, BOOTCFG_HEADER.size))
    if header.magic != BOOTCFG_MAGIC:
        return None
    if header.format_version != BOOTCFG_FORMAT_VERSION:
        return None
    if header.header_size != BOOTCFG_HEADER.size:
        return None
    if header.seq != ((~header.seq_inv) & 0xFFFFFFFF):
        return None
    if header.payload_len > copy_size - BOOTCFG_HEADER.size:
        return None
    if header.header_crc32 != crc32(pack_header_without_crc(header)):
        return None
    payload = flash.read(addr + BOOTCFG_HEADER.size, header.payload_len)
    if header.payload_crc32 != crc32(payload):
        return None
    return header.seq, payload


def b_newer(seq_a: int, seq_b: int) -> bool:
    return ((seq_b - seq_a) & 0xFFFFFFFF) < 0x80000000 and seq_a != seq_b


def load(flash: SimFlash, copy_size: int) -> LoadResult | None:
    a = read_copy(flash, 0, copy_size)
    b = read_copy(flash, copy_size, copy_size)
    if a and b:
        if b_newer(a[0], b[0]):
            return LoadResult("B", b[0], b[1])
        return LoadResult("A", a[0], a[1])
    if a:
        return LoadResult("A", a[0], a[1])
    if b:
        return LoadResult("B", b[0], b[1])
    return None


def save(flash: SimFlash, copy_size: int, payload: bytes, *, power_loss: str | None = None) -> None:
    if len(payload) % 4 != 0:
        raise ValueError("payload must be 4-byte aligned")
    current = load(flash, copy_size)
    seq = 1 if current is None else current.seq + 1
    dst = 0 if current is None or current.copy == "B" else copy_size
    header = make_header(seq, payload)
    flash.erase(dst, copy_size)
    if power_loss == "after-erase":
        return
    flash.write(dst + BOOTCFG_HEADER.size, payload)
    if power_loss == "after-payload":
        return
    flash.write(dst, header)


def corrupt_byte(flash: SimFlash, addr: int) -> None:
    flash.data[addr] ^= 0x01


def run_tests(page_size: int) -> None:
    flash = SimFlash(page_size)
    assert load(flash, page_size) is None

    save(flash, page_size, b"cfg1")
    result = load(flash, page_size)
    assert result == LoadResult("A", 1, b"cfg1")

    save(flash, page_size, b"cfg2")
    result = load(flash, page_size)
    assert result == LoadResult("B", 2, b"cfg2")

    corrupt_byte(flash, page_size + BOOTCFG_HEADER.size)
    result = load(flash, page_size)
    assert result == LoadResult("A", 1, b"cfg1")

    save(flash, page_size, b"cfg3", power_loss="after-erase")
    result = load(flash, page_size)
    assert result == LoadResult("A", 1, b"cfg1")

    save(flash, page_size, b"cfg4", power_loss="after-payload")
    result = load(flash, page_size)
    assert result == LoadResult("A", 1, b"cfg1")

    save(flash, page_size, b"cfg5")
    result = load(flash, page_size)
    assert result == LoadResult("B", 2, b"cfg5")


def main() -> int:
    parser = argparse.ArgumentParser(description="simulate SecBoot boot config A/B storage")
    parser.add_argument("--page-size", type=lambda text: int(text, 0), default=2048)
    args = parser.parse_args()
    run_tests(args.page_size)
    print("bootcfg A/B simulation PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
