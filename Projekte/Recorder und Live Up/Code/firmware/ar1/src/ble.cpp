#include "ble.h"

#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <Preferences.h>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "mbedtls/gcm.h"
#include "mbedtls/md.h"

namespace RecorderBle {
namespace {
constexpr char SERVICE_UUID[] = "73b90001-76e6-4959-9847-35866b493c85";
constexpr char HELLO_UUID[] = "73b90002-76e6-4959-9847-35866b493c85";
constexpr char RX_UUID[] = "73b90003-76e6-4959-9847-35866b493c85";
constexpr char TX_UUID[] = "73b90004-76e6-4959-9847-35866b493c85";
constexpr uint16_t MTU = 247;
constexpr size_t FRAME_MAX = 220;          // counter 4 + command 200 + tag 16
constexpr size_t OVERHEAD = 4 + 16 + 6;    // counter, tag, answer header (request, part, last)
constexpr uint8_t TO_DEVICE = 1, TO_PHONE = 2;
constexpr uint32_t SILENT_MS = 10000;      // a peer that sends nothing valid for this long is dropped

// The Bluetooth task only checks lengths and queues the bytes; everything else happens in the main loop.
enum class What : uint8_t { Connect, Disconnect, Mtu, Hello, Rx, Bad };
struct Event {
  What what;
  uint16_t value;                          // connection id or MTU
  uint16_t len;
  uint8_t data[FRAME_MAX];
};

QueueHandle_t events = nullptr;
BLEServer *server = nullptr;
BLECharacteristic *hello = nullptr, *tx = nullptr;
char name[20] = "Recorder";
uint8_t key[32];
bool keyReady = false;

// Session, owned by the main loop.
bool linked = false, keyed = false, heard = false;
uint8_t ks[32];
uint32_t rxCtr = 0, txCtr = 0, linkedSinceMs = 0;
uint16_t connId = 0, mtu = 23;
int badFrames = 0;

void push(What what, uint16_t value, const uint8_t *data = nullptr, size_t len = 0) {
  Event e;
  e.what = what;
  e.value = value;
  e.len = min(len, sizeof(e.data));
  if (data) memcpy(e.data, data, e.len);
  xQueueSend(events, &e, 0);
}

// The device nonce the next handshake will use. Only the Bluetooth task touches it.
uint8_t offered[16];

// A fresh nonce for every handshake, readable in HELLO before the phone answers.
void offerNonce() {
  uint8_t value[17] = {1};
  esp_fill_random(offered, sizeof(offered));
  memcpy(value + 1, offered, sizeof(offered));
  hello->setValue(value, sizeof(value));
}

class ServerEvents : public BLEServerCallbacks {
  void onConnect(BLEServer *, esp_ble_gatts_cb_param_t *param) override {
    offerNonce();
    push(What::Connect, param->connect.conn_id);
  }
  void onDisconnect(BLEServer *) override { push(What::Disconnect, 0); }
  void onMtuChanged(BLEServer *, esp_ble_gatts_cb_param_t *param) override { push(What::Mtu, param->mtu.mtu); }
};

class HelloEvents : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *c) override {
    // The event carries both nonces of this handshake: the one that was on offer and the phone's.
    uint8_t nonces[32];
    const bool ok = c->getLength() == 17 && c->getData()[0] == 1;
    if (ok) {
      memcpy(nonces, offered, 16);
      memcpy(nonces + 16, c->getData() + 1, 16);
    }
    // Android reuses a radio link that an app has just closed: the next handshake can arrive on the same
    // connection. It must never see a nonce twice, so a new one is on offer at once.
    offerNonce();
    push(ok ? What::Hello : What::Bad, 0, nonces, ok ? sizeof(nonces) : 0);
  }
};

class RxEvents : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *c) override {
    const size_t len = c->getLength();
    const bool ok = len > 20 && len <= FRAME_MAX;
    push(ok ? What::Rx : What::Bad, 0, c->getData(), ok ? len : 0);
  }
};

void sessionKey(const uint8_t *k, const uint8_t *deviceNonce, const uint8_t *phoneNonce, uint8_t *out) {
  uint8_t input[11 + 32];
  memcpy(input, "AR1-session", 11);
  memcpy(input + 11, deviceNonce, 16);
  memcpy(input + 27, phoneNonce, 16);
  mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), k, 32, input, sizeof(input), out);
}

