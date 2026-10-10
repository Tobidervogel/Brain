---
titel: Audio-Recorder
tags: [projekt, hardware, ki, audio]
erstellt: 2026-09-26
aktualisiert: 2026-10-09
---

# Audio-Recorder

Projekt seit 26.09.2026: ein kleines tragbares Aufnahmegerät mit Seeed XIAO ESP32S3 Sense. Aktuelles Ziel vom 05.10.2026: maximale stabile gleichzeitige Audio-/Videoaufnahme während der Nutzung, später ein umschaltbarer Nur-Audio-Sparmodus, hervorgehobene Fotos/Marker und möglichst lokale PC-/NAS-Auswertung für [[Live Up]] und [[Brain System]]. Die frühere pauschale 24/7-Audio-Vorgabe wurde präzisiert: nicht zwingend rund um die Uhr, bis zu 24 Stunden eher als spätere Möglichkeit. Nachts soll das Gerät möglichst wie das Handy geladen werden. DV1.4 ist bisher Testfirmware, kein fertiger Dauerrecorder. Seit dem Abend des 05.10.2026 gibt es **DV2.0**, die erste Firmware, die Video und Audio wirklich gleichzeitig auf die SD-Karte aufnimmt: siehe [[Recorder DV2 Lauftest]].

## Entscheidungen (26.09.2026)

