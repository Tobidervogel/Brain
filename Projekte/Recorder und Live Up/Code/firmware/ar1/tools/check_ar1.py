"""Checks one AR1 recording folder (NNNN.wav, info.json, log.txt) as it came off the card.

  python check_ar1.py <folder> [--sample]      --sample: only some segment files were downloaded

Exit code 0 = intact and nothing lost, 1 = damaged or inconsistent, 2 = intact but samples were lost.
"""

import hashlib
import json
import struct
import sys
from pathlib import Path

HEADER = 512
MISSING_MAX, GAP_MAX_MS = 1024, 500      # two microphone blocks against the clock; longest pause between blocks


def journal(text):
    """[(keyword, {name: value})] of the complete lines of a log.txt."""
    out = []
    for line in text.split("\n")[:-1]:      # what follows the last line end was cut off
        words = line.split()
        if words:
            out.append((words[0], dict(w.split("=", 1) for w in words[1:] if "=" in w)))
    return out


def wav_problems(raw, rate):
    """Structural errors of a finished segment file; [] if it is a clean WAV of this project."""
    if len(raw) < HEADER:
        return [f"only {len(raw)} bytes, shorter than the header"]
    riff, riff_size, wave, fmt, fmt_size = struct.unpack_from("<4sI4s4sI", raw, 0)
    kind, channels, file_rate, _, _, bits = struct.unpack_from("<HHIIHH", raw, 20)
    data, data_size = struct.unpack_from("<4sI", raw, HEADER - 8)
    found = []
    if (riff, wave, fmt, data) != (b"RIFF", b"WAVE", b"fmt ", b"data"):
        found.append("not a RIFF/WAVE file with the data chunk at byte 504")
    if (fmt_size, kind, channels, file_rate, bits) != (16, 1, 1, rate, 16):
        found.append(f"format is {(kind, channels, file_rate, bits)}, expected PCM mono {rate} Hz 16 bit")
    if data_size != len(raw) - HEADER:
        found.append(f"data chunk says {data_size} bytes, file has {len(raw) - HEADER}")
    if riff_size != len(raw) - 8:
        found.append(f"RIFF size {riff_size} != file size - 8 ({len(raw) - 8})")
    return found


