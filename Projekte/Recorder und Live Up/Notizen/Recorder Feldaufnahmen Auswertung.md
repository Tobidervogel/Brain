---
titel: Recorder Feldaufnahmen Auswertung
tags: [recorder, analyse, audio, video]
erstellt: 2026-10-05
aktualisiert: 2026-10-05
---

# Recorder-Auswertung: die eigenen Testaufnahmen 0021 und 0022

Auswertung am 05.10.2026. Quelle: die von Tobi per Handy nach `C:/Users/a/Documents/rec` übertragenen Dateien. Die zusätzliche Aufnahme 0023 entstand beim vorherigen USB-Ausleseversuch und wurde ebenfalls geprüft. Recorder-Firmware, Recorder-PC-Software und Medienoriginale wurden nicht bearbeitet.

## Ergebnis und Reichweite

**0021 ist eine sauber abgeschlossene 10-Minuten-Aufnahme. 0022 ist abgeschnitten, aber 2.221 vollständige Bilder und 3:42,656 Minuten Ton sind erhalten.** Nur der letzte begonnene JPEG-Block ist unvollständig. Für beide existieren gemeinsam abspielbare MP4-Prüfkopien; die gesicherten Originaldateien bleiben daneben erhalten.

Der Inhalt zeigt den Weg zum EDEKA, Pfandrückgabe, Aufenthalt im Markt, den Kauf einer Müllermilch laut Erzähler, das anschließende Trinken und die Rückkehr in einen Innenraum. Daneben spricht der Erzähler über Recorder-Bedienung, spätere App-Anbindung, weniger Handynutzung und die gewünschte Auswertung. Seine Testanweisung lautet: **hell yeah, baby**.

Die Bildsichtung umfasst **alle 23 geordneten Übersichten mit ungefähr zwei Sekunden Bildabstand sowie gezielte Einzelbilder**, nicht eine Sichtung jeder einzelnen Handlung bei voller Bildrate. Zusätzlich wurden sämtliche vollständig gespeicherten JPEGs technisch decodiert. Kurze Vorgänge außerhalb der Stichprobe oder außerhalb des Kamerasichtfelds können fehlen.

Der Ton wurde **lokal automatisch mit faster-whisper large-v3-turbo** transkribiert und die erkannten Texte wurden geprüft. Direkte akustische Wahrnehmung steht diesem Assistenten hier nicht zur Verfügung: Dies ist kein von Hand vollständig abgehörtes Wortprotokoll. Unsichere Erkennungen bleiben markiert. Modellwahrscheinlichkeiten sind keine Garantie für den tatsächlichen Wortlaut. In 0021 wurde ein fehlerhafter erster Lauf ab etwa 3:32 durch einen zweiten Lauf ohne Übernahme des vorherigen Textkontexts ersetzt; dieser liefert wieder sinnvolle Sprache statt einer falschen langen „So“-Schleife.

## Medienbestand

| Aufnahme | Vollständige Bilder | Vorhandener Ton | Abschluss | Einordnung |
|---|---:|---:|---|---|
| 0021 | 5.961 | 599,520 s | `time`, 600 s | Eigener vollständiger Feldtest |
| 0022 | 2.221 | 222,656 s | fehlend | Eigener abgebrochener Feldtest |
| 0023 | 2.062 | 206,496 s | `usb`, 207 s | Zusätzlicher Lauf beim Auslesen |

1024 × 768, MJPEG, Ziel 10 Bilder/s; getrenntes Mono-PCM mit 16 kHz / 16 Bit. Alle Dateien aus dem übergebenen Ordner wurden als bytegleiche Kopien gesichert und per SHA-256 verglichen. Der frühere PC-Kartenleser meldete tatsächlich „No Media / 0 Byte“; der spätere Zugriff erfolgte auf die von Tobi benannte lokale Kopie. Die Herkunft der Dateien wurde nicht aus einem vermeintlich funktionierenden Kartenleser erfunden.

