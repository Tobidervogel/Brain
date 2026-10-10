---
titel: Recorder Omi Anbindung
tags: [projekt, audio, ki, cloud, omi, selbstbetrieb]
erstellt: 2026-10-10
aktualisiert: 2026-10-10
---

# Recorder Omi Anbindung

Tobis Ziel vom 10.10.2026: den [[Audio-Recorder]] mit **Omi** verbinden (Open-Source-Projekt von BasedHardware, `github.com/BasedHardware/omi`, MIT-Lizenz; App, Backend und Firmware offen). Omi macht aus Transkripten Unterhaltungen, Erinnerungen und Aufgaben und hat einen Chat darüber.

Seit dem Abend des 10.10.2026 genauer: Omi soll **privat** laufen, auf Tobis eigenen Geräten (PC, NAS, S9+ mit Android 9), ohne laufende Zahlungen an Omi oder andere Anbieter, und alltagstauglich sein. Reihenfolge des Auftrags: 1. prüfen, ob es geht, 2. Backend selbst betreiben, 3. Handy-App, 4. Recorder als Omi-Gerät.

## Kurzfassung der Prüfung (Claude, 10.10.2026 abends)

Geprüft am Quellcode von Omi (`BasedHardware/omi`, Stand `ce9c39f` vom 10.10.2026), nicht an der Doku und nicht an einem laufenden Server.

- **Geht es? Ja, mit Einschränkungen.** Omi bringt selbst alles für einen lokalen Betrieb mit: Datenbank und Anmeldung als lokale Emulatoren, Dateiablage auf der Festplatte statt Google Cloud, und eine App-Variante für „Community builds“, die sich an einem lokalen Server anmeldet. Das ist der Weg für Tobi.
- **Ganz ohne laufende Kosten** geht es, wenn auch das Sprachmodell lokal läuft. Dann kostet nur der Strom. Ob ein lokales Modell auf Tobis PC schnell und gut genug ist, ist **nicht gemessen**. Nach Rechnung: nur mit der CPU zu langsam für den Alltag, mit der GTX 980 knapp machbar mit einem kleinen Modell, mit der RTX 3080 gut.
- **Billigste Lösung, falls das lokale Modell nicht reicht:** nur das Sprachmodell in der Cloud mieten (z. B. GPT-5 nano), alles andere bleibt zu Hause. Grob 1 bis 3 € im Monat. Die Transkripte gehen dann an diesen Anbieter.
- **`pc/recorder.py` passt zum Endpunkt** (Anmeldung, Felder), hat aber fünf Schwachstellen, eine davon kann ganze 10-Minuten-Stücke verwerfen (unten).
- **Die App verlangt Android 10 nicht wegen einer Bibliothek.** Die 56 Android-Plugins brauchen höchstens API 24. Für API 28 muss mindestens ein Aufruf im eigenen Kotlin-Code abgesichert werden. Ein echter Bau mit API 28 und ein Test auf Android 9 stehen aus.
- **Teil 2 bis 4 sind nicht begonnen**, weil diese Sitzung die beiden Ziel-Repos nicht erreicht (siehe „Zugriff“).

## 1a: Wie der Recorder Daten in Omi bekommt

### Zwei Routen, darum widerspricht sich die Doku

| Route | Anmeldung | Grenze |
|---|---|---|
| `POST /v1/dev/user/conversations/from-segments` (Developer-API) | `Authorization: Bearer omi_dev_…` (Developer-Key) | effektiv **25 je Stunde** |
| `POST /v1/conversations/from-segments` (für Omis eigene Apps) | Firebase-ID-Token der angemeldeten App | 30 je Stunde |

Beide teilen sich Felder und Verarbeitung (`routers/developer.py`). `pc/recorder.py` nutzt die erste Route mit Developer-Key; das ist richtig.

