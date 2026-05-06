#pragma once
#include <WebServer.h>
#include "station_store.h"

extern volatile int  currentStation;
extern volatile bool isPlaying;
extern volatile bool uiDirty;
void connectStation(int idx);
void stopPlayback();

static WebServer _webSrv(80);

static const char _WEB_CSS[] PROGMEM =
    "body{font-family:sans-serif;max-width:640px;margin:0 auto;padding:12px;color:#333}"
    "h2{color:#1565c0;margin:0 0 4px}"
    ".sub{color:#888;font-size:13px;margin:0 0 10px}"
    ".player{background:#e3f2fd;border-radius:8px;padding:12px 14px;margin:0 0 14px}"
    ".player-name{font-weight:600;font-size:14px;margin-bottom:8px;color:#1565c0;"
                 "overflow:hidden;text-overflow:ellipsis;white-space:nowrap}"
    ".ctrl-btn{border:none;padding:8px 14px;border-radius:5px;cursor:pointer;"
              "font-size:13px;margin:2px}"
    ".btn-stop{background:#e53935;color:#fff}"
    ".btn-play{background:#43a047;color:#fff}"
    ".btn-nav{background:#546e7a;color:#fff}"
    ".st{display:flex;align-items:center;gap:8px;padding:7px 10px;"
        "border:1px solid #e0e0e0;margin:3px 0;border-radius:6px}"
    ".active-st{border-color:#1565c0;background:#e8f4fd}"
    ".sn{font-weight:600;flex:1;min-width:0;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}"
    ".tag{font-size:11px;padding:2px 6px;border-radius:3px;flex-shrink:0;"
         "background:#e3f2fd;color:#1565c0}"
    ".tag.my{background:#fff3e0;color:#e65100}"
    ".del{background:#e53935;color:#fff;border:none;padding:4px 10px;"
         "border-radius:4px;cursor:pointer;flex-shrink:0;font-size:13px}"
    ".play{background:#1565c0;color:#fff;border:none;padding:4px 10px;"
          "border-radius:4px;cursor:pointer;flex-shrink:0;font-size:13px}"
    "h3{margin:16px 0 8px;color:#444}"
    "input{width:100%;padding:8px;margin:4px 0;box-sizing:border-box;"
          "border:1px solid #bbb;border-radius:5px;font-size:14px}"
    ".btn{border:none;padding:10px 18px;border-radius:5px;cursor:pointer;font-size:14px;margin-top:6px}"
    ".btn-add{background:#1565c0;color:#fff}"
    ".btn-restore{background:#757575;color:#fff}";

static void _webHandleRoot() {
    int total  = getTotalStationCount();
    int custom = getCustomStationCount();
    int active_builtin = total - custom;

    _webSrv.setContentLength(CONTENT_LENGTH_UNKNOWN);
    _webSrv.send(200, "text/html; charset=utf-8", "");

    _webSrv.sendContent(
        "<!DOCTYPE html><html><head>"
        "<meta charset=utf-8>"
        "<meta name=viewport content='width=device-width,initial-scale=1'>"
        "<title>Radio Stations</title><style>");
    _webSrv.sendContent(FPSTR(_WEB_CSS));
    _webSrv.sendContent("</style></head><body>");

    // Header
    {
        char hdr[120];
        snprintf(hdr, sizeof(hdr),
            "<h2>&#128251; Radio Stations</h2>"
            "<p class=sub>%d total &middot; %d built-in &middot; %d custom</p>",
            total, active_builtin, custom);
        _webSrv.sendContent(hdr);
    }

    // Player controls
    if (total > 0) {
        StationEntry cur = getStation((int)currentStation);
        char play[700];
        snprintf(play, sizeof(play),
            "<div class=player>"
            "<div class=player-name>&#127925; %s</div>"
            "<form method=POST action=/prev style='display:inline'>"
            "<button type=submit class='ctrl-btn btn-nav'>&#9664; Prev</button>"
            "</form>"
            "<form method=POST action=%s style='display:inline'>"
            "<button type=submit class='ctrl-btn %s'>%s</button>"
            "</form>"
            "<form method=POST action=/next style='display:inline'>"
            "<button type=submit class='ctrl-btn btn-nav'>Next &#9654;</button>"
            "</form>"
            "</div>",
            cur.name,
            isPlaying ? "/stop" : "/play",
            isPlaying ? "btn-stop" : "btn-play",
            isPlaying ? "&#9632; Stop" : "&#9654; Play");
        _webSrv.sendContent(play);
    }

    // Add Station form
    _webSrv.sendContent(
        "<h3>Add Station</h3>"
        "<form method=POST action=/add>"
        "<input type=text name=name placeholder='Station name' required maxlength=31>"
        "<input type=text name=url  placeholder='Stream URL (http://...)' required maxlength=199>"
        "<button type=submit class='btn btn-add'>+ Add</button>"
        "</form>");

    // Restore button
    _webSrv.sendContent(
        "<form method=POST action=/restore style='margin:8px 0 4px'>"
        "<button type=submit class='btn btn-restore'>&#8635; Restore built-in stations</button>"
        "</form>");

    // Station list
    _webSrv.sendContent("<h3>Stations</h3>");
    for (int i = 0; i < total; i++) {
        StationEntry s = getStation(i);
        const char *tagClass = s.isBuiltin ? "tag" : "tag my";
        bool isActive = ((int)currentStation == i);
        char row[900];
        snprintf(row, sizeof(row),
            "<div class='st%s'>"
            "<form method=POST action=/select style='margin:0'>"
            "<input type=hidden name=idx value=%d>"
            "<button type=submit class=play title=Play>&#9654;</button>"
            "</form>"
            "<span class=sn>%s%s</span>"
            "<span class='%s'>%s</span>"
            "<form method=POST action=/delete style='margin:0'>"
            "<input type=hidden name=idx value=%d>"
            "<button type=submit class=del title=Delete>&#10005;</button>"
            "</form></div>",
            isActive ? " active-st" : "",
            i,
            isActive ? "&#9654;&nbsp;" : "",
            s.name, tagClass, s.lang, i);
        _webSrv.sendContent(row);
    }

    _webSrv.sendContent("</body></html>");
    _webSrv.sendContent("");
}