// frame = counter (4, big endian) | ciphertext | tag (16). Returns the frame length.
size_t seal(const uint8_t *sk, uint8_t direction, uint32_t ctr, const uint8_t *plain, size_t len, uint8_t *frame) {
  uint8_t nonce[12] = {direction};
  frame[0] = ctr >> 24; frame[1] = ctr >> 16; frame[2] = ctr >> 8; frame[3] = ctr;
  memcpy(nonce + 8, frame, 4);
  mbedtls_gcm_context gcm;
  mbedtls_gcm_init(&gcm);
  mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, sk, 256);
  mbedtls_gcm_crypt_and_tag(&gcm, MBEDTLS_GCM_ENCRYPT, len, nonce, sizeof(nonce), nullptr, 0, plain, frame + 4, 16, frame + 4 + len);
  mbedtls_gcm_free(&gcm);
  return 4 + len + 16;
}

// Returns the plaintext length, or -1 for a forged, damaged or replayed frame.
int unseal(const uint8_t *sk, uint8_t direction, const uint8_t *frame, size_t len, uint32_t lastCtr, uint32_t *ctr, uint8_t *plain) {
  if (len < 20) return -1;
  *ctr = static_cast<uint32_t>(frame[0]) << 24 | frame[1] << 16 | frame[2] << 8 | frame[3];
  if (*ctr <= lastCtr) return -1;
  uint8_t nonce[12] = {direction};
  memcpy(nonce + 8, frame, 4);
  mbedtls_gcm_context gcm;
  mbedtls_gcm_init(&gcm);
  mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, sk, 256);
  const int rc = mbedtls_gcm_auth_decrypt(&gcm, len - 20, nonce, sizeof(nonce), nullptr, 0, frame + len - 16, 16, frame + 4, plain);
  mbedtls_gcm_free(&gcm);
  return rc == 0 ? static_cast<int>(len - 20) : -1;
}

void drop() {
  if (linked) server->disconnect(connId);
}

// Set when the phone did not confirm an indication (gone, asleep, not subscribed).
bool unconfirmed = false;

class TxEvents : public BLECharacteristicCallbacks {
  void onStatus(BLECharacteristic *, Status status, uint32_t) override {
    if (status != SUCCESS_INDICATE) unconfirmed = true;
  }
};

// The answer to command number req, as confirmed indications that each fit one packet.
void answer(uint32_t req, const String &json) {
  if (mtu < OVERHEAD + 3 + 16) return drop();
  const size_t part = mtu - 3 - OVERHEAD, total = json.length();
  uint8_t plain[MTU], frame[MTU];
  uint8_t seq = 0;
  for (size_t at = 0; linked && (at < total || !seq); at += part, seq++) {
    const size_t k = min(part, total - at);
    plain[0] = req >> 24; plain[1] = req >> 16; plain[2] = req >> 8; plain[3] = req;
    plain[4] = seq;
    plain[5] = at + k >= total;
    memcpy(plain + 6, json.c_str() + at, k);
    tx->setValue(frame, seal(ks, TO_PHONE, ++txCtr, plain, 6 + k, frame));
    unconfirmed = false;
    tx->indicate();                        // waits for the phone's confirmation, one second at most
    // A phone that does not confirm is gone: the rest of the answer would only hold up the main loop.
    if (unconfirmed || seq == 255) return drop();
  }
}

void handle(const Event &e) {
  switch (e.what) {
    case What::Connect:
      linked = true;
      keyed = heard = false;
      connId = e.value;
      mtu = 23;
      badFrames = 0;
      linkedSinceMs = millis();
      break;
    case What::Disconnect:
      linked = keyed = false;
      memset(ks, 0, sizeof(ks));
      rxCtr = txCtr = 0;
      BLEDevice::startAdvertising();       // the library does not advertise again on its own
      break;
    case What::Mtu:
      mtu = min<uint16_t>(e.value, MTU);
      break;
    case What::Hello:
      if (!linked || !keyReady) return drop();
      // A new handshake replaces the session. Its device nonce is fresh, so the key is too: old frames
      // do not fit any more and nothing is ever encrypted twice with the same key and counter.
      sessionKey(key, e.data, e.data + 16, ks);
      keyed = true;
      rxCtr = txCtr = 0;
      badFrames = 0;
      break;
    case What::Rx: {
      uint8_t plain[FRAME_MAX + 1];
      uint32_t ctr = 0;
      const int len = keyed ? unseal(ks, TO_DEVICE, e.data, e.len, rxCtr, &ctr, plain) : -1;
      if (len < 0 || len > 200) {
        if (++badFrames >= 3) drop();
        return;
      }
      rxCtr = ctr;
      heard = true;
      plain[len] = 0;
      answer(ctr, bleCommand(String(reinterpret_cast<char *>(plain))));
      break;
    }
    case What::Bad:
      if (++badFrames >= 3) drop();
      break;
  }
}

String hex(const uint8_t *bytes, size_t len) {
  static const char digits[] = "0123456789abcdef";
  String out;
  out.reserve(len * 2);
  for (size_t i = 0; i < len; i++) {
    out += digits[bytes[i] >> 4];
    out += digits[bytes[i] & 15];
  }
  return out;
}
}

