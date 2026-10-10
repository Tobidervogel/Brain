---
titel: Recorder AR1 Firmware
tags: [projekt, hardware, firmware, audio, bluetooth]
erstellt: 2026-10-06
aktualisiert: 2026-10-09
---

# Recorder AR1 Firmware

Nur-Audio-Firmware für den [[Audio-Recorder]] (Seeed XIAO ESP32S3 Sense), von Claude am 06.10.2026 gebaut. Sie löst die Lauftest-Firmware aus [[Recorder DV2 Lauftest]] ab; die Kamera wird nicht mehr angesprochen. Die vollständige Spezifikation mit Phasenplan steht in `A:/Recorder/docs/Audio-Recorder-V1.md`.

## Wo was liegt

- Firmware: `A:/Recorder/firmware/ar1` (PlatformIO, Arduino-ESP32 2.0.17, COM6). `dv2` bleibt unverändert als Rückfallstand.
- Werkzeuge in `tools/`: `ar1.py` (USB-Befehle, Download), `check_ar1.py` (prüft eine Aufnahme am PC), `test_ar1.py` (vor jedem Flashen), `devtest.py` (Gerätetests nach jedem Flashen), `ar1proto.py` (Referenz des Bluetooth-Protokolls), `pair.py` (Handy koppeln).
- Bauen: `python -m platformio run`, flashen mit `-t upload`. Nie mit Flash-Löschen aufspielen, sonst sind WLAN-Daten, Web-Login und Geräteschlüssel weg.

## Verhalten

- 16 kHz, 16 Bit, Mono. Segmente von genau 10 Minuten in `/rec/NNNNNN/0000.wav …`, Wechsel auf das Sample genau.
- `log.txt` ist das Journal (nur anhängen, jede Zeile sofort gesichert), `info.json` wird daraus erzeugt. Jedes Segment hat eine SHA-256-Prüfsumme.
- Aufnahme-ID z. B. `d0a130-000003-a0e4124b` (Gerät, Nummer, Zufall). Geräte-ID ist `d0a130`.
- Stopp und Pause antworten in Zehntelsekunden. Die Prüfsumme des letzten, kurzen Segments wird im Leerlauf nachgetragen; erst dann steht `DONE` im Journal und `state` ist `closed`.
- Nach einem Abbruch (Reset, Stromausfall, Kartenfehler) wird die Aufnahme im Leerlauf abgeschlossen: Kopf aus der Dateigröße, Prüfsumme, `state: recovered`.
- **Aufnehmen nach einem Neustart:** Bei Strom neu entscheidet `AUTOSTART` (Standard an). Nach jedem anderen Neustart gilt, was zuletzt per Befehl gewünscht war. Wer gestoppt oder pausiert hat, wird nicht ohne sein Wissen wieder aufgenommen.
- Uhrzeit vom Router (NTP) oder per Befehl `TIME`; die Uhr läuft über Neustarts weiter (Quelle dann `rtc`).
- Befehle über USB (und ab AR1.1 über Bluetooth): `STATUS`, `REC START|STOP|PAUSE|RESUME`, `SLEEP [s]`, `SHUTDOWN`, `TIMER …`, `TIME`, `AUTOSTART`, `LS`, `SEGS`, `DF`, `ERRORS`, `REBOOT`, `WIFI SET|ON|OFF|FORGET|SCAN`, `HTTP_AUTH`. Nur USB: `BLE_KEY`, `GET`, `DIR`, `RM`, `SELFTEST` und Testschalter.
- Zwei WLAN-Plätze: Heimnetz und Handy-Hotspot bleiben nebeneinander gespeichert.
- Zugangsdaten kommen über USB nur in Zeilen mit `AR_SECRET` oder `DV1_WIFI_AUTH`; die Werkzeuge geben solche Zeilen nie aus.

## Gerätetests am 06.10.2026 (Stand AR1.0, ohne Bluetooth)

| Test | Ergebnis |
|---|---|
| Selbsttest, Kartenbelegung | bestanden |
| 70 s mit 20-Sekunden-Segmenten | 4 Segmente, Prüfsummen am PC gleich `info.json`, Mikrofon-Samples gleich Datei-Samples, kein Verlust, Ringpuffer höchstens 0,4 %, Stopp nach 0,5 s |
| Pause und Fortsetzen | Pause nach 0,1 s, zweites Segment 5,1 s später, nichts verloren |
| Hartes Zurücksetzen mitten im Segment | fertiges Segment unverändert, offenes Segment mit 10,3 s wiederhergestellt, neue Aufnahme lief von selbst an |
| Pausiert, dann Zurücksetzen | Gerät bleibt aus |
| Timer und Tiefschlaf | `END reason=timer`; nach `SLEEP 12` Port weg und nach rund 16 s wieder da, Leerlauf |
| Simulierter Kartenfehler, volle Karte | Karte neu eingehängt, neue Aufnahme, alte abgeschlossen; `card_full` ohne neuen Ordner |

