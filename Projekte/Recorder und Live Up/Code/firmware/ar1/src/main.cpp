// Recorder AR1: audio only. PDM microphone -> PSRAM ring -> 10-minute WAV segments on the microSD.
// Specification: A:/Recorder/docs/Audio-Recorder-V1.md
#include <Arduino.h>
#include <FS.h>
#include <Preferences.h>
#include <SD.h>
#include <SPI.h>
#include <WebServer.h>
#include <WiFi.h>
#include <climits>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include "driver/i2s.h"
#include "esp_heap_caps.h"
#include "esp_mac.h"
#include "esp_rom_crc.h"
#include "esp_sleep.h"
#include "esp_sntp.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/message_buffer.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/stream_buffer.h"
#include "mbedtls/md.h"
#include "ble.h"
#include "wifi_setup.h"

#define FW_VERSION "AR1.2"
#define MOUNT "/sd"

// ---- knobs ----
static constexpr uint32_t SAMPLE_RATE = 16000;
static constexpr uint32_t SEGMENT_SECONDS = 600;
static constexpr int AUDIO_GAIN_SHIFT = 2;              // x4, saturating
static constexpr uint32_t SYNC_MS = 2000;               // at most this much audio is lost if power is cut
static constexpr uint32_t STAT_MS = 60000;              // one STAT line in the journal per minute
static constexpr uint32_t SD_SPI_HZ = 25000000;
static constexpr size_t AUDIO_RING_BYTES = 512 * 1024;  // 16 s between microphone and card
static constexpr size_t WAV_HEADER = 512;               // pads the header so sample data starts sector-aligned
static constexpr uint32_t MIN_FREE_MB = 64;             // below this no new segment is started
static constexpr uint32_t MAX_SEGMENTS = 8000;          // ponytail: 55 days per recording, then a new one starts
static constexpr int MAX_START_FAILURES = 5;
static constexpr int64_t MIC_STALL_US = 5000000;        // no samples for this long: restart
static constexpr time_t VALID_TIME = 1700000000;
static constexpr char TIME_ZONE[] = "CET-1CEST,M3.5.0,M10.5.0/3";

SET_LOOP_TASK_STACK_SIZE(16 * 1024);

// Collects writes into whole blocks so the card only ever sees sector-aligned multi-sector writes.
struct BufFile {
  int fd = -1;
  uint8_t *buf = nullptr;
  size_t cap = 0, n = 0;
  uint32_t maxWriteMs = 0;
  bool err = false;

  bool open(const char *path, size_t capacity) {
    if (!buf) {
      // PSRAM: Bluetooth and Wi-Fi need the internal RAM, and the card driver does not use DMA.
      buf = static_cast<uint8_t *>(heap_caps_malloc(capacity, MALLOC_CAP_SPIRAM));
      cap = capacity;
    }
    n = 0;
    maxWriteMs = 0;
    err = false;
    fd = buf ? ::open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666) : -1;
    return fd >= 0;
  }
  void put(size_t k) {
    if (!k) return;
    uint32_t t = millis();
    if (!err && ::write(fd, buf, k) != static_cast<ssize_t>(k)) err = true;
    t = millis() - t;
    if (t > maxWriteMs) maxWriteMs = t;
    n -= k;
    memmove(buf, buf + k, n);
  }
  void write(const void *data, size_t len) {
    const uint8_t *s = static_cast<const uint8_t *>(data);
    while (len) {
      const size_t k = min(len, cap - n);
      memcpy(buf + n, s, k);
      n += k;
      s += k;
      len -= k;
      if (n == cap) put(cap);
    }
  }
  // Makes everything written so far survive a power cut. Keeps a partial sector in RAM unless all=true.
  void sync(bool all) {
    put(all ? n : n & ~static_cast<size_t>(511));
    if (!err && ::fsync(fd) != 0) err = true;
  }
  void finish(const uint8_t *header, size_t headerLen) {
    sync(true);
    if (::lseek(fd, 0, SEEK_SET) != 0 || ::write(fd, header, headerLen) != static_cast<ssize_t>(headerLen)) err = true;
    if (::fsync(fd) != 0) err = true;
    if (::close(fd) != 0) err = true;
    fd = -1;
  }
  void abandon() {
    if (fd >= 0) ::close(fd);
    fd = -1;
  }
};

enum class State : uint8_t { Idle, Recording, Paused, Stopping };
enum class Cmd : uint8_t { Start, Stop, Pause, Resume };
struct Request {
  Cmd cmd;
  bool keepWant;      // do not touch the remembered wish to record (reboot, internal restart)
  char reason[16];
};

// What the recorder task publishes for USB, BLE and web. Copied as a whole under pubMux.
struct Shared {
  State state = State::Idle;
  bool card = false, want = false, finalized = false;
  uint32_t recN = 0, seg = 0, dropped = 0, ringMax = 0, writeMaxMs = 0;
  uint32_t totalMb = 0, freeMb = 0, sdErrors = 0, segLen = SEGMENT_SECONDS;
  uint64_t samples = 0;
  int64_t t0Us = 0;
  char recId[28] = "", stopReason[16] = "";
};
static Shared pub;
static portMUX_TYPE pubMux = portMUX_INITIALIZER_UNLOCKED;

struct ErrorLog {
  uint32_t count = 0;
  char last[10][48] = {};
};
static ErrorLog errors;

static QueueHandle_t requests = nullptr;
static SemaphoreHandle_t cardMutex = nullptr;   // card commands of the main loop against mounting and finalizing
static volatile bool cardWanted = false;        // the main loop waits for the card: finalizing steps aside
static WebServer server(80);
static bool serverRunning = false, micReady = false;
static uint32_t bootCount = 0;
static char deviceId[8] = "";
static bool autostart = true;
static constexpr size_t IO_BLOCK = 16384;
static uint8_t *ioBlock = nullptr;              // file transfers to the PC, never during a recording (PSRAM)

// Test switches (USB only, lost at the next restart).
static volatile uint32_t segLenSeconds = SEGMENT_SECONDS, minFreeMb = MIN_FREE_MB;
static volatile bool faultSd = false;

static char timeSrc[8] = "none";
static volatile uint32_t timeEpoch = 0;         // counts every time the clock is set (NTP, phone, PC)

static char timerAction[10] = "";
static uint32_t timerSetMs = 0, timerDelayMs = 0;
static volatile uint8_t sleepMode = 0;          // 1 = sleep at once, 2 = shutdown: finish the recordings first, 3 = reboot
static uint32_t sleepSinceMs = 0, wakeAfterS = 0;

static StaticStreamBuffer_t audioRingState;
static StreamBufferHandle_t audioRing = nullptr;
static volatile bool captureWanted = false, capturing = false;
static volatile uint32_t capSamples = 0, capDropped = 0, capGapMaxMs = 0, capPeak = 0, micLevel = 0;
static volatile int64_t capT0Us = 0, capLastUs = 0;

// ---- power board (Adafruit bq24074 on the solder pads; wiring: Brain note "Recorder Hardware Kette") ----

static constexpr int PIN_BAT = 1;         // D0: battery plus through two equal resistors, so half the voltage
static constexpr int PIN_PGOOD = 4;       // D3: low while the charger board has external power
static constexpr int PIN_CHG = 5;         // D4: low while the battery is charging
static constexpr float BAT_SCALE = 2.0f;  // tuning knob: voltage at the battery (multimeter) / voltage at D0
static constexpr int BAT_MIN_MV = 2500;   // below this nothing is connected to D0
static volatile int batMv = 0;
static volatile bool extPower = false, charging = false;
static uint32_t pgoodLowTicks = 0, chgLowTicks = 0, pinTicks = 0;   // main loop only, for the bench command PINS

// Rough charge from the resting voltage of a LiPo cell. Reads too high while charging.
static int batPercent(int mv) {
  static const int curve[][2] = {{3300, 0}, {3600, 5}, {3700, 15}, {3750, 30}, {3800, 45},
                                 {3900, 65}, {4000, 80}, {4100, 92}, {4200, 100}};
  if (mv <= curve[0][0]) return 0;
  for (size_t i = 1; i < sizeof(curve) / sizeof(curve[0]); i++) {
    if (mv < curve[i][0]) {
      return curve[i - 1][1] + (curve[i][1] - curve[i - 1][1]) * (mv - curve[i - 1][0]) / (curve[i][0] - curve[i - 1][0]);
    }
  }
  return 100;
}

static const char *powerName() {
  if (charging) return "charging";
  if (extPower) return "external";    // on the charger, battery full or missing
  return batMv >= BAT_MIN_MV ? "battery" : "unknown";
}

static void readPower() {
  static uint32_t lastMs = 0;
  pgoodLowTicks += !digitalRead(PIN_PGOOD), chgLowTicks += !digitalRead(PIN_CHG), pinTicks++;
  if (lastMs && millis() - lastMs < 2000) return;
  lastMs = millis();
  uint32_t sum = 0;
  for (int i = 0; i < 16; i++) sum += analogReadMilliVolts(PIN_BAT);
  batMv = static_cast<int>(sum / 16 * BAT_SCALE);
  extPower = !digitalRead(PIN_PGOOD);
  charging = !digitalRead(PIN_CHG);
}

// Only the main loop writes to the PC. Lines from the recorder task wait in a small buffer until the loop
// prints them (printEcho) and are dropped when it is full: two tasks writing at once lost whole answers,
// and without a PC listening every journal line would hold up the recorder.
extern TaskHandle_t loopTaskHandle;
static MessageBufferHandle_t echo = nullptr;
static constexpr size_t ECHO_LINE = 420;

static void usbLine(const char *prefix, const char *text) {
  String out;
  out.reserve(strlen(prefix) + strlen(text) + 2);
  out += prefix;
  out += text;
  out += "\r\n";
  if (xTaskGetCurrentTaskHandle() == loopTaskHandle) Serial.write(reinterpret_cast<const uint8_t *>(out.c_str()), out.length());
  else if (echo && out.length() <= ECHO_LINE) xMessageBufferSend(echo, out.c_str(), out.length(), 0);
}

static void printEcho() {
  static char line[ECHO_LINE];
  size_t len;
  // Never waits: if the PC is not reading, the lines stay where they are.
  while (static_cast<size_t>(Serial.availableForWrite()) >= ECHO_LINE && (len = xMessageBufferReceive(echo, line, sizeof(line), 0)) > 0) {
    Serial.write(reinterpret_cast<const uint8_t *>(line), len);
  }
}

static void noteError(const char *fmt, ...) {
  char text[48];
  va_list args;
  va_start(args, fmt);
  vsnprintf(text, sizeof(text), fmt, args);
  va_end(args);
  taskENTER_CRITICAL(&pubMux);
  strlcpy(errors.last[errors.count % 10], text, sizeof(errors.last[0]));
  errors.count++;
  taskEXIT_CRITICAL(&pubMux);
  usbLine("ERROR ", text);
}

