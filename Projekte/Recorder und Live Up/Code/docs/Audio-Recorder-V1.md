# Audio Recorder V1 – Spezifikation und Phasenplan

Stand 06.10.2026, Fassung 2 (nach Gegenprüfung durch vier unabhängige Prüfer und dem ersten Gerätetest).
Verbindliche Zielrichtung von Tobi: Kamera raus, zuerst ein robuster Audio-Recorder.
Der ESP32 sammelt, das Handy steuert (und überbrückt unterwegs), der PC archiviert, transkribiert und
verknüpft im Brain.

```
Handy (Live Up, Modul „Recorder“)
  ↕ Bluetooth LE (Steuerung, Metadaten)        ↕ WLAN (Dateien, Phase 4)
ESP32-Audio-Recorder (XIAO ESP32S3 Sense, Firmware AR1)
  ↕ WLAN (Heimnetz oder Handy-Hotspot), der Hub holt ab
PC-Recorder-Hub (A:/Recorder/recorder.py)
  ↓
NAS + Transkripte + Brain-Verweise
```

Leitsatz: **BLE steuert. WLAN überträgt. Der PC archiviert. Das Brain indexiert.**

## 1. Bestandsaufnahme (06.10.2026, am Gerät geprüft)

| Bereich | Stand |
|---|---|
| Gerät | XIAO ESP32S3 Sense an COM6. Bis heute Mittag Firmware DV2.0; die Kamera ist abgezogen, der Ton lief weiter. |
| WLAN | Antenne steckt. Die gespeicherten Zugangsdaten für JacobHome funktionieren: 192.168.0.112, −70 bis −77 dBm (am Schreibtisch schwach, einzelne Anmeldeversuche laufen in die 30-s-Grenze und klappen beim nächsten Mal). Uhrzeit per NTP vom Router. Scan findet 4 Netze. Statusseite vom PC (192.168.0.23) in 0,3 s erreichbar, auch während der Aufnahme. |
| Karte | 64 GB FAT32, 60,9 GB. Alte DV2-Ordner `/rec/0021` bis `/rec/0024` (dürfen laut Tobi gelöscht werden; AR1 beachtet nur sechsstellige Ordner). |
| Firmware DV2 | `A:/Recorder/firmware/dv2`, bleibt als Rückfallstand unverändert. Übernommen: Mikrofon-Einstellungen, Ringpuffer im PSRAM, blockweises Schreiben mit `fsync`, WAV-Kopf der auch abgeschnitten abspielbar bleibt, CRC-Blocktransfer über USB, WLAN-Einrichtung ohne Zugangsdaten im Quelltext. |
| PC-Seite | `A:/Recorder/recorder.py`: Empfang per `PUT` (Token, SHA-256), Ablage auf NAS mit Zurücklesen, faster-whisper, Stimmen, Transkript per `brainctl put`, Selbsttest. |
| Handy | S9+ (Android 9) über WLAN-ADB erreichbar, Live Up 0.8.0 installiert (= Quellstand `A:/LiveUp`). Bluetooth ist am Handy ausgeschaltet. |
| Werkzeuge | PlatformIO 6.2, Arduino-ESP32 2.0.17 (bringt die BLE-Bibliothek mit), Python mit `cryptography`. **Kein C++-Compiler für den PC und kein Bluetooth am PC**: Firmware-Logik lässt sich vor dem Flashen nicht als Unit-Test prüfen, BLE nicht vom PC aus. |

## 2. Zustände

| Zustand | Bedeutung | Erreichbar über |
|---|---|---|
| `idle` | Aufnahme aus, Gerät wach; unfertige Aufnahmen werden abgeschlossen (`finalizing`) | BLE, WLAN, USB |
| `recording` | Aufnahme läuft | BLE, USB, WLAN nur `/status.json` |
| `paused` | Aufnahme unterbrochen, alle Dateien abgeschlossen | BLE, WLAN, USB |
| Synchronisation | wie `idle`; der Hub oder das Handy holt Dateien ab | BLE, WLAN, USB |
| Tiefschlaf | Chip schläft | nichts; nur Timer, Reset-Taster oder Strom neu |
| stromlos | kein Akku, kein USB | nichts |

Aus Tiefschlaf oder ohne Strom lässt sich das Gerät nicht per Funk wecken. Bis Phase 6 wird nur ohne laufende
Aufnahme synchronisiert (Stopp, Pause, Ausschalten).

**Aufnehmen nach einem Neustart:** Bei Strom neu (`reset=poweron`) entscheidet `AUTOSTART` (Standard: an). Nach
jedem anderen Neustart (Watchdog, Absturz, Unterspannung, `REBOOT`, Aufwachen aus dem Tiefschlaf) gilt der zuletzt
per Befehl gewünschte Zustand, der dafür im NVS steht: Wer gestoppt oder pausiert hat, wird nicht ohne sein Wissen
wieder aufgenommen. Timer liegen nur im RAM und gehen bei einem Neustart verloren.

## 3. Firmware AR1

Ordner `A:/Recorder/firmware/ar1`. Die WLAN-Zugangsdaten bleiben unverändert im NVS-Bereich `dv1wifi`
(Format nicht ändern, nie mit Flash-Löschen aufspielen), müssen also nicht neu eingegeben werden.

### 3.1 Aufbau

- **Audio-Task** (Kern 1, Priorität 10): läuft immer. Liest das PDM-Mikrofon (GPIO42 Takt, GPIO41 Daten, 16 kHz,
  16 Bit, Mono), zieht den Gleichanteil ab, verstärkt ×4 mit Begrenzung. Während einer Aufnahme legt er ganze
  512-Sample-Blöcke in den Ringpuffer (512 KB im PSRAM = 16 s); ist der Ring voll, wird der Block verworfen und gezählt.
- **Recorder-Task** (Kern 1, Priorität 5, Watchdog 15 s): gehört allein alle Dateien. Leert den Ring, schreibt
  sektorweise, `fsync` alle 2 s, wechselt Segmente, rechnet SHA-256, schreibt Journal und `info.json`, hängt die
  Karte ein und schließt im Leerlauf unfertige Aufnahmen ab.