- **Rechte:** Der Key braucht den Bereich `conversations:write`. Ein Key ohne ausgewählte Bereiche darf nur lesen (`utils/scopes.py`, `routers/api_key_management.py`); dann antwortet Omi mit 403. Beim Anlegen also ausdrücklich „Unterhaltungen schreiben“ wählen.
- **Grenzen:** Zwei Zähler gelten gleichzeitig, `dev:conversations` 25 je Stunde und `dev:conversations_from_segments` 30 je Stunde (`utils/rate_limit_config.py`, gezählt in Redis). Bei Daueraufnahme schickt der Recorder 6 je Stunde; nach einer Pause mit Rückstau gibt es 429.
- **Felder** (`CreateConversationFromTranscriptRequest`): `transcript_segments` 1 bis 500 Stück mit `text`, `start`, `end` (Pflicht), `speaker` (`SPEAKER_00` …), `is_user`, optional `person_id`. Dazu `source` (`external_integration` ist erlaubt), `language`, `started_at`, `finished_at` (beide mit Zeitzone, sonst 422), `client_session_id` (gleiche Kennung liefert dieselbe Unterhaltung, auch nach Löschen kein Doppel).
- **Prüfungen:** Ein einziges Segment mit `end <= start`, `start < 0` oder leerem Text lässt die ganze Anfrage mit 422 scheitern.
- **Verarbeitung läuft in der Anfrage selbst:** Titel, Zusammenfassung, Erinnerungen und Aufgaben entstehen, bevor die Antwort kommt. Läuft die Anfrage noch, antwortet eine Wiederholung mit 409; nach 15 Minuten gilt ein hängender Auftrag als verwaist.

### Passt `pc/recorder.py`?

Im Grundsatz ja: Header, Felder, `source`, `language: de`, `is_user` für „Tobi“ und `client_session_id` stimmen. Gefunden:

1. **Ganze Stücke können verloren gehen.** Die Zeiten werden auf 0,01 s gerundet. Liefert Whisper ein Segment ohne Dauer (kommt vor), ist `end == start`, Omi lehnt die ganze Anfrage mit 422 ab und `recorder.py` benennt den Auftrag in `.abgelehnt` um. Abhilfe: solche Segmente auslassen oder `end` anheben.
2. **Zeiten verschoben.** `started_at` ist Dateibeginn plus Beginn des ersten Segments, die Segmentzeiten bleiben aber relativ zum Dateibeginn. Omi rechnet sie ab `started_at`; Anzeige und Ende liegen damit um die Stille am Dateianfang zu spät.
3. **10-Minuten-Stücke werden zu 10-Minuten-Unterhaltungen.** Omi trennt Unterhaltungen sonst an längeren Pausen. Ein einstündiges Gespräch würde sechs Unterhaltungen; ein Stück mit zwei Gesprächen eine. Besser: Segmente über Dateigrenzen sammeln und an Pausen schneiden.
4. **Zeitgrenze 60 s.** Mit einem lokalen Sprachmodell dauert die Verarbeitung länger. Es geht trotzdem nichts verloren (Wiederholung, 409, dann dieselbe Unterhaltung), aber es entstehen unnötige Anfragen.
5. **Bei 401, 403 und 429 alle 10 s ein neuer Versuch** ohne Pause dazwischen. Schadet nicht, füllt aber das Fenster mit Meldungen.

Behoben wird das in Teil 2 zusammen mit der Umstellung auf das eigene Backend.

## 1b: Fremde Dienste des Omi-Backends

Gesucht wurde im Code nach Clients, Umgebungsvariablen und Startprüfungen. „Lokal“ heißt: Omi hat es schon eingebaut oder es läuft als freier Container auf dem PC.