static const char *resetReasonName() {
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON: return "poweron";
    case ESP_RST_SW: return "software";
    case ESP_RST_PANIC: return "panic";
    case ESP_RST_INT_WDT: return "int_wdt";
    case ESP_RST_TASK_WDT: return "task_wdt";
    case ESP_RST_WDT: return "wdt";
    case ESP_RST_DEEPSLEEP: return "deepsleep";
    case ESP_RST_BROWNOUT: return "brownout";
    default: return "other";
  }
}

static time_t unixNow() {
  const time_t t = time(nullptr);
  return t >= VALID_TIME ? t : 0;
}

static void clockWasSet(const char *src) {
  strlcpy(timeSrc, src, sizeof(timeSrc));
  timeEpoch++;
}

// A 64-bit value written by the audio task can be read half-updated; read until two reads agree.
static int64_t stable(volatile int64_t &value) {
  int64_t v;
  do v = value; while (v != value);
  return v;
}

static Shared snapshot() {
  taskENTER_CRITICAL(&pubMux);
  const Shared s = pub;
  taskEXIT_CRITICAL(&pubMux);
  return s;
}

bool recordingActive() {
  const State s = snapshot().state;
  return s == State::Recording || s == State::Stopping;
}

// ---- WAV, checksum ----

static void wavHeader(uint8_t *h, uint32_t dataBytes) {
  const bool open = dataBytes == 0xFFFFFFFF;   // "until end of file": a segment cut off by a power loss still plays
  const uint32_t riffBytes = open ? 0xFFFFFFFF : WAV_HEADER - 8 + dataBytes;
  const uint32_t rate = SAMPLE_RATE, byteRate = SAMPLE_RATE * 2, fmtBytes = 16, junk = WAV_HEADER - 36 - 8 - 8;
  const uint16_t pcm = 1, mono = 1, align = 2, bits = 16;
  memset(h, 0, WAV_HEADER);
  uint8_t *p = h;
  auto put = [&p](const void *v, size_t len) { memcpy(p, v, len); p += len; };
  put("RIFF", 4); put(&riffBytes, 4); put("WAVE", 4);
  put("fmt ", 4); put(&fmtBytes, 4); put(&pcm, 2); put(&mono, 2); put(&rate, 4); put(&byteRate, 4); put(&align, 2); put(&bits, 2);
  put("JUNK", 4); put(&junk, 4);
  p = h + WAV_HEADER - 8;
  put("data", 4); put(&dataBytes, 4);
}

static void toHex(const uint8_t *bytes, size_t len, char *out) {
  static const char digits[] = "0123456789abcdef";
  for (size_t i = 0; i < len; i++) {
    out[2 * i] = digits[bytes[i] >> 4];
    out[2 * i + 1] = digits[bytes[i] & 15];
  }
  out[2 * len] = 0;
}

// Finalizing gives way as soon as the recorder gets a request or the main loop needs the card.
static bool mustYield() { return uxQueueMessagesWaiting(requests) || cardWanted; }

// SHA-256 of a file on the card; false if it could not be read to the end or had to give way.
static bool hashFile(const char *path, char *hex) {
  static uint8_t *block = static_cast<uint8_t *>(heap_caps_malloc(16384, MALLOC_CAP_SPIRAM));
  const int fd = block ? ::open(path, O_RDONLY) : -1;
  if (fd < 0) return false;
  mbedtls_md_context_t sha;
  mbedtls_md_init(&sha);
  mbedtls_md_setup(&sha, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
  mbedtls_md_starts(&sha);
  ssize_t got;
  bool yielded = false;
  while ((got = ::read(fd, block, 16384)) > 0) {
    mbedtls_md_update(&sha, block, got);
    esp_task_wdt_reset();
    delay(1);                 // the card driver polls instead of blocking: let the main loop (USB, Bluetooth) run
    if (mustYield()) {
      yielded = true;
      break;
    }
  }
  ::close(fd);
  uint8_t digest[32];
  mbedtls_md_finish(&sha, digest);
  mbedtls_md_free(&sha);
  toHex(digest, 32, hex);
  return got == 0 && !yielded;
}

// ---- journal (log.txt) ----
// Text lines "KEYWORD name=value ...", append only, every line synced at once. info.json is generated from it.

static bool journal(const char *dir, const char *fmt, ...) {
  char line[400], path[48];
  va_list args;
  va_start(args, fmt);
  vsnprintf(line, sizeof(line) - 1, fmt, args);
  va_end(args);
  usbLine("", line);
  const size_t len = strlen(line);
  line[len] = '\n';
  snprintf(path, sizeof(path), MOUNT "%s/log.txt", dir);
  FILE *f = fopen(path, "a");
  if (!f) return false;
  const bool ok = fwrite(line, 1, len + 1, f) == len + 1 && fflush(f) == 0 && fsync(fileno(f)) == 0;
  return fclose(f) == 0 && ok;
}

// Next complete line. A line without its line end was cut off by a power loss and does not count.
static bool nextLine(FILE *f, char *line, size_t cap) {
  static uint32_t calls = 0;
  if (++calls % 64 == 0) delay(1);   // a long journal must not keep the main loop from answering
  while (fgets(line, cap, f)) {
    const size_t len = strlen(line);
    if (len && line[len - 1] == '\n') return true;
    for (int c; (c = fgetc(f)) != EOF && c != '\n';) {}
  }
  return false;
}

// Copies the value of " name=" into out; false if the line has no such field.
static bool field(const char *line, const char *name, char *out, size_t cap) {
  const size_t len = strlen(name);
  for (const char *p = line; (p = strchr(p, ' ')) != nullptr;) {
    p++;
    if (!strncmp(p, name, len) && p[len] == '=') {
      p += len + 1;
      size_t i = 0;
      while (*p && *p != ' ' && *p != '\r' && *p != '\n' && i + 1 < cap) out[i++] = *p++;
      out[i] = 0;
      return true;
    }
  }
  if (cap) out[0] = 0;
  return false;
}

static int64_t num(const char *line, const char *name) {
  char v[24];
  return field(line, name, v, sizeof(v)) ? strtoll(v, nullptr, 10) : 0;
}

static bool is(const char *line, const char *keyword) {
  const size_t len = strlen(keyword);
  return !strncmp(line, keyword, len) && line[len] == ' ';
}

// The last 300 bytes of a journal. Returns the file size, -1 if there is no journal.
static long journalTail(const char *dir, char *tail, size_t *got) {
  char path[48];
  snprintf(path, sizeof(path), MOUNT "%s/log.txt", dir);
  FILE *f = fopen(path, "r");
  if (!f) return -1;
  fseek(f, 0, SEEK_END);
  const long size = ftell(f);
  fseek(f, size > 300 ? size - 300 : 0, SEEK_SET);
  *got = fread(tail, 1, 300, f);
  fclose(f);
  tail[*got] = 0;
  return size;
}

// True if the journal ends with a DONE line: closed, every checksum present, info.json written.
// The DONE line also carries the totals (segs, kb): the list of recordings then needs only this one read.
// segs and kb stay untouched if the line has none (recordings finished by an older build).
static bool journalDone(const char *dir, unsigned *segs = nullptr, unsigned *kb = nullptr) {
  char tail[301], v[16];
  size_t got;
  if (journalTail(dir, tail, &got) < 0 || !got || tail[got - 1] != '\n') return false;
  tail[got - 1] = 0;
  const char *last = strrchr(tail, '\n');
  last = last ? last + 1 : tail;
  if (!is(last, "DONE")) return false;
  if (segs && field(last, "segs", v, sizeof(v))) *segs = atoi(v);
  if (kb && field(last, "kb", v, sizeof(v))) *kb = strtoul(v, nullptr, 10);
  return true;
}

// One bit per segment number, filled by scanJournal (recorder task only).
static uint8_t hasLine[MAX_SEGMENTS / 8 + 1], hasSha[MAX_SEGMENTS / 8 + 1];
static bool inSet(const uint8_t *set, uint32_t n) { return n < MAX_SEGMENTS && (set[n / 8] >> (n % 8) & 1); }
static void mark(uint8_t *set, uint32_t n) {
  if (n < MAX_SEGMENTS) set[n / 8] |= 1 << (n % 8);
}

struct Scan {
  char rec[400];            // the REC line
  char reason[20], src[8];
  int64_t startUnix;        // wall clock at t_ms = 0; the latest TIMESYNC wins over the REC line
  bool ended, recovered, sdError;
  struct {
    uint32_t n;
    int64_t tMs;
  } opens[8];               // the latest OPEN lines, for segments that never got their SEG line
  uint32_t openCount;
};

// First pass over a journal: what kind of recording this is and which segments are closed and hashed.
static bool scanJournal(const char *dir, Scan &s) {
  char path[48], line[400], v[72];
  memset(&s, 0, sizeof(s));
  strcpy(s.src, "none");
  memset(hasLine, 0, sizeof(hasLine));
  memset(hasSha, 0, sizeof(hasSha));
  snprintf(path, sizeof(path), MOUNT "%s/log.txt", dir);
  FILE *in = fopen(path, "r");
  if (!in) return false;
  while (nextLine(in, line, sizeof(line))) {
    if (is(line, "REC")) {
      strlcpy(s.rec, line, sizeof(s.rec));
      if ((s.startUnix = num(line, "unix")) != 0) field(line, "time_src", s.src, sizeof(s.src));
    } else if (is(line, "TIMESYNC")) {
      s.startUnix = num(line, "unix") - num(line, "t_ms") / 1000;
      field(line, "src", s.src, sizeof(s.src));
    } else if (is(line, "OPEN")) {
      s.opens[s.openCount++ % 8] = {static_cast<uint32_t>(num(line, "n")), num(line, "t_ms")};
    } else if (is(line, "SEG") || is(line, "RECOVERED") || is(line, "HASH")) {
      const uint32_t n = num(line, "n");
      if (line[0] != 'H') mark(hasLine, n);
      if (line[0] == 'R') s.recovered = true;
      if (field(line, "sha256", v, sizeof(v)) && strlen(v) == 64) mark(hasSha, n);
    } else if (is(line, "END")) {
      s.ended = true;
      field(line, "reason", s.reason, sizeof(s.reason));
    } else if (is(line, "ERROR")) {
      if (field(line, "what", v, sizeof(v)) && !strcmp(v, "sd_write")) s.sdError = true;
    }
  }
  fclose(in);
  return s.rec[0] != 0;
}

// The checksum a later HASH line added for segment n.
static bool findHash(const char *dir, uint32_t n, char *sha, size_t cap) {
  char path[48], line[400];
  snprintf(path, sizeof(path), MOUNT "%s/log.txt", dir);
  FILE *in = fopen(path, "r");
  if (!in) return false;
  bool found = false;
  while (nextLine(in, line, sizeof(line))) {
    if (is(line, "HASH") && num(line, "n") == n) found = field(line, "sha256", sha, cap);
  }
  fclose(in);
  return found && strlen(sha) == 64;
}

enum class Info { Live, Stopped, Final };
static unsigned infoSegs = 0, infoKb = 0;   // what the last writeInfo counted (recorder task only)

// Writes info.json from the journal. Live: still recording, the open segment is listed too.
// Stopped: ended, checksums of short segments still missing. Final: everything is in place.
static bool writeInfo(const char *dir, Info phase) {
  static Scan s;
  char logPath[48], newPath[48], path[48], line[400], v[72];
  if (!scanJournal(dir, s)) return false;
  snprintf(logPath, sizeof(logPath), MOUNT "%s/log.txt", dir);
  snprintf(newPath, sizeof(newPath), MOUNT "%s/info.new", dir);
  snprintf(path, sizeof(path), MOUNT "%s/info.json", dir);
  FILE *in = fopen(logPath, "r");
  FILE *out = in ? fopen(newPath, "w") : nullptr;
  if (!out) {
    if (in) fclose(in);
    return false;
  }
  const int rate = num(s.rec, "rate");
  auto text = [&](const char *name) {
    field(s.rec, name, v, sizeof(v));
    return v;
  };
  fprintf(out, "{\n\"schema\":1,\"fw\":\"%s\"", text("fw"));
  fprintf(out, ",\"device\":\"%s\"", text("dev"));
  fprintf(out, ",\"boot\":%d,\"reset\":\"%s\"", static_cast<int>(num(s.rec, "boot")), text("reset"));
  fprintf(out, ",\n\"rec_id\":\"%s\",\"rec_n\":%d", text("id"), static_cast<int>(num(s.rec, "n")));
  fprintf(out, ",\n\"start_unix\":%lld,\"time_src\":\"%s\",\"start_up_ms\":%lld", static_cast<long long>(s.startUnix), s.src,
          static_cast<long long>(num(s.rec, "up_ms")));
  fprintf(out, ",\n\"sample_rate\":%d,\"bits\":16,\"channels\":1,\"segment_s\":%d,\n\"segments\":[", rate,
          static_cast<int>(num(s.rec, "seg_s")));

  uint64_t samples = 0;
  int64_t dropped = 0, openN = -1, openMs = 0;
  int count = 0;
  auto segment = [&](int n, int64_t tMs, int64_t segSamples, int64_t bytes, const char *rest) {
    fprintf(out, "%s\n{\"n\":%d,\"file\":\"%04d.wav\",\"t_ms\":%lld,\"unix\":%lld,\"duration_s\":%.3f,\"samples\":%lld,\"bytes\":%lld%s}",
            count++ ? "," : "", n, n, static_cast<long long>(tMs),
            static_cast<long long>(s.startUnix ? s.startUnix + tMs / 1000 : 0), rate ? segSamples / static_cast<double>(rate) : 0.0,
            static_cast<long long>(segSamples), static_cast<long long>(bytes), rest);
  };
  while (nextLine(in, line, sizeof(line))) {
    if (is(line, "OPEN")) {
      openN = num(line, "n");
      openMs = num(line, "t_ms");
    } else if (is(line, "SEG") || is(line, "RECOVERED")) {
      char rest[300], sha[72], end[16], power[12];
      const uint32_t n = num(line, "n");
      field(line, "sha256", sha, sizeof(sha));
      if (strlen(sha) != 64 && inSet(hasSha, n)) findHash(dir, n, sha, sizeof(sha));
      const bool hashed = strlen(sha) == 64;
      field(line, "end", end, sizeof(end));
      field(line, "power", power, sizeof(power));
      const int64_t segDropped = num(line, "dropped");
      snprintf(rest, sizeof(rest),
               ",\"dropped\":%lld,\"missing\":%d,\"gap_max_ms\":%d,\"ring_max_pct\":%.1f,\"write_max_ms\":%d,\"sha256\":%s%s%s,"
               "\"end\":\"%s\",\"wifi\":%s,\"rssi\":%d,\"power\":\"%s\"",
               static_cast<long long>(segDropped), static_cast<int>(num(line, "missing")), static_cast<int>(num(line, "gap_max_ms")),
               num(line, "ring_max") * 100.0 / AUDIO_RING_BYTES, static_cast<int>(num(line, "write_max_ms")), hashed ? "\"" : "",
               hashed ? sha : "null", hashed ? "\"" : "", end, num(line, "wifi") ? "true" : "false",
               static_cast<int>(num(line, "rssi")), power[0] ? power : "unknown");
      segment(n, num(line, "t_ms"), num(line, "samples"), num(line, "bytes"), rest);
      samples += num(line, "samples");
      if (segDropped > 0) dropped += segDropped;
      if (openN == n) openN = -1;
    }
  }
  if (openN >= 0) segment(openN, openMs, 0, 0, ",\"sha256\":null,\"end\":\"open\"");
  const char *state = phase == Info::Live || !s.ended ? "open" : phase == Info::Stopped ? "stopped" : s.recovered ? "recovered" : "closed";
  fprintf(out, "\n],\n\"state\":\"%s\",\"stop_reason\":\"%s\",\"samples\":%llu,\"dropped\":%lld,\"duration_s\":%.3f\n}\n", state,
          s.reason, static_cast<unsigned long long>(samples), static_cast<long long>(dropped),
          rate ? samples / static_cast<double>(rate) : 0.0);
  fclose(in);
  infoSegs = count;
  infoKb = (samples * 2 + static_cast<uint64_t>(count) * WAV_HEADER) >> 10;
  const bool ok = fflush(out) == 0 && fsync(fileno(out)) == 0;
  if (fclose(out) != 0 || !ok) return false;
  ::unlink(path);   // FAT cannot rename onto an existing file; info.json is derived, a missing one is rebuilt
  return ::rename(newPath, path) == 0;
}

// ---- hardware ----

// Same driver settings as the Arduino I2S library in PDM_MONO_MODE, proven on this board since DV1.2.
static bool setupMic() {
  i2s_config_t cfg = {};
  cfg.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM);
  cfg.sample_rate = SAMPLE_RATE;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  cfg.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.intr_alloc_flags = ESP_INTR_FLAG_LEVEL2;
  cfg.dma_buf_count = 32;          // 1 s of headroom for the capture task
  cfg.dma_buf_len = 512;
  cfg.use_apll = false;
  if (i2s_driver_install(I2S_NUM_0, &cfg, 0, nullptr) != ESP_OK) return false;
  if (i2s_set_clk(I2S_NUM_0, SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO) != ESP_OK) return false;
  i2s_pin_config_t pins = {};
  pins.mck_io_num = I2S_PIN_NO_CHANGE;
  pins.bck_io_num = I2S_PIN_NO_CHANGE;
  pins.ws_io_num = 42;
  pins.data_out_num = I2S_PIN_NO_CHANGE;
  pins.data_in_num = 41;
  return i2s_set_pin(I2S_NUM_0, &pins) == ESP_OK;
}

