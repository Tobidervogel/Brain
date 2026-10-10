"""Device tests for the AR1 firmware, run after every flash (specification, phase 1).

  python devtest.py            t1 t2 t3 t4 t4b t6 t7 (about 6 minutes)
  python devtest.py t2 t4      only these
  python devtest.py t5         30 minutes with real 10-minute segments, then the download (11 minutes over USB)

The recorder must be on USB. Recordings it makes are left on the card and copied to captures/.
"""

import subprocess
import sys
import time

import serial

import ar1
from check_ar1 import check

RATE = 16000
failures = []
port = None


def expect(ok, text):
    print(("    ok   " if ok else "    FAIL ") + text, flush=True)
    if not ok:
        failures.append(text)


def connect(timeout=40):
    global port
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        try:
            port = ar1.connect()
            return
        except serial.SerialException:
            time.sleep(0.5)
    sys.exit("recorder did not come back on " + ar1.PORT)


def cmd(text, seconds=10):
    return ar1.cmd(port, text, seconds)


def status():
    return ar1.ask(port, "STATUS")


def wait_for(what, seconds, test):
    """Polls STATUS until test(status) holds; returns the last status."""
    deadline = time.monotonic() + seconds
    while True:
        s = status()
        if s.get("ok") and test(s):
            return s
        if time.monotonic() > deadline:
            expect(False, f"{what} within {seconds} s (state {s.get('state')}, finalizing {s.get('finalizing')})")
            return s
        time.sleep(0.5)


def idle_and_finished(seconds=60):
    return wait_for("idle and all recordings finished", seconds, lambda s: s["state"] == "idle" and not s["finalizing"])


def stop():
    start = time.monotonic()
    s = cmd("REC STOP")
    return s, time.monotonic() - start


def fetch(number):
    folder = ar1.fetch(port, number)
    problems, lost, report = check(folder)
    print("\n".join("    " + line for line in report), flush=True)
    for problem in problems:
        expect(False, f"recording {number}: {problem}")
    import json
    return problems, lost, json.loads((folder / "info.json").read_text())


def hard_reset():
    """esptool resets the chip into its boot loader and back: the firmware gets no chance to clean up."""
    global port
    port.close()
    subprocess.run([sys.executable, "-m", "esptool", "--chip", "esp32s3", "--port", ar1.PORT, "chip_id"],
                   capture_output=True, check=True)
    time.sleep(2)
    connect()
    time.sleep(4)


def device_log():
    ar1.get(port, "/device.log", ar1.CAPTURES / "device.log")
    return (ar1.CAPTURES / "device.log").read_text(errors="replace").splitlines()


def fresh(seglen=20):
    stop()
    idle_and_finished()
    expect(cmd(f"SEGLEN {seglen}").get("ok"), f"SEGLEN {seglen}")


def t1():
    selftest = cmd("SELFTEST")
    expect(selftest.get("ok") is True, f"SELFTEST {selftest}")
    df = cmd("DF")
    expect(df.get("ok") and df["total_mb"] > 1000 and df["free_mb"] > 1000, f"DF {df}")
    s = status()
    expect(s["fw"].startswith("AR1") and s["sd"]["ok"], f"firmware {s['fw']}, card ok")


def t2():
    fresh()
    s = cmd("REC START")
    expect(s["state"] == "recording", "REC START answers with state recording")
    number = s["rec"]["n"]
    time.sleep(70)
    running = status()
    expect(running["rec"]["dropped"] == 0 and running["rec"]["ring_max_pct"] < 10,
           f"while recording: dropped {running['rec']['dropped']}, ring max {running['rec']['ring_max_pct']} %")
    s, took = stop()
    expect(s["state"] == "idle" and took < 2.5, f"REC STOP done after {took:.1f} s (state {s['state']})")
    idle_and_finished()
    problems, lost, info = fetch(number)
    samples = [seg["samples"] for seg in info["segments"]]
    expect(samples[:3] == [20 * RATE] * 3 and len(samples) == 4 and 9 * RATE < samples[3] < 12 * RATE, f"segments {samples}")
    expect([seg["end"] for seg in info["segments"]] == ["full", "full", "full", "stop"], "ends full, full, full, stop")
    expect(info["state"] == "closed" and all(seg["sha256"] for seg in info["segments"]) and not lost,
           f"state {info['state']}, every checksum present, nothing lost")


