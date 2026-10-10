---
titel: "Mein Adafruit BQ24074 Charger ist da, plus solche solche ähm, JST-PH 2mm Stecker"
datum: 2026-10-09
ki: claude
session: f7354c6c-e78d-444b-9111-63963a603d64
tags: [chat]
---

# Mein Adafruit BQ24074 Charger ist da, plus solche solche ähm, JST-PH 2mm Stecker

## Verlauf

**Tobi:** Mein Adafruit BQ24074 Charger ist da, plus solche solche ähm, JST-PH 2mm Stecker. Und Draht und Büchse. Keine Ahnung. Kit. Ähm, habe ich bei Eckstein Komponente. Bestellt. Äh, das ist da. Mach mir mal bitte für den ESP32 S3 ein Schaltplan. Und äh, plan mit mir, wie ich das jetzt zusammen mache. Beziehungsweise wie ich das jetzt löten soll. Ähm, der Lipo ist noch nicht da. Der kommt aber noch. Erstens, wie programmiere ich diesen Adafruit beziehungsweise kannst du mir da ein Programm bauen. Und zweitens, wie machen wir das mit dem Löten? Und Gehäuse. Machen wir dann noch mit 3D-Drucker.

**Claude:** Hier der Schaltplan:

**Claude:** Der Schaltplan oben zeigt alles, was zwischen Ladeplatine und XIAO gelötet wird; er liegt auch im Brain als [Recorder Schaltplan.svg](Knowledge/Recorder%20Schaltplan.svg).

## Programmieren

Die Adafruit-Platine wird nicht programmiert. Der bq24074 hat keine Software, er wird nur über Lötbrücken auf der Rückseite eingestellt. Programmiert wird der XIAO.

Dafür habe ich die Recorder-Firmware auf **AR1.2** erweitert: Sie liest Akkuspannung, „Strom liegt an" und „lädt" und zeigt sie im `STATUS` an. Sie baut ohne Warnungen und die PC-Tests bestehen. Aufgespielt und am Gerät getestet ist sie noch nicht, weil der Recorder gerade nicht am PC hängt. Die alte AR1.1 liegt als Rückfall daneben.

## Löten in Stufen

Nach jeder Stufe kannst du testen, bevor du weitermachst. Bis auf das Einstecken des Akkus geht alles schon jetzt.

1. **Vor dem Löten prüfen:** USB-C an die Ladeplatine, die grüne LED leuchtet. Ein Kabel aus dem Kit in die Buchse „Load Out" stecken und messen, welche Litze Plus führt.
2. **Strom (2 Lötstellen):** Die beiden Litzen aus „Load Out" an die Akku-Lötflächen auf der Unterseite des XIAO löten. Minus ist die Fläche näher am USB-Anschluss. Danach die Litzen mit Klebeband oder Heißkleber entlasten, die Flächen reißen sonst leicht ab.
3. **Test:** USB-C an die Ladeplatine, der XIAO muss auch ohne Akku starten.
4. **Meldeleitungen (2 Drähte):** `PGOOD` an D3 und `CHG` an D4, direkt und ohne Widerstand.
5. **Akkumessung:** Pad `LIPO` über R1 an D0, von D0 gehen R2 und C nach GND. Das braucht zwei gleiche Widerstände und einen 100-nF-Kondensator; im Kit sind nur Kabel.
6. **Später mit dem Gehäuse:** Taster, LED und Schiebeschalter. Dafür gibt es noch keine Firmware.

## Worauf du achten musst

- **Den Kabelfarben nicht trauen.** Bei solchen JST-Kits sind Rot und Schwarz gegenüber Adafruit oft vertauscht. Es zählt der Aufdruck + / − an der Buchse.
- **Akku:** Wenn er da ist, erst die Polung am Stecker gegen den Aufdruck der Buchse „Lipo Batt" prüfen, dann einstecken. Beim Löten bleibt er abgesteckt, und am Akku selbst wird nie gelötet.
- **Ladestrom:** Ab Werk 1 A, das lassen wir, bis klar ist, welcher Akku kommt. Für 0,5 A musst du erst die 1-A-Brücke auftrennen und dann 0,5 A zulöten. Wer nur zulötet, bekommt rund 1,3 A statt weniger.
- Ohne Akku kann die rote LED flackern.

## Was ich von dir brauche

1. Hast du ein Multimeter? Ohne geht Schritt 1 nur nach Aufdruck und Augenmaß.
2. Hast du im Arduino-Zeug zwei gleiche Widerstände zwischen 100 kΩ und 470 kΩ, einen 100-nF-Kondensator, einen Taster und eine LED?
3. Welchen LiPo hast du bestellt? Danach legen wir den Ladestrom fest.

Steck den Recorder an den PC und sag Bescheid, dann spiele ich AR1.2 auf und teste sie. Das Gehäuse zeichnen wir, wenn der Akku da ist und der Aufbau auf dem Tisch läuft, weil erst dann die Maße feststehen.

Der ganze Plan samt erwarteten `STATUS`-Werten je Stufe steht in [Recorder Hardware Kette.md](Knowledge/Recorder%20Hardware%20Kette.md).

