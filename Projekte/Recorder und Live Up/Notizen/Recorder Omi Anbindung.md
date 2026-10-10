---
titel: Recorder Omi Anbindung
tags: [projekt, audio, ki, cloud, omi]
erstellt: 2026-10-10
aktualisiert: 2026-10-10
---

# Recorder Omi Anbindung

Tobis Ziel vom 10.10.2026: den [[Audio-Recorder]] über die Cloud mit **Omi** verbinden (Open-Source-Projekt von BasedHardware, `github.com/BasedHardware/omi`, MIT-Lizenz; App, Backend und Firmware offen). Omi macht aus Transkripten Unterhaltungen, Erinnerungen und Aufgaben und hat einen Chat darüber.

## Ergebnis der Recherche (Claude, 10.10.2026)

Es geht, auf zwei Wegen:

1. **Über die Cloud (gewählt, gebaut):** Die PC-Seite transkribiert wie bisher lokal und schickt nur den Text an Omi. Developer-API: `POST https://api.omi.me/v1/dev/user/conversations/from-segments`, Anmeldung mit einem Developer-Key (`omi_dev_…`, in der Omi-App unter Developer → API Keys). Felder: `transcript_segments` (1 bis 500 Stück mit `text`, `start`, `end`, `speaker`, `is_user`), `language`, `started_at`, `source`, `client_session_id` (gleiche Kennung = kein Doppel). Limit: 30 Anfragen je Stunde; der Recorder braucht bei 10-Minuten-Stücken 6.
2. **Über Bluetooth (nicht gebaut):** Der Recorder gibt sich als Omi-Gerät aus. Die Omi-App erwartet den Dienst `19B10000-E8F2-537E-4F6C-D104768A1214` mit Audio (`19B10001-…`) und Codec (`19B10002-…`), Pakete mit 3 Byte Kopf, Codec 0 = PCM 16 kHz/16 Bit (das nimmt AR1 schon auf), 20 = Opus. `omiGlass` im Omi-Repo läuft auf demselben Board (XIAO ESP32S3). Nachteile: Handy muss in der Nähe sein, die Omi-App transkribiert dann in der Cloud, und es kollidiert mit der verschlüsselten Fernbedienung aus [[Recorder AR1 Firmware]]. Ob die Omi-App auf dem S9+ (Android 9) läuft, ist nicht geprüft.

Einen Upload von Audiodateien bietet die Developer-API nicht; deshalb geht Text, nicht Ton.

## Was gebaut ist

In `A:/Recorder/recorder.py` (PC-Seite):

- Liegt `A:/Recorder/omi_key.txt` mit dem Developer-Key im Ordner, legt `process()` jedes Transkript zusätzlich als Auftrag in `omi_outbox/` ab; `omi_flush()` schickt die Aufträge ab. Ohne die Datei bleibt alles lokal wie bisher.
- Kein Netz, falscher Schlüssel oder Limit: der Auftrag bleibt liegen und wird alle 10 s neu versucht. Lehnt Omi den Inhalt ab (400, 413, 422), wird er in `.abgelehnt` umbenannt.
- Stimme „Tobi“ (aus `enroll`) gilt bei Omi als Besitzer (`is_user`); Stellschraube `OMI_ME`. Eigener Omi-Server: Umgebungsvariable `OMI_URL`.
- `selftest.py` prüft den Auftrag und dass er bei fehlendem Netz liegen bleibt: besteht (10.10.2026).

## Offen

- **Nicht gegen das echte Omi getestet**, weil es noch kein Konto und keinen Schlüssel gibt. Die Doku nennt für diesen Endpunkt an einer Stelle den Developer-Key, an anderer ein Firebase-Token; der erste echte Lauf zeigt es (Meldung `Omi …: HTTP 401`).
- Tobi muss selbst: Omi-App installieren, Konto anlegen, Key erzeugen (Schreibrecht für Unterhaltungen), in `omi_key.txt` legen. Der Key gehört nicht ins Brain.
- Mit Omi verlassen die Transkripte das Haus (Server von Omi). Bisher war das Ziel möglichst lokale Auswertung; das Omi-Backend lässt sich auch selbst betreiben.

## GitHub

Am 10.10.2026 wollte Tobi alles zu Recorder, Lader und [[Live Up]] lesbar in `Tobidervogel/Brain` haben. Das Repo ist öffentlich. Tobi hat entschieden: dazulegen statt löschen, mit Chatprotokollen. Es liegt unter `Projekte/Recorder und Live Up/` (Notizen, Code ohne Aufnahmen und Token, 20 Chatprotokolle); `changes/` und `brain.zip` sind unberührt. Der Ordner ist eine Kopie von diesem Tag und aktualisiert sich nicht von selbst.

Verwandt: [[Audio-Recorder]], [[Recorder AR1 Firmware]], [[Live Up]], [[Brain System]]
