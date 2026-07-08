#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include "Config.h"

class WiFiManager {
public:
    WiFiManager(Config& config) : _config(config), _apMode(false) {}

    void begin() {
        WiFi.mode(WIFI_AP_STA);
        WiFi.softAP("RPC-FEB-Control");

        if (_config.hasWiFiCredentials()) {
            String ssid = _config.getSSID();
            String pass = _config.getPassword();
            WiFi.begin(ssid.c_str(), pass.c_str());

            int attempts = 0;
            while (WiFi.status() != WL_CONNECTED && attempts < 40) {
                delay(500);
                attempts++;
            }

            if (WiFi.status() == WL_CONNECTED) {
                _apMode = false;
                if (MDNS.begin("rpc-feb-control"))
                    MDNS.addService("http", "tcp", 80);
                return;
            }
        }
        _apMode = true;
    }

    bool isConnected() { return WiFi.status() == WL_CONNECTED; }
    bool isAPMode() { return _apMode; }

    String getLocalIP() {
        return _apMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
    }

    String getSSID() {
        if (_apMode) return "RPC-FEB-Control (AP)";
        return WiFi.SSID();
    }

    void reconnectIfNeeded() {
        if (!_apMode && WiFi.status() != WL_CONNECTED)
            WiFi.reconnect();
    }

private:
    Config& _config;
    bool _apMode;
};
