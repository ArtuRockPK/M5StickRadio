#pragma once
#include "M5StickCPlus2.h"
#include "station_store.h"

// Globals defined in sketch_apr28a.ino
extern volatile int  currentStation;
extern volatile int  volume;
extern volatile bool volumeMode;
extern volatile bool isPlaying;
extern volatile bool uiDirty;
extern char          streamTitle[];
extern char          deviceIP[];

// ── Spectrum ──────────────────────────────────────────────────────────────────
static int specH[8] = {};
static int specT[8] = {};

static void drawSpectrum() {
    const int SX     = 185;
    const int SY     = 24;
    const int MAXH   = 16;
    const int BOT    = SY + MAXH;
    const int BW     = 5;
    const int STRIDE = 6;

    M5.Display.fillRect(SX, SY, 47, MAXH, TFT_BLACK);

    const bool playing = isPlaying;
    for (int i = 0; i < 8; i++) {
        const int h    = specH[i] < 1 ? 1 : specH[i];
        const int barX = SX + i * STRIDE;
        const uint32_t col = !playing ? TFT_DARKGREY :
                              h <= 5  ? TFT_GREEN    :
                              h <= 11 ? TFT_YELLOW   : TFT_RED;
        M5.Display.fillRect(barX, BOT - h, BW, h, col);
    }
}

static bool tickSpectrum() {
    if (volumeMode) return false;

    static unsigned long lastTick = 0;
    const unsigned long  now      = millis();
    if (now - lastTick < 80UL) return false;
    lastTick = now;

    const bool playing = isPlaying;
    bool changed = false;
    for (int i = 0; i < 8; i++) {
        if (playing) { if (random(3) == 0) specT[i] = random(2, 17); }
        else         { specT[i] = 1; }

        if      (specH[i] < specT[i]) { specH[i]++; changed = true; }
        else if (specH[i] > specT[i]) { specH[i]--; changed = true; }
    }
    if (changed) drawSpectrum();
    return changed;
}

// ── Helpers ───────────────────────────────────────────────────────────────────
static void _volBar(int x, int y, int w, int h, int val, int maxVal) {
    M5.Display.fillRect(x, y, w, h, 0x2104);
    int fill = map(val, 0, maxVal, 0, w);
    if (fill > 0) M5.Display.fillRect(x, y, fill, h, TFT_GREEN);
    M5.Display.drawRect(x, y, w, h, TFT_DARKGREY);
}

// ── Public API ────────────────────────────────────────────────────────────────

void initDisplay() {
    M5.Display.setRotation(3);
    M5.Display.fillScreen(TFT_BLACK);
}

// Shown while connecting to saved WiFi
void drawConnecting() {
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.setTextDatum(MC_DATUM);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setTextSize(2);
    M5.Display.drawString("WiFi", M5.Display.width() / 2, M5.Display.height() / 2 - 12);
    M5.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
    M5.Display.setTextSize(1);
    M5.Display.drawString("Connecting...", M5.Display.width() / 2, M5.Display.height() / 2 + 8);
}

void drawConnectingProgress(int dots) {
    char buf[6] = "     ";
    for (int i = 0; i < dots && i < 4; i++) buf[i] = '.';
    M5.Display.setTextDatum(MC_DATUM);
    M5.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
    M5.Display.setTextSize(2);
    M5.Display.drawString(buf, M5.Display.width() / 2, M5.Display.height() / 2 + 28);
}

// Shown when no saved credentials — AP captive portal is running
void drawAPMode() {
    const int W = M5.Display.width();
    const int H = M5.Display.height();
    M5.Display.fillScreen(TFT_BLACK);

    // Orange header
    M5.Display.fillRect(0, 0, W, 18, 0xFD20);
    M5.Display.setTextDatum(MC_DATUM);
    M5.Display.setTextColor(TFT_WHITE, 0xFD20);
    M5.Display.setTextSize(1);
    M5.Display.drawString("WiFi Setup", W / 2, 9);

    M5.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
    M5.Display.setTextSize(1);
    M5.Display.drawString("Connect your phone to:", W / 2, 32);

    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setTextSize(2);
    M5.Display.drawString("RadioSetup", W / 2, 52);

    M5.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
    M5.Display.setTextSize(1);
    M5.Display.drawString("(open, no password)", W / 2, 72);

    M5.Display.drawString("Then open in browser:", W / 2, 90);

    M5.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
    M5.Display.setTextSize(1);
    M5.Display.drawString("192.168.4.1", W / 2, 108);
    (void)H;
}

