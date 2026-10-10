// Recorder DV2.0 walk test: after power-on it records video (MJPEG AVI) and audio (WAV)
// to the microSD at the same time, finalizes both files after REC_SECONDS and powers down.
#include <Arduino.h>
#include <FS.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>
#include <fcntl.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include "driver/i2s.h"
#include "esp_camera.h"
#include "esp_heap_caps.h"
#include "esp_rom_crc.h"
#include "esp_sleep.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/stream_buffer.h"
#include "wifi_setup.h"

#include <SD.h>
#include <SPI.h>

#define CARD SD
#define FW_VERSION "DV2.0"
#define MOUNT "/sd"

// ---- knobs ----
// Measured 05.10.2026 on this board (OV3660, 64 GB card on SPI) with audio running, static indoor scene:
//   VGA 27 fps 16 KB/frame, SVGA 27 fps 22 KB, XGA 27 fps 34 KB, HD 17 fps 38 KB, SXGA 14 fps 52 KB,
//   UXGA 13.7 fps 70 KB. The card sustains about 1600 KB/s; detailed outdoor scenes need 2-3x the bytes.
static constexpr uint32_t REC_SECONDS = 600;        // then finalize, unmount and power down
static constexpr framesize_t FRAME_SIZE = FRAMESIZE_XGA;   // 1024x768, the sensor's full view without scaling
static constexpr int JPEG_QUALITY = 12;             // 0..63, lower = better and larger
static constexpr float TARGET_FPS = 10.0f;          // 0 = every frame the sensor delivers
static constexpr int AUDIO_GAIN_SHIFT = 2;          // x4, saturating
static constexpr uint32_t SYNC_MS = 2000;           // at most this much is lost if power is cut
static constexpr uint32_t SD_SPI_HZ = 25000000;

static constexpr uint32_t SAMPLE_RATE = 16000;
static constexpr size_t AUDIO_RING_BYTES = 512 * 1024;   // 16 s between microphone and card
static constexpr size_t AVI_HEADER = 1024, WAV_HEADER = 512;
static constexpr uint32_t AVI_MAX_BYTES = 1900u * 1024 * 1024;

SET_LOOP_TASK_STACK_SIZE(16 * 1024);

// Collects writes into whole blocks so the card only ever sees sector-aligned multi-sector writes.
struct BufFile {
  int fd = -1;
  uint8_t *buf = nullptr;
  size_t cap = 0, n = 0;
  uint32_t pos = 0;        // bytes accepted so far
  uint32_t maxWriteMs = 0;
  bool err = false;

  bool open(const char *path, size_t capacity) {
    if (!buf) {
      buf = static_cast<uint8_t *>(heap_caps_malloc(capacity, MALLOC_CAP_DMA));
      cap = capacity;
    }
    n = 0;
    pos = 0;
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
    pos += len;
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
    ::close(fd);
    fd = -1;
  }
};

struct FrameRef {
  uint32_t offset, bytes;
  int32_t ms;
};

struct Recording {
  bool active = false, videoDead = false;
  uint32_t seconds = 0, session = 0;
  int frameSize = FRAME_SIZE, quality = JPEG_QUALITY;
  float fps = TARGET_FPS;
  char dir[20] = "", reason[24] = "";
  int64_t t0Us = 0, endUs = 0, firstFrameUs = 0, lastFrameUs = 0, nextDueUs = 0;
  uint32_t width = 0, height = 0;
  uint32_t frames = 0, skipped = 0, late = 0, fbFail = 0, fbFailRun = 0;
  uint32_t moviBytes = 0, maxFrameBytes = 0, maxGapMs = 0, maxLoopMs = 0, ringHigh = 0;
  uint32_t lastSyncMs = 0, lastLogMs = 0;
};

bool serviceMode = false;
static Recording rec;
bool recordingActive() { return rec.active; }
static BufFile avi, wav;
static File sessionLog;
static FrameRef *frameRefs = nullptr;
static uint32_t frameCap = 0;

static bool cameraReady = false, micReady = false, cardReady = false;
static uint16_t sensorPid = 0;
static uint32_t bootCount = 0;
static uint32_t pendingSeconds = REC_SECONDS;   // recording that still has to be started; 0 = none
static uint8_t cardRetries = 0;
static bool sleepPending = false, fieldTest = false;
static uint32_t wakeAfterS = 0;          // only FIELDTEST sets this; 0 = sleep until power is cycled
static uint8_t ioBlock[16384];           // file transfers to the PC, never during a recording
static const char *stopRequest = nullptr;
static WebServer server(80);
static bool serverRunning = false;

static StaticStreamBuffer_t audioRingState;
static StreamBufferHandle_t audioRing = nullptr;
static TaskHandle_t audioTask = nullptr;
static volatile bool audioRun = false, audioAlive = false;
static volatile uint32_t audioSamples = 0, audioDropped = 0, audioMaxGapMs = 0, audioPeak = 0;
static volatile int64_t audioT0Us = 0, audioLastUs = 0;

