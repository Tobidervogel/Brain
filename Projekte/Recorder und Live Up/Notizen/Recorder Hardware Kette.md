---
titel: Recorder Hardware Kette
tags: [projekt, hardware, audio, akku]
erstellt: 2026-10-07
aktualisiert: 2026-10-10
---

# Recorder Hardware Kette

Plan vom 07.10.2026 (Claude), was der [[Audio-Recorder]] außer dem XIAO ESP32S3 Sense noch braucht, um als Kette alltagstauglich zu sein. Tobi druckt das Gehäuse selbst (3D-Drucker). **Stand 09.10.2026: Ladeplatine und JST-Kabel-Kit sind da, der LiPo ist unterwegs** (am 07.10. stand hier: noch nichts bestellt). Bezugsquellen stehen unten.

## Stand 09.10.2026 — Teile da, Schaltplan und Lötplan

Die Ladeplatine (Adafruit bq24074) und das JST-PH-Kabel-Kit von Eckstein sind angekommen. Der LiPo ist bestellt und noch unterwegs; welches Modell, hat Tobi nicht gesagt. Das Gehäuse kommt danach aus dem 3D-Drucker.

![[Recorder Schaltplan.svg]]

### Am Original-Schaltplan von Adafruit nachgesehen (Rev B)

- Die Ladeplatine wird **nicht programmiert**. Der bq24074 hat keine Software; eingestellt wird nur über Lötbrücken. Programmiert wird der XIAO, siehe [[Recorder AR1 Firmware]] (AR1.2).
- Zwei JST-PH-Buchsen: „Lipo Batt“ (Akku) und „Load Out“ (Ausgang, rund 3 bis 4,4 V). Die Kabel aus dem Kit passen in beide.
- Die grüne LED (PGOOD) und die rote LED (CHG) hängen jeweils über 10 kΩ an OUT (höchstens 4,4 V). Die Pads `PGOOD` und `CHG` sind Open-Drain-Ausgänge, die nach Minus ziehen. Deshalb dürfen beide **direkt** an einen Pin des XIAO; die Firmware schaltet den internen Pull-up ein. Der frühere Hinweis „vorher messen, sonst 10 kΩ in Reihe“ ist damit überholt.
- Ladestrom-Brücken auf der Rückseite: 1,5 A (590 Ω), 1 A (1 kΩ, ab Werk geschlossen), 0,5 A (2 kΩ). Für 0,5 A **erst die 1-A-Brücke auftrennen, dann 0,5 A zulöten**. Wer nur zulötet, schaltet beide Widerstände parallel und bekommt rund 1,3 A statt weniger.
- Eingangsstrom ab Werk bis 1,5 A (EN1 low, EN2 high).

### Lötplan in Stufen

Den Akku beim Löten nie eingesteckt lassen und nie am Akku selbst löten.

0. **Vor dem Löten:** USB-C an die Ladeplatine, die grüne LED leuchtet. An „Load Out“ mit dem Multimeter rund 4,4 V messen. Ein Kabel aus dem Kit einstecken und messen, welche Litze Plus führt: den Farben nicht trauen, bei solchen Kits sind Rot und Schwarz gegenüber Adafruit oft vertauscht.
1. **Strom (Pflicht):** Kabel aus „Load Out“ an die Akku-Lötflächen auf der Unterseite des XIAO. Minus ist die Fläche näher am USB-Anschluss; Gegenprobe: Durchgang zum GND-Pin. Litzen danach mit Kaptonband oder einem Tropfen Heißkleber entlasten. Test: USB-C an die Ladeplatine, der XIAO startet auch ohne Akku.
2. **Meldeleitungen:** `PGOOD` → D3, `CHG` → D4, direkt.
3. **Akkumessung:** Pad `LIPO` → R1 → D0; von D0 gehen R2 und C nach GND. Zwei gleiche Widerstände (470 kΩ, es geht alles ab 100 kΩ) und 100 nF.
4. **Später, zusammen mit dem Gehäuse:** Taster D1 nach GND, LED mit 1 kΩ an D5, Schiebeschalter in der Plus-Leitung von „Load Out“. Dafür gibt es noch keine Firmware.

Wenn der LiPo da ist: Polung am Stecker gegen den Aufdruck der Buchse „Lipo Batt“ prüfen, erst dann einstecken. Ladestrom entscheiden, wenn das Modell bekannt ist (Tendenz 0,5 A).

### Prüfen mit der Firmware AR1.2

`python tools/ar1.py STATUS` zeigt `"power":{"state":…,"bat_mv":…,"bat_pct":…}`:

| Aufbau | erwartet |
|---|---|
| nichts angelötet | meist `unknown`, `bat_pct` ist `null` |
| USB-C an der Ladeplatine | `external`, beim Laden `charging` |
| nur Akku | `battery`, `bat_mv` zwischen 3300 und 4200 |

Weicht `bat_mv` vom Multimeter am Akku ab: `BAT_SCALE` in `main.cpp` anpassen.

### Steckbrett-Aufbau ohne Löten (09.10.2026, Tobis Wunsch)

Tobi will zuerst alles auf dem Steckbrett aufbauen, damit sich alles wieder ablöten oder abziehen lässt. Sein Kit ist das Elegoo „UNO R3 Most Complete Starter Kit“ (im Diktat „Lego“): Steckbrett mit 830 Punkten, Steckbrett-Netzteilmodul (höchstens 9 V Eingang), 9-V-Batterie mit Clip, LEDs, Taster, 104-Kondensatoren (100 nF), Jumper-Kabel. Widerstände hat er, ihre Werte sind noch nicht gelesen (vier Farbringe oder messen). Kein LiPo da.

- Ohne LiPo kommt der Strom über USB oder über 9 V → Netzteilmodul auf 5 V → `5V`-Pin des XIAO. **Nie 9 V an einen Pin, an die Ladeplatine oder an die Akku-Lötflächen.**
- Auf dem Steckbrett geht `LOAD +` der Ladeplatine an den `5V`-Pin des XIAO statt an die Akku-Lötflächen (kein Löten an den winzigen Flächen). Endgültig kommen sie später an die Lötflächen. Am `5V`-Pin liegt bei USB am XIAO dessen 5 V an: nie beide Quellen gleichzeitig, vor dem Flashen die `LOAD +`-Leitung abziehen.
- Spannungsteiler: zwei gleiche Widerstände von 10 kΩ bis 470 kΩ genügen; mit dem 100-nF-Kondensator (Aufdruck „104“) ist auch 10 kΩ genau genug.
- Ladeplatine und XIAO brauchen Stiftleisten, damit sie ins Steckbrett passen. Offen: hat der XIAO Stifte, und liegt der Ladeplatine eine Leiste bei?