- **Hauptschleife** (Priorität 1): USB, BLE-Befehle, WLAN, Webserver, Timer. Sie schickt dem Recorder-Task
  Aufträge und liest eine Kopie seines Zustands.
- Kamera wird nicht initialisiert; der Treiber ist nicht eingebunden.

Die Aufnahme hat damit Vorrang: Hauptschleife und Funk können das Schreiben nicht anhalten, ausgenommen kurze
Kartenzugriffe (`LS`, `DIR`), die sich die Karte über die Sperre des Dateisystems mit dem Recorder-Task teilen.
Während der Aufnahme sind `GET`, `RM`, `BENCH`, `FORMAT` und Datei-Downloads gesperrt. Wie voll der Ring dabei
wird, steht im Journal und wird in den Tests gemessen statt zugesagt.

### 3.2 Dateien auf der Karte

```
/device.log                 je Start eine Zeile BOOT, dazu TIMESYNC-Zeilen (Boot, Laufzeit, Uhrzeit)
/rec/000001/
  0000.wav 0001.wav ...     Segmente
  info.json                 Übersicht, aus dem Journal erzeugt
  log.txt                   Journal: nur anhängen, jede Zeile sofort gesichert
  0000.ok / 0000.okp        (ab Phase 3) Marker „vom Hub archiviert“ / „auf dem Handy gesichert“
```

- Ordner = fortlaufende sechsstellige Nummer (NVS-Zähler, vorhandene Ordner werden übersprungen). Der Ordner heißt
  bewusst nicht wie die Aufnahme-ID: kurze Namen, die ID steht in `info.json`.
- **Aufnahme-ID** = `<gerät>-<nummer>-<zufall>`, z. B. `d0a130-000002-d76b290b`. `gerät` = letzte drei Bytes der
  WLAN-MAC. Die acht Zufallsstellen halten die ID eindeutig, auch wenn der Zähler nach einem Flash-Löschen neu beginnt.
- **Segment**: genau 9 600 000 Samples (600 s). Der Wechsel passiert auf das Sample genau im Datenstrom. WAV-Kopf
  512 Byte (Daten beginnen an einer Sektorgrenze). Während der Aufnahme stehen die Längen auf `0xFFFFFFFF`, beim
  Abschluss werden die echten Längen eingetragen und gesichert, erst danach kommt die `SEG`-Zeile.
- **SHA-256** gilt für die ganze Datei. Der Kopf eines vollen Segments ist vorab bekannt und geht zuerst in die
  Prüfsumme, danach laufend die Daten: volle Segmente kosten keinen Lesezugriff. Ein Segment, das früher endet
  (Stopp, Pause, Abbruch), wird sofort abgeschlossen (`sha256=-`); seine Prüfsumme wird im Leerlauf nachgetragen.
  Stopp und Pause dauern deshalb nicht länger als ein paar Zehntelsekunden.

### 3.3 Journal `log.txt`

Textzeilen `SCHLÜSSELWORT name=wert ...`. Maßgeblich für alles, was `info.json` enthält. Eine Zeile gilt nur,
wenn sie vollständig ist (Zeilenende); die letzte kann durch einen Stromausfall abgeschnitten sein.

```
REC id=d0a130-000002-d76b290b n=2 fw=AR1.0 dev=d0a130 boot=1 reset=poweron seg_s=600 rate=16000 unix=0 time_src=none up_ms=3120
OPEN n=0 file=0000.wav t_ms=0 unix=0
SEG n=0 file=0000.wav t_ms=0 samples=9600000 bytes=19200512 dropped=0 missing=0 gap_max_ms=63 ring_max=2048 write_max_ms=56 sha256=<64 hex | -> end=full wifi=1 rssi=-76 power=unknown
STAT t_s=60 seg=0 samples=960000 dropped=0 missing=0 ring=0 ring_max=2048 write_max_ms=56 gap_max_ms=63 heap=121000 wifi=1 rssi=-76
TIMESYNC t_ms=123456 unix=1791285318 src=ntp
PAUSE t_ms=... / RESUME t_ms=...
ERROR what=sd_write
END reason=stop_ble t_ms=... samples=... dropped=... captured=... clock_ms=...
RECOVERED n=3 ... sha256=<64 hex> end=interrupted      (Segment ohne SEG-Zeile, im Leerlauf abgeschlossen)
HASH n=3 sha256=<64 hex>                               (Prüfsumme eines Segments, das mit sha256=- geschlossen wurde)
DONE unix=...                                          (letzte Zeile: alles abgeschlossen und geprüft)
```

- `t_ms` = Millisekunden seit Aufnahmebeginn nach der Geräteuhr (Pausen und verworfene Blöcke zählen mit).
- `dropped` = verworfen, weil der Ring voll war. `missing` = Samples laut Uhr minus gelieferte Samples
  (Verlust vor dem Ring, etwa im I2S-Treiber); `gap_max_ms` = längster Abstand zwischen zwei Mikrofonblöcken.
  `missing` und `gap_max_ms` gelten seit dem letzten Start oder Fortsetzen.
- `END`: `captured − dropped` muss `samples` ergeben, und `captured` muss zu `clock_ms` passen. Sonst ist unterwegs
  Ton verloren gegangen. `reason`: `stop_usb`, `stop_ble`, `timer`, `sleep`, `shutdown`, `reboot`, `card_full`,
  `max_segments`, `mic_error`, `sd_error`, `interrupted`.
- `end` eines Segments: `full`, `stop`, `pause`, `interrupted`.
- `STAT` alle 60 s.

### 3.4 `info.json`

Wird bei Start, nach jedem Segment, bei Stopp und beim Abschließen **aus dem Journal neu erzeugt**: `info.new`
schreiben und sichern, `info.json` löschen, umbenennen (FAT kann nicht auf eine vorhandene Datei umbenennen). Fehlt
`info.json`, wird sie beim nächsten Leerlauf neu erzeugt.