// Formats only when no readable FAT file system is found, never a card that mounts.
static bool mountCard(bool formatIfUnreadable) {
  SPI.begin(7, 8, 9, 21);
  return SD.begin(21, SPI, SD_SPI_HZ, MOUNT, 5, formatIfUnreadable);
}

// ---- audio task ----
// Runs all the time; outside a recording the samples only feed the level meter.

static void audioTaskFn(void *) {
  static int16_t block[512];
  const TickType_t wait = pdMS_TO_TICKS(500);
  size_t got = 0;
  // Empty what piled up while nobody was reading, then drop the microphone's start-up transient.
  for (int i = 0; i < 64; i++) {
    const int64_t t = esp_timer_get_time();
    i2s_read(I2S_NUM_0, block, sizeof(block), &got, wait);
    if (esp_timer_get_time() - t > 5000) break;
  }
  for (int i = 0; i < 15; i++) i2s_read(I2S_NUM_0, block, sizeof(block), &got, wait);
  int32_t sum = 0;
  for (int16_t v : block) sum += v;
  int32_t offsetQ8 = (sum / 512) * 256;         // the microphone sits on a constant offset of about 1400

  int64_t last = esp_timer_get_time();
  for (;;) {
    i2s_read(I2S_NUM_0, block, sizeof(block), &got, wait);
    if (!got) continue;
    const int64_t now = esp_timer_get_time();
    if (captureWanted && !capturing) {            // a recording starts exactly on a block boundary
      capSamples = capDropped = capGapMaxMs = capPeak = 0;
      capT0Us = last;
      capLastUs = last;
      capturing = true;
    } else if (!captureWanted && capturing) {
      capturing = false;
    }

    const size_t samples = got / 2;
    uint32_t peak = 0;
    for (size_t i = 0; i < samples; i++) {
      offsetQ8 += (block[i] * 256 - offsetQ8) >> 10;   // follows the offset slowly (64 ms), speech is untouched
      const int32_t v = constrain((block[i] - (offsetQ8 >> 8)) << AUDIO_GAIN_SHIFT, -32768, 32767);
      block[i] = v;
      if (static_cast<uint32_t>(abs(v)) > peak) peak = abs(v);
    }
    micLevel = peak;
    if (capturing) {
      const uint32_t gapMs = (now - last) / 1000;
      if (gapMs > capGapMaxMs) capGapMaxMs = gapMs;
      if (peak > capPeak) capPeak = peak;
      // Whole blocks only, so a full ring never shifts the sample alignment.
      if (xStreamBufferSpacesAvailable(audioRing) >= got) xStreamBufferSend(audioRing, block, got, 0);
      else capDropped += samples;
      capSamples += samples;
      capLastUs = now;
    }
    last = now;
  }
}

// ---- recorder task: owns every file on the card ----

struct Recording {
  uint32_t n = 0, segSamplesMax = 0, seg = 0, segSamples = 0, segDroppedAt = 0, segRingMax = 0;
  uint32_t runSamples = 0;          // written since the capture last started (pause/resume starts a new run)
  uint32_t droppedBefore = 0;       // dropped in earlier capture runs of this recording
  uint64_t captured = 0;            // samples the microphone delivered in earlier capture runs
  int64_t clockUs = 0;              // time those capture runs lasted
  uint64_t samples = 0;
  int64_t t0Us = 0, segT0Us = 0;
  uint32_t lastSyncMs = 0, lastStatMs = 0, startedMs = 0, writeMaxMs = 0, ringMax = 0, timeEpoch = 0;
  bool segOpen = false;
  char id[28] = "", dir[16] = "", segPath[40] = "";
  mbedtls_md_context_t sha;
};
static Recording rec;
static BufFile wav;
static State state = State::Idle;
static bool cardReady = false, wantRec = false, cardFull = false;
static volatile bool finalized = false;     // false: some recording on the card still has to be finished
static int startFailures = 0;
static uint32_t nextStartMs = 0, sdErrors = 0, totalMb = 0, freeMb = 0, deviceEpoch = 0;
static char stopReason[16] = "", failedDir[16] = "";