def check(folder, sample=False):
    """Returns (problems, lost_samples, report lines). sample=True: segment files that were not downloaded are skipped."""
    folder = Path(folder)
    problems, report = [], []

    def expect(ok, text):
        if not ok:
            problems.append(text)

    try:
        info = json.loads((folder / "info.json").read_text(encoding="utf-8"))
    except (OSError, ValueError) as error:
        return [f"info.json: {error}"], 0, report
    log = journal((folder / "log.txt").read_text(encoding="utf-8", errors="replace")) if (folder / "log.txt").exists() else []
    expect(log, "log.txt is missing or empty")
    rate, segment_samples = info["sample_rate"], info["segment_s"] * info["sample_rate"]
    expect(info.get("schema") == 1, "info.json: schema is not 1")

    rec = next((f for k, f in log if k == "REC"), {})
    ends = [f for k, f in log if k == "END"]
    done = bool(log) and log[-1][0] == "DONE"
    closed = {int(f["n"]): (k, f) for k, f in log if k in ("SEG", "RECOVERED")}
    hashes = {int(f["n"]): f["sha256"] for k, f in log if k == "HASH"}
    state = info["state"]
    expect(rec.get("id") == info["rec_id"], f"recording id differs: journal {rec.get('id')}, info.json {info['rec_id']}")
    expect(len(ends) <= 1, "more than one END line")
    expect(state in ("open", "stopped", "closed", "recovered"), f"unknown state {state}")
    expect((state == "open") == (not ends), f"state is {state} but the journal has {len(ends)} END lines")
    expect((state in ("closed", "recovered")) == done, f"state is {state} but the journal {'ends' if done else 'does not end'} with DONE")
    expect((state == "recovered") == (done and any(k == "RECOVERED" for k, _ in closed.values())),
           f"state is {state}: does not match the RECOVERED lines")

    total = lost = unhashed = skipped = verified = 0
    previous = None
    for seg in info["segments"]:
        n, name = seg["n"], seg["file"]
        expect(name == f"{n:04d}.wav", f"segment {n}: file name {name}")
        if seg["end"] == "open":
            expect(state == "open", f"segment {n} is open in a recording that is {state}")
            continue
        path = folder / name
        if not path.exists():
            if sample:              # everything that needs the audio is skipped, the bookkeeping is still checked
                skipped += 1
                expect(seg["sha256"] or not done, f"{name}: no checksum in a finished recording")
                expect(seg["bytes"] == HEADER + 2 * seg["samples"], f"{name}: bytes and samples disagree in info.json")
                previous = seg
                total += seg["samples"]
                lost += max(seg["dropped"], 0)
                if abs(seg.get("missing", 0)) > MISSING_MAX or seg.get("gap_max_ms", 0) >= GAP_MAX_MS:
                    lost = max(lost, abs(seg.get("missing", 0)), 1)
                continue
            problems.append(f"{name}: file is missing")
            continue
        verified += 1
        raw = path.read_bytes()
        for text in wav_problems(raw, rate):
            problems.append(f"{name}: {text}")
        expect(len(raw) == seg["bytes"] == HEADER + 2 * seg["samples"],
               f"{name}: {len(raw)} bytes on disk, info.json says {seg['bytes']} bytes and {seg['samples']} samples")
        kind, line = closed.get(n, (None, {}))
        logged = line.get("sha256", "-")
        logged = hashes.get(n, logged) if logged == "-" else logged
        if seg["sha256"] is None:
            unhashed += 1
            expect(not done and logged == "-", f"{name}: no checksum in info.json although the recording is finished")
        else:
            expect(seg["sha256"] == hashlib.sha256(raw).hexdigest(), f"{name}: SHA-256 differs from info.json")
            expect(logged == seg["sha256"], f"{name}: checksum in the journal differs from info.json")
        expect(kind is not None and int(line["samples"]) == seg["samples"], f"{name}: journal and info.json disagree")
        expect(seg["samples"] <= segment_samples, f"{name}: longer than a segment")
        expect((seg["end"] == "full") == (seg["samples"] == segment_samples),
               f"{name}: end={seg['end']} with {seg['samples']} of {segment_samples} samples")
        expect(abs(seg["duration_s"] - seg["samples"] / rate) < 0.002, f"{name}: duration_s does not match the samples")
        if previous:
            gap = seg["t_ms"] - previous["t_ms"] - previous["samples"] * 1000 / rate
            if previous["end"] == "full" and seg["end"] != "interrupted" and previous["dropped"] >= 0:
                # A full segment is followed without a gap; dropped samples shift the next start by their length.
                expect(abs(gap - previous["dropped"] * 1000 / rate) <= 1, f"{name}: starts {gap:.0f} ms after the previous segment ended")
            else:
                expect(gap >= -1, f"{name}: starts {-gap:.0f} ms before the previous segment ended")
        previous = seg
        total += seg["samples"]
        lost += max(seg["dropped"], 0)
        missing, gap_max = seg.get("missing", 0), seg.get("gap_max_ms", 0)
        if abs(missing) > MISSING_MAX or gap_max >= GAP_MAX_MS:
            lost = max(lost, abs(missing), 1)
        report.append(f"  {name}  {seg['samples'] / rate:8.2f} s  t={seg['t_ms'] / 1000:9.2f} s  end={seg['end']:<11} dropped={seg['dropped']} "
                      f"missing={missing} gap {gap_max} ms  ring {seg.get('ring_max_pct', 0)} %  write {seg.get('write_max_ms', 0)} ms"
                      f"{'' if seg['sha256'] else '  (noch ohne Pruefsumme)'}")

    wavs = {p.name for p in folder.glob("[0-9][0-9][0-9][0-9].wav")}
    listed = {seg["file"] for seg in info["segments"]}
    expect(wavs <= listed, f"files not in info.json: {sorted(wavs - listed)}")
    expect(set(closed) == {s["n"] for s in info["segments"] if s["end"] != "open"}, "journal and info.json list different segments")
    expect(total == info["samples"], f"segments add up to {total} samples, info.json says {info['samples']}")
    expect(info["dropped"] == sum(max(s.get("dropped", 0), 0) for s in info["segments"]), "info.json: dropped does not add up")

    if ends and "captured" in ends[0]:      # written by the firmware on a proper stop
        end = ends[0]
        captured, dropped, clock_ms = int(end["captured"]), int(end["dropped"]), int(end["clock_ms"])
        expect(captured - dropped == int(end["samples"]) == total,
               f"microphone delivered {captured}, {dropped} dropped, but {total} samples are in the files")
        drift = captured * 1000 / rate - clock_ms
        # One microphone block (32 ms) per capture run is the resolution of this comparison.
        runs = 1 + sum(1 for k, _ in log if k == "RESUME")
        expect(abs(drift) <= 64 * runs + clock_ms * 0.0005, f"samples and clock disagree by {drift:.0f} ms")
        lost = max(lost, dropped)
        report.append(f"  microphone {captured} samples, clock {clock_ms} ms, difference {drift:+.0f} ms")

    head = (f"{info['rec_id']}  fw {info['fw']}  boot {info['boot']} ({info['reset']})  state {state}"
            f"  stop {info['stop_reason'] or '-'}  {total / rate:.2f} s in {len(info['segments'])} segments"
            f"  start_unix {info['start_unix']} ({info['time_src']})" + (f"  {unhashed} ohne Pruefsumme" if unhashed else "")
            + (f"  Stichprobe: {verified} Segmente am PC geprueft, {skipped} nur nach den Angaben des Geraets" if sample else ""))
    return problems, lost, [head] + report


def main():
    if len(sys.argv) not in (2, 3):
        sys.exit(__doc__)
    problems, lost, report = check(sys.argv[1], sample="--sample" in sys.argv[2:])
    print("\n".join(report))
    if problems:
        print("ERGEBNIS: PROBLEME\n  - " + "\n  - ".join(problems))
        sys.exit(1)
    print("ERGEBNIS: OK" if not lost else f"ERGEBNIS: unbeschaedigt, aber Verluste ({lost} Samples oder Luecke)")
    sys.exit(2 if lost else 0)


if __name__ == "__main__":
    main()
