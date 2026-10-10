"""Live view of the power board wiring on the breadboard (bench command PINS of the AR1.2 firmware).

  python pins_live.py        one line whenever something changes, until Ctrl+C

A spare jumper from D1 (row 11 right), D2 or D5 works as a continuity tester: whatever it touches that is
connected to the recorder's minus shows up as "an Minus".
"""

import time

import serial

import ar1


def describe(p):
    lo, avg, hi = p["d0_min_mv"], p["d0_avg_mv"], p["d0_max_mv"]
    if hi < 250:
        d0 = "0 V (kein Signal an LIPO)"
    elif lo == 0 and hi < 700 and avg < 220:
        d0 = "BRUMMEN (Lader hat kein gemeinsames Minus)"
    elif hi - lo < 150:
        d0 = f"ruhig {2 * avg / 1000:.2f} V an LIPO"
    else:
        d0 = f"springt 0 bis {2 * hi / 1000:.1f} V (Lader hat Strom, sucht Akku)"
    probes = [name for name, key in (("D1", "d1"), ("D2", "d2"), ("D5", "d5")) if not p[key]]
    return (f"PGOOD an D3: {'JA' if not p['d3'] or p['pgood_low_pct'] > 50 else 'nein'}   "
            f"CHG an D4: {'JA' if not p['d4'] or p['chg_low_pct'] > 50 else 'nein'}   "
            f"D0: {d0}   Pruefdraht: {', '.join(probes) + ' an Minus' if probes else 'offen'}")


def main():
    port, last, shown = None, None, 0.0
    while True:
        try:
            port = port or ar1.connect()
            answer = ar1.cmd(port, "PINS", 3)
            text = describe(answer) if answer.get("ok") else None
        except (serial.SerialException, OSError) as error:
            busy = "PermissionError" in str(error)      # another window already holds the port
            port, text = None, "USB-Anschluss belegt: Die Anzeige laeuft schon in einem anderen Fenster" if busy else "Recorder nicht am USB"
        if text and (text != last or time.monotonic() - shown > 15):
            print(time.strftime("%H:%M:%S"), " ", text, flush=True)
            last, shown = text, time.monotonic()
        time.sleep(0.3 if port else 1.5)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        pass
