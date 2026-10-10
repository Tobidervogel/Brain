"""USB helper for the DV2 recorder firmware.

  dv2.py send "STATUS" [seconds]     send one command, print replies for some seconds
  dv2.py mon [seconds]               only listen
  dv2.py get /rec/0001/audio.wav out.wav
  dv2.py wifi "<ssid>" [store]       password comes from the environment variable DV_WIFI_PW
  dv2.py http /status.json [out]     fetch from the recorder's web server (login is read over USB)
"""

import json
import os
import sys
import time
import zlib

import serial

PORT = os.environ.get("DV_PORT", "COM6")


def connect():
    s = serial.Serial()
    s.port, s.baudrate, s.timeout = PORT, 115200, 0.2
    s.dtr = s.rts = False   # both low: opening the port must not reset the board
    s.open()
    return s


def lines(s, seconds, stop=None):
    """Yield text lines until the time is up or stop(line) is true."""
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        line = s.readline().decode(errors="replace").strip()
        if line:
            yield line
            if stop and stop(line):
                return


def show(line):
    if not line.startswith("DV1_WIFI_AUTH "):   # never echo the web login
        print(line, flush=True)


def get_block(s, remote, offset):
    """One CRC-checked block; None if it arrived damaged (the USB serial port drops bytes now and then)."""
    s.reset_input_buffer()
    s.write(f"GET {offset} 16384 {remote}\n".encode())
    for line in lines(s, 5):
        if line.startswith("DV2_ERROR"):
            sys.exit(line)
        if line.startswith("DV2_BLOCK "):
            size, length, crc = map(int, line.split()[1:])
            data = s.read(length)
            return (size, data) if len(data) == length and zlib.crc32(data) == crc else None
    return None


def get(s, remote, local):
    start, retries, size, data = time.monotonic(), 0, None, bytearray()
    s.timeout = 1
    while size is None or len(data) < size:
        block = get_block(s, remote, len(data))
        if block is None:
            retries += 1
            if retries > 200:
                sys.exit(f"giving up at {len(data)} bytes")
            continue
        size = block[0]
        data += block[1]
    with open(local, "wb") as f:
        f.write(data)
    seconds = time.monotonic() - start
    print(f"{remote} -> {local}: {size} bytes in {seconds:.1f} s "
          f"({size / 1024 / max(seconds, 0.001):.0f} KB/s, {retries} blocks repeated)")


def wifi(s, ssid, store=False):
    """store=True saves without the connection test, for a network that is not in range right now."""
    password = os.environ["DV_WIFI_PW"]
    command = "WIFI_STORE" if store else "WIFI_CONFIG"
    time.sleep(0.5)
    s.reset_input_buffer()   # old status lines are still queued on the board when the port opens
    s.write(f"{command} {ssid.encode().hex()} {password.encode().hex()}\n".encode())
    started = False
    for line in lines(s, 45, lambda l: started and l.startswith("DV1_WIFI_STATUS") and '"connecting"' not in l):
        started = started or '"connecting"' in line
        if line.startswith("DV1_WIFI_"):
            show(line)


def http(s, path, out=None):
    import requests
    from requests.auth import HTTPDigestAuth

    s.write(b"WIFI_AUTH\nWIFI_STATUS\n")
    user = password = ip = None
    for line in lines(s, 5):
        if line.startswith("DV1_WIFI_AUTH "):
            _, user, password = line.split()
        elif line.startswith("DV1_WIFI_STATUS "):
            ip = json.loads(line.split(" ", 1)[1])["ip"]
        if user and ip:
            break
    if not (user and ip):
        sys.exit(f"recorder is not on the network (ip={ip!r})")
    start = time.monotonic()
    r = requests.get(f"http://{ip}{path}", auth=HTTPDigestAuth(user, password), timeout=30)
    print(f"http://{ip}{path} -> {r.status_code}, {len(r.content)} bytes in {time.monotonic() - start:.1f} s")
    if out:
        with open(out, "wb") as f:
            f.write(r.content)
    elif r.ok:
        print(r.text[:3000])


def main():
    try:
        run(sys.argv[1], sys.argv[2:])
    except serial.SerialException as error:
        sys.exit(f"port closed ({error}); recorder powered down?")


def run(command, args):
    with connect() as s:
        if command == "send":
            s.write((args[0] + "\n").encode())
            for line in lines(s, float(args[1]) if len(args) > 1 else 3):
                show(line)
        elif command == "mon":
            for line in lines(s, float(args[0]) if args else 10):
                show(line)
        elif command == "get":
            get(s, args[0], args[1])
        elif command == "wifi":
            wifi(s, args[0], store="store" in args[1:])
        elif command == "http":
            http(s, *args)
        else:
            sys.exit(__doc__)


if __name__ == "__main__":
    main()
