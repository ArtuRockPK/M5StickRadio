#pragma once

struct RadioStation {
    const char* name;
    const char* url;
    const char* lang;
};

static const RadioStation stations[] = {

    // ── Confirmed working ────────────────────────────────────────────────────
    {"WDR EinsLive",    "http://www.wdr.de/wdrlive/media/einslive.m3u",              "DE"},

    // ── Various sources ──────────────────────────────────────────────────────
    {"Dance UK",      "http://uk2.internet-radio.com:8024/",             "UK"},
    {"BBC Radio 1",     "http://www.partyviberadio.com:8004/",        "EN"},
    {"DRUM & BASS",  "http://uk3.internet-radio.com:8192/live",                  "EN"},
    {"France Inter",    "https://icecast.radiofrance.fr/franceinter-midfi.mp3",      "FR"},
    {"Vesti FM",        "http://icecast.vgtrk.cdnvideo.ru/vestifm_mp3_192kbps",     "RU"},
    {"Record",          "http://radiorecord.hostingradio.ru/record96.aacp",          "RU"},
    {"RNE Radio 3",     "http://rne3-lp.rtveradio.cires21.com/rne3/rne3.mp3",       "ES"},
    {"Classical 102",   "http://tuner.classical102.com/listen.pls",                  "CL"},
    {"SomaFM Groove",   "https://somafm.com/groovesalad256.pls",                    "US"},

};

static const int STATION_COUNT = sizeof(stations) / sizeof(stations[0]);
