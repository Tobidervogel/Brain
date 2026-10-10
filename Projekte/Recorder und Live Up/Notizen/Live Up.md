---
titel: Live Up
tags: [projekt, android, ki, digital-wellbeing, fitness]
erstellt: 2026-09-26
aktualisiert: 2026-10-08
---

# Live Up

Tobis eigene Tracker- und Selbstbeschränkungs-App für Android. Name am 26.09.2026 festgelegt: **Live Up**, gedacht wie „Level Up". Löst den Arbeitstitel „Live improve" ab ([[Live improve]] ist nur noch ein Verweis).

Ziel: weniger Doomscrolling, Ziele verfolgen, „das Leben clean halten". Gegenstück zu [[The Bridge]] — TB umgeht Sperren, Live Up sperrt Tobi bewusst selbst. Er nennt den Widerspruch selbst und betrachtet es als Experiment mit sich.

## Reihenfolge — bewusst umgedreht (26.09.2026)

Zuerst war der KI-Türsteher als MVP geplant und die Fitness-Sachen als eigenes Projekt. Tobi hat das umgedreht: **die Tracker kommen zuerst in einer App** (Kalorien, Workout, Gesundheit, Schlafphasenwecker), der Sperr-/Türsteher-Teil später. Das ist auch technisch klüger: die Tracker sind reine App-Logik, der Sperr-Teil kämpft mit Accessibility Service, Device Admin und Herstellereinschränkungen.

