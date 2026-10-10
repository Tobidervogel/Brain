#include "wifi_setup.h"

#include <Preferences.h>
#include <WiFi.h>
#include "esp_system.h"

namespace RecorderWifi {
namespace {
struct Credentials {
  char ssid[33];
  char password[65];
};

Preferences preferences;
Credentials saved = {};
Credentials candidate = {};
bool storageReady = false;
bool hasSaved = false;
bool joining = false;
bool provisioning = false;
uint32_t joinStarted = 0;
uint32_t lastRetry = 0;
String webPassword;
String state = "unconfigured";
String input;
bool inputOverflow = false;
constexpr uint32_t JOIN_TIMEOUT_MS = 30000;

void clearCredentials(Credentials &credentials) {
  volatile char *data = reinterpret_cast<volatile char *>(&credentials);
  for (size_t i = 0; i < sizeof(credentials); ++i) data[i] = 0;
}

int nibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

bool decodeHex(const String &encoded, char *destination, size_t capacity) {
  if (encoded.length() % 2 || encoded.length() / 2 >= capacity) return false;
  for (size_t i = 0; i < encoded.length(); i += 2) {
    int hi = nibble(encoded[i]);
    int lo = nibble(encoded[i + 1]);
    if (hi < 0 || lo < 0 || (hi == 0 && lo == 0)) return false;
    destination[i / 2] = static_cast<char>((hi << 4) | lo);
  }
  destination[encoded.length() / 2] = 0;
  return true;
}

String encodeHex(const String &value) {
  static const char digits[] = "0123456789abcdef";
  String encoded;
  encoded.reserve(value.length() * 2);
  for (size_t i = 0; i < value.length(); ++i) {
    uint8_t c = static_cast<uint8_t>(value[i]);
    encoded += digits[c >> 4];
    encoded += digits[c & 15];
  }
  return encoded;
}

void emitStatus() {
  Serial.print("DV1_WIFI_STATUS ");
  Serial.println(statusJson());
}

void startJoin(const Credentials &credentials, bool isProvisioning) {
  WiFi.disconnect(false, false);
  WiFi.mode(WIFI_STA);
  WiFi.setHostname("recorder-dv1");
  WiFi.begin(credentials.ssid, credentials.password);
  joining = true;
  provisioning = isProvisioning;
  joinStarted = millis();
  lastRetry = joinStarted;
  state = "connecting";
  emitStatus();
}

void handleCommand(const String &command) {
  if (command == "WIFI_STATUS") {
    emitStatus();
  } else if (command == "WIFI_SCAN") {
    if ((joining && provisioning) || recordingActive()) {   // a scan would stall a recording for seconds
      Serial.println("DV1_WIFI_ERROR busy");
      return;
    }
    // A saved network that is out of range keeps the radio joining; setup must still be possible.
    joining = false;
    WiFi.setAutoReconnect(false);   // the radio refuses to scan while it is still trying to connect
    WiFi.disconnect(false, false);
    WiFi.mode(WIFI_STA);
    delay(500);
    const int count = WiFi.scanNetworks(false, false);
    WiFi.setAutoReconnect(true);
    lastRetry = millis();
    for (int i = 0; i < count; ++i) {
      Serial.printf("DV1_WIFI_SCAN %s %d\n", encodeHex(WiFi.SSID(i)).c_str(), WiFi.RSSI(i));
    }
    WiFi.scanDelete();
    Serial.println("DV1_WIFI_SCAN_DONE");
    if (!hasSaved && WiFi.status() != WL_CONNECTED) WiFi.mode(WIFI_OFF);
  } else if (command.startsWith("WIFI_CONFIG ") || command.startsWith("WIFI_STORE ")) {
    // WIFI_CONFIG saves only after a successful connection. WIFI_STORE saves at once,
    // for a network that is not in range while the recorder is being set up.
    const bool storeNow = command[5] == 'S';
    if (joining && provisioning) {
      Serial.println("DV1_WIFI_ERROR busy");
      return;
    }
    const int start = command.indexOf(' ') + 1;
    const int split = command.indexOf(' ', start);
    clearCredentials(candidate);
    bool valid = split > start &&
        decodeHex(command.substring(start, split), candidate.ssid, sizeof(candidate.ssid)) &&
        decodeHex(command.substring(split + 1), candidate.password, sizeof(candidate.password));
    const size_t passwordLength = strlen(candidate.password);
    valid = valid && candidate.ssid[0] && (passwordLength == 0 || passwordLength >= 8);
    if (!valid) {
      clearCredentials(candidate);
      Serial.println("DV1_WIFI_ERROR invalid_config");
      return;
    }
    if (!storeNow) {
      startJoin(candidate, true);
    } else if (storageReady && preferences.putBytes("credentials", &candidate, sizeof(candidate)) == sizeof(candidate)) {
      saved = candidate;
      hasSaved = true;
      clearCredentials(candidate);
      Serial.println("DV1_WIFI_STORED");
      startJoin(saved, false);
    } else {
      clearCredentials(candidate);
      Serial.println("DV1_WIFI_ERROR store_failed");
    }
  } else if (command == "WIFI_AUTH") {
    if (webPassword.length() != 32) {
      Serial.println("DV1_WIFI_ERROR auth_unavailable");
      return;
    }
    // Only the physically attached USB provisioning window requests this reply.
    Serial.print("DV1_WIFI_AUTH tobi ");
    Serial.println(webPassword);
  } else if (!appCommand(command)) {
    Serial.println("DV1_WIFI_ERROR unknown_command");
  }
}
}

String statusJson() {
  const bool connected = WiFi.status() == WL_CONNECTED && !joining;
  String result = "{\"state\":\"" + state + "\",\"connected\":";
  result += connected ? "true" : "false";
  result += ",\"saved\":";
  result += hasSaved ? "true" : "false";
  result += ",\"ip\":\"" + (connected ? WiFi.localIP().toString() : String("")) + "\"";
  result += ",\"gateway\":\"" + (connected ? WiFi.gatewayIP().toString() : String("")) + "\"";
  result += ",\"rssi\":" + String(connected ? WiFi.RSSI() : 0) + "}";
  return result;
}

bool authorize(WebServer &server) {
  if (webPassword.length() == 32 && server.authenticate("tobi", webPassword.c_str())) return true;
  server.requestAuthentication(DIGEST_AUTH, "Recorder DV1", "Anmeldung erforderlich");
  return false;
}

void begin() {
  input.reserve(256);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  storageReady = preferences.begin("dv1wifi", false);
  if (storageReady) {
    if (preferences.isKey("credentials") && preferences.getBytesLength("credentials") == sizeof(saved)) {
      preferences.getBytes("credentials", &saved, sizeof(saved));
      hasSaved = saved.ssid[0] && saved.ssid[32] == 0 && saved.password[64] == 0;
    }
    webPassword = preferences.getString("webpassword", "");
  }
  if (webPassword.length() != 32) {
    WiFi.mode(WIFI_STA);
    uint8_t bytes[16];
    esp_fill_random(bytes, sizeof(bytes));
    static const char digits[] = "0123456789abcdef";
    webPassword = "";
    for (uint8_t byte : bytes) {
      webPassword += digits[byte >> 4];
      webPassword += digits[byte & 15];
    }
    if (storageReady) preferences.putString("webpassword", webPassword);
  }
  if (hasSaved) startJoin(saved, false);
  else {
    WiFi.mode(WIFI_OFF);
    emitStatus();
  }
}

void service() {
  while (Serial.available()) {
    const char c = Serial.read();
    if (c == '\n') {
      serviceMode = true;
      if (inputOverflow) Serial.println("DV1_WIFI_ERROR line_too_long");
      else handleCommand(input);
      for (size_t i = 0; i < input.length(); ++i) input.setCharAt(i, 0);
      input = "";
      inputOverflow = false;
    } else if (c != '\r' && !inputOverflow) {
      if (input.length() < 256) input += c;
      else inputOverflow = true;
    }
  }

  const uint32_t now = millis();
  const char *expectedSsid = provisioning ? candidate.ssid : saved.ssid;
  if (joining && WiFi.status() == WL_CONNECTED && now - joinStarted >= 500 && WiFi.SSID() == expectedSsid) {
    joining = false;
    state = "connected";
    if (provisioning) {
      const bool stored = storageReady &&
          preferences.putBytes("credentials", &candidate, sizeof(candidate)) == sizeof(candidate);
      if (stored) {
        saved = candidate;
        hasSaved = true;
      } else state = "connected_unsaved";
      clearCredentials(candidate);
    }
    provisioning = false;
    emitStatus();
  } else if (joining && now - joinStarted >= JOIN_TIMEOUT_MS) {
    joining = false;
    lastRetry = now;   // pause before the next attempt instead of joining back to back
    const bool wasProvisioning = provisioning;
    provisioning = false;
    clearCredentials(candidate);
    state = "connection_failed";
    Serial.println("DV1_WIFI_ERROR connection_failed");
    emitStatus();
    if (wasProvisioning && hasSaved) startJoin(saved, false);
    else if (!hasSaved) WiFi.mode(WIFI_OFF);
  } else if (!joining && hasSaved && WiFi.status() != WL_CONNECTED && now - lastRetry >= JOIN_TIMEOUT_MS) {
    startJoin(saved, false);
  }
}
}
