import os
import subprocess
import threading
import time
from pathlib import Path

import pytest

from telemetry_monitor.monitor import Monitor
from telemetry_monitor.protocol import FRAME_CAN, FRAME_ENV, FRAME_IMU

ROOT = Path(__file__).resolve().parents[2]


def node_binary() -> str:
    override = os.environ.get("TELEMETRY_NODE")
    if override:
        return override
    return str(ROOT / "build" / "telemetry_node")


class Node:
    def __init__(self, link: str):
        binary = node_binary()
        if not Path(binary).is_file():
            pytest.skip(f"firmware binary not built: {binary}")
        self.lines: list[str] = []
        self.proc = subprocess.Popen(
            [binary, "--link", link],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
        )
        assert self.proc.stdout is not None
        self._ready = False
        self._lock = threading.Lock()
        self._thread = threading.Thread(target=self._drain, daemon=True)
        self._thread.start()
        deadline = time.monotonic() + 5
        while time.monotonic() < deadline:
            with self._lock:
                if any(line.startswith("UART_LINK=") for line in self.lines):
                    self._ready = True
                    break
            if self.proc.poll() is not None:
                break
            time.sleep(0.02)
        if not self._ready:
            self.stop()
            pytest.fail("firmware did not open the UART link:\n" + "".join(self.lines))

    def _drain(self) -> None:
        assert self.proc.stdout is not None
        for line in self.proc.stdout:
            with self._lock:
                self.lines.append(line)

    def output(self) -> str:
        with self._lock:
            return "".join(self.lines)

    def stop(self) -> None:
        if self.proc.poll() is None:
            self.proc.terminate()
            try:
                self.proc.wait(timeout=2)
            except subprocess.TimeoutExpired:
                self.proc.kill()
                self.proc.wait(timeout=2)


@pytest.fixture
def link(tmp_path):
    path = tmp_path / "uart"
    return str(path)


def test_frames_and_watchdog_recovery(link):
    node = Node(link)
    mon = Monitor(link, echo=False)
    try:
        assert mon.verify_env_recovery(timeout=3.0)
        env = [frame for frame in mon.decoder.frames if frame.type == FRAME_ENV]
        imu = [frame for frame in mon.decoder.frames if frame.type == FRAME_IMU]
        can = [frame for frame in mon.decoder.frames if frame.type == FRAME_CAN]
        assert env and imu and can
        assert 10.0 < env[0].fields["temp_c"] < 40.0
        assert 90000 < env[0].fields["pressure_pa"] < 110000
        assert 10.0 < env[0].fields["humidity_pct"] < 90.0
        assert 800 < imu[0].fields["az_mg"] < 1200
        ids = {frame.fields["id"] for frame in can}
        assert 0x180 in ids
        assert 0x280 in ids or 0x281 in ids
        assert 0x300 in ids

        before = len(mon.decoder.errors)
        mon.send_line("GLITCH")
        mon.poll(0.6, idle_error_s=5)
        assert len(mon.decoder.errors) > before

        mon.send_line("PING")
        mon.poll(0.6, idle_error_s=5)
        logs = [frame.fields.get("text", "") for frame in mon.decoder.frames if frame.kind == "log"]
        assert any("PONG" in text for text in logs)
        assert node.proc.poll() is None
    finally:
        mon.close()
        node.stop()


def test_tcp_endpoint_delivers_frames():
    port = 20000 + (os.getpid() % 20000)
    node = Node(f"tcp://127.0.0.1:{port}")
    mon = Monitor(f"socket://127.0.0.1:{port}", echo=False)
    try:
        mon.poll(1.0, idle_error_s=5)
        kinds = {frame.kind for frame in mon.decoder.frames}
        assert "env" in kinds
        assert "imu" in kinds
        assert node.proc.poll() is None
    finally:
        mon.close()
        node.stop()


def test_iwdg_resets_when_supervisor_stalls(link):
    node = Node(link)
    mon = Monitor(link, echo=False)
    try:
        mon.poll(0.4, idle_error_s=5)
        mon.send_line("STALL supervisor")
        deadline = time.monotonic() + 5
        while time.monotonic() < deadline:
            if "reason=iwdg" in node.output():
                break
            time.sleep(0.05)
        text = node.output()
        assert "IWDG_RESET" in text
        assert "reason=iwdg" in text
    finally:
        mon.close()
        node.stop()
