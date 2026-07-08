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
        _server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest* req) {
            serveFile(req, "/style.css", "text/css");
        });
        _server.on("/app.js", HTTP_GET, [](AsyncWebServerRequest* req) {
            serveFile(req, "/app.js", "application/javascript");
        });

        _server.on("/", HTTP_GET, [this](AsyncWebServerRequest* req) {
            serveFile(req, "/index.html", "text/html");
        });

        _server.on("/feb", HTTP_GET, [this](AsyncWebServerRequest* req) {
            serveFile(req, "/feb.html", "text/html");
        });

        _server.on("/wifi", HTTP_GET, [this](AsyncWebServerRequest* req) {
            serveFile(req, "/wifimanager.html", "text/html", false,
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

        _server.on("/api/dac/setall", HTTP_POST, [this](AsyncWebServerRequest* req) {
            handleApiSetAll(req);
        });

        _server.on("/api/dac/setfeb", HTTP_POST, [this](AsyncWebServerRequest* req) {
            handleApiSetFeb(req);
        });

        _server.on("/api/config/export", HTTP_GET, [this](AsyncWebServerRequest* req) {
            handleApiConfigExport(req);
        });

        _server.on("/api/config/import", HTTP_POST, [this](AsyncWebServerRequest* req) {
            handleApiConfigImport(req);
        });

        _server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest* req) {
            req->send(204);
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
        int feb = req->arg("feb").toInt();
        int chip = req->arg("chip").toInt();
        int channel = req->arg("channel").toInt();
        float voltage = req->arg("voltage").toFloat();

        if (feb < 0 || feb > 3 || chip < 0 || chip > 1 || channel < 1 || channel > 4) {
            req->send(400, "application/json", "{\"error\":\"Invalid parameters\"}");
            return;
        }

        _febs[feb].setDAC(chip, channel, voltage);
        String json = "{\"ok\":true,\"feb\":" + String(feb) + ","
            "\"chip\":" + String(chip) + ","
            "\"channel\":" + String(channel) + ","
            "\"target\":" + String(voltage) + "}";
        req->send(200, "application/json", json);
    }

    void handleApiDacEnable(AsyncWebServerRequest* req, bool enable) {
        int feb = req->arg("feb").toInt();
        int chip = req->arg("chip").toInt();

        if (feb < 0 || feb > 3 || chip < 0 || chip > 1) {
            req->send(400, "application/json", "{\"error\":\"Invalid parameters\"}");
            return;
        }

        if (enable) _febs[feb].enableDAC(chip);
        else _febs[feb].disableDAC(chip);

        String json = "{\"ok\":true,\"feb\":" + String(feb) + ","
            "\"chip\":" + String(chip) + ",\"enabled\":";
        json += enable ? "true" : "false";
        json += "}";
        req->send(200, "application/json", json);
    }

    void handleApiSetAll(AsyncWebServerRequest* req) {
        float voltage = req->arg("voltage").toFloat();
        String type = req->arg("type");
        int chStart, chEnd;
        if (type == "threshold") { chStart = 0; chEnd = 1; }
        else if (type == "vmon") { chStart = 2; chEnd = 3; }
        else { req->send(400, "application/json", "{\"error\":\"type must be threshold or vmon\"}"); return; }

        for (int feb = 0; feb < 4; feb++)
            for (int chip = 0; chip < 2; chip++)
                for (int ch = chStart; ch <= chEnd; ch++)
                    _febs[feb].setDAC(chip, ch + 1, voltage);

        req->send(200, "application/json", "{\"ok\":true}");
    }

    void handleApiSetFeb(AsyncWebServerRequest* req) {
        int feb = req->arg("feb").toInt();
        float voltage = req->arg("voltage").toFloat();
        String type = req->arg("type");
        if (feb < 0 || feb > 3) { req->send(400, "application/json", "{\"error\":\"Invalid FEB\"}"); return; }
        int chStart, chEnd;
        if (type == "threshold") { chStart = 0; chEnd = 1; }
        else if (type == "vmon") { chStart = 2; chEnd = 3; }
        else { req->send(400, "application/json", "{\"error\":\"type must be threshold or vmon\"}"); return; }

        for (int chip = 0; chip < 2; chip++)
            for (int ch = chStart; ch <= chEnd; ch++)
                _febs[feb].setDAC(chip, ch + 1, voltage);

        req->send(200, "application/json", "{\"ok\":true}");
    }

    void handleApiConfigExport(AsyncWebServerRequest* req) {
        String json = buildConfigJSON();
        AsyncWebServerResponse* response = req->beginResponse(200, "application/json", json);
        response->addHeader("Content-Disposition", "attachment; filename=dac_config.json");
        req->send(response);
    }

    void handleApiConfigImport(AsyncWebServerRequest* req) {
        String data = req->arg("data");
        int pos = data.indexOf("\"f\":[");
        if (pos < 0) { req->send(400, "text/plain", "Invalid config file"); return; }
        pos += 4;

        for (int feb = 0; feb < 4; feb++) {
            int tStart = data.indexOf("\"t\":[[", pos);
            if (tStart < 0) break;
            pos = tStart + 5;

            for (int chip = 0; chip < 2; chip++) {
                pos = data.indexOf('[', pos);
                if (pos < 0) break;
                pos++;
                for (int ch = 0; ch < 4; ch++) {
                    int end = pos;
                    while (end < (int)data.length() && data[end] != ',' && data[end] != ']') end++;
                    float val = data.substring(pos, end).toFloat();
                    _febs[feb].setDAC(chip, ch + 1, val);
                    pos = end + 1;
                }
            }

            int eStart = data.indexOf("\"e\":[", pos);
            if (eStart < 0) break;
            pos = eStart + 5;
            int e0 = data.substring(pos, pos + 1).toInt();
            int e1 = data.substring(pos + 2, pos + 3).toInt();
            if (e0) _febs[feb].enableDAC(0); else _febs[feb].disableDAC(0);
            if (e1) _febs[feb].enableDAC(1); else _febs[feb].disableDAC(1);
        }
        req->send(200, "application/json", "{\"ok\":true}");
    }

    String buildConfigJSON() {
        String json = "{\"v\":1,\"f\":[";
        for (int i = 0; i < 4; i++) {
            const FEBStatus& s = _febs[i].getStatus();
            if (i > 0) json += ",";
            json += "{\"t\":[[";
            for (int c = 0; c < 2; c++) {
                if (c > 0) json += "],[";
                for (int ch = 0; ch < 4; ch++) {
                    if (ch > 0) json += ",";
                    json += String((int)s.dacTarget[c][ch]);
                }
            }
            json += "]],\"e\":[";
            json += s.dacEnabled[0] ? "1" : "0";
            json += ",";
            json += s.dacEnabled[1] ? "1" : "0";
            json += "]}";
        }
        json += "]}";
        return json;
    }

    String buildStatusJSON() {
        String json = "{\"febs\":[";
        for (int i = 0; i < 4; i++) {
            const FEBStatus& s = _febs[i].getStatus();
            if (i > 0) json += ",";
            json += "{\"id\":" + String(i) + ",";
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

    static void serveFile(AsyncWebServerRequest* req, const String& path, const String& contentType) {
        File f = LittleFS.open(path, "r");
        if (!f) { req->send(404, "text/plain", "File not found"); return; }
        String content = f.readString();
        f.close();
        req->send(200, contentType, content);
    }

    static void serveFile(AsyncWebServerRequest* req, const String& path, const String& contentType,
                          bool unused, AwsTemplateProcessor callback) {
        (void)unused;
        File f = LittleFS.open(path, "r");
        if (!f) { req->send(404, "text/plain", "File not found"); return; }
        String content = f.readString();
        f.close();
        String result;
        int last = 0;
        while (true) {
            int s = content.indexOf('%', last);
            if (s < 0) { result += content.substring(last); break; }
            int e = content.indexOf('%', s + 1);
            if (e < 0) { result += content.substring(last); break; }
            result += content.substring(last, s);
            result += callback(content.substring(s + 1, e));
            last = e + 1;
        }
        req->send(200, contentType, result);
    }
};