Rückmeldungen beim Aufbau (09.10.2026): Der XIAO hat oben keine Pinbeschriftung (Pins nach USB‑Anschluss und 5V‑Messung zuordnen). Das Steckbrett hat rund 63 Spalten, der Lader kommt mit aufs gleiche Brett (die Skizze mit dem Lader außerhalb war nur schematisch). Der Lader hat drei GND‑Pins; sie sind auf der Platine verbunden, einer reicht. Die Pin‑Reihenfolge des Laders ist noch nicht bekannt (Foto vom Aufdruck steht aus).

**Aufbau steht (09.10.2026 abends, vier Fotos von Tobi):**

- Stiftleisten sind angelötet, beide Platinen stecken auf demselben Brett. Der XIAO sitzt in den Reihen 6 bis 12, das USB‑Ende liegt bei Reihe 12 unter der SD‑Karte, die Antenne am anderen Ende. Der Lader sitzt ab etwa Reihe 33 in der rechten Bretthälfte.
- Pin‑Reihenfolge des Laders, vom USB‑Ende aus (vom Foto abgelesen; der Aufdruck steht abwechselnd in zwei Spalten): `VBUS`, `GND`, `THERM`, `ISET`, `CE`, `PGOOD`, `CHG`, `OUT`, `GND`, `LIPO`, `GND`. Elf Pins, drei davon GND.
- Jumper am Lader laut Foto: schwarz an `GND` (zweiter Pin), grün an `PGOOD`, rot an `CHG`, rot an `OUT`, rot an `LIPO`. Am XIAO: rot an 5V (Reihe 12 links), schwarz an GND (Reihe 11 links), schwarz von D0 (Reihe 12 rechts) zum Teiler. Ob grün und rot genau auf D3 und D4 sitzen, lässt sich am Foto nicht sicher ablesen.
- Teiler bei Reihe 49 bis 53 mit zwei Widerständen zu **100 kΩ** (Tobis Angabe) und einem Kondensator; beide gehen in die Minus‑Schiene. Die beiden Minus‑Schienen sind oben mit einem Kabel verbunden.
- Auf den Fotos sehen die oberen drei Lader‑Pins (`VBUS`, `GND`, `THERM`) ungelötet aus. Genau dort steckt der schwarze GND‑Jumper. Nachfragen; sonst den Jumper auf einen der beiden unteren GND‑Pins umstecken.
- Der Lader zeigte laut Tobi grün (Strom liegt an).

**Erste Messung mit AR1.2 (09.10.2026):** `power.state` = `unknown`, `bat_mv` 4 bis 38. Grund, von Tobi danach genannt: Er hatte zum Flashen **alle** Jumper vom XIAO abgezogen (nötig wäre nur der rote von `OUT` gewesen). Die Messung sagt deshalb nichts über die Verdrahtung; die erste Deutung „der Teiler hängt dran, der Lader war stromlos“ war falsch. Ein offener D0 liest hier nahe 0. Nächster Schritt: vier Jumper wieder anstecken (GND Reihe 11 links, D0 Reihe 12 rechts, `PGOOD` auf D3 Reihe 9 rechts, `CHG` auf D4 Reihe 8 rechts), den roten weglassen, XIAO an den PC, Lader ans Netzteil, dann neu auslesen.

**Messungen mit vier Jumpern (09.10.2026, gegen 19 Uhr):** Tobi hat GND, D0, `PGOOD` und `CHG` wieder angesteckt, der rote Jumper von `OUT` blieb weg, der XIAO hing am PC.

- An der Powerbank kam nichts an: Sie musste von Hand gestartet werden und schaltet ohne nennenswerte Last vermutlich wieder ab. Für diesen Test ein Netzteil oder einen PC‑Anschluss nehmen.
- Mit Strom am Lader zeigt D0 keine Gleichspannung, sondern in jedem 200‑ms‑Fenster 0 bis rund 475 mV bei 142 mV Mittel: das Muster eines 50‑Hz‑Brummens, dessen untere Halbwelle abgeschnitten ist. In `STATUS` (ein Messwert alle 2 s) sah das wie ein langsames Pulsieren zwischen 0 und 900 mV aus. `PGOOD` war in keinem Moment auf Minus.
- Als Tobi am Brett hantierte, stand für gut eine Sekunde ruhig 2,09 V an D0, also **4,18 V an `LIPO`**. Teiler, Kondensator, D0 und der Faktor 2,0 stimmen damit.
- Deutung (noch nicht bestätigt): Dem Lader fehlt die sichere Minus‑Verbindung zum XIAO. Dazu passt, dass der schwarze Jumper am zweiten Lader‑Pin steckt und die oberen drei Pins auf den Fotos ungelötet aussehen. Anweisung an Tobi: schwarzen Jumper in die Reihe direkt neben `LIPO` stecken (beide Nachbarn sind GND). Bis zum Ende der Sitzung noch nicht umgesteckt.
- Nachtrag 09.10.2026, etwa 19:40 Uhr: Tobi hat den schwarzen Jumper umgesteckt (Bild mit Markierung und Steckplan bekam er im Chat). Danach ist das Brummen weg: D0 liegt ruhig bei 0 mV. Vom Lader kommt aber weiterhin nichts, weder Spannung an `LIPO` noch `PGOOD`; vermutlich hatte er beim Messen keinen Strom. Ob die grüne LED leuchtete und woran der Lader hing, hat Tobi nicht gesagt. Erbeten: ein Foto des ganzen Bretts mit Strom am Lader. Der rote Jumper von `OUT` ist bewusst noch nicht gesteckt (ohne sichere Minus-Verbindung flösse der Versorgungsstrom über die Signalpins zurück).
- Noch offen: `PGOOD` war auch in dem Moment mit gutem Kontakt nicht auf Minus. Bleibt das nach dem Umstecken so, die Reihen am XIAO mit einem Draht von Minus und dem Befehl `PINS` suchen (Reihe 9 rechts sollte D3 sein, Reihe 8 rechts D4).