```json
{
  "schema": 1, "fw": "AR1.0", "device": "d0a130", "boot": 1, "reset": "poweron",
  "rec_id": "d0a130-000002-d76b290b", "rec_n": 2,
  "start_unix": 1791286842, "time_src": "ntp | phone | usb | rtc | none", "start_up_ms": 3120,
  "sample_rate": 16000, "bits": 16, "channels": 1, "segment_s": 600,
  "segments": [
    {"n": 0, "file": "0000.wav", "t_ms": 0, "unix": 1791286842, "duration_s": 600.0,
     "samples": 9600000, "bytes": 19200512, "dropped": 0, "missing": 0, "gap_max_ms": 63,
     "ring_max_pct": 0.4, "write_max_ms": 56, "sha256": "<64 hex>", "end": "full",
     "wifi": true, "rssi": -76, "power": "unknown"}
  ],
  "state": "open | stopped | closed | recovered", "stop_reason": "stop_ble",
  "samples": 28800000, "dropped": 0, "duration_s": 1800.0
}
```

- `state`: `open` = läuft oder wurde abgebrochen und ist noch nicht abgeschlossen; `stopped` = beendet, Prüfsummen
  werden noch nachgetragen; `closed` = fertig; `recovered` = fertig, aber nach einem Abbruch wiederhergestellt.
  **Der Hub holt nur `closed` und `recovered`.**
- **Zeit:** Gilt eine `TIMESYNC`-Zeile, ist `start_unix` = `unix − t_ms/1000` der letzten, sonst der Wert der
  `REC`-Zeile. Jedes Segment bekommt `start_unix + t_ms/1000`. Ohne beides bleibt 0; der Hub findet die Zeit dann
  über `/device.log` (gleicher Boot, `up_ms`) oder nimmt das Abholdatum und vermerkt „Zeit unsicher“.
  `rtc` heißt: Die Uhr lief über einen Neustart weiter, in diesem Lauf gab es noch keine Quelle.
- `power` ist ab AR1.2 `charging`, `external` (am Ladegerät, Akku voll oder nicht da), `battery` oder `unknown`
  (keine Messschaltung angelötet; bis AR1.1 immer `unknown`). Der WLAN-Zustand ist echt.

### 3.5 Abbruch, Stromausfall und Fehler

- Offenes Segment: höchstens die letzten 2 s fehlen, die Datei bleibt abspielbar. Abgeschlossene Segmente werden
  nie wieder beschrieben. Das ist das Ziel; belegt ist, was die Tests zeigen (hartes Zurücksetzen automatisch,
  echtes Steckerziehen von Hand).
- **Abschließen im Leerlauf** (ein Weg für alles, wiederholbar, bricht bei einem Startauftrag sofort ab):
  abgeschnittene letzte Journalzeile abtrennen → jede `NNNN.wav` ohne `SEG`-Zeile: Kopf aus der Dateigröße eintragen,
  SHA-256 rechnen, `RECOVERED` → jedes Segment mit `sha256=-`: Prüfsumme rechnen, `HASH`-Zeile anhängen →
  fehlt `END`: `END reason=interrupted` (oder `sd_error`) → `info.json` → `DONE`.
  Solange aufgenommen wird, läuft das nicht; `STATUS` meldet `finalizing`. `SHUTDOWN` wartet darauf, `SLEEP` nicht.
- Schreibfehler: Der Kartentreiber gibt die Karte dann auf, das Journal ist nicht mehr schreibbar. Also: Dateien
  loslassen, Karte neu einhängen, `ERROR what=sd_write` ins alte Journal, neue Aufnahme starten (höchstens 5-mal
  hintereinander, mit Pause). Das Abschließen erledigt später den Rest.
- Zu wenig Platz (weniger als 64 MB oder weniger als ein Segment plus Reserve): Aufnahme endet mit `card_full`,
  es wird kein neuer Ordner angelegt. V1 löscht nie von selbst ungesicherte Aufnahmen.
- Mikrofon liefert 5 s nichts: `END reason=mic_error`, Neustart.
- Die Belegung wird einmal beim Einhängen gelesen (kann bei einer frisch formatierten Karte mehrere Sekunden
  dauern, dabei ist der Watchdog ausgesetzt), danach kostet die Prüfung nichts.
- Fehlerzähler und die letzten zehn Fehler bleiben im RAM und sind per `ERRORS` abrufbar; jeder Neustart steht mit
  Grund in `/device.log` und im nächsten Journal.

### 3.6 Befehle (USB und BLE, gleicher Wortlaut)

Antwort ist immer genau ein JSON-Objekt, über USB mit dem Präfix `AR `. Befehle antworten sofort mit dem
angenommenen Zustand; der Fortschritt steht in `STATUS`.

| Befehl | Wirkung |
|---|---|
| `STATUS` | Zustand, Aufnahme, Karte, WLAN (verbundenes Netz, gespeicherte Netze, IP), Bluetooth, Zeit, Timer, Fehler, `finalizing` |
| `REC START` / `REC STOP` | Aufnahme starten (eine pausierte fortsetzen) / sauber beenden |
| `REC PAUSE` / `REC RESUME` | Segment abschließen und warten / nächstes Segment derselben Aufnahme |
| `SLEEP [sekunden]` | sauber beenden, sofort Tiefschlaf; optional nach n Sekunden wieder wach (dann Leerlauf) |
| `SHUTDOWN` | beenden, abschließen, Synchronisation abwarten (ab Phase 3), Tiefschlaf |
| `TIMER <stop\|sleep\|shutdown\|start> <sekunden>` / `TIMER OFF` | einmaliger Timer, kein Wochenplan |
| `TIME <unix>` | Uhr stellen (Handy oder PC) |
| `AUTOSTART ON\|OFF` | bei Strom neu sofort aufnehmen |
| `LS [ab]` | Aufnahmen, 20 je Antwort (`done` = abgeschlossen und geprüft) |
| `SEGS <aufnahme> [ab]` | Segmente einer Aufnahme mit Länge, Abschluss, Prüfsumme vorhanden, Sicherungsmarker |
| `WIFI SET <ssid_hex> <passwort_hex>` | Netz speichern und verbinden (zwei Plätze: ein bekanntes Netz wird ersetzt, sonst der freie, sonst der zweite) |
| `WIFI ON\|OFF`, `WIFI FORGET <ssid_hex>`, `WIFI SCAN` (nur ohne Aufnahme) | WLAN schalten, Netz löschen, Netze suchen |
| `HTTP_AUTH` | Web-Login für Hub und Handy (über BLE verschlüsselt, über USB als `AR_SECRET`) |
| `DF` | Speicherplatz |
| `ERRORS` | Fehlerzähler und letzte Fehler |
| `REBOOT` | Neustart nach sauberem Abschluss; der gewünschte Zustand (aufnehmen oder nicht) bleibt |