Quellen:
- [Adafruit bq24074 Pinbelegung](https://learn.adafruit.com/adafruit-bq24074-universal-usb-dc-solar-charger-breakout/pinouts)
- [Adafruit bq24074 Schaltplan](https://learn.adafruit.com/adafruit-bq24074-universal-usb-dc-solar-charger-breakout/downloads)
- [Seeed-Wiki XIAO ESP32S3](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/)

**Tobi:** Also, mein Arduino Kit ist the most complete Starter Kit Uno R3 Project von Lego. Also Uno R3 Project. Und ähm, ich würde gerne folgendes für das erste Mal würde ich gerne, ich weiß nicht, ob das geht. Ähm, ich habe den Lipo noch nicht, beziehungsweise noch kein Lipo. Ich würde das mit der neuen Volt-Batterie machen. Ich habe da ja so einen Anschluss. Vielleicht geht das. Ich weiß nicht, ob es geht. Vielleicht geht es, vielleicht geht es auch nicht. Ähm, du müsstest mir im Prinzip helfen. Aber erstmals auf dieser Steckplatte da, die beim Arduino mit dabei ist, das im Prinzip drauf zu bekommen. Und äh, ich glaube, hier sind auch noch Widerstände dabei. Ähm, Ja, gib mir da einfach mal eine Anleitung dazu, das drauf zu bekommen. Widerstände sind dabei. Und zwar... ...470, da steht dann immer so eine Zahl drauf. 470... ...510... 10, glaube ich. Auf den anderen steht jetzt nichts drauf. Egal, schau mal, was mitgeliefert wird. An Widerständen. Also hier sind einige mit drin. Das sind doch gar keine 10. Ja, ich, ich weiß nicht, was hier alles drin ist. Hier ist einiges an Widerstände drin. Vielleicht haben wir die auch manuell dazu gemacht. Wir haben hier auch noch LEDs und so Zeug. Und Buttons. Und Controller und so. Ja. Hilf mir einfach mal, das Ganze auf der Steckplatte aufzubauen, bevor ich da jetzt irgendwas löte. Beziehungsweise ich kann zwar schon was löten, aber ich will es so weit haben, dass ich das auch jederzeit wieder ablöten könnte. Danke.

**Claude:** Ich prüfe kurz, was in deinem Kit wirklich steckt, damit die Anleitung zu den Teilen passt.

**Claude:** Ja, das geht auf dem Steckbrett, und ohne LiPo klappt es auch. Die 9‑V‑Batterie darf aber nur über das Netzteilmodul an den XIAO (Stufe 3). Dein Kit ist das Elegoo UNO R3 Most Complete Starter Kit. Laut [Elegoo-Teileliste](https://manuals.plus/asin/B01CZTLHGE) sind darin ein Steckbrett, ein Netzteilmodul (höchstens 9 V Eingang), eine 9‑V‑Batterie mit Clip, 104‑Kondensatoren, LEDs und Jumper‑Kabel. Widerstände stehen dort nicht ausdrücklich, deine Widerstände habe ich deshalb nicht bestätigt. Die Batterie fehlt bei manchen Lieferungen.

**Nie 9 V** an die Ladeplatine, an einen Pin des XIAO oder an die Akku‑Lötflächen. Das zerstört die Chips.

## Vorher: zwei Fragen

1. **Hat dein XIAO Stiftleisten?** Wenn die Pins nicht dran sind, brauchst du sie fürs Steckbrett. Stiftleisten anlöten ist jederzeit wieder ablötbar.
2. **Liegt der Ladeplatine eine Stiftleiste bei?** Die Adafruit-Platine kommt meist mit loser Leiste.

Sag mir, was davon der Fall ist. Der Aufbau unten funktioniert aber so oder so.

## Widerstände lesen

Du brauchst zwei **gleiche** Widerstände. 10 kΩ reichen, 100 kΩ oder 470 kΩ wären besser. Mit dem Kondensator ist 10 kΩ genau genug.

| Ringe (von der Seite, wo sie dichter sitzen) | Wert |
|---|---|
| braun, schwarz, **orange** | 10 kΩ |
| braun, schwarz, **gelb** | 100 kΩ |
| gelb, violett, **gelb** | 470 kΩ |
| braun, schwarz, **rot** | 1 kΩ (nicht nehmen) |
| rot, rot, braun | 220 Ω (nicht nehmen) |

Der dritte Ring ist das Entscheidende: orange = 10 k, gelb = 100 k. Zeig mir sonst ein Foto, oder miss sie, wenn du ein Multimeter hast.

Außerdem brauchst du einen Keramikkondensator mit dem Aufdruck **104**. Der ist im Kit.

## Aufbau in Stufen

Teste nach jeder Stufe, bevor du weitermachst.

**Stufe 1: nur der XIAO.**
- XIAO über die Mittelrille stecken, die USB‑Buchse zeigt über den Rand hinaus.
- USB‑C direkt an den PC.
- Er läuft wie bisher.

**Stufe 2: Ladeplatine.**
- Auch über die Mittelrille stecken, mit etwas Abstand zum XIAO.
- Nur USB‑C an die Ladeplatine, noch nichts verbunden.
- Die grüne LED leuchtet.

**Stufe 3: Strom.**
- Jumper von `LOAD +` auf den `5V`‑Pin des XIAO und von `LOAD −` auf `GND`. Dafür muss nichts an die winzigen Akku‑Flächen gelötet werden.
- USB‑C nur an die Ladeplatine.
- Der XIAO startet auch ohne Akku.

**Stufe 4: Meldeleitungen.**
- `PGOOD` auf D3 und `CHG` auf D4, je ein Jumper.

**Stufe 5: Akkumessung.**
- Pad `LIPO` über Widerstand 1 auf D0.
- Von D0 aus Widerstand 2 nach GND, ebenso der 104‑Kondensator nach GND.
- Ohne LiPo zeigt die Messung nur einen beliebigen Wert. Das ist normal.

## Strom ohne LiPo

- **Einfachste Variante:** Ladeplatine mit einem 5‑V‑Netzteil oder der Powerbank versorgen. Die Stromaufnahme des Recorders reicht problemlos.
- **Mit der 9‑V‑Batterie:** Du brauchst das kleine Netzteilmodul, das aufs Steckbrett kommt. Den Schalter bzw. die Brücke für diese Schiene auf **5 V** stellen, nicht auf 3,3 V und nicht auf 9 V. Dann geht der 5‑V‑Ausgang an den `5V`‑Pin des XIAO und GND an GND. Das ersetzt die Ladeplatine an dieser Stelle. Die 9‑V‑Batterie hält damit nur wenige Stunden, weil der Regler die Differenz als Wärme verbrennt, und ist nur zum Testen gedacht.

**Wichtig:** Am `5V`‑Pin darf immer nur **eine** Quelle hängen.
- Vor dem Flashen per USB am XIAO erst den Jumper von `LOAD +` abziehen.
- GND darf verbunden bleiben.

## Als Nächstes

Sobald der XIAO am PC hängt, spiele ich Firmware AR1.2 auf, und `STATUS` zeigt dir den Strom‑Zustand. Gib mir bis dahin Bescheid, ob der XIAO Stifte hat und was auf deinen Widerständen steht. Die Notiz [Recorder Hardware Kette](Knowledge/Recorder%20Hardware%20Kette.md) ist ergänzt.

**Tobi:** gut mach mir einen schaltplan und eine skitze wie ich es an dem brett anbringen soll

**Claude:** Zuerst der Schaltplan, dann die Skizze fürs Steckbrett.

**Claude:** In der ersten Fassung überlappt ein Text mit der gestrichelten Leitung. Hier die korrigierte Fassung, sie ersetzt die erste.

**Claude:** Zum Schluss die Vergrößerung für Stufe 5 (Akkumessung), die sich im großen Bild nicht sauber zeichnen ließ.

**Claude:** Der Schaltplan (mit der 9‑V‑Alternative) und die Steckbrett‑Skizze für die Stufen 1 bis 4 stehen oben, dazu die Vergrößerung für Stufe 5. Ich habe sie nur gezeichnet, nicht geprüft; aufgebaut ist noch nichts.

Die erste Fassung des Schaltplans hatte eine Textüberlappung. Gilt nur die zweite Fassung.

## So liest du die Skizze

- **XIAO:** Er steckt über der Mittelrille, USB‑C zeigt nach rechts. Wenn du ihn so einsteckst, bleiben oben drei freie Löcher pro Spalte und unten zwei.
- **Ladeplatine:** Sie liegt oberhalb des Bretts und steckt nicht darin. Die Jumper gehen von ihren Pins direkt ins Brett. Die Skizze verteilt die Pins zur Übersicht. Auf deiner Platine stehen sie anders, es zählen nur die Namen auf dem Aufdruck.
- **Jumper:** Du brauchst sechs Stück, davon zwei Buchse‑Stecker‑Kabel, falls die Pins der Platine Stifte haben. Hat die Ladeplatine keine Stifte, brauchst du zuerst eine angelötete Stiftleiste. Das lässt sich später wieder entlöten.
- **Schienen:** Die beiden `−`‑Schienen sind die GND‑Leitung. Prüfe, ob deine Schiene in der Mitte durchgehend ist. Bei manchen kurzen Brettern ist sie unterbrochen und muss dann mit einem Jumper überbrückt werden.

## Reihenfolge, damit nichts kaputtgeht

1. XIAO einstecken und per USB testen, noch ohne Ladeplatine.
2. Die fünf Jumper aus der Skizze stecken, **USB‑Kabel dabei abgezogen**.
3. USB‑C an die Ladeplatine, nicht an den XIAO. Die grüne LED an der Platine leuchtet, der XIAO startet.
4. Zum Flashen erst den roten Jumper (`OUT` → `5V`) abziehen, dann USB an den XIAO stecken.
5. Stufe 5 (Teiler) kommt zuletzt, erst wenn 1 bis 4 laufen.

Ohne LiPo zeigt die Akkumessung nur einen beliebigen Wert. Das ist normal.

Wenn der XIAO am PC hängt, spiele ich Firmware AR1.2 auf, dann sehen wir im `STATUS` den Strom‑Zustand.

**Tobi:** Was mache ich denn jetzt? Also löten. Ich meine, die, der, der ESP32 hat noch keine ähm, Steckbrettstecker dran gelötet. Und der Lipo-Charger auch noch nicht. Also dieser Ladechip. Die sind aber dabei. Also kann ich machen. Ähm, dann kann ich den Lipo-Charger nicht aufs gleiche Klemmbrett machen. Ich meine, das ist ja lang. Das hat ja irgendwie keine Ahnung. sechzig, fünfundsechzig Spalten. Also Spalten lang. Und irgendwie A bis J breit. Plus Minus und Plus Minus. Also Plus, Plus Minus und das halt mal zwei. Und genau, wie, wie löte ich denn jetzt an was an was? Also nur fürs Steckbrett jetzt. Wenn ich später den Lipo bekomme, das ist vermutlich am Montag oder heute, heute noch oder morgen, ähm, wie bringe ich denn da den mit rein? Und mit was wird denn das jetzt hier genau geladen?

**Claude:** ## Löten: nur Stiftleisten, sonst nichts

Für das Steckbrett lötest du bei beiden Platinen nur die Stiftleisten an. Das lässt sich später wieder entlöten. Die Steckbrett‑Jumper kommen danach per Steckverbindung, ohne weitere Lötstelle.

Mit dem Steckbrett als Halter klappt es am einfachsten:

1. Stiftleiste abbrechen: zwei Stücke zu je 7 Pins für den XIAO, ein Stück passend zur Lochreihe der Ladeplatine.
2. **Lange Pins nach unten** ins Steckbrett stecken, die kurze Seite zeigt nach oben. Beim XIAO stehen die beiden Leisten 6 Löcher auseinander, er liegt dann über der Mittelrille.
3. Die Platine oben auflegen, sodass die Pins durch die Löcher oder Randkontakte ragen.
4. Jeden Pin von oben anlöten: 2 bis 3 Sekunden, Lötkolben gleichzeitig an Pin und Kontaktfläche, dann wenig Zinn. Erst einen Eckpin anlöten, prüfen, ob alles gerade sitzt, dann den Rest.
5. Danach mit der Lupe prüfen, dass keine Zinnbrücke zwischen zwei Pins sitzt.

**Beim XIAO Sense:** Seeed sagt nichts dazu, auf welcher Seite gelötet wird. Zieh die Erweiterungsplatine vor dem Löten ab. Schiebe sie dabei seitlich ab und ziehe sie nie gerade nach oben oder unten. Nach dem Löten steckst du sie wieder auf. Wenn die Lötstellen sie berühren, kürze die überstehenden Pinenden mit einem Seitenschneider.

Die beiden Platinen sind winzig. Ohne Brücken und ohne zu lange Hitze wird es sauber.

## Beide Platinen auf ein Brett

Dein Brett hat rund 63 Spalten, es passt locker beides darauf. Es ist größer als meine Skizze, die Spalten sind aber gleich. Eine passende Anordnung:

- **XIAO** links, in den Spalten etwa 5 bis 11.
- **Ladeplatine** weiter rechts, ab etwa Spalte 25.

Die Ladeplatine steckst du so ein, dass die Platine selbst über die Mittelrille in die untere Hälfte ragt. Dann bleiben die vier oberen Lochreihen jeder Pin‑Spalte für Jumper frei. Ich kenne die Pin‑Reihenfolge der Platine nicht, die steht auf dem Aufdruck. Du steckst dann je einen Jumper aus der freien Spalte zum Ziel:

| Pin der Ladeplatine | Ziel |
|---|---|
| `OUT` | `5V` am XIAO |
| `GND` | obere − Schiene (zum XIAO‑GND per Jumper) |
| `PGOOD` | D3 |
| `CHG` | D4 |
| `LIPO` | R1 (Stufe 5) |

Das ist elektrisch dasselbe wie in meiner Skizze, nur kürzer. Hast du die Platine gelötet, mach ein Foto vom Aufdruck, dann ordne ich dir die Jumper konkret zu.

## Der LiPo, sobald er da ist

- Der Stecker kommt in die Buchse **„Lipo Batt“** der Ladeplatine. Dafür muss nichts gelötet werden.
- **Prüfe vorher die Polung**, mit Multimeter oder Aufdruck an der Buchse. Fremde Akkus sind manchmal gegenläufig gepolt.
- Erst der Akku, dann USB‑C. Beim ersten Laden bleib dabei und lade nicht unbeaufsichtigt.
- **Ladestrom:** ab Werk 1 A. Das passt zu 1800 bis 2000 mAh. Steht auf deinem Akku ein niedrigerer Maximalwert, ändern wir die Lötbrücke.
- Zieh dann das USB‑C an der Ladeplatine ab, der Recorder läuft aus dem Akku weiter. Mit dem Aufbau am `5V`‑Pin schaltet der XIAO etwas früher ab, schon ab etwa 3,5 V Akkuspannung. Für Tests ist das in Ordnung. Endgültig kommt der Akku später an die Lötflächen unter dem XIAO.

## Womit lädt man?

Der Akku wird über den **USB‑C‑Anschluss der Ladeplatine** geladen, nicht über den XIAO. Dafür nimmst du ein 5‑V‑Netzteil mit mindestens 1 A oder eine Powerbank. Ein USB‑C‑auf‑USB‑C‑Kabel geht, im Schaltplan der Platine sitzen die nötigen 5,1‑kΩ‑Widerstände. Ein PC‑USB‑Anschluss liefert oft nur 500 mA. Ob die Platine damit sauber zurechtkommt, habe ich nicht geprüft, nimm für den ersten Versuch das Netzteil.

Bei 2000 mAh und 1 A dauert eine volle Ladung etwa zwei bis zweieinhalb Stunden. Der XIAO braucht nebenbei rund 65 mA, das verlängert es etwas. Der Strom für die Aufnahme kommt dabei aus dem Netzteil, nicht aus dem Akku.

Quelle: [Seeed XIAO ESP32S3 Wiki](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/)

**Tobi:** Okay, folgendes. Ich habe gerade bemerkt, ja, erstens der Xaeo ist nicht beschriftet. Das musst du selber machen. Zweitens, der Lipo-Charger ähm, ist in der Skizze noch draußen. Also nicht auf dem Klemmbrett. Ähm, drittens, es gibt, glaube ich, drei GNDs. auf dem Lipo-Charger. Ähm, und ich weiß nicht, wohin damit. Und viertens, soll ich das mit dem Lipo zu D0 Soll ich das dann einfach in den leeren Platz reinmachen, da wo es auf jeden Fall nicht in Kontakt kommt mit dem anderen? Und ist es egal, also soll ich einfach alle drei GNDs in den Minus-Dingsbums da reinstecken? Ich, ich weiß nicht.

**Claude:** Zu allen vier Punkten, jeweils mit dem, was ich weiß und was nicht.

## 1. Der XIAO ist nicht beschriftet

Halte ihn so, dass der **USB‑C‑Anschluss nach oben** zeigt und die Bauteile zu dir schauen. Dann gilt, von oben nach unten gelesen:

| Linke Reihe | Rechte Reihe |
|---|---|
| D0 | **5V** |
| D1 | **GND** |
| D2 | 3V3 |
| D3 | D10 |
| D4 | D9 |
| D5 | D8 |
| D6 | D7 |

Das ist die Belegung aus dem Seeed‑Pinout. Ich habe dein Board nicht in der Hand, **prüf sie deshalb nach**, bevor du etwas anschließt:

- Steck USB an den XIAO und miss mit dem Multimeter am Pin rechts oben, direkt neben dem USB‑Anschluss, gegen den Pin darunter. Dort müssen 5 V anliegen. Das sind 5V und GND.
- Wenn du kein Multimeter hast: Klebe einen Streifen Tape auf die Pins und beschrifte ihn mit Stift. Die 5V‑Seite erkennst du an der Position neben dem USB‑Anschluss.

Auf dem Steckbrett liegt der XIAO mit USB nach rechts, wie in meiner Skizze. Da gilt dieselbe Zuordnung, nur um 90 Grad gedreht.

## 2. Die Ladeplatine war in der Skizze draußen

Das war nur schematisch gezeichnet, in Wirklichkeit steckt sie mit auf dem Brett, wie im letzten Text beschrieben. Eine Skizze mit der Platine auf dem Brett kann ich dir erst zeichnen, wenn ich die Pin‑Reihenfolge kenne. Mach dafür ein Foto von der Platine mit gut lesbarem Aufdruck an den Pin‑Löchern. Dann zeichne ich dir ein Bild mit genau den richtigen Spalten.

## 3. Die drei GND‑Pins

Alle drei sind auf der Platine miteinander verbunden. **Du brauchst nur einen davon**, die anderen beiden lässt du einfach frei. Du kannst auch alle drei an die Minus‑Schiene stecken, das schadet nicht. Wichtig ist nur, dass du in der Spalte eines GND‑Pins nichts anderes einsteckst.

Auf dem Brett gibt es zwei Minus‑Schienen, oben und unten. Ein Kabel am Brettende verbindet sie. Damit ist alles ein gemeinsames Minus. Das Minus erkennst du an der **blauen** Linie, Plus ist rot. Die Plus‑Schienen brauchst du für diesen Aufbau gar nicht.

## 4. Die Akkumessung

Ja, bau sie in einen **leeren Bereich** des Bretts. Die Regel dafür:

- Die fünf Löcher einer Spalte (a bis e oder f bis j) sind innen verbunden.
- Zwischen verschiedenen Spalten ist nichts verbunden.
- Teile verbinden sich nur, wenn sie in derselben Spalte stecken.

Nimm drei freie Spalten nebeneinander, die keine andere Platine berührt:

- Spalte 1: das Kabel von `LIPO` und ein Bein von R1.
- Spalte 2: das andere Bein von R1, ein Bein von R2, ein Bein vom Kondensator und das Kabel zu D0.
- Die zweiten Beine von R2 und vom Kondensator gehen in die Minus‑Schiene.

Das ist die Vergrößerung von vorhin, nur mit zwei Spalten. Der Teiler kommt zuletzt und ist ohne LiPo noch nicht nötig.

## Nächster Schritt

Löte erst die Stiftleisten an, dann schick mir das Foto der Ladeplatine. Dann bekommst du die fertige Skizze mit allen Spalten.

**Tobi:** habs fertig was jz also was tu ich jz

**Claude:** Der XIAO hängt gerade nicht am PC. Ich gehe die nächsten Schritte der Reihe nach durch.

## Schritt 1: Prüfen, ob die Lötstellen sauber sind

Sieh dir jeden Pin an, bei beiden Platinen: Keine Zinnbrücke zwischen zwei Nachbarpins, und jeder Pin ist rundherum mit Zinn benetzt. Brücken saugst du mit Entlötlitze weg.

## Schritt 2: XIAO einzeln testen

1. Stecke den XIAO über die Mittelrille, USB‑C nach rechts, die Pins in den Reihen wie in der Skizze.
2. Stecke die Sense‑Erweiterungsplatine wieder auf (falls abgezogen). Schiebe sie dabei seitlich auf, drücke sie nicht von oben.
3. Stecke ein USB‑C‑Kabel an den XIAO und an den PC.
4. Sag mir Bescheid. Dann sehe ich, ob COM6 wieder auftaucht, und spiele die Firmware **AR1.2** auf.

Dabei darf noch **nichts anderes** am Brett hängen: keine Ladeplatine, keine Jumper.

## Schritt 3: Ladeplatine allein testen

Das geht parallel, solange der XIAO noch nicht eingesteckt ist, oder danach.

1. Stecke die Ladeplatine auf das Brett, die Platine zeigt über die Mittelrille nach unten.
2. Stecke ein USB‑C‑Kabel mit Netzteil oder Powerbank an die **Ladeplatine**.
3. Die **grüne LED** muss leuchten. Ohne Akku kann die rote LED flackern, das ist normal.

## Schritt 4: Foto schicken

Mach ein Foto von der Ladeplatine, auf dem der Aufdruck an den Pins gut lesbar ist. Dann zeichne ich dir die Skizze mit den richtigen Spalten, und du steckst die Jumper nach dem Plan.

Fang mit Schritt 2 an und melde dich, sobald der XIAO am PC hängt.

**Tobi:** <local-command-caveat>The command below was run directly in Claude Code, not sent to you as a request, and its output goes straight to the user. It's recorded here as context for later messages.</local-command-caveat>

**Tobi:** <local-command-stdout>Set model to `claude-opus-5-5`</local-command-stdout>

**Tobi:** sind 100k wiederstände btw

Flashe und mach alles ready lipocharcher macht grün

**Tobi:** [Image: source: C:\Users\a\AppData\Local\Temp\claude\C--Users-a-Documents-AIs-Room\3847ddcb-e555-407f-83d3-b1514a05ef78\images\1.jpg]
[Image: source: C:\Users\a\AppData\Local\Temp\claude\C--Users-a-Documents-AIs-Room\3847ddcb-e555-407f-83d3-b1514a05ef78\images\2.jpg]
[Image: source: C:\Users\a\AppData\Local\Temp\claude\C--Users-a-Documents-AIs-Room\3847ddcb-e555-407f-83d3-b1514a05ef78\images\3.jpg]
[Image: source: C:\Users\a\AppData\Local\Temp\claude\C--Users-a-Documents-AIs-Room\3847ddcb-e555-407f-83d3-b1514a05ef78\images\4.jpg]

**Claude:** Der Recorder hängt am PC und antwortet (AR1.1, Karte und WLAN in Ordnung). Ich prüfe noch kurz den Spannungsteiler auf deinen Fotos, dann spiele ich AR1.2 auf und lese die Werte aus.

**Claude:** AR1.2 ist aufgespielt und läuft; der Selbsttest besteht. Gerade laufen die Gerätetests (rund 6 Minuten), danach bekommst du das Ergebnis und die ersten Strom‑Messwerte.

**Claude:** AR1.2 ist aufgespielt und läuft, aber die Strom‑Messung zeigt noch nichts an: `power` steht auf `unknown`, die Akkuspannung liegt bei 0 V. Die Gerätetests laufen gerade noch (rund 6 Minuten), das Ergebnis melde ich, sobald sie durch sind.

## Was die Messung sagt

- **D0 liegt sauber auf Minus.** Der Spannungsteiler hängt also dran, sonst würde der Wert herumspringen.
- **`PGOOD` kommt nicht an, und an `LIPO` liegt keine Spannung.** Am wahrscheinlichsten war der Lader beim Messen stromlos, weil das USB‑Kabel am XIAO hing. Bestätigt ist das nicht.

## Was ich von dir brauche

1. **Leuchtet die grüne LED am Lader jetzt gerade?** Wenn nicht: Steck ein zweites USB‑C‑Kabel mit Netzteil oder Powerbank an den Lader, der XIAO bleibt am PC. Ohne Akku ist das unbedenklich. Sag Bescheid, dann lese ich neu aus.
2. **Sind am Lader alle elf Pins gelötet?** Auf deinen Fotos sehen die oberen drei (`VBUS`, `GND`, `THERM`) ungelötet aus, und genau dort steckt dein schwarzes GND‑Kabel. Falls sie ungelötet sind: Steck das schwarze Kabel in die Reihe des `GND`‑Pins zwischen `OUT` und `LIPO`.

## Was auf den Fotos stimmt

- **Lader:** grün an `PGOOD`, rot an `CHG`, rot an `OUT`, rot an `LIPO`.
- **XIAO:** rot an 5V und schwarz an GND, beide am USB‑Ende links. Schwarz von D0 geht zum Teiler.
- **Teiler:** Beide Widerstände und der Kondensator gehen in die Minus‑Schiene, und die 100 kΩ passen.
- **Nicht sicher ablesbar:** ob grün und rot am XIAO genau auf D3 und D4 sitzen. Das zeigt die nächste Messung für `PGOOD`. `CHG` lässt sich erst mit dem LiPo prüfen.

## Wichtig, bevor der LiPo kommt

Solange das rote Kabel von `OUT` am `5V`‑Pin steckt, darf der XIAO **nicht am PC hängen, wenn ein LiPo eingesteckt ist**. Hat der Lader dann keinen eigenen Strom, schaltet er den Akku direkt auf `OUT`, und die 5 V vom PC lägen ungeregelt am Akku. Das kann den Akku überladen.

Zieh also vor jedem Anstecken an den PC erst das rote Kabel. Sauberer ist die endgültige Verdrahtung, sobald der Akku da ist: ein JST‑Kabel aus dem Kit von der Buchse „Load Out" an die beiden Akku‑Lötflächen unter dem XIAO, dann fällt das rote Kabel ganz weg. Das zeige ich dir, wenn es so weit ist.

Der Stand steht im Brain unter [Recorder Hardware Kette](Knowledge/Recorder%20Hardware%20Kette.md).

**Tobi:** Ich hab für das flashen alle pins von esp 32 weg soll ich sie wieder dran machen und was soll ich genau machen

**Claude:** Ja, steck sie wieder an. Für das Flashen hätte nur das rote Kabel von `OUT` weg müssen, die anderen dürfen bleiben. Das erklärt auch die Messung von vorhin: Es hing gar nichts am XIAO. Meine Aussage, der Teiler hänge dran, war deshalb falsch.

**Warte noch etwa zwei Minuten und lass das Brett so lange in Ruhe.** Die Gerätetests laufen gerade noch, der Recorder startet dabei mehrmals neu. Ich melde mich, wenn sie durch sind.

## Danach, in dieser Reihenfolge

1. Zieh beide USB‑Kabel ab, am XIAO und am Lader. Kabel umstecken nur ohne Strom.
2. Steck vier Kabel wieder an. **Das rote von `OUT` bleibt vorerst weg**, solange der XIAO am PC hängt:

| Kabel | kommt von | geht an den XIAO | Loch am Brett |
|---|---|---|---|
| schwarz | Minus‑Schiene | GND | Reihe 11, links (a bis c) |
| schwarz | Spannungsteiler | D0 | Reihe 12, rechts (i oder j) |
| grün | `PGOOD` am Lader | D3 | Reihe 9, rechts |
| rot | `CHG` am Lader | D4 | Reihe 8, rechts |

3. Steck den XIAO per USB an den PC und den Lader an ein Netzteil oder eine Powerbank. Die grüne LED am Lader muss leuchten.
4. Sag Bescheid, dann lese ich die Werte aus. Stimmt alles, steht dort `external` und eine Spannung um 4 V.

Die Reihen habe ich von deinem Foto abgelesen: Reihe 12 ist die letzte Reihe mit einem XIAO‑Pin, direkt unter der SD‑Karte am USB‑Ende. Zähl kurz nach, ob das bei dir stimmt. Wenn der XIAO eine Reihe versetzt sitzt, verschiebt sich alles mit.

## Noch offen von vorhin

Sind am Lader alle elf Pins gelötet? Auf den Fotos sehen die oberen drei ungelötet aus, und dort steckt dein schwarzes GND‑Kabel zur Minus‑Schiene. Falls ja: Steck es in die Reihe des `GND`‑Pins zwischen `OUT` und `LIPO`, der ist sicher gelötet.

Das rote Kabel von `OUT` an 5V (Reihe 12, links) kommt erst im nächsten Schritt dran, wenn der Recorder ohne PC nur am Lader laufen soll.

**Tobi:** <task-notification>
<task-id>bywqu3mj1</task-id>
<tool-use-id>toolu_018jtvaMS1gi8Hoc7Fo9tvup</tool-use-id>
<output-file>C:\Users\a\AppData\Local\Temp\claude\C--Users-a-Documents-AIs-Room\3847ddcb-e555-407f-83d3-b1514a05ef78\tasks\bywqu3mj1.output</output-file>
<status>completed</status>
<summary>Background command "Run the device tests after flashing" completed (exit code 0)</summary>
</task-notification>

**Claude:** Die Gerätetests sind durch und bestanden. Ich trage das Ergebnis noch im Brain nach, dann bekommst du die Zusammenfassung.

**Claude:** Die Gerätetests sind durch und alle bestanden: AR1.2 nimmt auf wie vorher, auch nach Reset, Tiefschlaf und Kartenfehler. Der Recorder hängt wieder am PC und ist im Leerlauf.

Die Strom‑Messung ist damit noch nicht geprüft. Ohne Kabel am XIAO zeigt sie weiter `unknown`, das ist in diesem Zustand richtig.

Du kannst jetzt loslegen, wie in meiner letzten Nachricht beschrieben:

1. Beide USB‑Kabel abziehen.
2. Die vier Kabel anstecken (GND, D0, `PGOOD`, `CHG`). Das rote von `OUT` bleibt weg.
3. XIAO an den PC, Lader ans Netzteil, grüne LED muss leuchten.
4. Mir Bescheid sagen, dann lese ich aus.

Bei den Tests sind acht kurze Aufnahmen aus deinem Zimmer entstanden, zusammen wenige MB. Sie liegen auf der Karte und als Kopie unter `A:/Recorder/firmware/ar1/captures/`. Sag, wenn ich sie löschen soll.

**Tobi:** done

**Tobi:** Hab nd geändert

**Claude:** In den letzten vier Minuten hat sich nichts geändert: Der Recorder sieht weiter das Brummen statt einer Spannung, `PGOOD` kommt nicht an. Das schwarze Kabel steckt also noch am alten Platz, oder das Umstecken hat nicht geholfen.

## Was feststeht

- **Firmware AR1.2 läuft**, alle Gerätetests bestanden. Danach habe ich noch einen Diagnose‑Befehl nachgeflasht, mit dem ich die Pins direkt sehe. Nach diesem zweiten Flashen liefen die Gerätetests nicht noch einmal.
- **Spannungsteiler und D0 funktionieren.** Als du am Brett warst, standen kurz 4,18 V da.
- **Die Powerbank taugt für diesen Test nicht**, vermutlich weil sie ohne Last wieder abschaltet. Nimm ein Netzteil oder einen zweiten USB‑Anschluss am PC.

## Was noch nicht geht

1. **Minus‑Verbindung zwischen Lader und XIAO.** Sie fehlt oder wackelt. Das ist meine Deutung aus den Messwerten, bestätigt ist sie erst, wenn nach dem Umstecken ein ruhiger Wert steht.
2. **`PGOOD` an D3.** Auch in dem kurzen Moment mit gutem Kontakt kam das Signal nicht an. Das kann am selben Fehler hängen oder ein zweiter sein.

## Dein nächster Schritt

Steck das schwarze Kabel am Lader in die freie Reihe direkt neben dem roten `LIPO`‑Kabel. Schreib mir dann kurz, und sag mir bitte auch:

- Haben die oberen drei Pins am Lader (`VBUS`, `GND`, `THERM`) Lötzinn wie die anderen?
- Woran hängt der Lader gerade: Netzteil, PC oder Powerbank?

Wenn `PGOOD` danach immer noch fehlt, suchen wir die richtige Reihe am XIAO mit einem einzelnen Draht. Das dauert eine Minute, ich sehe dabei live, welchen Pin du triffst.

Der Stand steht im Brain unter [Recorder Hardware Kette](Knowledge/Recorder%20Hardware%20Kette.md).

**Tobi:** machen wir jz mal mit Out

**Claude:** Du bekommst es als Bild. Ich markiere dir den einen Handgriff direkt auf deinem Foto und zeichne dazu einen Steckplan für alle Kabel.

**Claude:** Oben siehst du zwei Bilder: dein eigenes Foto mit dem einen Handgriff markiert und einen Steckplan für alle Kabel.

## Jetzt zu tun

1. **Das schwarze Kabel am Lader umstecken**, wie im Foto: oben raus (rot markiert), in das freie Loch neben dem `GND`‑Pin zwischen `OUT` und `LIPO` rein (grün markiert). Das andere Ende bleibt in der Minus‑Schiene.
2. **Schreib mir „umgesteckt".** Dann lese ich aus und sage dir, ob Minus passt.
3. **Erst danach** kommt das rote Kabel von `OUT` an 5V (Reihe 12 links).

Das rote `OUT`‑Kabel bitte wirklich erst nach meinem Okay. Ohne Minus‑Verbindung würde der Strom über die Signalpins zurückfließen und kann den XIAO beschädigen.

## Zum Abgleichen

Im Steckplan steht für jedes Kabel, an welchem Pin es am Lader sitzt und in welcher Reihe am XIAO. Vergleich deine Kabel damit, vor allem grün (Reihe 9 rechts) und das rote von `CHG` (Reihe 8 rechts).

Die Reihen 6 bis 12 habe ich von deinem Foto abgelesen. Reihe 12 ist die letzte Reihe mit einem XIAO‑Pin, unter der SD‑Karte. Sitzt dein XIAO eine Reihe anders, verschiebt sich alles um eine Reihe.

Auf dem Foto habe ich die Stelle des schwarzen Steckers nicht pixelgenau erkannt, deshalb ist die rote Markierung etwas größer. Gemeint ist das einzige schwarze Kabel, das am Lader steckt.

**Tobi:** umgesteckt

**Claude:** Das Umstecken hat etwas gebracht: Das Brummen ist weg, D0 liegt jetzt ruhig bei 0 V. Vom Lader selbst kommt aber weiterhin nichts an, weder Spannung an `LIPO` noch das `PGOOD`‑Signal. Auch in den letzten vier Minuten nicht.

Das sieht so aus, als hätte der Lader gerade keinen Strom. Sicher weiß ich es nicht, weil ich die LEDs nicht sehe.

Der XIAO selbst läuft stabil, daran liegt es nicht.

**Mach mir bitte ein Foto:** das ganze Brett von oben, so wie es jetzt ist, mit eingestecktem Strom am Lader. Wichtig ist, dass man die Lichter am Lader und alle Kabel‑Enden sieht.

Und sag mir dazu:

- Leuchtet am Lader gerade ein Licht, und in welcher Farbe?
- Woran hängt sein USB‑Kabel: Netzteil, PC oder Powerbank?

Das rote `OUT`‑Kabel bleibt bitte noch weg, bis `PGOOD` und die Spannung ankommen.

**Tobi:** Lest dir mal im Internet, also recherchiere mal. Könnte es vielleicht sein, dass erst ein angeschlossener Repo, also nicht nur ähm, das Micro-USB-C-Kabel nee, ähm, das, diese Spannung da drüber gibt, sondern einfach nur der Repo, also nur der eingesteckte Repo. Repo, L-I-P-O. Nicht Repo.

**Claude:** Nein, am fehlenden LiPo liegt es nicht, mit einer Einschränkung. Ich habe es im TI‑Datenblatt des Ladechips nachgelesen.

## Was auch ohne LiPo funktionieren muss

- **`PGOOD` (grünes Licht):** Der Chip meldet es, sobald am USB‑Eingang eine gültige Spannung anliegt. Ein Akku ist dafür nicht nötig.
- **`OUT`:** Liefert mit USB allein rund 4,4 V. TI schreibt ausdrücklich, dass das System auch mit fehlendem oder defektem Akku laufen soll.

## Was ohne LiPo anders ist

- **`LIPO`:** Hier hast du teilweise recht. Ohne Akku liegt dort keine feste Spannung. Der Chip sucht im Viertelsekundentakt nach einem Akku, der Pin springt dabei zwischen etwa 0 und 4,2 V hin und her. Eine echte Akkuspannung gibt es erst mit LiPo.
- **`CHG` (rotes Licht):** Bleibt ohne Akku aus. Das lässt sich erst mit dem LiPo prüfen.

## Was das für uns heißt

Genau dieses Springen zwischen 0 und 4,2 V habe ich vorhin für ein paar Sekunden gemessen. In diesem Moment war der Lader also versorgt, und Minus und Spannungsteiler haben funktioniert. Daraus folgen zwei Dinge:

1. **Im Moment hat der Lader keinen Strom.** D0 liegt ruhig bei 0 V. Wäre er versorgt, würde ich das Springen sehen.
2. **Das grüne Kabel verbindet `PGOOD` nicht mit D3.** Selbst in den Sekunden, in denen alles andere stimmte, kam `PGOOD` nicht an. Ein Ende sitzt falsch.

## Nächster Schritt

1. Prüf das grüne Kabel gegen den Steckplan: am Lader am Pin `PGOOD` (direkt neben `CHG`, Richtung USB‑Buchse), am XIAO in Reihe 9 rechts.
2. Gib dem Lader Strom vom Netzteil, bis die grüne LED leuchtet.
3. Schreib mir, dann lese ich aus.

Quellen:
- [TI‑Datenblatt BQ24074](https://www.ti.com/lit/ds/symlink/bq24074.pdf)
- [TI‑Forum: BAT‑Pin ohne Akku](https://e2e.ti.com/support/power-management/f/196/t/910148)
- [TI‑Forum: Akku‑Erkennung beim BQ24073](https://e2e.ti.com/support/power-management-group/power-management/f/power-management-forum/991137/bq24073-about-battery-detection-mode)
- [Adafruit, Produktseite 4755](https://www.adafruit.com/product/4755)

**Tobi:** look again hab überprüft

**Claude:** Die Live‑Anzeige läuft und zeigt im Moment unverändert `0 V`, `PGOOD nein`, `Pruefdraht offen`. Der Recorder selbst ist in Ordnung, er hängt am USB und ist im WLAN erreichbar.

Jetzt bist du dran, in deinem Tempo:

1. Lader mit Strom versorgen, bis seine grüne LED leuchtet, und in der Anzeige schauen, was bei D0 steht.
2. Mit dem Prüfdraht aus Reihe 11 rechts die beiden Minus‑Schienen antippen, wie oben beschrieben.

Schreib mir danach einfach, was in der Anzeige stand. Ich kann die Zeilen auch selbst nachlesen.

Die Anzeige belegt den USB‑Anschluss des Recorders. Solange sie läuft, kann ich nicht parallel auslesen oder flashen. Du beendest sie mit Strg+C im [Terminal‑Tab](#terminal/c1).

**Tobi:** gehts auch ohne PGOOD? bzw was macht des und wie fixen wir des?

Schau naochmals nach dem Schaltplan wie des dingf genau funktionier brauch man des ?

**Tobi:** # Workflow authoring reference

A workflow structures work across many agents — to be comprehensive (decompose and cover in parallel), to be confident (independent perspectives and adversarial checks before committing), or to take on scale one context can't hold (migrations, audits, broad sweeps). The script is where you encode that structure: what fans out, what verifies, what synthesizes.

When you do call it, the right move is often **hybrid**: scout inline first (list the files, find the channels, scope the diff) to discover the work-list, then call Workflow to pipeline over it. You don't need to know the shape before the *task* — only before the *orchestration step*.

Common single-phase workflows you can chain across turns:
- **Understand** — parallel readers over relevant subsystems → structured map
- **Design** — judge panel of N independent approaches → scored synthesis
- **Review** — dimensions → find → adversarially verify (the review-changes example)
- **Research** — multi-modal sweep → deep-read → synthesize
- **Migrate** — discover sites → transform each (worktree isolation) → verify

For larger work, run several in sequence — read each result before deciding the next phase. You stay in the loop; each workflow is one well-scoped fan-out.

**Ultracode.** When a system-reminder confirms ultracode is on, that opt-in is standing: author and run a workflow for every substantive task by default. The goal is the most exhaustive, correct answer you can produce — token cost is not a constraint. For multi-phase work (understand → design → implement → review), that often means several workflows in sequence — one per phase — so you stay in the loop between them. The quality patterns below (adversarial verify, multi-modal sweep, completeness critic, loop-until-dry) are the tools; pick what fits the task. Lean toward orchestrating with workflows and adversarially verifying your findings — unless the work is trivial or already verified. Solo only on conversational turns or trivial mechanical edits. When a reminder says ultracode is off, revert to the opt-in rule in the Workflow tool description.

Pass the script inline via `script` — do not Write it to a file first. Every invocation automatically persists its script to a file under the session directory and returns the path in the tool result. To iterate on a workflow, edit that file with Write/Edit and re-invoke Workflow with `{scriptPath: "<path>"}` instead of resending the full script.

Every script must begin with `export const meta = {...}`:
  export const meta = {
    name: 'find-flaky-tests',
    description: 'Find flaky tests and propose fixes',   // one-line, shown in permission dialog
    phases: [                                            // one entry per phase() call
      { title: 'Scan', detail: 'grep test logs for retries' },
      { title: 'Fix', detail: 'one agent per flaky test' },
    ],
  }
  // script body starts here — use agent()/parallel()/pipeline()/phase()/log()
  phase('Scan')
  const flaky = await agent('grep CI logs for retry markers', {schema: FLAKY_SCHEMA})
  ...

The `meta` object must be a PURE LITERAL — no variables, function calls, spreads, or template interpolation. Required fields: `name`, `description`. Optional: `whenToUse` (shown in the workflow list), `phases`. Use the SAME phase titles in meta.phases as in phase() calls — titles are matched exactly; a phase() call with no matching meta entry just gets its own progress group. Add `model` to a phase entry when that phase uses a specific model override.

Script body hooks:
- agent(prompt: string, opts?: {label?: string, phase?: string, schema?: object, model?: string, effort?: string, isolation?: 'worktree', agentType?: string}): Promise<any> — spawn a subagent. Without schema, returns its final text as a string. With schema (a JSON Schema), the subagent is forced to call a StructuredOutput tool and agent() returns the validated object — no parsing needed. Returns null if the user skips the agent mid-run or the subagent dies on a terminal API error after retries (filter with .filter(Boolean)). opts.label overrides the display label. opts.phase explicitly assigns this agent to a progress group (use this inside pipeline()/parallel() stages to avoid races on the global phase() state — same phase string → same group box). opts.model overrides the model for this agent call. Default to omitting it — the agent inherits the main-loop model (the resolved session model), which is almost always correct. Only set it when you're highly confident a different tier fits the task; when unsure, omit. opts.effort overrides the reasoning effort for this agent call ('low' | 'medium' | 'high' | 'xhigh' | 'max') — omit to inherit the session effort; use 'low' for cheap mechanical stages and higher tiers only for the hardest verify/judge stages. opts.isolation: 'worktree' runs the agent in a fresh git worktree — EXPENSIVE (~200-500ms setup + disk per agent), use ONLY when agents mutate files in parallel and would otherwise conflict; the worktree is auto-removed if unchanged. opts.agentType uses a custom subagent type (e.g. 'general-purpose', 'code-reviewer') instead of the default workflow subagent — resolved from the same registry as the Agent tool; composes with schema (the custom agent's system prompt gets a StructuredOutput instruction appended).
- pipeline(items, stage1, stage2, ...): Promise<any[]> — run each item through all stages independently, NO barrier between stages. Item A can be in stage 3 while item B is still in stage 1. This is the DEFAULT for multi-stage work. Wall-clock = slowest single-item chain, not sum-of-slowest-per-stage. Every stage callback receives (prevResult, originalItem, index) — use originalItem/index in later stages to label work without threading context through stage 1's return value. A stage that throws drops that item to `null` and skips its remaining stages.
- parallel(thunks: Array<() => Promise<any>>): Promise<any[]> — run tasks concurrently. This is a BARRIER: awaits all thunks before returning. A thunk that throws (or whose agent errors) resolves to `null` in the result array — the call itself never rejects, so `.filter(Boolean)` before using the results. Use ONLY when you genuinely need all results together.
- log(message: string): void — emit a progress message to the user (shown as a narrator line above the progress tree)
- phase(title: string): void — start a new phase; subsequent agent() calls are grouped under this title in the progress display
- args: any — the value passed as Workflow's `args` input, verbatim (undefined if not provided). Pass arrays/objects as actual JSON values in the tool call, NOT as a JSON-encoded string — `args: ["a.ts", "b.ts"]`, not `args: "[\"a.ts\", ...]"` (a stringified list reaches the script as one string, so `args.filter`/`args.map` throw). Use this to parameterize named workflows — e.g. pass a research question, target path, or config object directly instead of via a side-channel file.
- budget: {total: number|null, spent(): number, remaining(): number} — the turn's token target from the user's "+500k"-style directive. `budget.total` is null if no target was set. `budget.spent()` returns output tokens spent this turn across the main loop and all workflows — the pool is shared, not per-workflow. `budget.remaining()` returns `max(0, total - spent())`, or `Infinity` if no target. The target is a HARD ceiling, not advisory: once `spent()` reaches `total`, further `agent()` calls throw. Use for dynamic loops: `while (budget.total && budget.remaining() > 50_000) { ... }`, or static scaling: `const FLEET = budget.total ? Math.floor(budget.total / 100_000) : 5`.
- workflow(nameOrRef: string | {scriptPath: string}, args?: any): Promise<any> — run another workflow inline as a sub-step and return whatever it returns. Pass a name to invoke a saved workflow (same registry as {name: "..."}), or {scriptPath} to run a script file you Wrote earlier. The child shares this run's concurrency cap, agent counter, abort signal, and token budget — its agents appear under a "▸ name" group in /workflows and its tokens count toward budget.spent(). The args param becomes the child's `args` global. Nesting is one level only: workflow() inside a child throws. Throws on unknown name / unreadable scriptPath / child syntax error; catch to handle gracefully.

Subagents are told their final text IS the return value (not a human-facing message), so they return raw data. For structured output, use the schema option — validation happens at the tool-call layer so the model retries on mismatch.
Schemas need {type: 'object', properties: {...}} at root and required ⊆ properties; unsatisfiable ones throw at agent().

Workflow agents can reach all session-connected MCP tools via ToolSearch — schemas load on demand per agent. Caveat: interactively-authenticated MCP servers (e.g. claude.ai) may be absent in headless/cron runs.

Subagents get the same CLAUDE.md files injected at start that you did (except built-in agent types that omit them, such as Explore and Plan) — don't tell them to re-read those or paste their rules into the prompt; name the specific rule a stage needs, if any.

Scripts are plain JavaScript, NOT TypeScript — type annotations (`: string[]`), interfaces, and generics fail to parse. The script body runs in an async context — use await directly. Standard JS built-ins (JSON, Math, Array, etc.) are available — EXCEPT `Date.now()`/`Math.random()`/argless `new Date()`, which throw (they would break resume); pass timestamps in via `args`, stamp results after the workflow returns, and for randomness vary the agent prompt/label by index. No filesystem or Node.js API access.

DEFAULT TO pipeline(). Only reach for a barrier (parallel between stages) when you genuinely need ALL prior-stage results together.

A barrier is correct ONLY when stage N needs cross-item context from all of stage N-1:
- Dedup/merge across the full result set before expensive downstream work
- Early-exit if the total count is zero ("0 bugs found → skip verification entirely")
- Stage N's prompt references "the other findings" for comparison

A barrier is NOT justified by:
- "I need to flatten/map/filter first" — do it inside a pipeline stage: pipeline(items, stageA, r => transform([r]).flat(), stageB)
- "The stages are conceptually separate" — that's what pipeline() models. Separate stages ≠ synchronized stages.
- "It's cleaner code" — barrier latency is real. If 5 finders run and the slowest takes 3× the fastest, a barrier wastes 2/3 of the fast finders' idle time.

Smell test: if you wrote
  const a = await parallel(...)
  const b = transform(a)        // flatten, map, filter — no cross-item dependency
  const c = await parallel(b.map(...))
that middle transform doesn't need the barrier. Rewrite as a pipeline with the transform inside a stage. When in doubt: pipeline.

Concurrent agent() calls are capped at min(16, available CPUs - 2) per workflow — excess calls queue and run as slots free up. You can still pass 100 items to parallel()/pipeline() and they all complete; only ~10 run at any moment. Total agent count across a workflow's lifetime is capped at 1000 — a runaway-loop backstop set far above any real workflow. A single parallel()/pipeline() call accepts at most 4096 items; passing more is an explicit error, not a silent truncation.

When a barrier IS correct — dedup across all findings before expensive verification:
  const all = await parallel(DIMENSIONS.map(d => () => agent(d.prompt, {schema: FINDINGS_SCHEMA})))
  const deduped = dedupeByFileAndLine(all.filter(Boolean).flatMap(r => r.findings))  // <-- genuinely needs ALL at once
  const verified = await parallel(deduped.map(f => () => agent(verifyPrompt(f), {schema: VERDICT_SCHEMA})))

Loop-until-count pattern — accumulate to a target:
  const bugs = []
  while (bugs.length < 10) {
    const result = await agent("Find bugs in this codebase.", {schema: BUGS_SCHEMA})
    bugs.push(...result.bugs)
    log(`${bugs.length}/10 found`)
  }

Loop-until-budget pattern — scale depth to the user's "+500k" directive. Guard on budget.total: with no target set, remaining() is Infinity and the loop would run straight to the 1000-agent cap.
  const bugs = []
  while (budget.total && budget.remaining() > 50_000) {
    const result = await agent("Find bugs in this codebase.", {schema: BUGS_SCHEMA})
    bugs.push(...result.bugs)
    log(`${bugs.length} found, ${Math.round(budget.remaining()/1000)}k remaining`)
  }

Composing patterns — exhaustive review (find → dedup vs seen → diverse-lens panel → loop-until-dry):
  const seen = new Set(), confirmed = []
  let dry = 0
  while (dry < 2) {                                              // loop-until-dry
    const found = (await parallel(FINDERS.map(f => () =>          // barrier: collect all finders this round
      agent(f.prompt, {phase: 'Find', schema: BUGS})))).filter(Boolean).flatMap(r => r.bugs)
    const fresh = found.filter(b => !seen.has(key(b)))           // dedup vs ALL seen — plain code, not an agent
    if (!fresh.length) { dry++; continue }
    dry = 0; fresh.forEach(b => seen.add(key(b)))
    const judged = await parallel(fresh.map(b => () =>           // every fresh bug judged concurrently...
      parallel(['correctness','security','repro'].map(lens => () =>   // ...each by 3 distinct lenses
        agent(`Judge "${b.desc}" via the ${lens} lens — real?`, {phase: 'Verify', schema: VERDICT})))
        .then(vs => ({ b, real: vs.filter(Boolean).filter(v => v.real).length >= 2 }))))
    confirmed.push(...judged.filter(v => v.real).map(v => v.b))
  }
  return confirmed
  // dedup vs `seen`, NOT `confirmed` — else judge-rejected findings reappear every round and it never converges.

Quality patterns — common shapes; pick by task and compose freely:
- Adversarial verify: spawn N independent skeptics per finding, each prompted to REFUTE. Kill if ≥majority refute. Prevents plausible-but-wrong findings from surviving.
    const votes = await parallel(Array.from({length: 3}, () => () =>
      agent(`Try to refute: ${claim}. Default to refuted=true if uncertain.`, {schema: VERDICT})))
    const survives = votes.filter(Boolean).filter(v => !v.refuted).length >= 2
- Perspective-diverse verify: when a finding can fail in more than one way, give each verifier a distinct lens (correctness, security, perf, does-it-reproduce) instead of N identical refuters — diversity catches failure modes redundancy can't.
- Judge panel: generate N independent attempts from different angles (e.g. MVP-first, risk-first, user-first), score with parallel judges, synthesize from the winner while grafting the best ideas from runners-up. Beats one-attempt-iterated when the solution space is wide.
- Loop-until-dry: for unknown-size discovery (bugs, issues, edge cases), keep spawning finders until K consecutive rounds return nothing new. Simple counters (while count < N) miss the tail.
- Multi-modal sweep: parallel agents each searching a different way (by-container, by-content, by-entity, by-time). Each is blind to what the others surface; useful when one search angle won't find everything.
- Completeness critic: a final agent that asks "what's missing — modality not run, claim unverified, source unread?" What it finds becomes the next round of work.
- No silent caps: if a workflow bounds coverage (top-N, no-retry, sampling), `log()` what was dropped — silent truncation reads as "covered everything" when it didn't.

Scale to what the user asked for. "find any bugs" → a few finders, single-vote verify. "thoroughly audit this" or "be comprehensive" → larger finder pool, 3–5 vote adversarial pass, synthesis stage. When unsure, lean toward thoroughness for research/review/audit requests and toward brevity for quick checks.

These patterns aren't exhaustive — compose novel harnesses when the task calls for it (tournament brackets, self-repair loops, staged escalation, whatever fits).

Use this tool for multi-step orchestration where control flow should be deterministic (loops, conditionals, fan-out) rather than model-driven.

## Resume

The tool result includes a runId. To resume after a pause, kill, or script edit, relaunch with Workflow({scriptPath, resumeFromRunId}) — the longest unchanged prefix of agent() calls returns cached results instantly; the first edited/new call and everything after it runs live. Same script + same args → 100% cache hit. Before diagnosing why a completed workflow returned an empty or unexpected result, Read <transcriptDir>/journal.jsonl — it records each agent's actual return value; do not assume cached results are non-empty. Date.now()/Math.random()/new Date() are unavailable in scripts (they would break this) — stamp results after the workflow returns, or pass timestamps via args. Fallback when no journal is available: Read agent-<id>.jsonl files in the transcript directory and hand-author a continuation script.

**Claude:** Der Stand ist im Brain gesichert. Die vier Agenten arbeiten noch, das Ergebnis mit dem Plan für `PGOOD` und den nächsten Schritt kommt, sobald sie durch sind.

**Tobi:** <task-notification>
<task-id>wck81akcq</task-id>
<tool-use-id>toolu_01AGeAA5Pz5hyAorUq5VWEk1</tool-use-id>
<output-file>C:\Users\a\AppData\Local\Temp\claude\C--Users-a-Documents-AIs-Room\3847ddcb-e555-407f-83d3-b1514a05ef78\tasks\wck81akcq.output</output-file>
<status>completed</status>
<summary>Dynamic workflow "Verify charger pinout, bq24074 behaviour and XIAO power path, then adversarially review the next wiring steps" completed</summary>
<result>{"research":[{"key":"pinorder","result":{"summary":"The board file was fetched and parsed successfully, and it confirms your silkscreen reading in all 11 positions. From the USB-C end to the JST end the header is: VBUS, GND, THERM, ISET, CE, PGOOD, CHG, OUT, GND, LIPO, GND.\n\nFour sources agree: the .brd pad coordinates and signal names, the .sch pin-to-net mapping, Adafruit's rendered schematic (\"rev B\", 9/14/20), and Adafruit's top-view product photo. PGOOD is the exact centre pad (6th from either end); CHG is the 7th from the USB-C end.\n\n- **Status LEDs:** OUT -&gt; green LED D1 -&gt; 10 kOhm R11 -&gt; PGOOD net, and OUT -&gt; red LED D3 -&gt; 10 kOhm R7 -&gt; CHG net, as you believed. The header pads sit directly on IC pins 7 and 9 with no series part.\n- **Jumper defaults:** 1.0A closed (0.5A and 1.5A open), THERM closed (10 kOhm from TS to GND), EN1 low and EN2 high via cuttable traces on the bottom side.\n- **Ground:** all GND pads are one net. The USB-C shell tabs and the plated mounting holes are on no net in the CAD files.\n\nBearing on your measurement (inference, not measured): with valid input power the PGOOD pad should be pulled to GND and the green \"GOOD\" LED lit, so GPIO4 reading HIGH points to the wire not actually reaching the PGOOD pad. CHG reading HIGH with no battery is expected.\n\nFiles are in C:\\Users\\a\\AppData\\Local\\Temp\\claude\\C--Users-a-Documents-AIs-Room\\3847ddcb-e555-407f-83d3-b1514a05ef78\\scratchpad\\ (bq_dl\\Adafruit_BQ24074.brd and .sch, bq_ds\\bq24074.pdf, bq_img\\4755_top.jpg, bq_img2\\schematic.png; parser scripts in bq_scripts\\). Nothing was written to the vault.","findings":[{"question":"Could the board file be fetched and parsed?","answer":"Yes. Adafruit_BQ24074.brd (180,589 bytes) and Adafruit_BQ24074.sch (433,026 bytes) were downloaded from raw.githubusercontent.com (master) and parsed as EAGLE XML with python -I. The repo carries an empty marker file named REV_B, and Adafruit's rendered schematic is titled \"bq24074 Solar-DC-USB Lipo rev B, 9/14/20\".","evidence":"https://api.github.com/repos/adafruit/Adafruit-BQ24074-PCB/contents lists Adafruit_BQ24074.brd, Adafruit_BQ24074.sch, README.md, REV_B, assets/, license.txt. The parser computed absolute pad positions from element x/y/rot plus package pad offsets and joined them to &lt;signal&gt;&lt;contactref&gt;. Schematic image: https://cdn-learn.adafruit.com/assets/assets/000/094/727/original/adafruit_products_image.png","confidence":"high"},{"question":"What is the exact physical order of the 11 header pads, from the pad nearest the USB-C connector to the pad nearest the JST connectors?","answer":"Your reading is correct in every position. All pads are at y = 2.54 mm on a 2.54 mm pitch:\n1. VBUS (x = 6.35, EAGLE pad JP3.11)\n2. GND (x = 8.89, JP3.10)\n3. THERM (x = 11.43, JP3.9, IC pin 1 TS)\n4. ISET (x = 13.97, JP3.8, IC pin 16)\n5. CE (x = 16.51, JP3.7, net !CE, IC pin 4)\n6. PGOOD (x = 19.05, JP3.6, net !PGOOD, IC pin 7)\n7. CHG (x = 21.59, JP3.5, net !CHG, IC pin 9)\n8. OUT (x = 24.13, JP3.4, IC pins 10/11)\n9. GND (x = 26.67, JP3.3)\n10. LIPO (x = 29.21, JP3.2, net VLIPO, IC pins 2/3 BAT)\n11. GND (x = 31.75, JP3.1)\nEAGLE numbers the header in reverse: schematic pin 1 is the GND at the JST end and pin 11 is VBUS at the USB-C end. PGOOD at x = 19.05 is exactly the board centre (board width 38.1 mm), so it is the 6th pad from either end. The silk labels alternate above and below the pad row (VBUS above, GND below, THERM above, ISET below, CE above, PGOOD below, CHG above, OUT below, GND above, LIPO below, GND above), which makes miscounting easy.","evidence":".brd: element JP3, package 1X11_ROUND_76, placed at x=19.05 y=2.54 rot=R180; package pads 1..11 at x = -12.7..+12.7. Signals: GND (JP3.1, JP3.3, JP3.10), VLIPO (JP3.2), OUT (JP3.4), !CHG (JP3.5), !PGOOD (JP3.6), !CE (JP3.7), ISET (JP3.8), THERM (JP3.9), VBUS (JP3.11). Layer-21 silk texts sit at the same x values: VBUS 6.35, GND 8.89, THERM 11.43, ISET 13.97, CE 16.51, PGOOD 19.05, CHG 21.59, OUT 24.13, GND 26.67, LIPO 29.337, GND 31.75. The .sch nets give the same mapping. Adafruit's top photo (https://cdn-learn.adafruit.com/assets/assets/000/094/744/large1024/adafruit_products_4755_top_ORIG_2020_09.jpg) and the repo's assets/47XX.png show the same left-to-right order.","confidence":"high"},{"question":"Which end of the header is the USB-C end and which is the JST end?","answer":"Low x is the USB-C / DC-jack end; high x is the JST end. The board outline runs x = 0..38.1 mm, y = 0..33.02 mm. The USB-C connector X2 is at x = 2.794, y = 10.16 (signal pads at x = 6.34, shell tabs at x = 1.58 and 5.76). The DC jack X3 is on the same edge (x = 1.8..8.0, y = 16.2..27.2). Both JSTs are on the opposite edge at x = 34.544: BATT (\"LiPo Batt\") at y = 10.795 and SYSOUT (\"Load Out\") at y = 22.225. So the header pad nearest the USB-C is VBUS (x = 6.35) and the pad nearest the JSTs is GND (x = 31.75). The JST closest to the header row is the LiPo Batt connector.","evidence":".brd elements: X2 USB_C_CUSB31-CFM2AX-01-X x=2.794 y=10.16 R270; X3 DCJACK_2MM_SMT x=1.778 y=21.717; BATT JST-PH-2-SMT-RA x=34.544 y=10.795; SYSOUT x=34.544 y=22.225. Silk \"LiPo Batt\" at (34.0, 5.6), \"Load Out\" at (33.8, 27.4). Layer-20 outline wires span 0..38.1 by 0..33.02. The product photo shows USB-C on the left and both JSTs on the right, with the header along the bottom edge.","confidence":"high"},{"question":"Which net does each status LED connect to, and does the header pad sit directly on the IC pin?","answer":"Your belief is correct.\n- Green LED D1 (silk \"GOOD\"): anode on OUT, cathode -&gt; R11 (10 kOhm) -&gt; net !PGOOD.\n- Red LED D3 (silk \"CHG\"): anode on OUT, cathode -&gt; R7 (10 kOhm) -&gt; net !CHG.\n- Net !PGOOD contains exactly IC pin 7, header pad JP3.6 and R11.1. Net !CHG contains exactly IC pin 9, header pad JP3.5 and R7.1. Each is a plain copper trace with one via and no series component, so the header pads are directly on the open-drain IC pins.\nThere is no pull-up to a logic rail on the board; the only pull-up is the LED plus 10 kOhm to OUT (about 4.4 V maximum). Per the TI datasheet, PGOOD pulls to VSS when a valid input source is detected and is high-impedance otherwise; CHG pulls to VSS while charging and is high-impedance when done, disabled, or with the battery absent.","evidence":".brd signals: OUT includes D1.A and D3.A; N$1 = D1.C + R11.2; N$3 = D3.C + R7.2; !PGOOD = X4.7, JP3.6, R11.1; !CHG = X4.9, R7.1, JP3.5. D1 value GREEN at y=15.748 next to silk \"GOOD\"; D3 value RED at y=17.272 next to silk \"CHG\"; R7 and R11 value 10K. The .sch nets are identical and the device connects give !PGOOD -&gt; pad 7, !CHG -&gt; pad 9. The rendered schematic shows OUT -&gt; D1 GREEN -&gt; R11 10K -&gt; PGOOD and OUT -&gt; D3 RED -&gt; R7 10K -&gt; CHG. TI datasheet SLUS810N Table 7-1 (PGOOD pin 7, CHG pin 9), section 9.3.5.7, Tables 9-1 and 9-2.","confidence":"high"},{"question":"What is the default state of the charge-current jumpers (0.5 / 1.0 / 1.5 A)?","answer":"The 1.0A jumper is closed by default; 0.5A and 1.5A are open.\n- 1.0A: R9 = 1 kOhm from ISET to GND, giving 890/1000 = 0.89 A typical.\n- 0.5A: R5 = 2 kOhm, giving about 0.45 A.\n- 1.5A: R3 = 590 Ohm, giving about 1.5 A.\nThe resistors are switched in parallel, so closing a second jumper without cutting 1.0A adds the currents (for example 1 kOhm with 2 kOhm gives about 1.33 A). Adafruit's instruction is to cut the 1.0A trace first, then bridge the wanted jumper. The ISET header pad is directly on IC pin 16; charging is disabled if ISET is left with no resistor.","evidence":".brd/.sch: jumper \"1A\" is device SOLDERJUMPER variant CLOSED, package SOLDERJUMPER_CLOSEDWIRE (has a copper bridge pad named WIRE between pads 1 and 2). \"0.5A\" and \"1.5A\" use package SOLDERJUMPER_ARROW_NOPASTE (two separate pads, no bridge). Nets: ISET = R3.1, R5.1, R9.1, X4.16, JP3.8; N$7 = R9.2 + 1A.2; N$6 = R5.2 + 0.5A.2; N$5 = R3.2 + 1.5A.2; the other side
... (truncated 76533 chars, full result in C:\Users\a\AppData\Local\Temp\claude\C--Users-a-Documents-AIs-Room\3847ddcb-e555-407f-83d3-b1514a05ef78\tasks\wck81akcq.output)</result>
<diagnostics>Per-agent results: C:\Users\a\.claude\projects\C--Users-a-Documents-AIs-Room\3847ddcb-e555-407f-83d3-b1514a05ef78\subagents\workflows\wf_b4d016fd-c59/journal.jsonl — one {"type":"result",...} line per completed agent with its full return value.
If the result above is empty or unexpected, Read this file BEFORE diagnosing — do not assume agents returned non-empty results.
To re-run with edited post-processing: Workflow({scriptPath: 'C:\Users\a\.claude\projects\C--Users-a-Documents-AIs-Room\3847ddcb-e555-407f-83d3-b1514a05ef78\workflows\scripts\recorder-pgood-check-wf_b4d016fd-c59.js', resumeFromRunId: 'wf_b4d016fd-c59'}) — agents whose (prompt, opts) are unchanged replay from cache.</diagnostics>
<usage><agent_count>4</agent_count><agents_done>4</agents_done><agents_error>0</agents_error><agents_skipped>0</agents_skipped><agents_empty_result>0</agents_empty_result><subagent_tokens>827746</subagent_tokens><tool_uses>197</tool_uses><duration_ms>1783952</duration_ms></usage>
</task-notification>

**Claude:** Die vier Agenten sind durch. Ich trage die geprüften Fakten und zwei Korrekturen an meinen früheren Aussagen ins Brain ein, dann kommt die Antwort.

**Claude:** **Ja, es geht ohne `PGOOD`.** Laden, Versorgen und Akkumessung funktionieren ohne diese Leitung. Es fehlt dann nur die Meldung „hängt am Strom" in der Statusanzeige.

## Was `PGOOD` macht

Es ist ein reiner Meldeausgang des Ladechips. Er zieht nach Minus, sobald am Lader gültiger Strom anliegt, und schaltet damit die grüne LED „GOOD". Weder der Lader noch der XIAO brauchen ihn zum Arbeiten.

## Was die Prüfung ergeben hat

- **Die Pin‑Reihenfolge am Lader stimmt**, bestätigt aus Adafruits Original‑Platinendaten. `PGOOD` ist genau der mittlere Pin, der sechste von beiden Enden.
- **Der Chip meldet `PGOOD` sicher**, sonst gäbe es die 4,18 V an `LIPO` nicht. Das Signal kommt also nur nicht bis zum XIAO.
- Mögliche Ursachen, noch nicht unterschieden: kalte Lötstelle am `PGOOD`‑Pin des Laders, grünes Kabel eine Reihe daneben, oder ein Fehler am XIAO‑Ende.

## So finden wir den Fehler

1. **Das rote `CHG`‑Kabel ganz abziehen**, an beiden Enden. Säße es eine Reihe daneben, läge `OUT` mit 4,4 V direkt an einem XIAO‑Pin, und das verträgt er nicht.
2. Schau hin: **Leuchtet am Lader die grüne LED „GOOD"?**
3. USB‑Kabel **vom Lader** abziehen. Der XIAO bleibt am PC, die Live‑Anzeige läuft weiter.
4. Das Lader‑Ende des grünen Kabels in die Reihe stecken, in der am Lader das schwarze Kabel steckt. Nirgendwo anders hin, das Kabel hängt ungeschützt am XIAO‑Pin.

| Anzeige | Bedeutung |
|---|---|
| `PGOOD an D3: JA` | Kabel und XIAO‑Seite sind in Ordnung. Der Fehler liegt am Lader, dann löten wir die Leiste nach. |
| bleibt `nein` | Der Fehler liegt am XIAO‑Ende: Kabel, Reihe oder Lötstelle. |

Schreib mir das Ergebnis und ob die grüne LED geleuchtet hat.

## Zwei Korrekturen an dem, was ich vorher gesagt habe

- **`OUT` und PC nie gleichzeitig, auch ohne Akku.** Ich hatte geschrieben, ohne Akku sei das unbedenklich. Das war falsch. Solange das `OUT`‑Kabel am 5V‑Pin steckt, darf der XIAO nicht an den PC, sonst liegen die 5 V vom PC am Akku‑Anschluss des Laders, und dessen Grenze ist 5 V.
- **Die 5V‑Reihe messen wir nach, bevor `OUT` drankommt.** Am USB‑Ende des XIAO sitzen zwei Eck‑Pins, einer ist 5V, der andere D0. Dafür nehmen wir dein `LIPO`‑Kabel als Prüfspitze, das zeige ich dir im nächsten Schritt.

## Nebenbei

- Ultracode ist bei dir eingeschaltet. Die Prüfung lief deshalb mit vier Agenten, rund 30 Minuten und etwa 830.000 Tokens. Wenn dir das zu teuer ist, schalte es wieder aus.
- Einer der Agenten hat versehentlich einen „Datei speichern"‑Dialog bei dir ausgelöst. Den kannst du abbrechen, es muss nichts gespeichert werden.

Alles steht im Brain unter [Recorder Hardware Kette](Knowledge/Recorder%20Hardware%20Kette.md).

**Tobi:** MAche nochmals so ne grafik wo welches kabel auf dem klembrett hingehört ich löte die steckleisten nochmals neu und bitte mache du die grafiken damit ich des wieder schell aufbauen kann

**Claude:** Der Aufbauplan ist oben als Bild: jedes Kabel hat an beiden Enden dieselbe Nummer und die Lochnummer dazu. Er liegt auch im Brain als [Recorder Steckbrett-Aufbau.png](Knowledge/Recorder%20Steckbrett-Aufbau.png).

## Wo die Platinen sitzen

- **XIAO:** Stifte in **d6 bis d12** und **h6 bis h12**. Das USB‑Ende mit der SD‑Karte liegt bei Reihe 12, die Antenne bei Reihe 6.
- **Lader:** Stiftleiste in **Spalte i, Reihe 33 bis 43**. Die USB‑C‑Buchse liegt bei Reihe 33, die weißen Steckbuchsen liegen unten.

Sitzen die Platinen genau so, stimmen alle Lochnummern im Bild.

## Stufe 1: jetzt stecken

| Nr. | Farbe | von → nach | Zweck |
|---|---|---|---|
| 1 | schwarz | a11 → Minus‑Schiene links (blaue Linie) | Minus vom XIAO |
| 2 | schwarz | b11 → g41 | Minus zum Lader |
| 3 | rot | g42 → a49 | `LIPO` zum Spannungsteiler |
| 4 | schwarz | j12 → d52 | Messpunkt zu D0 |

Dazu die drei Bauteile: R1 von b49 nach b52, R2 von a52 in die Minus‑Schiene, der Kondensator (104) von c52 in die Minus‑Schiene.

Anders als vorher geht Minus jetzt mit Kabel 2 direkt vom Lader zum XIAO. Die Brücke oben zwischen den beiden Minus‑Schienen und das Kabel zur rechten Schiene brauchst du nicht mehr.

## Stufe 2: erst nach dem Nachmessen

Die Kabel 5 (grün, `PGOOD`), 6 (rot, `CHG`) und 7 (rot, `OUT`) haben im Bild hohle Punkte. Lass sie weg, bis Stufe 1 läuft und wir die Reihen nachgemessen haben. Eine Reihe daneben läge sonst `OUT` mit 4,4 V direkt an einem XIAO‑Pin.

## Beim Nachlöten

- Beide USB‑Kabel ab, Platinen aus dem Brett oder das Brett als Halter nutzen.
- **Alle** Stifte löten, beim Lader alle elf: Lötkolben 2 bis 3 Sekunden an Stift und Lötauge, dann Zinn. Es soll ein kleiner glänzender Kegel entstehen.
- Danach prüfen, dass kein Zinn zwei Nachbarstifte verbindet, besonders `LIPO` mit den beiden `GND` daneben und `CHG` mit `OUT`.

## Wenn Stufe 1 steckt

XIAO an den PC, Lader ans Netzteil, dann die Live‑Anzeige starten:

```bash
python A:/Recorder/firmware/ar1/tools/pins_live.py
```

Dort muss `springt 0 bis 4.2 V` stehen. Schreib mir dann, und wir machen Stufe 2.

Die Reihen 49 bis 54 für den Spannungsteiler habe ich so festgelegt, wie ich sie von deinen Fotos geschätzt habe. Steckt dein Teiler eine Reihe anders, lass ihn so. Wichtig ist nur, dass Kabel 3 in der Reihe von R1 oben landet und Kabel 4 in der Reihe, in der R1 unten, R2 und der Kondensator zusammenkommen.

## Erkenntnisse

_Noch keine kuratierte Zusammenfassung._