**Durchbruch am 09.10.2026 um 21:17 Uhr (Live-Anzeige `pins_live.py`):** Seitdem hat der Lader Strom, Minus ist mit dem XIAO verbunden und die Messung an `LIPO` funktioniert: rund 7 s lang ruhig 4,17 bis 4,18 V, dann 1 bis 2 s 0 V, alle etwa 13 s wieder (Akkusuche ohne Akku). Teiler, Kondensator, D0 und der Faktor 2,0 sind damit bestätigt. Was Tobi dafür geändert hat, hat er nicht gesagt. **`PGOOD` (D3) und `CHG` (D4) bleiben dauerhaft auf „nein“**, auch D1, D2 und D5 sehen kein Minus. `CHG` ist ohne Akku erwartbar aus; bei `PGOOD` sitzt ein Ende der grünen Leitung falsch oder die Lötstelle am Pin hat keinen Kontakt. Tobi fragte daraufhin, ob es auch ohne `PGOOD` geht: ja, es ist nur eine Meldeleitung.

**Braucht der Lader einen LiPo, damit etwas ankommt? (Tobis Frage, 09.10.2026, im TI‑Datenblatt SLUS810N nachgelesen)**

- **Nein für `PGOOD` und `OUT`.** `PGOOD` zieht nach Minus, sobald eine gültige Eingangsspannung anliegt (Abschnitt 9.3.5.7); der Baustein soll ausdrücklich auch ohne oder mit defektem Akku das System versorgen. USB allein genügt also.
- **Teils ja für `LIPO`.** Ohne Akku gibt es dort keine feste Spannung: Der Baustein sucht den Akku (Abschnitt 9.3.5.4), zieht 250 ms lang 7,5 mA und lädt dann 250 ms lang vor. Der Pin springt deshalb im Viertelsekundentakt zwischen etwa 0 und 4,2 V. Genau dieses Muster stand am 09.10. für ein paar Sekunden an D0 (Fenster mit 2,09 V, dann 0 bis 2,1 V gemischt). Eine echte Akkuspannung gibt es erst mit LiPo.
- `CHG` ist ohne Akku hochohmig (Tabelle in 9.3.5.7), lässt sich also erst mit LiPo prüfen.
- Ohne Eingangsspannung sind `PGOOD` und `CHG` hochohmig und der Akku‑Transistor ist an: `OUT` hängt direkt am Akku (Abschnitt 9.3.1). Das bestätigt die Warnung unten.
- Folgerung für die Fehlersuche: Liegt D0 ruhig bei 0 mV, hat der Lader keinen Strom. In den Sekunden, in denen `LIPO` sprang, war `PGOOD` an D3 trotzdem nie auf Minus: Die grüne Leitung verbindet `PGOOD` also nicht mit D3 (ein Ende sitzt falsch). Noch nicht behoben.

**Gegenprüfung durch vier Agenten (09.10.2026 spätabends, Ultracode war eingeschaltet; rund 830.000 Tokens, 30 Minuten):**

- **Pin-Reihenfolge des Laders bestätigt** aus Adafruits Platinendatei (`Adafruit_BQ24074.brd`, Rev B), vom USB-Ende zu den JST-Buchsen: `VBUS`, `GND`, `THERM`, `ISET`, `CE`, `PGOOD`, `CHG`, `OUT`, `GND`, `LIPO`, `GND`. `PGOOD` ist genau der mittlere Pin (der sechste von beiden Enden). Der Aufdruck steht abwechselnd über und unter der Lochreihe, deshalb verzählt man sich leicht.
- **`PGOOD` wird nicht gebraucht.** Reiner Meldeausgang (TI SLUS810N, Tabelle 7-1 und 9.3.5.7); am Netz hängen nur der Chip-Pin, der Leistenpin und die grüne LED. In der Firmware speist er nur den Text `power.state`. Bester Ersatz, falls nötig: ein Spannungsteiler an `VBUS`.
- Warum `PGOOD` nicht ankommt, ist offen. Rangfolge: kein Kontakt am `PGOOD`-Pin der Lader-Leiste (kalte Lötstelle); grünes Kabel am Lader eine Reihe zu weit Richtung JST (dann sitzt es auf `CHG` und das `CHG`-Kabel auf `OUT`, also 4,4 V direkt an GPIO5); Fehler am XIAO-Ende (Kabel, Reihe, Lötstelle D3). Trenntest: Lader stromlos, Lader-Ende des grünen Kabels in die Reihe des schwarzen Minus-Kabels am Lader stecken; die Anzeige muss `PGOOD an D3: JA` zeigen.
- **Das `CHG`-Kabel bleibt ab**, bis die Reihen nachgemessen sind (Schutz vor 4,4 V an GPIO5).
- **Messkabel:** Das vom Lader abgezogene Ende des `LIPO`-Kabels taugt als Prüfspitze (100 kΩ in Reihe); die Anzeige zeigt dann die Spannung der berührten Reihe (schreibt aber immer „an LIPO“). Erwartet: XIAO 5V 4,7 bis 5,3 V, 3V3 3,2 bis 3,4 V, Lader `OUT` ruhig 4,3 bis 4,5 V, `VBUS` etwa 4,5 bis 4,9 V, `CHG` 2 bis 3 V. `PGOOD` lässt sich so nicht prüfen (0 V auch ohne Kontakt). Nie das Kabelende nehmen, das direkt von D0 kommt.
- **XIAO, aus Seeeds Schaltplan:** Der `5V`-Pin ist direkt die USB-Leitung VBUS, ohne Diode. 3,3 V macht ein Schaltregler (SGM6029, Eingang 1,95 bis 5,5 V), kein Längsregler. Akku-Minus ist die Fläche näher am USB. Die SD-Karte ragt am USB-Ende heraus (bestätigt die Lage auf Tobis Fotos). GPIO1, 4 und 5 sind frei. Der eingebaute Lader ist laut Schaltplan auf rund 110 mA eingestellt (220 kΩ); die frühere Angabe 50 mA für den Sense stammt aus der Wiki-Tabelle. Widerspruch ungeklärt.
- **Lader, bisher nicht bedacht:** keine Zeitabschaltung (`TMR` liegt auf Minus) und kein Temperaturfühler (`THERM` fest auf 10 kΩ), also nur unter Aufsicht laden. Eingangsstrom bis etwa 1,6 A, dafür ein Netzteil mit 2 A statt eines PC-Anschlusses. Der Akku gehört nur in „LiPo Batt“, nie in „Load Out“ (dort 4,4 V ohne Ladeabschaltung).
- **Endgültige Verdrahtung ist noch nicht entschieden.** „Load Out“ an die Akku-Lötflächen bleibt der vorgesehene Weg, aber Adafruit empfiehlt eine Diode in der Plus-Leitung, wenn die Zielplatine einen eigenen Lader hat. 4,4 V an den Akku-Flächen liegen innerhalb der geprüften Chip-Grenzen, Seeed nennt aber keinen Höchstwert.

