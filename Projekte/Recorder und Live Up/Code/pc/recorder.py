"""Audio-Recorder, PC-Seite: nimmt Aufnahmen vom Geraet an, transkribiert sie und schreibt sie ins Brain.

Starten:        start.bat  (oder .venv\\Scripts\\python.exe recorder.py)
Stimme lernen:  .venv\\Scripts\\python.exe recorder.py enroll Tobi stimmprobe.wav

Das Geraet schickt jede fertige Datei per
    PUT http://<pc>:8765/upload/JJJJMMTT-HHMMSS.wav   (Header X-Token: <token.txt>, X-SHA256: <Pruefsumme>)
Der PC legt sie aufs NAS, liest sie von dort zurueck und antwortet erst dann 200 mit der Pruefsumme.
Nur bei genau dieser Antwort gilt die Datei auf dem Geraet als gesichert.
"""
import contextlib
import hashlib
import http.server
import json
import os
import re
import secrets
import shutil
import subprocess
import sys
import tempfile
import threading
import time
import traceback
import urllib.error
import urllib.request
from datetime import datetime, timedelta
from pathlib import Path

import numpy as np
import sherpa_onnx
from faster_whisper import WhisperModel, decode_audio

BASE = Path(__file__).resolve().parent
INBOX, FAILED, VOICES = (BASE / d for d in ("inbox", "failed", "voices"))
ARCHIVE = Path(os.environ.get("RECORDER_ARCHIVE", "//jacobnas/JacobNAS/Tobi/Recorder/audio"))
VAULT = Path(os.environ.get("BRAIN_ROOT", str(Path.home() / "Documents" / "AIs Room")))
BRAINCTL = Path.home() / ".brain-sync" / "v2" / "brainctl.py"
PORT = 8765
WHISPER = dict(model_size_or_path="large-v3-turbo", device="cpu", compute_type="int8")
SPEAKER_MODEL = BASE / "models" / "campplus.onnx"
SAME_VOICE = 0.5  # Stellschraube: hoeher = strenger. An echten Aufnahmen nachjustieren.
SR = 16000
NAME = re.compile(r"\d{8}-\d{6}\.wav")

# Omi (omi.me): liegt omi_key.txt (Developer-Key omi_dev_... aus der Omi-App) im Ordner, geht jedes Transkript
# zusaetzlich als Unterhaltung an Omi. Ohne die Datei bleibt alles lokal.
OMI_KEY_FILE = BASE / "omi_key.txt"
OMI_OUT = BASE / "omi_outbox"
OMI_URL = os.environ.get("OMI_URL", "https://api.omi.me") + "/v1/dev/user/conversations/from-segments"
OMI_ME = "Tobi"  # Name aus "enroll": diese Stimme gilt bei Omi als Besitzer

TOKEN_FILE = BASE / "token.txt"
if not TOKEN_FILE.exists():
    TOKEN_FILE.write_text(secrets.token_hex(16))
TOKEN = TOKEN_FILE.read_text().strip()


def start_of(name):
    return datetime.strptime(name[:15], "%Y%m%d-%H%M%S")


def sha256_of(path):
    digest = hashlib.sha256()
    with open(path, "rb") as f:
        while block := f.read(1 << 20):
            digest.update(block)
    return digest.hexdigest()


