---
titel: Beschreibung Live Up und Audio-Recorder
tags: [projekt, beschreibung, live-up, recorder]
erstellt: 2026-10-08
aktualisiert: 2026-10-08
---

# Beschreibung Live Up und Audio-Recorder

Zwei Projekte von Tobi, die zusammengehören: **Live Up** ist seine eigene Android-App, der **Audio-Recorder** ein selbstgebautes Aufnahmegerät mit einem ESP32. Die App steuert den Recorder. Stand dieser Beschreibung: 08.10.2026 (Live Up 0.12.0, Recorder-Firmware AR1.1). Einzelheiten stehen in [[Live Up]], [[Audio-Recorder]] und [[Recorder AR1 Firmware]].

## Der Audio-Recorder (ESP32)

**Was es ist.** Ein kleines Gerät, das Ton aufnimmt und sicher auf einer Speicherkarte ablegt. Es soll später am Körper getragen werden. Die Kamera des Boards ist bewusst abgezogen: zuerst ein robuster Audio-Recorder, alles andere später.

**Hardware.** Seeed Studio XIAO ESP32S3 Sense: ESP32-S3 mit zwei Kernen (240 MHz), 8 MB PSRAM, 8 MB Flash, WLAN und Bluetooth LE, eingebautes Mikrofon, microSD-Karte (64 GB). Strom kommt bisher über USB; Akku, Ladeplatine und Taster sind geplant, aber noch nicht bestellt ([[Recorder Hardware Kette]]).

**Was die Firmware (AR1) macht.**
- Sie nimmt mit 16 kHz, 16 Bit, Mono auf und schreibt WAV-Dateien in Stücken von 10 Minuten.
- Ein Zwischenspeicher für 16 Sekunden fängt langsame Kartenzugriffe ab. Alle 2 Sekunden wird gesichert.
- Jedes Stück bekommt eine Prüfsumme (SHA-256). Ein Journal hält jeden Schritt fest. Fällt der Strom aus, bleibt die Aufnahme bis zur letzten Sicherung abspielbar.
- Zustände: Leerlauf, Aufnahme, Pause, Tiefschlaf. Nach dem Einstecken nimmt das Gerät von selbst auf, wenn das so eingestellt ist.
- Bedient wird es mit Textbefehlen über USB oder Bluetooth, zum Beispiel `STATUS`, `REC START`, `REC STOP`, `REC PAUSE`, `TIMER`, `SLEEP`, `WIFI SET`.
- Die Bluetooth-Verbindung ist mit einem Schlüssel aus der einmaligen Kopplung verschlüsselt (AES-256-GCM).

**Der Weg der Aufnahmen.** Leitsatz: Bluetooth steuert, WLAN überträgt, der PC archiviert, das Brain findet wieder.
1. Der Recorder sammelt die Aufnahmen auf der Karte.
2. Das Handy steuert ihn über Bluetooth (Modul „Recorder“ in Live Up).
3. Der PC soll die fertigen Stücke über WLAN holen, prüfen, aufs NAS legen, mit Whisper in Text umschreiben und einen Verweis ins Brain eintragen. Das PC-Programm für Ablage und Umschrift gibt es schon (`A:/Recorder/recorder.py`), das Abholen noch nicht.
4. Der Recorder löscht nur, was nachweislich gesichert ist.

**Stand.**
- Fertig und am Gerät getestet: die Aufnahme (ein Dauerlauf über 19,6 Stunden ohne ein verlorenes Sample) und die Bluetooth-Fernbedienung vom Handy.
- Noch nicht gebaut: das Abholen über WLAN (Schritt 3), das Überbrücken über das Handy unterwegs, Akkubetrieb und Gehäuse.

**Wichtig.** Gespräche anderer ohne deren Wissen aufzunehmen ist strafbar (§ 201 StGB). Wer in der Nähe ist, muss von der Aufnahme wissen.

## Live Up (Android-App)

**Was es ist.** Eine App, die mehrere kleine Apps unter einem Dach bündelt. Sie läuft auf Tobis Samsung Galaxy S9+ (Android 9), ist in Kotlin mit Jetpack Compose geschrieben und speichert alles nur auf dem Handy. Es gibt kein Konto und keine Cloud.

**Die Startseite** zeigt den Tag auf einen Blick (Kalorien übrig, Eiweiß, Zeit am Handy) und darunter die Kacheln der Bereiche.

**Die Bereiche.**
- **Übersicht („Dein Tag“):** Essen und Eiweiß als Ringe, ein Tipp aus den eigenen Zahlen, Bildschirmzeit, Kacheln zu den Bereichen.
- **Essen:** Kalorien und Eiweiß zählen, nach Mahlzeiten sortiert. Produkte per Strichcode (Open Food Facts). Mahlzeiten lassen sich als Foto oder Text an ChatGPT schicken; die Antwort wird eingelesen. Das Tagesziel rechnet die App aus Größe, Gewicht, Alter und Aktivität.
- **Geld:** führt zu zwei Teilen.
  - **Geldguru:** Konto und Bargeld, Buchungen mit Kategorien, Budgets, Spartöpfe, Statistik. Kassenzettel und Kontoauszüge kann ChatGPT aus Fotos eintragen; die App prüft auf Doppelte.
  - **Wallet:** Karten, Ausweise, Tickets und NFC-Chips im Scheckkarten-Format, jede mit eigenem Kartenbild, als Stapel wie im Google Wallet. Die Nummer lässt sich als Strichcode zeigen, NFC-Chips lassen sich auslesen, ChatGPT kann Karten per JSON anlegen und gestalten. Der Stapel ist immer zu sehen; geschützte Karten sind dabei unscharf, ihre Daten zeigt erst der Fingerabdruck. Bezahlen kann es nicht.
- **Fahrschule:** Führerschein Klasse B mit BF17 zum Abhaken (Theoriestunden, Sonderfahrten, Schritte bis zur Prüfung). Die Kosten kommen aus dem Geldguru.
- **Recorder:** die Fernbedienung für den Audio-Recorder über Bluetooth: Zustand, Start, Stopp, Pause, Timer, WLAN einrichten, Dateiliste, Kopplung.
- **Einstellungen:** Profil, Ernährungsziele, Berechtigungen, Quellen.

**ChatGPT ohne Schlüssel.** Die App hat keinen eigenen KI-Zugang. Sie baut einen Auftrag, Tobi teilt ihn an die ChatGPT-App und fügt die Antwort (ein JSON-Block) wieder ein. Vor dem Übernehmen zeigt die App, was sich ändern würde.

**Was wieder entfernt wurde.** Lebensplan, Pause (Anti-Scroll), Aufgaben, Schule, Schlaf mit Wecker (07.10.2026) und Training (08.10.2026). Live Up weckt nicht mehr.

**Technik in Kürze.** Paket `de.tobidervogel.liveup`, Quelltext unter `A:\LiveUp`, drei SQLite-Datenbanken (Essen, Geldguru, Wallet), 82 automatische Tests. Rechte: Internet (nur für die Strichcode-Abfrage), Nutzungsdaten (Bildschirmzeit), Bluetooth (Recorder), NFC und Fingerabdruck (Wallet).

Verwandt: [[Live Up]], [[Audio-Recorder]], [[Recorder AR1 Firmware]], [[Recorder Hardware Kette]], [[Tobi]]
