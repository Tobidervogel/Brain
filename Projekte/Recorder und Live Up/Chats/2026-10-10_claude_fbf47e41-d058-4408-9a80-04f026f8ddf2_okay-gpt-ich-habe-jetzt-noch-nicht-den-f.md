---
titel: "Okay, GPT, ich habe jetzt noch nicht den finalen Lipo. Ähm, aber ich habe hier e"
datum: 2026-10-10
ki: claude
session: fbf47e41-d058-4408-9a80-04f026f8ddf2
tags: [chat]
---

# Okay, GPT, ich habe jetzt noch nicht den finalen Lipo. Ähm, aber ich habe hier e

## Verlauf

**Tobi:** Okay, GPT, ich habe jetzt noch nicht den finalen Lipo. Ähm, aber ich habe hier einen Lipo. Von glaube einer alten Vape. Mit äh, drei hundert sechzig Milliampere ähm, davor steht irgendwie dreizehntausenddreihundert ich weiß nicht was das heißt drei Komma sieben Volt eins Komma drei drei Watt und unten drunter steht dann noch eine Nummer und zwar drei fünf zwei null a zwei drei null neun zehn Ähm, kann ich den vielleicht für einen Test benutzen oder sollte ich doch dieses 9-Volt-Block an an Solarpanel-Dingsums da benutzen oder wie sieht es aus? Und passen meine meine ähm, also sollte ich das vielleicht an so einen so einen Kabel, an so einen äh, J ST-PH-Kabel dran machen oder passt es überhaupt? Passen diese diese Kabel zwei Millimeter in die Löcher von dem Lipo-Charger? Wie sieht denn das aus? Wie kann ich das jetzt hier mal ausprobieren? Der Lipo, der wird vermutlich erst am Montag ankommen.

**Claude:** Ja, die Vape-Zelle taugt für einen Test auf dem Tisch: betreiben kannst du den Recorder damit sofort, laden aber erst nach einer kleinen Änderung am Lader. Die 9-V-Batterie bringt dir nichts.

## Was du da hast

- **`13300`** ist die Bauform: rund, 13 mm dick, 30 mm lang. Das ist die übliche Zelle aus Einweg-Vapes.
- **360 mAh, 3,7 V, 1,33 Wh** passt zusammen (360 mAh × 3,7 V).
- **`3520A230910`** ist vermutlich Charge und Datum (10.09.2023); belegt ist das nicht.
- Solche Zellen haben in aller Regel **keine Schutzschaltung**, die saß in der Vape. Also nur als Testakku unter Aufsicht, nicht fürs Gerät am Körper.

## Test 1: Betrieb am Akku (geht sofort)

1. Zelle ansehen: nicht aufgebläht, keine Dellen.
2. Spannung mit deinem Messkabel prüfen. Unter etwa 3,0 V nicht verwenden.
3. Kein USB am Lader, XIAO nicht am PC, dann die Zelle an „Lipo Batt“ stecken.
4. Der Lader schaltet den Akku auf `OUT`, der XIAO startet und meldet im WLAN `battery` mit `bat_mv` zwischen 3300 und 4200.

Die Zelle reicht für etwa 3 bis 5 Stunden. Steck sie bei 3,3 V ab, denn weder Lader noch Firmware schalten bei leerem Akku ab.

## Test 2: Laden (nur nach Umbau)

Im Auslieferzustand lädt die Platine mit 1 A, das ist fast das Dreifache dessen, was diese kleine Zelle verträgt. Auch die 0,5-A-Brücke ist zu viel; sinnvoll sind rund 180 mA.

So stellst du das laut Adafruit ein:

1. Die Leiterbahn der 1,0-A-Brücke auf der Rückseite auftrennen.
2. Einen Widerstand von `ISET` (Reihe 36) nach Minus stecken: 5,1 kΩ ergibt rund 175 mA, 4,7 kΩ rund 190 mA. Die Stromwerte habe ich aus dem Gedächtnis gerechnet; den zulässigen Bereich (590 Ω bis 8,9 kΩ) nennt Adafruit.
3. USB an den Lader: die rote LED und `CHG an D4: JA` zeigen das Laden, nach gut zwei Stunden ist die Zelle voll.