Nur über USB: `GET <offset> <länge> <pfad>` und `DIR <pfad>` (Blocktransfer mit CRC), `RM /rec/<ordner>`,
`BLE_KEY` und `BLE_KEY NEW` (Kopplungsdaten), `SELFTEST`, `FORMAT YES` (nur wenn die Karte nicht lesbar ist) und
die Testschalter `SEGLEN <s>`, `MINFREE <mb>`, `FAULT sd` (nicht dauerhaft).

**Zugangsdaten** (Web-Login, Geräteschlüssel) kommen über USB nur in Zeilen mit dem Präfix `AR_SECRET ` bzw.
`DV1_WIFI_AUTH `; die PC-Werkzeuge geben solche Zeilen nie aus. Sie erscheinen nie im Journal und nie in `STATUS`.

### 3.7 Bluetooth LE (Phase 2)

Der ESP32-S3 kann nur Bluetooth LE. Ein eigener GATT-Dienst:

| | UUID | Eigenschaften |
|---|---|---|
| Dienst | `73b90001-76e6-4959-9847-35866b493c85` | |
| `HELLO` | `73b90002-76e6-4959-9847-35866b493c85` | lesen, schreiben |
| `RX` | `73b90003-76e6-4959-9847-35866b493c85` | schreiben mit Antwort |
| `TX` | `73b90004-76e6-4959-9847-35866b493c85` | Indication (mit Deskriptor 0x2902) |

- Gerätename `Recorder-<4 Hex>`. Eine Verbindung zur Zeit: während der Verbindung wird nicht geworben, nach dem
  Trennen startet die Hauptschleife das Werben neu. Kommt 10 s nach dem Verbinden kein gültiger Rahmen, trennt das Gerät.
- Das Handy verbindet sich direkt über die **Bluetooth-Adresse** aus der Kopplung (`BLEDevice::getAddress()`,
  nicht die WLAN-MAC), ohne Suche: auf Android 9 bräuchte die Suche die Standortfreigabe.
- Die Bibliotheks-Callbacks laufen im Bluetooth-Task mit kleinem Stack. Sie prüfen nur die Länge (`HELLO` genau
  17 Byte, `RX` 21 bis 220 Byte) und legen die Bytes in eine Warteschlange. Entschlüsseln, Ausführen und Antworten
  geschieht in der Hauptschleife.
- MTU: Das Gerät setzt 247, das Handy fordert 247. Jeder Rahmen passt in ein Paket; bei weniger als 223 trennt das
  Handy und versucht es neu.

**Authentifizierung und Verschlüsselung** liegen in der Anwendung, unabhängig vom Bluetooth-Pairing:

- Geräteschlüssel `K`: 32 Zufallsbytes, beim ersten BLE-Start erzeugt (Funk an, damit der Zufall echt ist), im NVS.
  Er verlässt das Gerät nur über USB (`BLE_KEY`), also nur mit dem Gerät in der Hand. `BLE_KEY NEW` ersetzt ihn.
- Für jede Aushandlung hält das Gerät ein frisches `Nd` (16 Zufallsbytes) in `HELLO` bereit (`0x01 ‖ Nd`): beim
  Verbinden und sofort wieder nach jedem Schreiben von `HELLO`.
  1. Handy liest `HELLO`.
  2. Handy schreibt `HELLO` ← `0x01 ‖ Np` (16 frische Zufallsbytes).
  3. Beide: `Ks = HMAC-SHA256(K, "AR1-session" ‖ Nd ‖ Np)`, beide Zähler beginnen neu.
  Eine neue Aushandlung auf derselben Verbindung ersetzt die Sitzung. Das ist nötig, weil Android die Funkstrecke
  einer eben geschlossenen Verbindung für die nächste weiterbenutzt: Das Gerät sieht dann keine neue Verbindung,
  nur ein zweites `HELLO` (am S9+ beobachtet). Weil `Nd` nie zweimal gilt, ist auch `Ks` jedes Mal neu.
  Beim Trennen werden `Ks` und beide Zähler gelöscht. Schlägt etwas fehl, baut das Handy die Verbindung neu auf.
- Rahmen in beide Richtungen: `ctr (uint32, Big Endian) ‖ AES-256-GCM(Ks, nonce, klartext) ‖ tag (16 Byte)`,
  `nonce = richtung (0x01 Handy→Gerät, 0x02 Gerät→Handy) ‖ sieben Nullbytes ‖ ctr`. Kein AAD.
  `ctr` beginnt je Richtung bei 1 und muss streng steigen. Ein falscher Rahmen wird verworfen; nach drei
  Fehlern trennt das Gerät.
- Klartext Handy→Gerät: eine Befehlszeile (UTF-8, höchstens 200 Byte).
- Klartext Gerät→Handy: `req (uint32 BE, ctr des Befehls) ‖ seq (uint8) ‖ last (uint8) ‖ daten`; die Daten aller
  Teile ergeben das JSON. Teilgröße = MTU − 3 − 26. Jeder Teil ist eine bestätigte Indication; so ist auch die
  Antwort auf `SLEEP` zugestellt, bevor das Gerät schläft.
- Handy: 5 s Zeitgrenze je Befehl, unvollständige Antworten verwerfen, Wiederholung als neuer Befehl.