- **Daueraufnahme, kein Taster.** Tobi will es tragen, solange er in seinem Zimmer bzw. zu Hause ist. Hinweis auf § 201 StGB wurde gegeben (Worte anderer ohne deren Wissen aufzunehmen ist strafbar); Tobi hält es in seinem Zimmer für zulässig. Empfehlung: Familie einmal Bescheid sagen.
- **Akku:** so lange wie möglich; Tobi lädt nachts und das Gerät nimmt beim Laden weiter auf.
- **Pro Tag eine Aufnahme ab 0 Uhr:** Das Gerät schickt 10-Minuten-Stücke, der PC sortiert sie nach Tag (Stücke über Mitternacht werden auf beide Tage verteilt).
- **Upload nur im Heim-WLAN.** Unterwegs über Handy-Hotspot + WireGuard kommt später.
- **Rohaudio liegt auf dem NAS** (`//jacobnas/JacobNAS/Tobi/Recorder/audio/JJJJ-MM-TT/`), Transkript im Brain. Das Gerät löscht nur, was nachweislich auf dem NAS angekommen ist (seit 26.09.2026 abends; vorher war `A:\Recorder\audio\` geplant).
- **Speicherkarte als Ringpuffer (Vorschlag, Firmware):** Gesicherte Dateien bleiben als zweite Kopie auf der Karte und werden erst gelöscht, wenn Platz gebraucht wird, älteste zuerst. Ist die Karte voll mit Ungesichertem, wird die älteste ungesicherte Datei überschrieben, damit die Aufnahme weiterläuft. Rhythmus nach dem ersten echten Tag festlegen.
- **Stimmen trennen:** ja, mit Namen für angelernte Stimmen.
- **Selbst bauen statt Omi** (Omi zu teuer). Werkzeug vorhanden: Lötkolben, 3D-Drucker.
- Die RTX 3080 gehört dem Bruder, siehe [[Hardware und Laufwerke]]; nur später als optionale Hilfe.


## Stand 05.10.2026 — erster Aufbau

Bestellt ist vorerst nur der **Seeed Studio XIAO ESP32S3 Sense**. Er soll zuerst auf einer Arduino-/Breadboard-Steckplatte programmiert und getestet werden. Gehäuse, Akku, Display, Ladeelektronik und Trageform kommen später.

Tobis Ziel bleibt: zuerst 24/7-Audio aufnehmen, später eventuell auch Video aufnehmen und beides auf PC/NAS analysieren lassen. Neue Ideen vom 05.10.: Das Gerät könnte zusätzlich ein lokales Wakeword/Schlüsselwort erkennen, Smart-Home-Befehle auslösen oder als persönliches Anwesenheits-/Bestätigungsgerät dienen. Für echte Zwei-Faktor-Authentifizierung ist Vorsicht nötig: Der XIAO kann ein Besitzfaktor oder Bestätigungsknopf sein, sollte aber nicht allein sicherheitskritische Freigaben ersetzen.

Mögliche Trageformen neben der Kette: Clip an Kleidung/Rucksack, Magnet-Dock im Gehäuse mit Gegenmagnet an Kleidung oder Halterung, Armband nur mit deutlich flacherem Gehäuse und kleinem Akku, oder abnehmbares Dock-System für Tisch, Wand und Körper. Für Audio ist wichtig: Mikrofonöffnung frei nach außen, nicht unter Stoff, und Gehäuse entkoppeln, damit Reiben und Klopfen nicht lauter als Sprache werden.


### Erster USB-Test (05.10.2026)

Der bestellte XIAO ESP32S3 Sense ist angekommen und wurde per USB-C an den PC gesteckt. Windows erkennt ihn als `Serielles USB-Gerät (COM6)` und `USB JTAG/serial debug unit`, VID/PID `303A:1001`. Lesender Test mit `esptool` erfolgreich: Chip `ESP32-S3` Revision v0.2, Dual Core + LP Core, 240 MHz, WLAN, Bluetooth LE 5, 8 MB PSRAM, 8 MB Flash, 40-MHz-Quarz, MAC `94:a9:90:d0:a1:30`. Es wurde noch keine Firmware geflasht und keine SD-Karte benötigt.

### DV1.0-Firmware (05.10.2026)

Projektordner: C:/Users/a/Documents/Codex/2026-10-05/schau-du-mal-im-brain-also/work/esp32-dv1. PlatformIO wurde benutzerlokal installiert. DV1.0 ist eine reine USB-Testfirmware ohne SD, WLAN-Zugangsdaten, Mikrofon oder Kamera. Build erfolgreich, Upload auf COM6 erfolgreich. Serieller Test mit 115200 Baud zeigt Startbanner und Heartbeats: Audio Recorder DV1.0, Board Seeed XIAO ESP32S3 Sense, 2 Kerne, 240 MHz, 8 MB Flash, rund 8 MB PSRAM, MAC 94:A9:90:D0:A1:30, Heartbeat alle 2 Sekunden. LED-Pin wird als 21 gemeldet; sichtbares LED-Verhalten ist noch nicht separat beobachtet.


### DV1.2/DV1.3 Mikrofon und Status-WLAN (05.10.2026)

Auf Tobis Wunsch „mach automatisch weiter“ wurde die Firmware fortgesetzt. DV1.2 testete das eingebaute PDM-Mikrofon auf GPIO42 (Clock) und GPIO41 (Data) mit 16 kHz/16 bit. Der erste Rohpegeltest zeigte echte Samples; danach wurde die Auswertung auf `dc_offset`, `noise_rms`, `peak_to_peak` und `loudness` verbessert. In ruhiger Umgebung meldet die Firmware typischerweise `loudness=quiet`, beim Anlauf/Reset gab es einen lauten Ausschlag. Ein gezielter Klatsch-/Sprachtest durch Tobi steht noch aus.

DV1.3 kombinierte USB-Heartbeat, Mikrofon-Pegeltest und ein eigenes Status-WLAN ohne Heimnetz-Zugangsdaten: SSID `Recorder-DV1`, ein damaliges gemeinsames Testpasswort, Statusseite `http://192.168.4.1/`, JSON `http://192.168.4.1/status.json`. Build und Upload auf COM6 erfolgreich. Serieller Test bestätigte den Access Point und laufende Mikrofonwerte. Der PC selbst hat keine WLAN-Schnittstelle, deshalb wurde die Webseite damals nicht vom PC geöffnet. Der AP wurde in DV1.4 ersetzt; das historische Testpasswort steht nicht mehr in der Notiz.

### DV1.3 Kamera-Selbsttest (05.10.2026)

Die DV1.3-Firmware wurde um die Kamera erweitert: XIAO-Sense-Pinmapping nach Seeed-Doku, JPEG QVGA (320x240), Statusseite mit /capture.jpg. Build ohne Warnungen, Upload auf COM6 erfolgreich. Seriell bestätigt: Camera initialized: JPEG QVGA und Boot-Selbsttest Camera self-test OK: jpeg_bytes=4014 width=320 height=240. Damit ist belegt, dass die Kamera initialisiert und mindestens ein JPEG im RAM erzeugt. Sichtprüfung des Bildes über WLAN steht noch aus, weil der PC keine WLAN-Schnittstelle hat.

### Softwareplanung und DV1.4 (05.10.2026)

Tobi will jetzt ausdruecklich Dauer-Audio plus moeglichst durchgehendes Video. Dazu extra, manuell hervorgehobene Fotos mit Marker, insbesondere fuer automatisches Kalorientracking: Essen/Abwiegen filmen oder fotografieren, nachts lokal analysieren und spaeter in [[Live Up]] nachtragen. Ausserdem Planung weiterer Nutzungen und Frage nach heimlichem Datenversand/Privatheit. Diese Aufnahme- und Analysefunktionen sind ein Entwurf und noch nicht implementiert.

Ausfuehrlicher gemeinsamer Entwurf: C:/Users/a/Documents/Codex/2026-10-05/schau-du-mal-im-brain-also/outputs/Recorder-Softwareplan.md. Rollen: ESP erfasst/markiert/puffert, NAS archiviert, PC transkribiert und analysiert nachts. NAS-CPU/RAM unbekannt; eigener persistenter Empfangsdienst auf XigmaNAS Embedded muss geprueft werden. PC erneut gemessen: Ryzen 5 1500X (4/8), rund 16 GB RAM, GTX 980 4 GB; Aufnahmeordner auf NAS lesend erreichbar. Kein zugesichertes Echtzeit-Videomodell auf diesem PC. Vorhandenes A:/Recorder ist ein Audio-Prototyp mit HTTP/Token; TLS, Video, Auftragsjournal und automatischer Live-Up-Sync fehlen noch.

Markerentwurf: eindeutige Ereignis-ID, Kategorie, Zeitstempel, Zusatzfoto und Verweis auf Audio-/Videokontext. Fuer Essen Zutaten, Waage, Teller und ggf. Reste zusammenfuehren. Bild allein beweist keine genaue Menge oder unsichtbares Oel. Automatisch pruefbare Eintraege erzeugen; unklare Zutaten/Mengen mit Unsicherheit und Rueckfrage behandeln. Live Up hat bereits JSON-Felder name/g/kcal/eiweiss/kh/fett, aber noch keine passende automatische Server-Synchronisation.

Rechenbeispiel, kein Messwert: 30 KB/JPEG ergeben bei 2/5/10 Bildern pro Sekunde rund 5,18/12,96/25,92 GB Video pro Tag, plus 2,76 GB Audio. Hohe stabile Bildrate, Aufloesung, Stromverbrauch und Kamera-Schaerfe muessen mit gleichzeitigem Audio+SD+WLAN gemessen werden. Testziel Audio 16 kHz/16 bit/mono in 10-Minuten-WAVs; bisherige Pegel-Testschleife eignet sich noch nicht fuer lueckenlose Dateiaufnahme. In DV1.4 wurden bei ruhiger Umgebung nur etwa 8.448 gewertete Samples pro Sekunde ausgegeben; eine volle lueckenlose 16-kHz-Aufnahme ist damit nicht nachgewiesen.

DV1.4 gebaut und auf COM6 geflasht. WLAN wird ueber lokales Fenster outputs/Recorder-WLAN.py per USB eingerichtet; keine Zugangsdaten in Quelltext, Logs oder Brain. Speicherung auf dem Geraet nach erfolgreicher Verbindung, im Entwicklungs-NVS noch ohne aktivierte Verschluesselung. Status-/Kameraseite verlangen individuelles Digest-Login. Der alte AP und sein gemeinsames Passwort sind deaktiviert. Ein beim ersten Versuch erkannter Neustart durch Webserverstart ohne initialisierten Netzwerkstack wurde behoben: HTTP startet erst bei WLAN-Verbindung. Frischer USB-Test bestand: ungueltige Konfiguration abgewiesen, Mikrofon und Heartbeats laufen stabil. Sechs lokale Protokolltests einschliesslich Ausschluss von Zugangsdaten in Statusdateien bestanden.

Tobi hat WLAN-Daten direkt im lokalen Fenster eingegeben. Geraet bestaetigt verbunden und gespeichert, IP 192.168.43.31, Gateway 192.168.43.1. Tobi bestaetigt ausdruecklich: Hotspot ist zunaechst absichtlich gewaehlt. Der PC liegt weiterhin auf 192.168.0.23; Statusseite/JPEG waren vom PC aus im anderen Netz nicht erreichbar, die HTTP-Zugangstests sind daher nicht bestanden/verifiziert. Oeffentlicher Internetzugang wurde nicht getestet. Fuer spaetere lokale Uploads brauchen die Geraete eine erreichbare Verbindung; kein Wechsel ins Heim-WLAN gegen diese Entscheidung. Eine Heimrouter-Sperre gilt nicht fuer Betrieb am Hotspot. microSD-Verfuegbarkeit ist noch offen.

Privatheitsentwurf: eigene Firmware, keine Cloud-Automatismen, Internetzugang am Router fuer den Recorder sperren und lokale Ziele erlauben, spaeter TLS mit Serverpruefung sowie Medienverschluesselung/Backup. Die aktuelle HTTP-Testseite verschluesselt Inhalte nicht. Eigener Anwendungscode enthaelt derzeit keine ausgehenden Cloud-/Upload-Anfragen; vorkompilierte Treiber und Hardware sind damit nicht vollstaendig auditiert. Secure Boot/Flash-/NVS-Verschluesselung erst fuer stabile Fassung pruefen; keine irreversiblen eFuse-Aenderungen bei diesen Tests. Netzwerkverkehr muss am Router/AP kontrolliert werden, nicht nur am Ethernet-PC.

Verwandt: [[Live Up]], [[Regg]], [[JacobNAS]], [[Hardware und Laufwerke]].

### Präzisierung und Übergabe an Claude (05.10.2026)

Tobi bittet ausdrücklich, den gesamten bisherigen Stand und folgende neuen Vorgaben im Brain festzuhalten. Er möchte danach vermutlich mit Claude weiterarbeiten (im Diktat „Cloud“; keine bestätigte Entscheidung für Cloud-Verarbeitung). Zunächst weiter testen; aus dieser Nachricht folgt kein Auftrag, sofort neue Firmware zu flashen oder das vollständige Backend zu bauen.

- **Maximalmodus:** wirklich die maximal mögliche stabile Video- und Audioaufnahme gleichzeitig. Eine große SD-Karte ist vermutlich geplant, aber noch nicht als gekauft oder eingesetzt bestätigt; Größe und Modell offen. Eine größere Karte allein beweist keine höhere Bildrate. Auflösung, Bildrate, JPEG-Qualität und lückenloses Audio gemeinsam am Gerät messen, bevor feste Grenzen zugesagt werden. Die früher genannten 2/5/10 Bilder/s waren Teststufen, keine von Tobi gewählte Obergrenze.
- **Sparmodus:** später umschaltbar auf ausschließlich Audioaufnahme. Kameraaufnahme und unnötige Aktivität sollen dann entfallen; Einsparung und tatsächliche Laufzeit messen. Die Umschaltung darf Audio nicht stillschweigend unterbrechen.
- **Getrennte Medien:** Video und Audio als getrennte Dateien/Spuren verwalten, aber über gemeinsame Zeitstempel, Boot-/Aufnahme-IDs und Marker sauber zuordnen. Separat abspielen, analysieren und archivieren; bei Bedarf gemeinsam auf einer Zeitachse anzeigen.
- **Tragen/Laufzeit:** während des Herumlaufens tragen und aufnehmen. Kein zwingender 24-Stunden-Dauerbetrieb im Alltag; eventuell später bis zu 24 Stunden. Nachts laden ähnlich dem Handy, später über eine Ladehalterung/ein Dock. Akkugröße, Dauerbetrieb beim Laden, Power-Path und Dock-Verdrahtung weiterhin ungeprüft/offen; die früher korrigierten Ladeboard-Aussagen bleiben maßgeblich.
- **Objektgedächtnis am PC:** Tobi hat von Software gehört, die Videos und Gegenstände analysiert und sich Gegenstände merkt. Gewünschte Fragen beispielsweise: „Wo habe ich das hingelegt?“ oder „Was habe ich wo eingegeben?“ Noch keine konkrete Software ausgewählt, installiert oder getestet. Daraus kein garantiertes, lückenloses Gedächtnis ableiten.
- **Grundidee der Auswertung:** beobachtete Gegenstände, Ablage-/Benutzungsmomente und sichtbare Eingaben als zeitgebundene Ereignisse mit Belegbild und Verweis auf das Originalvideo speichern. Ergebnis soll „zuletzt gesehen“ mit Fundstelle/Unsicherheit sein. Außerhalb des Sichtfelds bewegte Objekte, unlesbare Texte oder verdeckte/ maskierte Eingaben sind nicht belegt. Zugangsdaten gehören weiterhin nicht in normale Brain-Zusammenfassungen.
- **Hervorhebungen bleiben:** manuell ausgelöste Zusatzfotos und Marker für Essen, Waage, Zutaten und wichtige Momente; nachts mit Audio-/Videokontext auswerten, später automatisch und korrigierbar in Live Up nachtragen. Fotoerkennung allein beweist weder genaue Grammzahl noch unsichtbare Zutaten.
- **Lokale Verarbeitung bleibt bevorzugt:** Chip erfasst/puffert/markiert, NAS archiviert, PC verarbeitet. Ob die Tagesmenge nachts mit der vorhandenen Hardware fertig wird, ist eine offene Messfrage. Datenschutzentwurf, Hotspot-Entscheidung und fehlende Verschlüsselung der Testversion bleiben wie oben beschrieben.

Vorgeschlagener Ablauf für das Objektgedächtnis (Plan, nicht implementiert): Aufnahme -> lokale Sicherung -> Szenen/Gegenstände und sichtbare Texte erkennen -> Ereignisse mit Zeit und Quelle speichern -> spätere Frage durchsuchen -> Antwort mit passender Videostelle und Unsicherheit. Für Kalorien zusätzlich Produkt/Rezept, gewogene Menge und tatsächlich gegessene Portion verbinden.

**Übergabe:** C:/Users/a/Documents/Codex/2026-10-05/schau-du-mal-im-brain-also/outputs/Recorder-Uebergabe.md. Der ausführliche Softwareplan liegt nebenan als Recorder-Softwareplan.md und ist um diese Vorgaben ergänzt.

**Stand zum Weiterarbeiten:** Nur der Sense ist bestätigt vorhanden, aktuell USB statt Steckplatine; Firmwareprojekt work/esp32-dv1 im genannten Codex-Ordner, PlatformIO/Arduino, COM6. DV1.4-Testfirmware mit Mikrofonpegel, Kamera-QVGA-Test, USB-WLAN-Einrichtung und eingebautem Digest-Anmeldeschutz. Letzter lokal gelesener WLAN-Status bestätigt saved/connected am bewusst gewählten Hotspot 192.168.43.31; PC 192.168.0.23. Kein erfolgreicher HTTP-/JPEG-Zugangstest vom PC in diesem Netz, kein Internet-Test. SD-Aufnahme, simultaner gespeicherter Audio-/Video-Dauertest, Markerfunktion, Upload, Objektgedächtnis und Live-Up-Automatik noch nicht gebaut. Vor neuem Flashen das lokale WLAN-Fenster schließen, falls es COM6 belegt. Keine Zugangsdaten in der Übergabe.

**Nächste Tests:** Karte bestätigen/prüfen -> kurze vollständige WAV-Datei -> gleichzeitige Audio-/Video-Dateien mit Sample-/Frame-/Fehlerzählern -> höchste stabile Qualitätsstufe bestimmen -> Markerfoto -> Audio-Sparmodus und Verbrauch messen -> lokalen Upload und Nachtverarbeitung testen. Neuinstallation oder grundlegender Neuaufbau der bereits vorhandenen PC-Seite ist dafür nicht notwendig.

### DV2.0 Lauftest-Firmware (05.10.2026, Claude)

Tobi hat eine 64-GB-microSD eingesetzt und wollte ein Testprogramm, mit dem er das Gerät an einer Powerbank herumtragen kann. Gebaut, geflasht und am Gerät geprüft; Einzelheiten und Messwerte in [[Recorder DV2 Lauftest]]. Kurzfassung:

- Strom dran = Aufnahme. Video (MJPEG-AVI, 1024×768, 10 Bilder/s) und Audio (WAV, 16 kHz) als getrennte Dateien in `/rec/NNNN/`, dazu Zeitstempel, Zähler und Protokoll. Nach 10 Minuten sauberer Abschluss und Tiefschlaf.
- Die ersten drei Punkte der „Nächsten Tests“ sind damit erledigt: Karte geprüft (64 GB geht, als FAT32), vollständige WAV-Datei, gleichzeitige Audio-/Video-Dateien mit Zählern. Ton war in allen Messungen lückenlos.
- Die Kamera ist ein OV3660 und liefert bis 1024×768 rund 27 Bilder/s, bei 1600×1200 noch 13,7. Die Karte schafft etwa 1600 KB/s. Die höchste dauerhaft stabile Stufe draußen ist damit noch nicht festgelegt.
- WLAN: JacobHome ist auf dem Gerät gespeichert, die Verbindung aber nicht bestätigt, weil das Board am Schreibtisch kein einziges Netz sieht (vermutlich Antenne nicht aufgesteckt). Die frühere Hotspot-Entscheidung hat Tobi am 05.10. selbst durch „Heim-WLAN JacobHome“ ersetzt.
- Projektordner jetzt `A:/Recorder/firmware/dv2`. Der Codex-Ordner mit DV1.4 bleibt unverändert als Stand davor.

## Hardware (Einkaufsliste mit Amazon.de-Links, Preise vom 26.09.2026, noch nicht bestellt)

| Teil | Link | Preis |
|---|---|---|
| Seeed Studio XIAO ESP32S3 Sense (nur über „Alle Angebote“) | https://www.amazon.de/dp/B0C69FFVHH | ca. 23 € |
| SanDisk Ultra microSD 128 GB | https://www.amazon.de/dp/B0B7NTY2S6 | 23,52 € |
| EEMB LiPo 3,7 V 1800 mAh 103450 (PCM-Schutz, UL, 30,6 g, 34,5×52×10,3 mm) | https://www.amazon.de/dp/B09DPL8RZH | 11,59 € |
| Aideepen TC4056 USB-C Ladeboard 1 A mit Schutz, 6 Stück | https://www.amazon.de/dp/B0BZSB3SBN | 6,99 € |
| Waveshare 1,54" E-Paper-Modul V2, 200×200, Teilaktualisierung | https://www.amazon.de/dp/B0728BJTZC | 19,99 € |
| optional: Mikrotaster 6×6 mm, 100 Stück | https://www.amazon.de/dp/B0F1JXSD7Y | 5,98 € |
| optional: Widerstände 10 Ω–1 MΩ + 5-mm-LEDs | https://www.amazon.de/dp/B07YWX42RJ | 12,99 € |

Zusammen ca. 85 €, mit den optionalen Kleinteilen ca. 104 €.

### Günstigere Variante (Recherche 27.09.2026)

Gleiche Aufnahme- und Upload-Funktion ist deutlich billiger möglich:

- XIAO ESP32S3 **Sense ohne Stiftleisten** bei Reichelt: 15,40 € statt ca. 23 € bei Amazon. Es ist weiterhin das Original mit Digitalmikrofon und microSD-Slot; Versand bei Reichelt ab 5,95 €.
- 32-GB-microSD Class 10 ab ca. 6,99 € reicht für ca. 11,6 Tage unkomprimiertes Audio und entspricht der offiziell unterstützten Maximalgröße. 64 GB (ca. 13 € im Angebot) ergeben ca. 23 Tage, müssen aber als FAT32 formatiert und praktisch getestet werden.
- Das E-Paper ist reine Komfortausstattung. Eine einzelne Status-LED plus Taster erfüllt Aufnahme/Pause/Upload-Anzeige ebenfalls und spart rund 20 €. Den detaillierten Status kann später eine kleine lokale Webseite auf dem Handy zeigen.
- LED, Taster und die drei benötigten Widerstände einzeln kosten zusammen typischerweise unter 2 €; die großen Amazon-Sortimente für zusammen rund 19 € sind für dieses eine Gerät unnötig.
- Ein einzelnes 1-A-Ladeboard kostet im Elektronikhandel etwa 0,80–2,20 € statt 6,99 € für sechs Stück. Bei der 0,80-€-Variante bleibt der geschützte EEMB-Akku wichtig; USB-C-Varianten mit zusätzlichem Ausgang und Schutz kosten etwas mehr.
- Beim am Körper getragenen Akku bleibt die Empfehlung beim geschützten EEMB-Modell. An dieser Stelle nur wenige Euro mit einer unbekannten Zelle zu sparen, lohnt das zusätzliche Risiko nicht.

Empfohlene Sparfassung ohne Display: etwa **36–43 € für die Teile** (XIAO Sense, 32/64-GB-Karte, EEMB-Akku, einzelnes Ladeboard und Kleinteile), zuzüglich gegebenenfalls Versand. Mit E-Paper kommen etwa 14–20 € hinzu. Ein nacktes ESP32-S3-Board plus separates Mikrofon und SD-Modul spart gegenüber dem XIAO Sense kaum Geld, wird größer und aufwendiger und ist deshalb nicht empfohlen.

### Händler- und Variantenprüfung (27.09.2026)

- **Reichelt ist kein Fakeshop:** deutsche `reichelt elektronik GmbH`, ladungsfähige Anschrift in Sande, Handelsregister `HRB 219874` beim Amtsgericht Oldenburg, stationärer Shop sowie deutsches 14-tägiges Widerrufsrecht. Die unmittelbaren Rücksendekosten trägt der Kunde. Öffentliche Bewertungen berichten teils über verspätete Lieferungen und schwachen Service; das ist ein Service-Risiko, aber kein Hinweis auf einen anonymen Scamshop. Für zusätzlichen Käuferschutz PayPal, Amazon Pay oder Klarna statt Vorkasse wählen.
- Der konkrete Reichelt-Artikel **XIAO ESP32S3 Sense, OV3660, ohne Header** ist die richtige Originalversion. Reichelts Hersteller-Teilenummer `113991115` stimmt exakt mit Seeeds offizieller SKU überein. Lieferumfang: XIAO ESP32S3, steckbare Sense-Erweiterungsplatine und Antenne. Auf der Sense-Platine sitzen digitales Mikrofon und microSD-Slot; die Kamera ist abnehmbar.
- **„Ohne Header/Stiftleiste“** bedeutet nur, dass die zwei Reihen langer Metall-Steckpins an den seitlichen Lötaugen nicht vormontiert sind. Das verändert die Elektronik nicht. Für dieses kompakte Gerät ist es sogar passend: Kabel zu Display, LED, Taster, Akkumessung und Stromversorgung werden direkt an die Lötaugen gelötet. Das Sense-Aufsteckboard verwendet einen separaten B2B-Steckverbinder und bleibt vollständig nutzbar.
- Die früh genannte Kleinstshop-Empfehlung HD Robotics für das Ladeboard wird zurückgezogen. Der Shop ist nicht als Betrug belegt und nennt Anschrift sowie Widerrufsrecht, seine Rechtstexte enthalten jedoch unbearbeitete Platzhalter und fragwürdige Formulierungen zur Rückgabe geöffneter Elektronik. Für die Ersparnis von wenigen Euro ist dieses zusätzliche Rückgaberisiko unnötig.
- **Ersatzangebot:** Aideepen TC4056 USB-C bei Amazon, 6 Stück, ASIN `B0BZSB3SBN`, zuletzt 6,99 €. Das Board ist für eine 3,7-V-Einzelzelle ausgelegt, lädt mit 1 A und besitzt Lade-/Entladeschutz sowie getrennte Anschlüsse `B+/B−` und `OUT+/OUT−`. Amazon ist hier wegen der einfacheren Rückgabe und des Plattform-Käuferschutzes die bevorzugte sofort verfügbare Alternative; Verkäufer und Rücksendehinweis vor dem Bestellen noch einmal prüfen. Reichelts einzelne passende Platine für 2,20 € ist laut Produktseite erst ab 31.10.2026 lieferbar.
- Bei diesen günstigen USB-C-Ladeboards zum Laden ein normales 5-V-USB-A-Netzteil mit USB-A-auf-USB-C-Kabel verwenden. USB-C-auf-USB-C bzw. Power Delivery funktioniert bei dieser einfachen Modulbauart unter Umständen nicht.

- Die Kamera des XIAO wird nicht gebraucht und kann abgesteckt bleiben.
- Nur die **Sense**-Version geht (Aufsteckboard mit Mikrofon + SD-Slot). Nicht geeignet: Heemol „für XIAO ESP32-S3“ (B0H33VMGJM, 15,98 €) – Nachbau ohne Sense-Board, Beschreibung widersprüchlich (nennt RISC-V, WLAN 6, Zigbee; das sind ESP32-C6-Merkmale), 1 Bewertung. Von Tobi am 26.09.2026 angefragt.
- microSD als **FAT32** formatieren (Windows kann das über 32 GB nicht, z. B. mit Rufus). Seeed gibt offiziell nur 32 GB an; größere FAT32-Karten laufen meist, wird beim Aufbau getestet.
- Korrektur: „High Endurance“ ist unnötig. Bei 2,76 GB/Tag wird die 128-GB-Karte nur ca. 8-mal im Jahr komplett beschrieben. SanDisk High Endurance 128 GB wäre https://www.amazon.de/dp/B07NY23WBG (35,99 €).
- Akku: EEMB statt Noname, weil er am Körper getragen wird (Schutzschaltung, UL-gelistet). 1800 statt 2000 mAh, gleiche Bauform.
- **Externes Ladeboard** ist nötig: Der XIAO lädt selbst nur mit ca. 100 mA (Seeed-Forum: 110 mA), die Aufnahme braucht schon ca. 55 mA. Mit 1 A ist der Akku in ca. 2 h voll. Der XIAO hängt an OUT+/OUT− des Ladeboards.
- Gehäuse aus dem 3D-Drucker: Loch direkt über dem Mikrofon, Öse für die Kette.

## Mikrofon

MEMSensing MSM261D3526H1CPM (Datenblatt V1.2): digital (PDM), omnidirektional, Schallöffnung oben, **Rauschabstand 64 dB(A)**, Empfindlichkeit −26 dBFS bei 94 dB SPL, übersteuert erst bei 120 dB SPL, 0,67 mA. Rauschabstand auf Smartphone-Niveau, besser als das beliebte INMP441 (61 dB). Normales Gespräch in 1 m (ca. 60 dB SPL) landet roh bei ca. −60 dBFS, also leise in der Datei; die Firmware verstärkt digital (Seeed-Beispiel ×4, Stellschraube). Nur 16 kHz läuft stabil (laut Seeed-Forum), passt. Praxis: über der Kleidung tragen (drunter dumpf + Reiben), Loch im Gehäuse direkt über dem Mikrofon. Nach den ersten Aufnahmen Pegel am PC messen und Verstärkung einstellen.

## Bedienung (Plan für die Firmware)

- **E-Paper-Display** (1,54"): Aufnahme/Pause, Uhrzeit, Akku in %, freier Platz auf der Karte (in Tagen), ungesichert seit, letzter Upload. Aktualisierung alle paar Minuten und bei Ereignissen; Strom nur beim Umschalten. Passt auf den Akku (Modul ca. 33×48 mm).
- **LED:** kurzes Aufblitzen alle paar Sekunden = nimmt auf, aus = Pause, schnell = lädt hoch. Die eingebaute LED des XIAO geht nicht, sie hängt an GPIO21 wie die SD-Karte (CS) und flackert mit.
- **Taster:** kurz = Lesezeichen (Moment wird im Transkript markiert), lang = Aufnahme pausieren/fortsetzen (z. B. wenn Besuch kommt).
- **Akkustand:** Der XIAO kann den Akku nicht selbst messen; zwei Widerstände (z. B. 2× 100 kΩ) als Spannungsteiler an einen ADC-Pin.
- Pins reichen: SD belegt D8–D10 + GPIO21, das Display teilt sich SCK/MOSI mit der SD-Karte und braucht 4 weitere Pins (CS, DC, RST, BUSY), dazu je 1 Pin für Taster, LED und Akkumessung (ADC nur auf D0–D5).

Verbrauch laut Seeed-Wiki: Mikrofonaufnahme ca. 55 mA im Schnitt, WLAN ca. 110 mA. Schätzung: 1000 mAh ≈ 18 h, 2000 mAh ≈ 1,5 Tage, 3000 mAh ≈ 2 Tage. Muss am echten Gerät gemessen werden.

## Datenmengen (unkomprimiert, 16 kHz, 16 bit, mono)

Die Größe hängt nicht vom Inhalt ab: 32 KB/s = **115 MB pro Stunde = 2,76 GB pro Tag**, ca. 1 TB pro Jahr. 128 GB Karte ≈ 46 Tage Puffer. NAS hatte am 26.09.2026 1,46 TB frei ≈ 17 Monate. Falls es eng wird: im Archiv als FLAC speichern halbiert ungefähr, ohne Qualitätsverlust. Nach dem ersten echten Tag messen: Akkulaufzeit, Upload-Dauer, wie viel davon Sprache ist (bestimmt die Rechenzeit fürs Transkribieren).

## PC-Seite (gebaut 26.09.2026)

`A:\Recorder\`: `recorder.py` (Empfang + Transkription + Brain), `start.bat` (minimiert, niedrige Priorität), `selftest.py`, `token.txt` (Gerätetoken, geheim, nicht ins Brain), venv in `.venv`, Modelle in `models\`.

- Protokoll: `PUT http://<PC>:8765/upload/JJJJMMTT-HHMMSS.wav` mit Headern `X-Token` und `X-SHA256` (Prüfsumme, kann das Gerät schon beim Aufnehmen berechnen). Der PC prüft die Summe, kopiert aufs NAS, liest von dort zurück und antwortet erst dann 200 mit der Prüfsumme. 400 = kaputt angekommen, 409 = gleicher Name mit anderem Inhalt auf dem NAS (wird nicht überschrieben), 503 = NAS nicht erreichbar. Ein doppelter Upload wird erkannt und nicht doppelt transkribiert. Die lokale Kopie in `inbox\` wird nach dem Transkribieren gelöscht.
- faster-whisper `large-v3-turbo`, int8, CPU, mit VAD (Stille wird übersprungen). Gemessen: 29 s Sprache in 29 s, also etwa Echtzeit auf dem Ryzen 5 1500X.
- Stimmen: sherpa-onnx mit CAM++-Modell (VoxCeleb). Angelernte Stimmen per `recorder.py enroll Name probe.wav` in `voices\`; Unbekannte werden „Person 1, 2 …“, pro Programmlauf durchnummeriert. Schwelle `SAME_VOICE = 0.5` an echten Aufnahmen nachjustieren.
- Transkripte: `Transkripte/JJJJ-MM-TT.md` im Vault, geschrieben über `brainctl put`, mit Link zur Audiodatei pro Stück. Der Ordner wird nicht in den Startkontext geladen.
- Fehlgeschlagene Stücke landen in `failed\` (Audio bleibt erhalten, zum Wiederholen zurück nach `inbox\`).
- Selbsttest (Wegwerf-Vault, zwei Windows-Stimmen, Mitternachtswechsel) bestanden.

## Offen

- Getrennte Audio-/Videoaufnahme auf SD gibt es seit DV2.0 ([[Recorder DV2 Lauftest]], 05.10.2026); vorher war nur die Testfirmware DV1.4 vorhanden. Es fehlen noch: Nur-Audio-Sparmodus, Marker, Abgleich mit der echten Uhrzeit und Upload, evtl. Upload beim nächtlichen Laden.
- Antenne am XIAO aufstecken und WLAN prüfen; erste Aufnahme von draußen auswerten und danach Auflösung/Bildrate festlegen. Hardware ist inzwischen da; der frühere Eintrag „Firmware schreiben, sobald Hardware da ist“ war der damalige Stand.
- Windows-Firewall: Port 8765 für private Netzwerke freigeben (macht Tobi selbst).
- Autostart von `start.bat`.
- Tobis Stimme anlernen (ca. 30 s saubere Sprache).
- Wenn im Zimmer YouTube/Musik läuft, transkribiert Whisper das mit. Filter später, z. B. nur Abschnitte mit bekannter Stimme behalten.
- Tägliches Auswerten der Transkripte in `Personal/` und `Knowledge/`.

Verwandt: [[Regg]], [[Brain System]], [[Hardware und Laufwerke]], [[Tobi]]

## Preis- und Händlerprüfung (29.09.2026)

Recherche auf Tobis Wunsch nach besserem Preis-Leistungs-Verhältnis und gründlicher Prüfung der Shops. Noch keine Bestellung oder neue Kaufentscheidung. Preise inkl. MwSt., Versand Deutschland separat:

- Reichelt XIAO ESP32S3 Sense ohne Header (113991115): 15,40 EUR, lieferbar. https://www.reichelt.de/de/de/shop/produkt/xiao_esp32s3_sense_wifi_bt_kamera_ov3660_ohne_header-358353
- Reichelt Waveshare 12955, 1,54 Zoll, 200x200, schwarz/weiß, Modul mit SPI: 14,50 EUR, lieferbar. Exakte V2-Revision im Angebot nicht eindeutig zugesichert, vor Kauf bestätigen. https://www.reichelt.de/de/de/shop/produkt/entwicklerboards_-_display_epaper_1_54_schwarz_weiss-224225
- Reichelt Intenso Performance 32 GB: 8,95 EUR, lieferbar. https://www.reichelt.de/de/de/shop/produkt/microsdhc-speicherkarte_32gb_intenso_class_10_uhs-1-319398
- Alternative Intenso Performance 128 GB: 22,95 EUR. https://www.reichelt.de/de/de/shop/produkt/microsdxc-speicherkarte_128gb_intenso_class_10_uhs-1-319400
- Reichelt Versand 5,95 EUR je Bestellung. Drei Teile mit 32 GB geliefert: 44,80 EUR.
- EREMIT geschützter LiPo 1800 mAh 103450: 6,20 EUR plus 3,69 EUR Versand = 9,89 EUR, lieferbar 2–5 Tage. https://www.eremit.de/p/eremit-3-7v-1800mah-lipo-akku-103450
- EREMIT geschützter LiPo 2000 mAh 654060: 6,89 EUR plus Versand = 10,58 EUR; 60x40x6,4 mm, andere Bauform, 11 Prozent mehr Nennkapazität. https://www.eremit.de/p/eremit-3-7v-2000mah-lipo-akku-654060
- Vier Teile mit 32 GB und 1800 mAh inklusive beider Versandkosten: 54,69 EUR OHNE Ladeelektronik. Mit 128 GB: 68,69 EUR. Amazon-Ausgangspreise (85,09 EUR für fünf Positionen) stammen vom Nutzer, aktuelle Marketplace-Verkäufer nicht verifiziert.

Händlerbefund: Reichelt hat nachvollziehbare GmbH-Identität, HRB 219874 Oldenburg, Anschrift, 14-Tage-Widerruf und geschützte Zahlungswege; Rücksendekosten beim Käufer. EREMIT nennt Daniel Beck, Auf der Platt 12, 65594 Runkel, USt-ID DE312188501, WEEE/Batterieregistrierungen, 14-Tage-Widerruf und PayPal/Karte. Name/Adresse/USt-ID stimmen mit dem langjährigen eBay-Händler eremit_de überein. Das stützt die Händleridentität, ist keine unabhängige Zellprüfung oder Garantie vollständiger Rechtskonformität. Quellen: https://www.reichelt.de/de/de/shop/service/-12_42 ; https://www.reichelt.de/de/de/shop/service/-12_52 ; https://www.eremit.de/l/contact ; https://www.eremit.de/l/withdrawal ; https://www.eremit.de/l/shipping ; https://www.ebay.de/usr/eremit_de

### Wichtige Korrektur zur Ladeelektronik

Die frühere pauschale Empfehlung eines TP4056/TC4056 mit OUT-Anschlüssen für Weiteraufnahme während des Ladens war nicht ausreichend geprüft. Schutzschaltung und OUT-Anschlüsse belegen kein geeignetes Power-Path/Load-Sharing. Bei komputer.de kostet ein TP4056-USB-C-Modul mit Schutz 0,20 EUR plus 3,50 EUR Versand, der Verkäufer fordert jedoch ausdrücklich das Abtrennen der Last während des Ladens. Deshalb NICHT als fertige Lösung für den gewünschten Dauerbetrieb empfehlen. Geeignete Versorgung mit gleichzeitigem Laden muss vor Bestellung separat festgelegt werden. Quelle: https://www.komputer.de/zen/index.php?main_page=product_info&products_id=622

komputer.de nennt Michael Bauer, Aresing, USt-ID und PayPal, jedoch veraltete Widerrufstexte (BGB-InfoV) und alten EU-OS-Link; nur eingeschränkt empfehlenswert, kein Betrugsnachweis. Reichelts 2,20-EUR-Lader erst ab 31.10.2026; EREMIT-Lader ab 1 EUR in sämtlichen überprüften Varianten ausverkauft. BerryBase-Display 13,90 EUR plus 4,95 EUR Versand lohnt bei gemeinsamer Reichelt-Bestellung nicht. makerklang.shop mit 10-EUR-Display ausgeschlossen: auf geprüften Seiten kein eindeutig benannter Rechtsträger und auffällige Rabatt-/Verknappungsanzeigen; Betrug nicht bewiesen.

32 GB entsprechen Seeeds offizieller Größenangabe und rechnerisch etwa 11,6 Tagen 16-kHz/16-bit/Mono-Rohaufnahme vor Dateisystemreserve. 128 GB liegen außerhalb dieser offiziellen Angabe und benötigen FAT32 sowie einen Praxistest. https://wiki.seeedstudio.com/xiao_esp32s3_sense_filesystem/
### Ergänzung: Amazon ausdrücklich einbeziehen (29.09.2026)

Tobi möchte ausdrücklich den günstigsten passenden Gesamtwarenkorb über alle möglichen Shops, Amazon ist genauso erwünscht. Keine Festlegung auf Fachhändler.

Direkt im Amazon-Browser bestätigt: Intenso microSDXC Class 10 128 GB, ASIN B088MK26WL, 19,90 EUR, auf Lager, Verkauf und Versand Amazon, Rückgabeanforderung innerhalb 14 Tagen laut Kaufbox. Kostenloser Versand wurde nur für qualifizierte Erstbestellung angezeigt, daher nicht allgemein voraussetzen. https://www.amazon.de/dp/B088MK26WL

Damit Board und Display Reichelt inkl. Versand 35,85 EUR + EREMIT 1800 inkl. Versand 9,89 EUR + Amazon128 19,90 EUR = 65,64 EUR OHNE Ladeelektronik und zuzüglich eventuell anfallendem Amazon-Versand. Gegenüber der bisherigen 128-GB-Auswahl 3,05 EUR weniger, sofern Amazon versandfrei. 32-GB-Intenso bei Amazon inzwischen 8,99 EUR, nicht der veraltete Preisvergleichspreis 6,99 EUR; Reichelt32 8,95 EUR bleibt im gemeinsamen Warenkorb günstiger. Amazon zeigte daneben Intenso64 zu 9,99 EUR, Verkäufer dieses Angebots nicht separat geprüft. Lexar-Angebot um 11 EUR aus Preisvergleich nicht direkt bestätigt, nicht empfehlen.

Zusätzlich Botland, Hersteller-Direktshops, eBay/Idealo und AliExpress-Suchergebnisse berücksichtigt. Botland-Display 10,90 EUR plus mindestens4,99 EUR Versand ist einzeln teurer als das Reichelt-Display als Mitbestellung14,50 EUR. AliExpress-Lockpreise ohne verifizierte Sense-Variante, Händler und Endpreis nicht als bestätigte Ersparnis werten. Recherche ist kein Beleg für den absolut niedrigsten Preis im gesamten Internet.
## Aideepen-Lader konkret geprüft (30.09.2026)

Tobi hat den vollständigen Amazon-Artikeltext des Aideepen TC4056 USB-C, sechs Stück, bereitgestellt. Auch dieses konkrete Angebot fordert ausdrücklich: Beim Laden die Last am OUT-Anschluss trennen. Damit ist jetzt nicht nur ein vergleichbares Modul, sondern die bisher verlinkte Amazon-Variante selbst für die gewünschte Weiteraufnahme während des Ladens als alleinige Lösung ausgeschlossen. Frühere pauschale Empfehlung war falsch.

Laut Angebot für eine einzelne 3,7-V-LiPo/Li-Ion-Zelle mit 4,2-V-Ladeschluss; bis 1 A Ladestrom. Freigabe dieses Stroms für den konkreten EEMB1800 noch am Akkudatenblatt prüfen; 1800 mAh allein ist keine Ladestromfreigabe. Gewählte Variante hat Lötpads statt JST-Buchse. XIAO besitzt BAT-Anschlüsse für 3,7-V-Akku und eigene Ladeschaltung; externe Laderegler nicht ungeprüft parallel betreiben. Keine fertige Verdrahtung freigegeben. Quelle: vom Nutzer eingefügter Amazon-Artikel B0BZSB3SBN und https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/

### Eigene Feldaufnahmen: neue Bedienwünsche (05.10.2026, Codex)

Auswertung von 0021/0022 siehe [[Recorder Feldaufnahmen Auswertung]]. Aus dem lokal automatisch erkannten eigenen Bericht:

- Spätere App-Verbindung und weniger Handy-/Social-Media-Nutzung erneut bekräftigt; keine bereits funktionierende Verbindung behauptet.
- Gedanken zu LCD/Display mit **Restzeit bis Neustart**, **Akkuanzeige** und **Stoppknopf** (0021 WAV ca. 08:53–09:46). Sagt, noch Arduino-Zeug zu haben; konkrete Teile nicht erkennbar.
- Befestigung vor dem Körper und richtige/schräge Ausrichtung weiter offen; die Feldbilder zeigen überwiegend Gesicht/Hals/Himmel. Das Sichtfeld auf Hände/Objekte ist für das gewünschte Objektgedächtnis wichtiger als bloß mehr gespeicherte Bilder.
- Die damalige Entscheidung „kein Taster“ wird durch den neuen genannten Stoppknopf ergänzt; noch keine gebaute oder endgültig gewählte neue Bedienung. Software auf ausdrücklichen Wunsch unverändert gelassen.
- JSON für Kalorien-App und Geldguru in 0022 verlangt; Prüfdateien erzeugt, kein automatischer Import. Betrag/Rechnung aus Ton, exakte Portion und Zahlungsweg offen.

[[Recorder Testaufnahmen 0021-0022]] enthält die vollständige automatische Arbeitsfassung mit Unsicherheiten; [[Müllermilch Geschmackseindrücke]] die Geschmacksvorlieben. Für künftige ASR-Auswertung: Textwiederholung erkennen und ohne Fortsetzungskontext erneut prüfen, Hintergrundwerbung/Automatensprache nicht als Nutzerauftrag behandeln.

## Neue Zielrichtung: nur Audio, Firmware AR1 (06.10.2026, Claude)

Tobi hat am 06.10.2026 verbindlich umgestellt: **Kamera vorerst raus, zuerst ein robuster Audio-Recorder.** Der ESP32 sammelt, das Handy steuert per Bluetooth LE (als Modul in [[Live Up]]) und überbrückt unterwegs, der PC archiviert und transkribiert, das Brain bekommt nur Verweise. Die frühere Vorgabe „maximal Video und Audio gleichzeitig“ gilt damit vorerst nicht mehr. Tobi hat erlaubt, die Daten auf der SD-Karte zu löschen (noch nicht gelöscht).

- **Spezifikation und Phasenplan:** `A:/Recorder/docs/Audio-Recorder-V1.md` (Fassung 2, von vier unabhängigen Prüfern gegengelesen). Testvektoren für das BLE-Protokoll: `A:/Recorder/docs/ar1-proto-vectors.json`.
- **Am Gerät geprüft:** Antenne steckt. Die gespeicherten Zugangsdaten für JacobHome funktionieren (192.168.0.112, −70 bis −77 dBm, am Schreibtisch schwach; einzelne Anmeldeversuche scheitern und klappen beim nächsten Mal). Scan findet 4 Netze, Statusseite vom PC in 0,3 s erreichbar, Uhrzeit per NTP vom Router. Die Kamera ist abgezogen.
- **Firmware AR1.0** liegt in `A:/Recorder/firmware/ar1` (DV2 bleibt unverändert als Rückfallstand) und ist **geflasht**: nur Audio, 10-Minuten-WAV-Segmente in `/rec/NNNNNN/`, Journal `log.txt`, daraus erzeugte `info.json`, SHA-256 je Segment, Pause/Fortsetzen, Timer, Tiefschlaf, eigener Recorder-Task. Baut ohne Warnungen (866 KB).
- **Erster Gerätetest bestanden:** 70 s mit 20-Sekunden-Segmenten, 4 Segmente, SHA-256 am PC gleich `info.json`, Mikrofon-Samples gleich Datei-Samples, Uhr passt, kein Sample verworfen, Ringpuffer höchstens 0,4 %.
- **Werkzeuge:** `tools/ar1.py` (USB-Befehle, Download), `tools/check_ar1.py` (prüft eine Aufnahme), `tools/test_ar1.py` (vor jedem Flashen), `tools/ar1proto.py` (Referenz fürs BLE-Protokoll).
- **Noch nicht eingearbeitet** (Befunde der Prüfer, stehen in der Spezifikation): Nach einem Absturz oder Watchdog-Neustart nimmt der geflashte Stand wieder auf, auch wenn vorher gestoppt wurde; Prüfsumme kurzer Segmente erst im Leerlauf nachtragen; Zeitquelle `rtc`; Fehlerpfade und Testschalter. Die Tests T3 bis T8 stehen noch aus.
- **Noch nicht gebaut:** BLE-Dienst in der Firmware (Phase 2), das Recorder-Modul in Live Up (Sicherung des Quellstands unter `A:/LiveUp-work/_basis-recorder`, an `A:/LiveUp` ist nichts geändert), Web-API und Hub (Phase 3), Handy als Brücke (Phase 4), Strom (Phase 5).
- Am Handy ist Bluetooth ausgeschaltet; für den BLE-Test muss Tobi es einschalten.
- Die Sitzung endete am Nutzungslimit mitten in der Arbeit.

### Stand 07.10.2026 (Claude)

- Die Befunde der Prüfer sind eingearbeitet und am Gerät getestet (Stand AR1.0): Segmente, Prüfsummen, Pause, hartes Zurücksetzen mit Wiederherstellung, Timer, Tiefschlaf, simulierter Kartenfehler, volle Karte. Wer stoppt oder pausiert, wird nach einem Absturz nicht wieder aufgenommen.
- Über Nacht lief die Aufnahme 19,6 Stunden ohne Sampleverlust. Einzelheiten und die dabei gefundenen Schwächen in [[Recorder AR1 Firmware]].
- Die Bluetooth-Fassung AR1.1 ist seit dem 07.10.2026 auf dem Gerät. Das Recorder-Modul in [[Live Up]] 0.9.0 ist gekoppelt und steuert den Recorder: Start, Pause, Stopp, Timer, Netzsuche und Dateiliste sind am Handy geprüft.
- Noch nicht gebaut: Web-API zum Abholen und der PC-Hub (Phase 3), Handy als Brücke (Phase 4), Strom (Phase 5).
- Die SD-Karte ist seit dem 07.10.2026 leer: erst die alten DV2-Ordner und kurzen Tests, dann auf Tobis ausdrücklichen Wunsch alle Aufnahmen. (Vorher stand hier: noch nicht gelöscht.) Neu formatieren geht so: Karte am Handy oder PC als exFAT formatieren, in den Recorder stecken, dann `python ar1.py FORMAT YES`; der Recorder formatiert nur Karten, die er nicht lesen kann, und macht daraus FAT32.
- Tobi hat am 07.10.2026 um einen Plan für die PC-Seite gebeten (Phase 3: abholen, archivieren, transkribieren, im Brain verknüpfen). Der Plan steht in `A:/Recorder/docs/Audio-Recorder-V1.md`, Abschnitte 3.8, 4 und 10.

### Hardware für die Kette (07.10.2026, Claude)

Tobi will das Gerät jetzt alltagstauglich als Kette bauen; das Gehäuse druckt er selbst. Der Plan mit Teileliste, Verdrahtung und Quellen steht in [[Recorder Hardware Kette]]. Zwei Korrekturen zu älteren Angaben in dieser Notiz:

- Der eingebaute Lader des XIAO **Sense** lädt nur mit 50 mA, nicht mit rund 100 mA (das gilt für die Plus-Variante). Bei 64,5 mA Verbrauch während der Aufnahme wird der Akku daran nicht voll. Deshalb eine eigene Ladeplatine mit Power-Path (Vorschlag: Adafruit bq24074, Artikel 4755); der XIAO hängt an deren Ausgang.
- Im Tiefschlaf zieht der Sense laut Seeed 3 mA, nicht nur Mikroampere.

### Stand 09.10.2026 (Claude)

- Die Ladeplatine Adafruit bq24074 und ein JST-PH-Kabel-Kit sind von Eckstein gekommen, der LiPo ist noch unterwegs. Schaltplan und Lötplan in Stufen stehen in [[Recorder Hardware Kette]].
- Firmware AR1.2 mit Akkuspannung, „Strom liegt an“ und „lädt“ ist gebaut, aber noch nicht aufgespielt: [[Recorder AR1 Firmware]].
- Das Gehäuse will Tobi danach mit dem 3D-Drucker machen.