static void logf(const char *fmt, ...) {
  char line[400];
  va_list args;
  va_start(args, fmt);
  vsnprintf(line, sizeof(line), fmt, args);
  va_end(args);
  Serial.println(line);
  if (sessionLog) {
    sessionLog.println(line);
    sessionLog.flush();
  }
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

static String wallClock() {
  const time_t t = time(nullptr);
  if (t < 1700000000) return "";
  char text[24];
  strftime(text, sizeof(text), "%Y-%m-%d %H:%M:%S", localtime(&t));
  return text;
}

// A 64-bit value written by the audio task can be read half-updated; read until two reads agree.
static int64_t stable(volatile int64_t &value) {
  int64_t v;
  do v = value; while (v != value);
  return v;
}

// ---- container headers ----

struct Cursor {
  uint8_t *p;
  void cc(const char *fourcc) { memcpy(p, fourcc, 4); p += 4; }
  void u32(uint32_t v) { memcpy(p, &v, 4); p += 4; }
  void u16(uint16_t v) { memcpy(p, &v, 2); p += 2; }
};

// While recording the sizes are 0 ("until end of file"), so a file cut off by a power loss still plays.
static void aviHeader(uint8_t *h, uint32_t width, uint32_t height, float fps, uint32_t frames,
                      uint32_t moviBytes, uint32_t indexBytes, uint32_t maxFrame, bool final) {
  memset(h, 0, AVI_HEADER);
  const uint32_t usPerFrame = fps > 0 ? static_cast<uint32_t>(1e6f / fps + 0.5f) : 100000;
  Cursor b{h};
  b.cc("RIFF"); b.u32(final ? AVI_HEADER - 8 + moviBytes + 8 + indexBytes : 0); b.cc("AVI ");
  b.cc("LIST"); b.u32(192); b.cc("hdrl");
  b.cc("avih"); b.u32(56);
  b.u32(usPerFrame); b.u32(static_cast<uint32_t>(maxFrame * 1e6f / usPerFrame)); b.u32(0); b.u32(final ? 0x10 : 0);
  b.u32(frames); b.u32(0); b.u32(1); b.u32(maxFrame); b.u32(width); b.u32(height);
  b.u32(0); b.u32(0); b.u32(0); b.u32(0);
  b.cc("LIST"); b.u32(116); b.cc("strl");
  b.cc("strh"); b.u32(56);
  b.cc("vids"); b.cc("MJPG"); b.u32(0); b.u16(0); b.u16(0); b.u32(0);
  b.u32(usPerFrame); b.u32(1000000); b.u32(0); b.u32(frames); b.u32(maxFrame); b.u32(0xFFFFFFFF); b.u32(0);
  b.u16(0); b.u16(0); b.u16(width); b.u16(height);
  b.cc("strf"); b.u32(40);
  b.u32(40); b.u32(width); b.u32(height); b.u16(1); b.u16(24); b.cc("MJPG"); b.u32(width * height * 3);
  b.u32(0); b.u32(0); b.u32(0); b.u32(0);
  b.cc("JUNK"); b.u32(AVI_HEADER - 212 - 8 - 12);   // pads the header so frame data starts sector-aligned
  b.p = h + AVI_HEADER - 12;
  b.cc("LIST"); b.u32(final ? 4 + moviBytes : 0); b.cc("movi");
}

static void wavHeader(uint8_t *h, uint32_t riffBytes, uint32_t dataBytes) {
  memset(h, 0, WAV_HEADER);
  Cursor b{h};
  b.cc("RIFF"); b.u32(riffBytes); b.cc("WAVE");
  b.cc("fmt "); b.u32(16); b.u16(1); b.u16(1); b.u32(SAMPLE_RATE); b.u32(SAMPLE_RATE * 2); b.u16(2); b.u16(16);
  b.cc("JUNK"); b.u32(WAV_HEADER - 36 - 8 - 8);
  b.p = h + WAV_HEADER - 8;
  b.cc("data"); b.u32(dataBytes);
}

// ---- hardware ----

static void setupCamera() {
  camera_config_t c = {};
  c.ledc_channel = LEDC_CHANNEL_0;
  c.ledc_timer = LEDC_TIMER_0;
  c.pin_d0 = 15; c.pin_d1 = 17; c.pin_d2 = 18; c.pin_d3 = 16;
  c.pin_d4 = 14; c.pin_d5 = 12; c.pin_d6 = 11; c.pin_d7 = 48;
  c.pin_xclk = 10; c.pin_pclk = 13; c.pin_vsync = 38; c.pin_href = 47;
  c.pin_sccb_sda = 40; c.pin_sccb_scl = 39;
  c.pin_pwdn = -1; c.pin_reset = -1;
  c.xclk_freq_hz = 20000000;
  c.pixel_format = PIXFORMAT_JPEG;
  c.frame_size = FRAMESIZE_UXGA;   // allocates large frame buffers; the working size is set per recording
  c.jpeg_quality = 10;
  c.fb_count = 3;
  c.fb_location = CAMERA_FB_IN_PSRAM;
  c.grab_mode = CAMERA_GRAB_WHEN_EMPTY;

  const esp_err_t err = esp_camera_init(&c);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", err);
    return;
  }
  sensor_t *s = esp_camera_sensor_get();
  sensorPid = s->id.PID;
  if (sensorPid == OV3660_PID) {
    s->set_vflip(s, 1);
    s->set_brightness(s, 1);
    s->set_saturation(s, -2);
  }
  cameraReady = true;
}