**Zeitbasis:** Alle folgenden Audio-Fundstellen sind ungefähre automatisch geschätzte Satzzeiten in Minuten/Sekunden ab Anfang der WAV-Nutzdaten. Für 0021 sind etwa **0,463 s** zu addieren, um zur gemeinsamen Aufnahme-/MP4-Zeit zu gelangen; für 0022 ungefähr **0,462 s**. Bildzeiten aus 0021/0023 beziehen sich auf den Aufnahmestart. 0022-Bildzeiten sind mangels `frames.csv` geschätzt. Kalenderdatum folgt dem heutigen Nutzerauftrag; die Aufnahmen selbst enthalten keine echte Uhrzeit (`time` leer). Zwischen den Aufnahmen ist die Dauer der Unterbrechung unbekannt.

## Chronologie: 0021

| Fundstelle | Inhalt / Beobachtung | Herkunft und Grenze |
|---|---|---|
| Ton 00:03–00:20 | Testet, ob es funktioniert; hofft, dass die Powerbank nicht abschaltet. | Sprache automatisch erkannt; genauer Nebensatz über den Powerbank-Auslöser unklar. Keine Messung der Powerbank. |
| Bild 00:00–00:14 | Innenraum, Türbereich, dann draußen. Kamera anfangs gedreht, Gesicht und Decke/Himmel nah. | Sichtbeobachtung. |
| Ton 00:23–00:34 | Ziel EDEKA, viele Pfandflaschen abgeben; „Ich hab Bock“. | ASR „Pfannflaschen“ kontextuell als Pfandflaschen normalisiert. |
| Ton 00:36–00:45 | Nennt es eigenes Testvideo Nummer 1 nach Claudes Aufnahmen; sagt, die alten Tests gelöscht zu haben. | Selbstauskunft. Codex hat keine alten Tests gelöscht. |
| Ton 00:46–01:16 | Überlegt, wie der Recorder vor ihm hängen soll, richtige bzw. schräge Ausrichtung; spricht ihn wie eine Person an. Erwähnt Pfandrückgabe und „Money“. | Sprache; Bild kippt um etwa 00:50 in überwiegend aufrechte Lage. |
| Ton ca. 00:59 | Erste Erkennung meldet „Da ist ein Kater“. | Zweiter Lauf bestätigt den Ausdruck nicht separat; kein klarer Katzennachweis im Bild. |
| Ton 01:24–01:36 | Wünscht Auswertung durch Claude oder Chatty. Ein nur im ersten Lauf erkannter Satz fragt möglicherweise nach verbrannten Kalorien. | Die Kalorienfrage ist sprachlich unsicher; vorsichtige Szenariorechnung weiter unten. |
| Ton 01:39–02:07 | Recorder soll später mit seinen Apps verbunden sein. Will Handy weniger nutzen und weniger auf Social Media scrollen; findet das Projekt gut. | Klare inhaltliche Selbstaussagen, keine bereits funktionierende App-Verbindung. |
| Ton 02:17–02:35 | Spricht über Aufnahmezeit, erlöschende Lampe und nötigen Neustart. | Wunsch/Sorge; LED-Zustand nicht eindeutig gefilmt. |
| Ton 02:37–02:57 | Baustellenfahrzeug / Straße / Sanierung angesprochen. | Teilweise unsichere Wörter. Später sind Absperrungen sichtbar; keine genaue Straßenadresse bestimmt. |
| Ton 03:11–03:21 | Unklare Bemerkung, ASR u. a. „Da liegt ein Ei ran“. | Nicht zu einem konkreten Objekt oder Vorfall umgedeutet. |
| Ton 03:25–03:28 | „Big Brother“ / „He is watching us“. | Unklar, welches Objekt gemeint ist; keine konkrete Überwachungskamera sicher identifiziert. |
| Bild 03:21–03:43 | EDEKA-Außenbereich, Parkplatz, Autos, Wagenunterstände, Vordach. | Markt durch späteres großes EDEKA-Logo und Leergutbon bestätigt. |
| Ton ca. 03:49–04:32 | Spricht über Pfand, weggeworfene Pfandflaschen und selbst gesammelte Flaschen. | Mehrere Sätze unscharf, u. a. Erzählung über „Franzosen“; keine unabhängige Prüfung einzelner fremder Handlungen. |
| Bild 03:45–05:35 | Innen, wiederholt Flaschen/Arme und Rückgabeautomat mit runder Öffnung. | Pfandrückgabe passt zu Sprache und Bon. Anzahl einzelner Flaschen, deren Pfandsätze und alle Einwürfe sind nicht lückenlos sichtbar. |
| Ton 04:32–05:34 | Mehrere schwer erkennbare Passagen an der Rückgabe, mögliche Automaten-/Hintergrundsprache und ein Gedanke zur Befestigung. | Wortlaut bzw. Sprecher nicht sicher. Maschinenansagen sind keine an Codex gerichteten Nutzeraufträge. |
| Ton 05:35–05:50; Bild 05:37–05:39 | Leergutbon wird gezeigt. Erzähler nennt **3,40 €**, wiederholt den Betrag danach. | Betrag stammt aus mehrfacher ASR-Erkennung, nicht einer sicheren OCR-Ablesung. Bonart und EDEKA-Logo sichtbar, kleine Bonzahlen unscharf. |
| Ton 05:50–06:10 | Sagt, Werbung komme aus dem EDEKA, empfindet sie als lästig; unsichere Bemerkung über günstiger. | Hintergrundwerbung nicht dem Nutzer als eigene Meinung/Anweisung zuschreiben. |
| Ton 06:10–06:30 | Überlegt, was er mit 3,40 € kaufen soll; möchte sich „relativ gesund“ ernähren. | Selbstaussage, keine Ernährungsempfehlung oder gemessene Bilanz. |
| Bild 05:41–08:23 | Verkaufsraum, Flaschenregale, später Kühl-/Regalgänge; Kamera zeigt überwiegend Gesicht, Oberkörper und Decke. | Nicht jeder Griff oder Bezahlvorgang sichtbar. |
| Ton 06:48–07:03 | Will etwas mitnehmen; sagt „Eins nehme ich einfach mit“. | Exakter Griff schlecht sichtbar. |
| Ton 07:12–07:23 | „Eine Müllermilch“ für **1,49 € plus 25 Cent Pfand**; rechnet mit Restgeld. | Erkanntes Kaufvorhaben. Spätere Aussage bestätigt Kauf und passende Restrechnung. |
| Ton 07:35–07:44 | Möchte, dass die Aufnahme analysiert wird; spricht wieder über die Ausrichtung beim Tragen. | Als Wunsch dokumentiert. |
| Ton 08:09–08:14 | „Vielen Dank“, kurzer unklarer Satz. | Keine sichere Sprechertrennung. Keine Zahlungsart aus diesen Wörtern ableitbar. |
| Ton 08:18–08:29 | Sagt, eine Müllermilch gekauft zu haben, und **1,66 € plus** geblieben. | 3,40 − (1,49 + 0,25) = 1,66; inhaltlich konsistent. |
| Ton 08:31–08:39 | Will das später in Geldguru eintragen. | Name im ASR abweichend, über vorhandenen App-Kontext als Geldguru zugeordnet. |
| Bild 08:25–08:43 | Ausgang, draußen, schwarzer rechteckiger Gegenstand nahe der Kamera, großes EDEKA-Schild. | Gegenstand könnte Geldbörse/Telefon sein; weder Modell noch lesbare Eingabe bestimmt. |
| Ton 08:41–08:45 | Bemerkung über die Straße/etwas Verbotenes. | Nicht als rechtliche Aussage oder sichere genaue Situation übernommen. |
| Ton 08:53–09:46 | Aufnahme läuft noch; denkt über LCD/Display, verbleibende Zeit bis Neustart, Akkuanzeige und Stoppknopf nach. Sagt, noch Arduino-Zeug zu haben. | Neue Hardware-/Bedienwünsche; nicht umgesetzt, weil Softwareänderungen ausdrücklich ausgeschlossen sind. |
| Bild 09:19–09:59 | Müller-Flasche mit weißem befestigtem Deckel, orangebraunem Etikett/Keksabbildung. Deckel wird benutzt, bei 09:37 Trinken sichtbar. Absperrung und Verkehrszeichen im Hintergrund. | Marke sichtbar. Exakte Sorte aus Ton besser bestimmt; Menge/Rest nicht messbar. |
| Ton 09:47–09:59,5 | Nennt **Schoko-Cookie-Geschmack**; vergleicht ihn mit Cookie-Eis und bewertet ihn negativ. | Satz wird am automatischen 10-Minuten-Ende abgeschnitten und in 0022 wieder aufgegriffen. |

