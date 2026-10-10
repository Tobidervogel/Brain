"""Checks one recording folder (video.avi, audio.wav, info.json, frames.csv) and saves three sample frames.

  python check.py <folder>
"""

import csv
import json
import struct
import sys
import wave
from pathlib import Path

import cv2
import numpy as np

folder = Path(sys.argv[1])
if not (folder / "info.json").exists():
    # Cut off by a power loss: no index, provisional headers, no info.json. It must still play.
    video, frames = cv2.VideoCapture(str(folder / "video.avi")), 0
    while video.read()[0]:
        frames += 1
    with wave.open(str(folder / "audio.wav")) as w:
        samples = len(w.readframes(w.getnframes())) // 2
    print(f"Abgebrochene Aufnahme: {frames} Bilder abspielbar, {samples / 16000:.1f} s Ton lesbar")
    print("ERGEBNIS:", "OK" if frames and samples else "PROBLEME")
    sys.exit(0 if frames and samples else 1)
info = json.loads((folder / "info.json").read_text())
problems = []


def expect(ok, text):
    if not ok:
        problems.append(text)


# ---- AVI: walk the container byte by byte ----
raw = (folder / "video.avi").read_bytes()
u32 = lambda at: struct.unpack_from("<I", raw, at)[0]
expect(raw[:4] == b"RIFF" and raw[8:12] == b"AVI ", "AVI: no RIFF/AVI header")
expect(u32(4) == len(raw) - 8, f"AVI: RIFF size {u32(4)} != file size - 8 ({len(raw) - 8})")
expect(raw[1012:1016] == b"LIST" and raw[1020:1024] == b"movi", "AVI: movi list not at byte 1012")
movi_end = 1016 + 4 + u32(1016)
at, chunks = 1024, []
while at < movi_end and raw[at:at + 4] == b"00dc":
    size = u32(at + 4)
    jpeg = raw[at + 8:at + 8 + size]
    expect(jpeg[:2] == b"\xff\xd8", f"AVI: frame {len(chunks)} does not start like a JPEG")
    chunks.append((at - 1020, size))
    at += 8 + size + (size & 1)
expect(at == movi_end, f"AVI: frame chunks end at {at}, movi list says {movi_end}")
expect(len(chunks) == info["frames"], f"AVI: {len(chunks)} frames in file, info.json says {info['frames']}")
expect(raw[at:at + 4] == b"idx1" and u32(at + 4) == 16 * len(chunks), "AVI: index missing or wrong size")
index = [struct.unpack_from("<4sIII", raw, at + 8 + 16 * i) for i in range(len(chunks))]
expect(all(e[0] == b"00dc" and (e[2], e[3]) == c for e, c in zip(index, chunks)), "AVI: index does not match frames")
expect(at + 8 + 16 * len(chunks) == len(raw), "AVI: bytes after the index")
header_fps = 1e6 / u32(32)

# ---- AVI: let a real decoder play it ----
video = cv2.VideoCapture(str(folder / "video.avi"))
decoded, sample_at = 0, {0, len(chunks) // 2, len(chunks) - 1}
while True:
    ok, frame = video.read()
    if not ok:
        break
    if decoded in sample_at:
        cv2.imwrite(str(folder / f"frame_{decoded:05d}.jpg"), frame)
        shape = frame.shape
    decoded += 1
expect(decoded == len(chunks), f"AVI: decoder got {decoded} of {len(chunks)} frames")
expect(decoded and shape[:2] == (info["height"], info["width"]), "AVI: wrong picture size")

# ---- frame timing ----
times = np.array([int(row["t_ms"]) for row in csv.DictReader((folder / "frames.csv").open())])
gaps = np.diff(times) if len(times) > 1 else np.array([0])
nominal = 1000 / info["fps_target"] if info["fps_target"] else float(np.median(gaps))
video_s = (times[-1] - times[0]) / 1000 if len(times) > 1 else 0
expect(abs(header_fps - info["fps_avg"]) < 0.01, f"AVI: header says {header_fps:.3f} fps, measured {info['fps_avg']}")

# ---- WAV ----
with wave.open(str(folder / "audio.wav")) as w:
    params = (w.getnchannels(), w.getsampwidth(), w.getframerate())
    audio = np.frombuffer(w.readframes(w.getnframes()), dtype=np.int16).astype(np.int32)
expect(params == (1, 2, 16000), f"WAV: format is {params}")
expect(len(audio) == info["audio_samples"], f"WAV: {len(audio)} samples in file, info.json says {info['audio_samples']}")
expect(info["audio_dropped"] == 0, f"audio: {info['audio_dropped']} samples dropped (card too slow)")
expect(abs(info["audio_missing"]) <= 1024, f"audio: {info['audio_missing']} samples missing against the clock")
expect(info["sd_error"] == 0, "card reported a write error")
zero_run = longest = 0
for is_zero in (audio == 0):
    zero_run = zero_run + 1 if is_zero else 0
    longest = max(longest, zero_run)
audio_s = len(audio) / 16000
rms = float(np.sqrt(np.mean((audio - audio.mean()) ** 2))) if len(audio) else 0

print(f"Video : {info['width']}x{info['height']} q{info['quality']}, {len(chunks)} Bilder in {video_s:.1f} s = "
      f"{info['fps_avg']:.2f} Bilder/s (Ziel {info['fps_target']}), {len(raw) / 1e6:.1f} MB, "
      f"{np.mean([c[1] for c in chunks]) / 1024 if chunks else 0:.1f} KB/Bild")
print(f"Timing: Abstand Median {np.median(gaps):.0f} ms, laengster {gaps.max()} ms, "
      f"{int((gaps > 1.5 * nominal).sum())} Luecken ueber {1.5 * nominal:.0f} ms, Sensor-Bilder uebersprungen {info['frames_skipped']}")
print(f"Audio : {audio_s:.2f} s, fehlend laut Uhr {info['audio_missing']} Samples, verworfen {info['audio_dropped']}, "
      f"Pegel RMS {rms:.0f}, Spitze {int(np.abs(audio).max()) if len(audio) else 0}, Gleichanteil {audio.mean():.0f}, "
      f"uebersteuert {int((np.abs(audio) >= 32767).sum())}, laengste Nullfolge {longest}")
print(f"Sync  : Audio beginnt bei {info['audio_t0_ms']:.0f} ms, Video bei {info['video_t0_ms']:.0f} ms; "
      f"Audio {audio_s:.2f} s gegen Video {video_s:.2f} s")
print(f"Last  : Schreiben max {info['video_write_max_ms']} ms (Video) / {info['audio_write_max_ms']} ms (Audio), "
      f"Schleife max {info['loop_max_ms']} ms, Audiopuffer max {info['ring_high_pct']} %, Ende: {info['stop_reason']}")
print("ERGEBNIS:", "OK" if not problems else "PROBLEME\n  - " + "\n  - ".join(problems))
sys.exit(1 if problems else 0)