// Same driver settings as the Arduino I2S library in PDM_MONO_MODE, which DV1.2 proved on this board.
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

static void reportCard() {
  Serial.printf("Card: %s type=%d size_mb=%u\n", cardReady ? "mounted" : "NOT MOUNTED",
                cardReady ? CARD.cardType() : 0, cardReady ? static_cast<unsigned>(CARD.cardSize() >> 20) : 0);
}

// ---- audio ----

static void audioTaskFn(void *) {
  static int16_t block[512];
  const TickType_t wait = pdMS_TO_TICKS(500);   // finite, so a silent microphone cannot keep recStop waiting
  size_t got = 0;
  // Empty what piled up while nobody was reading, then drop the microphone's start-up transient.
  for (int i = 0; i < 64 && audioRun; i++) {
    const int64_t t = esp_timer_get_time();
    i2s_read(I2S_NUM_0, block, sizeof(block), &got, wait);
    if (esp_timer_get_time() - t > 5000) break;
  }
  for (int i = 0; i < 15 && audioRun; i++) i2s_read(I2S_NUM_0, block, sizeof(block), &got, wait);
  int32_t sum = 0;
  for (int16_t v : block) sum += v;
  int32_t offsetQ8 = (sum / 512) * 256;         // the microphone sits on a constant offset of about 1400

  int64_t last = esp_timer_get_time();
  audioT0Us = last;
  audioLastUs = last;
  while (audioRun) {
    i2s_read(I2S_NUM_0, block, sizeof(block), &got, wait);
    if (!got) continue;
    const int64_t now = esp_timer_get_time();
    const uint32_t gapMs = (now - last) / 1000;
    if (gapMs > audioMaxGapMs) audioMaxGapMs = gapMs;
    last = now;

    const size_t samples = got / 2;
    uint32_t peak = audioPeak;
    for (size_t i = 0; i < samples; i++) {
      offsetQ8 += (block[i] * 256 - offsetQ8) >> 10;   // follows the offset slowly (64 ms), speech is untouched
      const int32_t v = constrain((block[i] - (offsetQ8 >> 8)) << AUDIO_GAIN_SHIFT, -32768, 32767);
      block[i] = v;
      if (static_cast<uint32_t>(abs(v)) > peak) peak = abs(v);
    }
    audioPeak = peak;
    // Whole blocks only, so a full ring never shifts the sample alignment.
    if (xStreamBufferSpacesAvailable(audioRing) >= got) xStreamBufferSend(audioRing, block, got, 0);
    else audioDropped += samples;
    audioSamples += samples;
    audioLastUs = now;
  }
  audioAlive = false;
  vTaskDelete(nullptr);
}

static void drainAudio() {
  static uint8_t chunk[4096];
  const uint32_t waiting = xStreamBufferBytesAvailable(audioRing);
  if (waiting > rec.ringHigh) rec.ringHigh = waiting;
  size_t got;
  while ((got = xStreamBufferReceive(audioRing, chunk, sizeof(chunk), 0)) > 0) wav.write(chunk, got);
}

// ---- status ----

struct Json {
  String s = "{";
  void raw(const char *key, const String &value) {
    if (s.length() > 1) s += ',';
    s += '"'; s += key; s += "\":"; s += value;
  }
  void num(const char *key, double value, int decimals = 0) {
    raw(key, decimals ? String(value, decimals) : String(static_cast<long long>(llround(value))));
  }
  void str(const char *key, const String &value) { raw(key, "\"" + value + "\""); }
  String done() { return s + "}"; }
};

