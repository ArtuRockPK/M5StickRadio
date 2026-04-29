#pragma once

struct RadioStation {
    const char* name;
    const char* url;
    const char* lang;
};

static const RadioStation stations[] = {
    // Confirmed working — keep first
    {"WDR EinsLive",   "http://www.wdr.de/wdrlive/media/einslive.m3u",              "DE"},
    // English
    {"Capital FM",     "http://vis.media-ice.musicradio.com/CapitalMP3",             "EN"},
    {"BBC Radio 1",    "http://stream.live.vc.bbcmedia.co.uk/bbc_radio_one",        "EN"},
    {"Radio Paradise", "https://stream.radioparadise.com/mp3-192",                  "EN"},
    // French
    {"France Inter",   "https://icecast.radiofrance.fr/franceinter-midfi.mp3",      "FR"},
    // Russian
    {"Vesti FM",       "http://icecast.vgtrk.cdnvideo.ru/vestifm_mp3_192kbps",     "RU"},
    {"Record",         "http://radiorecord.hostingradio.ru/record96.aacp",          "RU"},
    // Spanish
    {"RNE Radio 3",    "http://rne3-lp.rtveradio.cires21.com/rne3/rne3.mp3",       "ES"},
    // Classical
    {"Classical 102",  "http://tuner.classical102.com/listen.pls",                  "CL"},
    // Ambient / USA
    {"SomaFM Groove",  "https://somafm.com/groovesalad256.pls",                    "US"},
};

static const int STATION_COUNT = sizeof(stations) / sizeof(stations[0]);
