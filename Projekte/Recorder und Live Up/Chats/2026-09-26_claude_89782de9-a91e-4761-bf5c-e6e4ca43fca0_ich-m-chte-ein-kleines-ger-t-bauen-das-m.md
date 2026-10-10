---
titel: "Ich möchte ein kleines gerät bauen das mir 24/7 audio aufnimt und des dann wenn "
datum: 2026-09-26
ki: claude
session: 89782de9-a91e-4761-bf5c-e6e4ca43fca0
tags: [chat]
---

# Ich möchte ein kleines gerät bauen das mir 24/7 audio aufnimt und des dann wenn 

## Verlauf

**Tobi:** Ich möchte ein kleines gerät bauen das mir 24/7 audio aufnimt und des dann wenn es im internet ist auf mein handy oder pc hochläd wo es transkripiert6 wird und dann in ein brain kommt

**Claude:** Ich trage das Projekt noch in den Index ein, dann kommt der Plan.

**Claude:** Gute Idee, und sie passt zu dem, was du schon hast: faster-whisper aus [Regg](Knowledge/Regg.md), die RTX 3080 und das Brain. Vorher aber ein Problem, das du kennen musst.

## Das rechtliche Problem

Nach **§ 201 StGB** ist es strafbar, ohne Einwilligung aufzunehmen, was andere nicht öffentlich sagen. Das gilt zu Hause, in der Schule und bei Freunden. Strafbar ist schon die Aufnahme selbst. Späteres Löschen oder Herausfiltern der fremden Stimmen ändert daran nichts. Wenn das Gerät wirklich 24/7 läuft, nimmt es ständig deine Familie und deine Mitschüler auf.

Mein Vorschlag deshalb: **Das Gerät nimmt auf, wenn du den Taster drückst, und eine LED zeigt die laufende Aufnahme an.** Dauerbetrieb nur, wenn du allein bist oder alle Anwesenden Bescheid wissen und einverstanden sind. Die Hardware bleibt dabei gleich, nur die Firmware verhält sich anders. Du kannst die Teile also schon bestellen, bevor das entschieden ist.

## Aufbau

**Gerät (ca. 30–40 €)**
- **Seeed XIAO ESP32S3 Sense**: Mikrofon, microSD-Slot und die Ladeschaltung für den Akku sind schon auf der Platine, Größe etwa wie ein Daumennagel
- 32-GB-microSD, kleiner LiPo-Akku (500–1000 mAh), ein Taster, ein Gehäuse (3D-Druck oder kleine Box)

**Firmware**
- Nimmt 16 kHz mono in 5-Minuten-Stücken auf die SD-Karte auf. Das sind etwa 115 MB pro Stunde, 32 GB reichen also für rund 11 Tage ohne Upload.
- Stille wird übersprungen. Die Pegelschwelle muss man einmal mit dem echten Mikrofon einstellen.
- Die Uhr wird bei jeder WLAN-Verbindung neu gestellt, weil sie sonst mit der Zeit abweicht.
- Im Heim-WLAN schickt das Gerät die Dateien an den PC und löscht sie erst, wenn der PC den Empfang bestätigt. Ist der PC aus, bleiben sie auf der Karte.
- Der Akku hält realistisch etwa einen Tag, du müsstest also täglich laden.

**PC**
- Ein kleiner Python-Empfänger nimmt die Dateien an, faster-whisper transkribiert sie auf der 3080 (deutlich schneller als Echtzeit).
- Das Audio kommt nach `A:` oder aufs NAS, weil `C:` fast voll ist.
- Die Transkripte kommen in einen eigenen Vault-Ordner, zum Beispiel `Transkripte/`. Der wird nicht in jeden KI-Start geladen, sonst ist der Kontext sofort voll.
- Einmal am Tag liest eine KI (Claude oder dein lokales Ollama) die Transkripte und schreibt die Fakten in `Personal/` und `Knowledge/`.

**Ohne Bauen:** Ein altes Android-Handy mit Aufnahme-App erledigt dasselbe. Es gibt auch ein fertiges Open-Source-Gerät, Omi.

Ich habe das Projekt als [Audio-Recorder](Knowledge/Audio-Recorder.md) angelegt und im Index verlinkt.

## Offene Fragen (je ein Satz reicht)