### Steckbrett-Aufbauplan mit Lochnummern (09.10.2026, spätabends)

Tobi lötet die Stiftleisten neu und wollte eine Grafik, nach der er alles schnell wieder aufbauen kann. Das Bild ist der verbindliche Plan für das Steckbrett; `Recorder Schaltplan.svg` weiter oben zeigt dagegen die spätere endgültige Verdrahtung über die Akku-Lötflächen.

![[Recorder Steckbrett-Aufbau.png]]

- **XIAO:** Stifte in d6 bis d12 und h6 bis h12, USB-Ende (SD-Karte) bei Reihe 12. Links von oben: D7, D8, D9, D10, 3V3, GND (Reihe 11), 5V (Reihe 12). Rechts von oben: D6, D5, D4 (Reihe 8), D3 (Reihe 9), D2, D1, D0 (Reihe 12).
- **Lader:** Stiftleiste in Spalte i, Reihen 33 bis 43, USB-C-Buchse bei Reihe 33: `VBUS` 33, `GND` 34, `THERM` 35, `ISET` 36, `CE` 37, `PGOOD` 38, `CHG` 39, `OUT` 40, `GND` 41, `LIPO` 42, `GND` 43.
- **Stufe 1 (sofort):** 1 schwarz a11 → linke Minus-Schiene; 2 schwarz b11 → g41 (Minus direkt vom XIAO zum Lader); 3 rot g42 → a49; R1 100 kΩ b49 → b52; R2 100 kΩ a52 → Minus-Schiene; C (104) c52 → Minus-Schiene; 4 schwarz d52 → j12 (D0).
- **Stufe 2 (erst nach dem Nachmessen der Reihen):** 5 grün g38 → j9 (`PGOOD` an D3); 6 rot g39 → j8 (`CHG` an D4); 7 rot g40 → c12 (`OUT` an 5V, dann XIAO nie am PC).
- Neu gegenüber dem ersten Aufbau: Minus geht mit Kabel 2 direkt vom Lader zum XIAO. Die Brücke zwischen den beiden Minus-Schienen und das Kabel vom Lader zur rechten Schiene entfallen.
- Das Bild erzeugt `A:/Recorder/docs/steckbrett.py` (Pillow); dort lassen sich Löcher und Kabel ändern.

**`PGOOD` funktioniert (09.10.2026, 23:27 Uhr):** Nachdem Tobi die Stiftleisten neu gelötet und nach dem Aufbauplan gesteckt hat, zeigt die Live-Anzeige durchgehend `PGOOD an D3: JA`, dazu wie zuvor `LIPO` zwischen 0 und 4,19 V (kein Akku) und `CHG an D4: nein` (ohne Akku erwartbar). Die Ursache lag also an Lötstelle oder Steckplatz, nicht am Chip. `OUT` hat Tobi bewusst noch nicht angeschlossen. Nicht geprüft: `power.state` in `STATUS` (der Anschluss war durch die Anzeige belegt) und `CHG` (geht erst mit LiPo). Nächster Schritt: Reihen von `OUT` (g40) und 5V (c12) mit dem Messkabel nachmessen, dann der Test mit `OUT`.

**Der Lader versorgt den XIAO (09.10.2026, gegen 23:45 Uhr):**

- Vorher mit dem Messkabel nachgemessen (Werte aus der Live-Anzeige): Reihe g40 ruhig **4,41 V** (`OUT`), Reihe c12 **5,10 V** (5V-Pin des XIAO am PC). Beide Reihen stimmen mit dem Aufbauplan überein.
- Dann Kabel 7 (g40 → c12) gesteckt, beide USB-Kabel ab, Strom nur am Lader: Der XIAO verschwand am PC (COM6 weg) und antwortete 36 Sekunden später wieder im WLAN (`http://192.168.0.112/status.json` liefert 401, also läuft der Webserver). Damit ist belegt, dass `OUT` und die Minus-Leitung den XIAO tragen.
- Für diesen ersten Versuch sollte Tobi das grüne `PGOOD`-Kabel (und `CHG`, falls gesteckt) abziehen; ob er das getan hat, ist nicht bestätigt. Sie kommen bei abgestecktem Lader wieder dran.
- Ohne USB am XIAO gibt es keine Live-Anzeige. Der Strom-Zustand ließe sich dann nur über die Statusseite (Login nötig) oder später über die App ablesen; in Live Up fehlt die Anzeige noch.
- Zurück an den PC nur in dieser Reihenfolge: USB vom Lader ab, Kabel 7 an c12 ziehen, erst dann den XIAO an den PC.

**Gefahr mit LiPo und dem Steckbrett‑Aufbau:** Solange `OUT` am `5V`‑Pin des XIAO hängt, darf der XIAO **nicht per USB an den PC, wenn ein LiPo eingesteckt ist**. Ohne Strom am Lader schaltet der bq24074 den Akku direkt auf `OUT`; die 5 V vom PC lägen dann ungeregelt am Akku. (Hier stand zuerst: ohne Akku unkritisch. **Korrigiert am 09.10.2026:** Die Regel gilt ohne Ausnahme, auch ohne Akku. Ist der Lader stromlos, liegen die 5 V vom PC über den eingeschalteten Akku-Transistor am `BAT`-Anschluss, dessen Höchstwert 5 V beträgt; ein USB-Anschluss darf darüber liegen.) Abhilfe: vor dem Anstecken an den PC den roten Jumper `OUT` → `5V` ziehen, oder gleich die endgültige Verdrahtung bauen (`Load Out` per JST‑Kabel an die Akku‑Lötflächen des XIAO, dort lädt der XIAO nur geregelt mit 50 mA nach).

### Testakku aus einer alten Vape (10.10.2026)