// Main radio UI — triggered by uiDirty
void drawUI() {
    const int W = M5.Display.width();

    M5.Display.fillScreen(TFT_BLACK);

    if (volumeMode) {
        // ── Volume mode ──────────────────────────────────────────────────────
        M5.Display.fillRect(0, 0, W, 22, TFT_RED);
        M5.Display.setTextDatum(MC_DATUM);
        M5.Display.setTextColor(TFT_WHITE, TFT_RED);
        M5.Display.setTextSize(1);
        M5.Display.drawString("VOLUME    A: +    B: -    3s: exit", W / 2, 11);

        StationEntry cur = getStation((int)currentStation);
        M5.Display.setTextDatum(TL_DATUM);
        M5.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
        M5.Display.setTextSize(1);
        M5.Display.drawString(cur.name, 6, 30);

        M5.Display.setTextDatum(MC_DATUM);
        M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
        M5.Display.setTextSize(4);
        char vbuf[4];
        snprintf(vbuf, sizeof(vbuf), "%d", (int)volume);
        M5.Display.drawString(vbuf, W / 2 - 22, 72);

        M5.Display.setTextSize(2);
        M5.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
        M5.Display.drawString("/21", W / 2 + 34, 76);

        _volBar(10, 112, W - 20, 14, (int)volume, 21);

    } else {
        // ── Normal mode ──────────────────────────────────────────────────────

        StationEntry cur = getStation((int)currentStation);

        // Header
        M5.Display.fillRect(0, 0, W, 18, TFT_NAVY);
        M5.Display.setTextDatum(TL_DATUM);
        M5.Display.setTextColor(TFT_DARKGREY, TFT_NAVY);
        M5.Display.setTextSize(1);
        M5.Display.drawString("A:Next  B:Prev  2xA:Vol", 4, 5);

        // Genre/lang badge
        M5.Display.fillRect(W - 28, 0, 28, 18, TFT_RED);
        M5.Display.setTextDatum(MC_DATUM);
        M5.Display.setTextColor(TFT_WHITE, TFT_RED);
        M5.Display.drawString(cur.lang, W - 14, 9);

        // Station name — max 15 chars, leaves x=185..239 free for spectrum
        char nameBuf[17];
        strncpy(nameBuf, cur.name, 15);
        nameBuf[15] = '\0';
        if (strlen(cur.name) > 15) nameBuf[14] = '~';
        M5.Display.setTextDatum(TL_DATUM);
        M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
        M5.Display.setTextSize(2);
        M5.Display.drawString(nameBuf, 4, 24);

        // Stream title / status
        M5.Display.setTextColor(TFT_CYAN, TFT_BLACK);
        M5.Display.setTextSize(1);
        const char *src = (streamTitle[0] != '\0') ? streamTitle :
                          (isPlaying               ? "On Air"        : "Connecting...");
        char tbuf[40];
        strncpy(tbuf, src, 37);
        tbuf[37] = '\0';
        if (strlen(src) > 37) tbuf[36] = '~';
        M5.Display.drawString(tbuf, 4, 58);

        // Station counter
        M5.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
        char pos[16];
        snprintf(pos, sizeof(pos), "%d / %d", (int)currentStation + 1, getTotalStationCount());
        M5.Display.drawString(pos, 4, 74);

        // IP address (web admin hint)
        if (deviceIP[0] != '\0') {
            M5.Display.setTextColor(0x4208, TFT_BLACK);   // dim grey
            M5.Display.drawString(deviceIP, 4, 86);
        }

        // Volume
        char vl[8];
        snprintf(vl, sizeof(vl), "Vol%2d", (int)volume);
        M5.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
        M5.Display.drawString(vl, 4, 100);
        _volBar(50, 98, W - 58, 12, (int)volume, 21);
    }
}