## Chronologie: 0022

| Fundstelle | Inhalt / Beobachtung | Herkunft und Grenze |
|---|---|---|
| Ton 00:00–00:12 | Aufnahme sei kurz beendet worden; wiederholt Schoko-Cookie-Vergleich mit Cookie-Eis, negative Bewertung. | Das Ende des Vergleichssatzes ist nicht sicher erkannt. |
| Ton 00:18–00:39 | Vergleicht eine Zimtschnecken-Sorte mit Müller-Milchreis, findet sie schlecht, ist von Müller enttäuscht und erinnert sich an besseren Geschmack. | Das zusätzliche Wort vor „Zimtschnecken“ ist unsicher. Erwähnung beweist keinen zweiten heutigen Kauf/Verzehr. |
| Ton 00:39–01:01 | Persönliche Erinnerung an frühere Klassenlehrerin, Klassenstufe und Süßigkeiten. | Erkennung dieser Erzählung unsicher; kein Name und kein überprüfter historischer Vorfall. Im Transkript als unsicher erhalten. |
| Ton 01:01–01:31 | Stracciatella-/Cookie-Eis als Vergleich, „chemisch“ als Geschmackseindruck; manche Müllermilch-Sorten mag er, viele nicht. | Geschmacksvorliebe, keine chemische Analyse oder Sicherheitsbewertung. |
| Bild ca. 00:56–01:06, 01:14–01:16, 01:40, 02:12–02:14, 02:30–02:38 | Mehrere sichtbare Trinkvorgänge aus derselben im Bild geführten Flasche. | Mengen pro Schluck und vollständiges Leeren nicht sicher bestimmt. Übersichtzeiten nur ungefähr. |
| Ton 01:32–01:36; Bild ca. 01:34 | Sagt, früher habe dort ein Kirschbaum gestanden. Kamera zeigt Garten, Rasen, Zaun und Nadelbaum. | Historischer Kirschbaum ist Selbstauskunft, nicht aus heutigem Bild bewiesen. |
| Ton 01:37–02:12 | Zweifelt, ob Claude/Chatty transkribieren. Auftrag: Wenn transkribiert, soll die Antwort **„hell yeah, baby“** enthalten. | Als direkt an den Auswerter gerichtete Nutzeranweisung berücksichtigt. |
| Ton 02:21–02:28 | Fragt sich, ob die Datei beim Abziehen kaputtgeht. | Technische Prüfung zeigt tatsächlich unvollständigen Schluss, aber vorhandene Daten lesbar. |
| Bild ca. 02:40–03:10 | Übergang durch Glas-/Türbereich nach innen, Möbel/Regale, Wandblatt/Kalender, Treppenkonstruktion. | Kalenderdruck zu klein zum verlässlichen Abschreiben. |
| Ton 02:41–03:13 | Fordert **JSON für Kalorien-App und Geldguru**, Einnahme und Ausgabe jeweils separat/insgesamt. | Aus dieser Anweisung wurden Prüfentwürfe und nachvollziehbare Rechenwerte erstellt. |
| Bild ca. 03:12–03:42 | Zimmer mit Holz-Möbel und violettem gemustertem Vorhang; Kopf/Oberkörper nah. | Keine lesbare Computer-/Handyeingabe. |
| Ton 03:18–03:31 | Will zuerst GPT testen, danach gegebenenfalls Claude. Kündigt Abstecken an; „Tut mir leid“. | Announcement passt zum abgeschnittenen Dateiende. Der tatsächliche elektrische Abbruchmoment ist nicht vollständig beweisbar. |
| Ton bis 03:42,656 | Weitere PCM-Daten erhalten, kein verlässlicher zusätzlicher Schlussauftrag erkannt. | Nicht als elf Sekunden garantierte absolute Stille bezeichnet. |

