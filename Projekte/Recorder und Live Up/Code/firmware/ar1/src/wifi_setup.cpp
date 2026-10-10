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

// Slot 0 keeps the key and layout of DV1.4, so older firmware still finds its network.
// Slot 1 was added for the phone's hotspot: home network and hotspot stay stored side by side.
constexpr int SLOTS = 2;
const char *const SLOT_KEY[SLOTS] = {"credentials", "credentials2"};

Preferences preferences;
Credentials saved[SLOTS] = {};
Credentials candidate = {};
bool storageReady = false;
bool hasSlot[SLOTS] = {};
int current = 0;            // the slot the radio is trying or using
bool radioOff = false;      // WIFI OFF, kept across restarts
bool joining = false;
bool provisioning = false;
uint32_t joinStarted = 0;
uint32_t lastRetry = 0;
String webPassword;
String state = "unconfigured";
String input;
char hostname[24] = "recorder";
bool inputOverflow = false;
constexpr uint32_t JOIN_TIMEOUT_MS = 30000;

bool anySaved() { return hasSlot[0] || hasSlot[1]; }

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
  const String line = "DV1_WIFI_STATUS " + statusJson() + "\r\n";
  Serial.write(reinterpret_cast<const uint8_t *>(line.c_str()), line.length());
}