| Dienst | Wofür | Pflicht oder abschaltbar | Lokal ersetzbar (womit) | Kostenlose Stufe reicht? Sonst Kosten |
|---|---|---|---|---|
| Google Cloud Firestore | gesamte Datenbank | Pflicht | **ja**: Firestore-Emulator (firebase-tools, Java 21), den Omi selbst für die lokale Entwicklung nutzt | lokal kostenlos. Daten liegen im Arbeitsspeicher und werden nur beim sauberen Beenden gesichert; regelmäßige Sicherung baut Teil 2 dazu |
| Firebase Authentication | Anmeldung von App und Backend | Pflicht | **ja**: Auth-Emulator plus Omis Anmeldung für lokale Server (`POST /v1/auth/local-dev/custom-token`) | kostenlos. Nachteil siehe „Sicherheit“ |
| Google Cloud Storage | Audio, Sprachprofile, Chat-Dateien | Pflicht im Cloud-Betrieb | **ja, eingebaut**: Ablage auf der Platte (`OMI_LOCAL_STORAGE_ROOT`, bei `OMI_ENV_STAGE=local`) | kostenlos; Sicherung aufs NAS per SMB |
| Redis | Grenzen, Zwischenspeicher, Sperren | Pflicht (Verbindung beim Start) | **ja**: Redis-Container | kostenlos |
| Typesense | Suche in Unterhaltungen und Erinnerungen | abschaltbar (ohne `TYPESENSE_HOST` aus) | **ja**: Typesense-Container 27.1 | kostenlos |
| Pinecone | Vektorsuche: der Chat findet passende Erinnerungen und Unterhaltungen | abschaltbar, läuft ohne weiter (`database/vector_db.py`), der Chat findet dann weniger | **nicht eingebaut**: eigener kleiner Vektorspeicher auf dem PC nötig (Teil 2) | Pinecone hat eine Gratisstufe, die Daten liegen dann aber in den USA |
| OpenAI, Sprachmodell (`gpt-6-luna`, `gpt-5-nano`) | Titel, Zusammenfassung, Erinnerungen, Aufgaben, Chat | **Pflicht für die eigentliche Omi-Funktion**; ohne Modell bleibt nur das Rohtranskript | **ja**: jedes OpenAI-kompatible lokale Modell (Ollama oder llama.cpp); die Modellnamen muss Teil 2 umlenken | lokal nur Strom; Cloud-Rückfall siehe „Kosten“ |
| OpenAI, Embeddings (`text-embedding-3-large`) | Vektoren für die Vektorsuche | nur mit Vektorsuche | **ja**: lokales Embedding-Modell über Ollama | kostenlos |
| Omi-LLM-Gateway (eigener Dienst) | leitet Modellaufrufe weiter | abschaltbar (`OMI_LLM_GATEWAY_FEATURE_MODE=off`, dann direkt) | nicht nötig | – |
| Pusher (eigener Omi-Dienst) | Nachverarbeitung, wenn die App Ton live streamt | nur für Teil 4 | **ja**: Container aus dem Omi-Repo | kostenlos |
| Geplante Jobs (Tageszusammenfassung, Pflege der Erinnerungen, Selbstheilung; bei Omi als Cloud-Run-Jobs) | Hintergrundarbeit | ohne sie fehlen Tageszusammenfassung und Pflege | **ja**: dieselben Skripte (`modal/job.py` u. a.) per Zeitplan im Container | kostenlos |
| Live-Spracherkennung (Deepgram, Soniox, Speechmatics, Modulate) | Echtzeit-Transkript, wenn ein Gerät über die App streamt | für Tobis Weg (PC transkribiert) nicht nötig | **ja**: App-Einstellung „Local Whisper“ (eigener Whisper-Server auf dem PC) oder Omis Parakeet-Dienst (braucht NVIDIA-GPU mit CUDA, das Streaming-Modell kann nur Englisch) | kostenlos |
| Sprechertrennung (Parakeet/pyannote, externer Diarizer) | Stimmen zuordnen | abschaltbar | `recorder.py` macht das schon lokal (sherpa-onnx) | – |
| Perplexity | Websuche im Chat | abschaltbar | nein | Funktion entfällt |
| Gemini, ElevenLabs | Vorlesen, Übersetzung | abschaltbar | Vorlesen über die Sprachausgabe des Handys | Funktion entfällt in Omi |
| Firebase Cloud Messaging | Push-Mitteilungen (z. B. Erinnerung an Aufgaben) | abschaltbar | nein (Google-Dienst) | kostenlos, bräuchte aber ein eigenes Firebase-Projekt; im lokalen Profil ohne Push |
| Stripe | Bezahlung, Abos | abschaltbar | – | – (aber Tarif-Grenzen, siehe unten) |
| Cloud Tasks, Cloud KMS | Sync-Aufträge, Bildschirmfotos der Desktop-App | abschaltbar (Standard `inline`) | – | – |
| PostHog, LangSmith, Crashlytics, Intercom | Statistik, Fehlerberichte, Support | abschaltbar | – | entfällt |
| Hume, Twilio, Google Maps/RapidAPI, Anmeldungen bei Google-Kalender, Notion, Whoop, X, GitHub | Gefühlsanalyse, Telefonate, Ortsnamen, Integrationen | abschaltbar | nein | Funktionen entfallen |
| ngrok | Erreichbarkeit von außen | nicht nötig | im Heimnetz direkt; unterwegs später WireGuard | kostenlos |