## Zusätzliche Aufnahme 0023

Aufnahme vom USB-Ausleseversuch: meist feste, schräg nach oben gerichtete Zimmeransicht mit Deckenleuchte, Holz-Möbel, geöffneter Tür und Stoff/Kleidung. Eine Person setzt um 00:24 einen schwarzen Bügelkopfhörer mit Mikrofonarm auf; danach oft Finger an Mund/Kinn. Kein Display oder konkret betätigtes Gerät sichtbar. Die automatische Erkennung liefert keine verständlichen Wörter. Das beweist keine absolute Stille. Der Lauf wurde mit dem vorhandenen STOP beendet; es war keine Firmwareänderung.

## Geldguru: getrennte Einnahme und Ausgabe

| Posten | Betrag | Nachweis |
|---|---:|---|
| Pfandrückgabe | **+3,40 €** | 0021 Ton 05:35–05:50 und wieder 06:10; Leergutbon im Bild |
| Müllermilch | −1,49 € | 0021 Ton 07:12–07:16 |
| Neuer Flaschenpfand | −0,25 € | 0021 Ton 07:17–07:21 |
| Einkauf insgesamt | **−1,74 €** | Summe beider Kaufpositionen |
| Differenz / übrig | **+1,66 €** | Rechnung und ausdrücklich erkannt bei 08:26–08:29 |

