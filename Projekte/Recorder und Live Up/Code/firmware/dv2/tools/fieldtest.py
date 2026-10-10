"""End-to-end test of the unattended run, as on a power bank:
record -> finalize -> power down -> (timed wake) -> automatic recording -> sudden reset -> automatic recording.

  python fieldtest.py <seconds>
"""

import json
import subprocess
import sys
import time

import serial

import dv2


def wait_port(timeout):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        try:
            return dv2.connect()
        except serial.SerialException:
            time.sleep(0.5)
    sys.exit("board did not come back")


def status(s):
    s.reset_input_buffer()
    s.write(b"STATUS\n")
    for line in dv2.lines(s, 5):
        if line.startswith("DV2_STATUS "):
            return json.loads(line.split(" ", 1)[1])
    sys.exit("no STATUS reply")


seconds = int(sys.argv[1])
s = dv2.connect()
s.write(b"STOP\n")
list(dv2.lines(s, 4))
s.reset_input_buffer()
s.write(f"FIELDTEST {seconds} 15\n".encode())
try:   # nothing is sent from here on, exactly like on a power bank
    for line in dv2.lines(s, seconds + 60, lambda l: l.startswith("Powering down")):
        if line.startswith(("recording", "t=", "finished", "Powering", "ERROR", "DV2_ERROR")):
            print(line, flush=True)
    s.close()
except serial.SerialException:
    pass   # the port disappears with the deep sleep

time.sleep(3)
try:
    dv2.connect().close()
    print("FEHLER: Board ist nach der Aufnahme nicht ausgegangen")
except serial.SerialException:
    print("Aus: USB-Port ist weg, Board schlaeft")

s = wait_port(40)
time.sleep(10)
woke = status(s)
print(f"Nach dem Einschalten: reset={woke['reset']} state={woke['state']} session={woke['session']} "
      f"geplant={woke['planned_s']} s, {woke['frames']} Bilder, {woke['audio_s']} s Ton")

s.close()
# esptool resets the chip into its boot loader and back: the recording is cut off without any clean-up.
subprocess.run([sys.executable, "-m", "esptool", "--chip", "esp32s3", "--port", dv2.PORT, "chip_id"],
               capture_output=True, check=True)
time.sleep(2)
s = wait_port(40)
time.sleep(8)
after = status(s)
print(f"Nach hartem Abbruch: reset={after['reset']} state={after['state']} session={after['session']}, "
      f"{after['frames']} Bilder, {after['audio_s']} s Ton")
s.write(b"STOP\n")
for line in dv2.lines(s, 10, lambda l: l.startswith("finished")):
    if line.startswith("finished"):
        print(line)
print(f"Sitzungen: Feldtest {woke['session'] - 1:04d}, abgebrochen {woke['session']:04d}, danach {after['session']:04d}")