### Was im Selbstbetrieb wegfällt oder schlechter wird

Nichts davon fällt stillschweigend weg; Stand nach Code, Umfang erst nach Teil 2 bestätigt:

- **Websuche im Chat** (Perplexity), **Push-Mitteilungen**, **Vorlesen** über Omi, **Integrationen** (Kalender, Notion usw.), **Telefonate**, **Ortsnamen** zu Unterhaltungen, **Gefühlsanalyse**, **Support-Chat**.
- **Omis App-Marktplatz** ist leer, weil er in Omis Datenbank liegt.
- **Vektorsuche** im Chat, bis der lokale Ersatz gebaut ist.
- **Qualität** von Zusammenfassungen und Erinnerungen hängt am lokalen Modell und ist schwächer als bei Omis Cloud-Modellen.
- **Anmeldung mit Google oder Apple** geht nicht; die App meldet sich über den lokalen Server an.

### Tarif-Grenzen

Omi rechnet auch im Selbstbetrieb mit Tarifen (`utils/subscription.py`). Ein neues Konto bekommt „basic“ (Gratis) mit Monatsgrenzen für Zuhören, Verarbeitung und Chatfragen. Teil 2 setzt Tobis Konto per Skript auf den Tarif „unlimited“; ohne das sperrt Omi nach dem Gratis-Kontingent z. B. den Chat (402 `quota_exceeded`) und das Live-Zuhören.

### Sicherheit

Die lokale Anmeldung prüft keine Unterschrift: Wer Port 8000 und 9099 am PC erreicht, kann sich als jedes Konto ausgeben. Darum nur im Heimnetz betreiben und in der Windows-Firewall auf PC und Handy beschränken. Teil 2 prüft, ob sich die lokale Anmeldung zusätzlich mit einem Geheimnis schützen lässt.

## Sprachmodell lokal: reicht Tobis Hardware?

PC laut [[Audio-Recorder]]: Ryzen 5 1500X (4 Kerne, 2017), rund 16 GB RAM, GTX 980 mit 4 GB. Whisper läuft dort bisher auf der CPU, ungefähr in Echtzeit.

Annahme für ein 10-Minuten-Stück mit viel Sprache: rund 10 bis 20 Modellaufrufe, zusammen grob 40 000 bis 80 000 Token Eingabe und 3 000 bis 6 000 Token Ausgabe. Die Zeiten sind aus Speicherbandbreite und Rechenleistung geschätzt, **nicht gemessen**:

| Hardware | Modell | geschätzt je 10-Minuten-Stück | Urteil |
|---|---|---|---|
| nur CPU (Ryzen 5 1500X) | 4 Mrd. Parameter, 4 Bit | 20 bis 40 min | zu langsam: eine Stunde Gespräch braucht Stunden |
| GTX 980, 4 GB | 3 bis 4 Mrd. Parameter, 4 Bit, ganz auf der Karte | 3 bis 8 min | knapp alltagstauglich; Ergebnisse auf Deutsch spürbar schwächer |
| RTX 3080, 10 GB (Bruder) | 8 Mrd. Parameter, 4 Bit (14 Mrd. knapp) | 1 bis 2 min | gut |

- Ollama unterstützt laut eigener Doku NVIDIA-Karten ab Compute Capability 5.0; die GTX 980 hat 5.2. Der nötige Treiberstand ist vor dem Einrichten zu prüfen.
- Omi verlangt vom Modell feste JSON-Antworten und für den Chat Werkzeugaufrufe. Beides können aktuelle kleine Modelle (z. B. Qwen3) in Ollama; wie zuverlässig mit Omis Vorgaben, zeigt erst der Test.
- Teil 2 bringt deshalb einen Messlauf mit: ein echtes Transkript durch die Omi-Verarbeitung schicken und Zeit und Ergebnis ansehen. Danach entscheidet Tobi zwischen lokal und Cloud-Rückfall.

## Kosten