Ohne Widerstand an `ISET` lädt die Platine gar nicht. Für den endgültigen Akku lötest du die Brücke später wieder zu. Wenn du die Bahn nicht auftrennen willst, lass das Laden bis Montag und mach nur Test 1.

## Anschluss

- **JST-PH** hat 2,0 mm Raster und gehört in die Buchse „Lipo Batt“; dort passt es. In die Lochreihe der Platine (2,54 mm) passt der Stecker nicht, da sitzen ohnehin deine Stiftleisten.
- **Empfehlung:** ein Kabel aus dem Kit an die beiden Litzen der Zelle löten. Draht an Draht, jede Litze einzeln, über jede Lötstelle Schrumpfschlauch, nie an der Zelle selbst löten.
- **Polung** nach dem `+` am Aufdruck der Buchse richten, nicht nach der Kabelfarbe.
- Lose Litzen direkt ins Steckbrett würde ich nicht nehmen: Rutschen sie heraus, ist die ungeschützte Zelle schnell kurzgeschlossen.

## 9-V-Batterie

Am Solar-/DC-Eingang wäre sie nur eine zweite Stromquelle wie das USB-Kabel. Einen Akku ersetzt sie nicht, also kannst du damit weder `CHG` noch den Akkubetrieb prüfen. Die Regel „nie 9 V an `LIPO`, `OUT` oder einen Pin“ bleibt.

Zwei Dinge gelten weiter: Solange `OUT` am 5V-Pin hängt und ein Akku steckt, darf der XIAO nicht an den PC. Und laden nur unter Aufsicht auf einer feuerfesten Unterlage, weil der Lader weder Zeitabschaltung noch Temperaturfühler hat.

Das steht jetzt auch in [Recorder Hardware Kette.md](Knowledge/Recorder%20Hardware%20Kette.md). Hat die Zelle noch ihre beiden Litzen dran, und hast du einen Widerstand mit 4,7 oder 5,1 kΩ?

**Tobi:** Machen wir nochmal kurz eine kleine Grafik, auch wenn es nur im äh, Mehrheit ist. Wie ich genau den Repo bzw. den Akku, der hat mir übrigens jetzt angezeigt. Das war, glaube ich, irgendeine Ladespannung oder irgendwie sowas. Ähm, wie ich den jetzt ja, wann die Stecker dran löten soll. Ähm, das Also die, die Stecker passen rein, sind halt schwer wieder rauszuholen. Geht. Ähm, wie wie soll ich die jetzt da dran machen? Beziehungsweise ich will die jetzt so dran machen, wie ich später den Lipo dran machen werde.

**Claude:** Die Skizze oben zeigt den Aufbau: Die Zelle bekommt ein Kabel mit JST-PH-Stecker aus dem Kit und kommt damit in „Lipo Batt“, genau wie später der LiPo, der seinen Stecker schon mitbringt.

## Reihenfolge

1. **Plus-Litze des Kabels finden.** Kabel ohne Zelle in „Lipo Batt“ stecken, USB an den Lader, die beiden freien Enden auseinanderhalten. Mit deinem Messkabel beide Enden antippen: Das Ende, das zeitweise rund 4,2 V zeigt, ist Plus. Mit Klebeband markieren, denn auf die Kabelfarbe ist bei solchen Kits kein Verlass.
2. **Alles stromlos machen.** USB vom Lader ab, Kabel aus der Buchse ziehen. Gelötet wird nur am losen Kabel.
3. **Schrumpfschlauch zuerst auffädeln**, je ein Stück auf beide Litzen des Kabels, weit weg von der Lötstelle.
4. **Erst Plus löten.** Die Minus-Litze der Zelle so lange mit Klebeband abkleben, damit sich die blanken Enden nicht berühren. Plus der Zelle an die markierte Plus-Litze, Schlauch drüber, schrumpfen.
5. **Dann Minus**, ein Stück versetzt wie in der Skizze, ebenfalls mit Schlauch.
6. **Einstecken nur ohne USB am Lader.** Solange der Ladestrom nicht heruntergestellt ist, würde die Zelle sonst mit 1 A geladen.