static void publish() {
  taskENTER_CRITICAL(&pubMux);
  pub.state = state;
  pub.card = cardReady;
  pub.want = wantRec;
  pub.finalized = finalized;
  pub.recN = rec.n;
  pub.seg = rec.seg;
  pub.samples = rec.samples;
  pub.dropped = rec.droppedBefore + capDropped;
  pub.ringMax = rec.ringMax;
  pub.writeMaxMs = max(rec.writeMaxMs, wav.maxWriteMs);
  pub.totalMb = totalMb;
  pub.freeMb = freeMb;
  pub.sdErrors = sdErrors;
  pub.segLen = rec.segSamplesMax / SAMPLE_RATE;
  pub.t0Us = rec.t0Us;
  strlcpy(pub.recId, rec.id, sizeof(pub.recId));
  strlcpy(pub.stopReason, stopReason, sizeof(pub.stopReason));
  taskEXIT_CRITICAL(&pubMux);
}

// The wish to record survives every restart that is not a power-on: who stopped or paused is not recorded again.
static void rememberWant(bool want) {
  Preferences prefs;
  prefs.begin("ar1", false);
  if (prefs.getBool("want", false) != want) prefs.putBool("want", want);
  prefs.end();
}

// False if the card does not answer; the old numbers then stay.
static bool readSpace() {
  const uint64_t total = SD.totalBytes();
  if (!total) return false;
  totalMb = total >> 20;
  freeMb = totalMb - (SD.usedBytes() >> 20);
  return true;
}

// Room a recording needs before it may open another segment.
static uint32_t neededMb(uint32_t segSamples) {
  const uint32_t knob = minFreeMb;
  return max<uint32_t>(knob, (segSamples * 2 >> 20) + 16);
}

static uint32_t droppedNow() { return rec.droppedBefore + capDropped; }
static int64_t recMs() { return (esp_timer_get_time() - rec.t0Us) / 1000; }

// Samples the clock says should exist minus samples received in this capture run; about 0 = no gap.
static int missingNow() {
  return (stable(capLastUs) - stable(capT0Us)) * static_cast<int64_t>(SAMPLE_RATE) / 1000000 - capSamples;
}

// Ties the recording's timeline to the wall clock whenever the clock was set since the last time.
static void journalTime() {
  if (rec.timeEpoch == timeEpoch || !unixNow()) return;
  rec.timeEpoch = timeEpoch;
  journal(rec.dir, "TIMESYNC t_ms=%lld unix=%lld src=%s", static_cast<long long>(recMs()), static_cast<long long>(unixNow()), timeSrc);
}