static String statusJson() {
  const int64_t now = esp_timer_get_time();
  const int64_t audioT0 = stable(audioT0Us), audioLast = stable(audioLastUs);
  const uint32_t samples = audioSamples;
  Json j;
  j.str("fw", FW_VERSION);
  j.num("boot", bootCount);
  j.str("reset", resetReasonName());
  j.str("state", rec.active ? "recording" : cardReady ? "idle" : "no_card");
  j.str("time", wallClock());
  j.num("session", rec.session);
  j.str("dir", rec.dir);
  j.str("stop_reason", rec.reason);
  j.num("elapsed_s", rec.t0Us ? ((rec.active ? now : rec.endUs) - rec.t0Us) / 1e6 : 0, 1);
  j.num("planned_s", rec.seconds);
  j.num("sensor_pid", sensorPid);
  j.num("width", rec.width);
  j.num("height", rec.height);
  j.num("quality", rec.quality);
  j.num("fps_target", rec.fps, 2);
  j.num("fps_avg", rec.frames > 1 ? (rec.frames - 1) * 1e6 / (rec.lastFrameUs - rec.firstFrameUs) : 0, 3);
  j.num("frames", rec.frames);
  j.num("frames_skipped", rec.skipped);
  j.num("frames_late", rec.late);
  j.num("frame_gap_max_ms", rec.maxGapMs);
  j.num("fb_fail", rec.fbFail);
  j.num("video_dead", rec.videoDead);
  j.num("video_bytes", rec.moviBytes);
  j.num("frame_max_bytes", rec.maxFrameBytes);
  j.num("video_t0_ms", rec.frames ? (rec.firstFrameUs - rec.t0Us) / 1000.0 : 0, 1);
  j.num("video_write_max_ms", avi.maxWriteMs);
  j.num("audio_write_max_ms", wav.maxWriteMs);
  j.num("loop_max_ms", rec.maxLoopMs);
  j.num("audio_samples", samples);
  j.num("audio_s", samples / static_cast<double>(SAMPLE_RATE), 3);
  // Samples the clock says should exist minus samples received: about 0 means no gap.
  j.num("audio_missing", samples ? (audioLast - audioT0) * static_cast<double>(SAMPLE_RATE) / 1e6 - samples : 0);
  j.num("audio_dropped", audioDropped);
  j.num("audio_gap_max_ms", audioMaxGapMs);
  j.num("audio_peak", audioPeak);
  j.num("audio_t0_ms", audioT0 ? (audioT0 - rec.t0Us) / 1000.0 : 0, 1);
  j.num("ring_high_pct", rec.ringHigh * 100.0 / AUDIO_RING_BYTES, 1);
  j.num("sd_error", avi.err || wav.err);
  j.num("camera", cameraReady);
  j.num("mic", micReady);
  j.num("heap_free", ESP.getFreeHeap());
  j.num("heap_min", ESP.getMinFreeHeap());
  j.num("psram_free", ESP.getFreePsram());
  j.raw("wifi", RecorderWifi::statusJson());
  return j.done();
}

static void logProgress() {
  logf("t=%us frames=%u fps=%.2f skipped=%u late=%u gap_max=%ums fb_fail=%u video=%uKB frame_max=%uB "
       "v_write_max=%ums a_write_max=%ums loop_max=%ums audio=%.1fs a_dropped=%u a_gap_max=%ums ring_high=%uB "
       "a_t0=%dms v_t0=%dms heap=%u wifi=%d",
       static_cast<unsigned>((esp_timer_get_time() - rec.t0Us) / 1000000), rec.frames,
       rec.frames > 1 ? (rec.frames - 1) * 1e6 / (rec.lastFrameUs - rec.firstFrameUs) : 0.0, rec.skipped, rec.late,
       rec.maxGapMs, rec.fbFail, rec.moviBytes / 1024, rec.maxFrameBytes, avi.maxWriteMs, wav.maxWriteMs,
       rec.maxLoopMs, audioSamples / static_cast<double>(SAMPLE_RATE), static_cast<unsigned>(audioDropped),
       static_cast<unsigned>(audioMaxGapMs), rec.ringHigh,
       static_cast<int>((stable(audioT0Us) - rec.t0Us) / 1000), static_cast<int>((rec.firstFrameUs - rec.t0Us) / 1000),
       ESP.getFreeHeap(), WiFi.status() == WL_CONNECTED);
}

// ---- recording ----