Der bestellte LiPo kommt voraussichtlich erst am Montag, 12.10.2026. Tobi hat eine Zelle aus einer alten Vape: Aufdruck `13300`, 360 mAh, 3,7 V, 1,33 Wh, darunter `3520A230910`. Er fragte, ob sie für einen Test taugt, ob stattdessen die 9-V-Batterie an den Solar-/DC-Eingang soll und ob die JST-PH-Kabel passen.

- `13300` ist die Bauform (rund, 13 mm Durchmesser, 30 mm lang), die übliche Zelle aus Einweg-Vapes. 360 mAh × 3,7 V = 1,33 Wh passt. Die untere Nummer ist vermutlich Charge und Datum (10.09.2023); nicht belegt.
- Solche Zellen haben in aller Regel **keine Schutzschaltung** (die saß in der Vape). Also kein Schutz gegen Kurzschluss und Tiefentladung. Nur als Testakku auf dem Tisch, nicht für das Gerät am Körper.
- **Entladetest geht sofort:** Zelle an `LIPO`, kein USB am Lader. Der bq24074 schaltet den Akku dann auf `OUT`; geprüft werden `power.state` = `battery` und `bat_mv`. Rund 360 mAh reichen bei 65 bis 100 mA für etwa 3 bis 5 Stunden. Unter 3,3 V abstecken, denn weder Lader noch Firmware schalten bei leerem Akku ab.
- **Laden mit der Platine im Auslieferzustand nicht:** 1 A wären fast 3C, auch die 0,5-A-Brücke ist mit 1,4C zu viel. Richtwert 0,5C, also rund 180 mA. Weg laut Adafruit-Pinout-Seite: Leiterbahn der 1,0-A-Brücke auftrennen, dann ein Widerstand von 590 Ω bis 8,9 kΩ von `ISET` (Reihe 36) nach Minus. Mit 5,1 kΩ ergeben sich rund 175 mA, mit 4,7 kΩ rund 190 mA (Faktor 890 AΩ aus dem TI-Datenblatt, aus dem Gedächtnis, nicht neu nachgelesen). Ohne Widerstand an `ISET` lädt die Platine gar nicht. Für den endgültigen Akku die Brücke wieder zulöten oder den Widerstand passend wählen.
- **9-V-Batterie bringt nichts:** Sie wäre nur eine zweite Eingangsquelle wie USB und ersetzt keinen Akku; `CHG` und `battery` lassen sich damit nicht prüfen. Die Regel „nie 9 V an `LIPO`, `OUT` oder einen Pin“ bleibt.
- **Anschluss:** JST-PH hat 2,0 mm Raster und gehört in die Buchse „Lipo Batt“. In die Lochreihe der Platine (2,54 mm) passt der Stecker nicht, dort sitzen ohnehin die Stiftleisten. Empfohlen: ein Kabel aus dem Kit an die beiden Litzen der Zelle löten (Draht an Draht, jede Litze einzeln, jede Lötstelle in Schrumpfschlauch, nie an der Zelle selbst löten), Polung nach dem `+` am Aufdruck der Buchse, nicht nach der Kabelfarbe.
- Vor dem ersten Einstecken: Zelle ansehen (nicht aufgebläht, keine Dellen) und die Spannung mit dem Messkabel prüfen; unter etwa 3,0 V nicht verwenden.

Nachtrag 10.10.2026: Tobi will die Zelle genauso anschließen wie später den LiPo, also mit JST-PH-Stecker in „Lipo Batt“. Die Stecker aus dem Kit passen in die Buchse, gehen aber schwer wieder heraus (am Gehäuse ziehen, nie an den Litzen). Die Zelle hat ihm eine Spannung angezeigt; den Wert hat er nicht genannt. Er bekam im Chat eine Skizze: Litzen der Zelle versetzt an das Kit-Kabel löten, Schrumpfschlauch über jede Lötstelle, Plus-Litze des Kabels vorher in der Buchse ausmessen und markieren.

**Zelle ist angelötet und eingesteckt (10.10.2026):** Tobi meldet, sie sei dran, und „die Leuchten leuchten ein bisschen, wenig“ (welche, hat er nicht gesagt). Vom PC aus war der Recorder nicht zu sehen: kein COM6 (nur COM1) und keine Antwort von `http://192.168.0.112/status.json` bei vier Versuchen über etwa eine Minute; die Adresse fehlte auch in der Nachbarliste des PCs. Das WLAN startet in der Firmware unabhängig von der Stromquelle, der XIAO läuft also vermutlich nicht. Deutung, nicht gemessen: Ohne USB sind `PGOOD` und `CHG` hochohmig und die LEDs des Laders müssten aus sein; ein schwaches Glimmen passt zu einem unversorgten XIAO, in dessen Pins über die Meldeleitungen ein kleiner Strom fließt (Kabel 7 von `OUT` nach 5V fehlt oder die Zelle ist zu leer). Nächster Schritt: Kabel 7 prüfen oder an c12 ziehen und den XIAO an den PC, dann zeigt `pins_live.py` die Spannung der Zelle.

**Wie Tobi die Zelle wirklich angeschlossen hat (10.10.2026, seine Beschreibung per Diktat):** nicht gelötet. Ein JST-PH-Kabel aus dem Kit steckt in einer Buchse des Laders (welche, hat er nicht gesagt), die freien Enden in einer bisher unbenutzten Plus-/Minus-Schiene des Steckbretts; die Zelle hängt über zwei Steckkabel an derselben Schiene. Am Lader hat er nichts umgestellt (Ladestrom also weiter 1 A). Ergebnis: Die LEDs leuchten „sehr, sehr schwach“, der Recorder erscheint weiter weder an USB noch im WLAN. Die Spannung der Zelle ist noch nie gemessen worden.