Nie an der Zelle selbst löten, nur an ihren Litzen, und zügig arbeiten, damit die Hitze nicht in die Zelle wandert.

## Stecker wieder herausbekommen

Dass die Stecker stramm sitzen, ist bei JST-PH normal. Zieh am Kunststoffgehäuse, nie an den Litzen: mit den Fingernägeln oder einer flachen Pinzette seitlich greifen und leicht hin und her wackeln.

Welchen Wert hat dir die Zelle angezeigt? Liegt er zwischen etwa 3,0 und 4,2 V, kannst du loslegen; darunter würde ich sie nicht nehmen.

**Tobi:** ist dran, schau mal, ob es funktioniert. Ich glaube, der liegt in die Leuchten ein bisschen. Wenig.

**Claude:** Kurzer Stand: Ich habe über USB und WLAN nach dem Recorder gesucht und trage das Ergebnis noch in die Notiz ein.

**Claude:** Von hier aus sehe ich den Recorder nicht, er läuft also sehr wahrscheinlich nicht. Am PC hängt er nicht (nur COM1), und im WLAN antwortet er unter `192.168.0.112` auch nach vier Versuchen über etwa eine Minute nicht.

Das schwache Leuchten passt dazu. Am Akku allein, ohne USB am Lader, müssten die LEDs des Laders ganz aus sein. Ein Glimmen entsteht typischerweise, wenn der XIAO keinen Strom bekommt und über die grüne und rote Meldeleitung ein kleiner Strom in seine Pins fließt. Das ist meine Deutung, gemessen ist sie nicht.

Prüf bitte in dieser Reihenfolge:

1. **Steckt USB am Lader?** Dann sofort abziehen, sonst wird die kleine Zelle mit 1 A geladen.
2. **Wird etwas warm?** Zelle und Lader kurz anfassen. Wenn ja, den Stecker sofort ziehen, dann stimmt vermutlich die Polung nicht.
3. **Steckt Kabel 7** von g40 (`OUT`) nach c12 (5V-Pin)? Das ist die Versorgung des XIAO; gestern war es zum Arbeiten am PC gezogen.
4. **Welche LEDs glimmen**, die am Lader (grün/rot) oder die am XIAO?

Am schnellsten sehe ich selbst, was los ist, wenn du Kabel 7 an c12 ziehst und den XIAO dann per USB an den PC steckst. Ohne dieses Kabel ist das auch mit eingesteckter Zelle unbedenklich. Sag Bescheid, sobald er dran ist, dann lese ich die Spannung der Zelle aus. Liegt sie unter etwa 3,3 V, ist die Zelle zu leer, um den XIAO zu starten.

**Tobi:** Okay, das hat jetzt erstmal nicht gewirkt. Ich habe einer von meinen JST-PH-2mm-Steckern reingesteckt. Ähm, die dann auf die andere Seite die noch nicht mit dem Plus-Minus besetzt war, da bei Plus-Minus reingesteckt. Ähm, und dann beim Libo habe ich so zwei von diesen Aufsteckkabeln auf diese Aufstecker dra- dran gemacht. Und dann auch in den Stromkreis rein. Ähm, das Ding ist, mal gesehen, die Lampen leuchten, aber nur sehr schwach. Sehr, sehr schwach. Die Spannung ist angekommen, denke ich mal. Ähm, Ich habe nichts bei dem bei dem Lipo-Charger eingestellt, vielleicht war das auch der Fehler oder so, ähm, aber ich habe das jetzt nicht irgendwie anders eingesteckt. Die Frage ist, könnte ich das mal mit einem neuen Volt-Block versuchen, also einem neuen Volt-Block, ich habe so einen Anschluss, entweder auf ähm, auf diese Steckdinger, wo ich dann reinstecken kann, oder Also damit es dann auch wieder über den JST-PH-Stecker reingeht oder über den ähm, über den Stecker von von dem Solar-Aufladeding weil da habe ich auch einen Stecker für einen neuen Voltblock aber geht das theoretisch oder wie sieht das aus weil ich glaube der Lipo der ist ein bisschen zu klein von der Leistung her