1. **Aufnahmemodus:** Taster, dauerhaft nur wenn du allein bist, oder etwas anderes?
2. **Tragen oder hinstellen:** Am Körper (Clip, Kette) oder stationär im Zimmer?
3. **Werkzeug:** Hast du einen Lötkolben und Zugang zu einem 3D-Drucker?
4. **Upload:** Nur im Heim-WLAN, oder auch unterwegs über den Hotspot deines Handys? Das Zweite ist deutlich mehr Aufwand, weil dann dein PC von außen erreichbar sein muss.
5. **Rohaudio:** Nach dem Transkribieren löschen oder behalten?
6. **Soll ich die PC-Seite schon bauen?** Empfänger, Transkription und Ablage ins Brain hängen nicht von der Hardware ab, das kann ich sofort machen.

**Tobi:** <pasted_content id="cf9d">
Kann sein, aber es ist nicht illegal, vierundzwanzig sieben aufzunehmen, wenn es nun in meinem Zimmer ist. Außerdem du musst mal schauen, also erstens, ich würde das mit dem Trasse drücken und dann Aufnahme nicht machen, soll wirklich vierundzwanzig sieben aufnehmen und keine Ahnung, ne Akkulaufzeit von ewig haben, damit ich das irgendwann vielleicht nachts oder so wirklich anstecken kann während des nimms vermutlich auch auf genau, es soll dann irgendwann um null Uhr null soll es eine neue Aufnahme machen und dann schickt es im Prinzip an meinen Server oder Lads auf DNAS hoch, mein PC sucht dann immer nach und keine Ahnung irgendwie sowas dann erreicht so ein Mini Akku für so lange Laufzeit genau auf Fragen der Auftreibermodus machen wir nicht, machen wir ja dauerhaft dann ich werde das vermutlich tragen an der Kette, solange ich halt im im in meinem Denkspin in meinem Zimmer oder bei mir zu Hause dann Werkzeuge, ich habe Lötkolm, ich habe drei D Drucker genau, also hab ich Upload im Heimwlan oder unterwegs über den Hotspot eines Handys weiter ist deutlich mehr Aufwand, weil dein PC von außen erreichbar sein muss, es geht über ein Wireguard Zugang oder was auch immer, aber das macht man dann später dann Ru Audio. Ich würde sagen, die Ro Audio wird bei mir halt zu Hause auf einer Festplatte aufbewahrt, aber Transkript ist halt auch da. Ich würde übrigens falls es irgendeinen Model kann oder so dann diese vierundzwanzig Stunden Aufnahmen gern auch stimmen trennen, wenn es geht, falls mein Oder drin ist oder was auch immer, also dass es das erkennt, dass zwei Stimmen sind oder mehrere soll ich die PC Seite schon bauen noch ja okay doch bau mal die anderen Dinger muss ich ja dann erstmal bestellen oder ich würde das glaube ich auch ohne Omni machen, weil Omni ist doch teuer also zumindest glaube ich, dass die teuer waren der Vices waren teuer auf jeden Fall
</pasted_content id="cf9d">

**Claude:** Du hast entschieden: dauerhaft aufnehmen. Dann baue ich jetzt die PC-Seite. Zuerst schaue ich nach, was auf dem PC schon installiert ist.

**Claude:** Die Grafikkarte ist eine **GTX 980 mit 4 GB**, keine RTX 3080 (die stand nur in einer alten Ollama-Anleitung). Das ändert die Modellwahl. Ich prüfe noch CPU und RAM.

**Claude:** Das passt schon in etwa, nur die Grafikkarte ist schwächer als gedacht. Bevor ich etwas ins Brain schreiben lasse, schaue ich mir an, wie `brainctl` Ordner abgleicht und speichert.

**Claude:** Testaudio und Modell sind da. Jetzt schreibe ich das Programm.

**Claude:** Jetzt kommt der Selbsttest, der einen Wegwerf-Vault nutzt. Die Testaufnahme ist auf 23:59:58 datiert, damit auch der Tageswechsel um Mitternacht mitgeprüft wird.

**Claude:** Die GTX 980 klappt nicht: Die Whisper-Bibliothek unterstützt diese Kartengeneration nicht mehr (`invalid device ordinal`). Ich entferne die 2 GB CUDA-Pakete wieder, es bleibt bei der CPU.

**Claude:** CPU also. Jetzt kommen noch die Startdatei und die Brain-Notizen.

