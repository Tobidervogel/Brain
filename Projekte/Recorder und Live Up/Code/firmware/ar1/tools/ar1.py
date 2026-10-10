"""USB helper for the AR1 recorder firmware.

  ar1.py STATUS                        send one command (several words allowed), print the JSON answer
  ar1.py mon [seconds]                 only listen
  ar1.py dir /rec                      raw directory listing
  ar1.py get /rec/000001/0000.wav out.wav
  ar1.py fetch <recording number>      download a whole recording into captures/ and check it
  ar1.py fetch <number> 0,58,116       only these segments (a long recording takes hours over USB)
"""

import json
import os
import subprocess
import sys
import time
import zlib
from pathlib import Path

import serial

PORT = os.environ.get("AR_PORT", "COM6")
CAPTURES = Path(__file__).resolve().parent.parent / "captures"


def connect():
    s = serial.Serial()
    s.port, s.baudrate, s.timeout = PORT, 115200, 0.2
    s.dtr = s.rts = False   # both low: opening the port must not reset the board
    s.open()
    return s


def lines(s, seconds, stop=None):
    """Yield complete text lines until the time is up or stop(line) is true."""
    deadline = time.monotonic() + seconds
    pending = b""
    while time.monotonic() < deadline:
        pending += s.readline()      # returns early when its timeout hits in the middle of a line
        if not pending.endswith(b"\n"):
            continue
        line, pending = pending.decode(errors="replace").strip(), b""
        if line:
            yield line
            if stop and stop(line):
                return


def visible(line):
    """False for lines that carry credentials (web login, device key): they never reach the console or a log."""
    return not line.startswith(("AR_SECRET", "DV1_WIFI_AUTH"))


def cmd(s, text, seconds=10):
    """Send one command and return its JSON answer as a dict."""
    s.reset_input_buffer()
    s.write((text + "\n").encode())
    for line in lines(s, seconds):
        if line.startswith("AR {"):
            try:
                return json.loads(line[3:])
            except ValueError:      # the USB serial port loses bytes now and then
                return {"ok": False, "error": "damaged answer"}
        # DV1_WIFI_ERROR also appears on its own when a Wi-Fi join times out: only an answer to the old WIFI_ commands.
        if line.startswith("AR_ERROR") or (line.startswith("DV1_WIFI_ERROR") and text.startswith("WIFI_")):
            return {"ok": False, "error": line}
    return {"ok": False, "error": "no answer"}


def ask(s, text, seconds=10, tries=3):
    """Like cmd, for commands that may be repeated (STATUS, LS, DF ...): asks again after a lost or damaged answer."""
    for _ in range(tries):
        answer = cmd(s, text, seconds)
        if answer.get("error") not in ("damaged answer", "no answer"):
            break
    return answer


def listing(s, path):
    """[(name, size, is_dir)] of a folder on the card."""
    s.reset_input_buffer()
    s.write(f"DIR {path}\n".encode())
    found = []
    for line in lines(s, 20, lambda l: l.startswith(("AR_DIR_DONE", "AR_ERROR"))):
        if line.startswith("AR_DIR ") and not line.startswith("AR_DIR_DONE"):
            parts = line.split()
            found.append((parts[1], int(parts[2]), len(parts) > 3))
    return found


def get_block(s, remote, offset):
    """One CRC-checked block; None if it arrived damaged (the USB serial port drops bytes now and then)."""
    s.reset_input_buffer()
    s.write(f"GET {offset} 16384 {remote}\n".encode())
    for line in lines(s, 5):
        if line.startswith("AR_ERROR"):
            sys.exit(f"{remote}: {line}")
        if line.startswith("AR_BLOCK "):
            size, length, crc = map(int, line.split()[1:])
            data = s.read(length)
            return (size, data) if len(data) == length and zlib.crc32(data) == crc else None
    return None


def get(s, remote, local):
    start, retries, size, data = time.monotonic(), 0, None, bytearray()
    old_timeout, s.timeout = s.timeout, 1
    while size is None or len(data) < size:
        block = get_block(s, remote, len(data))
        if block is None:
            retries += 1
            if retries > 200:
                sys.exit(f"giving up at {len(data)} bytes")
            continue
        size = block[0]
        data += block[1]
    s.timeout = old_timeout
    Path(local).write_bytes(data)
    seconds = time.monotonic() - start
    print(f"{remote} -> {local}: {size} bytes in {seconds:.1f} s "
          f"({size / 1024 / max(seconds, 0.001):.0f} KB/s, {retries} blocks repeated)", flush=True)


def fetch(s, number, only=None):
    """Download the files of one recording; returns the local folder. only = segment numbers: the rest stays on the card."""
    name = f"{int(number):06d}"
    folder = CAPTURES / name
    folder.mkdir(parents=True, exist_ok=True)
    for old in folder.iterdir():
        old.unlink()
    for file, _, is_dir in sorted(listing(s, f"/rec/{name}")):
        wanted = only is None or not file.endswith(".wav") or int(file[:4]) in only
        if not is_dir and wanted:
            get(s, f"/rec/{name}/{file}", folder / file)
    return folder


def main():
    args = sys.argv[1:]
    if not args:
        sys.exit(__doc__)
    try:
        with connect() as s:
            if args[0] == "mon":
                for line in lines(s, float(args[1]) if len(args) > 1 else 10):
                    if visible(line):
                        print(line, flush=True)
            elif args[0] == "dir":
                for name, size, is_dir in listing(s, args[1]):
                    print(f"{size:>10}  {name}{'/' if is_dir else ''}")
            elif args[0] == "get":
                get(s, args[1], args[2])
            elif args[0] == "fetch":
                only = {int(n) for n in args[2].split(",")} if len(args) > 2 else None
                folder = fetch(s, args[1], only)
                s.close()
                checker = [sys.executable, str(Path(__file__).with_name("check_ar1.py")), str(folder)]
                sys.exit(subprocess.call(checker + (["--sample"] if only is not None else [])))
            else:
                print(json.dumps(cmd(s, " ".join(args)), indent=1))
    except serial.SerialException as error:
        sys.exit(f"port closed ({error}); recorder asleep or unplugged?")


if __name__ == "__main__":
    main()