Folgen: Befehle lassen sich weder fälschen noch wiederholen, Antworten ebenso wenig, und WLAN-Passwort,
Web-Login und Dateinamen sind auf dem Funkweg nicht lesbar. Vom PC aus lässt sich das mangels Bluetooth nicht
angreifend prüfen; geprüft wird mit festen Vektoren (Python, Kotlin, Selbsttest der Firmware) und mit Testknöpfen
in der App (falscher Schlüssel, wiederholter Rahmen).

**Kopplung:** `tools/pair.py` liest `BLE_KEY` über USB und gibt Adresse und Schlüssel über die Standardeingabe von
`adb shell run-as de.tobidervogel.liveup sh -c 'cat > files/recorder_pair.json'` an die App; nichts davon erscheint
in der Konsole, in der Prozessliste oder in einer Zwischendatei. Am sichersten per Kabel (`adb -d`); über WLAN-ADB
läuft der Schlüssel auf Android 9 unverschlüsselt durchs Heimnetz, das geht nur mit dem Schalter `--wlan`.

### 3.8 WLAN und Web-API (Phase 3a)

Digest-Login wie bisher (`tobi` + Zufallspasswort im NVS). Hostname `recorder-<gerät>` (vor dem Einschalten des
WLAN setzen), zusätzlich mDNS. Zwei WLAN-Plätze (`credentials` unverändert, dazu `credentials2`), damit Heimnetz
und Handy-Hotspot nebeneinander gespeichert bleiben; verbunden wird mit dem, das da ist.

| Aufruf | Zweck |
|---|---|
| `GET /status.json` | wie `STATUS`, aus dem RAM, auch während der Aufnahme |
| `GET /api/list` | abgeschlossene Segmente ohne Hub-Marker; `?all=1` alles, `?from=<nummer>` setzt fort; stückweise gesendet |
| `GET /api/file?p=/rec/000002/0000.wav` mit `Range: bytes=N-` | Download ab Byte N (`206`, `Content-Range`), `416` hinter dem Ende |
| `POST /api/confirm?p=…&sha256=…&by=pc\|phone` | legt `0000.ok` bzw. `0000.okp` an |

- Während der Aufnahme antworten `list`, `file` und `confirm` mit `409`.
- Ein Startauftrag (USB, BLE, Timer) bricht einen laufenden Download nach dem aktuellen Block ab; der Hub setzt
  später mit `Range` fort.
- `confirm`: nur `POST`, Pfad genau `/rec/\d{6}/\d{4}\.wav`, Marker nur, wenn das Segment abgeschlossen ist und die
  Prüfsumme der des Journals gleicht (sonst `409`). Doppelt bestätigen ist unschädlich. Der Marker enthält die
  Prüfsumme und ist gesichert, bevor `200` kommt.
- **Löschen:** nie ohne Marker. Wird Platz gebraucht, zuerst die ältesten Segmente mit `.ok` (auf dem NAS geprüft),
  danach die mit `.okp` (nur auf dem Handy). Jede Löschung steht in `/device.log`. `CLEAN` löscht auf Befehl alles
  mit `.ok`.
- Die Verbindung ist in V1 unverschlüsseltes HTTP im eigenen WLAN; TLS ist ein späterer Schritt.

## 4. PC-Recorder-Hub (Phase 3b)

`recorder.py` wird erweitert, nicht neu gebaut.

- **Abholer:** alle 30 s die letzte Adresse fragen (`/status.json`, 2 s Zeitgrenze; sonst `recorder-<gerät>.local`;
  das Feld `device` muss passen) → `/api/list` → je Segment nach `inbox/<name>.dl`, Fortsetzen nur bei `206` mit
  passendem Anfang → Größe und SHA-256 prüfen → NAS → zurücklesen → `/api/confirm` → Transkription → Brain-Verweis.
  Dazu je Aufnahme `info.json` und `log.txt` ins Archiv.
- **Name:** `JJJJMMTT-HHMMSS_<rec_id>_<NNNN>.wav` (Ortszeit des Segmentbeginns) unter
  `//jacobnas/JacobNAS/Tobi/Recorder/audio/<JJJJ-MM-TT>/`. Der Name beginnt wie bisher mit der Zeit, deshalb bleiben
  Upload, Verarbeitung und Selbsttest unverändert nutzbar.
- **Zustand je Segment** in SQLite (`hub.db`, Standardbibliothek), Schlüssel `(rec_id, n)`: `sha256`, `bytes`, `name`,
  `nas_at`, `confirmed_at`, `transcribed_at`, `linked_at`, `error`. Abgleich bei jedem Kontakt: kein `nas_at` →
  holen; `nas_at` ohne Marker → erneut bestätigen. Datum und Name werden einmal festgeschrieben, Doppelte erkennt
  der Hub über den Schlüssel, nicht über den Pfad. Die Anzeige für das Handy leitet daraus Tobis Stufen ab:
  aufgenommen, abgeschlossen, auf Handy kopiert, auf PC kopiert, auf NAS kopiert, transkribiert, im Brain verknüpft.
- Weicht die Prüfsumme ab: einmal ganz neu laden; weicht sie wieder ab, mit der tatsächlichen Prüfsumme archivieren,
  `error=sha_mismatch`, nie bestätigen, anzeigen.
- **Ausschalten:** Nach `SHUTDOWN` wartet das Gerät höchstens 2 min auf die erste angemeldete Anfrage, bleibt wach,
  solange Anfragen kommen (60 s Ruhe beendet), und schläft, sobald kein fertiges Segment mehr ohne Marker ist;
  harte Grenze 30 min (einstellbar).
- `PUT /upload` bleibt der Eingang für das Handy. `GET /api/status` und eine kleine mobile Seite, mit Token.
- Transkription ohne Übernahme des vorherigen Textes (`condition_on_previous_text=False`), wegen der
  Wiederholungsschleife aus der Feldauswertung.