**Claude:** Jetzt aktualisiere ich die Projektnotiz mit den Entscheidungen und dem Stand.

**Claude:** Die PC-Seite ist fertig und getestet. Die Firmware für das Gerät schreibe ich, sobald die Teile da sind.

**Zur Rechtslage:** Wenn du allein in deinem Zimmer bist, ist das in Ordnung. Sobald jemand reinkommt und redet, werden aber auch seine Worte aufgenommen. Wenn du deiner Familie einmal sagst, dass du das Ding trägst, bist du auf der sicheren Seite.

**Zum Akku:** Endlos hält so ein kleiner Akku nicht. Realistisch ist etwa ein Tag, die echte Laufzeit messe ich, wenn das Gerät läuft. Dein Plan geht trotzdem auf: nachts einstecken, und es nimmt beim Laden weiter auf.

## Was auf dem PC läuft (`A:\Recorder`)

- **[recorder.py](A:/Recorder/recorder.py)** wartet auf Aufnahmen vom Gerät, schreibt sie mit Whisper ab und trennt die Stimmen. Das Ergebnis landet pro Tag in `Transkripte/2026-09-26.md` im Brain, bei jedem Abschnitt steht ein Link zur Audiodatei.
- Das Gerät löscht eine Datei erst, wenn der PC den Empfang bestätigt hat. Wird dieselbe Datei zweimal geschickt, wird sie nicht doppelt abgeschrieben.
- Um 0 Uhr beginnt ein neuer Tag, und ein Stück, das über Mitternacht läuft, wird auf beide Tage aufgeteilt.
- **Stimmen:** Angelernte Stimmen erscheinen mit Namen, unbekannte als „Person 1“, „Person 2“ und so weiter.
- **Selbsttest bestanden.** Ich habe mit zwei Windows-Stimmen ein Gespräch über Mitternacht simuliert. Die Sätze wurden richtig abgeschrieben, beide Stimmen richtig auseinandergehalten und auf die zwei Tage verteilt.
- **Geschwindigkeit:** Deine GTX 980 wird von der Whisper-Bibliothek nicht mehr unterstützt, deshalb rechnet die CPU. Sie schafft etwa Echtzeit, also 1 Stunde Reden in 1 Stunde. Stille wird übersprungen, das reicht also. Die RTX 3080 deines Bruders habe ich als spätere Option notiert.

## Was du machen musst

1. **Bestellen**, ca. 40 €:
   - Seeed Studio **XIAO ESP32S3 Sense**. Es muss die Sense-Version sein, nur die hat Mikrofon und SD-Slot.
   - microSD **32 GB „High Endurance“**
   - LiPo 3,7 V mit **ca. 1000 mAh und Schutzschaltung**
2. **Firewall freigeben**, einmal in PowerShell als Administrator:
   ```powershell
   New-NetFirewallRule -DisplayName "Audio-Recorder" -Direction Inbound -Protocol TCP -LocalPort 8765 -Profile Private -Action Allow
   ```
3. **Deine Stimme anlernen:** Nimm ca. 30 Sekunden mit der Windows-Sprachaufzeichnung auf, dann:
   ```bash
   /a/Recorder/.venv/Scripts/python.exe /a/Recorder/recorder.py enroll Tobi "C:/Pfad/zur/aufnahme.m4a"
   ```
   Mit Familienmitgliedern geht das genauso, sie erscheinen dann mit Namen statt als „Person 1“.

Gestartet wird alles mit `A:\Recorder\start.bat`. **Soll ich die Datei in den Autostart legen**, damit der PC immer bereit ist, wenn er an ist?

Später kommt vielleicht noch ein Problem: Wenn in deinem Zimmer YouTube oder Musik läuft, schreibt Whisper das mit ab. Einen Filter baue ich, wenn die ersten echten Aufnahmen da sind.

Im Brain habe ich [Audio-Recorder](Knowledge/Audio-Recorder.md), [Hardware und Laufwerke](Personal/Hardware%20und%20Laufwerke.md) und [Tobi](Personal/Tobi.md) aktualisiert: deine Entscheidungen, deine PC-Hardware, die 3080 deines Bruders, Lötkolben und 3D-Drucker.

