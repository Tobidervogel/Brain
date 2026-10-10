"""Pairs the phone with the recorder: reads the Bluetooth address and the device key over USB and hands
them to Live Up through adb. Nothing secret is printed, logged or written to a file on the PC.

  python pair.py            phone on a USB cable (adb -d)
  python pair.py --wlan     phone over Wi-Fi adb; on Android 9 that link is not encrypted, so the key
                            crosses the home network in the clear. Use the cable if anyone else is on it.
  python pair.py --new      make a new device key first (every earlier pairing stops working)

The recorder must be on USB, Live Up (debug build) must have been started once on the phone.
"""

import json
import shutil
import subprocess
import sys
import time

import ar1

PACKAGE = "de.tobidervogel.liveup"
ADB = shutil.which("adb") or "A:/Android/Sdk/platform-tools/adb.exe"


def pairing(port):
    """The pairing data as the recorder sends it (an AR_SECRET line)."""
    port.reset_input_buffer()
    port.write(b"BLE_KEY\n")
    for line in ar1.lines(port, 5):
        if line.startswith("AR_SECRET "):
            return line[len("AR_SECRET "):]
    sys.exit("the recorder did not answer BLE_KEY (firmware older than AR1.1?)")


def main():
    wlan, new = "--wlan" in sys.argv[1:], "--new" in sys.argv[1:]
    with ar1.connect() as port:
        if new and not ar1.cmd(port, "BLE_KEY NEW").get("ok"):
            sys.exit("BLE_KEY NEW failed")
        time.sleep(0.2)
        payload = pairing(port)
    data = json.loads(payload)
    if len(data.get("key", "")) != 64 or len(data.get("addr", "")) != 17:
        sys.exit("unexpected pairing data")
    # The data travels on standard input: no command line, no temporary file, no shared storage.
    target = ["-e"] if wlan else ["-d"]
    result = subprocess.run([ADB, *target, "shell", f"run-as {PACKAGE} sh -c 'mkdir -p files && cat > files/recorder_pair.json'"],
                            input=payload.encode(), capture_output=True)
    if result.returncode or result.stderr.strip():
        sys.exit(f"adb failed: {result.stderr.decode(errors='replace').strip() or result.returncode}")
    print(f"Kopplungsdaten fuer {data['name']} ({data['addr']}) liegen in Live Up. Modul Recorder oeffnen, dann uebernimmt die App sie.")
    if wlan:
        print("Hinweis: Uebertragen ueber WLAN-ADB. Wer das Heimnetz mitlesen konnte, kennt den Schluessel; 'pair.py --new' per Kabel ersetzt ihn.")


if __name__ == "__main__":
    main()
