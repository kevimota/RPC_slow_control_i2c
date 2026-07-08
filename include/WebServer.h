#pragma once

#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include "FEBManager.h"
#include "Config.h"
#include "WiFiManager.h"

class WebServer {
public:
    WebServer(FEBManager* febs, Config& config, WiFiManager& wifiMgr)
        : _server(80), _febs(febs), _config(config), _wifiMgr(wifiMgr) {}

    void begin() {
        _server.serveStatic("/style.css", LittleFS, "/style.css");
        _server.serveStatic("/app.js", LittleFS, "/app.js");

        _server.on("/", HTTP_GET, [this](AsyncWebServerRequest* req) {
            req->send(LittleFS, "/index.html", "text/html");
        });

        _server.on("/feb", HTTP_GET, [this](AsyncWebServerRequest* req) {
            req->send(LittleFS, "/feb.html", "text/html");
        });

        _server.on("/wifi", HTTP_GET, [this](AsyncWebServerRequest* req) {
            req->send(LittleFS, "/wifimanager.html", "text/html", false,
                std::bind(&WebServer::wifiProcessor, this, std::placeholders::_1));
        });

        _server.on("/wifi", HTTP_POST, [this](AsyncWebServerRequest* req) {
            handleWiFiPost(req);
        });

        _server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* req) {
            req->send(200, "application/json", buildStatusJSON());
        });

        _server.on("/api/dac", HTTP_POST, [this](AsyncWebServerRequest* req) {
            handleApiDac(req);
        });

        _server.on("/api/dac/enable", HTTP_POST, [this](AsyncWebServerRequest* req) {
            handleApiDacEnable(req, true);
        });

        _server.on("/api/dac/disable", HTTP_POST, [this](AsyncWebServerRequest* req) {
            handleApiDacEnable(req, false);
        });

        _server.onNotFound([](AsyncWebServerRequest* req) {
            req->send(404, "text/plain", "Not found");
        });

        _server.begin();
    }

private:
    AsyncWebServer _server;
    FEBManager* _febs;
    Config& _config;
    WiFiManager& _wifiMgr;

    String wifiProcessor(const String& var) {
        if (var == "SSID") return _wifiMgr.getSSID();
        if (var == "IP") return _wifiMgr.getLocalIP();
        if (var == "AP_MODE") return _wifiMgr.isAPMode() ? " (AP mode - no STA connection)" : "";
        return String();
    }

    void handleWiFiPost(AsyncWebServerRequest* req) {
        String ssid = req->arg("ssid");
        String pass = req->arg("pass");
        _config.saveWiFi(ssid, pass);
        req->send(200, "text/html",
            "<html><body><h2>Credentials saved. Restarting...</h2>"
            "<script>setTimeout(function(){location.href='/';},3000);</script></body></html>");
        delay(1000);
        ESP.restart();
    }

    void handleApiDac(AsyncWebServerRequest* req) {
        String febStr = req->arg("feb");
        if (febStr.length() != 1) {
            req->send(400, "application/json", "{\"error\":\"Invalid FEB\"}");
            return;
        }
        char feb = febStr[0];
        int chip = req->arg("chip").toInt();
        int channel = req->arg("channel").toInt();
        float voltage = req->arg("voltage").toFloat();

        if (feb < 'A' || feb > 'D' || chip < 0 || chip > 1 || channel < 1 || channel > 4) {
            req->send(400, "application/json", "{\"error\":\"Invalid parameters\"}");
            return;
        }

        _febs[feb - 'A'].setDAC(chip, channel, voltage);
        String json = "{\"ok\":true,\"feb\":\"" + String(feb) + "\","
            "\"chip\":" + String(chip) + ","
            "\"channel\":" + String(channel) + ","
            "\"target\":" + String(voltage) + "}";
        req->send(200, "application/json", json);
    }

    void handleApiDacEnable(AsyncWebServerRequest* req, bool enable) {
        String febStr = req->arg("feb");
        if (febStr.length() != 1) {
            req->send(400, "application/json", "{\"error\":\"Invalid FEB\"}");
            return;
        }
        char feb = febStr[0];
        int chip = req->arg("chip").toInt();

        if (feb < 'A' || feb > 'D' || chip < 0 || chip > 1) {
            req->send(400, "application/json", "{\"error\":\"Invalid parameters\"}");
            return;
        }

        if (enable) _febs[feb - 'A'].enableDAC(chip);
        else _febs[feb - 'A'].disableDAC(chip);

        String json = "{\"ok\":true,\"feb\":\"" + String(feb) + "\","
            "\"chip\":" + String(chip) + ",\"enabled\":";
        json += enable ? "true" : "false";
        json += "}";
        req->send(200, "application/json", json);
    }

    String buildStatusJSON() {
        String json = "{\"febs\":[";
        for (int i = 0; i < 4; i++) {
            _febs[i].update();
            const FEBStatus& s = _febs[i].getStatus();
            if (i > 0) json += ",";
            char fid = 'A' + i;
            json += "{\"id\":\"" + String(fid) + "\",";
            json += "\"pcf\":" + String(s.pcfValue) + ",";
            json += "\"temp\":[" + String(s.temp[0], 1) + "," + String(s.temp[1], 1) + "],";
            json += "\"adc\":[";
            for (int c = 0; c < 2; c++) {
                if (c > 0) json += ",";
                json += "[";
                for (int ch = 0; ch < 4; ch++) {
                    if (ch > 0) json += ",";
                    json += String((int)s.adc[c][ch]);
                }
                json += "]";
            }
            json += "],";
            json += "\"dac\":{\"target\":[";
            for (int c = 0; c < 2; c++) {
                if (c > 0) json += ",";
                json += "[";
                for (int ch = 0; ch < 4; ch++) {
                    if (ch > 0) json += ",";
                    json += String((int)s.dacTarget[c][ch]);
                }
                json += "]";
            }
            json += "],\"enabled\":[";
            json += s.dacEnabled[0] ? "true" : "false";
            json += ",";
            json += s.dacEnabled[1] ? "true" : "false";
            json += "]}";
            json += "}";
        }
        json += "],";
        json += "\"wifi\":{\"ip\":\"" + _wifiMgr.getLocalIP() + "\",";
        json += "\"ssid\":\"" + _wifiMgr.getSSID() + "\",\"connected\":";
        json += _wifiMgr.isConnected() ? "true" : "false";
        json += "}}";
        return json;
    }
};