- Tobis Vermutung „die Zelle ist zu schwach“ trifft so nicht zu: Eine Vape-Zelle liefert mehrere Ampere, der XIAO braucht rund 0,1 A (Spitzen um 0,3 A). Möglich ist aber, dass sie **leer** ist; das ist etwas anderes.
- Wahrscheinlichere Ursachen, ungeprüft: Zelle tiefentladen; Wackelkontakt der Steckkabel an der Zelle; Polung an Schiene oder Kit-Kabel vertauscht; die Schienen eines 830er-Bretts sind oft in der Mitte unterbrochen; Stecker in „Load Out“ statt „Lipo Batt“.
- Dazu kommt beim Steckbrett-Aufbau: `OUT` geht an den 5V-Pin, dahinter sitzt im XIAO noch eine Diode. Am Akku kommen deshalb rund 0,3 V weniger am Regler an als über die Akku-Lötflächen (abgeschätzt, nicht gemessen); eine halb leere Zelle reicht dann nicht mehr zum Starten.
- **9-V-Batterie:** Tobi fragte, ob er sie über den JST-Stecker oder über die Hohlbuchse anschließen kann. Über JST (`LIPO` oder `OUT`) nie: `LIPO` verträgt höchstens 5 V, und `OUT` hinge mit 9 V direkt am XIAO. Über die Hohlbuchse ist es zulässig (Adafruit-Produktseite, am 10.10.2026 nachgelesen: Eingang 5 bis 10 V, geschützt bis 28 V); damit ist der frühere Satz „nie 9 V an die Ladeplatine“ für die Hohlbuchse zu streng gewesen. Es ersetzt aber nur das USB-Kabel und testet keinen Akkubetrieb. Die Vape-Zelle muss dabei abgesteckt sein, sonst wird sie mit dem eingestellten 1 A geladen. Die Polung der Hohlbuchse (Mitte Plus) steht nicht auf der Produktseite; vor dem Einstecken prüfen.

**Messwert von Tobi (10.10.2026):** „Spannung ist <0.1mA“, mehr hat er nicht geschrieben. Unklar sind Messgerät, Messstelle und Einheit (mA ist Strom, nicht Spannung). Er hat demnach offenbar doch ein Messgerät. Er fragte, ob er die Zelle am Lader gleich laden kann. Antwort: erst direkt an den Anschlüssen der Zelle im Bereich Gleichspannung (20 V) messen. Ab etwa 3,0 V ist die Zelle in Ordnung und der Fehler liegt am Kontakt; laden dann nur mit heruntergestelltem Strom (`ISET`, rund 175 mA) und unter Aufsicht. Liegt sie wirklich nahe 0 V, ist sie tiefentladen und wird nicht mehr geladen (Brandgefahr), sondern mit abgeklebten Enden zur Batteriesammlung gebracht. Der Lader würde sie trotzdem laden wollen und hat keine Zeitabschaltung; er schützt hier also nicht.

**Tobi hat ein Multimeter und hat gemessen (10.10.2026):** Damit ist die offene Frage nach dem Multimeter beantwortet.

- Direkt an der Zelle **4,19 V**: Sie ist voll und in Ordnung, laden ist nicht nötig. Die Vermutung „tiefentladen“ ist damit vom Tisch.
- An der Schiene bei den Enden des JST-Kabels schwankend 4,12 bis 4,4 V, auf der Ladeplatine hinter der Buchse 4,6 V. Werte über 4,19 V können nicht aus der Zelle stammen; es muss also noch eine andere Quelle anliegen (USB oder 9 V am Lader, oder der XIAO am PC mit gestecktem Kabel 7). Welche, hat Tobi nicht gesagt.
- Er hat auch im mA-Bereich quer über die Zelle und die Schiene gemessen (Anzeige 0,02 bis 0,3 mA). Das ist ein Kurzschluss über das Messgerät; dass kaum Strom floss, deutet auf eine durchgebrannte Sicherung im mA-Eingang oder einen falschen Bereich. Hinweis an Tobi: Strom nur in Reihe messen, hier gar nicht nötig.
- Am XIAO blinkt jetzt eine rote LED, „nicht mehr so dunkel“. Das ist dessen eigene Ladeanzeige, die blinkt, wenn am 5V-Pin Spannung anliegt und an seinen Akku-Flächen kein Akku hängt. Es zeigt nur, dass Spannung ankommt.
- Vom PC aus weiter nichts zu sehen: kein COM6, und eine Suche über das ganze Heimnetz (192.168.0.1 bis 254, Ping und `status.json`) fand kein Gerät mit der Anmeldung „Recorder DV1“. Der XIAO bekommt also Spannung, kommt aber nicht ins WLAN. Vermutung, nicht belegt: Er startet und fällt beim Einschalten des Funks wegen Spannungseinbruch wieder aus (dünne Steckkabel, Schienenkontakte, Diode hinter dem 5V-Pin). Prüfbar über den Reset-Grund (`brownout`), sobald er am PC hängt.

**Ausgelesen am 10.10.2026 um 11:09 Uhr (XIAO am PC, Kabel 7 an c12 gezogen, Zelle am Lader, keine Quelle am Lader):**

- `STATUS`: `power.state` = **`battery`**, `bat_mv` 4098, `bat_pct` 91. `PINS`: D0 ruhig 2039 bis 2061 mV, `PGOOD` und `CHG` nie auf Minus. Die Akkumessung über den Lader funktioniert damit zum ersten Mal mit einem echten Akku. Gegen Tobis Multimeter (4,19 V an der Zelle) liest die Firmware rund 2 % zu wenig; `BAT_SCALE` wäre 2,04 statt 2,0. Noch nicht geändert, weil die beiden Werte nicht im selben Moment gemessen wurden.
- Sonst ist der XIAO gesund: Karte in Ordnung, WLAN verbunden (192.168.0.112, RSSI −71), keine Fehler.
- **Startzähler 596, im `device.log` der Karte steht davor aber Start 38** (09.10.2026, 23:38 Uhr, der Test mit Strom vom Lader). Dazwischen liegen 557 Starts ohne Protokollzeile: Der Zähler wird ganz am Anfang erhöht, die Zeile erst nach dem Einbinden der Karte geschrieben. Der XIAO ist am Akku also Hunderte Male angelaufen und sofort wieder ausgefallen. Mit 4,41 V vom Lader lief derselbe Aufbau, mit der Zelle nicht.
- Deutung: Die Zuleitung bricht beim Anlaufen ein, nicht die Zelle (die ist voll). Verdächtig sind die Steckkabel an den Anschlüssen der Zelle, die Schienenkontakte und der zusätzliche Verlust hinter dem 5V-Pin. Der Reset-Grund dieser Starts ist nicht protokolliert, „Spannungseinbruch“ ist daher eine Folgerung, kein Messwert.
- Die LEDs am Lader: Tobi sah Grün und Rot kurz leuchten, solange der XIAO stromlos war; beim Anstecken des XIAO an den PC gingen beide aus. Das bestätigt die Deutung von oben: Bei stromlosem XIAO fließt ein kleiner Strom von `OUT` über die LEDs und die Meldeleitungen in dessen Pins (harmlos, rund 0,2 mA); mit versorgtem XIAO liegen die Pins auf 3,3 V und die LEDs bleiben aus.
- Vorschlag für den nächsten Versuch: Zelle fest an das JST-Kabel löten statt Steckkabel und Schiene, dazu ein Elko (100 µF aus dem Elegoo-Kit) zwischen 5V (Reihe 12 links) und GND (Reihe 11 links) am XIAO, Minus-Streifen an GND. Danach Kabel 7 stecken und am Startzähler ablesen, ob er durchläuft.