void newKey() {
  esp_fill_random(key, sizeof(key));       // called with the radio on, so the numbers are truly random
  Preferences prefs;
  prefs.begin("ar1", false);
  keyReady = prefs.putBytes("blekey", key, sizeof(key)) == sizeof(key);
  prefs.end();
  drop();
}

void begin(const char *deviceId) {
  events = xQueueCreate(8, sizeof(Event));
  snprintf(name, sizeof(name), "Recorder-%s", deviceId + 2);
  BLEDevice::init(name);
  BLEDevice::setMTU(MTU);
  Preferences prefs;
  prefs.begin("ar1", false);
  keyReady = prefs.getBytesLength("blekey") == sizeof(key) && prefs.getBytes("blekey", key, sizeof(key)) == sizeof(key);
  prefs.end();
  if (!keyReady) newKey();

  server = BLEDevice::createServer();
  server->setCallbacks(new ServerEvents());
  BLEService *service = server->createService(SERVICE_UUID);
  hello = service->createCharacteristic(HELLO_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  hello->setCallbacks(new HelloEvents());
  BLECharacteristic *rx = service->createCharacteristic(RX_UUID, BLECharacteristic::PROPERTY_WRITE);
  rx->setCallbacks(new RxEvents());
  tx = service->createCharacteristic(TX_UUID, BLECharacteristic::PROPERTY_INDICATE);
  tx->addDescriptor(new BLE2902());        // the phone switches the indications on here
  tx->setCallbacks(new TxEvents());
  service->start();
  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(SERVICE_UUID);
  advertising->setScanResponse(true);
  advertising->setMinInterval(0x1E0);      // 300 to 500 ms: found within a second or two, little power
  advertising->setMaxInterval(0x320);
  BLEDevice::startAdvertising();
}

void service() {
  Event e;
  while (xQueueReceive(events, &e, 0)) handle(e);
  // The event queue can overflow while a long command runs. A lost Disconnect would leave the recorder
  // unreachable (no advertising), a lost Connect would leave a peer without a nonce: the stack knows the truth.
  // The stack updates its count a moment before or after it calls us, so only a mismatch that lasts counts.
  static uint32_t mismatchSince = 0;
  const bool peer = server->getConnectedCount() > 0;
  if (linked == peer || uxQueueMessagesWaiting(events)) {
    mismatchSince = 0;
  } else if (!mismatchSince) {
    mismatchSince = millis() | 1;
  } else if (millis() - mismatchSince > 2000) {
    mismatchSince = 0;
    if (linked) {
      e.what = What::Disconnect;
      handle(e);
    } else {
      server->disconnect(server->getConnId());
    }
  }
  if (linked && !heard && millis() - linkedSinceMs > SILENT_MS) {
    linkedSinceMs = millis();
    drop();
  }
}

bool connected() { return linked && keyed; }

String pairingJson() {
  String address = BLEDevice::getAddress().toString().c_str();
  address.toUpperCase();                   // Android only accepts upper-case addresses
  return "{\"v\":1,\"addr\":\"" + address + "\",\"key\":\"" + hex(key, sizeof(key)) + "\",\"name\":\"" + name + "\"}";
}

bool selfTest() {
  uint8_t k[32], deviceNonce[16], phoneNonce[16], sk[32], frame[64], plain[32];
  for (int i = 0; i < 32; i++) k[i] = i;
  for (int i = 0; i < 16; i++) deviceNonce[i] = 0x40 + i, phoneNonce[i] = 0x80 + i;
  sessionKey(k, deviceNonce, phoneNonce, sk);
  const size_t len = seal(sk, TO_DEVICE, 1, reinterpret_cast<const uint8_t *>("STATUS"), 6, frame);
  uint32_t ctr = 0;
  const bool opens = unseal(sk, TO_DEVICE, frame, len, 0, &ctr, plain) == 6 && ctr == 1 && !memcmp(plain, "STATUS", 6);
  const bool replay = unseal(sk, TO_DEVICE, frame, len, 1, &ctr, plain) < 0;
  const bool reflected = unseal(sk, TO_PHONE, frame, len, 0, &ctr, plain) < 0;
  frame[len - 1] ^= 1;
  const bool forged = unseal(sk, TO_DEVICE, frame, len, 0, &ctr, plain) < 0;
  frame[len - 1] ^= 1;
  return hex(sk, 32) == "1e7ee98af7b2cd00c20ee5f27f307087781ea72836ea9250098ef29a365784b5" &&
         hex(frame, len) == "0000000166a54fc1b2bced526ce6d9e85749aba977901bf2418c" && opens && replay && reflected && forged;
}
}