def t3():
    fresh()
    number = cmd("REC START")["rec"]["n"]
    time.sleep(8)
    start = time.monotonic()
    s = cmd("REC PAUSE")
    expect(s.get("state") == "paused" and time.monotonic() - start < 2.5, f"REC PAUSE -> {s.get('state')} in {time.monotonic() - start:.1f} s")
    time.sleep(5)
    s = cmd("REC RESUME")
    expect(s.get("state") == "recording" and s["rec"]["n"] == number, f"REC RESUME -> {s.get('state')}, same recording")
    time.sleep(8)
    stop()
    idle_and_finished()
    problems, lost, info = fetch(number)
    segs = info["segments"]
    expect(len(segs) == 2 and segs[0]["end"] == "pause" and segs[1]["end"] == "stop", f"ends {[s['end'] for s in segs]}")
    pause_ms = segs[1]["t_ms"] - segs[0]["samples"] * 1000 / RATE if len(segs) == 2 else 0
    expect(4000 < pause_ms < 8000, f"second segment starts {pause_ms:.0f} ms after the first ended")
    expect(info["state"] == "closed" and not lost, f"state {info['state']}, nothing lost")


def t4():
    fresh()
    boots = sum(line.startswith("BOOT") for line in device_log())
    number = cmd("REC START")["rec"]["n"]
    time.sleep(30)
    hard_reset()
    s = wait_for("recording again after the reset", 15, lambda s: s["state"] == "recording")
    expect(s["rec"]["n"] != number and s["reset"] != "poweron", f"new recording {s['rec']['n']} after reset={s['reset']}")
    stop()
    idle_and_finished()
    problems, lost, info = fetch(number)
    segs = info["segments"]
    expect(info["state"] == "recovered" and info["stop_reason"] == "interrupted", f"state {info['state']}, stop {info['stop_reason']}")
    expect(len(segs) == 2 and segs[0]["end"] == "full" and segs[0]["samples"] == 20 * RATE and segs[0]["sha256"],
           "the segment finished before the reset is intact")
    expect(len(segs) == 2 and segs[1]["end"] == "interrupted" and 6 * RATE < segs[1]["samples"] <= 11 * RATE,
           f"the open segment was completed with {segs[1]['samples'] / RATE if len(segs) == 2 else 0:.1f} s (at most 2 s lost)")
    expect(sum(line.startswith("BOOT") for line in device_log()) == boots + 1, "/device.log has a new BOOT line")
    problems, lost, info = fetch(s["rec"]["n"])
    expect(info["state"] == "closed", "the recording after the reset is closed")


def t4b():
    fresh()
    cmd("REC START")
    time.sleep(5)
    expect(cmd("REC PAUSE").get("state") == "paused", "paused")
    hard_reset()
    time.sleep(6)
    s = status()
    expect(s["state"] == "idle" and not s["want"], f"after a reset a paused recorder stays off (state {s['state']}, want {s['want']})")
    idle_and_finished()


def t6():
    fresh()
    number = cmd("REC START")["rec"]["n"]
    s = cmd("TIMER stop 8")
    expect(s["timer"] and s["timer"]["action"] == "stop" and 6 <= s["timer"]["in_s"] <= 8, f"timer set: {s['timer']}")
    time.sleep(11)
    s = idle_and_finished()
    expect(s["rec"]["stop_reason"] == "timer" and s["timer"] is None, f"stopped by the timer ({s['rec']['stop_reason']})")
    problems, lost, info = fetch(number)
    expect(info["stop_reason"] == "timer" and info["state"] == "closed", "journal says END reason=timer")

    global port
    answer = cmd("SLEEP 12")
    expect(answer.get("state") == "going_to_sleep", f"SLEEP 12 -> {answer}")
    port.close()
    time.sleep(5)
    try:
        ar1.connect().close()
        expect(False, "USB port is gone while the recorder sleeps")
    except serial.SerialException:
        expect(True, "USB port is gone while the recorder sleeps")
    started = time.monotonic()
    connect(40)
    time.sleep(3)
    s = status()
    expect(s["reset"] == "deepsleep" and s["state"] == "idle" and not s["want"],
           f"awake again after {time.monotonic() - started + 5:.0f} s: reset={s['reset']}, state {s['state']}")


