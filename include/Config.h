#pragma once

#include <Arduino.h>
#include <LittleFS.h>

class Config {
public:
    bool begin() {
        return LittleFS.begin();
    }

    String getSSID() { return readFile("/ssid.txt"); }
    String getPassword() { return readFile("/pass.txt"); }

    void saveWiFi(const String& ssid, const String& password) {
        writeFile("/ssid.txt", ssid);
        writeFile("/pass.txt", password);
    }

    void deleteWiFi() {
        LittleFS.remove("/ssid.txt");
        LittleFS.remove("/pass.txt");
    }

    bool hasWiFiCredentials() {
        return LittleFS.exists("/ssid.txt");
    }

private:
    String readFile(const char* path) {
        File file = LittleFS.open(path, "r");
        if (!file) return "";
        String content = file.readString();
        content.trim();
        file.close();
        return content;
    }

    bool writeFile(const char* path, const String& content) {
        File file = LittleFS.open(path, "w");
        if (!file) return false;
        file.print(content);
        file.close();
        return true;
    }
};