- **Brain:** Das Transkript liegt als Textdatei neben dem Segment auf dem NAS. Im Vault steht je Aufnahme nur ein
  Block in `Transkripte/JJJJ-MM-TT.md`: Datum, Aufnahme-ID, Link zum Transkript, NAS-Pfad, Unsicherheiten
  (Zeit unsicher, Verluste, wiederhergestellt). Themen und bestätigte Fakten bleiben in V1 leer und werden später
  ergänzt. Immer über `brainctl.py hash` und `put`. Keine WAV-Dateien im Vault.

## 5. Handy: Modul „Recorder“ in Live Up (Phase 2 und 4)

Eigene Kachel in der App-Auswahl, Paket `de.tobidervogel.liveup.recorder`, eingebaut wie die anderen Module
(`App.Recorder` in `ui/Design.kt`, Symbol und Kachelliste in `hub/Auswahl.kt`, Zweig in `MainActivity.kt`).

- **Status**: Zustand, laufende Dauer, Segment, Verluste, Pegel, Karte, WLAN, Uhrzeit, Timer, Fehler.
- **Steuern**: Start, Stopp, Pause, Fortsetzen, Schlafen, Ausschalten, Timer (in 10/30/60 min oder zu einer Uhrzeit).
- **WLAN**: Name und Passwort eingeben und verschlüsselt ans Gerät geben; Netze suchen; WLAN aus.
- **Dateien**: Aufnahmen und Segmente mit Größe und Zustand, Speicherplatz.
- **Kopplung**: einmalig; die App übernimmt Adresse und Schlüssel aus `files/recorder_pair.json` in ihren privaten
  Speicher, löscht die Datei sofort und protokolliert nichts davon.
- Uhr des Recorders bei jeder Verbindung stellen (`TIME`).
- **Verbindungsablauf**, jede Operation erst im Callback der vorigen: `connectGatt(ctx, false, cb, TRANSPORT_LE)` →
  `requestMtu(247)` → `discoverServices` → CCCD von `TX` auf Indication → `HELLO` lesen → `HELLO` schreiben →
  Befehle. Bei Status ≠ 0 oder Trennung: `gatt.close()`, 1 s warten, höchstens 3 Versuche. Nach jedem Schließen
  4 s Ruhe vor dem nächsten Verbinden: Android trennt eine weiterbenutzte Funkstrecke sonst nach ein bis drei
  Sekunden doch noch (Status 22).
- **Android 9:** nur die alten GATT-Aufrufe und -Callbacks (`onCharacteristicChanged(gatt, ch)`,
  `ch.value = …; gatt.writeCharacteristic(ch)`); die neuen Varianten gibt es erst ab Android 13 und würden hier
  stumm nie aufgerufen.
- Rechte: `BLUETOOTH` (bis Android 11), `BLUETOOTH_CONNECT` (ab Android 12, zur Laufzeit). Kein Standort.
- Vor jeder Installation `testDebugUnitTest` (Protokoll gegen dieselben festen Vektoren wie die Firmware) und `lintDebug`.
- Phase 4: Den Hotspot schaltet Tobi in den Android-Einstellungen ein; die App gibt nur Name und Passwort ans Gerät.
  Die IP des Recorders kommt aus dem BLE-`STATUS`. Segmente per HTTP holen (dafür `usesCleartextTraffic`, Digest-Login
  von Hand mit eigenem Test), Prüfsumme prüfen, mit `by=phone` bestätigen, Kopie behalten, bis der Hub sie mit
  `200` und Prüfsumme angenommen hat. Hub-Adresse und Upload-Token stehen in der Kopplungsdatei. V1: Handy zum Hub
  nur im Heim-WLAN; mobile Daten erst mit einem Tunnel (WireGuard).

## 6. Strom (Phase 5, nur Planung)

- Akkumessung über Spannungsteiler an einem ADC-Pin (D0–D5).
- USB-Erkennung nur über ein Messsignal: Spannungsteiler von VBUS (z. B. 100 kΩ oben, 150 kΩ unten → 3,0 V bei
  5 V) oder das Power-Good-Signal eines geprüften Ladebausteins. **Nie 5 V direkt an einen GPIO.**
- Der eingebaute Lader des XIAO Sense schafft nur 50 mA, die Aufnahme braucht im Schnitt 64,5 mA (Seeed): am
  eingebauten Lader wird der Akku bei laufender Aufnahme nicht voll. Die einfachen TP4056/TC4056-Platinen sind für
  gleichzeitiges Laden und Aufnehmen nicht freigegeben.
- Vorschlag vom 07.10.2026 (noch nicht bestellt, Preise in Deutschland ungeprüft): Ladeplatine mit Power-Path
  Adafruit bq24074 (Artikel 4755), Ausgang an die Akku-Lötflächen des XIAO, `PGOOD` und `CHG` als Meldesignale.
  Teileliste und Verdrahtung stehen im Brain unter „Recorder Hardware Kette“.
- Taster an einem RTC-fähigen Pin, damit er aus dem Tiefschlaf wecken kann.
- Stromsparen (WLAN nur bei Bedarf, BLE-Intervall, CPU-Takt) erst messen, dann festlegen.

## 7. Risiken

