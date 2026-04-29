/*
 * Internet Radio for M5StickCPlus2
 * Requires library: ESP32-audioI2S
 * https://github.com/schreibfaul1/ESP32-audioI2S/wiki
 *
 * Controls:
 *   A (single)  — next station
 *   B (single)  — previous station
 *   A (double)  — enter volume mode
 *   Volume mode: A = vol+,  B = vol-,  3s inactivity = auto-exit
 *
 * RTOS:
 *   Core 0 — audioTask  : HTTP + decode + I2S  (priority 2)
 *   Core 1 — loop()     : buttons + display     (priority 1)
 */
#include "M5StickCPlus2.h"
#include "Arduino.h"
#include "WiFi.h"
#include "Audio.h"
#include "secrets.h"
#include "stations.h"
#include "ui.h"

#define I2S_BCLK 26
#define I2S_LRC   0
#define I2S_DOUT 25

#define VOLUME_TIMEOUT_MS 3000
#define DOUBLE_CLICK_MS    350

Audio audio;

// ── Shared state (read Core 1, written Core 0 + Core 1) ──────────────────────
volatile int  currentStation = 0;
volatile int  volume         = 15;
volatile bool volumeMode     = false;
volatile bool isPlaying      = false;  // set by audioTask via audio.isRunning()
volatile bool uiDirty        = false;
char          streamTitle[128] = "";

// ── RTOS ─────────────────────────────────────────────────────────────────────
static QueueHandle_t stationQueue;

// ── Button state (Core 1 only) ────────────────────────────────────────────────
static unsigned long lastPressA    = 0;
static bool          pendingClickA = false;
static unsigned long lastActivity  = 0;

// ─────────────────────────────────────────────────────────────────────────────

void connectStation(int idx) {
    currentStation = idx;
    streamTitle[0] = '\0';
    isPlaying      = false;
    uiDirty        = true;
    xQueueOverwrite(stationQueue, &idx);
}

void handleButtons() {
    M5.update();
    const unsigned long now = millis();

    if (M5.BtnA.wasPressed()) {
        if (volumeMode) {
            int v = (int)volume + 1; if (v > 21) v = 21;
            volume = v; audio.setVolume(v);
            lastActivity = now; uiDirty = true;
        } else if (pendingClickA && (now - lastPressA < DOUBLE_CLICK_MS)) {
            pendingClickA = false;
            volumeMode    = true;
            lastActivity  = now;
            uiDirty       = true;
        } else {
            pendingClickA = true;
            lastPressA    = now;
        }
    }

    if (pendingClickA && !volumeMode && (now - lastPressA >= DOUBLE_CLICK_MS)) {
        pendingClickA = false;
        connectStation(((int)currentStation + 1) % STATION_COUNT);
    }

    if (M5.BtnB.wasPressed()) {
        if (volumeMode) {
            int v = (int)volume - 1; if (v < 0) v = 0;
            volume = v; audio.setVolume(v);
            lastActivity = now; uiDirty = true;
        } else {
            connectStation(((int)currentStation - 1 + STATION_COUNT) % STATION_COUNT);
        }
    }

    if (volumeMode && (now - lastActivity > VOLUME_TIMEOUT_MS)) {
        volumeMode = false;
        uiDirty    = true;
    }
}

// ─── Audio task — Core 0 ─────────────────────────────────────────────────────

void audioTask(void *param) {
    int  idx;
    bool prevRunning = false;

    for (;;) {
        if (xQueueReceive(stationQueue, &idx, 0) == pdTRUE) {
            // Signal "connecting" before blocking HTTP call
            isPlaying    = false;
            prevRunning  = false;
            uiDirty      = true;
            audio.connecttohost(stations[idx].url);
        }

        audio.loop();

        // audio.isRunning() is the authoritative source: true only when
        // audio frames are actually being decoded and sent to I2S.
        const bool running = audio.isRunning();
        if (running != prevRunning) {
            prevRunning = running;
            isPlaying   = running;
            uiDirty     = true;   // triggers drawUI() on Core 1
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

// ─────────────────────────────────────────────────────────────────────────────

void setup() {
    auto cfg = M5.config();
    cfg.internal_mic = false;
    M5.begin(cfg);

    initDisplay();
    drawConnecting();

    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    int dots = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        drawConnectingProgress(++dots % 5);
    }

    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume((int)volume);

    stationQueue = xQueueCreate(1, sizeof(int));
    xTaskCreatePinnedToCore(audioTask, "audio", 8192, NULL, 2, NULL, 0);

    connectStation(0);
}

void loop() {
    handleButtons();
    tickSpectrum();
    if (uiDirty) {
        uiDirty = false;
        drawUI();
    }
}

// ─── Audio callbacks (Core 0) ─────────────────────────────────────────────────

void audio_showstreamtitle(const char *info) {
    strncpy(streamTitle, info, sizeof(streamTitle) - 1);
    streamTitle[sizeof(streamTitle) - 1] = '\0';
    uiDirty = true;
}
void audio_info(const char *info)        { Serial.println(info); }
void audio_id3data(const char *info)     { Serial.print("id3: ");  Serial.println(info); }
void audio_eof_mp3(const char *info)     { Serial.print("eof: ");  Serial.println(info); }
void audio_showstation(const char *info) { Serial.print("stn: ");  Serial.println(info); }
void audio_bitrate(const char *info)     { Serial.print("bps: ");  Serial.println(info); }
void audio_commercial(const char *info)  { Serial.print("ad: ");   Serial.println(info); }
void audio_icyurl(const char *info)      { Serial.print("url: ");  Serial.println(info); }
void audio_lasthost(const char *info)    { Serial.print("host: "); Serial.println(info); }
void audio_eof_speech(const char *info)  { Serial.print("tts: ");  Serial.println(info); }
