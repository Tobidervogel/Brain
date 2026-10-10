---
titel: Recorder DV2 Lauftest
tags: [projekt, hardware, firmware, audio, video]
erstellt: 2026-10-05
aktualisiert: 2026-10-05
---

# Recorder DV2 Lauftest

Erste Firmware für den [[Audio-Recorder]] (Seeed XIAO ESP32S3 Sense), die wirklich aufnimmt: Video und Audio gleichzeitig auf die microSD. Von Claude am 05.10.2026 gebaut, geflasht und am Gerät getestet. Auftrag von Tobi: ein Testprogramm, mit dem er das Gerät an einer Powerbank herumtragen kann; nach 10 Minuten soll es von selbst aufhören, damit beim Abziehen nichts kaputtgeht; ins WLAN JacobHome einloggen; die Aufnahme will er danach ChatGPT („Chatty“) oder Claude zur Bewertung geben.

## Verhalten

- **Strom dran = Aufnahme.** Etwa 3 Sekunden nach dem Einstecken beginnt die Aufnahme von selbst, ohne Taster und ohne WLAN.
- **Nach 600 Sekunden** schließt die Firmware beide Dateien sauber ab, hängt die Karte aus und geht in den Tiefschlaf. Danach ist Abziehen gefahrlos. Neu einstecken startet die nächste Aufnahme.
- **Anzeige:** Die orange LED auf dem Board hängt an derselben Leitung wie die Karte (GPIO21) und flackert deshalb, solange geschrieben wird. LED dauerhaft aus = fertig. (Aus dem Schaltplan abgeleitet, von Claude nicht gesehen.)
- **Stromausfall mittendrin:** Alle 2 Sekunden wird der Stand auf die Karte gesichert. Die Dateiköpfe sind so angelegt, dass auch eine abgeschnittene Datei abspielbar bleibt. Getestet mit hartem Reset mitten in der Aufnahme: 80 von rund 90 Bildern und 7,6 s Ton lesbar, die nächste Aufnahme startete von selbst.
- **Fehlerfälle:** Kamera fällt aus → Ton läuft weiter. Karte hängt länger als 500 ms (dann gibt der SD-Treiber auf) → neu einhängen und in neuem Ordner die restliche Zeit aufnehmen, höchstens 5-mal. Schleife hängt 15 s → Watchdog startet neu und damit eine neue Aufnahme.

## Dateien auf der Karte

Pro Aufnahme ein Ordner `/rec/NNNN/` (fortlaufende Nummer):

| Datei | Inhalt |
|---|---|
| `video.avi` | MJPEG, nur Bild. Bildrate im Kopf ist der gemessene Durchschnitt. |
| `audio.wav` | 16 kHz, 16 bit, mono, getrennt vom Video. Gleichanteil des Mikrofons entfernt, Pegel ×4. |
| `frames.csv` | Zeitstempel jedes Bildes in ms ab Aufnahmestart, für exakte Zuordnung zum Ton. |
| `info.json` | Alle Zähler: Bilder, Lücken, fehlende Ton-Samples, Schreibzeiten, `audio_t0_ms` und `video_t0_ms` (Versatz zwischen Ton und Bild). |
| `log.txt` | Alle 10 s eine Zeile, auch der Grund des letzten Neustarts. Bleibt auch bei Stromausfall erhalten. |

Eine abgeschnittene Aufnahme hat kein `info.json` und kein `frames.csv`; Bildrate und Versatz stehen dann in der letzten Zeile von `log.txt`.

## Einstellung und Messwerte

Voreinstellung: **1024×768 (XGA), 10 Bilder/s, JPEG-Qualität 12**. XGA ist beim eingebauten Sensor (OV3660, erkannt als `0x3660`) das volle Sichtfeld ohne Umrechnung. Alle Stellschrauben stehen oben in `src/main.cpp`.

Gemessen am 05.10.2026 am Gerät, Ton lief immer mit, ruhige Szene im Zimmer:

| Auflösung | Bilder/s (ungebremst) | KB je Bild | KB/s |
|---|---|---|---|
| 640×480 | 27,4 | 16 | 439 |
| 800×600 | 27,7 | 22 | 615 |
| 1024×768 | 27,1 | 34 | 916 |
| 1280×720 | 17,0 | 38 | 642 |
| 1280×1024 | 14,4 | 52 | 746 |
| 1600×1200 | 13,7 | 70 | 948 |