Ein hartes Zurücksetzen ist kein echter Stromausfall (die Karte bleibt versorgt). Der Test mit Steckerziehen von Hand steht noch aus, ebenso die Dauerläufe.

## Dauerlauf über Nacht (06./07.10.2026)

Ungeplant, aber aufschlussreich: Der 30-Minuten-Test brach am PC ab, die Aufnahme lief auf dem Gerät weiter. Am 07.10. gestoppt nach **19,6 Stunden am Stück** (Aufnahme 11, 118 Segmente, 2,2 GB, Firmware AR1.0 ohne Bluetooth, an USB, WLAN verbunden bei −78 dBm):

- kein verworfenes und kein fehlendes Sample, längster Abstand zwischen zwei Mikrofonblöcken 64 ms
- Ringpuffer höchstens 5,9 % gefüllt, obwohl die Karte einmal 1,1 s für einen Schreibvorgang brauchte
- freier Speicher stabil (rund 130 KB, Minimum 121 KB)
- Stichprobe am PC geprüft (07.10.2026): Die Segmente 0, 60 und 117 stimmen mit den Prüfsummen des Geräts überein. Mikrofon-Samples und Datei-Samples sind gleich (1 128 868 864), Samples und Uhr weichen um 0 ms ab. Die übrigen 115 Segmente sind nur nach den Angaben des Geräts geprüft. (Ein erster Download war abgebrochen, weil der Recorder abgesteckt wurde.)

Zwei Schwächen fielen dabei auf; beide sind behoben und seit dem 07.10.2026 auf dem Gerät:

- Nach dem Stopp einer langen Aufnahme kam die Antwort auf `REC STOP` erst nach über 10 Sekunden. Der Recorder-Task rechnete Prüfsummen, und weil der Kartentreiber pollt statt zu warten, kam die Hauptschleife (USB, später Bluetooth) nicht zum Zug. Jetzt gibt der Recorder-Task bei langer Kartenarbeit regelmäßig ab.
- Einzelne USB-Antworten gingen verloren oder kamen abgeschnitten an, weil Recorder-Task und Hauptschleife gleichzeitig zum PC schrieben. Jetzt schreibt nur noch die Hauptschleife; Journalzeilen warten in einem Puffer und werden verworfen, wenn kein PC liest. Das PC-Werkzeug wertet außerdem nur noch vollständige Zeilen aus und fragt bei einer beschädigten Antwort noch einmal.

## AR1.1 mit Bluetooth (seit 07.10.2026 auf dem Gerät)

- Neue Dateien `src/ble.cpp` und `ble.h`: verschlüsselte Fernbedienung nach Abschnitt 3.7 der Spezifikation. `SELFTEST` prüft die Verschlüsselung gegen `A:/Recorder/docs/ar1-proto-vectors.json`.
- Neue Befehle: `SEGS`, `WIFI SET|ON|OFF|FORGET|SCAN`, `HTTP_AUTH`, über USB `BLE_KEY` und `BLE_KEY NEW`.
- Zwei WLAN-Plätze in `wifi_setup.cpp`.
- `REBOOT` antwortet jetzt erst und startet dann neu.
- Größe 1,45 MB (vorher 0,87 MB), baut ohne Warnungen.
- Koppeln: `python tools/pair.py` bei angestecktem Recorder; Handy per Kabel oder mit `--wlan` über WLAN-ADB.
- Die Gegenprüfung durch Agenten ist am Nutzungslimit gescheitert und wird nicht wiederholt (Tobi will wenig Agenten). Den Bluetooth-Teil der App hat Claude selbst gegen die Firmware gelesen.

### Tests mit AR1.1 am 07.10.2026