static void _webHandleAdd() {
    String name = _webSrv.arg("name"); name.trim();
    String url  = _webSrv.arg("url");  url.trim();
    if (name.length() > 0 && url.length() > 0)
        storeAdd(name.c_str(), url.c_str());
    _webSrv.sendHeader("Location", "/");
    _webSrv.send(302, "text/plain", "");
}

static void _webHandleDelete() {
    int idx = _webSrv.arg("idx").toInt();
    storeDeleteByIdx(idx);

    int newTotal = getTotalStationCount();
    if (newTotal == 0) {
        // nothing to play
    } else if ((int)currentStation == idx) {
        connectStation(idx > 0 ? idx - 1 : 0);
    } else if ((int)currentStation > idx) {
        currentStation--;
        uiDirty = true;
    }

    _webSrv.sendHeader("Location", "/");
    _webSrv.send(302, "text/plain", "");
}

static void _webHandleRestore() {
    storeRestoreBuiltins();
    _webSrv.sendHeader("Location", "/");
    _webSrv.send(302, "text/plain", "");
}

static void _webHandleSelect() {
    int idx = _webSrv.arg("idx").toInt();
    if (idx >= 0 && idx < getTotalStationCount())
        connectStation(idx);
    _webSrv.sendHeader("Location", "/");
    _webSrv.send(302, "text/plain", "");
}

static void _webHandleStop() {
    stopPlayback();
    _webSrv.sendHeader("Location", "/");
    _webSrv.send(302, "text/plain", "");
}

static void _webHandlePlay() {
    if (getTotalStationCount() > 0)
        connectStation((int)currentStation);
    _webSrv.sendHeader("Location", "/");
    _webSrv.send(302, "text/plain", "");
}

static void _webHandleNext() {
    int total = getTotalStationCount();
    if (total > 0)
        connectStation(((int)currentStation + 1) % total);
    _webSrv.sendHeader("Location", "/");
    _webSrv.send(302, "text/plain", "");
}

static void _webHandlePrev() {
    int total = getTotalStationCount();
    if (total > 0)
        connectStation(((int)currentStation - 1 + total) % total);
    _webSrv.sendHeader("Location", "/");
    _webSrv.send(302, "text/plain", "");
}

static void webServerSetup() {
    _webSrv.on("/",        HTTP_GET,  _webHandleRoot);
    _webSrv.on("/add",     HTTP_POST, _webHandleAdd);
    _webSrv.on("/delete",  HTTP_POST, _webHandleDelete);
    _webSrv.on("/restore", HTTP_POST, _webHandleRestore);
    _webSrv.on("/select",  HTTP_POST, _webHandleSelect);
    _webSrv.on("/stop",    HTTP_POST, _webHandleStop);
    _webSrv.on("/play",    HTTP_POST, _webHandlePlay);
    _webSrv.on("/next",    HTTP_POST, _webHandleNext);
    _webSrv.on("/prev",    HTTP_POST, _webHandlePrev);
    _webSrv.begin();
}

static void webServerHandle() {
    _webSrv.handleClient();
}
