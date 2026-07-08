#include <Arduino.h>
#include <Wire.h>
#include <LittleFS.h>
#include "PCF8574A.h"
#include "AD7417.h"
#include "AD5316.h"
#include "FEBManager.h"
#include "Config.h"
#include "WiFiManager.h"
#include "WebServer.h"

#ifndef I2C_SDA
#define I2C_SDA 10
#endif
#ifndef I2C_SCL
#define I2C_SCL 11
#endif

Config config;
FEBManager febs[4];
WiFiManager wifiMgr(config);
WebServer server(febs, config, wifiMgr);

void setup() {
    Wire.begin(I2C_SDA, I2C_SCL, 100000);

    Serial.begin(115200);
    delay(500);

    config.begin();

    for (int i = 0; i < 4; i++)
        febs[i].begin(i);

    PCF8574A::enableAll();

    wifiMgr.begin();
    server.begin();

    Serial.print("Ready at http://");
    Serial.println(wifiMgr.getLocalIP());
}

void loop() {
    static unsigned long lastUpdate = 0;
    unsigned long now = millis();

    if (now - lastUpdate >= 500) {
        lastUpdate = now;
        for (int i = 0; i < 4; i++)
            febs[i].update();
        wifiMgr.reconnectIfNeeded();
    }

    delay(1);
}
