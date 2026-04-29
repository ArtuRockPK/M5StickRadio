/*
 * Internet Radio for M5StickCPlus2
 * Requires library: ESP32-audioI2S
 * https://github.com/schreibfaul1/ESP32-audioI2S/wiki
 *
 * Controls:
 *   A (single)  — next station
 *   B (single)  — previous station
 *   A (double)  — enter / exit volume mode
 *   Volume mode: A = vol+,  B = vol-,  3s inactivity = auto-exit
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

// Global state (shared with ui.h)
int    currentStation = 0;
int    volume         = 15;
bool   volumeMode     = false;
String streamTitle    = "";

// Button A double-click tracking
static unsigned long lastPressA    = 0;
static bool          pendingClickA = false;
static unsigned long lastActivity  = 0;

// ─────────────────────────────────────────────────────────────────────────────

void connectStation(int idx) {
    currentStation = idx;
    streamTitle    = "";
    drawUI();  // immediate feedback before HTTP request
    audio.connecttohost(stations[idx].url);
}

void handleButtons() {
    M5.update();
    const unsigned long now = millis();

    if (M5.BtnA.wasPressed()) {
        if (volumeMode) {
            volume = min(21, volume + 1);
            audio.setVolume(volume);
            lastActivity = now;
            drawUI();
        } else if (pendingClickA && (now - lastPressA < DOUBLE_CLICK_MS)) {
            // Double click — enter volume mode
            pendingClickA = false;
            volumeMode    = true;
            lastActivity  = now;
            drawUI();
        } else {
            pendingClickA = true;
            lastPressA    = now;
        }
    }

    // Pending single click timed out → next station
    if (pendingClickA && !volumeMode && (now - lastPressA >= DOUBLE_CLICK_MS)) {
        pendingClickA = false;
        connectStation((currentStation + 1) % STATION_COUNT);
    }

    if (M5.BtnB.wasPressed()) {
        if (volumeMode) {
            volume = max(0, volume - 1);
            audio.setVolume(volume);
            lastActivity = now;
            drawUI();
        } else {
            connectStation((currentStation - 1 + STATION_COUNT) % STATION_COUNT);
        }
    }

    // Auto-exit volume mode after inactivity
    if (volumeMode && (now - lastActivity > VOLUME_TIMEOUT_MS)) {
        volumeMode = false;
        drawUI();
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
    audio.setVolume(volume);
    connectStation(0);
}

void loop() {
    handleButtons();
    audio.loop();
    tickSpectrum();
}

// ─── Audio event callbacks ────────────────────────────────────────────────────

void audio_showstreamtitle(const char *info) {
    streamTitle = info;
    drawUI();
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