- **Einmalig: 0 €.** Omi (MIT), Docker Desktop (privat kostenlos), Ollama und die Modelle sind frei. Keine neue Hardware nötig.
- **Laufend, lokal: nur Strom.** Läuft der PC ohnehin (für `recorder.py`), kommen grob 2 bis 5 € im Monat für Backend und Modell dazu. Muss der PC nur dafür durchlaufen, grob 15 bis 20 € im Monat (60 bis 80 W rund um die Uhr bei 0,35 €/kWh). Beides geschätzt. Das Backend kann auch nur bei Bedarf laufen; `recorder.py` hält die Aufträge so lange in `omi_outbox/`.
- **Laufend mit Cloud-Rückfall:** GPT-5 nano kostet laut Preisvergleichen 0,05 $ je Million Eingabe-Token und 0,40 $ je Million Ausgabe-Token (Stand Herbst 2026, auf der OpenAI-Seite nachprüfen). Bei 2 bis 3 Stunden Gespräch am Tag grob 1 bis 3 € im Monat. Gratisstufen wie die von Gemini scheiden aus: Google verwendet dort die Daten zur Verbesserung seiner Produkte.

## 1c: Warum die Omi-App Android 10 verlangt

- `app/android/app/build.gradle` setzt `minSdkVersion 29`, `targetSdkVersion 36`. Flutter ab 3.44, JDK 21, NDK 28.
- **Keine Bibliothek erzwingt API 29.** Geprüft an den genauen Versionen aus `pubspec.lock`: 56 Plugins mit Android-Teil, höchster Bedarf API 24 (u. a. `flutter_sound`, `flutter_tts`, `webview_flutter_android`, `url_launcher_android`). Firebase Auth, Intercom (SDK 18.10.0) und `awesome_notifications` brauchen 23. Die Git-Abhängigkeiten (`whisper_flutter_new`, `opus_flutter_android`) brauchen 21 bzw. 19. Twilio Voice 6.10.2 ließ sich nicht prüfen (Maven Central lehnte den Abruf ab); die Telefonfunktion wird für den Recorder nicht gebraucht und kann notfalls raus.
- **Im eigenen Kotlin-Code** steckt mindestens ein Aufruf, den es erst ab Android 10 gibt, ohne Prüfung: `PhoneMicForegroundService.kt` ruft `startForeground(…, FOREGROUND_SERVICE_TYPE_MICROPHONE)`. Auf Android 9 wäre das ein `NoSuchMethodError`, den das umgebende `catch (Exception)` nicht fängt; die App würde beim Aufnehmen mit dem Handy-Mikrofon abstürzen. Die übrigen gefundenen neuen Aufrufe (Companion Device ab 33, Short Service ab 34) stehen hinter einer Versionsprüfung. Vollständig zeigt das erst Lint beim Bauen.
- **Folgerung:** Ein Bau für API 28 ist nach Code wahrscheinlich möglich: `minSdk` auf 28, die ungeschützten Aufrufe absichern, Android Lint (NewApi) findet den Rest. Gebaut und auf Android 9 getestet ist das **nicht**. Für BLE-Suche braucht Android 9 außerdem die Standortfreigabe.
- **Für den eigenen Server vorgesehen:** Flavor `dev` mit Profil `local_dev` (`app/lib/env/environment_profile.dart`): Firebase-Projekt `demo-omi-local`, Auth-Emulator, Server nur im privaten Netz. Adresse über `OMI_API_BASE_URL` und `OMI_FIREBASE_AUTH_EMULATOR_HOST` beim Bauen.

## Plan für Teil 2 bis 4 (noch nicht gebaut)

- **Teil 2, Backend:** Docker Compose nach dem Vorbild von Omis eigenem lokalen Aufbau (`scripts/dev-harness`): Firestore- und Auth-Emulator mit regelmäßiger Sicherung (auch aufs NAS), Redis, Typesense, Backend mit `OMI_ENV_STAGE=local` und Ablage auf der Platte, Pusher, Zeitplan für die Jobs. Ollama läuft direkt unter Windows (einfacher Zugriff auf die Grafikkarte). Änderungen im Fork: Modellaufrufe auf das lokale Modell umlenken, lokaler Vektorspeicher statt Pinecone, Skripte für Tarif „unlimited“ und Developer-Key, Messlauf. `recorder.py`: Punkte 1 bis 5 von oben, eigenes Backend über `OMI_URL`, Unterhaltungen an Pausen schneiden.
- **Teil 3, Handy:** zuerst den Bau der Omi-App mit API 28 versuchen (Flavor `dev`, Lint, Test im Emulator mit Android 9). Gelingt das nicht sauber, kommt ein Modul in [[Live Up]], das über die Developer-API liest (`/v1/dev/user/conversations`, `/memories`, `/action-items`, Fragen über `/ask`). Vorläufige Einschätzung: Die Bruchstellen der Omi-App liegen weniger im Android-9-Bau als im Drumherum: Flutter-Werkzeugkette mit JDK 21 und NDK auf Tobis PC, Neubau bei jedem Omi-Update, Android 9 bei Omi ungetestet. Entschieden wird nach dem Bauversuch.
- **Teil 4, Recorder als Omi-Gerät:** nur wenn Teil 3 eine laufende Omi-App ergibt. Live-Transkript dann über „Local Whisper“ auf dem PC statt über einen bezahlten Dienst.