def t7():
    fresh()
    first = cmd("REC START")["rec"]["n"]
    time.sleep(6)
    expect(cmd("FAULT sd").get("ok"), "FAULT sd accepted")
    s = wait_for("a new recording after the card error", 20, lambda s: s["state"] == "recording" and s["rec"]["n"] != first)
    expect(s["sd"]["errors"] == 1, f"card error counted ({s['sd']['errors']})")
    second = s["rec"]["n"]
    time.sleep(4)
    stop()
    idle_and_finished()
    problems, lost, info = fetch(first)
    expect(info["stop_reason"] == "sd_error" and info["state"] == "recovered", f"failed recording: stop {info['stop_reason']}, state {info['state']}")
    problems, lost, info = fetch(second)
    expect(info["state"] == "closed", "the recording after the card error is closed")

    before = cmd("LS")
    total = cmd("DF")["total_mb"]
    expect(cmd(f"MINFREE {total + 1}").get("ok"), "MINFREE above the size of the card")
    s = cmd("REC START", 15)
    time.sleep(1)
    s = status()
    expect(s["state"] == "idle" and s["rec"]["stop_reason"] == "card_full" and not s["want"],
           f"no recording on a full card (state {s['state']}, reason {s['rec']['stop_reason']})")
    expect(cmd("LS") == before, "no new folder was created")
    expect(cmd("MINFREE 64").get("ok"), "MINFREE back to 64")


def t5():
    """30 minutes with real segments. Joins a 10-minute-segment recording that is already running."""
    s = status()
    if s["state"] == "recording" and s["rec"]["seg_s"] == 600:
        number = s["rec"]["n"]
        print(f"    joining recording {number} at {s['rec']['elapsed_s']:.0f} s", flush=True)
    else:
        fresh(600)
        number = cmd("REC START")["rec"]["n"]
    worst_ring, polls, unanswered = 0.0, 0, 0
    while True:
        s, listing = status(), cmd("LS")
        polls += 1
        if "rec" not in s or "recs" not in listing:      # an answer that did not arrive is a finding, not a crash
            unanswered += 1
            print(f"    poll {polls}: STATUS -> {str(s)[:120]}  LS -> {str(listing)[:120]}", flush=True)
            time.sleep(2)
            continue
        worst_ring = max(worst_ring, s["rec"]["ring_max_pct"])
        if polls % 30 == 0:
            print(f"    {s['rec']['elapsed_s']:.0f} s  seg {s['rec']['seg']}  dropped {s['rec']['dropped']}  missing {s['rec']['missing']}  "
                  f"ring max {s['rec']['ring_max_pct']} %  wifi {s['wifi']['connected']} {s['wifi']['rssi']}  heap {s['heap']}", flush=True)
        if s["state"] != "recording" or s["rec"]["elapsed_s"] >= 1815:
            break
        time.sleep(10)
    expect(s["state"] == "recording" and s["rec"]["n"] == number, f"still the same recording after 30 minutes (state {s['state']})")
    stop()
    idle_and_finished(120)
    problems, lost, info = fetch(number)
    full = [seg for seg in info["segments"] if seg["end"] == "full"]
    expect(len(full) == 3 and all(seg["samples"] == 600 * RATE for seg in full), f"{len(full)} full 10-minute segments")
    expect(not lost and worst_ring < 10, f"nothing lost, ring buffer at most {worst_ring} % with STATUS and LS every 10 s")
    expect(unanswered == 0, f"{unanswered} of {polls} polls without a proper answer")


TESTS = {"t1": t1, "t2": t2, "t3": t3, "t4": t4, "t4b": t4b, "t6": t6, "t7": t7, "t5": t5}


def main():
    names = sys.argv[1:] or ["t1", "t2", "t3", "t4", "t4b", "t6", "t7"]
    connect()
    for name in names:
        print(f"--- {name.upper()}", flush=True)
        try:
            TESTS[name]()
        except (KeyError, TypeError, serial.SerialException) as error:
            expect(False, f"{name} aborted: {error!r}")
            connect()
    cmd("SEGLEN 600")
    print(f"\nERGEBNIS: {'OK' if not failures else str(len(failures)) + ' FEHLER'}")
    for text in failures:
        print("  - " + text)
    sys.exit(1 if failures else 0)


if __name__ == "__main__":
    main()
