import argparse
import sys
import time

import serial

from telemetry_monitor.monitor import Monitor


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Decode TelemetryNode UART frames")
    parser.add_argument("--port", required=True, help="pty path, symlink, or socket://host:port")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--seconds", type=float, default=0.0, help="read this long, then exit")
    parser.add_argument("--log", default="", help="append decoded lines and errors to this file")
    parser.add_argument("--check", action="store_true", help="stall the env task and require watchdog recovery")
    args = parser.parse_args(argv)

    mon = None
    for _ in range(25):
        try:
            mon = Monitor(args.port, args.baud, log_path=args.log or None, echo=True)
            break
        except serial.SerialException:
            time.sleep(0.2)
    if mon is None:
        print(f"ERROR could not open {args.port}", file=sys.stderr)
        return 1
    try:
        if args.check:
            ok = mon.verify_env_recovery()
            return 0 if ok else 1
        if args.seconds > 0:
            mon.poll(args.seconds)
            return 0
        while True:
            mon.poll(1.0)
    except KeyboardInterrupt:
        return 0
    finally:
        mon.close()


if __name__ == "__main__":
    sys.exit(main())
