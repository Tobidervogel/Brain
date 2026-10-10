# Recorder und Live Up

Lesbarer Auszug aus dem Brain, Stand 10.10.2026. Der Rest des Repos (`changes/`, `brain.zip`) ist die verschlüsselte Brain-Sicherung und bleibt unberührt.

## Notizen

- [Audio-Recorder](Notizen/Audio-Recorder.md): das Projekt, Entscheidungen, Verlauf (Seeed XIAO ESP32S3 Sense)
- [Recorder Hardware Kette](Notizen/Recorder%20Hardware%20Kette.md): Ladeplatine Adafruit bq24074, LiPo, Schaltplan, Lötplan, Fehlersuche
- [Recorder AR1 Firmware](Notizen/Recorder%20AR1%20Firmware.md): Nur-Audio-Firmware mit Bluetooth und Strommessung
- [Recorder DV2 Lauftest](Notizen/Recorder%20DV2%20Lauftest.md): frühere Firmware mit Video und Audio
- [Recorder Feldaufnahmen Auswertung](Notizen/Recorder%20Feldaufnahmen%20Auswertung.md) und [Transkript 0021-0022](Notizen/Recorder%20Testaufnahmen%200021-0022.md)
- [Recorder Omi Anbindung](Notizen/Recorder%20Omi%20Anbindung.md): Verbindung zu Omi (BasedHardware)
- [Live Up](Notizen/Live%20Up.md), [Live improve](Notizen/Live%20improve.md), [Beschreibung Live Up und Audio-Recorder](Notizen/Beschreibung%20Live%20Up%20und%20Audio-Recorder.md)

![Schaltplan](Notizen/Recorder%20Schaltplan.svg)

## Code

- `Code/firmware/ar1`: aktuelle Firmware (PlatformIO), `tools/` für USB, Tests, Koppeln
- `Code/firmware/dv2`: Rückfallstand mit Kamera
- `Code/pc`: Empfang, Transkription (faster-whisper), Ablage im Brain, optional Omi
- `Code/docs`: Spezifikation `Audio-Recorder-V1.md`, Testvektoren, Steckbrett-Zeichnung

Nicht enthalten: Aufnahmen, Stimmproben, Modelle, Gerätetoken, die Live-Up-App selbst.

## Chats

Die Gesprächsprotokolle von Claude und Codex, in denen es um Recorder, Lader oder Live Up geht.