void startJoin(const Credentials &credentials, bool isProvisioning) {
  WiFi.disconnect(false, false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(credentials.ssid, credentials.password);
  joining = true;
  provisioning = isProvisioning;
  joinStarted = millis();
  lastRetry = joinStarted;
  state = "connecting";
  emitStatus();
}

// Reads a candidate from "<ssid_hex> <password_hex>" (the password may be empty for an open network).
bool parseCandidate(const String &arguments) {
  const int split = arguments.indexOf(' ');
  clearCredentials(candidate);
  bool valid = decodeHex(split < 0 ? arguments : arguments.substring(0, split), candidate.ssid, sizeof(candidate.ssid)) &&
               (split < 0 || decodeHex(arguments.substring(split + 1), candidate.password, sizeof(candidate.password)));
  const size_t passwordLength = strlen(candidate.password);
  valid = valid && candidate.ssid[0] && (passwordLength == 0 || passwordLength >= 8);
  if (!valid) clearCredentials(candidate);
  return valid;
}

bool store(int slot) {
  if (!storageReady || preferences.putBytes(SLOT_KEY[slot], &candidate, sizeof(candidate)) != sizeof(candidate)) return false;
  saved[slot] = candidate;
  hasSlot[slot] = true;
  clearCredentials(candidate);
  return true;
}

// The slot that already holds this network, else a free one, else the second: the first stays the home network.
int slotFor(const char *ssid) {
  for (int i = 0; i < SLOTS; i++) {
    if (hasSlot[i] && !strcmp(saved[i].ssid, ssid)) return i;
  }
  return hasSlot[0] ? 1 : 0;
}

// Found networks as (ssid, rssi), strongest first, each name once. The radio is offline for the scan.
int scan() {
  joining = false;
  WiFi.setAutoReconnect(false);   // the radio refuses to scan while it is still trying to connect
  WiFi.disconnect(false, false);
  WiFi.mode(WIFI_STA);
  delay(500);
  const int count = WiFi.scanNetworks(false, false);
  WiFi.setAutoReconnect(true);
  lastRetry = millis();
  return count;
}

void afterScan() {
  WiFi.scanDelete();
  if ((!anySaved() || radioOff) && WiFi.status() != WL_CONNECTED) WiFi.mode(WIFI_OFF);
}

void handleCommand(const String &command) {
  if (command == "WIFI_STATUS") {
    emitStatus();
  } else if (command == "WIFI_SCAN") {
    if ((joining && provisioning) || recordingActive()) {   // the radio is offline while it scans
      Serial.println("DV1_WIFI_ERROR busy");
      return;
    }
    // A saved network that is out of range keeps the radio joining; setup must still be possible.
    const int count = scan();
    for (int i = 0; i < count; ++i) {
      Serial.printf("DV1_WIFI_SCAN %s %d\n", encodeHex(WiFi.SSID(i)).c_str(), WiFi.RSSI(i));
    }
    afterScan();
    Serial.println("DV1_WIFI_SCAN_DONE");
  } else if (command.startsWith("WIFI_CONFIG ") || command.startsWith("WIFI_STORE ")) {
    // WIFI_CONFIG saves only after a successful connection. WIFI_STORE saves at once,
    // for a network that is not in range while the recorder is being set up.
    const bool storeNow = command[5] == 'S';
    if (joining && provisioning) {
      Serial.println("DV1_WIFI_ERROR busy");
      return;
    }
    if (!parseCandidate(command.substring(command.indexOf(' ') + 1))) {
      Serial.println("DV1_WIFI_ERROR invalid_config");
      return;
    }
    radioOff = false;
    if (!storeNow) {
      startJoin(candidate, true);
    } else if (store(0)) {
      current = 0;
      Serial.println("DV1_WIFI_STORED");
      startJoin(saved[0], false);
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
  // After a scan the radio is offline until the next join; do not keep reporting the old state.
  String result = "{\"state\":\"" + (state == "connected" && !connected ? String("disconnected") : state) + "\",\"connected\":";
  result += connected ? "true" : "false";
  result += ",\"saved\":";
  result += anySaved() ? "true" : "false";
  result += ",\"off\":";
  result += radioOff ? "true" : "false";
  result += ",\"ssid_hex\":\"" + (connected ? encodeHex(WiFi.SSID()) : String("")) + "\",\"nets\":[";
  for (int i = 0, listed = 0; i < SLOTS; i++) {
    if (hasSlot[i]) result += String(listed++ ? "," : "") + "\"" + encodeHex(saved[i].ssid) + "\"";
  }
  result += "],\"ip\":\"" + (connected ? WiFi.localIP().toString() : String("")) + "\"";
  result += ",\"gateway\":\"" + (connected ? WiFi.gatewayIP().toString() : String("")) + "\"";
  result += ",\"rssi\":" + String(connected ? WiFi.RSSI() : 0) + "}";
  return result;
}

// The WIFI commands of the remote control (Bluetooth and USB). Empty if the line is none of them.
String command(const String &c) {
  static const char *const busy = "{\"ok\":false,\"error\":\"busy\"}";
  if (!c.startsWith("WIFI ")) return "";
  if (c.startsWith("WIFI SET ")) {
    if (joining && provisioning) return busy;
    if (!parseCandidate(c.substring(9))) return "{\"ok\":false,\"error\":\"invalid_config\"}";
    const int slot = slotFor(candidate.ssid);
    if (!store(slot)) {
      clearCredentials(candidate);
      return "{\"ok\":false,\"error\":\"store_failed\"}";
    }
    current = slot;
    radioOff = false;
    if (storageReady) preferences.putBool("off", false);
    startJoin(saved[slot], false);
  } else if (c.startsWith("WIFI FORGET ")) {
    char ssid[33];
    if (!decodeHex(c.substring(12), ssid, sizeof(ssid))) return "{\"ok\":false,\"error\":\"invalid_config\"}";
    bool found = false;
    for (int i = 0; i < SLOTS; i++) {
      if (!hasSlot[i] || strcmp(saved[i].ssid, ssid)) continue;
      found = true;
      hasSlot[i] = false;
      clearCredentials(saved[i]);
      if (storageReady) preferences.remove(SLOT_KEY[i]);
      if (i == current) {
        joining = false;
        WiFi.disconnect(false, true);
        state = "unconfigured";
        current = i ^ 1;
        lastRetry = millis() - JOIN_TIMEOUT_MS;   // try the other network right away
      }
    }
    if (!found) return "{\"ok\":false,\"error\":\"unknown_network\"}";
    if (!anySaved()) WiFi.mode(WIFI_OFF);
  } else if (c == "WIFI OFF" || c == "WIFI ON") {
    radioOff = c.endsWith("OFF");
    if (storageReady) preferences.putBool("off", radioOff);
    joining = false;
    if (radioOff) {
      WiFi.disconnect(true, false);
      WiFi.mode(WIFI_OFF);
      state = "off";
    } else if (anySaved()) {
      if (!hasSlot[current]) current ^= 1;
      startJoin(saved[current], false);
    } else {
      state = "unconfigured";
    }
  } else if (c == "WIFI SCAN") {
    if ((joining && provisioning) || recordingActive()) return busy;
    const int count = scan();
    String out = "{\"ok\":true,\"nets\":[";
    int listed = 0;
    for (int i = 0; i < count && listed < 15; ++i) {    // sorted by strength; a name counts once
      bool seen = WiFi.SSID(i).length() == 0;
      for (int j = 0; j < i && !seen; ++j) seen = WiFi.SSID(j) == WiFi.SSID(i);
      if (seen) continue;
      out += String(listed++ ? "," : "") + "{\"ssid_hex\":\"" + encodeHex(WiFi.SSID(i)) + "\",\"rssi\":" + WiFi.RSSI(i) + "}";
    }
    afterScan();
    return out + "]}";
  } else {
    return "{\"ok\":false,\"error\":\"unknown_command\"}";
  }
  return "{\"ok\":true}";
}

String webLogin() {
  return webPassword.length() == 32 ? "{\"ok\":true,\"user\":\"tobi\",\"password\":\"" + webPassword + "\"}"
                                    : String("{\"ok\":false,\"error\":\"auth_unavailable\"}");
}

bool authorize(WebServer &server) {
  if (webPassword.length() == 32 && server.authenticate("tobi", webPassword.c_str())) return true;
  server.requestAuthentication(DIGEST_AUTH, "Recorder DV1", "Anmeldung erforderlich");
  return false;
}

void begin(const char *deviceId) {
  snprintf(hostname, sizeof(hostname), "recorder-%s", deviceId);
  WiFi.setHostname(hostname);   // only taken over when the Wi-Fi mode changes, so before the first WiFi.mode()
  input.reserve(256);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  storageReady = preferences.begin("dv1wifi", false);
  if (storageReady) {
    for (int i = 0; i < SLOTS; i++) {
      if (preferences.isKey(SLOT_KEY[i]) && preferences.getBytesLength(SLOT_KEY[i]) == sizeof(Credentials)) {
        preferences.getBytes(SLOT_KEY[i], &saved[i], sizeof(Credentials));
        hasSlot[i] = saved[i].ssid[0] && saved[i].ssid[32] == 0 && saved[i].password[64] == 0;
      }
    }
    webPassword = preferences.getString("webpassword", "");
    radioOff = preferences.getBool("off", false);
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
  if (!hasSlot[current]) current ^= 1;
  if (radioOff) {
    WiFi.mode(WIFI_OFF);
    state = "off";
    emitStatus();
  } else if (anySaved()) {
    startJoin(saved[current], false);
  } else {
    WiFi.mode(WIFI_OFF);
    emitStatus();
  }
}

void service() {
  while (Serial.available()) {
    const char c = Serial.read();
    if (c == '\n') {
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
  const char *expectedSsid = provisioning ? candidate.ssid : saved[current].ssid;
  if (joining && WiFi.status() == WL_CONNECTED && now - joinStarted >= 500 && WiFi.SSID() == expectedSsid) {
    joining = false;
    state = "connected";
    if (provisioning) {
      if (store(0)) current = 0;
      else state = "connected_unsaved";
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
    if (!wasProvisioning && hasSlot[current ^ 1]) current ^= 1;   // next time the other stored network
    if (wasProvisioning && anySaved()) startJoin(saved[current], false);
    else if (!anySaved()) WiFi.mode(WIFI_OFF);
  } else if (!joining && !radioOff && anySaved() && WiFi.status() != WL_CONNECTED && now - lastRetry >= JOIN_TIMEOUT_MS) {
    if (!hasSlot[current]) current ^= 1;
    startJoin(saved[current], false);
  }
}
}