**Tobi:** <pasted_content id="cf9d">
Okay folgendes Überdenkmal hat es ein Mikrofon zweitens Wir brauchen eine größere SD Karte und drittens gibt es eine größeren Lipu, den wir beteilt das Gehäuse irgendwie größer oder irgendwie sowas oder es wird gewichtiger an irgendeiner Kette oder sowas, aber trotzdem gibt es was Besseres und folgendes soll die Records erst löschen, nachdem sie ja auf der Nass sind und nachgeprüft ist, ob es wirklich drauf ist und halt wenn wenn kein Speicherplatz mehr auf der Karte ist, dann wird das gelöscht oder irgendwie sowas, wir müssen dann Rhythmus entwickeln. Aber schau mal, wenn wenn wir den ersten Tag durch haben im Prinzip und wir müssen mal schauen, wie das mit der Audio aussieht, wie viel also im Prinzip wie wie lang wie viel Audio ist also in Gigabytes, wie wie groß die Dateien werden so you know genau
</pasted_content id="cf9d">

**Claude:** Ich prüfe zuerst die Hardware-Daten (Mikrofon, SD-Grenze, Ladestrom) und wie viel Platz auf dem NAS frei ist.

**Claude:** Auf dem NAS sind 1,46 TB frei. Jetzt die Datenblätter.

**Claude:** Den Selbsttest passe ich an: Er prüft jetzt auch beschädigte Uploads, die Ablage auf dem Archivlaufwerk und den Fall „gleicher Name, anderer Inhalt“.

**Claude:** Test bestanden. Jetzt prüfe ich noch, ob das echte NAS beschreibbar ist und das Zurücklesen dort funktioniert.

**Claude:** Das NAS funktioniert. Jetzt aktualisiere ich die Projektnotiz mit den neuen Entscheidungen.

**Claude:** Ja, der XIAO ESP32S3 Sense hat ein Mikrofon. Die Aufnahmen werden jetzt erst dann als gesichert gemeldet, wenn sie nachweislich auf dem NAS liegen. Beim Laden gibt es aber ein Problem, das die Einkaufsliste ändert.

## 1. Mikrofon
Ein digitales Mikrofon sitzt auf dem Sense-Aufsteckboard. Die Kamera auf demselben Board brauchst du nicht, du kannst sie einfach abstecken.

## 2. Größere SD-Karte
Nimm eine **128-GB-Karte der Sorte „High Endurance“**, die ist für Dauerschreiben gebaut. Seeed gibt offiziell nur 32 GB an. Größere Karten laufen meistens, wenn sie als **FAT32** formatiert sind. Windows kann FAT32 über 32 GB nicht selbst formatieren, das geht zum Beispiel mit Rufus. Ob deine Karte läuft, testen wir beim Aufbau, notfalls nimmst du 32 GB.

## 3. Größerer Akku
Ja, geht. Ich empfehle **2000 mAh** (ca. 10×34×50 mm, ca. 38 g). Wenn dir das Gewicht an der Kette nichts ausmacht, nimm 3000 mAh (ca. 55 g).

Das Problem: Der XIAO lädt selbst nur mit etwa 100 mA, die Aufnahme verbraucht aber schon rund 55 mA. Beim Laden nachts käme fast nichts im Akku an. Deshalb kommt ein **TP4056-Ladeboard mit USB-C (1 A, ca. 2 €)** dazu. Damit ist ein 2000-mAh-Akku in 2–3 Stunden voll. Das ist die Verbesserung, nach der du gefragt hast: ein stärkerer Chip hilft hier nicht, ein richtiges Ladeboard schon.

Laut Datenblatt schätze ich die Laufzeit so:

| Akku | Laufzeit (geschätzt) |
|---|---|
| 1000 mAh | ca. 18 h |
| 2000 mAh | ca. 1,5 Tage |
| 3000 mAh | ca. 2 Tage |

Die echte Laufzeit messen wir am fertigen Gerät.

## 4. Erst löschen, wenn es geprüft auf dem NAS liegt
Das habe ich eingebaut und getestet:
- Das Gerät schickt zu jeder Datei eine Prüfsumme mit.
- Der PC legt die Datei unter `//jacobnas/JacobNAS/Tobi/Recorder/audio/` ab, liest sie vom NAS zurück und bestätigt erst, wenn alles übereinstimmt.
- Kommt eine Datei beschädigt an oder ist das NAS nicht erreichbar, gibt es keine Bestätigung, und das Gerät behält die Datei.
- Liegt auf dem NAS schon eine andere Datei mit gleichem Namen, wird sie nie überschrieben.