- Die Karte schafft über SPI (25 MHz) rund **1600 KB/s** Dauerschreiben, längster Einzelblock im Test 72 ms.
- Ton war in jeder Messung lückenlos: Abweichung gegen die Uhr höchstens 512 Samples (das ist die Messgenauigkeit, ein Mikrofonblock), nichts verworfen, Zwischenpuffer nie über 2 % gefüllt.
- Draußen mit vielen Details werden die Bilder 2- bis 3-mal so groß. Die Voreinstellung braucht drinnen etwa 350 KB/s, hat also Reserve. Wird die Karte zu langsam, sinkt nur die Bildrate, der Ton bleibt.
- **10-Minuten-Lauf wie an der Powerbank** (Ordner 0018, am PC nachgeprüft): 5975 Bilder = 9,96 Bilder/s, 209 MB Video (340 KB/s), Ton 599,5 s lückenlos. 18 Bildabstände über 150 ms, davon 2 über 500 ms (längster 720 ms), weil die Karte einzelne Schreibvorgänge bis 673 ms verzögert hat. Der Ton blieb davon unberührt (Zwischenpuffer höchstens 5 % voll). Danach sauber abgeschlossen, Tiefschlaf bestätigt (USB-Port verschwand), nach dem Wiedereinschalten startete die nächste Aufnahme von selbst.
- 10 Minuten in dieser Einstellung sind also rund 210 MB Video plus 19 MB Ton; auf die Karte passen grob 40 Stunden.

Das ist noch nicht das Maximum: bis XGA liefert die Kamera 27 Bilder/s. Welche Kombination aus Auflösung und Bildrate Tobi will, entscheidet er nach der ersten Aufnahme draußen.

## SD-Karte

64-GB-microSD (59,5 GB nutzbar), steckte vorher in einem Android-Handy: exFAT, Name „android“, Ordner `Android` und `.android_secure`, etwa 1,2 GB belegt. Der Chip kann kein exFAT lesen. Mit Tobis ausdrücklichem Ja im Gerät als FAT32 formatiert (7,7 s); die alten Handy-Daten sind gelöscht. Damit ist auch die offene Frage aus dem Recorder-Projekt beantwortet: eine 64-GB-Karte funktioniert, wenn sie FAT32 ist.

## WLAN

- Zugangsdaten für **JacobHome** sind auf dem Gerät gespeichert (nicht im Quelltext, nicht im Brain). Die früheren Hotspot-Daten sind damit überschrieben; Tobi wollte jetzt ausdrücklich das Heim-WLAN.
- **Verbindung noch nicht bestätigt.** Ein Scan am Schreibtisch fand kein einziges Netz, und der Hotspot direkt daneben kam zuvor nur mit −70 dBm an. Vermutlich ist die kleine Antenne (liegt dem XIAO bei, wird an den winzigen runden Stecker in der Board-Ecke geklipst) nicht aufgesteckt. Ob der Name exakt „JacobHome“ geschrieben wird, konnte deshalb auch nicht geprüft werden.
- Mit Verbindung gibt es eine passwortgeschützte Seite auf dem Gerät: Status, „Aufnahme beenden“ und Download der Dateien (nur wenn gerade nicht aufgenommen wird). Das Login zeigt das Fenster `Recorder-WLAN.cmd` aus dem Codex-Ordner.
- Internet braucht das Gerät nicht. Die Aufnahme läuft ohne WLAN genauso; das wurde in allen Tests so gemessen.

## Projekt und Werkzeuge

Ordner `A:/Recorder/firmware/dv2` (PlatformIO, Arduino-ESP32 2.0.17, COM6). `wifi_setup.cpp` stammt aus Codex' DV1.4 und wurde erweitert. Der Codex-Ordner mit DV1.4 ist unverändert.

- Bauen und flashen: `python -m platformio run -t upload` im Projektordner. Vorher das Fenster `Recorder-WLAN` schließen.
- `tools/dv2.py` – USB-Befehle: `send STATUS`, `send STOP`, `send "REC <s> <größe> <qualität> <bilder/s>"`, `get`, `wifi`.
- `tools/fetch.py <nr>` – Aufnahme über USB holen und prüfen (ca. 90 KB/s, für kurze Tests).
- `tools/check.py <ordner>` – prüft Video, Ton und Zähler einer Aufnahme, speichert drei Beispielbilder.
- `tools/remote_check.py <nr>` – prüft eine lange Aufnahme direkt auf der Karte, holt nur Ton und drei Bilder.
- `tools/matrix.py` – misst Auflösungen durch. `tools/fieldtest.py <s>` – kompletter Ablauf wie an der Powerbank.
- Testaufnahmen vom 05.10. liegen unter `captures/`. Auf der Karte sind die Ordner 0001 bis 0020 Claudes Tests am Schreibtisch. Sie enthalten Bild und Ton aus Tobis Zimmer während des Testens, zusammen gut 20 Minuten; auf den Bildern ist außer Tobi eine weitere Person zu sehen. Tobi wurde darauf hingewiesen; löschen kann er sie jederzeit.

