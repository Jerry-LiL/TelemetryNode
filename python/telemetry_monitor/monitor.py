"""pySerial monitor: decode frames, log errors, check watchdog recovery."""

from __future__ import annotations

import sys
import time

import serial

from telemetry_monitor.protocol import FRAME_ENV, Decoder, Frame

class Monitor:
    def __init__(self, port: str, baud: int = 115200, log_path: str | None = None, echo: bool = True) -> None:
        self.decoder = Decoder()
        self.echo = echo
        self._log = open(log_path, "a", encoding="utf-8") if log_path else None
        self.ser = serial.serial_for_url(port, baudrate=baud, timeout=0.2)

    def close(self) -> None:
        self.ser.close()
        if self._log is not None:
            self._log.close()
            self._log = None

    def send_line(self, line: str) -> None:
        if not line.endswith("\n"):
            line += "\n"
        self.ser.write(line.encode("ascii"))
        self.ser.flush()

    def _note_error(self, message: str) -> None:
        text = f"ERROR {message}"
        print(text, file=sys.stderr)
        if self._log is not None:
            self._log.write(text + "\n")
            self._log.flush()

    def _emit(self, frame: Frame) -> None:
        line = frame.text()
        if self.echo:
            print(line, flush=True)
        if self._log is not None:
            self._log.write(line + "\n")
            self._log.flush()

    def poll(self, seconds: float, idle_error_s: float = 2.0) -> None:
        """Read for `seconds`. Log an error if the link stays silent for idle_error_s."""
        end = time.monotonic() + seconds
        last_byte = time.monotonic()
        idle_reported = False
        seen_errors = len(self.decoder.errors)
        while time.monotonic() < end:
            data = self.ser.read(256)
            now = time.monotonic()
            if data:
                last_byte = now
                idle_reported = False
                for frame in self.decoder.feed(data):
                    self._emit(frame)
            elif (now - last_byte) >= idle_error_s and not idle_reported:
                self._note_error("serial timeout")
                idle_reported = True
            while seen_errors < len(self.decoder.errors):
                self._note_error(self.decoder.errors[seen_errors])
                seen_errors += 1

    def verify_env_recovery(self, timeout: float = 3.0) -> bool:
        self.poll(0.8, idle_error_s=timeout)
        kinds = {frame.kind for frame in self.decoder.frames}
        if not {"env", "imu", "can"}.issubset(kinds):
            self._note_error(f"missing baseline frames, saw {sorted(kinds)}")
            return False
        mark = len(self.decoder.frames)
        self.send_line("STALL env 800")
        self.poll(timeout, idle_error_s=timeout)
        recovered = [
            frame for frame in self.decoder.frames[mark:]
            if frame.kind == "wdg" and frame.fields.get("task") == "env"
        ]
        if not recovered:
            self._note_error("no watchdog recovery frame for env")
            return False
        wdg_index = self.decoder.frames.index(recovered[0])
        resumed = [frame for frame in self.decoder.frames[wdg_index + 1:] if frame.type == FRAME_ENV]
        if not resumed:
            self._note_error("env samples did not resume after watchdog recovery")
            return False
        return True
