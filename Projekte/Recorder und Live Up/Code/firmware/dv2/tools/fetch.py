"""Downloads one recording from the card over USB and checks it.

  python fetch.py <session number> [small|cut]     small = without video.avi and audio.wav
                                                   cut = recording that lost power (has no info.json)
"""

import subprocess
import sys
from pathlib import Path

import dv2

session = f"{int(sys.argv[1]):04d}"
names = ["log.txt"] if "cut" in sys.argv[2:] else ["info.json", "frames.csv", "log.txt"]
names += [] if "small" in sys.argv[2:] else ["audio.wav", "video.avi"]
folder = Path(__file__).resolve().parent.parent / "captures" / session
folder.mkdir(parents=True, exist_ok=True)
with dv2.connect() as s:
    for name in names:
        dv2.get(s, f"/rec/{session}/{name}", folder / name)
if "audio.wav" in names:
    sys.exit(subprocess.call([sys.executable, str(Path(__file__).with_name("check.py")), str(folder)]))