Der Selbsttest ist bestanden, und das echte NAS lässt sich beschreiben und zurücklesen.

Eine Einschränkung: Windows kann beim Zurücklesen aus seinem Zwischenspeicher lesen statt direkt von der NAS-Festplatte. Übertragungsfehler fängt die Prüfung trotzdem ab, einen Schreibfehler auf der NAS-Platte selbst nicht zwingend.

Für die Karte schlage ich diesen Rhythmus vor:
- Gesicherte Dateien bleiben als zweite Kopie auf der Karte.
- Erst wenn sie voll wird, löscht das Gerät die ältesten gesicherten Dateien.
- Ist die Karte trotzdem voll, weil lange nichts hochgeladen wurde, wird die älteste ungesicherte Datei überschrieben, damit die Aufnahme weiterläuft.

Das baue ich so in die Firmware ein, außer du willst es anders.

## 5. Wie groß die Dateien werden
Das lässt sich schon vorher genau ausrechnen, weil unkomprimiertes Audio immer gleich groß ist, egal was drauf ist:

| Zeitraum | Größe |
|---|---|
| 1 Stunde | 115 MB |
| 1 Tag | **2,76 GB** |
| 1 Jahr | ca. 1 TB |

- Die 128-GB-Karte reicht für etwa **46 Tage**, bevor sie voll ist.
- Auf dem NAS sind 1,46 TB frei, das reicht für etwa **17 Monate**.
- Wird es eng, kann man das Archiv als FLAC speichern. Das halbiert die Größe ohne Qualitätsverlust.

Nach dem ersten echten Tag messen wir die Dinge, die man nicht ausrechnen kann: Akkulaufzeit, wie lange der Upload dauert und wie viel davon wirklich Sprache ist. Davon hängt ab, wie lange dein PC mit dem Abschreiben beschäftigt ist.

## Einkaufsliste (ca. 45 €)
- Seeed Studio **XIAO ESP32S3 Sense**
- microSD **128 GB High Endurance**
- LiPo 3,7 V **2000 mAh mit Schutzschaltung**
- **TP4056 USB-C Ladeboard mit Schutzschaltung**

Im Brain ist alles in [Audio-Recorder](Knowledge/Audio-Recorder.md) eingetragen. Die Frage nach dem Autostart von `start.bat` ist noch offen.