Alter bestätigt: **16** (das „vierzehn" im Diktat war ein Erkennungsfehler).

## Stand 0.1.0 — gebaut am 26.09.2026

Projektordner `A:\LiveUp`, Paket `de.tobidervogel.liveup`. Debug-APK gebaut.

Fünf Tabs unten: **Heute, Essen, Training, Schlaf, Gesund**.

| Teil | Was drin ist |
|---|---|
| Heute | Kalorien/Eiweiß/Training gegen Tagesziel, letzte Nacht, Gewicht, Ziele selbst einstellen |
| Essen | Eintrag mit kcal + Eiweiß/Kohlenhydrate/Fett, Tagesliste, Makrosumme, Löschen |
| Training | Schnellwahl Parkour/Kraft/Laufen/Dehnen, Minuten, Notiz, Verlauf |
| Schlaf | Messung starten/beenden, Schlafphasenwecker mit Zeit + Fenster + Schwelle, Bewegungsdiagramm, Verlauf |
| Gesund | Gewicht und freie Notiz („falls irgendwas ist"), Verlauf |

## Technische Entscheidungen

- **Versionen von [[The Bridge]] übernommen**: AGP 8.4.1, Kotlin 1.9.22, Gradle 8.7, compileSdk 34, minSdk 26. Damit läuft dieselbe JDK/SDK-Kombination, die hier schon funktioniert (JDK 17 aus `C:\Users\a\miniconda3\Library`, SDK `A:\Android\Sdk`).
- **Jetpack Compose + Material 3**, aber eigenes Farbschema: nahezu schwarz (`#0E0F0C`) mit Limette (`#C6F94B`) als einzigem Akzent. Bewusst nicht das Material-Standardviolett — genau der „generisch KI-gemachte" Look, den Tobi nicht will. Icon: Doppel-Chevron nach oben als Vektor.
- **Kein Room**, nur `SQLiteOpenHelper`. Room bräuchte KSP und damit deutlich längere Builds; für fünf Tabellen ohne Migrationen zahlt sich das nicht aus. Abfragen laufen vorerst auf dem Hauptthread (kleine Datenmengen).
- **Kein RAG** — von Tobi ausdrücklich gestrichen.
- Zustand über einen Versionszähler statt ViewModel/Flow: spart die ganze Schichtung bei dieser Datenmenge.
- `java.time` ohne Desugaring nutzbar, weil minSdk 26.

## Schlafphasenwecker — wie er arbeitet

Aktigrafie: ein Vordergrunddienst (`SleepService`) liest den Beschleunigungssensor und schreibt **pro Minute** einen Bewegungswert (aufsummierte Änderung des Beschleunigungsvektors). Liegt das Handy still, ist der Wert nahe null.

Im Weckfenster vor der spätesten Zeit wird geweckt, sobald die Bewegung der letzten 3 Minuten über der Schwelle liegt — sonst spätestens zur eingestellten Zeit. Geklingelt wird über eine **Benachrichtigung mit Vollbild-Absicht**, nicht `startActivity()`: ab Android 10 darf ein Dienst aus dem Hintergrund keine Activity öffnen. Dazu ein `AlarmManager`-Backstop, falls der Dienst die Nacht nicht überlebt.

**Kalibrieren nötig:** die Bewegungsschwelle (Standard 0,35) hängt von Bett und Gerät ab und ist deshalb in der App einstellbar. Zu niedrig = zu früh geweckt, zu hoch = erst zur Endzeit.

## Noch nicht gebaut (bewusst)

- **KI-Türsteher** (App öffnen nur nach Überzeugen einer KI) und Sperren/Limits — der eigentliche Wellbeing-Teil.
- **KI-Kalorienschätzung** aus Foto, Barcode oder Rezept. Wartet darauf, ob „Chatty"/ChatGPT oder lokal Ollama ([[Regg]]-Infrastruktur) die Grundlage wird.
- **PC-Client + NAS-Server** für geräteübergreifende Limits und Sync.
- Assistent im Bixby-Stil, der Kontext kennt (z. B. ob Nachrichten da sind, via [[Belegramm]]-MCP).
- 24/7-Audioaufnahme kommt später dazu, siehe [[Audio-Recorder]].

## Was OS-seitig geht (für den späteren Sperr-Teil, geprüft)

- **Eine Android-App sperrt nicht den PC.** Device Admin gibt es nur auf Android → eigener Windows-Client, gemeinsamer Server auf der [[JacobNAS]].
- **Device Admin sperrt keine anderen Apps** (nur Bildschirm/Passwort/Wipe). Er dient nur dazu, die eigene App uninstall-fest zu machen.
- **App-Sperren = Accessibility Service** (erkennt die geöffnete App, legt ein Overlay drüber). So arbeitet jeder App-Blocker.
- **App-Nutzungszeit = `UsageStatsManager`** (Bordmittel).
- **Webseiten sehen/sperren, auch in Firefox = lokaler VpnService** — den gibt es in [[The Bridge]] schon.

## Studienlage (Design-Grundlage)

- Schon kleine Reduktion wirkt: 1 Woche weniger Handy → Depression −24,8 %, Angst −16,1 %, Schlaflosigkeit −14,5 %. 2 Wochen Handy-Internet blockiert → Bildschirmzeit 314→161 min/Tag, Wirkung hielt nach 4 Wochen an (PNAS Nexus; BMC Medicine RCT 2025).
- **Reibung schlägt Zielsetzen**: Graustufen senkten die Bildschirmzeit stärker als bloßes Ziele-setzen (Zimmermann & Sobolev 2023) → Graustufen als Eskalationsstufe einbauen.
- **Limits wirken nur bei Über-Nutzern** (Facebook −33 %); weiche Selbstverpflichtung wirkt überraschend gut.
- **Zu streng wird abgeschaltet** (Autonomie-Reibungs-Konflikt, JMIR) → der verhandelbare KI-Türsteher trifft genau diese Lücke: Override mit Kosten statt unmöglicher Mauer.
- Vorbild: Googles „Pause Point" (Android 17), 10-Sekunden-Pause vor markierten Apps, bewusst schwer abschaltbar.

## Bauen

```
cd /a/LiveUp && JAVA_HOME="C:/Users/a/miniconda3/Library" ./gradlew.bat assembleDebug
```

APK: `app/build/outputs/apk/debug/app-debug.apk`.

## Offen / zu prüfen

- Akkuoptimierung: Samsung kann den Messdienst nachts killen — Ausnahme für Live Up nötig.
- Vollbild-Absicht ist ab Android 14 eingeschränkt; auf dem S9 (Android 10) unkritisch, auf dem Test-AVD (Android 14) eventuell freizugeben.
- Bewegungsschwelle über echte Nächte einstellen.
- Auf dem S9 noch nicht getestet.

Verwandt: [[The Bridge]], [[Regg]], [[Belegramm]], [[Audio-Recorder]], [[JacobNAS]], [[Tobi]]

## BMI nach WHO-Alterskurve (26.09.2026)

Tobi wollte BMI in der App. Eingebaut, aber **nicht** mit den Erwachsenengrenzen: bei Kindern und Jugendlichen gelten 25/30 nicht. In `Growth.kt` liegen die echten Werte aus der WHO-Tabelle „BMI-for-age BOYS 5 to 19 years (z-scores)", Stützstellen alle 6 Monate von 10:0 bis 19:0, dazwischen linear interpoliert.

- Mit **16 Jahren**: Übergewicht ab **+1SD = 23,5**, starkes Übergewicht ab **+2SD = 27,9**.
- Tobi bei 82,5 kg / 177,5 cm → **BMI 26,2**, nach WHO-Kurve „Übergewicht" (nicht „stark").
- Statt fester Altersangabe wird **Geburtsjahr/-monat** gespeichert — die Grenzen wandern dadurch von selbst mit.
- Sechs Unit-Tests in `GrowthTest.kt`. Einer prüft die Interpolation gegen die echte WHO-Zeile 16:3 (die zwischen zwei Stützstellen liegt) — Abweichung unter 0,06 BMI.
- In der App steht dabei, dass der BMI Muskeln und Fett nicht unterscheidet und keine Diagnose ist.

Nur Jungen-Tabelle hinterlegt (die App ist für Tobi).

## Demo auf dem PC (26.09.2026)

Läuft im Emulator `Belegramm_API34_Test`, auf S9-Maße gestellt:

```
adb shell wm size 1080x2220 && adb shell wm density 420
```

- `A:\LiveUp\demo.cmd` spielt die APK auf ein angestecktes Handy und startet sie.
- **Fallstrick 1:** Git Bash schreibt Android-Pfade in Windows-Pfade um (`/data/local/tmp` → `C:/Program Files/Git/data/...`). Lösung: `export MSYS_NO_PATHCONV=1`.
- **Fallstrick 2:** Daten per `run-as sqlite3` einfüllen, **während die App läuft**, geht verloren — beim `force-stop` rollt SQLite die fremden Schreibvorgänge über das Journal zurück. Erst App beenden, dann füllen.
- **Fallstrick 3:** `gradlew ... | tail` verschluckt den Exit-Code von Gradle; ein fehlgeschlagener Build sieht dann erfolgreich aus. Immer in eine Logdatei umleiten und `$?` prüfen.
- Der Emulator läuft auf GMT, der PC auf CEST — Testdaten in Gerätezeit erzeugen, sonst stimmen die Uhrzeiten nicht.

## ChatGPT Plus deckt die KI-Schätzung nicht ab

Tobi hat ein **ChatGPT-Plus-Abo** und meinte damit „Chatty". Plus gilt aber nur für App und Website, **nicht für die API** — eine App kann damit nicht rechnen lassen. Offene Wege:

1. **Barcode → Open Food Facts** (frei, ohne Schlüssel, echte Nährwerte). Für verpacktes Essen besser als jede KI-Schätzung.
2. **Teilen-Funktion**: Live Up baut den Text/das Foto, teilt es in die ChatGPT-App, Antwort zurück einfügen. Nutzt das Plus-Abo, kostet nichts, ist aber Handarbeit.
3. **Eigener API-Schlüssel** (getrennte Abrechnung, Cent-Beträge).
4. **Ollama lokal** auf dem PC mit RTX 3080 — frei, aber der PC muss laufen.

Noch nicht entschieden.

## Rueckmeldung Runde 2 (26.09.2026) — Umbau bestellt

Tobi hat 0.1.0 auf dem Geraet angeschaut. Das Logo gefaellt ihm. Der Rest soll umgebaut werden:

**Struktur**
- Einstellungen/Ziele gehoeren **nicht** auf den Startbildschirm, sondern an einen eigenen Ort.
- Navigation eher als Hub: Haupt-Tabs plus Unter-Tabs, "mehrere Apps in einer App".
- Die Schreibweise mit Gedankenstrich in den Abschnittsueberschriften ("Ziele — selbst gesetzt") raus.
- **Alle Verlaufslisten mit Loeschkreuz pro Zeile raus.** Findet er haesslich.

**Essen**
- KI-Schaetzung soll wirklich rein, manuelles Eintragen reicht nicht: er weiss die Werte nicht.
- Barcode-Scan.
- "Multimix": mehrere Dinge zusammen schaetzen lassen, der KI etwas erklaeren, ein Rezept als Bild anhaengen, ggf. zusaetzlich selbst eingeben.

**Training**
- Kein Nachtrag-Log. Stattdessen ein **Ablauf, den man startet** und der durchfuehrt.

**Schlaf**
- Ring-Einsteller wie in der Vorlage-App: 24-Stunden-Ziffernblatt, Mond-Griff fuer die Schlafenszeit,
  Glocken-Griff fuer den Alarm, Alarmfenster als Farbbogen, darunter "Schlafenszeit 22:30" und
  "Alarm 04:00 - 05:00", darunter ein grosser Knopf "Jetzt schlafen".
- Echte **Schlafphasen** (Wach / REM / Leicht / Tief) mit Kurve und Prozentanteilen.
- Journal- und Statistik-Ansicht mit Wochenleiste, Auswertung ueber mehrere Tage.

**Gesundheit**
- Weg von "Gewicht und Symptome", hin zu **"wie geht es mir heute"**: freiwillig, schnell, spaeter auswertbar.
- Ob Gewicht ueberhaupt dort bleibt, ist offen.

**Auftrag:** andere Apps anschauen (Kalorientracker, Fitness-/Trainings-Apps, Schlafphasen-Apps) und deren
UI-Muster uebernehmen. Tobi bietet an, weitere Screenshots zu schicken.

## Entscheidung KI-Weg (26.09.2026)

Tobi hat sich fuer **Teilen an die ChatGPT-App** entschieden. Damit nutzt er sein Plus-Abo, es kostet
nichts und funktioniert auch unterwegs; der Preis ist Handarbeit.

Bauweise daraus:
- **Barcode -> Open Food Facts** wird in jedem Fall gebaut: frei, kein Schluessel, echte Naehrwerte
  statt Schaetzung. Deckt alles Verpackte ab.
- **Selbstgekochtes / Multimix** laeuft ueber einen Teilen-Ablauf: Live Up stellt die Frage samt Bildern
  zusammen, schickt sie per Android-Teilen an ChatGPT, Tobi kopiert die Antwort zurueck, Live Up liest
  die Werte daraus aus. Die Antwort sollte deshalb in einem festen, leicht erkennbaren Format angefordert
  werden, damit das Zurueckschreiben nicht in Handarbeit ausartet.
- Die KI-Anbindung wird so gekapselt, dass spaeter ein API-Schluessel oder Ollama eingehaengt werden kann,
  ohne die Oberflaeche anzufassen.

## Umbau auf 0.2 (26.09.2026) — nach der UI-Recherche

Vier Recherche-Agenten haben Kalorientracker, Trainings-Apps, Schlaf-Apps und Stimmungs-Tagebuecher
ausgewertet (118 Web-Abrufe). Die wichtigsten uebernommenen Befunde:

- **Fuenf Haupt-Tabs, jeder Bereich innen flach:** Heute · Essen · Training · Schlaf · Ich.
  Material 3 sieht drei bis fuenf Ziele vor und **warnt ausdruecklich davor, Tabs an die
  Navigation Bar zu haengen oder zu verschachteln**. Damit ist Tobis "mehrere Apps in einer App"
  beantwortet, ohne Untertab-Baum.
- **Alle Einstellungen im Ich-Tab**, nie auf dem Startbildschirm (Muster von Fitbit, Google Maps, Centr).
- **Kein Loeschkreuz pro Zeile.** Antippen oeffnet das Bearbeiten, Loeschen liegt dort drin.
  Der Wochenstreifen auf "Heute" ersetzt die Verlaufsliste.
- **Ein grosser Ring statt drei**, Rest statt Verbrauch; Makros als Leisten darunter.
  Drei Boegen uebereinander sind auf dem Handy schlecht lesbar.
- **Feste Mahlzeiten-Slots** (Yazio) statt flacher Zeitliste — Lifesum hat umgestellt und wird dafuer kritisiert.
- **Startseite kurz halten:** Samsung Health 2026 und Oura werden fuer ueberlange, dunn belegte
  Startseiten kritisiert; bei Samsung Health tragen 37 % der Beschwerden den Wortlaut "altes Design zurueck".

### REM ist nicht messbar — wichtig

Die Polysomnographie-Studie **Fino et al. (2020, Journal of Sleep Research)** hat vier Handy-Apps
geprueft: **keine einzige konnte REM-Schlaf erkennen.** Eine REM-Prozentzahl ohne Uhr waere erfunden.
Live Up zeigt deshalb nur Wach/Leicht/Tief, und REM erst, wenn eine Smartwatch Daten liefert.

### Gebaut

- Navigation auf fuenf Tabs, "Ich" mit Gewicht, BMI, Tageszielen und Profil.
- Neues "Heute": Wochenstreifen (nach Befinden eingefaerbt), Befinden-Karte mit einem Tipp,
  drei Kacheln, Start-Chips. Neue Tabelle `mood`, Datenbank auf Version 2 per ALTER TABLE
  hochgestuft statt neu angelegt — bestehende Eintraege bleiben erhalten.
- Neues "Essen": Ring, Makro-Leisten, drei Abläufe (Schaetzen / Barcode / Selbst), vier Slots.
- **Schaetzen ueber ChatGPT-Teilen** (`Estimate.kt`): Live Up baut Frage plus Fotos und die
  Anweisung, in einer Strich-Tabelle zwischen `### LIVEUP` und `### ENDE` zu antworten.
  Der Parser ist absichtlich tolerant — Markdown-Tabellen, Kopfzeilen, Sternchen, Komma-Dezimalzahlen
  und Prosa drumherum werden verkraftet. 8 Tests dafuer.
- **Barcode** ueber `play-services-code-scanner` (fertige Vollbildoberflaeche, **keine
  Kameraberechtigung noetig**) und Abfrage bei **Open Food Facts** (frei, ohne Schluessel).
  Unbekanntes Produkt fuehrt direkt ins Eingabefeld statt in eine Fehlermeldung — genau dort
  brechen Nutzer laut dem dokumentierten OFF-Fehler sonst ab.
- Training ist bewusst ein ehrlicher Platzhalter: das Nachtrag-Formular ist raus, der Ablauf-Player
  noch nicht drin.

### Gefundene Fehler

- Das Antwortfeld war **einzeilig** — eine mehrzeilige ChatGPT-Antwort waere beim Einfuegen zu einer
  Zeile zusammengefallen und der Parser haette Murks bekommen. `Field` hat jetzt `lines`.
- `{ version++ }` ist `() -> Int`, erwartet wurde `() -> Unit`.

### Geraet

Das S9+ (`SM-G965F`, seriell `2b9d573a3c027ece`) haengt per USB am PC und laesst sich mit
`adb -s <id>` direkt bespielen. **Es laeuft Android 9, nicht Android 10** — der fruehere Eintrag
war falsch. Getestet wurde 0.2 direkt darauf: fuenf Tabs, Schaetzen-Ablauf und Parser laufen.

## Neuausrichtung 27.09.2026 — verbindliche Vorgaben von Tobi

Tobi war unzufrieden: Er fühlte sich überhört ("bitte missachte nicht meine Anweisung"). Diese Liste ist verbindlich:

1. **Komplett neues Design** — schön, professionell, "was jeder gern nutzen würde". **Nicht schwarz-grün**, nicht nach KI aussehend.
2. **Beim App-Start eine App auswählen.** Jede App bleibt für sich ("einzelne Apps in einer App"):
   **Übersicht · Essen/Kalorien · Training · Schlaf · Sperre · Lernen · Lebensplan · Einstellungen.**
3. **Training mit 3D-Modellen**, die zeigen, wie eine Übung geht und wie lange; richtige Übungsbibliothek, richtige Pläne.
4. **Schlaftracker muss funktionieren** und **aussehen wie die Vorlage** (Leap Fitness Schlaftracker).
5. **Einstellungen**, in denen man alles einstellen kann.
6. **Kleines Kaloriendefizit**, einstellbar.
7. **Alle Berechtigungen sauber** anfragen und verwalten.
8. **Echte Designs von Menschen** ansehen (Dribbble & Co.) und sich daran orientieren.
9. **Erst nach Open-Source-Projekten suchen**, die Teile schon lösen, statt alles neu zu erfinden und Fehler einzubauen.
   (ChatGPT fragen hat er vorgeschlagen — Claude hat darauf keinen Zugriff.)
10. **Schriften:** darf beliebige herunterladen und einbauen.

## Befund Nacht 26./27.09.2026 (Schlaftracker "hat nicht funktioniert")

Am Handy ausgelesen:
- **Aufgezeichnet hat er doch:** Nacht 23:09 -> 07:07, 478 Messpunkte, keine Lücke. Der Dienst überlebte die Nacht (obwohl nicht von der Akku-Optimierung ausgenommen).
- **Nicht geweckt, weil der Wecker aus war:** `alarm_on` stand nie in den Einstellungen, Standard war `false`. Tobi hatte die Weckzeit auf 05:35 gezogen. Fehler von Claude: Wecker muss standardmäßig AN sein.
- **Sah leer aus, weil die Schwellen falsch waren:** echtes Grundrauschen Median 0,011 (P90 0,016, P99 0,07, Max 0,35). Die festen Schwellen 0,05/0,35 stuften fast alles als "Tief" ein.
- **Lösung:** Schwellen relativ zum Grundrauschen der jeweiligen Nacht (Median). Stufen: <1,5x ruhig, <3x leicht, <6x stark, darüber Wach; Tief = 10 min vor und nach ohne jede Regung. An der echten Nacht getestet: ca. 30 % Tief, 66 % Leicht, 2 % Wach, Tiefschlaf-Blöcke etwa alle 90 min in der ersten Hälfte — plausibel.
- Parallel lief die Vorlage-App (Leap Fitness) und funktionierte einwandfrei.


## Wecker neu gebaut (27.09.2026, nachmittags)

Muster aus Simple Alarm Clock (Apache-2.0). Datei `Wecker.kt`.
- **Feste Weckzeit steht immer im System** (`setAlarmClock`), sobald "Alarm" an ist — auch ohne "Jetzt schlafen". Vorher hing der Wecker komplett am Messdienst.
- **Klingeln im Vordergrunddienst** (`KlingelDienst`): Ton über den Wecker-Kanal (geht durch "Nicht stören"), wird in ~30 s lauter, Vibration, Weckbildschirm. Grund: Bei eingeschaltetem Bildschirm zeigt Android nur ein Banner, die alte Weck-Activity wäre stumm geblieben. Steht der Weckerton auf 0, wird er dafür auf halb gestellt und danach zurückgesetzt.
- **Schlummern 5 min** und **Aus** im Weckbildschirm und in der Benachrichtigung.
- Der Messdienst weckt nur noch **früher** (Leichtschlaf im Fenster); die feste Zeit für heute wird dann übersprungen, damit es nicht zweimal klingelt. Beim Wecken endet die Nacht automatisch.
- **Neustart, App-Update, Zeitumstellung:** `SystemEmpfaenger` stellt Wecker und Erinnerung neu und setzt eine laufende Nacht (< 16 h) fort.
- Auf Tobis S9+ getestet (27.09. 14:49): Wecker löste pünktlich aus, Ton + Vibration + Weckbildschirm liefen, Tobi hat über "Aus" in der Benachrichtigung beendet. Danach wieder auf **Mo 28.09. 5:35** gestellt.
- **Achtung:** "Stopp erzwingen" in den Android-Einstellungen löscht bei jeder App alle Wecker, bis sie wieder geöffnet wird.
- Weckbildschirm im Schlaf-Design (Navy-Verlauf, Bernstein-Mond, Outfit-Schrift), nicht mehr schwarz-grün.

## Neubau der Apps (27.09.2026, nachmittags)

**Einstellungen** (fertig): Profil (Name, Geburtsmonat, Größe, Gewicht mit Verlauf und Trend kg/Woche, BMI nach WHO-Alterskurve), Ernährung & Ziel (Aktivitätsstufe NASEM, Defizit 0/5/10/15 % gedeckelt 500 kcal, automatisch oder Handwert, Makros "Aufteilen": 1,6 g Eiweiß/kg, 30 % Fett, Rest KH; Nachsteuer-Hinweis nach 14 Tagen Gewichtsdaten), Wecker & Schlaf (alles, inkl. Weckton, Erinnerung, Zuverlässigkeits-Tipps für Samsung), Berechtigungen (Pflicht / für die Sperre, jeweils "Erlauben" → Systemseite), Quellen & Lizenzen.

**Essen** (fertig): Tagesring "kcal übrig", Makro-Balken, 7-Tage-Streifen (Punkt grün = im Ziel ±10 %), Mahlzeiten-Karten mit Plus. Eintragen auf 4 Wegen: Barcode (Open Food Facts, Produkte werden lokal gemerkt → zweiter Scan offline), ChatGPT-Schätzen (Teilen + "Aus Zwischenablage einfügen"), "Zuletzt gegessen" mit Mehrfachauswahl, von Hand (Menge rechnet Werte je 100 g um). Verlauf 7/30 Tage mit Ziellinie, Ø kcal, Tage im Ziel, Ø Eiweiß.

**Training** (fertig, noch nicht am Handy angesehen): eigene **3D-Figur** (Canvas, 17 Gelenke, Hände/Füße per IK, Kamera per Wischen drehbar, Tiefenschattierung, Geräte Kasten/Stange/Wand). **38 von 42 Übungen animiert** (Beine, Drücken, Ziehen, Rumpf, Sprünge & Parkour, Mobilität); ohne Animation: Schulterrolle, BWS drehen, Schulter an der Wand. Jede Übung: Muskeln (nach free-exercise-db, gemeinfrei), Schritte, typische Fehler, leichtere/schwerere Variante. Bewegungen am PC geprüft: Test rendert alle Übungen als Bild (build/figur) und prüft Knochenlängen und Bodenkontakt. **Progression** (doppelte Progression nach wger-Konzept): alle Sätze geschafft → nächstes Mal +1 Wdh bzw. +5 s, wird im Plan gespeichert; ab 20 Wdh Hinweis auf schwerere Variante. Neue Pläne: Ganzkörper Einsteiger, Core 10 Minuten, HIIT 15 Minuten (zusätzlich zu Parkour Basis, Oberkörper, Beine & Sprungkraft, Mobilität). Workout-Player: Figur zeigt die aktuelle (in der Pause die nächste) Übung, Wiederholungen mit −/+, Zeitsätze mit Ring-Countdown, Pause −15/Weiter/+15.

**Schlafscore:** Dauer zählt jetzt mindestens gegen 8 h (AASM: 8–10 h mit 13–18 J). Vorher gab Tobis Wunschziel 23:30–5:35 (6 h 05) volle Punkte für 7 h 47. Im Wecker-Editor steht bei kürzerem Ziel "Mit 16 brauchst du 8–10 Stunden."

## Neubau Teil 2 (27.09.2026, später Nachmittag) — alle 8 Apps fertig

**Sperre:** Bedienungshilfe erkennt die Vordergrund-App (nur Fensterwechsel + Paketname, liest KEINE Inhalte), sperrt per HOME + eigenem Sperrbildschirm im eigenen Task (Verfahren von TimeLimit/Curbox, kein GPL-Code kopiert). Regel pro App: **Ablenkung** (gesperrt im Fokus und nachts), **Tageslimit**, **immer sperren**. **Fokus-Modus** 25/50/90/120 min (hart, keine Ausnahme; Geräteadmin-Abschalten wird währenddessen blockiert). **Nachtsperre** nutzt Schlafenszeit und Wecker aus der Schlaf-App. Bei Limit/Nacht: "Noch 5 Minuten" max. 2×/Tag mit Hürde (Grund wählen + 15 s warten). Sperrbildschirm bietet "Stattdessen: Karteikarten / Kurz trainieren" (öffnet Live Up direkt dort). Bildschirmzeit heute/7 Tage aus UsageEvents (Reef-Verfahren, Schnitt an Mitternacht). Geräteadmin optional mit Warntext. Kein VPN (Konflikt mit TB), kein Private DNS (bräuchte adb-Systemeinstellung).
Einrichten am Handy: Einstellungen → Eingabehilfe → Installierte Dienste → "Live Up Sperre" an; Nutzungsdaten erlauben.

**Lernen:** Karteikarten mit **FSRS v6** (wie Anki), portiert aus der offiziellen Referenz py-fsrs (MIT) und gegen deren Test geprüft (Intervallfolge 0, 2, 11, 46, 163, 498, … exakt gleich). Eine gefundene Kotlin-Portierung war fehlerhaft (falsche Vergessenskurve, Absturz bei deutschem Zahlenformat) und wurde nicht genommen. Karten: von Hand, als Liste einfügen (| ; Tab), oder **per ChatGPT aus Thema oder Heftfotos** (Teilen + Zwischenablage). Lernrunde mit Aufdecken und 4 Knöpfen mit Intervall-Vorschau; Reihenfolge wie Anki. Statistik: Abfragen/Tag, % gewusst, neu/im Lernen/gefestigt, Vorschau 7 Tage, neue Karten pro Tag einstellbar (Standard 20). Eigene Datenbank lernen.db.

**Lebensplan:** Lebensrad mit 8 Bereichen (Körper & Training, Schlaf, Ernährung, Schule & Lernen, Freunde & Familie, Hobbys & Parkour, Handy & Fokus, Kopf & Zukunft), Wochen-Check-in 1–10 + "Was lief gut / Was nehme ich mir vor", Vergleich zum letzten Mal, echte Zahlen aus den anderen Apps je Bereich, Vorschlag für den schwächsten Bereich, Ziele mit Etappen und Frist, Rückblick. Eigene Datenbank leben.db.

**Übersicht ("Dein Tag"):** drei Ringe (Essen, Training, Schlaf), Kachel je App mit Sprung in die App, Wochenzahlen, regelbasierter Tipp (größte Lücke: zu wenig Schlaf, viel Bildschirmzeit, Kalorienziel, Training, Eiweiß).

**Aufgeräumt:** alte Bildschirme aus 0.1/0.2 (Heute, Ich, altes Essen, altes Training, schwarz-grünes Theme) gelöscht. Schlafscore in der Auswahl kommt jetzt direkt aus der Berechnung statt aus Text. 67 Unit-Tests grün.

## Rückmeldung 27.09.2026 (nachmittags)

- **Lebensplan** hat Claude richtig verstanden (Tobi bestätigt).
- **Schlaf-App neu gestalten:** nicht 1:1 die Leap-Fitness-Vorlage, sondern eigenes Design passend zum hellen Rest von Live Up, "damit es nicht so schwarz ist"; Inspiration aus echten Designs im Netz.
- **The Bridge** soll als eigene App-Kachel in Live Up.
- Tobi will, dass Claude überlegt, was die App noch besser machen würde, und **Beispiele** baut, die es noch nicht gibt — z. B. einen **auf ihn angepassten Trainingsplan**. Er bietet an, Daten von ChatGPT zu holen, falls nötig.
- Die Bedienungshilfe "Live Up Sperre" hat Tobi selbst eingeschaltet (am 27.09. um 16:17 aktiv gesehen).

## Weitere Wünsche (27.09.2026, abends)

- **Lernen richtig ausbauen** ("wirklich viel Mühe"): Das ChatGPT-Format hat bei Tobi nicht funktioniert — es kam nur **eine** Karte an statt vieler. Außerdem will er **Tests/Lektionen**: ChatGPT bringt ihm etwas bei, dann ein Test, bei dem er Antworten **ausfüllt**.
- **Essen – Portionsberater:** Beim Kochen fragen können: "Das Essen ist für N Personen — wie viel davon soll ich essen, damit ich mein Ziel einhalte?"
- **Training:** findet er "schon echt gut". Soll auf ihn angepasst sein: **totaler Anfänger**, die ersten Wochen bei fast nichts beginnen und langsam steigern.
- **Sperre "komplett fest":** nicht auseinandernehmbar, aber mit einem geheimen **Sperrcode wie eine Seed-Phrase** theoretisch überspringbar. Tobi: "mal schauen", Claude soll darüber nachdenken.
- **Später:** eine Begleit-App für **Wireless Debugging** (ADB ohne Kabel über einen speziellen Zugang) — ausdrücklich "kommt später".
- Umsetzung läuft in Arbeitskopien unter `A:\LiveUp-work\` (schlaf, bridge, training, lernen, essen, sperre), danach Zusammenführung nach `A:\LiveUp`.

## Große Vision (28.09.2026, von Tobi diktiert) — noch nicht gebaut

- **Eigener Launcher** statt Startbildschirm: zuerst Statistiken sehen, Knopf/Suchleiste zu allen Apps (wie der Launcher davor, nur besser). Nach einem Neustart soll man nicht auf dem normalen Startbildschirm landen.
- **Gesundheits-Log** als neue App (Tobi hat eine Nierenkrankheit): Kranktage, Befinden, evtl. Blutdruck.
- **Brain-Tab** (später): Zugriff auf AIs Room.
- **Finanzen** ("Finanzguru"-artig).
- **Kalender**, mit dem ALLE Daten aus allen Apps verknüpft sind.
- **Lernen wird zu "Schule"**, Sdui-Anbindung (eigener MCP für Sdui) als Wunsch. Tabs: **Heute/Übersicht** (Hausaufgaben, Fächer anpassbar, Lernaufgaben, regelmäßige Aufgaben wie Buch/Bibel lesen), **Arbeiten** (anstehende Arbeiten/Prüfungen mit Infos, was drankommt, Notenschlüssel; Probearbeiten von ChatGPT erzeugen lassen oder selbst schreiben), **Lernen wie Anton** (ChatGPT-Lektionen: lernen → abfragen → weiterlernen), **Karteikarten mit Fach-Filter**, **Ablage**: Schulzeug hochladen, per KI (ChatGPT, PC wenn an) in Stapel sortiert.
- **Einstellungen fürs Handy**: Samsung-Schnellleiste (Herunterziehen) selbst gestalten, Benachrichtigungen ändern.
- **Mini-LLM auf dem Handy** (kleines lokales Modell) mit Zugriff auf andere Daten, organisiert Kleinkram.
- **Wireless-Debugging-Begleit-App** (ADB ohne Kabel, spezieller Zugang) — Tobi: "wenn du fertig bist".

## Stand 28.09.2026 mittags

- **Lernen**: Reiter "Tests" mit **Lektion & Test** fertig und auf dem Handy getestet: Lektion lesen, Frage für Frage ausfüllen (Lückentext, Multiple Choice, Wahr/Falsch, Kurzantwort), strenge Bewertung bei Grammatik (englische Kurzformen zählen gleich), Note, Auswertung je Kapitel, Fehler wiederholen oder als Karteikarten lernen. ChatGPT-Austausch jetzt per **JSON-Codeblock** (Grund für "nur eine Karte": die ChatGPT-Android-App kopiert Tabellen als reinen Text, die Striche gehen verloren). Mitgeliefert: **8 Englisch-Grammatik-Lektionen** (Zeiten, if-Sätze, Passiv, indirekte Rede, Relativsätze, Gerund/Infinitiv, Adjektiv/Adverb, Gesamttest) mit 95 Fragen.
- **Essen**: Portionsberater ("Gericht für N Personen — wie viel darf ich?") und JSON-Format für die Schätzung übernommen.
- Schlaf (hell, Schlafphasen im Mittelpunkt), Bridge, Training (Anfänger-Programm), Sperre (Sperrcode fürs Lockern) werden gebaut. Die Sperre bekommt bewusst KEIN Abfangen von System-Einstellungen/Deinstallation — nur den Sperrcode innerhalb der App.
- Letzte Nacht (27./28.09.) wurde nicht gemessen ("Jetzt schlafen" nicht gedrückt).

## Eiweiß wegen Nierenerkrankung umgestellt (28.09.2026) — am Abend wieder aufgehoben, siehe unten

Vorher empfahl die App 1,6 g Eiweiß/kg (~130 g) und schlug bei "fehlendem" Eiweiß Skyr, Quark, Eier vor. Wegen Tobis Nierenerkrankung geändert: Eiweißziel 0,9 g/kg (DGE-Referenz 15–18 J., ~75 g), keine "mehr Eiweiß"-Tipps mehr (Portionsberater, ChatGPT-Tellerfrage, Übersicht-Tipp), in den Einstellungen der Hinweis, das Ziel mit dem Arzt abzustimmen. Offen: Welche Vorgaben hat sein Arzt (Eiweiß, Salz, Kalium, Phosphat, Trinkmenge)? Das sollte in die Einstellungen, sobald bekannt.

**Korrektur 28.09.2026 abends (Tobi):** „Mit dem Protein, das kannst du wegmachen. Da habe ich gar keine Begrenzung dazu.“ Umgesetzt (Anforderung T1/G4): keine Eiweiß-Obergrenze, keine Nieren-Hinweise in der App, kein alter hoher Wert wird automatisch eingesetzt. „Aufteilen“ schlägt 0,9 g/kg nur als Richtwert vor (DGE), das Ziel ist frei änderbar. Portionsberater und ChatGPT-Tellerfrage ohne Eiweiß-Vorgaben.

## Arbeitsauftrag und Masterliste (ab 28.09.2026 abends)

Tobi hat einen vollständigen Anforderungskatalog geschickt (IDs A1–K), abgelegt unter `A:\LiveUp\docs\Arbeitsauftrag-2026-09-28.md`. Der Status jeder Anforderung (offen / in Arbeit / in Arbeitskopie / integriert / am Gerät geprüft / zurückgestellt) steht **nur** in `A:\LiveUp\docs\Anforderungen.md`, dazu seine direkten Wünsche T1–T14. Reihenfolge laut Auftrag: 0 Bestand → 1 Stabilität (Schlafabstürze, Trainings-Timer, Bridge-403) → 2 Historie/Kalender/Aufgaben → 3 Schule → 4 PDF/KI → 5 GPS/Pläne → 6 Fokus → 7 Sdui/Updates → später Launcher, Gesundheit, Finanzen, Brain, Mini-KI.

- Ursache der Schlafstatistik-Abstürze (C1): ein früher `return@Column` in einer Compose-Spalte (SchlafAuswertung.kt) → `ArrayIndexOutOfBoundsException … Stack.pop`. Dasselbe Muster steckte in der Karteikarten-Runde. Regel für den Code: in Inline-Composables nie früh zurückspringen, sondern if/else.
- Ursache des wiederkehrenden Bridge-403 (H1): siehe [[The Bridge]].
- Sperre mit Sperrcode (T7) vorerst nicht gebaut; Tobi plant die Sperre neu (F1).

## Stand 29.09.2026 früh — Version 0.2.0 installiert

- **Aufgaben-App (A3, neu):** einmalige Aufgaben, Termine, Hausaufgaben, Arbeiten, wiederkehrende Gewohnheiten (z. B. Bibel lesen). Eine Aufgabe = ein Datensatz, Serien einzeln auslassen/verschieben, Erledigt-Liste mit Datum, Erinnerungen (zur Uhrzeit, vorher, Vortag 18 Uhr). Damit kann Tobi ab 0.2.0 Hausaufgaben und Arbeiten eintragen (T6).
- **Essen-Verlauf (A1):** Tag per Kalender wählbar, Verlauf Woche/Monat/Jahr/Gesamt/frei; jeder Tag antippbar. Es gibt keine automatische Löschung nach 30 Tagen, nur Anzeige-Begrenzungen.
- Englisch-Lektionen entfernt (T2), nichts wird automatisch erzeugt (T3).
- In Arbeit (Arbeitskopien unter `A:\LiveUp-work\`): Paket 1 (heller Schlaf mit Phasen und neuer Statistik, Trainings-Timer mit Pause/Abbruch, Bridge-403-Reparatur) und Paket 3 (Lernen → **Schule** mit Fächern, Stundenplan, Hausaufgaben mit Foto, Arbeiten mit echtem Notenschlüssel, Lernen wie Anton, Materialablage).
- Nacht 28./29.09.: Tobi hat mit Live Up gemessen (20:57–23:00 und 23:08–04:50). Die fremde App „Sleep Recorder“ war danach nur geöffnet (Einstellungen), Tobi hat damit **nicht** gemessen und war wach.

## Vorgaben 29.09.2026 (Tobi, diktiert) — verbindlich

- **Schlaf (T15/T17):** Basis ist die Schlafphasen-App aus 0.2.0, alle Bildschirme hell. Kein Nacht-/Gute-Nacht-Messbildschirm, nur ein schlichter Hinweis „Messung aktiv – du kannst das Handy ausschalten“. Die Statistik gestaltet Claude eigenständig, nichts von anderen Apps übernehmen. Inhalt: wie lange ungefähr geschlafen, geschätzte Phasen, in welcher Phase vermutlich geweckt; Wecker bleibt. Der zuvor gebaute helle Neubau (Kopie `schlaf`) wird deshalb nicht übernommen.
- **Training (T16):** Dashboard „Heute geplant“ (trainiere ich heute oder nicht, antippen startet), spontan Laufen oder Radfahren mit GPS ohne Uhr. Plan selbst erstellen oder per ChatGPT für Tobi erzeugen lassen (seine Daten und sein Können mitgeben, ChatGPT darf Rückfragen stellen), über Wochen steigernd, um eine Gewohnheit aufzubauen.
- **Geldguru (T18, neu):** wie Finanzguru.
  - Heute: Kontostand, Bargeld, Gesamtvermögen, Ein- und Ausgaben pro Woche, Verlauf.
  - Hinzufügen: Geld ein/aus, bar und Konto, Kartenzahlung. Tobi hat nur eine Bankkarte, kein Online-Banking, also keine automatische Kontoverbindung. Belege, Bargeldfotos und Kontoauszüge per ChatGPT auswerten; doppelte Belege erkennen; Auszug und Kassenzettel zusammenführen, damit man sieht, was gekauft wurde.
  - Budgets/Kategorien/Sparen: monatlich und jährlich, Sparziele, Geld mit Grund zurücklegen (Beispiel von Tobi: 900 € „gehört nicht mehr mir“). Ausdrücklich **keine** Beispieldaten anlegen.
  - Statistiken.
- **Sperre:** vorerst „Wird entwickelt“, alles Übrige entfernt (0.2.1). **Lebensplan:** „Wird entwickelt“, Daten bleiben.
- **Aufgaben** hätte Tobi gern wie „XDO“, eine App, die ihm früher angeblich jemand gebaut hat. Sie ist auf PC, Brain und in den Protokollen nicht auffindbar; Rückfrage offen.
- **Kalender:** „Kalender war schon nice“, soll kommen (A2). **Schule** so wie im Arbeitsauftrag.
- Tobis Reihenfolge: WLAN-Debugging → Bridge (bei Fehler protokollieren und automatisch neu verbinden) → Schlaf hell → Training → Schule → Kalender → Geldguru. WLAN-Debugging und Bridge setzt Claude nicht weiter um, weil die Arbeit daran wiederholt von der Sicherheitsprüfung angehalten wurde. Umgesetzte Reihenfolge daher ab Schlaf.

## Stand 29.09.2026 mittags — Version 0.2.1

Am Gerät mit Tobis echten Daten geprüft:
- Schlafstatistik ohne Absturz, auch bei leeren Tagen und Wochen.
- Englisch-Lektionen weg.
- Aufgaben: anlegen, abhaken, rückgängig, löschen.
- Essen-Verlauf „Gesamt“, Tag antippen.
- Sperre und Lebensplan als Platzhalter.

Danach am selben Abend weitergebaut, siehe nächster Abschnitt.

## Stand 29.09.2026 abends — Version 0.5.0 (alles am Gerät mit Tobis Daten geprüft)

Tobi wünschte ab 17 Uhr ausdrücklich: **wenig Agenten** (Nutzungslimit), lieber langsam, einzeln abhaken. Getestet wird über sein selbst eingerichtetes WLAN-ADB, bei Ausfall über USB. Claude nutzt Tobis Sperrbildschirm-Code nicht (PINs eingeben ist tabu).

- **Schlaf (0.3.0):**
  - Aufbau von 0.2.0, alle Bildschirme hell.
  - Statt Nachtbildschirm nur „Messung aktiv – du kannst das Handy jetzt ausschalten“, Beenden mit Rückfrage, der Bildschirm wird nicht wachgehalten.
  - Eigene Statistik (Woche/Monat/Jahr/Gesamt/frei): geschätzter Schlaf, Phasenanteile, „vermutlich aufgewacht aus …“, alle Messungen mit Grund.
  - Zu kurze oder leere Messungen zählen nicht: 20:57–23:00 „Nicht gewertet“, 17/19/1 Punkte „Zu kurz“.
- **Training (0.3.3):**
  - Timer B1/B2 am Gerät bestätigt: keine vorzeitige „Geschafft“-Wertung, Pause, Abbruch dokumentiert, Hintergrund hält an, keine ±15 s.
  - Dashboard „Heute geplant“, ein Planmodell (Anfängerprogramm, ChatGPT-Plan, eigener Plan).
  - Plan mit ChatGPT: Prompt mit Rückfragen und App-Daten, ohne Gesundheitsdaten; einfügen, Vorschau, aktivieren.
  - GPS-Laufen und -Radfahren ohne Uhr: Test draußen steht noch aus, die Standort-Erlaubnis gibt Tobi selbst.
  - Verlauf mit Details und Löschen, Kalorien nur als Schätzung (MET-Formel sichtbar).
- **Absturz-Lehre:** `LocalDate.ofInstant` (Java 9) fehlt auf Android 9 und ließ den Trainings-Verlauf abstürzen. Behoben. Seitdem vor jeder Installation Android-Lint (NewApi).
- **Geldguru (0.4.x):**
  - Start ohne Beispieldaten.
  - Heute: Vermögen, Konto, Bargeld, zurückgelegt, frei verfügbar, Woche, Verlauf.
  - Hinzufügen: Ausgabe, Einnahme, Umbuchen, dazu ChatGPT-Auswertung von Kassenzettel, Kontoauszug und Bargeld mit Dubletten-Erkennung und Zusammenführen von Auszug und Kassenzettel.
  - Budgets, Töpfe mit Grund und Sparziel, Kategorien, Statistik.
  - Karte verbinden geht nur mit Online-Banking. Das ist erklärt, bis dahin gibt es Auszug-Fotos.
- **Aufgaben wie XDO (0.5.0):**
  - Tobis Vorbild ist „XDO – die ToDo-App“ von Rau Media GmbH (iOS; Tobi sagte „David Rau“). Übernommen ist die Mechanik, nicht das Aussehen.
  - Reiter Heute / Eingang / Listen, Schnell-Eingabe mit Enter, Stern statt Prioritäten.
  - Aufgaben wandern je nach Datum oder Liste von selbst.
  - Langer Druck: Stern, heute oder morgen, nach oben oder unten.
- **Sperre und Lebensplan:** „Wird entwickelt“, bei der Sperre ist der Rest entfernt.
- **Offen:**
  - Schule (Paket 3), Kalender (A2).
  - Ein Fehler im Kalorientracker, den Tobi gefunden hat. Was genau, ist noch unbekannt.
  - Icon-Farbe (G1).
  - GPS-Test draußen.
  - WLAN-Debugging und Bridge setzt Claude nicht weiter um, weil die Sicherheitsprüfung die Arbeit daran mehrfach angehalten hat.

## Wünsche 29.09.2026 abends (Version 0.5.1 gebaut, noch nicht installiert)

- **Geldguru mit ChatGPT:** ChatGPT soll alles eintragen und ändern können, also alte Rechnungen, alte Kontoauszüge und ganze Listen, mit Datum und Namen. Tobi gibt ChatGPT die Unterlagen und fügt einen Codeblock ein. Gebaut ist die Seite „Mit ChatGPT eintragen“: Die App gibt ihren Stand samt aller Buchungen mit Nummer mit. Das JSON kann neu, ändern, löschen, Kategorien, Budgets, Töpfe und Kontostand enthalten. Vor dem Übernehmen gibt es eine Vorschau mit Dubletten-Abgleich.
- **Online-Banking noch nicht.** Tobi fotografiert Auszüge. Fotografieren geht jetzt direkt in der App, die Originale bleiben im App-Speicher.
- **Startbildschirm:** meistgenutzte Apps vorne, der Rest dahinter.
- **Später: Live Up als Launcher (T11).** Oben die Benachrichtigungen, darunter das Heutige und eine Eingabe bzw. Suche, welche App geöffnet werden soll.

## 30.09.2026 — Wecker von Family Link blockiert, Version 0.6.0

- **Befund Wecker (T28):** Die Weckzeit stand auf **03:00** mit 60 Minuten Aufwachphase und Schlafenszeit 22:15. Am 29.09. um 17:15 war sie noch auf 05:31 mit 30 Minuten gewesen, verstellt hat sie also nicht Claude. Tobi bestätigte am 30.09.: **03:00 war gewollt.** Die Messung lief von 21:54 bis 02:41. Um 02:41:54 wurde Leichtschlaf erkannt und das Wecken versucht. Zwei Sekunden später zeigte Android „Vom Administrator deaktiviert“ (ActionDisabledByAdminDialog): **Google Family Link** (Profilverwaltung) hatte Live Up in der **Schlafenszeit gesperrt**. Die Sperrbildschirme von Family Link erschienen um 22:45, 04:50 und 05:01. Es gab weder Weckbildschirm noch Benachrichtigung, geklingelt hat nichts, und die Nacht war um 02:41 abgeschnitten.
- **Lösung in 0.6.0:**
  - Die App erkennt die Sperre (FLAG_SUSPENDED). Sie beendet dann nicht die Messung und merkt sich die Blockade.
  - Danach erscheint eine rote Warnung im Schlaf-Tracker und in den Einstellungen, dazu der Knopf „Ersatzwecker in der Uhr-App“.
  - Solange Family Link installiert ist, zeigt die App einen gelben Hinweis, den man ausblenden kann.
  - **Dauerhaft lösen kann es nur ein Elternteil:** Live Up in Family Link auf „Immer erlaubt“ setzen.
- **Geldguru „Stand aktualisieren“ (T29):** Konto und Bargeld neu eintragen. Der Unterschied wird entweder als Einnahme oder Ausgabe unter „Sonstiges“ gebucht und zählt dann in der Statistik, oder er wird nur angeglichen (Korrektur). Die Statistik zeigt den Vermögensverlauf gesamt, nur fürs Konto oder nur fürs Bargeld.
- **Schule (S1+S2):** Aus „Lernen“ wird die App **Schule**. Sie hat fünf Reiter:
  - Heute: Stunden des nächsten Schultags, fällige Hausaufgaben, nächste Arbeiten, Karten.
  - Hausaufgaben: nach Fälligkeit gruppiert, mit Fachfilter.
  - Arbeiten: Themen, echter Notenschlüssel (keine erfundenen Notengrenzen), Material, „Lernblock planen“.
  - Lernen: Karten, Stapel, Tests, Statistik.
  - Material: Kamera, Datei, PDF, Bild und Text, im App-Speicher; Doppelte erkennt die App.
  - Dazu kommt die Seite „Fächer & Stundenplan“ mit Farben, Umbenennen, Löschen, Stundenplan Mo–Fr und optionalen Stundenzeiten.
  - Hausaufgaben und Arbeiten bleiben in aufgaben.db (zusätzliche Felder Buch, Seite, Notenschlüssel, „Wann machst du sie?“), mit Material-Anhängen.
  - Offen ist noch S3: Lernen wie Anton, keine erfundene Note mehr, Schul-Einstellungen.
- **The Bridge:** Tobi hat am 30.09. erneut danach gefragt. Die Antwort bleibt: Claude baut sie nicht ein (H1/T22).
- **Nächste Schritte (Tobi, 30.09.):** Zur Schule gehört noch der **automatische Stundenplan und eine Vertretungsplan-Analyse** (E1). Plan: Screenshot oder PDF aus Sdui teilen, ChatGPT liefert einen Codeblock, daraus wird der Stundenplan bzw. die Änderungen je Tag. Das Sdui-Passwort kommt nicht in die App. Danach folgt der Kalender (A2).
- **Warum keine Bridge:** [[The Bridge]] tunnelt das Handy am Router vorbei, um die zeitgesteuerte Internetsperre der FritzBox zu umgehen, also die Aufsicht der Familie. Dabei hilft Claude nicht, auch nicht über den Einbau in Live Up. Das hat Claude Tobi am 30.09. so erklärt.
- **0.6.0 installiert und am Gerät geprüft (30.09. 18:35–18:55):** Schule (Fächer, Stundenplan, Hausaufgaben, Arbeiten, Lernblock, Material mit Bild und PDF), Geldguru „Stand aktualisieren“ (nur Vorschau, echte Daten nicht angetastet) und die Family-Link-Karte. Eine Prüfrunde (3 Prüfer, 1 Gegenprüfer) fand 10 Fehler, alle behoben. Neu dazugekommen: Die App merkt sich, wann Family Link sie sperrt und wieder freigibt. Sie warnt rot, wenn die Weckzeit in der Sperrzeit liegt, und **holt einen blockierten Wecker nach**, sobald die Sperre endet. Ob das Nachholen wirklich klingelt, lässt sich erst in einer echten Nacht prüfen.

## 01.10.2026 — Wecker bestanden, Anti-Scroll „Pause“ (T30)

- **Wecker in der Nacht zum 01.10.:** Weckzeit 04:00 mit 60 Minuten Fenster, Messung 21:36–03:36. Um 03:36 erschien der Weckbildschirm, Tobi bestätigte ihn nach 8 Sekunden. Family Link hat nicht gesperrt; Tobi hat in der App „Immer erlaubt“ bestätigt. Damit ist T28 in einer echten Nacht bestanden.
- **Neuer Wunsch (Plan aus ChatGPT, von Tobi eingefügt): Anti-Scroll-System „Pause“** an der Stelle der alten Sperre. Leitsatz: Impuls → Pause → Beten → Realität → Alternative → Ziel → bewusste Entscheidung → starke Reibung. Die Punkte stehen einzeln als P1–P14 in `docs/Anforderungen.md`.
  - Apps bekommen eine Stufe 0–4: Werkzeug, normal, Ablenkung, Scroll-Gefahr, gesperrt. Tobi stuft selbst ein, die Liste ist nach Nutzung sortiert.
  - Beim Öffnen kommt ein Bildschirm davor: „Einen Moment. Bete kurz und entscheide danach.“ mit Countdown ohne Überspringen. Danach folgen die Zahlen (heute, 7 Tage, Hochrechnung aufs Jahr), die Absicht, Alternativen, eine Atemübung (4-4-6) und eigene Ziele mit eigenem Bild.
  - Die Reibung wächst mit den Minuten des Tages (Grenze 90): 10 s, ab 15 min 20 s mit Absicht, ab 30 min 30 s mit Atmen, ab 45 min Ziele und Alternative, ab 60 min Papiercode, ab 90 min Schluss für heute.
  - Papiercodes: ein Blatt mit 40 Einmal-Codes, die App speichert nur Prüfsummen. Die erste Freigabe am Tag kostet 1 Code, die zweite 2, die dritte 3 plus 30 s, danach gibt es keine mehr. Ein neues Blatt gilt erst nach 24 Stunden.
  - Aufgaben als Voraussetzung: Scroll-Apps erst, wenn die Aufgaben mit Stern für heute erledigt sind.
  - Lockern (Stufe senken, Grenze erhöhen, Regel abschalten) braucht einen Papiercode, sobald ein Blatt eingerichtet ist.
  - Technik: Bedienungshilfe „Live Up Pause“ erkennt nur den App-Namen, keine Inhalte. Tobi schaltet sie selbst ein. Telefon und Einstellungen sind nie einstufbar. Die App verspricht keine unknackbare Sperre.
  - Später (Version 2): Reels und Shorts getrennt sperren, ein zugesandtes Reel 45 Sekunden.
- **Stand 01.10. 13:50: 0.7.0 mit „Pause“ Version 1 ist installiert.** Eine Prüfrunde fand 15 Fehler, alle behoben. Am Gerät im Probelauf geprüft: Stufen, Pause-Bildschirm, Zahlen, Absicht, Alternativen, Atmen, Ziele, Pflichtaufgaben-Regel, Papiercodes (Blatt, falscher und richtiger Code, Lockern mit 2 Codes). Alle Testeinstellungen sind wieder entfernt, Tobi hat noch kein Code-Blatt.
- **Noch offen bei „Pause“:** Tobi muss die Bedienungshilfe „Live Up Pause“ selbst einschalten (Einstellungen → Eingabehilfe → Installierte Dienste). Erst dann löst die Pause beim echten Öffnen einer App aus – das ist am Gerät noch ungeprüft. Reels/Shorts getrennt kommt als Version 2.
- Unter Pause → Heute startet ein Antippen einer App einen Probelauf ihrer Pause.
- Messwert vom 01.10.: Android zählte bis 13:30 Uhr 3:58 Stunden Instagram.

## 01.10.2026 nachmittags — Schule mit ChatGPT, NAS-Ablage, Schul-Brain (T31–T33)

- **Wunsch von Tobi:** Bei der Schule soll ChatGPT auch alles importieren können (T31). Alle Daten und Fotos sollen in einen Ordner auf dem NAS, je App ein Ordner (T32). Die Schule bekommt ein riesiges „Brain“ mit allen Schulbüchern und Übungen (T33).
- **T31 ist gebaut** (noch nicht installiert, Prüfrunde läuft): Schule → Heute → „Mit ChatGPT eintragen“.
  - Tobi teilt Fotos, Screenshots oder PDFs samt Auftrag an ChatGPT und fügt einen Codeblock ein.
  - Daraus werden Fächer, Stundenplan, Stundenzeiten, Hausaufgaben, Arbeiten, Vertretungen und Karteikarten. Vorhandene Hausaufgaben und Arbeiten lassen sich über ihre Nummer ändern oder löschen.
  - Vor dem Speichern gibt es eine Vorschau; schon Vorhandenes wird nicht doppelt angelegt, Rückfragen von ChatGPT werden angezeigt.
  - Vertretungen stehen in „Heute“ an der Stunde (entfällt = durchgestrichen) und in einer eigenen Liste. Dafür gibt es die Klasse als Einstellung, damit ChatGPT den Plan filtert.
  - Damit ist auch der Stundenplan- und Vertretungsplan-Import (E1) abgedeckt. Eine echte Automatik aus Sdui gibt es nicht (kein Passwort in der App).
- **T32/T33 Plan:** Ordner `//jacobnas/JacobNAS/Tobi/LiveUp/<App>/…`, für die Schule zusätzlich `Schule/Brain/<Fach>/…`. Das NAS bietet nur SMB (445) und SSH (22) an, kein FTP und kein WebDAV (am 01.10. geprüft). Die App braucht deshalb eine SMB-Bibliothek; dafür ist Tobis Zustimmung zum Download nötig. Die NAS-Zugangsdaten trägt Tobi selbst in der App ein.
- Das Handy hat intern nur noch etwa 1,5 GB frei (SD-Karte rund 52 GB). Schulbücher gehören deshalb aufs NAS und werden nur bei Bedarf geladen.

## 0.9.0 — Recorder-Modul (06./07.10.2026)

Neue Kachel **Recorder**: die Fernbedienung für den [[Audio-Recorder]] über Bluetooth LE (Anforderung T34 in `docs/Anforderungen.md`). Paket `de.tobidervogel.liveup.recorder`, Akzentfarbe Beerenrot, Symbol Mikrofon.

- Reiter Status, Steuern, WLAN, Dateien, dazu „Kopplung und Diagnose“ mit drei Testknöpfen (Verbindungstest, falscher Schlüssel, wiederholter Rahmen).
- `RecorderProtokoll.kt` (Verschlüsselung, reine Kotlin-Datei mit Tests gegen dieselben Vektoren wie die Firmware), `RecorderBle.kt` (Verbindung, nur die alten GATT-Aufrufe wegen Android 9), `RecorderKopplung.kt`, `RecorderDaten.kt`, `RecorderApp.kt`.
- Verbindung direkt über die Bluetooth-Adresse aus der Kopplung, ohne Suche und ohne Standort. Rechte: `BLUETOOTH` (bis Android 11), `BLUETOOTH_CONNECT` (ab Android 12).
- Kopplung: PC-Werkzeug `A:/Recorder/firmware/ar1/tools/pair.py` legt Adresse und Schlüssel per ADB in den App-Speicher; die App übernimmt sie beim Öffnen des Moduls und löscht die Datei.
- Gebaut von einem Agenten nach Claudes Spezifikation; 190 Unit-Tests grün (17 neu), Lint ohne NewApi. Sicherung des Stands davor: `A:/LiveUp-work/_basis-recorder`.
- **Am 07.10.2026 auf dem S9+ installiert und mit dem echten Recorder geprüft** (Firmware AR1.1): Kopplung über `pair.py`, Verbindung in ein bis zwei Sekunden, Status mit echten Werten, Start, Pause, Fortsetzen, Stopp, Timer setzen und löschen, Netzsuche, Dateiliste. Die drei Testknöpfe (Verbindungstest, falscher Schlüssel, wiederholter Rahmen) bestehen. 30 Minuten Aufnahme mit verbundener App ohne Sampleverlust. Eine STATUS-Abfrage dauert über Bluetooth rund 0,8 s. Nicht am Gerät geprüft: Schlafen, Ausschalten, WLAN-Daten eingeben und senden, WLAN an/aus, Autostart-Schalter, Timer zu einer Uhrzeit (das Handy war zuletzt gesperrt). Einzelheiten zur Firmware in [[Recorder AR1 Firmware]].
- Von Claude nachgebessert: 4 s Ruhe nach dem Schließen einer Verbindung; der WLAN-Reiter zeigt den Namen des verbundenen und der gespeicherten Netze; die Dateiliste zeigt Dauer und Größe und beginnt mit der neuesten Aufnahme.
- Das Handy war am 07.10. morgens nicht mehr über WLAN-ADB erreichbar (vermutlich Neustart); nach einmal Anstecken hat der Helfer WLAN-ADB wieder eingeschaltet.

## 07.10.2026 abends — Umbau gewünscht: fünf Module weg, Geld mit Wallet, Fahrschule

- **Tobis Auftrag (als eingefügter Text):** Lebensplan, Pause, Aufgaben, Schule und Schlaf „ganz weg“. Die Kachel „Geldguru“ soll „Geld“ heißen und nach dem Antippen eine Auswahl „Wallet“ oder „Geldguru“ zeigen. Neu dazu ein Modul „Fahrschule“: was schon gemacht ist (aus der Geldguru-Kategorie „Fahrschule“), Stunden zum Abhaken, verbunden mit dem Geldguru. Danach will er seine Finanzen prüfen lassen und dafür ein Video der Bank-Ansicht schicken.
- **Stand der App:** 0.9.0 mit dem Recorder-Modul vom 06./07.10. Der Schul-Import mit ChatGPT und der Teilen-Eingang vom 01.10. (0.8.0) sind noch im Code; die NAS-Anbindung wurde nie begonnen.
- **Noch nichts entfernt.** Gemacht ist nur die Bestandsaufnahme und eine Sicherung des Quelltexts unter `A:\LiveUp-backup\2026-10-07_vor-umbau\quelle` (das Projekt hat keine Versionsverwaltung).
- **Was beim Entfernen zu beachten ist:**
  - Der Wecker steckt zusammen mit der Schlafmessung im Kern der App. „Schlaf ganz weg“ würde ihn mitnehmen, wenn Tobi das nicht ausdrücklich ausschließt.
  - Der JSON-Leser liegt im Schul-Paket (`lernen/Einlesen.kt`), wird aber vom ChatGPT-Import des Geldgurus und vom Trainingsplan-Import gebraucht. Er muss vorher umziehen.
  - Daten auf dem Handy (Stand 01.10.): 10 gemessene Nächte, 2 Aufgaben-Serien, Schule und Lebensplan praktisch leer.
- **Tobis Antworten (07.10.):** Module raus **und Daten löschen**; der Wecker geht mit weg; Wallet „wie Google Wallet“ (Karten, Ausweise, Tickets, Schnellansicht, eigenes Design); Führerschein Klasse B / BF17.
- **(Überholt, siehe nächster Abschnitt.) Stand des Umbaus am 07.10. um 21:16:** Lebensplan und Pause sind aus dem Quelltext entfernt, Build und Tests grün. Offen sind Schule, Aufgaben, Schlaf mit Wecker, das einmalige Löschen der Daten, der Umbau der Übersicht, „Geld“ mit Wallet und das Fahrschul-Modul. Auf dem Handy läuft weiter 0.9.0.
- **Bank-Abgleich (T39, erledigt):** Tobi hat am 07.10. um 18:55 am Sparkassen-Terminal die Kontoumsätze gefilmt. Alle 30 Umsätze (12.08.–05.10.) stehen mit demselben Betrag im Geldguru, und der Kontostand stimmte zum Zeitpunkt des Videos auf den Cent. Käufe, die Tobi im Geldguru mit `*` markiert, fehlen noch in der Umsatzliste der Bank, sind in ihrem Saldo aber schon abgezogen. Das Admin-Detail dazu: Die Eröffnungsbuchung im Geldguru ist als normale Einnahme gespeichert.
- Tobi markiert im Geldguru Buchungen, die die Bank noch nicht zeigt, mit einem `*` vor dem Text.
- Das Handy war am 07.10. abends nicht verbunden; die letzte Datensicherung vom Handy ist vom 01.10.

## 07.10.2026, 21:57 — Umbau fertig: Version 0.10.0 auf dem Handy

- **Live Up besteht jetzt aus:** Übersicht, Essen, Training, Geld (Wallet + Geldguru), Fahrschule, Recorder, Einstellungen. Version 0.10.0 (versionCode 17), per USB installiert und am Gerät geprüft.
- **Entfernt (Tobis Entscheidung):** Lebensplan, Pause, Aufgaben, Schule, Schlaf samt Wecker, Weckbildschirm und Schlafenszeit-Erinnerung. Die Daten dieser Module sind beim ersten Start vom Handy gelöscht worden (einmaliges Aufräumen in `Aufraeumen.kt`; liveup.db ist Version 8 ohne Schlaf-Tabellen). Sicherung davor: `A:\LiveUp-backup\2026-10-07_2150_vor-installation\handy\`.
- **Live Up weckt nicht mehr.** Ein Wecker für 08.10. 03:45 war gestellt und ist mit der Installation abgesagt worden. Tobi braucht die Uhr-App.
- **Geld:** Die Kachel heißt „Geld“ und führt zu einer Auswahl „Wallet“ oder „Geldguru“. Das Wallet legt Karten, Ausweise, Tickets und Sonstiges ab (Name, Nummer, Farbe, Fotos, Ablaufdatum), zeigt sie als Stapel und hat eine eigene Datenbank (wallet.db, Fotos in `files/wallet`). Bezahlen per NFC gibt es nicht. Ein echter Strichcode aus der Nummer fehlt; dafür bräuchte es eine neue Bibliothek (Tobi fragen).
- **Geldguru:** Im Buchungs-Editor gibt es den Schalter „Anfangsbestand“. Die Buchung vom 14.03.2026 (939,88 €, „gesperrtes Geld der Eltern“) ist damit umgestellt und zählt nicht mehr als Einnahme. Kontostände unverändert: 1.456,82 € Konto, 2,40 € Bargeld (07.10. abends).
- **Fahrschule:** Klasse B / BF17. Zum Abhaken: 12 + 2 Theorie-Doppelstunden, 12 Sonderfahrten (5 Überland, 4 Autobahn, 3 Dunkelheit), Zähler für Übungsstunden, sechs Schritte (Sehtest, Erste-Hilfe-Kurs, Passfoto, Antrag, beide Prüfungen). Die Kosten kommen aus der Geldguru-Kategorie „Fahrschule“ (07.10.: 456,49 €); „Zahlung eintragen“ schreibt in den Geldguru. Tobi hat noch nichts abgehakt.
- **Nicht am Gerät geprüft:** im Wallet das Bild aus der Galerie, der Code-Scanner und die volle Helligkeit auf der Detailseite.
- **Hinfällig:** Schul-Import mit ChatGPT (T31), Schul-Brain (T33), Anti-Scroll „Pause“ (T30). Die NAS-Ablage je App (T32) ist weiter offen und nie begonnen.
- Werkzeug-Hinweis: `uiautomator dump` lieferte am 07.10. auf dem S9+ nur „null root node“; der Gerätetest lief über Bildschirmfotos und Koordinaten (Anzeige 720×1480).

## 08.10.2026, 13:50 — Version 0.11.0: Training weg, Wallet mit Sperre, NFC, eigenem Design und JSON

- **Live Up besteht jetzt aus:** Übersicht, Essen, Geld (Wallet + Geldguru), Fahrschule, Recorder, Einstellungen. Version 0.11.0 (versionCode 18), per USB installiert.
- **Training entfernt** (Tobi: „Mach mir Training ganz weg“), wie am Vortag als „Modul raus und Daten löschen“ verstanden. liveup.db ist Version 9 und enthält nur noch Essen, Befinden, Gewicht und Produkte. Gelöscht: 3 Trainings, 1 Trainingsplan, 2 GPS-Strecken. Sicherung davor: `A:\LiveUp-backup\2026-10-08_1330_vor-0110\handy\liveup.db`. Standort-, Benachrichtigungs- und Vibrationsrecht sind weg.
- **Wallet-Sperre:** öffnet erst nach Fingerabdruck, ersatzweise PIN oder Muster; schließt, sobald die App in den Hintergrund geht; Bildschirmfotos sind gesperrt. Gesperrt ist die Oberfläche, die Daten sind nicht eigens verschlüsselt. **Das Entsperren selbst ist ungeprüft: Das kann nur Tobi mit seinem Finger.**
- **S9+-Eigenheit:** Der System-Dialog `BiometricPrompt` fragt auf dem S9+ den Iris-Scanner statt des Fingers und landet nach 9 s bei der PIN. Deshalb hört die Sperrseite direkt über `FingerprintManager` auf den Fingerabdruck-Sensor. Am Handy ist ein Finger hinterlegt.
- **NFC-Chips:** Im Karten-Editor „Chip scannen“. Gelesen wird, was der Chip offen hergibt (Kennung, Typ, Hersteller, NTAG-Speicher, NDEF-Inhalt, bei Bankkarten nur der Name der Bezahl-Anwendung). Nicht gelesen: Kartennummern und Umsätze, geschützter Speicher. Es wird nichts geschrieben und nichts nachgespielt. NFC ist am Handy aus (08.10.); am echten Chip ungeprüft. RFID mit 125 kHz kann kein Handy lesen.
- **Eigenes Aussehen und JSON:** Jede Karte hat Farbe, Verlauf, Schriftfarbe, Zeichen und weitere Angaben. „Mit ChatGPT gestalten“ teilt einen Auftrag; die JSON-Antwort legt Karten an, ändert, löscht und gestaltet sie, mit Vorschau. Der Auftrag enthält keine Kartennummern, Notizen, Fotos oder Chip-Daten. Schlüssel dürfen groß oder klein geschrieben sein.
- **Tobis Wallet** enthielt am 08.10. schon vier eigene Karten (Bankkarte, Personalausweis, Mediatheks-Ausweis, Erste-Hilfe-Kurs) mit zehn Fotos. Sie sind unverändert. Eine Kopie davon liegt nicht auf dem PC: Claude hat sie nach der Prüfung wieder aus der Sicherung gelöscht.
- Fahrschule: Tobi hat bis 08.10. drei Theoriestunden abgehakt.
- Werkzeug-Hinweis: `uiautomator dump` geht auf dem S9+ nach einer Neuinstallation oft eine Weile nicht („null root node“); dann über Bildschirmfotos arbeiten. Solange das Wallet angezeigt wird, liefert `screencap` wegen der Sperre eine leere Datei.
- Beschreibung beider Projekte für Außenstehende: [[Beschreibung Live Up und Audio-Recorder]].

## 08.10.2026, 15:00 — Version 0.12.0: Karten wie im Geldbeutel, Strichcode, Kostenschätzung Fahrschule

- **Tobis Wunsch (diktiert):** Die Karten im Wallet sollen aussehen wie im Geldbeutel und auf den ersten Blick erkennbar sein, mit einem eigenen Bild (auch von ChatGPT). Der Stapel soll ohne Fingerabdruck zu sehen sein, Sensibles unscharf; der Fingerabdruck erst bei „sensible Daten anzeigen“.
- **Gebaut:** Scheckkarten-Format, Kartenbild aus Kamera, Galerie oder einem gespeicherten Foto, Ausschnitt verschieben und vergrößern. Geschützte Karten (Vorgabe) zeigen gesperrt ein unscharfes Bild, Name und Schloss, keine Nummer. „Sensible Daten anzeigen“ → Fingerabdruck → alles sichtbar, bis die App in den Hintergrund geht. Je Karte zwei Schalter: „Mit Fingerabdruck schützen“ und „Kartenbild immer scharf zeigen“.
- **Bild von ChatGPT:** Im Wallet unter dem Sternchen gibt es einen Auftrag zum Kopieren (1586 × 1000 Pixel, ohne Name und Nummern). Tobi gibt ChatGPT ein Foto der Karte dazu, speichert das Ergebnis und wählt es unter „Kartenbild“ aus der Galerie.
- **Strichcode:** Die Nummer einer Karte lässt sich als Strichcode zeigen (Code 128, Code 39, Codabar, 2 aus 5, EAN-13, QR). Der Scanner im Editor übernimmt Nummer und Format der echten Karte. Gedacht für den Mediatheks-Ausweis; ob der Scanner dort das Display liest, ist offen. Bibliothek: ZXing core 3.5.4 als Datei unter `A:\LiveUp\app\libs\` (lag schon im Gradle-Speicher, kein Download).
- **Handy als RFID-Karte geht nicht:** Eine App kann keine fremde Karte nachbilden (keine feste Kennung, kein MIFARE/NTAG, kein 125 kHz). So Tobi erklärt.
- **NFC:** Tobi hat NFC am 08.10. eingeschaltet und den Chip seines Personalausweises gescannt.
- **Fahrschule, Kostenschätzung:** offene Posten aus den Haken, Preise einstellbar. Mit üblichen Werten (65 € je Fahrstunde, 25 Übungsstunden, 80 € je Sonderfahrt, Prüfungen samt TÜV) am 08.10.: noch ca. 3.059 €, insgesamt ca. 3.515 €. Die Preise seiner Fahrschule hat Tobi noch nicht eingetragen.
- **Ungeprüft:** Entsperren mit Tobis Finger, Vergrößern mit zwei Fingern, Bild aus der Galerie.
- Bauhinweis: Der PC hat einmal mitten im Bau keinen Arbeitsspeicher mehr gehabt (Gradle-Absturz); einfach neu bauen. `./gradlew.bat --offline` geht.