**Zelle ist an den JST-Stecker gelötet (10.10.2026, nach 11:09 Uhr, Tobis Meldung).** Für den Elko bekam er eine Skizze mit Lochnummern: 100 µF, langes Bein (Plus) in **b12** (5V), kurzes Bein mit dem Streifen (Minus) in **c11** (GND). a11 und b11 sind durch Kabel 1 und 2 belegt, c12 durch Kabel 7. Reihenfolge für den Versuch: XIAO vom PC ab, Polung des gelöteten Steckers mit dem Multimeter prüfen, Zelle in „Lipo Batt“, Elko stecken, zuletzt Kabel 7 an c12; kein USB am Lader. Ergebnis steht noch aus.

**Der Recorder läuft am Akku (10.10.2026, 11:32 Uhr):** Nach Tobis Meldung „steckt“ antwortete `http://192.168.0.112/status.json` von 11:32:44 bis 11:34:09 Uhr bei 24 von 25 Abfragen mit 401 (Webserver läuft), eine Abfrage um 11:33:14 Uhr blieb ohne Antwort. Am PC hing er dabei nicht (nur COM1), der Strom kommt also aus der Vape-Zelle über `LIPO` → `OUT` → 5V-Pin. Damit ist der Akkubetrieb über den Lader zum ersten Mal belegt. Geändert wurden gegenüber dem Fehlversuch zwei Dinge zugleich (Zelle angelötet statt Steckkabel und Schiene, dazu der Elko an b12/c11); welches davon den Ausschlag gab, ist nicht getrennt geprüft. Ob er in dieser Zeit neu gestartet ist (die eine fehlende Antwort), zeigt erst der Startzähler: zuletzt 596, ein sauberer Lauf ergibt 597. Noch offen: Laufzeit, `CHG` beim Laden, Abschalten bei leerem Akku. Die Zelle hat keinen Tiefentladeschutz, also bei 3,3 V oder nach rund drei Stunden abstecken.

Stand davor: Vorschlag, noch nichts davon gebaut. Offen: Hat die Zelle noch Litzen, und welche Widerstandswerte hat Tobi (5,1 kΩ ist im Elegoo-Kit üblich)?

### Offen (09.10.2026)

- Hat Tobi ein Multimeter, zwei gleiche Widerstände (100 bis 470 kΩ), 100 nF, Taster und LED? Im Kit von Eckstein sind nur Kabel.
- Welcher LiPo ist bestellt? Danach den Ladestrom festlegen.
- Abschalten bei leerem Akku (sauber beenden, dann Tiefschlaf) fehlt noch in der Firmware; die Schwelle erst am echten Akku messen.
- Gehäuse erst zeichnen, wenn der Akku da ist und der Aufbau auf dem Tisch läuft.

## Warum der Chip allein nicht reicht

- Der eingebaute Lader des XIAO Sense lädt nur mit **50 mA** (Seeed-Wiki; die frühere Angabe „ca. 100 mA“ gilt für die Plus-Variante). Die Mikrofonaufnahme braucht laut Seeed im Schnitt **64,5 mA** bei 3,8 V (Spitze 109 mA). Am eingebauten Lader wird der Akku bei laufender Aufnahme also nicht voll; ein Nutzer im Seeed-Forum hat genau das gemessen.
- Der XIAO kann den Akkustand nicht selbst messen und meldet nicht, ob er am Strom hängt.
- Im Tiefschlaf zieht der Sense laut Seeed noch **3 mA** (nicht 14 µA wie der XIAO ohne Aufsteckboard). Für längeres Liegen braucht es deshalb einen echten Schalter.

## Teile

| Teil | Wofür | Hinweis |
|---|---|---|
| Geschützter LiPo 3,7 V, 1500 bis 2000 mAh, mit JST-PH-2.0-Stecker | Strom für einen Tag | z. B. Bauform 103450 mit 1800 mAh (34 × 52 × 10 mm, rund 31 g): rechnerisch gut 24 h bei 70 mA. Flacher: 603450 mit 1200 mAh, rund 16 h. |
| Ladeplatine mit Power-Path: Adafruit bq24074 (Artikel 4755), USB-C | Laden und gleichzeitig versorgen | 14,95 $ bei Adafruit. Ladestrom ab Werk 1 A, per Lötbrücke 0,5 A oder 1,5 A. Ausgang höchstens 4,4 V. Meldet „Strom liegt an“ (PGOOD) und „lädt“ (CHG). |
| Taster 6 × 6 mm | Pause, Lesezeichen, Wecken aus dem Tiefschlaf | an einen Pin, der auch im Schlaf wach ist |
| Schiebeschalter (optional) | Akku ganz trennen | für Transport und Lagerung |
| LED mit 1 kΩ (optional) | zeigt „nimmt auf“ | auch als Hinweis für andere |
| 2 × 470 kΩ, 1 × 100 nF | Akkuspannung messen | Spannungsteiler |
| Silikonlitze, Schrumpfschlauch, Kaptonband, doppelseitiges Schaumband | Verdrahtung, Isolierung, Entkopplung | |
| Magnetisches USB-C-Kabel, dessen Spitze im Gerät bleibt | einfachste Ladehalterung | Kette abends nur anklicken |
| Kette oder Band mit Sicherheitsverschluss | Tragen | öffnet bei Zug |
| Multimeter | Polung und Spannungen prüfen | Pflicht, falls nicht vorhanden |
| USB-Strommessgerät (optional) | echten Verbrauch messen | vor der Wahl der Akkugröße sinnvoll |

## Verdrahtung

