"""Manifest builder/parser for xy_secboot_manifest_t."""

from __future__ import annotations

import hashlib
import hmac
import struct
from dataclasses import dataclass

from .constants import (
    IMAGE_TYPE_APP,
    MANIFEST_MAGIC,
    MANIFEST_SIGNED_LEN,
    MANIFEST_SIZE,
    MANIFEST_VERSION,
)
from .crc import crc32

_FORMAT = "<IHHIIIIIIII16s16s64s128sI"


@dataclass(slots=True)
class Manifest:
    magic: int
    header_version: int
    header_len: int
    product_id: int
    image_type: int
    image_addr: int
    image_size: int
    entry_addr: int
    image_version: int
    min_boot_version: int
    security_counter: int
    key_id: bytes
    nonce: bytes
    image_hash: bytes
    signature: bytes
    header_crc32: int

    @classmethod
    def build(
        cls,
        image: bytes,
        *,
        product_id: int,
        image_addr: int,
        entry_addr: int,
        image_version: int,
        security_counter: int,
        min_boot_version: int = 0,
        key_id: bytes = b"dev-key-01",
        hmac_key: bytes | None = None,
    ) -> "Manifest":
        image_hash = hashlib.sha256(image).digest().ljust(64, b"\x00")
        manifest = cls(
            magic=MANIFEST_MAGIC,
            header_version=MANIFEST_VERSION,
            header_len=MANIFEST_SIZE,
            product_id=product_id,
            image_type=IMAGE_TYPE_APP,
            image_addr=image_addr,
            image_size=len(image),
            entry_addr=entry_addr,
            image_version=image_version,
            min_boot_version=min_boot_version,
            security_counter=security_counter,
            key_id=key_id[:16].ljust(16, b"\x00"),
            nonce=b"\x00" * 16,
            image_hash=image_hash,
            signature=b"\x00" * 128,
            header_crc32=0,
        )
        if hmac_key is not None:
            tag = hmac.new(hmac_key, manifest.pack()[:MANIFEST_SIGNED_LEN], hashlib.sha256).digest()
            manifest.signature = tag.ljust(128, b"\x00")
        manifest.header_crc32 = crc32(manifest.pack()[:-4])
        return manifest

    @classmethod
    def unpack(cls, data: bytes) -> "Manifest":
        if len(data) != MANIFEST_SIZE:
            raise ValueError(f"manifest size must be {MANIFEST_SIZE}, got {len(data)}")
        return cls(*struct.unpack(_FORMAT, data))

    def pack(self) -> bytes:
        return struct.pack(
            _FORMAT,
            self.magic,
            self.header_version,
            self.header_len,
            self.product_id,
            self.image_type,
            self.image_addr,
            self.image_size,
            self.entry_addr,
            self.image_version,
            self.min_boot_version,
            self.security_counter,
            self.key_id[:16].ljust(16, b"\x00"),
            self.nonce[:16].ljust(16, b"\x00"),
            self.image_hash[:64].ljust(64, b"\x00"),
            self.signature[:128].ljust(128, b"\x00"),
            self.header_crc32,
        )

    def summary(self) -> dict[str, str | int]:
        return {
            "magic": f"0x{self.magic:08x}",
            "header_version": self.header_version,
            "header_len": self.header_len,
            "product_id": f"0x{self.product_id:08x}",
            "image_type": self.image_type,
            "image_addr": f"0x{self.image_addr:08x}",
            "image_size": self.image_size,
            "entry_addr": f"0x{self.entry_addr:08x}",
            "image_version": self.image_version,
            "min_boot_version": self.min_boot_version,
            "security_counter": self.security_counter,
            "key_id": self.key_id.rstrip(b"\x00").decode("utf-8", errors="replace"),
            "image_hash_sha256": self.image_hash[:32].hex(),
            "signature_head": self.signature[:32].hex(),
            "header_crc32": f"0x{self.header_crc32:08x}",
        }
