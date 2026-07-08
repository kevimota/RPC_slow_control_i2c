#pragma once

#include <Arduino.h>
#include <Wire.h>

#define PCF8574A_BASE_ADDR 0x38

class PCF8574A {
public:
    PCF8574A() : _address(0) {}

    void begin(uint8_t address) { _address = address; }

    uint8_t read() {
        Wire.requestFrom(_address, (uint8_t)1);
        uint8_t value = 0;
        while (Wire.available()) value = Wire.read();
        return value;
    }

    bool write(uint8_t value) {
        Wire.beginTransmission(_address);
        Wire.write(value);
        return Wire.endTransmission() == 0;
    }

    static void selDac(char feb, int chip) {
        int nFEB = feb - 'A';
        if (nFEB < 0 || nFEB > 3 || chip < 0 || chip > 1) return;
        for (int i = 0; i < 4; i++) {
            uint8_t val = readRaw(PCF8574A_BASE_ADDR + 2 * i);
            for (int j = 0; j < 2; j++) {
                if (i == nFEB && j == chip)
                    val &= ~(1 << j);
                else
                    val |= (1 << j);
            }
            writeRaw(PCF8574A_BASE_ADDR + 2 * i, val);
        }
    }

    static void enDac(char feb, int chip) {
        int nFEB = feb - 'A';
        if (nFEB < 0 || nFEB > 3 || chip < 0 || chip > 1) return;
        uint8_t val = readRaw(PCF8574A_BASE_ADDR + 2 * nFEB);
        val &= ~(1 << (2 + chip));
        writeRaw(PCF8574A_BASE_ADDR + 2 * nFEB, val);
    }

    static void disDac(char feb, int chip) {
        int nFEB = feb - 'A';
        if (nFEB < 0 || nFEB > 3 || chip < 0 || chip > 1) return;
        uint8_t val = readRaw(PCF8574A_BASE_ADDR + 2 * nFEB);
        val |= (1 << (2 + chip));
        writeRaw(PCF8574A_BASE_ADDR + 2 * nFEB, val);
    }

    static void enableAll() {
        for (char feb = 'A'; feb <= 'D'; feb++)
            for (int chip = 0; chip < 2; chip++)
                enDac(feb, chip);
    }

private:
    uint8_t _address;

    static uint8_t readRaw(uint8_t addr) {
        Wire.requestFrom(addr, (uint8_t)1);
        uint8_t value = 0;
        while (Wire.available()) value = Wire.read();
        return value;
    }

    static bool writeRaw(uint8_t addr, uint8_t value) {
        Wire.beginTransmission(addr);
        Wire.write(value);
        return Wire.endTransmission() == 0;
    }
};
