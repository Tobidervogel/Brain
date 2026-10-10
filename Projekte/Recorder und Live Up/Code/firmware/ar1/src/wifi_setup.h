#pragma once

#include <Arduino.h>
#include <WebServer.h>

// Commands that are not Wi-Fi provisioning go to the application.
bool appCommand(const String &command);
bool recordingActive();

namespace RecorderWifi {
void begin(const char *deviceId);   // host name becomes recorder-<deviceId>
void service();
String statusJson();
bool authorize(WebServer &server);
String command(const String &line);   // "WIFI ..." of the remote control; "" if the line is none of them
String webLogin();                    // user and password of the web server, as JSON
}
