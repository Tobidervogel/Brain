#pragma once

#include <Arduino.h>
#include <WebServer.h>

// Set by any USB command or web action: the recorder then stays awake after a recording.
extern bool serviceMode;
// Commands that are not Wi-Fi provisioning go to the application.
bool appCommand(const String &command);
bool recordingActive();

namespace RecorderWifi {
void begin();
void service();
String statusJson();
bool authorize(WebServer &server);
}