| Risiko | Umgang |
|---|---|
| Nur Bluetooth LE, kein klassisches Bluetooth | eigener GATT-Dienst, kleine Rahmen, keine Audiodaten über BLE |
| Upload stört Audio | Recorder-Task mit Vorrang; Downloads in V1 nur ohne Aufnahme; Ringpufferstand wird protokolliert und gemessen |
| BLE und WLAN gleichzeitig kosten Speicher und Strom | nach dem Einbau Heap und Ringpuffer messen, Vergleich mit Phase 1 |
| Handy-Hotspot: erreicht das Handy den Recorder? | in Phase 4 am echten Gerät testen, vorher nichts zusagen |
| Tiefschlaf verhindert Fernsteuerung | klar anzeigen; Wecken nur per Timer, Taster, Strom |
| Akku und Power-Path offen | keine Kaufempfehlung vor der Prüfung |
| FAT ist nicht ausfallsicher | nur anhängen, `fsync`, Abschließen aus dem Journal; echtes Steckerziehen als Test, keine Garantie |
| Schwaches WLAN am Schreibtisch (−76 dBm) | Antenne und Platz prüfen; Durchsatz in Phase 3 messen |
| Bluetooth am Handy aus, Android 9 | App bittet ums Einschalten; direkte Verbindung per Adresse; alte GATT-Aufrufe |
| Unverschlüsseltes HTTP im WLAN, Digest ohne Bindung an die Adresse | nur im eigenen Netz; Bestätigen nur per `POST` mit eigener Prüfung; TLS später |
| Aufnahmen anderer Personen (§ 201 StGB) | Pause und Stopp überstehen jeden Neustart außer Strom neu; Hinweis steht im Brain |
| Firmware-Logik ohne Unit-Tests | kleine Testschalter, Gerätetests nach jedem Flashen, Gegenlesen vor dem Dauereinsatz |

## 8. Phasen

| Phase | Inhalt | Fertig, wenn |
|---|---|---|
| 0 | Bestandsaufnahme, WLAN mit Antenne real testen | erledigt am 06.10.2026 |
| 1 | AR1.0: nur Audio, Segmente, Journal, `info.json`, SHA-256, Abschließen, USB-Befehle, Prüfwerkzeug | T1–T8 bestanden |
| 2 | AR1.1: BLE-Dienst; Live-Up-Modul mit Status, Steuerung, Timer, WLAN, Dateien | P2-Liste bestanden |
| 3a | Web-API mit `Range`, Liste, Bestätigung, zwei WLAN-Plätze | P3a-Liste bestanden |
| 3b | Hub holt ab, archiviert, transkribiert, verknüpft; Status für das Handy | P3b-Liste bestanden |
| 4 | Handy als Brücke: Hotspot, Download, Prüfung, Weitergabe, Bestätigung | Segment kommt über das Handy geprüft beim Hub an; ohne Marker wird nichts gelöscht |
| 5 | Taster, Akkumessung, USB-Erkennung, Power-Path wählen, Verbrauch messen | Messwerte und Schaltung dokumentiert |
| 6 | Dauerläufe 6, 12, 24 Stunden; danach Upload während der Aufnahme | 24 h mit `dropped=0`, `|missing|` ≤ 1024, `gap_max_ms` < 500 |

Vor jedem Flashen: bauen (keine Warnungen) und `tools/test_ar1.py`. Nach jedem Flashen: `tools/devtest.py`.

### Phase 1: Tests

| Test | Ablauf | Erwartung |
|---|---|---|
| T1 | `SELFTEST`, `DF` direkt nach dem Start | Proben stimmen; Belegung gelesen |
| T2 | `SEGLEN 20`, 70 s aufnehmen, stoppen, über USB holen | 4 Segmente (20/20/20/10 s), SHA-256 am PC = `info.json`, Mikrofon-Samples = Datei-Samples, Uhr passt, `dropped=0` |
| T3 | Pause, 5 s warten, Fortsetzen, Stopp | Segment endet mit `pause`, das nächste beginnt entsprechend später, Antworten sofort |
| T4 | Hartes Zurücksetzen mitten im Segment; danach `REC STOP` und abschließen lassen | fertige Segmente unverändert, das offene wird abgeschlossen (`interrupted`), `state=recovered`, neue Zeile in `/device.log` |
| T4b | Pausiert, dann Zurücksetzen | Gerät bleibt aus (nimmt nicht wieder auf) |
| T5 | 30 Minuten mit echten 10-Minuten-Segmenten, WLAN verbunden, alle 10 s `STATUS` und `LS` | 3 volle Segmente, `dropped=0`, Ringpuffer unter 10 % |
| T6 | `TIMER stop 20`; `SLEEP 15` | `END reason=timer`; Port weg, nach 15 s wieder da, `reset=deepsleep`, Leerlauf |
| T7 | `FAULT sd`; `MINFREE` über dem freien Platz | `sd_error`, Karte neu eingehängt, neue Aufnahme, alte wird abgeschlossen; `card_full` ohne neuen Ordner |
| T8 | Aufnahme ohne erreichbares WLAN | wie T2 |
| von Hand | 5-mal Stecker ziehen bei `SEGLEN 20`, einmal kurz nach einem Segmentwechsel | alle früher fertigen Segmente bestehen die Prüfung |

### Prüflisten der nächsten Phasen

- **P2:** Vektoren stimmen in Python, Kotlin und `SELFTEST` · Handy verbindet nach App-Neustart erneut (Werben nach
  Trennung) · Start, Stopp, Pause, Timer, Schlafen vom Handy · falscher Schlüssel und wiederholter Rahmen werden
  abgewiesen (Testknopf) · WLAN-Daten kommen an, ohne im Journal oder auf USB zu erscheinen · Heap, Ringpuffer und
  Verluste wie in Phase 1 (30 Minuten mit verbundener App).
- **P3a:** Download bricht ab und setzt mit `Range` fort, SHA stimmt · falsche Prüfsumme bei `confirm` erzeugt
  keinen Marker · Segment ohne Marker überlebt `CLEAN` · `REC START` während eines Downloads startet sofort ·
  Durchsatz in MB/s bei gemessenem RSSI.
- **P3b** (gegen ein Schein-Gerät in `selftest.py`): Abbruch mitten im Download, Fortsetzung · NAS nicht erreichbar,
  keine Bestätigung · zweiter Lauf lädt nichts doppelt · verlorene Bestätigung wird nachgeholt · eine echte Aufnahme
  liegt geprüft auf dem NAS und ist im Brain verknüpft.

## 9. Stand der Umsetzung (07.10.2026)

