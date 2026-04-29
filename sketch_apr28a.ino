/*
 * Internet Radio for M5StickCPlus2
 * Requires library: ESP32-audioI2S
 * https://github.com/schreibfaul1/ESP32-audioI2S/wiki
 */
#include "M5StickCPlus2.h"
#include "Arduino.h"
#include "WiFi.h"
#include "Audio.h"
#include "secrets.h"

#define I2S_DOUT 25
#define I2S_BCLK 26
#define I2S_LRC   0

Audio audio;

void setup() {
    auto cfg = M5.config();
    cfg.internal_mic = false;
    M5.begin(cfg);
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) delay(1500);
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(21);
    audio.connecttohost("http://www.wdr.de/wdrlive/media/einslive.m3u");
}

void loop() {
    audio.loop();
}

void audio_info(const char *info)            { Serial.print("info: ");        Serial.println(info); }
void audio_id3data(const char *info)         { Serial.print("id3: ");         Serial.println(info); }
void audio_eof_mp3(const char *info)         { Serial.print("eof_mp3: ");     Serial.println(info); }
void audio_showstation(const char *info)     { Serial.print("station: ");     Serial.println(info); }
void audio_showstreamtitle(const char *info) { Serial.print("streamtitle: "); Serial.println(info); }
void audio_bitrate(const char *info)         { Serial.print("bitrate: ");     Serial.println(info); }
void audio_commercial(const char *info)      { Serial.print("commercial: ");  Serial.println(info); }
void audio_icyurl(const char *info)          { Serial.print("icyurl: ");      Serial.println(info); }
void audio_lasthost(const char *info)        { Serial.print("lasthost: ");    Serial.println(info); }
void audio_eof_speech(const char *info)      { Serial.print("eof_speech: ");  Serial.println(info); }