static bool recStart(uint32_t seconds, int frameSize, int quality, float fps) {
  if (rec.active || !cardReady) return false;

  Preferences prefs;
  prefs.begin("dv2", false);
  uint32_t session = prefs.getUInt("session", 0);
  char dir[20];
  do snprintf(dir, sizeof(dir), "/rec/%04u", static_cast<unsigned>(++session)); while (CARD.exists(dir));
  prefs.putUInt("session", session);
  prefs.end();
  CARD.mkdir("/rec");
  if (!CARD.mkdir(dir)) return false;

  rec = Recording{};
  rec.session = session;
  strlcpy(rec.dir, dir, sizeof(rec.dir));
  rec.seconds = seconds;
  rec.frameSize = frameSize;
  rec.quality = quality;
  rec.fps = fps;
  rec.width = resolution[frameSize].width;
  rec.height = resolution[frameSize].height;
  stopRequest = nullptr;

  frameCap = seconds * 30 + 64;
  FrameRef *refs = static_cast<FrameRef *>(heap_caps_realloc(frameRefs, frameCap * sizeof(FrameRef), MALLOC_CAP_SPIRAM));
  if (!refs) return false;
  frameRefs = refs;

  rec.videoDead = !cameraReady;
  if (cameraReady) {
    sensor_t *s = esp_camera_sensor_get();
    s->set_framesize(s, static_cast<framesize_t>(frameSize));
    s->set_quality(s, quality);
    for (int i = 0; i < 5 && !rec.videoDead; i++) {   // frames from before the change
      camera_fb_t *fb = esp_camera_fb_get();
      if (fb) esp_camera_fb_return(fb);
      else rec.videoDead = true;
    }
  }

  char path[48];
  snprintf(path, sizeof(path), MOUNT "%s/video.avi", dir);
  bool ok = avi.open(path, 32768);
  snprintf(path, sizeof(path), MOUNT "%s/audio.wav", dir);
  ok = wav.open(path, 16384) && ok;
  if (!ok) {
    if (avi.fd >= 0) ::close(avi.fd);
    if (wav.fd >= 0) ::close(wav.fd);
    avi.fd = wav.fd = -1;
    return false;
  }
  sessionLog = CARD.open(String(dir) + "/log.txt", FILE_WRITE);

  uint8_t header[AVI_HEADER];
  aviHeader(header, rec.width, rec.height, fps, static_cast<uint32_t>(seconds * (fps > 0 ? fps : 30)), 0, 0, 0, false);
  avi.write(header, AVI_HEADER);
  wavHeader(header, 0xFFFFFFFF, 0xFFFFFFFF);
  wav.write(header, WAV_HEADER);

  xStreamBufferReset(audioRing);
  audioSamples = audioDropped = audioMaxGapMs = audioPeak = 0;
  audioT0Us = audioLastUs = 0;
  rec.t0Us = esp_timer_get_time();
  rec.lastSyncMs = rec.lastLogMs = millis();
  rec.active = true;
  if (micReady) {
    audioRun = audioAlive = true;
    if (xTaskCreatePinnedToCore(audioTaskFn, "audio", 4096, nullptr, 10, &audioTask, 1) != pdPASS) {
      audioRun = audioAlive = false;
    }
  }
  enableLoopWDT();
  logf("=== Recorder %s boot=%u reset=%s time=%s ===", FW_VERSION, static_cast<unsigned>(bootCount),
       resetReasonName(), wallClock().c_str());
  logf("recording %s: %us %ux%u quality=%d fps=%.2f sensor=0x%x camera=%d mic=%d", dir,
       static_cast<unsigned>(seconds), static_cast<unsigned>(rec.width), static_cast<unsigned>(rec.height), quality,
       fps, sensorPid, cameraReady, micReady);
  return true;
}

static void recStop(const char *reason) {
  disableLoopWDT();
  strlcpy(rec.reason, reason, sizeof(rec.reason));
  audioRun = false;
  for (int i = 0; audioAlive && i < 400; i++) delay(5);
  drainAudio();
  rec.endUs = esp_timer_get_time();

  uint8_t header[AVI_HEADER];
  const float fps = rec.frames > 1 ? (rec.frames - 1) * 1e6f / (rec.lastFrameUs - rec.firstFrameUs) : rec.fps;
  const uint32_t indexBytes = rec.frames * 16;
  uint32_t entry[4] = {0, 0x10, 0, 0};   // fourcc, keyframe flag, offset, size
  memcpy(entry, "00dc", 4);
  avi.write("idx1", 4);
  avi.write(&indexBytes, 4);
  for (uint32_t i = 0; i < rec.frames; i++) {
    entry[2] = frameRefs[i].offset;
    entry[3] = frameRefs[i].bytes;
    avi.write(entry, 16);
  }
  aviHeader(header, rec.width, rec.height, fps, rec.frames, rec.moviBytes, indexBytes, rec.maxFrameBytes, true);
  avi.finish(header, AVI_HEADER);

  const uint32_t audioBytes = wav.pos - WAV_HEADER;
  wavHeader(header, WAV_HEADER - 8 + audioBytes, audioBytes);
  wav.finish(header, WAV_HEADER);

  File csv = CARD.open(String(rec.dir) + "/frames.csv", FILE_WRITE);
  csv.println("frame,t_ms,bytes");
  for (uint32_t i = 0; i < rec.frames; i++) {
    csv.printf("%u,%d,%u\n", static_cast<unsigned>(i), static_cast<int>(frameRefs[i].ms),
               static_cast<unsigned>(frameRefs[i].bytes));
  }
  csv.close();

  rec.active = false;
  File info = CARD.open(String(rec.dir) + "/info.json", FILE_WRITE);
  info.print(statusJson());
  info.close();
  logProgress();
  logf("finished %s: reason=%s frames=%u audio=%.1fs sd_error=%d", rec.dir, reason, rec.frames,
       audioSamples / static_cast<double>(SAMPLE_RATE), avi.err || wav.err);
  sessionLog.close();
  stopRequest = nullptr;

  const int64_t leftS = rec.seconds - (rec.endUs - rec.t0Us) / 1000000;
  if (!strcmp(reason, "sd_write_error") && leftS > 5 && cardRetries < 5) {
    // The SD driver gives up for good when the card stays busy for 500 ms.
    // A fresh mount and a new session record the rest of the time.
    cardRetries++;
    CARD.end();
    cardReady = false;
    pendingSeconds = leftS;
  } else {
    sleepPending = true;
  }
}

