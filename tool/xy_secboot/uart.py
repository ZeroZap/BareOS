"""UART transport v1 host implementation."""

from __future__ import annotations

import struct
import time
from dataclasses import dataclass

try:
    import serial
    from serial.tools import list_ports
except ImportError:  # pragma: no cover - runtime environment check
    serial = None
    list_ports = None

from .constants import (
    PACKET_NAMES,
    REASON_NAMES,
    UART_ACK_SIZE,
    UART_DEFAULT_PAYLOAD,
    UART_HEADER_SIZE,
    UART_MAGIC,
    UART_VERSION,
    PacketType,
    Reason,
)
from .crc import crc16_ccitt, crc32
from .package import SecbootPackage


@dataclass(slots=True)
class Frame:
    type: int
    seq: int = 0
    session_id: int = 0
    offset: int = 0
    payload: bytes = b""
    flags: int = 0

    def pack(self) -> bytes:
        header = bytearray(UART_HEADER_SIZE)
        header[0:2] = UART_MAGIC
        header[2] = UART_VERSION
        header[3] = self.type & 0xFF
        header[4] = self.flags & 0xFF
        struct.pack_into("<HIIH", header, 6, self.seq & 0xFFFF, self.session_id, self.offset, len(self.payload))
        struct.pack_into("<H", header, 18, crc16_ccitt(bytes(header)))
        return bytes(header) + self.payload + struct.pack("<I", crc32(self.payload))

    @classmethod
    def unpack(cls, header: bytes, payload: bytes, payload_crc: bytes) -> "Frame":
        if len(header) != UART_HEADER_SIZE:
            raise ValueError("bad header length")
        if header[0:2] != UART_MAGIC:
            raise ValueError("bad frame magic")
        if header[2] != UART_VERSION:
            raise ValueError(f"unsupported UART version {header[2]}")
        expected_header_crc = struct.unpack_from("<H", header, 18)[0]
        check = bytearray(header)
        check[18] = 0
        check[19] = 0
        if crc16_ccitt(bytes(check)) != expected_header_crc:
            raise ValueError("bad header crc")
        if crc32(payload) != struct.unpack("<I", payload_crc)[0]:
            raise ValueError("bad payload crc")
        seq, session_id, offset, length = struct.unpack_from("<HIIH", header, 6)
        if length != len(payload):
            raise ValueError("payload length mismatch")
        return cls(type=header[3], flags=header[4], seq=seq, session_id=session_id, offset=offset, payload=payload)


@dataclass(slots=True)
class Ack:
    type: int
    ack_seq: int
    reason: int
    next_offset: int
    detail: int

    @property
    def ok(self) -> bool:
        return self.type == PacketType.ACK and self.reason == Reason.OK

    def describe(self) -> str:
        pkt = PACKET_NAMES.get(self.type, str(self.type))
        reason = REASON_NAMES.get(self.reason, str(self.reason))
        return f"{pkt} seq={self.ack_seq} reason={reason} next=0x{self.next_offset:x} detail=0x{self.detail:x}"


def require_pyserial() -> None:
    if serial is None:
        raise RuntimeError("pyserial is required. Install with: python -m pip install pyserial")


def available_ports() -> list[str]:
    require_pyserial()
    return [port.device for port in list_ports.comports()]