| Phase | Stand |
|---|---|
| 0 | erledigt |
| 1 | Firmware fertig und am Gerät getestet: T1 bis T7 bestanden. T5 (30 Minuten, echte Segmente) bestanden, alle Segmente am PC gegen die Prüfsummen geprüft. Ungeplanter Dauerlauf über Nacht: 19,6 Stunden, 118 Segmente, kein verworfenes oder fehlendes Sample, Ringpuffer höchstens 5,9 %, längster Schreibvorgang 1,1 s; drei Segmente am PC geprüft, die übrigen nur nach den Angaben des Geräts. Offen: T8 (ohne erreichbares WLAN) und das Steckerziehen von Hand. |
| 2 | Firmware AR1.1 mit Bluetooth geflasht, Live Up 0.9.0 mit dem Modul „Recorder“ auf dem S9+ installiert. Am Gerät geprüft: Kopplung, Verbindung, Status, Start, Pause, Fortsetzen, Stopp, Timer, Netzsuche, Dateiliste; Verbindungstest, falscher Schlüssel und wiederholter Rahmen bestanden; 30 Minuten Aufnahme mit verbundener App ohne Verlust (Ringpuffer höchstens 3,1 %, freier Speicher rund 74 KB statt 130 KB ohne Bluetooth). Nicht am Gerät geprüft: Schlafen und Ausschalten vom Handy, WLAN an/aus, Autostart-Schalter. |
| 3a, 3b, 4, 5, 6 | nicht begonnen. Für Phase 5 gibt es einen Hardware-Plan (Brain: „Recorder Hardware Kette“). |

Was die Tests an Fehlern gezeigt haben und wie sie behoben sind:

- Antwort auf `REC STOP` kam nach einer langen Aufnahme erst nach über 10 s: Der Recorder-Task gibt bei langer Kartenarbeit jetzt regelmäßig ab.
- USB-Antworten gingen verloren, wenn zwei Tasks gleichzeitig zum PC schrieben: Nur noch die Hauptschleife schreibt.
- Android benutzt die Funkstrecke einer eben geschlossenen Verbindung weiter: frisches `Nd` für jede Aushandlung (Firmware), 4 s Ruhe vor dem Neuverbinden (App).
- Mit Bluetooth blieben nur rund 60 KB interner Speicher frei: Die großen Puffer liegen im PSRAM.
- Die Liste der Aufnahmen zählte jede Datei einzeln (1,85 s bei 23 Aufnahmen): Die `DONE`-Zeile trägt jetzt Segmentzahl und Größe, die Liste liest je Aufnahme nur diese Zeile. Die Liste beginnt mit der neuesten Aufnahme.
- Eine Antwort, deren Teil das Handy nicht bestätigt, wird abgebrochen und die Verbindung getrennt, statt die Hauptschleife sekundenlang aufzuhalten.

Die Gegenprüfung des Quelltexts durch Agenten ist am Nutzungslimit ausgefallen; die Spezifikation wurde davor von vier Prüfern gegengelesen, der Bluetooth-Teil der App von Claude selbst gegen die Firmware gelesen.

## 10. Plan für Phase 3 (PC), Stand 07.10.2026

Ziel: Der PC holt fertige Aufnahmen über WLAN vom Recorder, prüft sie, legt sie aufs NAS, transkribiert sie und
trägt einen Verweis ins Brain ein. Erst wenn eine Datei geprüft auf dem NAS liegt, gilt sie als gesichert.
Die Einzelheiten stehen in 3.8 und 4; hier die Reihenfolge der Arbeit.

| Schritt | Inhalt | Geprüft durch |
|---|---|---|
| 3a.1 | Firmware: `GET /api/list` (fertige Segmente ohne Marker, seitenweise) | Abruf vom PC, Vergleich mit `LS`/`SEGS` |
| 3a.2 | Firmware: `GET /api/file` mit `Range`, auch für `info.json`, `log.txt`, `/device.log`; Abbruch bei Startauftrag | Download mit Abbruch und Fortsetzung, SHA-256 stimmt; Durchsatz in MB/s bei gemessenem RSSI |
| 3a.3 | Firmware: `POST /api/confirm` legt den Marker an; `CLEAN`; Löschen nur mit Marker, wenn Platz fehlt | falsche Prüfsumme erzeugt keinen Marker; Segment ohne Marker überlebt `CLEAN` |
| 3a.4 | Firmware: Hostname und mDNS; `SHUTDOWN` wartet auf den Hub (höchstens 2 min auf die erste Anfrage, 60 s Ruhe beendet, Obergrenze 30 min) | `SHUTDOWN` mit und ohne laufenden Hub |
| 3b.1 | PC: `tools/hubpair.py` legt Adresse und Web-Login in `A:/Recorder/recorder.json` ab (über USB gelesen, nie angezeigt) | Datei vorhanden, nichts in der Konsole |
| 3b.2 | PC: Abholer in `recorder.py` (Gerät finden, Liste, Download nach `inbox/<name>.dl` mit Fortsetzung, Prüfsumme, NAS, Zurücklesen, Bestätigen), Zustand in `hub.db` | `selftest.py` gegen ein Schein-Gerät: Abbruch und Fortsetzung, NAS fehlt, nichts doppelt, verlorene Bestätigung wird nachgeholt |
| 3b.3 | PC: Transkription ohne Textübernahme; Transkript als Datei neben dem Segment auf dem NAS; im Vault nur ein Verweis je Aufnahme über `brainctl` | Selbsttest im Wegwerf-Vault; eine echte Aufnahme |
| 3b.4 | PC: `GET /api/status` und eine kleine Seite fürs Handy (Token) | Aufruf vom Handy im WLAN |
| 3b.5 | Autostart von `start.bat`; Firewall-Freigabe für Port 8765 nur im privaten Netz (macht Tobi) | Neustart des PC |

Grundsätze: `recorder.py` wird erweitert, nicht neu gebaut. Synchronisiert wird nur, wenn nicht aufgenommen wird.
Der Recorder löscht nichts ohne Marker. Zugangsdaten stehen nur in `recorder.json` und `token.txt`, nie im Brain.

Offene Entscheidungen von Tobi: ob der Hub immer läuft (Autostart), ob alles automatisch transkribiert wird oder
erst auf Wunsch, und wie lange gesicherte Aufnahmen auf der Karte bleiben sollen.