3,40 € ist die Pfandgutschrift, 1,74 € der vollständige Einkaufsbetrag einschließlich neuem Pfand. 1,66 € ist die Differenz und **keine zusätzliche dritte Einnahme**. Die Erwähnung der Zimtschnecken-Sorte wird nicht als zweiter Einkauf gebucht. „Plus“ ist die Beschreibung des Erzählers, kein berechneter Geschäftsgewinn: Auch die Herkunft früher bezahlter Pfandbeträge ist unbekannt.

Die Beträge sind gut durch die erkannte Sprache und ihre interne Rechnung gestützt. **Zahlungsweg ist nicht sicher erkannt.** Bonverrechnung und Auszahlung des Restbetrags in Bargeld sind naheliegend, bleiben aber eine ausdrücklich gekennzeichnete Einordnungsannahme. Deshalb gibt es ein Befund-JSON mit offenem Zahlungsweg und separat eine klar beschriftete Bargeld-Vorlage für die App-Vorschau. Nichts wurde in Live Up importiert, kein Konto- oder Bargeldbestand verändert.

## Kalorien: Produktdaten und tatsächliche Trinkmenge getrennt

Im Film sind eine Müller-Flasche und mehrere Schlucke sichtbar; im Ton wird Schoko-Cookie ausdrücklich genannt. Die aktuellen Herstellerdaten für **Müllermilch Original Schoko-Cookie** nennen **423 g / 400 ml je Pfandflasche** und pro **100 g**: 69 kcal, 3,4 g Eiweiß, 10,1 g Kohlenhydrate, 1,6 g Fett. Für eine ganze solche Flasche ergibt das rechnerisch **291,87 kcal**, 14,382 g Eiweiß, 42,723 g Kohlenhydrate und 6,768 g Fett. Quelle: [Molkerei Müller, Schoko-Cookie](https://allesmuelleroderwas.de/produkte/muellermilch/muellermilch-original-in-der-flasche/schoko-cookie-geschmack), geprüft 05.10.2026. Herstellerwerte beziehen sich auf g, nicht einfach auf ml; 400 ml wurden deshalb nicht als 400 g gerechnet. Verpackungsangaben können von der Website abweichen.

**Das ist der Nährwert einer ganzen Standardflasche, keine Messung der tatsächlich getrunkenen Portion.** Weder eine genaue Gramm-/Milliliterangabe noch ein verlässlicher Nachweis des restlosen Leerens konnte aus den geprüften Erkennungen/Bildern gewonnen werden. Die JSON-Datei führt die tatsächliche Menge und Energie deshalb als offen; daneben liegt eine ausdrücklich bedingte Live-Up-Vorlage für den Fall, dass die ganze Flasche getrunken wurde. Null bedeutet unbekannt, nicht null Kalorien. Die bloß erwähnte Zimtschnecken-Sorte wird nicht zusätzlich als heutiger Verzehr eingetragen. Ihre Existenz wurde auf der [Herstellerseite](https://allesmuelleroderwas.de/produkte/muellermilch/muellermilch-original-in-der-flasche/zimtschnecken-geschmack) bestätigt; daraus folgt kein weiterer Konsum.

### Möglicherweise gewünschter Kalorienverbrauch beim Gehen

Der erste ASR-Lauf erkennt bei 01:24 eine mögliche Frage nach verbrannten Kalorien; der zweite verliert diesen Satz. Deshalb nur eine grobe, bedingte Rechnung: Aus den Außen-Szenen lassen sich ungefähr **8 Minuten Gehen** ansetzen, mit Unterbrechungen und unbekanntem Tempo. Angenommen, Tobis frühere Selbstauskunft von etwa **82,5 kg** gilt noch, ergibt die Schofield-Gleichung für Jungen 10–18 Jahre einen geschätzten Grundumsatz von **1,471 kcal/min**. Mit 2,7–4,7 METy für langsames bis normales Gehen ergibt das für diese 8 Minuten rund **32–55 kcal insgesamt**, darin rund 12 kcal Grundumsatz; **20–44 kcal zusätzlich**. Dies ist ein Szenario, keine Messung und keine exakte Bilanz des ganzen Ausflugs. Marktaufenthalt und mögliche Aufnahmeunterbrechungen sind darin nicht vollständig berücksichtigt. Quelle für Formel und Jugendwerte: [NCCOR, Anwendung](https://www.nccor.org/tools-youthcompendium/how-to-use/), [NCCOR, beobachtete/imputierte Aktivitätswerte](https://www.nccor.org/tools-youthcompendium/view-all-categories/). Kein Verbrauchswert wurde automatisch in die App übernommen.

## Erkenntnisse zum Recorder

- **Alltagsdokumentation funktioniert grundsätzlich:** Ortsszenen, eigener Bericht, Pfandbon, Produktmarke und Trinken sind erkennbar. Das Audio trägt wichtige Daten bei, die das Bild nicht sicher lesen lässt: Beträge, Sorte, Absichten.
- **Trageausrichtung ist der stärkste inhaltliche Engpass:** Häufig Selbstansicht von unten, Hals/Shirt/Himmel/Decke. Für „Wo habe ich etwas hingelegt?“ fehlen damit oft Tisch, Hände, Blickrichtung und Gegenstände. Höhere Bildrate allein behebt dieses Sichtfeldproblem nicht.
- Kleine Schrift auf Bon/Flasche bleibt unscharf; Bewegung, sehr geringer Abstand und Sonne/Gegenlicht beeinträchtigen die Erkennung. Kein exakter Barcode, keine Nährwerttabelle und kein Kalenderdatum wurden erfunden.
- Mittlere Bildrate 0021: 9,934 Bilder/s. Es gibt 23 Abstände >150 ms, zwei >500 ms, maximal 936 ms. Das ergibt gelegentliche Stocker. Sensorbilder, die absichtlich zur Drosselung ausgelassen werden, sind keine entsprechend vielen verlorenen gespeicherten Bilder.
- Audio beginnt rund 0,46 s nach dem ersten Bild. Die MP4 berücksichtigt diesen Versatz. Für 0021/0023 verwendet sie echte CSV-Bildzeiten, für 0022 nur eine Näherung aus 9,96 Bildern/s.
- Audio-Ringpuffer meldet in den vorhandenen Zählern kein Verwerfen. `audio_missing=512` ist eine 32-ms-Zeit-/Sampledifferenz in der blockweisen Messung, kein alleiniger Beweis für ein hörbares Loch oder garantierte Lückenlosigkeit.
- 0022 hat 569 Samples an oder unmittelbar nahe der Vollaussteuerung (~0,016 %); 0021 nur 14. Kurzzeitige Übersteuerung ist möglich. Durchschnittspegel sind kein Rauschabstand und keine objektive Verständlichkeitsquote.
- Alle vorhandenen Fortschrittslogs melden `wifi=0`. Ein gespeichert vorhandenes WLAN-Profil belegt keine gelungene Verbindung; die genaue Ursache wurde nicht bestimmt.
- Abziehen mitten im Lauf bewahrt viel Material, erzeugt hier aber fehlende Schlussmetadaten und ein abgeschnittenes JPEG. Die Originaldatei wurde nicht repariert; die abspielbare Kopie lässt ausschließlich diesen unvollständigen Schlussframe aus.
- Ein kontextbehafteter Whisper-Lauf kann bei dieser Aufnahme den vorherigen Text endlos wiederholen und weiteren Inhalt verlieren. Erneute Verarbeitung ohne Textfortsetzung ist für eine robuste spätere Pipeline ein wichtiger Prüfpunkt. Bestehende Software wurde dafür nicht geändert.

## Eigene Anweisungen und künftige Wünsche aus den Aufnahmen

| Auftrag / Wunsch | Fundstelle | Ergebnis dieser Auswertung |
|---|---|---|
| Video/Ton auswerten und wiedergeben, was gesagt wurde | 0021 ca. 01:26–01:36, 07:35–07:44; aktueller Chat | Transkript, Bildlog, Techniklog, Rohdaten und Belegbilder gespeichert |
| „hell yeah, baby“ antworten | 0022 02:01–02:11 | Berücksichtigt |
| JSON für Kalorien-App | 0022 02:41–02:57 | Produktwerte recherchiert; tatsächliche Portion offen; Befund und bedingte Vorlage erstellt |
| JSON für Geldguru, Einnahme/Ausgabe getrennt | 0022 02:57–03:13 | +3,40 / −1,74 separat; keine doppelte +1,66-Buchung; Zahlungsweg als offen bzw. Annahme markiert |
| Später mit Apps verbinden, weniger Handy/Scrolling | 0021 01:39–02:07 | Als persönliche Absicht und Projektziel festgehalten |
| Display/Restzeit bis Neustart, Akku, Stoppknopf | 0021 08:53–09:46 | Als neue Wünsche für spätere Arbeit festgehalten; nicht umgesetzt |
| Recorder vor dem Körper tragen, Ausrichtung prüfen | 0021 ca. 00:46–01:05 und 07:35–07:44 | Sichtfeldproblem dokumentiert, künftige Befestigungsentscheidung offen |
| Abstecken als Dateiabbruch-Test | 0022 03:25–03:31 | Folgen an der Datei dokumentiert, keinen weiteren Abbruch durchgeführt |

## Was hier ausdrücklich unbekannt bleibt

Exakte Aufnahme-Uhrzeit und vollständige Route/Adresse; Anzahl/Pfandsätze jeder zurückgegebenen Flasche; sicher gelesener Bonbetrag aus Bild allein; Zahlungsweg; tatsächlich getrunkene Menge/Rest; etwaige nicht gefilmte weitere Käufe; sichere Sprecherzuordnung aller Hintergrundsätze; jedes einzelne Wort in verrauschten Passagen; exakte Framezeiten von 0022; alle möglicherweise nach Abziehen ungesicherten Daten; objektiv gemessener Energieverbrauch. Diese offenen Punkte sind keine Nullwerte und werden nicht mit geratenen Tatsachen aufgefüllt.

## Dateien zum Nachprüfen

- [Transkript mit Zeitmarken und Unsicherheiten](C:/Users/a/Documents/Codex/2026-10-05/okay-ich-wei-nicht-ob-du/outputs/Recorder-Transkript.md)
- [Ausführliche sichtbare Einzelbeobachtungen](C:/Users/a/Documents/Codex/2026-10-05/okay-ich-wei-nicht-ob-du/outputs/Recorder-Bildprotokoll.md)
- [Vollständige technische Analyse und Messgrenzen](C:/Users/a/Documents/Codex/2026-10-05/okay-ich-wei-nicht-ob-du/outputs/Recorder-Technik.md)
- [Geldbeträge und Belege als JSON](C:/Users/a/Documents/Codex/2026-10-05/okay-ich-wei-nicht-ob-du/outputs/Recorder-Geldguru-Befunde.json)
- [Bedingte Geldguru-Vorlage: Bargeldverrechnung bitte prüfen](C:/Users/a/Documents/Codex/2026-10-05/okay-ich-wei-nicht-ob-du/outputs/Geldguru-Bargeld-Vorlage.json)
- [Kalorienbefund: Portion offen](C:/Users/a/Documents/Codex/2026-10-05/okay-ich-wei-nicht-ob-du/outputs/Recorder-Kalorien-Befunde.json)
- [Bedingte Live-Up-Vorlage: nur bei vollständigem Verzehr](C:/Users/a/Documents/Codex/2026-10-05/okay-ich-wei-nicht-ob-du/outputs/LiveUp-Ganze-Flasche-Vorlage.json)
- [0021 mit Bild und Ton](C:/Users/a/Documents/Codex/2026-10-05/okay-ich-wei-nicht-ob-du/outputs/Recorder-0021-Pruefkopie.mp4)
- [0022 mit Bild und Ton, Zeitachse geschätzt](C:/Users/a/Documents/Codex/2026-10-05/okay-ich-wei-nicht-ob-du/outputs/Recorder-0022-Pruefkopie.mp4)
- [Zusätzliche USB-Aufnahme 0023](C:/Users/a/Documents/Codex/2026-10-05/okay-ich-wei-nicht-ob-du/outputs/Recorder-0023-Pruefkopie.mp4)
- [SHA-256-Verzeichnis der Originalkopien](C:/Users/a/Documents/Codex/2026-10-05/okay-ich-wei-nicht-ob-du/outputs/Recorder-Originale/manifest.json)
- [Alle geordneten Bildübersichten](C:/Users/a/Documents/Codex/2026-10-05/okay-ich-wei-nicht-ob-du/outputs/Bilduebersichten/Liesmich.md)
- [Unbearbeitete automatische Erkennung zur Gegenprüfung](C:/Users/a/Documents/Codex/2026-10-05/okay-ich-wei-nicht-ob-du/outputs/ASR-Rohdaten/Liesmich.md)
- [Abschließender Datei-/Hash-Abgleich](C:/Users/a/Documents/Codex/2026-10-05/okay-ich-wei-nicht-ob-du/outputs/Recorder-Abschlusspruefung.json)

Originale, Prüfkopien, Messdaten und die Sprache bleiben gemeinsam im Ausgabeordner. Die MP4 ist komprimiert und ersetzt die gesicherten Originale nicht. Die Berichtsinhalte werden außerdem im Brain verknüpft. Der normale Brain-Sync sichert die Notizen; die großen Videos werden dadurch nicht automatisch als Recorder-Medien auf das NAS archiviert.


Verwandt: [[Recorder DV2 Lauftest]], [[Audio-Recorder]], [[Live Up]], [[Recorder Testaufnahmen 0021-0022]], [[Müllermilch Geschmackseindrücke]].