- Alle kurzen Gerätetests (Selbsttest, Segmente, Pause, Zurücksetzen, Timer, Tiefschlaf, Kartenfehler, volle Karte) bestehen auch mit Bluetooth.
- Handy gekoppelt und über die App gesteuert, siehe [[Live Up]]. Testknöpfe der App bestanden: Verbindungstest, falscher Schlüssel, wiederholter Rahmen.
- 30 Minuten Aufnahme mit verbundener App, WLAN und Abfragen über USB: drei volle Segmente, alle Prüfsummen am PC bestätigt, kein Verlust, Ringpuffer höchstens 3,1 %.
- Zweiter WLAN-Platz mit einem erfundenen Netz getestet: gespeichert, Anmeldung scheitert, das Heimnetz kommt von selbst zurück, Netz wieder gelöscht; das Testpasswort erschien nirgends auf der seriellen Leitung.
- Freier interner Speicher mit Bluetooth rund 74 KB (ohne Bluetooth 130 KB). Die großen Puffer liegen deshalb im PSRAM.
- Die Antwort auf `LS` brauchte während der Aufnahme 1,85 s, weil jede Datei gezählt wurde. Jetzt steht Segmentzahl und Größe in der `DONE`-Zeile; die Liste ist nach Nummer sortiert, neueste zuerst.
- Gekoppelt wurde über WLAN-ADB, weil das Handy nicht am Kabel hing: Der Geräteschlüssel ging dabei einmal unverschlüsselt durchs Heimnetz. `pair.py --new` per Kabel ersetzt ihn.
- Karte aufgeräumt (Tobi hatte das Löschen erlaubt): die alten DV2-Ordner 0021 bis 0024 und die kurzen Testaufnahmen sind gelöscht. Behalten: Aufnahme 11 (19,6 Stunden) und 22 (30 Minuten) als Material für den PC-Abgleich, dazu die letzten kurzen Testläufe. **Später am 07.10.2026 hat Tobi verlangt, alles zu löschen („mach da mal alles weg“): Seitdem ist `/rec` leer, auch Aufnahme 11 und 22 sind weg; 60,9 GB frei.** Auf der Karte liegen nur noch `/device.log` und ein leerer Ordner `.android_secure` vom Handy. Am PC liegen weiter Kopien einzelner Testaufnahmen unter `A:/Recorder/firmware/ar1/captures/` (darunter drei Segmente von Aufnahme 11 und Aufnahme 22 vollständig).
- Nicht getestet: Aufnahme ohne erreichbares WLAN, echtes Steckerziehen, Schlafen und Ausschalten vom Handy.

## AR1.2 mit Strommessung (09.10.2026, seit dem Abend auf dem Gerät)

Auslöser: Die Ladeplatine ist da ([[Recorder Hardware Kette]]). Tobi fragte, wie man die Adafruit-Platine programmiert: gar nicht, sie hat keine Software. Programmiert wird der XIAO.