static void recStep() {
  const int64_t stepStart = esp_timer_get_time();
  drainAudio();

  camera_fb_t *fb = rec.videoDead ? nullptr : esp_camera_fb_get();
  if (rec.videoDead) {
    delay(20);
  } else if (!fb) {
    rec.fbFail++;
    if (++rec.fbFailRun >= 2) {
      rec.videoDead = true;        // keep the audio going
      logf("ERROR: camera stopped delivering frames, continuing with audio only");
    }
  } else {
    rec.fbFailRun = 0;
    const int64_t ts = static_cast<int64_t>(fb->timestamp.tv_sec) * 1000000 + fb->timestamp.tv_usec;
    const int64_t interval = rec.fps > 0 ? static_cast<int64_t>(1e6f / rec.fps) : 0;
    if (ts < rec.t0Us) {
      // captured before this recording started
    } else if (rec.frames && ts < rec.nextDueUs - 15000) {
      rec.skipped++;
    } else {
      if (rec.frames) {
        const uint32_t gapMs = (ts - rec.lastFrameUs) / 1000;
        if (gapMs > rec.maxGapMs) rec.maxGapMs = gapMs;
        if (interval && ts - rec.lastFrameUs > interval * 3 / 2) rec.late++;
      } else {
        rec.firstFrameUs = ts;
      }
      // Stay on the nominal grid; start a new grid only after falling a whole interval behind.
      rec.nextDueUs = rec.frames && ts < rec.nextDueUs + interval ? rec.nextDueUs + interval : ts + interval;

      uint32_t chunk[2];
      memcpy(chunk, "00dc", 4);
      chunk[1] = fb->len;
      frameRefs[rec.frames] = {4 + rec.moviBytes, static_cast<uint32_t>(fb->len),
                               static_cast<int32_t>((ts - rec.t0Us) / 1000)};
      avi.write(chunk, 8);
      avi.write(fb->buf, fb->len);
      if (fb->len & 1) avi.write("", 1);
      rec.moviBytes += 8 + fb->len + (fb->len & 1);
      if (fb->len > rec.maxFrameBytes) rec.maxFrameBytes = fb->len;
      rec.lastFrameUs = ts;
      rec.frames++;
    }
    esp_camera_fb_return(fb);
  }

  const uint32_t nowMs = millis();
  if (nowMs - rec.lastSyncMs >= SYNC_MS) {
    rec.lastSyncMs = nowMs;
    drainAudio();
    avi.sync(false);
    wav.sync(false);
  }
  if (nowMs - rec.lastLogMs >= 10000) {
    rec.lastLogMs = nowMs;
    logProgress();
  }
  const uint32_t stepMs = (esp_timer_get_time() - stepStart) / 1000;
  if (stepMs > rec.maxLoopMs) rec.maxLoopMs = stepMs;

  if (stopRequest) recStop(stopRequest);
  else if (avi.err || wav.err) recStop("sd_write_error");
  else if (esp_timer_get_time() - rec.t0Us >= static_cast<int64_t>(rec.seconds) * 1000000) recStop("time");
  else if (rec.frames >= frameCap) recStop("frame_limit");
  else if (rec.moviBytes >= AVI_MAX_BYTES) recStop("size_limit");
}

static void powerDown() {
  Serial.println("Powering down. Safe to unplug.");
  CARD.end();
  if (cameraReady) esp_camera_deinit();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(200);
  if (wakeAfterS) esp_sleep_enable_timer_wakeup(static_cast<uint64_t>(wakeAfterS) * 1000000);
  esp_deep_sleep_start();
}

static void idleStep() {
  static uint32_t lastMountTry = 0;
  if (sleepPending) {
    sleepPending = false;
    if (!serviceMode || fieldTest) powerDown();
  }
  if (!cardReady && millis() - lastMountTry >= 5000) {
    lastMountTry = millis();
    cardReady = mountCard(false);
    if (cardReady) reportCard();
  }
  if (cardReady && pendingSeconds) {
    const uint32_t seconds = pendingSeconds;
    pendingSeconds = 0;
    if (!recStart(seconds, rec.frameSize, rec.quality, rec.fps)) {
      Serial.println("ERROR: recording did not start");
      if (cardRetries++ < 5) {     // mount again and retry
        CARD.end();
        cardReady = false;
        pendingSeconds = seconds;
      } else {
        sleepPending = true;
      }
    }
  }
  delay(5);
}

// ---- USB commands (bench use) ----