Große Aufnahmen holt man am schnellsten, indem man die Karte in den Kartenleser am PC (`E:`) steckt.

## Was unterwegs gelernt wurde

- Die USB-Schnittstelle des ESP32-S3 verliert bei großen Datenmengen einzelne Bytes. Dateien deshalb nur blockweise mit Prüfsumme übertragen (macht `dv2.py get`).
- Ist ein gespeichertes WLAN nicht erreichbar, hat die DV1.4-Logik jede neue WLAN-Einrichtung mit „busy“ abgelehnt. Behoben; zusätzlich gibt es `WIFI_STORE` (speichern ohne Verbindungstest).
- Das Mikrofon liefert einen festen Gleichanteil von etwa 1400. Die Firmware zieht ihn ab.
- Vier unabhängige Prüf-Agenten haben den Code vor dem Lauftest gegengelesen; ihre echten Funde (Abbruch bei kurzem Karten-Hänger, endloses Warten beim Stoppen, blockierte WLAN-Einrichtung) sind eingebaut.

## Offen

- Antenne aufstecken, dann WLAN und Download-Seite prüfen.
- Erste Aufnahme draußen auswerten: Datenrate, Lücken, Bildschärfe, Bildausrichtung (hängt davon ab, wie das Board getragen wird), Tonpegel.
- Danach Auflösung und Bildrate festlegen; Nur-Audio-Sparmodus, Marker, Upload zu PC/NAS und Stromverbrauch sind noch nicht gebaut.
- Kein ffmpeg am PC. Zum Zusammenführen von Bild und Ton in eine MP4 fehlt noch ein Werkzeug.

Verwandt: [[Audio-Recorder]], [[Hardware und Laufwerke]], [[JacobNAS]]


## Zwei eigene Testaufnahmen: Auswertungsauftrag (05.10.2026, Codex)

Tobi hat nach Claudes DV2-Arbeit selbst eine vollständige und eine unvollständige Testaufnahme gemacht. Er beauftragt Codex ausdrücklich, Video und Audio auszulesen, vollständig zu transkribieren und ein möglichst ausführliches Log aller erkennbaren Inhalte und Erkenntnisse zu erstellen. An ihn gerichtete, erkennbare eigene Anweisungen in den Aufnahmen sollen berücksichtigt werden. **Die Recorder-Software soll dabei unverändert bleiben.**

Über USB nur lesend festgestellt: Auf der Karte liegen die ursprünglichen Aufnahmen `/rec/0021/` und `/rec/0022/`. 0021 enthält `video.avi` (182.766.952 Byte), `audio.wav` (19.185.152 Byte), `log.txt`, `frames.csv` und `info.json`; 0022 enthält `video.avi` (69.961.728 Byte), `audio.wav` (7.125.504 Byte) und `log.txt`, aber keine Abschlussmetadaten. Zusammen rund 279 MB. Die Inhalte sind zu diesem Zeitpunkt noch nicht kopiert oder analysiert; aus Dateigrößen allein keine Lückenfreiheit oder Bildqualität behaupten.

Beim Anschließen per USB startete die unveränderte DV2 automatisch eine zusätzliche Aufnahme 0023. Codex kündigte den nötigen Stopp an und beendete diese mit dem vorhandenen `STOP` sauber (206,5 Sekunden Audio, `sd_error=0`), damit die vorhandenen Dateien ausgelesen werden konnten. Kein Flash, keine Firmware- oder WLAN-Konfigurationsänderung, keine Löschung. Ein beliebiger serieller Befehl versetzt DV2 bis zum Neustart in den Service-Modus, sodass sie zum Auslesen wach bleibt.

Wegen der langsamen seriellen Übertragung wählte Tobi anschließend ausdrücklich die microSD im PC-Kartenleser. Tobi bestätigt, dass die Karte eingelegt ist; die lesende Windows-Prüfung meldet jedoch USB-Kartenleser `E:` mit **No Media / 0 Byte**, keinen weiteren Kartendatenträger. Um erneutes Einstecken oder Rückkehr zu Recorder-USB gebeten. Stand dieser Ergänzung: Auswertung wartet auf lesbaren Medienzugriff. Keine Formatierung oder Reparatur an der Karte vorgenommen.

