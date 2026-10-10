"""Selbsttest: .venv\\Scripts\\python.exe selftest.py  (Wegwerf-Vault und -Archiv, nicht Brain und NAS)"""
import hashlib
import os
import tempfile
import threading
import time
import urllib.error
import urllib.request
from pathlib import Path

os.environ["BRAIN_ROOT"] = vault = tempfile.mkdtemp()
os.environ["RECORDER_ARCHIVE"] = archive = tempfile.mkdtemp()
import recorder as r  # noqa: E402  (liest die Pfade beim Import)

r.OMI_KEY_FILE, r.OMI_OUT = Path(vault) / "omi_key.txt", Path(vault) / "omi_outbox"  # nichts an das echte Omi
r.OMI_KEY_FILE.write_text("omi_dev_test")
TEST = r.BASE / "test"
PROBE = (TEST / "probe.wav").read_bytes()
HASH = hashlib.sha256(PROBE).hexdigest()
NAME = "19991231-235958.wav"  # laeuft ueber Mitternacht


def put(name, token=r.TOKEN, data=PROBE, sha=HASH):
    req = urllib.request.Request(f"http://127.0.0.1:{r.PORT}/upload/{name}", data=data, method="PUT",
                                 headers={"X-Token": token, "X-SHA256": sha})
    try:
        with urllib.request.urlopen(req) as res:
            return res.status, res.read().decode()
    except urllib.error.HTTPError as e:
        return e.code, ""


for folder in (r.INBOX, r.FAILED):
    folder.mkdir(exist_ok=True)
threading.Thread(target=r.http.server.ThreadingHTTPServer(("127.0.0.1", r.PORT), r.Upload).serve_forever,
                 daemon=True).start()
ok = (200, HASH)
assert put(NAME, token="falsch")[0] == 403
assert put("../boese.wav")[0] == 400
assert put("19991399-000000.wav")[0] == 400
assert put(NAME, sha="0" * 64)[0] == 400  # Pruefsumme passt nicht: nichts wird gesichert
assert not (Path(archive) / "1999-12-31" / NAME).exists()
assert put(NAME) == ok
assert (Path(archive) / "1999-12-31" / NAME).read_bytes() == PROBE

voices = r.Voices()
voices.known.add("Hedda", voices.embed(r.decode_audio(str(TEST / "hedda.wav"), sampling_rate=r.SR)))
model = r.WhisperModel(**r.WHISPER, download_root=str(r.BASE / "models"))
t = time.time()
r.process(r.INBOX / NAME, model, voices)
print(f"{len(PROBE) / 32000:.0f} s Audio in {time.time() - t:.0f} s verarbeitet")
assert put(NAME) == ok and not (r.INBOX / NAME).exists()  # doppelter Upload wird nicht doppelt verarbeitet
assert put(NAME, data=b"x" * 100, sha=hashlib.sha256(b"x" * 100).hexdigest())[0] == 409  # nichts ueberschreiben

old = (Path(vault) / "Transkripte/1999-12-31.md").read_text(encoding="utf-8")
new = (Path(vault) / "Transkripte/2000-01-01.md").read_text(encoding="utf-8")
print(old, new, sep="\n")
assert "Hedda:" in old and "Person 1:" in new and "Hedda:" in new

job, = (r.json.loads(f.read_text(encoding="utf-8")) for f in r.OMI_OUT.glob("*.json"))
segs = job["transcript_segments"]
assert job["client_session_id"] == NAME[:-4] + "-0" and job["language"] == "de"
assert len({s["speaker"] for s in segs}) == 2 and all(s["end"] > s["start"] and s["text"] for s in segs)
r.OMI_URL = "http://127.0.0.1:9/"  # nicht erreichbar: der Auftrag muss liegen bleiben
r.omi_flush()
assert len(list(r.OMI_OUT.glob("*.json"))) == 1
print("OK")
