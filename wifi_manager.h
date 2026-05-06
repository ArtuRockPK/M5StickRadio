#pragma once
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>

static bool wm_loadCreds(String &ssid, String &pass) {
    Preferences p;
    p.begin("wifi", true);
    ssid = p.getString("ssid", "");
    pass = p.getString("pass", "");
    p.end();
    return ssid.length() > 0;
}

static void wm_saveCreds(const String &ssid, const String &pass) {
    Preferences p;
    p.begin("wifi", false);
    p.putString("ssid", ssid);
    p.putString("pass", pass);
    p.end();
}

// Blocks until the user submits credentials via captive portal, then reboots.
static void runWifiSetup() {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP("RadioSetup");   // open AP, IP 192.168.4.1
    delay(300);

    // Synchronous scan before web server starts
    int n = WiFi.scanNetworks(false, false);

    DNSServer dns;
    dns.start(53, "*", IPAddress(192, 168, 4, 1));

    WebServer srv(80);
    String savedSsid, savedPass;
    bool   done = false;

    // Build network list once
    String netHtml;
    netHtml.reserve(n * 100);
    for (int i = 0; i < n; i++) {
        String name = WiFi.SSID(i);
        String safe = name;
        safe.replace("'", "\\'");
        safe.replace("<", "&lt;");
        safe.replace(">", "&gt;");
        netHtml += "<div class=net onclick=\"F('" + safe + "')\">" +
                   safe + " <small>(" + WiFi.RSSI(i) + " dBm)</small></div>";
    }

    auto sendPage = [&]() {
        String html =
            "<!DOCTYPE html><html><head>"
            "<meta charset=utf-8>"
            "<meta name=viewport content='width=device-width,initial-scale=1'>"
            "<title>Radio Setup</title>"
            "<style>"
            "body{font-family:sans-serif;max-width:420px;margin:24px auto;padding:12px}"
            "h2{color:#1565c0;margin:0 0 8px}"
            "p{color:#555;margin:4px 0 12px}"
            ".net{padding:10px 12px;border:1px solid #ddd;margin:4px 0;"
                 "border-radius:6px;cursor:pointer;display:flex;justify-content:space-between}"
            ".net:hover{background:#e8eaf6}"
            "input{width:100%;padding:9px;margin:5px 0;box-sizing:border-box;"
                  "border:1px solid #bbb;border-radius:5px;font-size:15px}"
            "button{background:#1565c0;color:#fff;border:none;padding:12px;"
                   "border-radius:5px;cursor:pointer;width:100%;font-size:16px;margin-top:8px}"
            "</style></head><body>"
            "<h2>&#128251; WiFi Setup</h2>"
            "<p>Tap a network or enter the name:</p>" +
            netHtml +
            "<form method=POST action=/save>"
            "<input type=text   name=ssid id=S placeholder='Network name' required>"
            "<input type=password name=pass   placeholder='Password (blank if open)'>"
            "<button>&#10003; Connect &amp; Save</button>"
            "</form>"
            "<script>function F(n){document.getElementById('S').value=n}</script>"
            "</body></html>";
        srv.send(200, "text/html", html);
    };

    srv.on("/", HTTP_GET, [&]() { sendPage(); });

    srv.on("/save", HTTP_POST, [&]() {
        savedSsid = srv.arg("ssid");
        savedPass = srv.arg("pass");
        srv.send(200, "text/html",
            "<html><body style='font-family:sans-serif;text-align:center;padding:40px'>"
            "<h2 style=color:green>&#10003; Saved!</h2>"
            "<p>Rebooting device...</p></body></html>");
        done = true;
    });

    // Captive portal: redirect everything to the setup page
    srv.onNotFound([&]() {
        srv.sendHeader("Location", "http://192.168.4.1/");
        srv.send(302, "text/plain", "");
    });

    srv.begin();

    while (!done) {
        dns.processNextRequest();
        srv.handleClient();
        delay(2);
    }

    delay(1500);
    wm_saveCreds(savedSsid, savedPass);
    ESP.restart();
}