class Upload(http.server.BaseHTTPRequestHandler):
    def do_PUT(self):
        name = self.path.removeprefix("/upload/")
        if not secrets.compare_digest(self.headers.get("X-Token", ""), TOKEN):
            return self.answer(403, "falsches Token")
        try:
            day = f"{start_of(name):%Y-%m-%d}" if NAME.fullmatch(name) else None
        except ValueError:
            day = None
        size = int(self.headers.get("Content-Length") or 0)
        want = self.headers.get("X-SHA256", "").lower()
        if not day or not 0 < size < 500_000_000 or len(want) != 64:
            return self.answer(400, "ungueltig")
        stored = ARCHIVE / day / name
        try:
            if stored.exists():  # schon gesichert, nur die Antwort ging verloren
                return self.answer(200, want) if sha256_of(stored) == want else self.answer(409, "anderer Inhalt")
            part = INBOX / (name + ".part")
            digest, left = hashlib.sha256(), size
            with open(part, "wb") as f:
                while left and (block := self.rfile.read(min(left, 1 << 20))):
                    f.write(block)
                    digest.update(block)
                    left -= len(block)
            if left or digest.hexdigest() != want:
                part.unlink()
                return self.answer(400, "unvollstaendig oder beschaedigt")
            stored.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(part, stored.with_suffix(".part"))
            os.replace(stored.with_suffix(".part"), stored)
            # ponytail: Zuruecklesen kann aus dem SMB-Cache von Windows kommen; reicht gegen Uebertragungsfehler
            if sha256_of(stored) != want:
                stored.unlink()
                return self.answer(500, "NAS-Pruefung fehlgeschlagen")
            os.replace(part, INBOX / name)
        except OSError as e:
            return self.answer(503, f"Ablage nicht erreichbar: {e}")
        self.answer(200, want)

    def answer(self, code, text):
        body = text.encode()
        self.send_response(code)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)


class Voices:
    def __init__(self):
        config = sherpa_onnx.SpeakerEmbeddingExtractorConfig(model=str(SPEAKER_MODEL), num_threads=2)
        self.extractor = sherpa_onnx.SpeakerEmbeddingExtractor(config)
        self.known = sherpa_onnx.SpeakerEmbeddingManager(self.extractor.dim)
        for file in VOICES.glob("*.npy"):
            self.known.add(file.stem, np.load(file))
        self.unknown = 0

    def embed(self, clip):
        stream = self.extractor.create_stream()
        stream.accept_waveform(sample_rate=SR, waveform=clip)
        stream.input_finished()
        return np.array(self.extractor.compute(stream), dtype=np.float32)

    def who(self, clip):
        vector = self.embed(clip)
        name = self.known.search(vector, threshold=SAME_VOICE)
        if not name:
            # ponytail: Unbekannte nur pro Programmlauf durchnummeriert; wer oft vorkommt, per enroll benennen
            self.unknown += 1
            name = f"Person {self.unknown}"
            self.known.add(name, vector)
        return name


def append_note(day, lines):
    rel = f"Transkripte/{day:%Y-%m-%d}.md"
    path = VAULT / rel
    old = path.read_bytes() if path.exists() else None
    text = old.decode("utf-8") if old else (
        f"---\ndatum: {day:%Y-%m-%d}\ntags: [transkript, audio]\n---\n\n"
        f"# Transkript {day:%Y-%m-%d}\n\nAutomatisch vom [[Audio-Recorder]].\n")
    text += "\n" + "\n\n".join(lines) + "\n"
    with tempfile.TemporaryDirectory() as tmp:
        draft = Path(tmp) / "entwurf.md"
        draft.write_bytes(text.encode("utf-8"))
        expected = hashlib.sha256(old).hexdigest() if old is not None else "-"
        result = subprocess.run(
            [sys.executable, str(BRAINCTL), "put", "--path", rel, "--expected", expected, "--input", str(draft)],
            capture_output=True, text=True, encoding="utf-8")
    if result.returncode or '"gespeichert"' not in result.stdout:
        raise RuntimeError(f"brainctl put {rel}: {result.stdout}{result.stderr}")