static void benchCard(unsigned megabytes) {
  static constexpr size_t BLOCK = 32768;
  uint8_t *block = static_cast<uint8_t *>(heap_caps_malloc(BLOCK, MALLOC_CAP_DMA));
  const int fd = block ? ::open(MOUNT "/bench.bin", O_WRONLY | O_CREAT | O_TRUNC, 0666) : -1;
  if (fd < 0) {
    free(block);
    Serial.println("DV2_ERROR bench_open");
    return;
  }
  memset(block, 0xA5, BLOCK);
  uint32_t maxMs = 0, slow = 0, lastSync = millis();
  const uint32_t start = lastSync;
  bool ok = true;
  for (unsigned i = 0; ok && i < megabytes * 32; i++) {
    uint32_t t = millis();
    ok = ::write(fd, block, BLOCK) == static_cast<ssize_t>(BLOCK);
    if (millis() - lastSync >= SYNC_MS) {
      ok = ok && ::fsync(fd) == 0;
      lastSync = millis();
    }
    t = millis() - t;
    if (t > maxMs) maxMs = t;
    if (t >= 100) slow++;
  }
  ::fsync(fd);
  ::close(fd);
  const uint32_t ms = millis() - start;
  ::unlink(MOUNT "/bench.bin");
  free(block);
  Serial.printf("DV2_BENCH ok=%d mb=%u kb_per_s=%u max_block_ms=%u blocks_over_100ms=%u\n", ok, megabytes,
                ms ? static_cast<unsigned>(megabytes * 1024000ull / ms) : 0, static_cast<unsigned>(maxMs),
                static_cast<unsigned>(slow));
}

bool appCommand(const String &c) {
  const bool cardCommand = c.startsWith("LS ") || c.startsWith("GET ") || c.startsWith("BENCH ") || c == "DF";
  if (c == "STATUS") {
    Serial.print("DV2_STATUS ");
    Serial.println(statusJson());
  } else if (c == "STOP") {
    pendingSeconds = 0;
    fieldTest = false;
    if (rec.active) stopRequest = "usb";
  } else if (c.startsWith("REC ") || c.startsWith("FIELDTEST ")) {
    unsigned seconds = 0, size = FRAME_SIZE, wake = 0;
    int quality = JPEG_QUALITY;
    float fps = TARGET_FPS;
    const bool field = c[0] == 'F';
    if (field) sscanf(c.c_str(), "FIELDTEST %u %u", &seconds, &wake);
    else sscanf(c.c_str(), "REC %u %u %d %f", &seconds, &size, &quality, &fps);
    if (!seconds || seconds > 3600 || size > FRAMESIZE_UXGA || !recStart(seconds, size, quality, fps)) {
      Serial.println("DV2_ERROR rec_not_started");
    } else if (field) {
      fieldTest = true;      // power down afterwards as on a power bank, but wake up again for the next test
      wakeAfterS = wake;
    }
  } else if (c == "FORMAT YES") {
    if (cardReady) {
      Serial.println("DV2_ERROR card_is_readable_not_formatting");
    } else {
      cardReady = mountCard(true);
      reportCard();
    }
  } else if (cardCommand && (rec.active || !cardReady)) {
    Serial.println("DV2_ERROR busy_or_no_card");
  } else if (c.startsWith("LS ")) {
    File dir = CARD.open(c.substring(3));
    for (File f = dir ? dir.openNextFile() : File(); f; f = dir.openNextFile()) {
      Serial.printf("DV2_LS %s %u%s\n", f.name(), static_cast<unsigned>(f.size()), f.isDirectory() ? " dir" : "");
    }
    Serial.println("DV2_LS_DONE");
  } else if (c.startsWith("GET ")) {
    // "GET <offset> <length> <path>": one block per request with a CRC, because bulk output
    // over the USB serial port loses bytes now and then; the PC repeats a damaged block.
    uint8_t *chunk = ioBlock;
    unsigned offset = 0, length = 0;
    int pathAt = 0;
    sscanf(c.c_str(), "GET %u %u %n", &offset, &length, &pathAt);
    File f = pathAt ? CARD.open(c.substring(pathAt)) : File();
    if (!f || f.isDirectory() || !f.seek(offset)) {
      Serial.println("DV2_ERROR not_found");
    } else {
      const int got = f.read(chunk, min<size_t>(length, sizeof(ioBlock)));
      Serial.setTxTimeoutMs(2000);
      Serial.printf("DV2_BLOCK %u %d %u\n", static_cast<unsigned>(f.size()), got,
                    static_cast<unsigned>(esp_rom_crc32_le(0, chunk, got)));
      Serial.write(chunk, got);
      Serial.flush();
      Serial.setTxTimeoutMs(100);
    }
  } else if (c.startsWith("BENCH ")) {
    benchCard(constrain(c.substring(6).toInt(), 1, 64));
  } else if (c == "DF") {
    const uint32_t t = millis();
    const unsigned total = CARD.totalBytes() >> 20, used = CARD.usedBytes() >> 20;
    Serial.printf("DV2_DF total_mb=%u used_mb=%u took_ms=%u\n", total, used, static_cast<unsigned>(millis() - t));
  } else {
    return false;
  }
  return true;
}