**Claude:** Die 9-V-Batterie über den JST-Stecker bitte auf keinen Fall: Dieser Anschluss verträgt höchstens 5 V, und die 9 V lägen über `OUT` direkt am XIAO. Über die runde Solar-/DC-Buchse geht es dagegen, hilft dir aber nicht weiter.

## 9-V-Batterie an der runden Buchse

- **Zulässig:** Adafruit gibt für diesen Eingang 5 bis 10 V an. Mein früherer Satz „nie 9 V an die Ladeplatine“ war für diese Buchse zu streng.
- **Bringt nichts Neues:** Die Batterie ersetzt dort nur das USB-Kabel. Dass der Lader den XIAO so versorgt, hast du gestern schon gezeigt; den Akkubetrieb testet das nicht.
- **Nur ohne die Vape-Zelle:** Steckt sie dabei, wird sie mit dem eingestellten 1 A geladen.
- **Polung vorher prüfen:** In der Mitte des Steckers muss Plus liegen. Auf der Produktseite steht dazu nichts.

## Warum es vermutlich nicht läuft

Zu wenig Leistung hat die Zelle nicht. Eine Vape-Zelle liefert mehrere Ampere, der XIAO braucht rund 0,1 A. Sie kann aber leer sein, und ihre Spannung haben wir noch nie gemessen. Der Recorder ist weiterhin weder am PC noch im WLAN zu sehen.

Die wahrscheinlichen Ursachen, alle noch ungeprüft:

1. **Zelle leer.** Auf dem Steckbrett gehen im XIAO zusätzlich etwa 0,3 V verloren (geschätzt), eine halb leere Zelle reicht dann nicht zum Starten.
2. **Wackelkontakt.** Steckkabel halten an den Anschlüssen der Zelle schlecht.
3. **Schiene unterbrochen.** Bei diesen Steckbrettern sind die Plus-/Minus-Schienen oft in der Mitte geteilt. Kabel vom Lader und Zelle müssen in derselben Hälfte stecken.
4. **Polung vertauscht.** Bei den Kit-Kabeln stimmt die Farbe oft nicht. Wird am Lader oder an der Zelle etwas warm, zieh sofort den Stecker.
5. **Falsche Buchse.** Der Stecker gehört in „Lipo Batt“, nicht in „Load Out“.

## Nächster Schritt

Eine Messung klärt Punkt 1 bis 4 auf einmal:

1. Kabel 7 am XIAO ziehen (c12, 5V-Pin). USB am Lader bleibt ab.
2. Den XIAO per USB an den PC stecken.
3. Mir Bescheid sagen, dann lese ich die Spannung aus, die von der Zelle am Lader ankommt.

Zeigt die Messung 0 V, ist es Kontakt, Schiene oder Buchse. Liegt sie unter etwa 3,3 V, ist die Zelle leer. Erscheint ein Wert zwischen 3,5 und 4,2 V, liegt der Fehler hinter dem Lader.

**Tobi:** spannung ist<0.1mA kann ichdie da auch gleich laden?

**Claude:** Noch nicht laden: Erst muss klar sein, ob die Zelle wirklich leer ist oder nur der Kontakt fehlt. „<0,1 mA“ ist ein Strom, keine Spannung, daraus kann ich den Zustand der Zelle nicht ablesen.

Miss bitte so:

1. Messgerät auf Gleichspannung stellen (Bereich 20 V, Zeichen `V⎓`), nicht auf mA. Im Strombereich schließt das Messgerät die Zelle kurz.
2. Die Spitzen direkt an die beiden Anschlüsse der Zelle halten, nicht an die Schiene des Steckbretts.

Je nach Wert:

| Spannung an der Zelle | Bedeutung | Laden? |
|---|---|---|
| 3,0 bis 4,2 V | Zelle ist in Ordnung, der Fehler liegt am Kontakt oder an der Schiene | ja, aber erst nach dem Herunterstellen des Ladestroms |
| etwa 2,5 bis 3,0 V | leer, aber noch brauchbar | ja, ebenfalls nur mit kleinem Strom und unter Aufsicht |
| deutlich darunter oder nahe 0 V | tiefentladen und innen beschädigt | nein, wegen Brandgefahr |