static bool openSegment(int64_t t0Us) {
  if (!readSpace()) return false;                       // card gone: handled like a write error
  if (freeMb < neededMb(rec.segSamplesMax)) {
    cardFull = true;
    return false;
  }
  snprintf(rec.segPath, sizeof(rec.segPath), MOUNT "%s/%04u.wav", rec.dir, static_cast<unsigned>(rec.seg));
  if (!wav.open(rec.segPath, 16384)) return false;
  uint8_t header[WAV_HEADER];
  wavHeader(header, 0xFFFFFFFF);
  wav.write(header, WAV_HEADER);
  // The checksum covers the finished file. The header of a full segment is known now, so it goes in first;
  // a segment that ends early gets its checksum later, when the recorder is idle.
  wavHeader(header, rec.segSamplesMax * 2);
  mbedtls_md_init(&rec.sha);
  mbedtls_md_setup(&rec.sha, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
  mbedtls_md_starts(&rec.sha);
  mbedtls_md_update(&rec.sha, header, WAV_HEADER);
  rec.segOpen = true;
  rec.segSamples = 0;
  rec.segT0Us = t0Us;
  rec.segDroppedAt = droppedNow();
  rec.segRingMax = 0;
  wav.sync(false);
  return !wav.err && journal(rec.dir, "OPEN n=%u file=%04u.wav t_ms=%lld unix=%lld", static_cast<unsigned>(rec.seg),
                             static_cast<unsigned>(rec.seg), static_cast<long long>((t0Us - rec.t0Us) / 1000),
                             static_cast<long long>(unixNow()));
}

// Final header, then the SEG line. False on a card error: the journal then has no SEG line for this
// segment and finalizing completes it later.
static bool closeSegment(const char *end) {
  if (!rec.segOpen) return true;
  rec.segOpen = false;
  uint8_t header[WAV_HEADER];
  wavHeader(header, rec.segSamples * 2);
  wav.finish(header, WAV_HEADER);
  if (wav.maxWriteMs > rec.writeMaxMs) rec.writeMaxMs = wav.maxWriteMs;
  char sha[65] = "-";
  if (rec.segSamples == rec.segSamplesMax) {
    uint8_t digest[32];
    mbedtls_md_finish(&rec.sha, digest);
    toHex(digest, 32, sha);
  }
  mbedtls_md_free(&rec.sha);
  if (wav.err) return false;
  const bool wifi = WiFi.status() == WL_CONNECTED;
  const bool ok = journal(rec.dir,
                          "SEG n=%u file=%04u.wav t_ms=%lld samples=%u bytes=%u dropped=%u missing=%d gap_max_ms=%u ring_max=%u "
                          "write_max_ms=%u sha256=%s end=%s wifi=%d rssi=%d power=%s",
                          static_cast<unsigned>(rec.seg), static_cast<unsigned>(rec.seg),
                          static_cast<long long>((rec.segT0Us - rec.t0Us) / 1000), static_cast<unsigned>(rec.segSamples),
                          static_cast<unsigned>(WAV_HEADER + rec.segSamples * 2), static_cast<unsigned>(droppedNow() - rec.segDroppedAt),
                          missingNow(), static_cast<unsigned>(capGapMaxMs), static_cast<unsigned>(rec.segRingMax),
                          static_cast<unsigned>(wav.maxWriteMs), sha, end, wifi, wifi ? WiFi.RSSI() : 0, powerName());
  rec.seg++;
  return ok;
}

// captured - dropped must equal samples, and captured must match clock_ms: otherwise audio was lost on the way.
static bool journalEnd(const char *reason) {
  return journal(rec.dir, "END reason=%s t_ms=%lld samples=%llu dropped=%u captured=%llu clock_ms=%lld", reason,
                 static_cast<long long>(recMs()), static_cast<unsigned long long>(rec.samples),
                 static_cast<unsigned>(rec.droppedBefore), static_cast<unsigned long long>(rec.captured),
                 static_cast<long long>(rec.clockUs / 1000));
}

static void stat() {
  const bool wifi = WiFi.status() == WL_CONNECTED;
  journal(rec.dir, "STAT t_s=%u seg=%u samples=%llu dropped=%u missing=%d ring=%u ring_max=%u write_max_ms=%u gap_max_ms=%u heap=%u wifi=%d rssi=%d",
          static_cast<unsigned>(recMs() / 1000), static_cast<unsigned>(rec.seg), static_cast<unsigned long long>(rec.samples),
          static_cast<unsigned>(droppedNow()), missingNow(), static_cast<unsigned>(xStreamBufferBytesAvailable(audioRing)),
          static_cast<unsigned>(rec.ringMax), static_cast<unsigned>(max(rec.writeMaxMs, wav.maxWriteMs)),
          static_cast<unsigned>(capGapMaxMs), static_cast<unsigned>(ESP.getFreeHeap()), wifi, wifi ? WiFi.RSSI() : 0);
}

static void abandonSegment() {
  wav.abandon();
  if (rec.segOpen) mbedtls_md_free(&rec.sha);
  rec.segOpen = false;
}

// Starts the capture and waits for the audio task to pick it up on a block boundary.
static bool startCapture() {
  uint8_t scrap[512];
  while (xStreamBufferReceive(audioRing, scrap, sizeof(scrap), 0) > 0) {}
  captureWanted = true;
  for (int i = 0; !capturing && i < 200; i++) delay(5);
  return capturing;
}

static void haltCapture() {
  captureWanted = false;
  for (int i = 0; capturing && i < 200; i++) delay(5);
}

// From here on the audio task no longer touches the cap* counters; what they counted moves to the recording.
static void stopCapture() {
  haltCapture();
  rec.droppedBefore += capDropped;
  rec.captured += capSamples;
  rec.clockUs += stable(capLastUs) - stable(capT0Us);
  capDropped = capSamples = 0;
  capT0Us = capLastUs = 0;
  rec.runSamples = 0;
}

static bool startRecording() {
  cardFull = false;
  if (!micReady) return false;
  if (freeMb < neededMb(segLenSeconds * SAMPLE_RATE)) {   // before anything is created: no empty folders on a full card
    cardFull = true;
    return false;
  }
  Preferences prefs;
  prefs.begin("ar1", false);
  uint32_t n = prefs.getUInt("rec", 0);
  char dir[16];
  do snprintf(dir, sizeof(dir), "/rec/%06u", static_cast<unsigned>(++n)); while (SD.exists(dir));
  prefs.putUInt("rec", n);
  prefs.end();
  SD.mkdir("/rec");
  if (!SD.mkdir(dir)) return false;

  rec = Recording{};
  rec.n = n;
  rec.segSamplesMax = segLenSeconds * SAMPLE_RATE;
  rec.timeEpoch = timeEpoch;
  strlcpy(rec.dir, dir, sizeof(rec.dir));
  snprintf(rec.id, sizeof(rec.id), "%s-%06u-%08x", deviceId, static_cast<unsigned>(n), static_cast<unsigned>(esp_random()));
  stopReason[0] = 0;
  const time_t now = unixNow();
  if (!startCapture()) return false;
  rec.t0Us = stable(capT0Us);
  if (!journal(dir, "REC id=%s n=%u fw=" FW_VERSION " dev=%s boot=%u reset=%s seg_s=%u rate=%u unix=%lld time_src=%s up_ms=%lld", rec.id,
               static_cast<unsigned>(n), deviceId, static_cast<unsigned>(bootCount), resetReasonName(),
               static_cast<unsigned>(segLenSeconds), static_cast<unsigned>(SAMPLE_RATE), static_cast<long long>(now),
               now ? timeSrc : "none", static_cast<long long>(rec.t0Us / 1000))) return false;
  if (!openSegment(rec.t0Us)) return false;
  writeInfo(dir, Info::Live);
  rec.lastSyncMs = rec.lastStatMs = rec.startedMs = millis();
  state = State::Recording;
  return true;
}

static void drain() {
  static uint8_t chunk[4096];
  if (faultSd) {               // test switch: behave as if a write had failed
    faultSd = false;
    wav.err = true;
    return;
  }
  const uint32_t waiting = xStreamBufferBytesAvailable(audioRing);
  if (waiting > rec.ringMax) rec.ringMax = waiting;
  if (waiting > rec.segRingMax) rec.segRingMax = waiting;
  size_t got;
  while (!wav.err && (got = xStreamBufferReceive(audioRing, chunk, sizeof(chunk), 0)) > 0) {
    for (size_t at = 0; at < got && !wav.err;) {
      const size_t room = (rec.segSamplesMax - rec.segSamples) * 2;
      const size_t k = min(got - at, room);
      wav.write(chunk + at, k);
      mbedtls_md_update(&rec.sha, chunk + at, k);
      at += k;
      rec.segSamples += k / 2;
      rec.runSamples += k / 2;
      rec.samples += k / 2;
      if (rec.segSamples == rec.segSamplesMax) {       // sample-exact segment change, nothing is lost in between
        const int64_t nextT0 = stable(capT0Us) + (static_cast<int64_t>(rec.runSamples) + capDropped) * 1000000 / SAMPLE_RATE;
        if (!closeSegment("full") || rec.seg >= MAX_SEGMENTS || !openSegment(nextT0)) {
          wav.err = true;
          return;
        }
        writeInfo(rec.dir, Info::Live);
      }
    }
  }
}

// Ends the capture and closes the files. pause=true keeps the recording open for REC RESUME.
// The last, short segment is closed without a checksum; finalizing adds it while the recorder is idle.
static void stopRecording(const char *reason, bool pause) {
  const bool running = state == State::Recording;   // from Paused there is nothing left to close
  state = State::Stopping;
  publish();
  bool ok = true;
  if (running) {
    haltCapture();
    drain();
    ok = !wav.err && closeSegment(pause ? "pause" : "stop");
    stopCapture();
    journalTime();
  }
  if (pause) {
    ok = ok && journal(rec.dir, "PAUSE t_ms=%lld", static_cast<long long>(recMs()));
    writeInfo(rec.dir, Info::Live);
    state = State::Paused;
  } else {
    ok = ok && journalEnd(reason);
    writeInfo(rec.dir, Info::Stopped);
    strlcpy(stopReason, reason, sizeof(stopReason));
    state = State::Idle;
    finalized = false;
  }
  if (!ok) noteError("close_failed %s", rec.dir);
  readSpace();
}

// The recording cannot go on (write error, card full, too many segments). The files stay as they are;
// what is missing (final header, checksum, END line) is added by finalizing once the card works again.
static void failRecording() {
  const bool tooLong = rec.seg >= MAX_SEGMENTS;
  stopCapture();
  abandonSegment();
  strlcpy(stopReason, cardFull ? "card_full" : tooLong ? "max_segments" : "sd_error", sizeof(stopReason));
  noteError("recording_failed %s %s", rec.dir, stopReason);
  state = State::Idle;
  finalized = false;
  if (cardFull || tooLong) {     // the card itself works: close the journal properly
    journalEnd(stopReason);
    writeInfo(rec.dir, Info::Stopped);
    if (cardFull) wantRec = false;   // V1 never deletes recordings on its own
    return;
  }
  sdErrors++;
  if (millis() - rec.startedMs < 60000) startFailures++;
  // The SD driver gives up for good when the card stays busy for 500 ms; only a fresh mount helps.
  // Until then the old journal cannot be written either: the cause is noted there after the mount.
  strlcpy(failedDir, rec.dir, sizeof(failedDir));
  xSemaphoreTake(cardMutex, portMAX_DELAY);
  SD.end();
  cardReady = false;
  xSemaphoreGive(cardMutex);
  nextStartMs = millis() + 2000;
}

static bool resumeRecording() {
  cardFull = false;
  if (!startCapture()) return false;
  const int64_t t0 = stable(capT0Us);
  if (!journal(rec.dir, "RESUME t_ms=%lld", static_cast<long long>((t0 - rec.t0Us) / 1000)) || !openSegment(t0)) return false;
  writeInfo(rec.dir, Info::Live);
  rec.lastSyncMs = rec.lastStatMs = millis();
  state = State::Recording;
  return true;
}

// ---- finalizing: one way to finish every recording that is not DONE yet; runs only while idle ----
// Repeatable at every step. Gives way to a request (mustYield) and is then simply started again.

enum class Fin { Done, Again, NotOurs, Failed };

static Fin finalizeDir(const char *dir) {
  static Scan s;
  char path[48], line[400], sha[72], tail[301];
  size_t got;

  // 1. A last line without its line end was cut off by a power loss: remove the fragment.
  const long size = journalTail(dir, tail, &got);
  if (size < 0) return Fin::NotOurs;
  snprintf(path, sizeof(path), MOUNT "%s/log.txt", dir);
  if (got && tail[got - 1] != '\n') {
    const char *lastEnd = strrchr(tail, '\n');
    if (!lastEnd && size > 300) return Fin::Failed;
    if (::truncate(path, lastEnd ? size - static_cast<long>(got) + (lastEnd - tail) + 1 : 0) != 0) return Fin::Failed;
  }
  if (!scanJournal(dir, s)) return Fin::NotOurs;

  // 2. Segment files without a SEG line: header from the file size, checksum, RECOVERED.
  //    Collected first, repaired afterwards: the folder is not changed while it is being listed.
  struct {
    unsigned n;
    uint32_t size;
  } todo[8];
  size_t todoCount = 0;
  bool again = false;
  File folder = SD.open(dir);
  for (File f = folder ? folder.openNextFile() : File(); f; f = folder.openNextFile()) {
    unsigned n = 0;
    char extra = 0;
    if (f.isDirectory() || strlen(f.name()) != 8 || sscanf(f.name(), "%4u.wav%c", &n, &extra) != 1) continue;
    if (inSet(hasLine, n)) continue;
    if (todoCount == 8) again = true;       // the next pass takes the rest
    else todo[todoCount++] = {n, static_cast<uint32_t>(f.size())};
  }
  folder.close();
  for (size_t i = 0; i < todoCount; i++) {
    const unsigned n = todo[i].n;
    const uint32_t dataBytes = todo[i].size > WAV_HEADER ? (todo[i].size - WAV_HEADER) & ~1u : 0;
    snprintf(path, sizeof(path), MOUNT "%s/%04u.wav", dir, n);
    uint8_t header[WAV_HEADER];
    wavHeader(header, dataBytes);
    const int fd = ::open(path, O_WRONLY);
    const bool written = fd >= 0 && ::write(fd, header, WAV_HEADER) == static_cast<ssize_t>(WAV_HEADER) && ::fsync(fd) == 0;
    if (fd >= 0) ::close(fd);
    if (!written) return Fin::Failed;
    if (!hashFile(path, sha)) return mustYield() ? Fin::Again : Fin::Failed;
    // t_ms is exact if the journal still has the OPEN line of this segment, otherwise the nominal position.
    int64_t tMs = static_cast<int64_t>(n) * num(s.rec, "seg_s") * 1000;
    for (const auto &open : s.opens) {
      if (open.n == n && (open.tMs || !n)) tMs = open.tMs;
    }
    if (!journal(dir, "RECOVERED n=%u file=%04u.wav t_ms=%lld samples=%u bytes=%u dropped=-1 missing=0 gap_max_ms=0 ring_max=0 "
                      "write_max_ms=0 sha256=%s end=interrupted wifi=0 rssi=0 power=unknown",
                 n, n, static_cast<long long>(tMs), static_cast<unsigned>(dataBytes / 2),
                 static_cast<unsigned>(WAV_HEADER + dataBytes), sha)) return Fin::Failed;
  }

  // 3. Segments that were closed without a checksum (stop, pause): add it as a HASH line.
  unsigned pending[8];
  size_t pendingCount = 0;
  snprintf(path, sizeof(path), MOUNT "%s/log.txt", dir);
  FILE *in = fopen(path, "r");
  if (!in) return Fin::Failed;
  while (nextLine(in, line, sizeof(line))) {
    if (!is(line, "SEG") || inSet(hasSha, num(line, "n"))) continue;
    if (pendingCount == 8) again = true;
    else pending[pendingCount++] = num(line, "n");
  }
  fclose(in);
  for (size_t i = 0; i < pendingCount; i++) {
    if (inSet(hasSha, pending[i])) continue;
    snprintf(path, sizeof(path), MOUNT "%s/%04u.wav", dir, pending[i]);
    if (!hashFile(path, sha)) return mustYield() ? Fin::Again : Fin::Failed;
    if (!journal(dir, "HASH n=%u sha256=%s", pending[i], sha)) return Fin::Failed;
    mark(hasSha, pending[i]);
  }
  if (again) return Fin::Again;

  // 4. No END line: the recording was cut off. Then info.json, and DONE as the very last line.
  if (!s.ended && !journal(dir, "END reason=%s", s.sdError ? "sd_error" : "interrupted")) return Fin::Failed;
  if (!writeInfo(dir, Info::Final)) return Fin::Failed;
  return journal(dir, "DONE unix=%lld segs=%u kb=%u", static_cast<long long>(unixNow()), infoSegs, infoKb) ? Fin::Done : Fin::Failed;
}

// One pass over /rec.
static void finalizeAll() {
  xSemaphoreTake(cardMutex, portMAX_DELAY);
  bool again = false;
  File root = SD.open("/rec");
  for (File d = root ? root.openNextFile() : File(); d && !again; d = root.openNextFile()) {
    char dir[16];
    if (!d.isDirectory() || strlen(d.name()) != 6) continue;   // DV2 used four digits; those folders are not ours
    snprintf(dir, sizeof(dir), "/rec/%s", d.name());
    esp_task_wdt_reset();
    if (journalDone(dir)) continue;
    const Fin result = finalizeDir(dir);
    if (result == Fin::Failed) noteError("finalize_failed %s", dir);
    again = result == Fin::Again || mustYield();
  }
  root.close();
  readSpace();
  xSemaphoreGive(cardMutex);
  finalized = !again;
}

static void onMounted() {
  // The first count of free space reads the whole FAT if the card does not carry a valid count
  // (freshly formatted): that can take many seconds, so the watchdog is off for it.
  esp_task_wdt_delete(nullptr);
  readSpace();
  esp_task_wdt_add(nullptr);
  finalized = false;
  if (failedDir[0]) {
    journal(failedDir, "ERROR what=sd_write");
    failedDir[0] = 0;
  }
  static bool logged = false;
  if (logged) return;
  logged = true;
  FILE *f = fopen(MOUNT "/device.log", "a");
  if (!f) return;
  fprintf(f, "BOOT boot=%u reset=%s fw=" FW_VERSION " dev=%s unix=%lld time_src=%s\n", static_cast<unsigned>(bootCount),
          resetReasonName(), deviceId, static_cast<long long>(unixNow()), unixNow() ? timeSrc : "none");
  fclose(f);
}

// Lets the PC date recordings of this boot that ended before the clock was set.
static void deviceTime() {
  if (deviceEpoch == timeEpoch || !unixNow()) return;
  deviceEpoch = timeEpoch;
  FILE *f = fopen(MOUNT "/device.log", "a");
  if (!f) return;
  fprintf(f, "TIMESYNC boot=%u up_ms=%lld unix=%lld src=%s\n", static_cast<unsigned>(bootCount),
          static_cast<long long>(esp_timer_get_time() / 1000), static_cast<long long>(unixNow()), timeSrc);
  fclose(f);
}

static void handle(const Request &rq) {
  switch (rq.cmd) {
    case Cmd::Start:
      wantRec = true;
      startFailures = 0;
      nextStartMs = 0;
      if (!rq.keepWant) rememberWant(true);
      // fall through: starting a paused recording continues it
    case Cmd::Resume:
      if (state != State::Paused) break;
      wantRec = true;
      if (!rq.keepWant) rememberWant(true);
      if (!resumeRecording()) failRecording();
      break;
    case Cmd::Stop:
      wantRec = false;
      if (!rq.keepWant) rememberWant(false);
      if (state != State::Idle) stopRecording(rq.reason, false);
      break;
    case Cmd::Pause:
      if (state != State::Recording) break;
      rememberWant(false);
      stopRecording("pause", true);
      break;
  }
}

static void recorderTask(void *) {
  esp_task_wdt_add(nullptr);
  uint32_t lastMountTry = 0;
  for (;;) {
    esp_task_wdt_reset();
    Request rq;
    if (xQueueReceive(requests, &rq, state == State::Recording ? 0 : pdMS_TO_TICKS(50))) {
      handle(rq);
      publish();     // before the long work below (finalizing) hides the new state for seconds
    }

    if (state == State::Recording) {
      drain();
      const uint32_t nowMs = millis();
      if (!wav.err && nowMs - rec.lastSyncMs >= SYNC_MS) {
        rec.lastSyncMs = nowMs;
        wav.sync(false);
      }
      if (!wav.err) journalTime();
      if (!wav.err && nowMs - rec.lastStatMs >= STAT_MS) {
        rec.lastStatMs = nowMs;
        stat();
        if (nowMs - rec.startedMs >= 60000) startFailures = 0;
      }
      if (wav.err) {
        failRecording();
      } else if (esp_timer_get_time() - stable(capLastUs) > MIC_STALL_US) {
        // Segments end by sample count: without samples the recording would hang and look healthy.
        noteError("mic_stalled");
        stopRecording("mic_error", false);
        publish();
        delay(200);
        ESP.restart();
      } else {
        delay(20);
      }
    } else if (!cardReady) {
      if (!lastMountTry || millis() - lastMountTry >= 5000) {
        lastMountTry = millis() | 1;
        xSemaphoreTake(cardMutex, portMAX_DELAY);
        cardReady = mountCard(false);
        xSemaphoreGive(cardMutex);
        if (cardReady) onMounted();
      }
    } else if (state == State::Idle && wantRec && !sleepMode && millis() >= nextStartMs) {
      if (!startRecording()) {
        stopCapture();
        abandonSegment();
        if (cardFull) {
          wantRec = false;
          strlcpy(stopReason, "card_full", sizeof(stopReason));
          noteError("card_full free_mb=%u", static_cast<unsigned>(freeMb));
        } else if (++startFailures >= MAX_START_FAILURES) {
          wantRec = false;
          noteError("giving_up_after_%d_failures", MAX_START_FAILURES);
        } else {
          noteError("start_failed");
          finalized = false;
          xSemaphoreTake(cardMutex, portMAX_DELAY);
          SD.end();
          cardReady = false;
          xSemaphoreGive(cardMutex);
          nextStartMs = millis() + 2000 * startFailures;
        }
      }
    } else if (state == State::Idle && !wantRec && !finalized && (sleepMode == 0 || sleepMode == 2)) {
      finalizeAll();
    }
    if (cardReady && state != State::Stopping) deviceTime();
    publish();
  }
}

// ---- commands (USB now, BLE later: same words, one JSON object as the answer) ----

struct Json {
  String s = "{";
  void raw(const char *key, const String &value) {
    if (s.length() > 1) s += ',';
    s += '"'; s += key; s += "\":"; s += value;
  }
  void num(const char *key, double value, int decimals = 0) {
    raw(key, decimals ? String(value, decimals) : String(static_cast<long long>(llround(value))));
  }
  void flag(const char *key, bool value) { raw(key, value ? "true" : "false"); }
  void str(const char *key, const String &value) { raw(key, "\"" + value + "\""); }
  String done() { return s + "}"; }
};

static const char *stateName(const Shared &s) {
  if (!s.card && s.state == State::Idle) return "no_card";
  switch (s.state) {
    case State::Recording: return "recording";
    case State::Paused: return "paused";
    case State::Stopping: return "stopping";
    default: return "idle";
  }
}

static String statusJson() {
  const Shared s = snapshot();
  Json j;
  j.flag("ok", true);
  j.str("fw", FW_VERSION);
  j.str("dev", deviceId);
  j.num("boot", bootCount);
  j.str("reset", resetReasonName());
  j.num("up_s", millis() / 1000);
  j.str("state", stateName(s));
  j.flag("want", s.want);
  j.flag("autostart", autostart);
  j.flag("finalizing", s.card && !s.finalized && s.state == State::Idle);   // unfinished recordings are being completed
  j.num("unix", unixNow());
  j.str("time_src", unixNow() ? timeSrc : "none");
  Json r;
  r.num("n", s.recN);
  r.str("id", s.recId);
  r.num("seg", s.seg);
  r.num("seg_s", s.segLen);
  r.num("elapsed_s", s.state == State::Idle || !s.t0Us ? 0 : (esp_timer_get_time() - s.t0Us) / 1e6, 1);
  r.num("samples", s.samples);
  r.num("dropped", s.dropped);
  r.num("missing", s.state == State::Recording ? missingNow() : 0);
  r.num("gap_max_ms", capGapMaxMs);
  r.num("ring_pct", xStreamBufferBytesAvailable(audioRing) * 100.0 / AUDIO_RING_BYTES, 1);
  r.num("ring_max_pct", s.ringMax * 100.0 / AUDIO_RING_BYTES, 1);
  r.num("write_max_ms", s.writeMaxMs);
  r.str("stop_reason", s.stopReason);
  j.raw("rec", r.done());
  j.num("mic_level", micLevel);
  Json c;
  c.flag("ok", s.card);
  c.num("total_mb", s.totalMb);
  c.num("free_mb", s.freeMb);
  c.num("errors", s.sdErrors);
  j.raw("sd", c.done());
  j.raw("wifi", RecorderWifi::statusJson());
  j.flag("ble", RecorderBle::connected());
  Json p;
  const int mv = batMv;
  p.str("state", powerName());
  p.num("bat_mv", mv);
  p.raw("bat_pct", mv >= BAT_MIN_MV ? String(batPercent(mv)) : String("null"));
  j.raw("power", p.done());
  if (timerAction[0]) {
    Json t;
    t.str("action", timerAction);
    t.num("in_s", (timerDelayMs - min<uint32_t>(timerDelayMs, millis() - timerSetMs)) / 1000);
    j.raw("timer", t.done());
  } else {
    j.raw("timer", "null");
  }
  Json e;
  taskENTER_CRITICAL(&pubMux);
  const ErrorLog log = errors;
  taskEXIT_CRITICAL(&pubMux);
  e.num("count", log.count);
  e.str("last", log.count ? log.last[(log.count - 1) % 10] : "");
  j.raw("errors", e.done());
  j.num("heap", ESP.getFreeHeap());
  j.num("heap_min", ESP.getMinFreeHeap());
  return j.done();
}

static String fail(const char *why) { return String("{\"ok\":false,\"error\":\"") + why + "\"}"; }

static void request(Cmd cmd, const char *reason, bool keepWant = false) {
  Request rq = {cmd, keepWant, ""};
  strlcpy(rq.reason, reason, sizeof(rq.reason));
  xQueueSend(requests, &rq, pdMS_TO_TICKS(1000));
}

// Waits up to 1.5 s for the recorder task to reach one of two states, so the answer already shows the result.
static String settled(State wanted, State alsoFine) {
  for (int i = 0; i < 150; i++) {
    const State now = snapshot().state;
    if (now == wanted || now == alsoFine) break;
    delay(10);
  }
  return statusJson();
}

static bool lockCard() {
  cardWanted = true;
  const bool ok = xSemaphoreTake(cardMutex, pdMS_TO_TICKS(15000)) == pdTRUE;
  cardWanted = false;
  return ok;
}

static bool removeTree(const String &path) {
  File dir = SD.open(path);
  if (!dir || !dir.isDirectory()) return false;
  for (File f = dir.openNextFile(); f; f = dir.openNextFile()) {
    const String child = path + "/" + f.name();
    const bool isDir = f.isDirectory();
    f.close();
    if (isDir ? !removeTree(child) : !SD.remove(child)) return false;
  }
  dir.close();
  return SD.rmdir(path);
}

// Recordings, newest first, 20 per answer. from = how many of the newest to skip.
// Sorted by number: the order inside the folder is useless once folders have been deleted and their places reused.
static String listRecordings(unsigned from, const Shared &s) {
  static constexpr unsigned PAGE = 20, MOST = 4096;   // ponytail: lists the 4096 lowest-placed folders at most
  static uint32_t *numbers = static_cast<uint32_t *>(heap_caps_malloc(MOST * sizeof(uint32_t), MALLOC_CAP_SPIRAM));
  unsigned total = 0;
  File root = SD.open("/rec");
  for (File d = root ? root.openNextFile() : File(); d && numbers && total < MOST; d = root.openNextFile()) {
    if (d.isDirectory() && strlen(d.name()) == 6) numbers[total++] = atoi(d.name());
  }
  root.close();
  std::sort(numbers, numbers + total, [](uint32_t a, uint32_t b) { return a > b; });

  String out = "{\"ok\":true,\"recs\":[";
  unsigned listed = 0;
  for (unsigned i = from; i < total && listed < PAGE; i++, listed++) {
    char dir[16], item[96];
    const unsigned n = numbers[i];
    snprintf(dir, sizeof(dir), "/rec/%06u", n);
    // The running recording is known from memory; its journal belongs to the recorder task.
    const bool active = s.state != State::Idle && n == s.recN;
    unsigned segs = UINT_MAX, kb = 0;
    bool done = false;
    if (active) {
      segs = s.seg + 1;
      kb = (s.samples * 2 + static_cast<uint64_t>(segs) * WAV_HEADER) >> 10;
    } else {
      done = journalDone(dir, &segs, &kb);
    }
    if (segs == UINT_MAX) {     // not finished yet, or finished by an older build: count the files
      uint64_t bytes = 0;
      segs = 0;
      File d = SD.open(dir);
      for (File f = d ? d.openNextFile() : File(); f; f = d.openNextFile()) {
        if (String(f.name()).endsWith(".wav")) {
          segs++;
          bytes += f.size();
        }
      }
      kb = bytes >> 10;
    }
    snprintf(item, sizeof(item), "%s{\"n\":%u,\"segs\":%u,\"kb\":%u,\"done\":%s}", listed ? "," : "", n, segs, kb, done ? "true" : "false");
    out += item;
  }
  out += "],\"next\":";
  out += from + listed < total ? String(from + listed) : String("null");
  return out + "}";
}

// Segments of one recording, 20 per answer, straight from its journal.
static String listSegments(unsigned n, unsigned from) {
  char dir[16], path[48], line[400], v[72], id[28] = "";
  uint8_t hashed[MAX_SEGMENTS / 8 + 1] = {};
  snprintf(dir, sizeof(dir), "/rec/%06u", n);
  snprintf(path, sizeof(path), MOUNT "%s/log.txt", dir);
  FILE *in = fopen(path, "r");
  if (!in) return "{\"ok\":false,\"error\":\"unknown_recording\"}";
  bool ended = false, done = false, recovered = false;
  while (nextLine(in, line, sizeof(line))) {
    if (is(line, "REC")) field(line, "id", id, sizeof(id));
    else if (is(line, "HASH")) mark(hashed, num(line, "n"));
    else if (is(line, "END")) ended = true;
    else if (is(line, "DONE")) done = true;
    else if (is(line, "RECOVERED")) recovered = true;
  }
  rewind(in);
  String out = String("{\"ok\":true,\"n\":") + n + ",\"id\":\"" + id + "\",\"state\":\"" +
               (!ended ? "open" : !done ? "stopped" : recovered ? "recovered" : "closed") + "\",\"segs\":[";
  unsigned index = 0, listed = 0;
  bool more = false;
  while (nextLine(in, line, sizeof(line))) {
    if (!is(line, "SEG") && !is(line, "RECOVERED")) continue;
    if (index++ < from) continue;
    if (listed == 20) {
      more = true;
      break;
    }
    char end[16], item[128], marker[40];
    const unsigned seg = num(line, "n");
    field(line, "sha256", v, sizeof(v));
    field(line, "end", end, sizeof(end));
    snprintf(marker, sizeof(marker), "%s/%04u.ok", dir, seg);
    const bool ok = SD.exists(marker);
    snprintf(marker, sizeof(marker), "%s/%04u.okp", dir, seg);
    snprintf(item, sizeof(item), "%s{\"n\":%u,\"s\":%lld,\"end\":\"%s\",\"sha\":%s,\"ok\":%s,\"okp\":%s}", listed ? "," : "", seg,
             static_cast<long long>(num(line, "samples")), end, strlen(v) == 64 || inSet(hashed, seg) ? "true" : "false",
             ok ? "true" : "false", SD.exists(marker) ? "true" : "false");
    out += item;
    listed++;
  }
  fclose(in);
  out += "],\"next\":";
  out += more ? String(from + listed) : String("null");
  return out + "}";
}

static void powerDown() {
  usbLine("", "Sleeping. Safe to unplug.");
  Serial.flush();
  lockCard();
  SD.end();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(200);
  if (wakeAfterS) esp_sleep_enable_timer_wakeup(static_cast<uint64_t>(wakeAfterS) * 1000000);
  esp_deep_sleep_start();
}

enum class Src { Usb, Ble, Web };

static String command(const String &c, Src src) {
  const Shared s = snapshot();
  const char *by = src == Src::Usb ? "usb" : src == Src::Ble ? "ble" : "web";
  char reason[16];
  if (c == "STATUS") return statusJson();
  if (c == "REC START") {
    if (!s.card) return fail("no_card");
    request(Cmd::Start, by);
    for (int i = 0; i < 300 && snapshot().state != State::Recording; i++) delay(10);
    return statusJson();
  }
  if (c == "REC STOP") {
    snprintf(reason, sizeof(reason), "stop_%s", by);
    request(Cmd::Stop, reason);
    return settled(State::Idle, State::Idle);
  }
  if (c == "REC PAUSE") {
    if (s.state != State::Recording) return fail("not_recording");
    request(Cmd::Pause, by);
    return settled(State::Paused, State::Idle);
  }
  if (c == "REC RESUME") {
    if (s.state != State::Paused) return fail("not_paused");
    request(Cmd::Resume, by);
    return settled(State::Recording, State::Idle);
  }
  if (c == "SLEEP" || c.startsWith("SLEEP ") || c == "SHUTDOWN") {
    // ponytail: until the hub exists SHUTDOWN only waits for the recordings to be finished (phase 3 adds the sync window)
    const long wake = c.startsWith("SLEEP ") ? c.substring(6).toInt() : 0;
    if (wake < 0 || wake > 7 * 86400) return fail("bad_sleep");
    const bool hadTimer = timerAction[0];
    timerAction[0] = 0;
    wakeAfterS = wake;
    sleepSinceMs = millis();
    sleepMode = c[1] == 'L' ? 1 : 2;
    request(Cmd::Stop, sleepMode == 1 ? "sleep" : "shutdown");
    return String("{\"ok\":true,\"state\":\"going_to_sleep\",\"timer_cleared\":") + (hadTimer ? "true}" : "false}");
  }
  if (c == "REBOOT") {
    timerAction[0] = 0;
    sleepMode = 3;
    request(Cmd::Stop, "reboot", true);   // comes back the way it was: the remembered wish to record stays
    return "{\"ok\":true,\"state\":\"rebooting\"}";
  }
  if (c == "TIMER OFF") {
    timerAction[0] = 0;
    return statusJson();
  }
  if (c.startsWith("TIMER ")) {
    char action[10] = "";
    unsigned seconds = 0;
    if (sscanf(c.c_str(), "TIMER %9s %u", action, &seconds) != 2 || !seconds || seconds > 7 * 86400 ||
        (strcmp(action, "stop") && strcmp(action, "sleep") && strcmp(action, "shutdown") && strcmp(action, "start"))) {
      return fail("bad_timer");
    }
    strlcpy(timerAction, action, sizeof(timerAction));
    timerSetMs = millis();
    timerDelayMs = seconds * 1000;
    return statusJson();
  }
  if (c.startsWith("TIME ")) {
    const long long unix = atoll(c.c_str() + 5);
    if (unix < VALID_TIME || unix > 4102444800LL) return fail("bad_time");
    setenv("TZ", TIME_ZONE, 1);
    tzset();
    const timeval tv = {static_cast<time_t>(unix), 0};
    settimeofday(&tv, nullptr);
    clockWasSet(src == Src::Ble ? "phone" : by);
    return statusJson();
  }
  if (c == "AUTOSTART ON" || c == "AUTOSTART OFF") {
    autostart = c.endsWith("ON");
    Preferences prefs;
    prefs.begin("ar1", false);
    prefs.putBool("autostart", autostart);
    prefs.end();
    return statusJson();
  }
  if (c == "ERRORS") {
    taskENTER_CRITICAL(&pubMux);
    const ErrorLog log = errors;
    taskEXIT_CRITICAL(&pubMux);
    String out = "{\"ok\":true,\"count\":" + String(log.count) + ",\"sd_errors\":" + String(s.sdErrors) + ",\"last\":[";
    const uint32_t shown = min<uint32_t>(log.count, 10);
    for (uint32_t i = 0; i < shown; i++) {
      out += i ? ",\"" : "\"";
      out += log.last[(log.count - shown + i) % 10];
      out += '"';
    }
    return out + "]}";
  }
  if (c == "DF") {
    Json j;
    j.flag("ok", s.card);
    j.num("total_mb", s.totalMb);
    j.num("free_mb", s.freeMb);
    return j.done();
  }
  if (c == "LS" || c.startsWith("LS ")) {
    if (!s.card) return fail("no_card");
    if (!lockCard()) return fail("busy");
    const String out = listRecordings(c.length() > 3 ? c.substring(3).toInt() : 0, s);
    xSemaphoreGive(cardMutex);
    return out;
  }
  if (c.startsWith("SEGS ")) {
    unsigned n = 0, from = 0;
    if (sscanf(c.c_str(), "SEGS %u %u", &n, &from) < 1 || n > 999999) return fail("bad_number");
    if (!s.card) return fail("no_card");
    if (!lockCard()) return fail("busy");
    const String out = listSegments(n, from);
    xSemaphoreGive(cardMutex);
    return out;
  }
  if (c.startsWith("WIFI ")) {
    const String answer = RecorderWifi::command(c);
    return answer == "{\"ok\":true}" ? statusJson() : answer;
  }
  if (c == "HTTP_AUTH") return RecorderWifi::webLogin();   // over USB this goes out as AR_SECRET, see appCommand
  if (src != Src::Usb) return fail("unknown_command");

  // ---- only with the recorder in hand: pairing data and bench commands ----
  if (c == "BLE_KEY") return RecorderBle::pairingJson();
  if (c == "BLE_KEY NEW") {
    RecorderBle::newKey();
    return "{\"ok\":true}";
  }
  if (c.startsWith("SEGLEN ")) {
    const int seconds = c.substring(7).toInt();
    if (seconds < 2 || seconds > 3600) return fail("bad_seglen");
    segLenSeconds = seconds;
    return "{\"ok\":true,\"seglen\":" + String(seconds) + "}";
  }
  if (c.startsWith("MINFREE ")) {
    const long mb = c.substring(8).toInt();
    if (mb < static_cast<long>(MIN_FREE_MB) || mb > 4000000) return fail("bad_minfree");
    minFreeMb = mb;
    return "{\"ok\":true,\"minfree\":" + String(mb) + "}";
  }
  if (c == "FAULT sd") {
    if (s.state != State::Recording) return fail("not_recording");
    faultSd = true;
    return "{\"ok\":true}";
  }
  if (c == "PINS") {   // bench view of the power board wiring: levels of D1..D5 and what D0 does within 200 ms
    static const int pins[] = {2, 3, 4, 5, 6};
    Json j;
    j.flag("ok", true);
    for (int i = 0; i < 5; i++) {
      pinMode(pins[i], INPUT_PULLUP);
      char key[4] = {'d', static_cast<char>('1' + i), 0};
      j.num(key, digitalRead(pins[i]));
    }
    uint32_t lo = UINT32_MAX, hi = 0, sum = 0;
    for (int i = 0; i < 200; i++) {
      const uint32_t mv = analogReadMilliVolts(PIN_BAT);
      lo = min(lo, mv), hi = max(hi, mv), sum += mv;
      delay(1);
    }
    j.num("d0_min_mv", lo);
    j.num("d0_avg_mv", sum / 200);
    j.num("d0_max_mv", hi);
    j.num("pgood_low_pct", pinTicks ? pgoodLowTicks * 100.0 / pinTicks : 0, 1);   // since the last PINS
    j.num("chg_low_pct", pinTicks ? chgLowTicks * 100.0 / pinTicks : 0, 1);
    j.num("ticks", pinTicks);
    pgoodLowTicks = chgLowTicks = pinTicks = 0;
    return j.done();
  }
  if (c == "SELFTEST") {
    uint8_t h[WAV_HEADER], digest[32];
    char hex[65], v[16];
    mbedtls_md(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), reinterpret_cast<const uint8_t *>("abc"), 3, digest);
    toHex(digest, 32, hex);
    const bool sha = !strcmp(hex, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    wavHeader(h, 640000);
    uint32_t riff, data;
    memcpy(&riff, h + 4, 4);
    memcpy(&data, h + 508, 4);
    const bool wave = !memcmp(h, "RIFF", 4) && !memcmp(h + 504, "data", 4) && riff == 640504 && data == 640000;
    const char *line = "SEG n=7 t_ms=4200000 rssi=-71 end=full";
    const bool parse = num(line, "n") == 7 && num(line, "rssi") == -71 && field(line, "end", v, sizeof(v)) &&
                       !strcmp(v, "full") && !field(line, "ms", v, sizeof(v)) && is(line, "SEG") && !is(line, "SE");
    const bool ble = RecorderBle::selfTest();
    const bool bat = batPercent(3000) == 0 && batPercent(3300) == 0 && batPercent(3950) == 72 &&
                     batPercent(4200) == 100 && batPercent(4400) == 100;
    Json j;
    j.flag("ok", sha && wave && parse && ble && bat);
    j.flag("sha256", sha);
    j.flag("wav_header", wave);
    j.flag("journal_parser", parse);
    j.flag("ble_crypto", ble);
    j.flag("bat_curve", bat);
    return j.done();
  }
  if (c.startsWith("RM ")) {
    const String path = c.substring(3);
    if (s.state != State::Idle || !s.card) return fail("busy_or_no_card");
    if (!path.startsWith("/rec/") || path.indexOf("..") >= 0 || path.indexOf('/', 5) >= 0 || path.length() < 6) return fail("bad_path");
    if (!lockCard()) return fail("busy");
    const bool ok = snapshot().state == State::Idle && removeTree(path);
    xSemaphoreGive(cardMutex);
    finalized = false;             // the recorder task takes another look at the card and counts the free space again
    return ok ? "{\"ok\":true}" : fail("not_removed");
  }
  if (c == "FORMAT YES") {
    if (s.card) return fail("card_is_readable_not_formatting");
    xSemaphoreTake(cardMutex, portMAX_DELAY);
    const bool ok = !snapshot().card && mountCard(true);
    SD.end();                      // the recorder task mounts it again and takes over
    xSemaphoreGive(cardMutex);
    return ok ? "{\"ok\":true}" : fail("format_failed");
  }
  return fail("unknown_command");
}

// Raw USB transfers for the PC tools; they answer with lines of their own instead of one JSON object.
static bool usbTransfer(const String &c) {
  if (!c.startsWith("DIR ") && !c.startsWith("GET ")) return false;
  const Shared s = snapshot();
  if (!s.card || !ioBlock || (c[0] == 'G' && s.state != State::Idle) || !lockCard()) {
    usbLine("AR_ERROR ", "busy_or_no_card");
    return true;
  }
  if (c[0] == 'D') {
    File dir = SD.open(c.substring(4));
    char item[96];
    for (File f = dir ? dir.openNextFile() : File(); f; f = dir.openNextFile()) {
      snprintf(item, sizeof(item), "%s %u%s", f.name(), static_cast<unsigned>(f.size()), f.isDirectory() ? " dir" : "");
      usbLine("AR_DIR ", item);
    }
    usbLine("AR_DIR_DONE", "");
  } else {
    // "GET <offset> <length> <path>": one block per request with a CRC, because bulk output
    // over the USB serial port loses bytes now and then; the PC repeats a damaged block.
    unsigned offset = 0, length = 0;
    int pathAt = 0;
    sscanf(c.c_str(), "GET %u %u %n", &offset, &length, &pathAt);
    File f = pathAt ? SD.open(c.substring(pathAt)) : File();
    if (!f || f.isDirectory() || !f.seek(offset)) {
      usbLine("AR_ERROR ", "not_found");
    } else {
      const int got = f.read(ioBlock, min<size_t>(length, IO_BLOCK));
      Serial.setTxTimeoutMs(2000);
      Serial.printf("AR_BLOCK %u %d %u\n", static_cast<unsigned>(f.size()), got,
                    static_cast<unsigned>(esp_rom_crc32_le(0, ioBlock, got)));
      Serial.write(ioBlock, got);
      Serial.flush();
      Serial.setTxTimeoutMs(1000);
    }
  }
  xSemaphoreGive(cardMutex);
  return true;
}

bool appCommand(const String &c) {
  if (usbTransfer(c)) return true;
  const String answer = command(c, Src::Usb);
  if (answer == fail("unknown_command")) return false;
  // Credentials get a prefix of their own; the PC tools never print or log such lines.
  usbLine(c == "HTTP_AUTH" || c == "BLE_KEY" ? "AR_SECRET " : "AR ", answer.c_str());
  return true;
}

String bleCommand(const String &line) { return command(line, Src::Ble); }

static void runTimer() {
  if (!timerAction[0] || millis() - timerSetMs < timerDelayMs) return;
  char action[10];
  strlcpy(action, timerAction, sizeof(action));
  timerAction[0] = 0;
  usbLine("timer: ", action);
  if (!strcmp(action, "start")) request(Cmd::Start, "timer");
  else if (!strcmp(action, "stop")) request(Cmd::Stop, "timer");
  else command(!strcmp(action, "sleep") ? "SLEEP" : "SHUTDOWN", Src::Usb);
}

void setup() {
  Serial.setTxBufferSize(4096);
  Serial.begin(115200);
  Serial.setTxTimeoutMs(1000);    // answers wait for room; journal lines never wait (printEcho)
  echo = xMessageBufferCreate(2048);
  delay(1000);

  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  snprintf(deviceId, sizeof(deviceId), "%02x%02x%02x", mac[3], mac[4], mac[5]);
  Preferences prefs;
  prefs.begin("ar1", false);
  bootCount = prefs.getUInt("boot", 0) + 1;
  prefs.putUInt("boot", bootCount);
  autostart = prefs.getBool("autostart", true);
  // Power-on: AUTOSTART decides. Any other restart (watchdog, crash, brown-out, reboot, wake-up):
  // what was last asked for by command, so a stopped or paused recorder stays off.
  wantRec = esp_reset_reason() == ESP_RST_POWERON ? autostart : prefs.getBool("want", false);
  prefs.putBool("want", wantRec);
  prefs.end();
  pinMode(PIN_PGOOD, INPUT_PULLUP);   // open drain on the charger board; unconnected reads "no power"
  pinMode(PIN_CHG, INPUT_PULLUP);
  setenv("TZ", TIME_ZONE, 1);
  tzset();
  if (unixNow()) strlcpy(timeSrc, "rtc", sizeof(timeSrc));   // the clock ran on through the restart, no source yet in this run
  Serial.printf("\n=== Recorder %s dev=%s boot=%u reset=%s record=%d ===\n", FW_VERSION, deviceId,
                static_cast<unsigned>(bootCount), resetReasonName(), wantRec);

  requests = xQueueCreate(4, sizeof(Request));
  cardMutex = xSemaphoreCreateMutex();
  ioBlock = static_cast<uint8_t *>(heap_caps_malloc(IO_BLOCK, MALLOC_CAP_SPIRAM));
  uint8_t *ring = static_cast<uint8_t *>(heap_caps_malloc(AUDIO_RING_BYTES, MALLOC_CAP_SPIRAM));
  if (ring) audioRing = xStreamBufferCreateStatic(AUDIO_RING_BYTES, 1, ring, &audioRingState);
  micReady = audioRing && setupMic();
  if (!micReady) noteError("mic_failed");

  esp_task_wdt_init(15, true);   // a stuck recorder task restarts the device and with it the recording
  if (micReady) xTaskCreatePinnedToCore(audioTaskFn, "audio", 4096, nullptr, 10, nullptr, 1);
  xTaskCreatePinnedToCore(recorderTask, "recorder", 12288, nullptr, 5, nullptr, 1);

  sntp_set_time_sync_notification_cb([](struct timeval *) { clockWasSet("ntp"); });
  RecorderWifi::begin(deviceId);
  RecorderBle::begin(deviceId);
  server.on("/status.json", [] {
    if (RecorderWifi::authorize(server)) server.send(200, "application/json", statusJson());
  });
}

void loop() {
  RecorderWifi::service();
  if (WiFi.status() == WL_CONNECTED) {
    if (!serverRunning) {
      serverRunning = true;
      server.begin();
      // No internet behind the parental lock, but the router itself may answer NTP.
      static char ntpHost[16];
      strlcpy(ntpHost, WiFi.gatewayIP().toString().c_str(), sizeof(ntpHost));
      configTzTime(TIME_ZONE, ntpHost);
    }
    server.handleClient();
  }
  RecorderBle::service();
  printEcho();
  readPower();
  runTimer();
  if (sleepMode) {
    const Shared s = snapshot();
    // SLEEP and REBOOT go at once; SHUTDOWN first lets the recordings be finished (five minutes at most).
    const bool ready = sleepMode != 2 || s.finalized || !s.card || millis() - sleepSinceMs > 300000;
    if (s.state == State::Idle && !uxQueueMessagesWaiting(requests) && ready) {
      if (sleepMode == 3) {
        lockCard();          // nothing is being written while the chip restarts
        Serial.flush();
        ESP.restart();
      }
      powerDown();
    }
  }
  delay(5);
}