// ---- web ----

static void listFiles(String &html) {
  File root = CARD.open("/rec");
  for (File dir = root ? root.openNextFile() : File(); dir; dir = root.openNextFile()) {
    if (!dir.isDirectory()) continue;
    html += "<h3>Aufnahme ";
    html += dir.name();
    html += "</h3><ul>";
    for (File f = dir.openNextFile(); f; f = dir.openNextFile()) {
      html += "<li><a href='/f?p=";
      html += f.path();
      html += "'>";
      html += f.name();
      html += "</a> ";
      html += static_cast<unsigned>(f.size() / 1024);
      html += " KB</li>";
    }
    html += "</ul>";
  }
}

static void handleRoot() {
  if (!RecorderWifi::authorize(server)) return;
  String html = F("<!doctype html><meta charset=utf-8><meta name=viewport content='width=device-width,initial-scale=1'>"
                  "<meta http-equiv=refresh content=5><title>Recorder</title>"
                  "<body style='font-family:system-ui;margin:20px;line-height:1.4'><h1>Recorder " FW_VERSION "</h1>");
  if (rec.active) {
    html += "<p><b>Nimmt auf:</b> ";
    html += static_cast<unsigned>((esp_timer_get_time() - rec.t0Us) / 1000000);
    html += " von ";
    html += rec.seconds;
    html += " s, ";
    html += rec.frames;
    html += " Bilder.</p><p><a href='/stop'>Aufnahme jetzt beenden</a></p>";
  } else if (cardReady) {
    html += F("<p><b>Bereit.</b> Keine Aufnahme aktiv.</p>");
    listFiles(html);
  } else {
    html += F("<p><b>Keine lesbare SD-Karte.</b></p>");
  }
  html += F("<h3>Status</h3><pre style='white-space:pre-wrap;word-break:break-all'>");
  html += statusJson();
  html += F("</pre>");
  server.send(200, "text/html", html);
}

static void handleStop() {
  if (!RecorderWifi::authorize(server)) return;
  serviceMode = true;
  pendingSeconds = 0;
  fieldTest = false;
  if (rec.active) stopRequest = "web";
  server.sendHeader("Location", "/");
  server.send(303);
}

static void handleFile() {
  if (!RecorderWifi::authorize(server)) return;
  const String path = server.arg("p");
  if (rec.active || !cardReady) {
    server.send(409, "text/plain", "Waehrend der Aufnahme gibt es keine Downloads.");
    return;
  }
  File f = path.startsWith("/rec/") && path.indexOf("..") < 0 ? CARD.open(path) : File();
  if (!f || f.isDirectory()) {
    server.send(404, "text/plain", "Nicht gefunden.");
    return;
  }
  serviceMode = true;
  server.sendHeader("Content-Disposition", "attachment; filename=\"" + path.substring(5, 9) + "-" + f.name() + "\"");
  server.setContentLength(f.size());
  server.send(200, "application/octet-stream", "");
  WiFiClient client = server.client();   // own loop: stops reading the card when the browser goes away
  for (int got; client.connected() && (got = f.read(ioBlock, sizeof(ioBlock))) > 0;) {
    if (client.write(ioBlock, got) != static_cast<size_t>(got)) break;
  }
  f.close();
}

void setup() {
  Serial.setTxBufferSize(4096);
  Serial.begin(115200);
  delay(1000);

  Preferences prefs;
  prefs.begin("dv2", false);
  bootCount = prefs.getUInt("boot", 0) + 1;
  prefs.putUInt("boot", bootCount);
  prefs.end();
  Serial.printf("\n=== Recorder %s boot=%u reset=%s ===\n", FW_VERSION, static_cast<unsigned>(bootCount),
                resetReasonName());

  uint8_t *ring = static_cast<uint8_t *>(heap_caps_malloc(AUDIO_RING_BYTES, MALLOC_CAP_SPIRAM));
  if (ring) audioRing = xStreamBufferCreateStatic(AUDIO_RING_BYTES, 1, ring, &audioRingState);
  setupCamera();
  micReady = audioRing && setupMic();
  cardReady = mountCard(false);
  Serial.printf("Camera: %s sensor=0x%x  Mic: %s\n", cameraReady ? "ok" : "FAILED", sensorPid,
                micReady ? "ok" : "FAILED");
  reportCard();

  RecorderWifi::begin();
  server.on("/", handleRoot);
  server.on("/stop", handleStop);
  server.on("/f", handleFile);
  server.on("/status.json", [] {
    if (RecorderWifi::authorize(server)) server.send(200, "application/json", statusJson());
  });
  esp_task_wdt_init(15, true);   // a stuck recording loop restarts the device and with it the recording
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
      configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", ntpHost);
    }
    server.handleClient();
  }
  if (rec.active) recStep();
  else idleStep();
}
