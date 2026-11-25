"""UART frame codec shared by the monitor and its tests.

Wire layout, little-endian:
  AA 55 | version | type | seq u16 | len u8 | payload | crc16
CRC-16/CCITT-FALSE covers version through payload.
"""

from __future__ import annotations

import struct
from dataclasses import dataclass, field

SOF = b"\xAA\x55"
VERSION = 1
MAX_PAYLOAD = 48

FRAME_ENV = 0x01
FRAME_IMU = 0x02
FRAME_CAN = 0x03
FRAME_WDG = 0x10
FRAME_LOG = 0x11

TASK_NAMES = {
    0: "env",
    1: "imu",
    2: "telem",
    3: "supervisor",
}


def crc16_ccitt(data: bytes) -> int:
    crc = 0xFFFF
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return crc


@dataclass
class Frame:
    type: int
    seq: int
    payload: bytes
    kind: str
    fields: dict = field(default_factory=dict)

    def text(self) -> str:
        if self.kind == "env":
            f = self.fields
            return (
                f"env seq={self.seq} t={f['temp_c']:.2f}C "
                f"p={f['pressure_pa']}Pa rh={f['humidity_pct']:.2f}% "
                f"t_ms={f['time_ms']}"
            )
        if self.kind == "imu":
            f = self.fields
            return (
                f"imu seq={self.seq} ax={f['ax_mg']} ay={f['ay_mg']} az={f['az_mg']} mg "
                f"gx={f['gx_dps']:.1f} gy={f['gy_dps']:.1f} gz={f['gz_dps']:.1f} dps "
                f"t_ms={f['time_ms']}"
            )
        if self.kind == "can":
            f = self.fields
            data = " ".join(f"{b:02X}" for b in f["data"][: f["dlc"]])
            return f"can seq={self.seq} id=0x{f['id']:03X} dlc={f['dlc']} {data}"
        if self.kind == "wdg":
            f = self.fields
            return (
                f"wdg seq={self.seq} task={f['task']} cause={f['cause']} "
                f"recoveries={f['recoveries']} stale_ms={f['stale_ms']}"
            )
        if self.kind == "log":
            f = self.fields
            return f"log seq={self.seq} sev={f['severity']} code={f['code']} {f['text']}"
        return f"frame seq={self.seq} type=0x{self.type:02X} len={len(self.payload)}"


def encode_frame(frame_type: int, seq: int, payload: bytes, corrupt: bool = False) -> bytes:
    if len(payload) > MAX_PAYLOAD:
        raise ValueError("payload too long")
    body = bytes((VERSION, frame_type)) + struct.pack("<HB", seq, len(payload)) + payload
    crc = crc16_ccitt(body)
    raw = bytearray(SOF + body + struct.pack("<H", crc))
    if corrupt and payload:
        raw[7] ^= 0xFF
    return bytes(raw)


def _parse_payload(frame_type: int, payload: bytes) -> tuple[str, dict]:
    if frame_type == FRAME_ENV and len(payload) >= 14:
        temp, pressure, humidity, time_ms = struct.unpack_from("<iIHI", payload)
        return "env", {
            "temp_c": temp / 100.0,
            "temp_centi_c": temp,
            "pressure_pa": pressure,
            "humidity_pct": humidity / 100.0,
            "humidity_centi_pct": humidity,
            "time_ms": time_ms,
        }
    if frame_type == FRAME_IMU and len(payload) >= 18:
        ax, ay, az, gx, gy, gz, temp, time_ms = struct.unpack_from("<7hI", payload)
        return "imu", {
            "ax_mg": ax,
            "ay_mg": ay,
            "az_mg": az,
            "gx_dps": gx / 10.0,
            "gy_dps": gy / 10.0,
            "gz_dps": gz / 10.0,
            "temp_centi_c": temp,
            "time_ms": time_ms,
        }
    if frame_type == FRAME_CAN and len(payload) >= 11:
        ident, dlc = struct.unpack_from("<HB", payload)
        data = payload[3:11]
        return "can", {"id": ident, "dlc": dlc, "data": data}
    if frame_type == FRAME_WDG and len(payload) >= 8:
        task_id, cause, recoveries, stale_ms = struct.unpack_from("<BBHI", payload)
        return "wdg", {
            "task_id": task_id,
            "task": TASK_NAMES.get(task_id, str(task_id)),
            "cause": cause,
            "recoveries": recoveries,
            "stale_ms": stale_ms,
        }
    if frame_type == FRAME_LOG and len(payload) >= 2:
        text = payload[2:].decode("ascii", errors="replace")
        return "log", {"severity": payload[0], "code": payload[1], "text": text}
    return "unknown", {}


def decode_frame(raw: bytes) -> Frame | None:
    if len(raw) < 9 or raw[:2] != SOF or raw[2] != VERSION:
        return None
    length = raw[6]
    total = 9 + length
    if length > MAX_PAYLOAD or len(raw) < total:
        return None
    body = raw[2:7 + length]
    expect = crc16_ccitt(body)
    got = struct.unpack_from("<H", raw, 7 + length)[0]
    if got != expect:
        return None
    payload = raw[7:7 + length]
    kind, fields = _parse_payload(raw[3], payload)
    return Frame(type=raw[3], seq=struct.unpack_from("<H", raw, 4)[0], payload=payload, kind=kind, fields=fields)


class Decoder:
    """Byte stream decoder. Resyncs on the SOF marker when a frame is rejected."""

    def __init__(self) -> None:
        self.buf = bytearray()
        self.frames: list[Frame] = []
        self.errors: list[str] = []

    def feed(self, data: bytes) -> list[Frame]:
        self.buf.extend(data)
        produced: list[Frame] = []
        while True:
            if len(self.buf) < 2:
                break
            if self.buf[0] != SOF[0] or self.buf[1] != SOF[1]:
                del self.buf[0]
                continue
            if len(self.buf) < 7:
                break
            if self.buf[2] != VERSION:
                self.errors.append("bad version")
                del self.buf[0]
                continue
            length = self.buf[6]
            if length > MAX_PAYLOAD:
                self.errors.append(f"bad length {length}")
                del self.buf[0]
                continue
            total = 9 + length
            if len(self.buf) < total:
                break
            raw = bytes(self.buf[:total])
            frame = decode_frame(raw)
            if frame is None:
                self.errors.append(f"crc mismatch at buf[{len(self.frames)}]")
                del self.buf[:total]
                continue
            del self.buf[:total]
            if frame.kind == "unknown":
                self.errors.append(f"unknown type 0x{frame.type:02X}")
            self.frames.append(frame)
            produced.append(frame)
        return produced
