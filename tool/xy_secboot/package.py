"""SBP package builder/parser."""

from __future__ import annotations

import struct
from dataclasses import dataclass
from pathlib import Path

from .constants import PACKAGE_HEADER_SIZE, PACKAGE_MAGIC, PACKAGE_VERSION, SUITE_MARKET
from .crc import crc32
from .manifest import Manifest

_HEADER_FORMAT = "<4sHHIIII"


def align_image(image: bytes, alignment: int = 4) -> bytes:
    if alignment <= 0:
        raise ValueError("alignment must be positive")
    pad_len = (-len(image)) % alignment
    if pad_len == 0:
        return image
    return image + (b"\xFF" * pad_len)


@dataclass(slots=True)
class SecbootPackage:
    suite_id: int
    manifest: Manifest
    image: bytes
    package_crc32: int = 0

    @classmethod
    def build(cls, image: bytes, manifest: Manifest, suite_id: int = SUITE_MARKET) -> "SecbootPackage":
        if len(image) != manifest.image_size:
            raise ValueError("image length must match manifest image_size")
        if len(image) % 4 != 0:
            raise ValueError("image length must be 4-byte aligned")
        package = cls(suite_id=suite_id, manifest=manifest, image=image)
        package.package_crc32 = package.calc_crc32()
        return package

    @classmethod
    def read(cls, path: str | Path) -> "SecbootPackage":
        data = Path(path).read_bytes()
        if len(data) < PACKAGE_HEADER_SIZE:
            raise ValueError("package is too short")
        magic, version, header_len, suite_id, manifest_len, image_len, package_crc = struct.unpack(
            _HEADER_FORMAT, data[:PACKAGE_HEADER_SIZE]
        )
        if magic != PACKAGE_MAGIC:
            raise ValueError("bad package magic")
        if version != PACKAGE_VERSION:
            raise ValueError(f"unsupported package version {version}")
        if header_len != PACKAGE_HEADER_SIZE:
            raise ValueError(f"unsupported header length {header_len}")
        end_manifest = header_len + manifest_len
        end_image = end_manifest + image_len
        if end_image != len(data):
            raise ValueError("package length mismatch")
        check = bytearray(data)
        struct.pack_into("<I", check, 20, 0)
        if crc32(bytes(check)) != package_crc:
            raise ValueError("package crc32 mismatch")
        return cls(
            suite_id=suite_id,
            manifest=Manifest.unpack(data[header_len:end_manifest]),
            image=data[end_manifest:end_image],
            package_crc32=package_crc,
        )

    def write(self, path: str | Path) -> None:
        path = Path(path)
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(self.pack())

    def pack(self) -> bytes:
        manifest = self.manifest.pack()
        header = struct.pack(
            _HEADER_FORMAT,
            PACKAGE_MAGIC,
            PACKAGE_VERSION,
            PACKAGE_HEADER_SIZE,
            self.suite_id,
            len(manifest),
            len(self.image),
            0,
        )
        package_crc = crc32(header + manifest + self.image)
        header = struct.pack(
            _HEADER_FORMAT,
            PACKAGE_MAGIC,
            PACKAGE_VERSION,
            PACKAGE_HEADER_SIZE,
            self.suite_id,
            len(manifest),
            len(self.image),
            package_crc,
        )
        return header + manifest + self.image

    def calc_crc32(self) -> int:
        return crc32(self.pack()[:20] + b"\x00\x00\x00\x00" + self.pack()[24:])

    def summary(self) -> dict[str, str | int]:
        return {
            "suite_id": self.suite_id,
            "manifest_len": len(self.manifest.pack()),
            "image_len": len(self.image),
            "package_crc32": f"0x{self.package_crc32:08x}",
            **self.manifest.summary(),
        }