- Neu in `main.cpp`: Block „power board“. D0 (GPIO1) misst die halbe Akkuspannung (16 Messwerte gemittelt, alle 2 s), D3 (GPIO4) = `PGOOD`, D4 (GPIO5) = `CHG`, beide mit internem Pull-up.
- `STATUS` hat ein neues Feld `power`: `state` (`charging`, `external`, `battery`, `unknown`), `bat_mv` und `bat_pct` (grobe Schätzung aus der Ruhespannung, beim Laden zu hoch; `null` ohne Messschaltung).
- Die `SEG`-Zeilen im Journal und damit `info.json` tragen jetzt den echten Zustand statt `power=unknown`.
- Stellschraube `BAT_SCALE` (Standard 2,0): Spannung am Akku laut Multimeter geteilt durch die Spannung an D0.
- `SELFTEST` prüft zusätzlich die Akkukurve (`bat_curve`).
- Baut ohne Warnungen (1,46 MB), `test_ar1.py` besteht. (Nachmittags stand hier: nicht geflasht, weil der Recorder nicht am PC hing.)
- **Am 09.10.2026 gegen 18:10 Uhr aufgespielt**, nachdem Tobi die Stiftleisten angelötet und den Recorder aufs Steckbrett gesetzt hatte. `SELFTEST` besteht einschließlich `bat_curve`. `devtest.py` (t1, t2, t3, t4, t4b, t6, t7) endet mit „ERGEBNIS: OK“: Segmente, Pause, hartes Zurücksetzen, Timer, Tiefschlaf, Kartenfehler und volle Karte laufen wie mit AR1.1. Die Testaufnahmen 38 bis 45 (zusammen wenige MB, Ton aus Tobis Zimmer) liegen auf der Karte und unter `captures/`.
- **Die Strommessung selbst ist noch nicht bestätigt.** Beim Aufspielen und Testen hatte Tobi alle Jumper vom XIAO abgezogen; `power` zeigte deshalb `unknown` mit `bat_mv` um 0 bis 38, und die `SEG`-Zeilen tragen `power=unknown`. Die Messung mit angesteckten Leitungen und Strom am Lader steht aus.
- `AUTOSTART` steht auf dem Gerät auf aus (so vorgefunden, nicht geändert).
- **Neuer Befehl `PINS` (nur USB, am 09.10.2026 abends nachgeflasht, Version bleibt AR1.2):** zeigt die Pegel von D1 bis D5 mit Pull‑up, den kleinsten, mittleren und größten Wert an D0 über 200 ms und den Anteil der Zeit, in der `PGOOD` und `CHG` seit dem letzten Aufruf auf Minus lagen. Gedacht zum Suchen von Verdrahtungsfehlern auf dem Steckbrett; beim ersten Aufruf bekommen auch D1, D2 und D5 einen Pull‑up. Nach diesem Nachflashen lief `devtest.py` nicht noch einmal.
- **`tools/pins_live.py` (09.10.2026):** Live-Anzeige für die Fehlersuche am Steckbrett. Fragt `PINS` etwa dreimal pro Sekunde ab und schreibt bei jeder Änderung eine Zeile in Klartext: `PGOOD an D3`, `CHG an D4`, was an D0 anliegt (0 V, Brummen, springt 0 bis 4,2 V, ruhige Akkuspannung) und ob ein Prüfdraht an D1, D2 oder D5 Minus berührt (Durchgangsprüfer ohne Multimeter; nur an Minus halten, nie an `OUT`, `LIPO` oder `VBUS`). Start: `python A:/Recorder/firmware/ar1/tools/pins_live.py`. Solange sie läuft, ist COM6 belegt.
- Anlass: Sechs Messrunden über den Chat haben den Verdrahtungsfehler nicht gefunden, weil Claude weder die LEDs noch Tobis Handgriffe sieht. Tobi beantwortet Rückfragen nach LED und Netzteil meist nicht; eine Anzeige, die er selbst sieht, ist der bessere Weg.
- Erkenntnis dabei: `STATUS` misst D0 nur alle 2 s über wenige Millisekunden. Ein 50‑Hz‑Brummen erscheint dort als langsames Pulsieren; erst `PINS` zeigt es als das, was es ist.
- Rückfall: die bisherige AR1.1 liegt als `A:/Recorder/firmware/ar1/firmware-AR1.1.bin`.
- Fehlt noch: Abschalten bei leerem Akku, Taster, LED, Anzeige in [[Live Up]].

## Was unterwegs gelernt wurde

- FAT kann nicht auf eine vorhandene Datei umbenennen: erst löschen, dann umbenennen.
- `bit` ist in Arduino ein Makro; eigene Funktionen dürfen nicht so heißen.
- Die Uhr des Chips überlebt einen Reset. Ohne eigene Kennzeichnung sähe eine alte Zeit wie eine frische aus.
- Der Kartentreiber gibt die Karte nach einem Schreibfehler auf; bis zum Neu-Einhängen ist auch das Journal nicht schreibbar.
- Die Bluetooth-Adresse des Chips ist nicht die WLAN-MAC.
- Am Schreibtisch kommt JacobHome nur mit −70 bis −77 dBm an; einzelne Anmeldeversuche scheitern und klappen beim nächsten Mal.
- Android benutzt die Funkstrecke einer eben geschlossenen Bluetooth-Verbindung für die nächste weiter und trennt sie nach ein bis drei Sekunden doch. Der Recorder sieht dann keine neue Verbindung, nur eine zweite Aushandlung. Lösung: frische Zufallszahl für jede Aushandlung (Firmware), 4 s Ruhe vor dem Neuverbinden (App).
- Die Bluetooth-Bibliothek zählt ihre Verbindungen einen Moment vor oder nach dem eigenen Rückruf: Wer beides vergleicht, muss eine Abweichung erst gelten lassen, wenn sie anhält.
- `uiautomator dump` liefert bei der Compose-Oberfläche von Live Up nichts; am Handy wurde mit Bildschirmfotos und Koordinaten getestet.
- Ist das Handy gesperrt, lässt sich die App nicht weiter testen (PIN wird nicht eingegeben).

Verwandt: [[Audio-Recorder]], [[Recorder DV2 Lauftest]], [[Live Up]], [[JacobNAS]]