def omi_queue(wav, start, spoken):
    """Legt das Transkript fuer Omi bereit; omi_flush schickt es ab. Ohne omi_key.txt passiert nichts."""
    if not OMI_KEY_FILE.exists() or not spoken:
        return
    OMI_OUT.mkdir(exist_ok=True)
    numbers = {}
    segments = [{"text": text, "start": round(a, 2), "end": round(b, 2), "is_user": who == OMI_ME,
                 "speaker": f"SPEAKER_{numbers.setdefault(who, len(numbers)):02d}"} for a, b, who, text in spoken]
    for i in range(0, len(segments), 500):  # Omi nimmt hoechstens 500 Segmente je Anfrage
        part = segments[i:i + 500]
        job = {"transcript_segments": part, "language": "de", "source": "external_integration",
               "started_at": (start + timedelta(seconds=part[0]["start"])).astimezone().isoformat(),
               "client_session_id": f"{wav.stem}-{i}"}  # gleiche Kennung = kein Doppel bei Wiederholung
        (OMI_OUT / f"{wav.stem}-{i:05d}.json").write_text(json.dumps(job, ensure_ascii=False), encoding="utf-8")


def omi_flush():
    if not OMI_KEY_FILE.exists():
        return
    headers = {"Authorization": "Bearer " + OMI_KEY_FILE.read_text().strip(), "Content-Type": "application/json"}
    for job in sorted(OMI_OUT.glob("*.json")):
        try:
            urllib.request.urlopen(urllib.request.Request(OMI_URL, job.read_bytes(), headers), timeout=60).close()
        except urllib.error.HTTPError as e:
            print(f"Omi {job.name}: HTTP {e.code} {e.read()[:200]!r}", flush=True)
            if e.code not in (400, 413, 422):
                return  # Schluessel, Limit (30 je Stunde) oder Serverfehler: naechste Runde
            job.rename(job.with_suffix(".abgelehnt"))  # am Inhalt liegt es: nicht ewig wiederholen
            continue
        except OSError as e:
            return print(f"Omi nicht erreichbar: {e}", flush=True)
        job.unlink()


def process(wav, model, voices):
    start = start_of(wav.name)
    audio = decode_audio(str(wav), sampling_rate=SR)
    segments, _ = model.transcribe(audio, language="de", vad_filter=True)
    target = ARCHIVE / f"{start:%Y-%m-%d}" / wav.name
    days, speaker, spoken = {}, "?", []
    for seg in segments:
        if not (text := seg.text.strip()):
            continue
        clip = audio[int(seg.start * SR):int(seg.end * SR)]
        if len(clip) >= SR:  # unter 1 s ist die Stimme unsicher: gilt als letzter Sprecher
            speaker = voices.who(clip)
        at = start + timedelta(seconds=seg.start)
        if at.date() not in days:
            days[at.date()] = [f"### {at:%H:%M} · [Audio](<{target.as_uri()}>)"]
        days[at.date()].append(f"**{at:%H:%M:%S} {speaker}:** {text}")
        spoken.append((seg.start, seg.end, speaker, text))
    for day, lines in days.items():
        append_note(day, lines)
    omi_queue(wav, start, spoken)
    wav.unlink()  # das Original liegt geprueft auf dem NAS
    print(f"{wav.name}: {sum(len(v) - 1 for v in days.values())} Saetze", flush=True)


def worker(model, voices):
    while True:
        for wav in sorted(INBOX.glob("*.wav")):
            try:
                process(wav, model, voices)
            except Exception:
                traceback.print_exc()
                with contextlib.suppress(OSError):  # zum erneuten Transkribieren zurueck nach inbox/ legen
                    shutil.move(wav, FAILED / wav.name)
        omi_flush()
        time.sleep(10)


def main():
    for folder in (INBOX, FAILED, VOICES):
        folder.mkdir(exist_ok=True)
    if sys.argv[1:2] == ["enroll"]:
        name, sample = sys.argv[2:4]
        np.save(VOICES / f"{name}.npy", Voices().embed(decode_audio(sample, sampling_rate=SR)))
        return print(f"Stimme {name} gespeichert")
    model = WhisperModel(**WHISPER, download_root=str(BASE / "models"))
    threading.Thread(target=worker, args=(model, Voices()), daemon=True).start()
    print(f"Warte auf Aufnahmen an Port {PORT} (Token: {TOKEN_FILE})", flush=True)
    http.server.ThreadingHTTPServer(("", PORT), Upload).serve_forever()


if __name__ == "__main__":
    main()
