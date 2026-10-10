#pragma once

#include <Arduino.h>

// Remote control over Bluetooth LE (specification section 3.7). Commands arrive encrypted and
// authenticated with the device key; the answer goes back the same way, cut into indications.
namespace RecorderBle {
void begin(const char *deviceId);   // after the Wi-Fi setup, so the random generator has a radio behind it
void service();                     // main loop: handshake, commands, answers, advertising
bool connected();
String pairingJson();               // address and key for the phone; only ever sent over USB
void newKey();                      // replaces the device key: every phone has to be paired again
bool selfTest();                    // fixed vectors of docs/ar1-proto-vectors.json
}

// Runs one command that came in over Bluetooth and returns its JSON answer (main.cpp).
String bleCommand(const String &line);