Eine tiefentladene Zelle gehört mit abgeklebten Enden zur Batteriesammlung. Der Lader schützt dich hier nicht: Er würde auch eine solche Zelle laden und hat keine Zeitabschaltung.

Zum Laden gilt weiter, was oben stand: Im jetzigen Zustand lädt die Platine mit 1 A, das ist für 360 mAh fast das Dreifache des Verträglichen. Vorher die 1,0-A-Brücke auftrennen und 5,1 kΩ von `ISET` nach Minus stecken (rund 175 mA).

Welchen Wert zeigt das Messgerät in Volt direkt an der Zelle?

**Tobi:** Okay, also ich habe so ein Messgerät, das kann beides messen, also ein Multimeter. Bei Volt hat es jetzt 4,19 Volt gemessen. Bei Milliampere, habe ich auch ausgetestet, hat es 0,03 maximal angezeigt. Ähm, die Kontakte müssten funktionieren. Ich kann nochmal ausprobieren. Also mit Kabel ähm, kommt 4,19 Volt durch ähm, und maximal 0,3 Milliampere. Auf der Steckplatine messe ich dann bei den Kabeln Eingang für diese JST-PH-Stecker, messe ich dann ähm, 4,1, äh, 4,9 Volt und auch 0,02 Milliampere. Beziehungsweise Volt schwanken so ein bisschen ähm, zwischen, ich hatte jetzt zwischen 12 und 4, also 4,12 und 4,4 schwankend. Ähm, Milliampere sind gleich geblieben. Also 0,02. Hinter dem Stecker, also hinter dem äh, Dingsumster-Stecker, wo der JSTPH-Stecker drinsteckt, auf der Platine, messe ich vier Komma sechs Volt ähm, und null Komma null zwei Milliampere du könntest mal versuchen noch mal mit dem Chip zu verbinden ich glaube nämlich der blinkt da jetzt rot jetzt ist nicht mehr so dunkel

**Claude:** Ich habe gerade noch einmal nach dem Recorder gesucht und deine Messwerte notiert. Verbinden kann ich mich weiterhin nicht: Er hängt nicht am PC (nur COM1), antwortet nicht unter `192.168.0.112`, und eine Suche über das ganze Heimnetz hat ihn auch unter keiner anderen Adresse gefunden.

## Was deine Messungen sagen

- **Die Zelle ist voll.** 4,19 V direkt an der Zelle heißt: in Ordnung und geladen. Laden ist also gar nicht nötig.
- **Es liegt noch eine zweite Quelle an.** 4,4 V an der Schiene und 4,6 V auf der Platine können nicht aus einer Zelle mit 4,19 V kommen. Steckt gerade USB oder die 9-V-Batterie am Lader, oder hängt der XIAO an einem Netzteil?
- **Das rote Blinken am XIAO** ist dessen eigene Ladeanzeige. Sie blinkt, wenn am 5V-Pin Spannung anliegt und an seinen Akku-Flächen kein Akku hängt. Es zeigt also nur, dass Spannung ankommt, nicht, dass er läuft.
- **Bitte nicht mehr im mA-Bereich quer über Zelle oder Schiene messen.** So schließt das Messgerät die Zelle kurz. Dass nur 0,02 mA angezeigt wurden, deutet darauf, dass die Sicherung im mA-Eingang durch ist oder der Bereich nicht passte. Strom misst man in Reihe, und hier brauchen wir ihn nicht.

Der XIAO bekommt also Spannung, kommt aber nicht ins WLAN. Meine Vermutung, noch nicht belegt: Er startet und fällt beim Einschalten des Funks wieder aus, weil die Spannung über die dünnen Steckkabel und Schienenkontakte kurz einbricht.

## Damit ich etwas sehen kann

1. Alle Quellen am Lader abziehen (USB und 9 V). Die Zelle darf stecken bleiben.
2. Kabel 7 am XIAO ziehen (c12, 5V-Pin).
3. Den XIAO per USB an den PC stecken und mir Bescheid sagen.

