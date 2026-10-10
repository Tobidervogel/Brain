"""Host test, run before every flash:  python test_ar1.py

Builds recording folders exactly as the firmware writes them and makes sure check_ar1 accepts the good one
and catches every kind of damage. Without this a green device test would prove nothing.
Also checks the Bluetooth reference (ar1proto) against the stored vectors and the secret filter of ar1.py.
"""

import hashlib
import json
import shutil
import struct
import tempfile
from pathlib import Path

import ar1
import ar1proto
from check_ar1 import HEADER, check

RATE, SEG_S = 16000, 2
FULL = SEG_S * RATE


def wav(samples, seed):
    data = bytes((seed + i) % 251 for i in range(samples * 2))
    head = struct.pack("<4sI4s4sIHHIIHH4sI", b"RIFF", HEADER - 8 + len(data), b"WAVE", b"fmt ", 16, 1, 1, RATE, RATE * 2, 2, 16,
                       b"JUNK", HEADER - 52)
    return head + bytes(HEADER - 52) + b"data" + struct.pack("<I", len(data)) + data


def make(folder, lengths=(FULL, FULL, 9000), ends=("full", "full", "stop"), dropped=0, final=True):
    """final=False: stopped, but the short last segment has no checksum yet and there is no DONE line."""
    folder.mkdir()
    log = [f"REC id=d0a130-000001-9f3a2c1b n=1 fw=AR1.0 dev=d0a130 boot=3 reset=poweron seg_s={SEG_S} rate={RATE} unix=0 time_src=none up_ms=2100"]
    segments, later, t_ms = [], [], 0
    for n, (samples, end) in enumerate(zip(lengths, ends)):
        raw = wav(samples, n)
        (folder / f"{n:04d}.wav").write_bytes(raw)
        sha = hashlib.sha256(raw).hexdigest()
        short = end != "full"       # the firmware closes short segments without a checksum and adds it later
        log.append(f"OPEN n={n} file={n:04d}.wav t_ms={t_ms} unix=0")
        log.append(f"SEG n={n} file={n:04d}.wav t_ms={t_ms} samples={samples} bytes={len(raw)} dropped=0 missing=0 gap_max_ms=40 "
                   f"ring_max=2048 write_max_ms=40 sha256={'-' if short else sha} end={end} wifi=0 rssi=0 power=unknown")
        if short and final:
            later.append(f"HASH n={n} sha256={sha}")
        segments.append({"n": n, "file": f"{n:04d}.wav", "t_ms": t_ms, "unix": 0, "duration_s": samples / RATE,
                         "samples": samples, "bytes": len(raw), "dropped": 0, "missing": 0, "gap_max_ms": 40,
                         "ring_max_pct": 0.4, "write_max_ms": 40, "sha256": sha if final or not short else None, "end": end,
                         "wifi": False, "rssi": 0, "power": "unknown"})
        t_ms += samples * 1000 // RATE
    total = sum(lengths)
    log.append(f"END reason=stop_usb t_ms={t_ms} samples={total} dropped={dropped} captured={total + dropped} clock_ms={t_ms}")
    log += later + (["DONE unix=0"] if final else [])
    info = {"schema": 1, "fw": "AR1.0", "device": "d0a130", "boot": 3, "reset": "poweron", "rec_id": "d0a130-000001-9f3a2c1b",
            "rec_n": 1, "start_unix": 0, "time_src": "none", "start_up_ms": 2100, "sample_rate": RATE, "bits": 16, "channels": 1,
            "segment_s": SEG_S, "segments": segments, "state": "closed" if final else "stopped", "stop_reason": "stop_usb",
            "samples": total, "dropped": 0, "duration_s": total / RATE}
    (folder / "log.txt").write_text("\n".join(log) + "\n")
    (folder / "info.json").write_text(json.dumps(info))
    return info


def edit_info(folder, change):
    info = json.loads((folder / "info.json").read_text())
    change(info)
    (folder / "info.json").write_text(json.dumps(info))


def edit_log(folder, change):
    (folder / "log.txt").write_text(change((folder / "log.txt").read_text()))


def flip_byte(folder, name="0001.wav"):
    raw = bytearray((folder / name).read_bytes())
    raw[HEADER + 100] ^= 1
    (folder / name).write_bytes(raw)


def truncate(folder):
    raw = (folder / "0002.wav").read_bytes()
    (folder / "0002.wav").write_bytes(raw[:-512])


def late_start(info):
    info["segments"][1]["t_ms"] += 250


def wrong_samples(info):
    info["segments"][2]["samples"] -= 1


def set_state(state):
    def change(info):
        info["state"] = state
    return change


def no_checksum(info):
    info["segments"][2]["sha256"] = None