Vorbereitung im Codex-Arbeitsordner `C:/Users/a/Documents/Codex/2026-10-05/okay-ich-wei-nicht-ob-du/work/`: Kopieren mit SHA-256-Abgleich, lokale Whisper-Transkription mit dem bereits vorhandenen large-v3-turbo-Modell, vollständige JPEG-Dekodierung, Bildübersichten und Tonmessung. MP4-Prüfkopien können mit vorhandenem PyAV erzeugt werden; ffmpeg als eigenes Programm wurde nicht installiert. Bei der abgebrochenen Aufnahme fehlen genaue Einzelbild-Zeitstempel; eine spätere gemeinsame Wiedergabe hat dort nur eine geschätzte Bildzeitachse. Die Originale sollen unverändert erhalten bleiben.

## Auswertung der eigenen Feldaufnahmen abgeschlossen (05.10.2026, Codex)

Der vorherige Wartestand ist überholt: Tobi übertrug die Karte über sein Handy und nannte `C:/Users/a/Documents/rec` als Quelle. Alle 13 vorhandenen Dateien aus 0021/0022/0023 bytegleich in den Codex-Ausgabeordner kopiert und SHA-256 verglichen. Keine Änderung an Recorder-Software, Originalen oder WLAN, kein Flash und keine Löschung.

- **0021:** sauber bei 600 s beendet; 5.961 vollständig decodierbare Bilder, 599,520 s PCM. 23 Bildabstände >150 ms, davon zwei >500 ms; größter 936 ms. Kein von der Firmware gezählter Audioverlust; `audio_missing=512` ist nur eine blockweise Zeit-/Sampledifferenz und kein eindeutiger Lückenbeweis.
- **0022:** 2.221 vollständige JPEGs und 222,656 s PCM; letzter begonnener JPEG-Block um mindestens 148 Byte abgeschnitten, finale Metadaten/CSV fehlen. Im Ton kündigt Tobi Abstecken an. Exakte Schlusszeit/Framezeiten und gegebenenfalls ungesicherte Daten bleiben unbekannt.
- **0023:** zusätzlicher USB-Lauf, sauber STOP bei 207 s; 2.062 Bilder, 206,496 s PCM. Im Bild Zimmer/Headset, ASR keine verständlichen Wörter.
- In allen vorhandenen Fortschrittslogs WLAN nicht verbunden. Keine wirkliche Uhrzeit in den Metadaten.
- Bildsichtung sämtlicher 23 chronologischen Übersichten im ~2-s-Abstand plus Einzelbilder; alle vollständigen JPEGs technisch decodiert. Selbstansicht von unten dominiert, Hände/Ablagen oft nicht im Bild, kleine Etikett-/Bontexte unscharf. Kein lückenloses Objektgedächtnis.
- Lokale large-v3-turbo-Transkription in zwei Läufen, Unsicherheiten markiert; keine direkte manuelle Hörprüfung möglich. Ein kontextbehafteter Erstlauf erzeugte ab ca. 212 s eine falsche „So“-Schleife. Zweiter Lauf ohne Textfortsetzung liefert dort echten weiteren Inhalt. Bestehende Recorder-Pipeline dafür nicht verändert.
- **MP4-Zusammenführung ist jetzt erfolgt:** vorhandenes PyAV, kein ffmpeg-Programm installiert. Echte CSV-Zeitpunkte für 0021/0023, Näherung 9,96 fps für 0022; Audioanfang rund 0,46 s nach erstem Bild berücksichtigt. Originale unverändert, nur unvollständigen Schlussframe in 0022-Prüfkopie ausgelassen.

Inhalt/Ergebnisse vollständig unter [[Recorder Feldaufnahmen Auswertung]], Transkript unter [[Recorder Testaufnahmen 0021-0022]], Geschmacksnotizen unter [[Müllermilch Geschmackseindrücke]]. Wichtig: mehrfach erkannte **3,40 € Pfand**, **1,49 € Müllermilch + 0,25 € neuer Pfand = 1,74 € Ausgabe**, **1,66 € Differenz**; keine zusätzliche Restgeld-Einnahme. Zahlungsweg unklar. Schoko-Cookie-Flasche sichtbar/genannt, tatsächliche Trinkmenge unklar; volle Standardflasche als bedingte Nährwertvorlage statt behauptetem Verzehr.

Ausgaben: `C:/Users/a/Documents/Codex/2026-10-05/okay-ich-wei-nicht-ob-du/outputs/`. Dort ausführlicher Bericht, vollständiger automatischer Transkript-Arbeitsstand mit Lücken/Unsicherheiten, Bild-/Techniklog, Roh-ASR, 23 Bildübersichten, Belegbilder, Messdaten, drei MP4s und JSON-Befunde/beschriftete Vorlagen. Kein Import in Live Up und kein automatisches NAS-Medienarchiv durchgeführt; Brain-Sync sichert Notizen, nicht diese großen Videodateien.