Quellen:
- [Seeed Wiki: XIAO ESP32S3 Getting Started](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/) (Mikrofon, SD-Grenze 32 GB, Verbrauch)
- [Seeed-Forum: ESP32S3 Battery charger](https://forum.seeedstudio.com/t/esp32s3-battery-charger-automatic/292989) (Ladestrom ca. 110 mA)
- [Instructables: TP4056 mit XIAO ESP32S3](https://www.instructables.com/Li-ion-Cell-Charger-With-TP4056-and-XIAO-ESP32S3/)

**Tobi:** <pasted_content id="cf9d">
Okay, suche mir auf Amazon oder irgendwie sowas die Links raus für die Dinge, die ich holen soll mal such mal wie wie laut die Audio circa ist, weil man muss ja auch was aufnehmen, also es muss ja auch was ankommen so und auch nicht irgendeinen Rauschen oder was auch immer so eine muss ja wirklich was ankommen wie gut die Audio ist, wie laut sie ist genau und kannst noch irgendwie sowas wie ein Lichtchen oder keine Ahnung irgendwie sowas hinzufügen, wo ich dann weiß, dass es aufnimmt, dass Katlet, wo einfach den Status anzeigt oder ein Bildschirm oder irgendwie sowas, wo ich dann auch mit reinbauen kann. So wenn Bildschirm dann so ein so ein Kindle like wie heißen diese Bildschirme, die so im Prinzip nur Akku verbrauchen, wenn die geupdated wird so circa und vielleicht ein Button oder irgendwie sowas, keine Ahnung Control Panel, was auch immer irgendwie sowas einfach mitnehmen
</pasted_content id="cf9d">

**Claude:** Ich schaue mir zuerst an, wie gut das eingebaute Mikrofon wirklich ist, und suche danach die Links.

**Claude:** Das Mikrofon-Datenblatt habe ich. Jetzt suche ich die Teile auf Amazon.de.

**Claude:** XIAO gefunden (ca. 23 €, nur über „Alle Angebote“). Weiter mit der Speicherkarte.

**Claude:** EEMB-Akku mit Schutzschaltung und UL-Zertifizierung, 30 g. Jetzt das Ladeboard.

**Claude:** Ladeboard gefunden (6er-Pack, 7 €). Jetzt das E-Paper-Display.

**Claude:** Das Display ist das Original von Waveshare, Version 2 mit Teilaktualisierung. Jetzt noch Kleinteile: Taster, LED und Widerstände.

**Claude:** Alle Links sind zusammen. Jetzt trage ich Einkaufsliste, Mikrofonwerte und Bedienkonzept ins Brain ein.

**Claude:** Hier sind die Links. Das Mikrofon ist gut genug, und Display, LED und Taster passen alle noch mit ins Gerät. Bestellt habe ich nichts.

## Einkaufsliste (Amazon.de, Preise von heute)

| Teil | Preis |
|---|---|
| [Seeed Studio XIAO ESP32S3 Sense](https://www.amazon.de/dp/B0C69FFVHH) | ca. 23 € |
| [SanDisk Ultra microSD 128 GB](https://www.amazon.de/dp/B0B7NTY2S6) | 23,52 € |
| [EEMB LiPo 1800 mAh 103450](https://www.amazon.de/dp/B09DPL8RZH) | 11,59 € |
| [USB-C-Ladeboard 1 A mit Schutz, 6 Stück](https://www.amazon.de/dp/B0BZSB3SBN) | 6,99 € |
| [Waveshare 1,54" E-Paper V2](https://www.amazon.de/dp/B0728BJTZC) | 19,99 € |
| *optional:* [Taster 6×6 mm, 100 Stück](https://www.amazon.de/dp/B0F1JXSD7Y) | 5,98 € |
| *optional:* [Widerstände + LEDs](https://www.amazon.de/dp/B07YWX42RJ) | 12,99 € |

Zusammen **ca. 85 €**, mit den optionalen Kleinteilen ca. 104 €. Die optionalen Teile brauchst du nur, wenn du keine Taster, LEDs und Widerstände rumliegen hast.

Zur Auswahl:
- **XIAO:** Gibt es auf Amazon nur über „Alle Angebote“, also von Drittanbietern.
- **Speicherkarte, Korrektur:** „High Endurance“ brauchst du doch nicht. Die Karte wird nur etwa 8-mal im Jahr komplett vollgeschrieben, das hält die normale Ultra locker aus und spart 12 €.
- **Akku:** Ich habe EEMB statt eines Noname-Akkus genommen, weil du ihn am Körper trägst. Er hat eine Schutzschaltung und eine Sicherheitszertifizierung (UL). Er hat 1800 statt 2000 mAh, aber dieselbe Größe (34×52×10 mm, 31 g).
- **Display:** Das Original von Waveshare in Version 2. Es kann auch nur einen Teil des Bildes neu zeichnen, das spart Strom.

## Wie gut und wie laut das Mikrofon ist
Ich habe das Datenblatt gelesen:

| Eigenschaft | Wert |
|---|---|
| Rauschabstand | **64 dB(A)** |
| Richtung | nimmt rundum auf |
| Übersteuert ab | 120 dB, also erst bei sehr lautem Schall |

Ein Rauschabstand von 64 dB ist so gut wie bei einem Handy-Mikrofon und besser als beim beliebten Bastel-Mikrofon INMP441 (61 dB). Normale Gespräche kommen sauber an, und das Eigenrauschen stört nicht.

Ein normales Gespräch in 1 m Entfernung ist in der Rohdatei ziemlich leise. Deshalb verstärkt die Firmware das Signal digital, das macht auch das Beispiel von Seeed so. Nach den ersten echten Aufnahmen messe ich die Pegel am PC und stelle die Verstärkung richtig ein.

Für die Praxis:
- **Über der Kleidung tragen.** Darunter klingt alles dumpf und raschelt.
- Das Gehäuse braucht ein kleines Loch **direkt über dem Mikrofon**.
- Laut einem Nutzer im Seeed-Forum läuft das Mikrofon nur mit 16 kHz Abtastrate stabil. Das ist genau die Rate, die wir sowieso benutzen.

## Status-Anzeige und Bedienung
Du meinst **E-Paper** oder **E-Ink**, also die Technik aus dem Kindle. Das Display braucht nur Strom, wenn sich das Bild ändert. Das 1,54-Zoll-Display ist ungefähr so groß wie der Akku, beides lässt sich also gut übereinander ins Gehäuse bauen.

- **Display:** zeigt Aufnahme oder Pause, Uhrzeit, Akkustand in %, wie viele Tage noch auf die Karte passen, seit wann etwas ungesichert ist und wann zuletzt hochgeladen wurde.
- **LED:** blitzt alle paar Sekunden kurz auf, solange aufgenommen wird. Aus heißt Pause, schnelles Blinken heißt Upload. Die LED, die schon auf dem XIAO sitzt, geht nicht, weil sie am selben Pin wie die SD-Karte hängt und nur mitflackert. Deshalb kommt eine eigene LED dazu.
- **Taster:**
  - kurz drücken: **Lesezeichen**, der Moment wird im Transkript markiert
  - lang drücken: **Pause** oder weiter, zum Beispiel wenn Besuch kommt
- **Akkustand:** Den kann der XIAO nicht selbst messen. Dafür braucht es zwei Widerstände, die sind im optionalen Set dabei.

Ich habe nachgezählt: Die Pins des XIAO reichen für SD-Karte, Display, Taster, LED und Akkumessung.

Einkaufsliste, Mikrofonwerte und Bedienkonzept stehen in [Audio-Recorder](Knowledge/Audio-Recorder.md). Wenn die Teile da sind, schreibe ich die Firmware und sage dir, wie du alles verdrahtest.

Quellen:
- [Datenblatt MSM261D3526H1CPM](https://files.seeedstudio.com/wiki/XIAO-BLE/mic-MSM261D3526H1CPM-ENG.pdf)
- [Seeed Wiki: Mikrofon am XIAO ESP32S3](https://wiki.seeedstudio.com/xiao_esp32s3_sense_mic/)
- [Seeed-Forum: Mikrofon-Probleme bei anderen Abtastraten](https://forum.seeedstudio.com/t/xiao-sense-esp32s3-mic-struggling-with-getting-clear-samples-using-arduino-i2s-api-and-mic-noise/276944)

**Tobi:** geht des au oder gibts da ne regel? https://www.amazon.de/dp/B0H33VMGJM?th=1

**Claude:** Das Brain war kurz von einer anderen Sitzung gesperrt, ich versuche das Speichern noch einmal.

**Claude:** Die Änderung war noch nicht drin, weil der erste Versuch schon vorher abgebrochen war. Ich trage sie jetzt nach.

**Claude:** **Nein, das geht nicht.** Das ist die nackte ESP32-S3-Platine ohne das Sense-Aufsteckboard. Es fehlen also genau die beiden Teile, die wir brauchen: **das Mikrofon und der SD-Karten-Slot**. Auch unter den anderen Varianten auf der Seite ist keine Sense-Version (nur ESP32-S3, C3, C6).

Außerdem ist das kein Original von Seeed, sondern ein Nachbau der Marke Heemol mit bisher nur einer Bewertung. Die Beschreibung widerspricht sich: Sie nennt RISC-V, WLAN 6 und Zigbee. Das sind Eigenschaften des ESP32-C6, der ESP32-S3 hat nichts davon. Offenbar wurde der Text von einer anderen Variante kopiert, und man weiß nicht sicher, was man bekommt.

**Die Regel:**
1. Im Namen muss **„Sense“** stehen.
2. In der Beschreibung müssen **Kamera, digitales Mikrofon und SD-Karten-Slot** erwähnt sein. Die Kamera ist ein gutes Erkennungszeichen, denn nur die Sense-Version hat eine.
3. Am besten von **Seeed Studio** selbst.

Man könnte die nackte Platine nehmen und Mikrofon und SD-Slot einzeln dazulöten. Das spart etwa 7 €, macht das Gerät aber größer, und das übliche Einzelmikrofon (INMP441) ist schlechter als das auf dem Sense-Board. Lohnt sich nicht.

Bleib also bei diesem Link: [Seeed Studio XIAO ESP32S3 Sense](https://www.amazon.de/dp/B0C69FFVHH) (ca. 23 €, über „Alle Angebote“). Den Nachbau habe ich im Brain als ungeeignet vermerkt.

## Erkenntnisse

_Noch keine kuratierte Zusammenfassung._
