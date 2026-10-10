"""Runs short recordings with different camera settings and prints what the board sustained.

  python matrix.py <seconds> <size:quality:fps> [<size:quality:fps> ...]     size 8=VGA 9=SVGA 10=XGA 11=HD 12=SXGA 13=UXGA
"""

import json
import sys

import dv2

seconds = int(sys.argv[1])
with dv2.connect() as s:
    for spec in sys.argv[2:]:
        size, quality, fps = spec.split(":")
        s.reset_input_buffer()
        s.write(f"REC {seconds} {size} {quality} {fps}\n".encode())
        for line in dv2.lines(s, seconds + 30, lambda l: l.startswith(("finished", "DV2_ERROR"))):
            if "ERROR" in line:
                print(line)
        s.write(b"STATUS\n")
        for line in dv2.lines(s, 5, lambda l: l.startswith("DV2_STATUS")):
            if line.startswith("DV2_STATUS"):
                i = json.loads(line.split(" ", 1)[1])
                frames = max(i["frames"], 1)
                print(f"{i['dir']} {i['width']}x{i['height']} q{i['quality']} Ziel {i['fps_target']:>5}: "
                      f"{i['fps_avg']:6.2f} Bilder/s, {i['video_bytes'] / frames / 1024:5.1f} KB/Bild, "
                      f"{i['video_bytes'] / 1024 / max(i['elapsed_s'], 1):5.0f} KB/s, groesstes {i['frame_max_bytes'] / 1024:.0f} KB, "
                      f"Luecke max {i['frame_gap_max_ms']} ms, spaet {i['frames_late']}, uebersprungen {i['frames_skipped']}, "
                      f"Schreiben max {i['video_write_max_ms']} ms, Schleife max {i['loop_max_ms']} ms, "
                      f"Audio fehlt {i['audio_missing']} verworfen {i['audio_dropped']} Puffer {i['ring_high_pct']} %, "
                      f"Fehler {i['sd_error']}/{i['fb_fail']}", flush=True)