DAMAGE = {
    "one bit flipped in a full segment": flip_byte,
    "one bit flipped in the short last segment": lambda f: flip_byte(f, "0002.wav"),
    "segment cut short": truncate,
    "segment file missing": lambda f: (f / "0000.wav").unlink(),
    "stray segment file": lambda f: (f / "0007.wav").write_bytes(wav(10, 7)),
    "gap after a full segment": lambda f: edit_info(f, late_start),
    "sample count off by one": lambda f: edit_info(f, wrong_samples),
    "state open although the journal ended": lambda f: edit_info(f, set_state("open")),
    "state stopped although the journal is done": lambda f: edit_info(f, set_state("stopped")),
    "state recovered without a RECOVERED line": lambda f: edit_info(f, set_state("recovered")),
    "finished recording with a missing checksum": lambda f: edit_info(f, no_checksum),
    "DONE line missing": lambda f: edit_log(f, lambda t: t.replace("DONE unix=0\n", "")),
    "checksum line of the short segment differs": lambda f: edit_log(f, lambda t: t.replace("HASH n=2 sha256=", "HASH n=2 sha256=0")[:-1] + "\n"),
    "header says 8 kHz": lambda f: (f / "0000.wav").write_bytes(
        (lambda raw: raw[:24] + struct.pack("<I", 8000) + raw[28:])((f / "0000.wav").read_bytes())),
    "journal without one of the segments": lambda f: edit_log(
        f, lambda t: "\n".join(l for l in t.splitlines() if not l.startswith("SEG n=1")) + "\n"),
}


def recordings(tmp):
    make(tmp / "good")
    problems, lost, _ = check(tmp / "good")
    assert not problems and not lost, problems
    # Right after a stop: the short segment has no checksum yet. That is a valid state, not damage.
    make(tmp / "stopped", final=False)
    problems, lost, _ = check(tmp / "stopped")
    assert not problems and not lost, problems
    for name, damage in DAMAGE.items():
        folder = tmp / name.replace(" ", "_")
        make(folder)
        damage(folder)
        assert check(folder)[0], f"not detected: {name}"
    # Samples dropped by a full ring buffer: files are intact, but the loss must be reported.
    make(tmp / "lossy", dropped=512)
    problems, lost, _ = check(tmp / "lossy")
    assert not problems and lost == 512, (problems, lost)
    # Samples that vanished between the microphone and the files without being counted as dropped.
    make(tmp / "vanished")
    edit_log(tmp / "vanished", lambda t: t.replace(f"captured={2 * FULL + 9000}", f"captured={2 * FULL + 9512}"))
    assert check(tmp / "vanished")[0], "not detected: uncounted loss"
    # A long pause between two microphone blocks is a loss even if every counter says zero.
    make(tmp / "gap")
    edit_info(tmp / "gap", lambda info: info["segments"][0].update(gap_max_ms=900))
    problems, lost, _ = check(tmp / "gap")
    assert not problems and lost, (problems, lost)
    return len(DAMAGE) + 1


def protocol():
    stored = json.loads(ar1proto.VECTORS.read_text())
    assert stored == ar1proto.vectors(), "docs/ar1-proto-vectors.json is out of date: run ar1proto.py"
    ks = bytes.fromhex(stored["ks"])
    frame = bytes.fromhex(stored["command"]["frame"])
    assert ar1proto.unseal(ks, ar1proto.TO_DEVICE, frame, 0) == (1, b"STATUS")
    frames = [bytes.fromhex(f) for f in stored["response"]["frames"]]
    assert max(map(len, frames)) <= stored["response"]["mtu"] - 3 and len(frames) > 1
    assert ar1proto.join_response(ks, frames) == (1, stored["response"]["payload"].encode())
    rejected = 0
    for bad, last in ((frame, 1),                                    # replayed: counter not above the last one
                      (frame[:-1] + bytes([frame[-1] ^ 1]), 0),      # tag damaged
                      (frame[:4] + bytes([frame[4] ^ 1]) + frame[5:], 0),   # ciphertext damaged
                      ((2).to_bytes(4, "big") + frame[4:], 0)):      # counter changed after sealing
        try:
            ar1proto.unseal(ks, ar1proto.TO_DEVICE, bad, last)
        except ValueError:
            rejected += 1
    try:
        ar1proto.unseal(bytes(32), ar1proto.TO_DEVICE, frame, 0)     # wrong key
    except ValueError:
        rejected += 1
    try:
        ar1proto.unseal(ks, ar1proto.TO_PHONE, frame, 0)             # a command reflected back as an answer
    except ValueError:
        rejected += 1
    assert rejected == 6, rejected


def secrets():
    assert ar1.visible("AR {\"ok\":true}") and ar1.visible("STAT t_s=60")
    assert not ar1.visible("AR_SECRET {\"key\":\"00\"}") and not ar1.visible("DV1_WIFI_AUTH tobi abc")


def main():
    tmp = Path(tempfile.mkdtemp())
    try:
        kinds = recordings(tmp)
    finally:
        shutil.rmtree(tmp)
    protocol()
    secrets()
    print(f"OK: intact and freshly stopped recordings accepted, {kinds} kinds of damage detected, losses reported; "
          "protocol vectors and secret filter fine")


if __name__ == "__main__":
    main()