Dann lese ich aus, welche Spannung von der Zelle am Lader ankommt und ob der XIAO zuletzt wegen Spannungseinbruch neu gestartet ist.

**Tobi:** Fertig, also der Repo ist noch am Lader dran. Ich habe aber out von C zwölf getrennt, also C zwölf beim Xio weg. Ich habe den jetzt mal angesteckt. Ähm, und kurz hat es bei beim Lader GHG und Goat geleuchtet, also grün und rot. Ähm, als ich die, die, Xio reingemacht habe, sind beide Lämpchen wieder ausgegangen. Genau, ich hoffe, dem geht's gut.

**Claude:** Der Recorder ist jetzt am PC und antwortet. Ich lese noch sein Startprotokoll aus, um zu sehen, warum er vorhin am Akku nicht ins WLAN kam.

**Claude:** Dem XIAO geht es gut, und die Akkumessung funktioniert jetzt zum ersten Mal mit einem echten Akku.

## Was ich ausgelesen habe

- **Stromquelle:** Der Recorder meldet `battery` mit 4,10 V und 91 %. Dein Multimeter zeigte 4,19 V, die Firmware liest also rund 2 % zu wenig; das lässt sich später mit einer Stellschraube ausgleichen.
- **Zustand:** Karte in Ordnung, WLAN verbunden (`192.168.0.112`), keine Fehler.
- **Lader:** Keine Quelle am Lader erkannt und kein Laden, so wie es sein soll.

## Warum er vorhin am Akku nicht lief

Der Startzähler steht auf 596, im Protokoll auf der Karte ist der letzte Eintrag davor Start 38 von gestern Abend. Dazwischen liegen 557 Starts ohne Protokollzeile. Der XIAO ist am Akku also Hunderte Male angelaufen und jedes Mal sofort wieder ausgefallen.

Mit 4,41 V vom Lader lief derselbe Aufbau gestern, mit der Zelle nicht. Die Zelle ist voll, also bricht sehr wahrscheinlich die Zuleitung beim Anlaufen ein: Steckkabel an der Zelle, Schienenkontakte und der Umweg über den 5V-Pin. Das ist eine Folgerung, der Grund der Abstürze selbst wurde nicht protokolliert.

## Die LEDs am Lader

Was du gesehen hast, ist harmlos. Solange der XIAO stromlos ist, fließt ein winziger Strom vom Lader über die grüne und rote LED in seine Pins, deshalb glimmen sie. Sobald der XIAO Strom hat, gehen sie aus.

## Nächster Versuch

1. Die Zelle fest an das JST-Kabel löten, wie in der Skizze, statt über Steckkabel und Schiene.
2. Einen Elko mit 100 µF aus dem Elegoo-Kit am XIAO zwischen 5V (Reihe 12 links) und GND (Reihe 11 links) stecken, der Streifen am Elko zeigt zu GND. Er fängt die Stromspitze beim Start ab.
3. Den XIAO vom PC abziehen, dann Kabel 7 wieder an c12 stecken.

Kommt er danach innerhalb einer Minute ins WLAN, sehe ich das von hier.

**Tobi:** Zelle ist an Stecker angelötet. Ähm, bitte sag mir nochmal, wie genau ich das mit dem Elko machen soll. Kannst du da einfach ein Bild Also wieder so eine Zeichnung generieren, wenn es auch in Mehrheit ist. Damit ich das besser check. Danke. Und was soll ich danach machen? Also soll ich dann den PC wieder abziehen und dann wieder out anstecken?

**Claude:** Die Skizze zeigt die Löcher links neben dem XIAO: Das lange Bein des Elkos kommt in b12, das kurze Bein auf der Seite mit dem Streifen in c11. Die beiden Löcher liegen schräg nebeneinander, die Beine reichen dafür. Nimm einen Elko mit 100 µF; die aufgedruckte Spannung muss mindestens 10 V sein, das erfüllen die aus dem Kit.

Ja, genau in der Reihenfolge, die du sagst: erst den PC ab, dann `OUT` dran.

