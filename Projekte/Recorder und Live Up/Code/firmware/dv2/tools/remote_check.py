"""Checks a long recording on the card without downloading the video: container structure,
three sample frames and the complete audio track.

  python remote_check.py <session number>
"""

import json
import struct
import sys
import wave
from pathlib import Path

import cv2
import numpy as np

import dv2

session = f"{int(sys.argv[1]):04d}"
folder = Path(__file__).resolve().parent.parent / "captures" / session
folder.mkdir(parents=True, exist_ok=True)
problems = []


def read(s, name, offset, length):
    """Bytes offset..offset+length of a file on the card; also returns the file size."""
    data, size = bytearray(), None
    while len(data) < length:
        block = dv2.get_block(s, f"/rec/{session}/{name}", offset + len(data))
        if block:
            size = block[0]
            data += block[1]
            if not block[1]:
                break
    return bytes(data[:length]), size


def expect(ok, text):
    if not ok:
        problems.append(text)


with dv2.connect() as s:
    s.timeout = 1
    for name in ("info.json", "frames.csv", "log.txt", "audio.wav"):
        dv2.get(s, f"/rec/{session}/{name}", folder / name)
    info = json.loads((folder / "info.json").read_text())
    frames, movi = info["frames"], info["video_bytes"]

    head, size = read(s, "video.avi", 0, 1024)
    u32 = lambda data, at: struct.unpack_from("<I", data, at)[0]
    expect(head[:4] == b"RIFF" and u32(head, 4) == size - 8, "AVI: RIFF size does not match the file size")
    expect(u32(head, 48) == frames and u32(head, 140) == frames, "AVI: frame count in header differs from info.json")
    expect(head[1012:1016] == b"LIST" and u32(head, 1016) == 4 + movi, "AVI: movi list size wrong")
    expect(size == 1024 + movi + 8 + 16 * frames, f"AVI: file size {size} is not header + frames + index")
    index, _ = read(s, "video.avi", 1024 + movi, 8 + 16 * frames)
    expect(index[:4] == b"idx1" and u32(index, 4) == 16 * frames, "AVI: index missing")
    for n in (0, frames // 2, frames - 1):
        _, _, offset, length = struct.unpack_from("<4sIII", index, 8 + 16 * n)
        jpeg, _ = read(s, "video.avi", 1020 + offset + 8, length)
        picture = cv2.imdecode(np.frombuffer(jpeg, np.uint8), cv2.IMREAD_COLOR)
        expect(picture is not None and picture.shape[:2] == (info["height"], info["width"]), f"frame {n} does not decode")
        (folder / f"frame_{n:05d}.jpg").write_bytes(jpeg)

with wave.open(str(folder / "audio.wav")) as w:
    audio = np.frombuffer(w.readframes(w.getnframes()), dtype=np.int16).astype(np.int32)
times = np.array([int(line.split(",")[1]) for line in (folder / "frames.csv").read_text().splitlines()[1:]])
gaps = np.diff(times)
expect(len(audio) == info["audio_samples"], "WAV: sample count differs from info.json")
expect(info["audio_dropped"] == 0 and abs(info["audio_missing"]) <= 1024, "audio: samples lost")
expect(info["sd_error"] == 0 and info["fb_fail"] == 0, "card or camera error")
expect(len(times) == frames, "frames.csv: wrong number of rows")

print(f"Video : {info['width']}x{info['height']} q{info['quality']}, {frames} Bilder in {info['elapsed_s']} s = "
      f"{info['fps_avg']:.2f} Bilder/s, {size / 1e6:.0f} MB ({movi / 1024 / info['elapsed_s']:.0f} KB/s), "
      f"groesstes Bild {info['frame_max_bytes'] / 1024:.0f} KB")
print(f"Timing: Abstand Median {np.median(gaps):.0f} ms, laengster {gaps.max()} ms, "
      f"{int((gaps > 150).sum())} Luecken ueber 150 ms, davon {int((gaps > 500).sum())} ueber 500 ms")
print(f"Audio : {len(audio) / 16000:.2f} s, fehlend laut Uhr {info['audio_missing']}, verworfen {info['audio_dropped']}, "
      f"RMS {np.sqrt(np.mean((audio - audio.mean()) ** 2)):.0f}, Spitze {np.abs(audio).max()}, "
      f"Gleichanteil {audio.mean():.0f}, uebersteuert {int((np.abs(audio) >= 32767).sum())}")
print(f"Last  : Schreiben max {info['video_write_max_ms']} / {info['audio_write_max_ms']} ms, Schleife max "
      f"{info['loop_max_ms']} ms, Audiopuffer max {info['ring_high_pct']} %, Heap min {info['heap_min']}, "
      f"Ende: {info['stop_reason']}")
print("ERGEBNIS:", "OK" if not problems else "PROBLEME\n  - " + "\n  - ".join(problems))