class SecbootUartClient:
    def __init__(self, port: str, baud: int = 115200, timeout: float = 1.0, session_id: int = 0) -> None:
        require_pyserial()
        self.port = port
        self.baud = baud
        self.timeout = timeout
        self.session_id = session_id
        self.ser = None
        self.max_payload = UART_DEFAULT_PAYLOAD

    def __enter__(self) -> "SecbootUartClient":
        self.open()
        return self

    def __exit__(self, exc_type, exc, tb) -> None:
        self.close()

    def open(self) -> None:
        self.ser = serial.Serial(self.port, self.baud, timeout=self.timeout)
        self.ser.reset_input_buffer()

    def close(self) -> None:
        if self.ser is not None:
            self.ser.close()
            self.ser = None

    def write_frame(self, frame: Frame) -> None:
        if self.ser is None:
            raise RuntimeError("serial port is not open")
        self.ser.write(frame.pack())
        self.ser.flush()

    def read_frame(self, timeout: float | None = None) -> Frame:
        if self.ser is None:
            raise RuntimeError("serial port is not open")
        end = time.monotonic() + (self.timeout if timeout is None else timeout)
        while time.monotonic() < end:
            b = self.ser.read(1)
            if not b:
                continue
            if b != UART_MAGIC[:1]:
                continue
            second = self.ser.read(1)
            if second != UART_MAGIC[1:2]:
                continue
            rest = self.ser.read(UART_HEADER_SIZE - 2)
            if len(rest) != UART_HEADER_SIZE - 2:
                continue
            header = b + second + rest
            length = struct.unpack_from("<H", header, 16)[0]
            payload = self.ser.read(length)
            payload_crc = self.ser.read(4)
            if len(payload) != length or len(payload_crc) != 4:
                continue
            return Frame.unpack(header, payload, payload_crc)
        raise TimeoutError("timeout waiting for secboot frame")

    def read_ack(self, timeout: float | None = None) -> Ack:
        frame = self.read_frame(timeout)
        if frame.type not in (PacketType.ACK, PacketType.NACK, PacketType.ERROR):
            raise RuntimeError(f"expected ACK/NACK/ERROR, got {PACKET_NAMES.get(frame.type, frame.type)}")
        if len(frame.payload) != UART_ACK_SIZE:
            raise RuntimeError("bad ACK payload length")
        ack_seq, reason, next_offset, detail = struct.unpack("<HHII", frame.payload)
        return Ack(frame.type, ack_seq, reason, next_offset, detail)

    def hello(self) -> Frame:
        self.write_frame(Frame(PacketType.HELLO, seq=0, session_id=self.session_id))
        caps = self.read_frame()
        if caps.type != PacketType.CAPS:
            raise RuntimeError(f"expected CAPS, got {PACKET_NAMES.get(caps.type, caps.type)}")
        if len(caps.payload) >= 4:
            self.max_payload = struct.unpack_from("<H", caps.payload, 2)[0]
        return caps

    def send_with_ack(self, frame: Frame, retries: int = 10) -> Ack:
        last_ack: Ack | None = None
        for _ in range(retries):
            self.write_frame(frame)
            try:
                ack = self.read_ack()
            except TimeoutError:
                continue
            last_ack = ack
            if ack.ok:
                return ack
            if ack.reason in (Reason.BAD_HEADER_CRC, Reason.BAD_PAYLOAD_CRC, Reason.BUSY):
                continue
            return ack
        if last_ack is not None:
            return last_ack
        raise TimeoutError("no ACK from bootloader")

    def flash_package(self, package: SecbootPackage, payload_size: int = UART_DEFAULT_PAYLOAD, retries: int = 10, progress=None) -> Ack:
        payload_size = min(payload_size, self.max_payload or UART_DEFAULT_PAYLOAD)
        if payload_size <= 0:
            raise ValueError("payload size must be positive")
        if payload_size % 4 != 0:
            raise ValueError("payload size must be 4-byte aligned")
        seq = 1
        manifest_ack = self.send_with_ack(
            Frame(PacketType.MANIFEST, seq=seq, session_id=self.session_id, payload=package.manifest.pack()), retries
        )
        if not manifest_ack.ok:
            raise RuntimeError(manifest_ack.describe())
        seq = (seq + 1) & 0xFFFF
        offset = 0
        image = package.image
        if len(image) % 4 != 0:
            raise ValueError("package image length must be 4-byte aligned")
        while offset < len(image):
            chunk = image[offset : offset + payload_size]
            ack = self.send_with_ack(
                Frame(PacketType.DATA, seq=seq, session_id=self.session_id, offset=offset, payload=chunk), retries
            )
            if not ack.ok:
                raise RuntimeError(ack.describe())
            offset += len(chunk)
            seq = (seq + 1) & 0xFFFF
            if progress is not None:
                progress(min(offset, len(image)), len(image))
        end_ack = self.send_with_ack(Frame(PacketType.END, seq=seq, session_id=self.session_id, offset=offset), retries)
        if not end_ack.ok:
            raise RuntimeError(end_ack.describe())
        return end_ack

    def abort(self) -> Ack:
        self.write_frame(Frame(PacketType.ABORT, session_id=self.session_id))
        return self.read_ack()

    def reset(self) -> Ack:
        self.write_frame(Frame(PacketType.RESET, session_id=self.session_id))
        return self.read_ack()