## Zugriff (Stand 10.10.2026)

Diese Sitzung erreichte nur `Tobidervogel/Brain`. `Tobidervogel/Recorder-LiveUp` und der Omi-Fork (`Tobidervogel/omi`) waren nicht zugänglich: Die GitHub-App von Claude ist für beide nicht freigegeben, oder der Fork heißt anders. Deshalb liegt diese Notiz vorerst nur in der Brain-Kopie; ins Recorder-LiveUp-Repo kommt sie, sobald es erreichbar ist. Für Teil 2 bis 4 muss Tobi beide Repos für Claude freigeben (claude.ai, GitHub verbinden, App für die Repos installieren).

## Bisheriger Stand (vor der Prüfung)

Recherche vom Vormittag des 10.10.2026, jetzt in Teilen überholt:

1. **Über die Cloud (gebaut):** Die PC-Seite transkribiert wie bisher lokal und schickt nur den Text an Omi (Route und Felder wie oben). Einen Upload von Audiodateien bietet die Developer-API nicht; deshalb geht Text, nicht Ton.
2. **Über Bluetooth (nicht gebaut):** Der Recorder gibt sich als Omi-Gerät aus. Die Omi-App erwartet den Dienst `19B10000-E8F2-537E-4F6C-D104768A1214` mit Audio (`19B10001-…`) und Codec (`19B10002-…`), Pakete mit 3 Byte Kopf, Codec 0 = PCM 16 kHz/16 Bit (das nimmt AR1 schon auf), 20 = Opus. `omiGlass` im Omi-Repo läuft auf demselben Board (XIAO ESP32S3). Nachteile: Handy muss in der Nähe sein, und es muss neben der verschlüsselten Fernbedienung aus [[Recorder AR1 Firmware]] laufen.

### Was gebaut ist

In `A:/Recorder/recorder.py` (PC-Seite):

- Liegt `A:/Recorder/omi_key.txt` mit dem Developer-Key im Ordner, legt `process()` jedes Transkript zusätzlich als Auftrag in `omi_outbox/` ab; `omi_flush()` schickt die Aufträge ab. Ohne die Datei bleibt alles lokal wie bisher.
- Kein Netz, falscher Schlüssel oder Limit: der Auftrag bleibt liegen und wird alle 10 s neu versucht. Lehnt Omi den Inhalt ab (400, 413, 422), wird er in `.abgelehnt` umbenannt.
- Stimme „Tobi“ (aus `enroll`) gilt bei Omi als Besitzer (`is_user`); Stellschraube `OMI_ME`. Eigener Omi-Server: Umgebungsvariable `OMI_URL`.
- `selftest.py` prüft den Auftrag und dass er bei fehlendem Netz liegen bleibt: besteht (10.10.2026).
- **Nicht gegen ein echtes Omi getestet.** Die frühere offene Frage „Developer-Key oder Firebase-Token“ ist am Code geklärt: Developer-Key mit Schreibrecht, siehe 1a.
- Der Key gehört nicht ins Brain.

## GitHub

Am 10.10.2026 wollte Tobi alles zu Recorder, Lader und [[Live Up]] lesbar in `Tobidervogel/Brain` haben. Das Repo ist öffentlich. Tobi hat entschieden: dazulegen statt löschen, mit Chatprotokollen. Es liegt unter `Projekte/Recorder und Live Up/` (Notizen, Code ohne Aufnahmen und Token, 20 Chatprotokolle); `changes/` und `brain.zip` sind unberührt. Der Ordner ist eine Kopie von diesem Tag und aktualisiert sich nicht von selbst.

Verwandt: [[Audio-Recorder]], [[Recorder AR1 Firmware]], [[Live Up]], [[Brain System]]