1. **XIAO vom PC abziehen.**
2. **Polung des gelöteten Steckers prüfen**, weil neu gelötet ist. Halte den Stecker neben die Buchse „Lipo Batt“ und miss an den beiden Metallkontakten des Steckers: rote Spitze an den Kontakt, der auf die Seite mit dem `+` kommt. Das Multimeter muss rund +4,2 V zeigen, ohne Minuszeichen.
3. **Zelle in „Lipo Batt“ stecken**, falls sie nicht schon steckt.
4. **Elko stecken** wie in der Skizze.
5. **Kabel 7 an c12 stecken** (das andere Ende sitzt in g40 am Lader).

Dabei bleibt kein USB am Lader, und der XIAO kommt nicht an den PC, solange Kabel 7 steckt.

Sag mir Bescheid, wenn Kabel 7 dran ist. Ich schaue dann, ob der Recorder im WLAN erscheint, und sehe später am Startzähler, ob er durchgelaufen ist.

**Tobi:** stect

**Claude:** Es funktioniert: Der Recorder läuft jetzt am Akku. Er antwortet seit 11:32 Uhr im WLAN unter `192.168.0.112` und hängt dabei nicht am PC, der Strom kommt also aus der Vape-Zelle über den Lader.

In gut anderthalb Minuten kamen 24 von 25 Abfragen an, eine blieb ohne Antwort. Ob das nur ein WLAN-Aussetzer war oder ein Neustart, sehe ich erst am Startzähler, wenn er wieder am PC hängt (zuletzt 596, ein sauberer Lauf ergibt 597).

Was den Ausschlag gab, ist nicht getrennt geprüft: Du hast die Zelle angelötet und den Elko gesteckt, beides zugleich.

Für den weiteren Test:

- **Laufzeit:** Die Zelle reicht für etwa drei Stunden. Sie hat keinen Schutz gegen Tiefentladung und der Recorder schaltet noch nicht selbst ab, also spätestens dann abstecken und nicht unbeaufsichtigt laufen lassen.
- **Kein USB an den Lader**, solange der Ladestrom auf 1 A steht.
- **Zurück an den PC** nur so: erst Kabel 7 an c12 ziehen, dann den XIAO anstecken.

Wenn du ihn wieder an den PC steckst, lese ich Startzähler und Akkuspannung aus. Das Ergebnis steht in [Recorder Hardware Kette.md](Knowledge/Recorder%20Hardware%20Kette.md).

**Tobi:** Perfekt. Kurze Frage, passt dieser Lipo theoretisch auch für den späteren Lipo? Also, wenn ich den jetzt fürs Laden teste, passt das auch für den späteren Lipo? Oder muss man dann nochmal was umstellen?

**Claude:** Ja, der Ladetest mit der Vape-Zelle gilt auch für den späteren LiPo. Umstellen musst du danach nur eine Sache: den Ladestrom.

**Was sich überträgt:** Beide Akkus haben dieselbe Chemie und dieselbe Ladeschlussspannung von 4,2 V, hängen an derselben Buchse und werden vom Lader gleich behandelt. Wenn also die rote LED, die Meldung „lädt“ in der Firmware und das Abschalten bei vollem Akku mit der Vape-Zelle funktionieren, funktionieren sie auch später.

**Was du umstellst:**

| | Vape-Zelle (360 mAh) | späterer LiPo (um 1800 bis 2000 mAh) |
|---|---|---|
| Ladestrom | rund 175 mA | 0,5 A |
| Einstellung | 1,0-A-Brücke auftrennen, 5,1 kΩ von `ISET` nach Minus | Widerstand an `ISET` entfernen, 0,5-A-Brücke zulöten |

Das Auftrennen der 1,0-A-Brücke ist keine verlorene Arbeit: Für die geplanten 0,5 A muss sie ohnehin getrennt werden. Willst du später doch 1 A, lötest du sie einfach wieder zu.

Zwei Dinge sagt dir der Test mit der kleinen Zelle nicht: wie lange das Laden des großen Akkus dauert und wie warm der Lader bei 0,5 A wird. Und beim neuen LiPo prüfst du vor dem ersten Einstecken wieder die Polung am Stecker, weil er seinen eigenen mitbringt.

## Erkenntnisse

_Noch keine kuratierte Zusammenfassung._
