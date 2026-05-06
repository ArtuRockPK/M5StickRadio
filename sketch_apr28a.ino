/*
 * Internet Radio for M5StickCPlus2
 * Libraries required:
 *   - ESP32-audioI2S  https://github.com/schreibfaul1/ESP32-audioI2S
 *
 * Controls:
 *   A single       — next station
 *   B click        — previous station
 *   A double       — enter volume mode
 *
 * Controls — Volume mode:
 *   A              — volume +
 *   B click        — volume -
 *   3s idle        — exit volume mode
 *
 * RTOS:
 *   Core 0 — audioTask  : HTTP + decode + I2S  (priority 2)
 *   Core 1 — loop()     : buttons + display + web server  (priority 1)
 */
#include "M5StickCPlus2.h"
#include "Arduino.h"
#include "WiFi.h"
#include "Audio.h"
#include "wifi_manager.h"
#include "station_store.h"
#include "web_server.h"
#include "ui.h"

#define I2S_BCLK 26
#define I2S_LRC   0
#define I2S_DOUT 25

#define VOLUME_TIMEOUT_MS 3000
#define DOUBLE_CLICK_MS    350

// ── Library objects ───────────────────────────────────────────────────────────
Audio audio;

// ── Shared state ─────────────────────────────────────────────────────────────
volatile int  currentStation = 0;
volatile int  volume         = 15;
volatile bool volumeMode     = false;
volatile bool isPlaying      = false;
volatile bool uiDirty        = false;
char          streamTitle[128] = "";
char          deviceIP[20]     = "";

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

void stopPlayback() {
    streamTitle[0] = '\0';
    isPlaying      = false;
    uiDirty        = true;
    int stop = -1;
    xQueueOverwrite(stationQueue, &stop);
}

// ─── Button handling ──────────────────────────────────────────────────────────

void handleButtons() {
    M5.update();
    const unsigned long now = millis();

    // ── Button A ─────────────────────────────────────────────────────────────
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
        connectStation(((int)currentStation + 1) % getTotalStationCount());
    }

    // ── Button B ─────────────────────────────────────────────────────────────

    if (M5.BtnB.wasClicked()) {
        if (volumeMode) {
            int v = (int)volume - 1; if (v < 0) v = 0;
            volume = v; audio.setVolume(v);
            lastActivity = now; uiDirty = true;
        } else {
            connectStation(((int)currentStation - 1 + getTotalStationCount()) % getTotalStationCount());
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
            isPlaying   = false;
            prevRunning = false;
            uiDirty     = true;
            if (idx >= 0) {
                StationEntry st = getStation(idx);
                audio.connecttohost(st.url);
            } else {
                audio.stopSong();
            }
        }

        audio.loop();

        const bool running = audio.isRunning();
        if (running != prevRunning) {
            prevRunning = running;
            isPlaying   = running;
            uiDirty     = true;
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

// ─────────────────────────────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);
    delay(200);

    auto cfg = M5.config();
    cfg.internal_mic = false;
    M5.begin(cfg);

    initDisplay();
    storeLoad();   // load custom stations before audio task starts

    // ── WiFi provisioning ──────────────────────────────────────────────────
    String ssid, pass;
    if (!wm_loadCreds(ssid, pass)) {
        drawAPMode();
        runWifiSetup();   // blocks, then reboots
        return;
    }

    drawConnecting();
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());

    unsigned long t = millis();
    int dots = 0;
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - t > 15000) {
            // Connection timed out — go back to setup portal
            drawAPMode();
            runWifiSetup();   // blocks, then reboots
            return;
        }
        delay(500);
        drawConnectingProgress(++dots % 5);
    }

    WiFi.localIP().toString().toCharArray(deviceIP, sizeof(deviceIP));

    // ── Start web server (station management) ─────────────────────────────
    webServerSetup();

    // ── Audio ─────────────────────────────────────────────────────────────
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume((int)volume);

    stationQueue = xQueueCreate(1, sizeof(int));
    xTaskCreatePinnedToCore(audioTask, "audio", 8192, NULL, 2, NULL, 0);

    connectStation(0);
}

void loop() {
    handleButtons();
    tickSpectrum();
    webServerHandle();
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
void audio_info(const char *info)        { (void)info; }
void audio_id3data(const char *info)     { (void)info; }
void audio_eof_mp3(const char *info)     { (void)info; }
void audio_showstation(const char *info) { (void)info; }
void audio_bitrate(const char *info)     { (void)info; }
void audio_commercial(const char *info)  { (void)info; }
void audio_icyurl(const char *info)      { (void)info; }
void audio_lasthost(const char *info)    { (void)info; }
void audio_eof_speech(const char *info)  { (void)info; }