| Von | Nach | Zweck |
|---|---|---|
| Akku (JST) | Ladeplatine `BATT` | Akku |
| Ladeplatine `LOAD` + und − | XIAO `BAT+` und `BAT−` (Lötflächen unten; Minus liegt näher am USB-Anschluss) | Versorgung, optional über den Schiebeschalter |
| Ladeplatine `PGOOD` | XIAO D3 (GPIO4) | Strom liegt an |
| Ladeplatine `CHG` | XIAO D4 (GPIO5) | Akku lädt |
| Akku-Plus → 470 kΩ → D0 (GPIO1) → 470 kΩ → Minus, 100 nF von D0 nach Minus | | Akkuspannung (halbiert) |
| Taster | D1 (GPIO2) nach Minus | Bedienung, Wecken |
| LED + 1 kΩ | D5 (GPIO6) nach Minus | Anzeige |

Frei am Sense sind D0 bis D7; die Karte belegt D8 bis D10 und GPIO21, das Mikrofon GPIO41 und GPIO42.

## Worauf zu achten ist

- **Polung des Akkusteckers messen**, bevor er in die Ladeplatine kommt. Adafruit warnt ausdrücklich, dass fremde Akkus anders gepolt sein können.
- **Nie 5 V an einen Pin des XIAO.** `PGOOD` und `CHG` vor dem Anschließen mit dem Multimeter messen (müssen unter 3,3 V bleiben), sonst je 10 kΩ in Reihe. (Überholt am 09.10.2026: laut Adafruit-Schaltplan direkt anschließbar, siehe oben.)
- Ladestrom auf **0,5 A** stellen: schonend, 1800 mAh sind in vier bis fünf Stunden voll.
- Nur ein Akku mit Schutzschaltung. Im Gehäuse nicht quetschen, keine spitzen Lötstellen am Akku, etwas Luft lassen. Nicht am Körper laden.
- Zum Flashen kann weiter der USB-Anschluss des XIAO benutzt werden; dabei lädt dessen eigener Lader mit 50 mA mit, das ist unkritisch.
- Gehäuse: Loch direkt über dem Mikrofon (mit Stoff oder Schaum gegen Wind), Antenne an den Rand und weg vom Akku, USB-C der Ladeplatine nach außen, Taster von außen drückbar, Fenster für die LED, Recorder nicht starr an den Akku kleben (Körperschall).
- Grobe Größe mit 1800 mAh: etwa 60 × 40 × 22 mm, rund 60 g.

## Bezugsquellen (geprüft am 07.10.2026, noch nichts bestellt)

Preise inklusive Mehrwertsteuer, Versand kommt dazu. Tobi hat nach Links gefragt.

| Teil | Link | Preis und Stand |
|---|---|---|
| Ladeplatine Adafruit bq24074 (4755) | https://eckstein-shop.de/AdafruitUniversalUSB2FDC2FSolarLithiumIon2FPolymerchargerEN | 17,79 €, lieferbar, 1 bis 3 Tage |
| dieselbe bei BerryBase | https://www.berrybase.de/en/adafruit-universal-usb-dc-solar-lithium-ion-polymer-charger | 18,90 €, derzeit nicht lieferbar |
| LiPo 2000 mAh LP103450 mit JST-PH (PKNERGY) | https://eckstein-shop.de/LiPo-Akku-Lithium-Ion-Polymer-Batterie-37V-2000mAh-mit-JST-PHR-2-Stecker-LP103450-EN | 7,95 €, lieferbar; laut Shop mit Schutzschaltung, 50 × 34 × 10 mm, 35 g. Polung messen. |
| Alternative: EREMIT 1800 mAh 103450 | https://www.eremit.de/p/eremit-3-7v-1800mah-lipo-akku-103450 | 6,20 €, auf Lager, Schutzschaltung, JST-PH |
| Taster 6 × 6 × 6 mm, 10 Stück | https://eckstein-shop.de/10Stk6x6x6mmDIP-4miniDrucktasterEingabetasterAC250VDC12V50mAEN | Preis nicht abgelesen |
| JST-PH-2.0-Kabel, 20 Paare | https://eckstein-shop.de/2Pin-JST-PH-20-Kable-Kit-20-Paare-EN | Preis nicht abgelesen; Silikonlitze 24 AWG, 10 cm |
| Widerstände, 600 Stück, 30 Werte | https://eckstein-shop.de/600xMetallschichtMetallfilmWiderstC3A4nde02C25W20bis1M30SortenEN | 3,95 €; ob 470 kΩ dabei ist, vor dem Kauf nachsehen |
| Keramikkondensatoren, 30 Werte bis 100 nF | https://eckstein-shop.de/30x10KeramikKondensatorenSortiment2pF-100nF30Werte | 2,80 € |
| Magnetisches USB-C-Kabel (Beispiele) | https://www.amazon.de/dp/B07DDDL62C und https://www.amazon.de/dp/B0BLGSXM36 | Preise nicht geprüft; mit der Ladeplatine ausprobieren |
| Band mit Sicherheitsverschluss | https://www.amazon.de/dp/B01L6OWLX2 (10 mm) oder https://www.amazon.de/dp/B082YRQSFP (Durable) | 5,49 € bzw. 6,49 € laut Suchergebnis |

Ohne geprüften Link: Schiebeschalter, Schrumpfschlauch, Multimeter. Die Ladeplatine und der Akku kommen zusammen bei Eckstein auf 25,74 € plus Versand.

## Offen

- Echter Verbrauch mit Bluetooth und mit WLAN: messen, dann Akkugröße festlegen.
- Firmware und App (Phase 5): Akkustand, „lädt“ und „Strom liegt an“ anzeigen, Taster, LED, WLAN nur am Ladeplatz, Stromsparen.

## Quellen

- Seeed-Wiki XIAO ESP32S3 (Ladestrom, Verbrauch, 5V-Pin, Akkuflächen): https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/
- Seeed-Forum, Akku wird am eingebauten Lader nicht voll: https://forum.seeedstudio.com/t/esp32s3-battery-charging-problem/294601
- Adafruit bq24074, Artikel 4755: https://www.adafruit.com/product/4755 und https://learn.adafruit.com/adafruit-bq24074-universal-usb-dc-solar-charger-breakout

Verwandt: [[Audio-Recorder]], [[Recorder AR1 Firmware]], [[Live Up]]
