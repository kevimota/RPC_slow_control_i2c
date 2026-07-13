#pragma once

#include <Arduino.h>
#include <Wire.h>
#include "esp_task_wdt.h"
#include "PCF8574A.h"
#include "AD7417.h"
#include "AD5316.h"

struct FEBStatus {
    uint8_t pcfValue;
    float adc[2][4];
    float temp[2];
    float dacTarget[2][4];
    bool dacEnabled[2];
};

class FEBManager {
public:
    FEBManager() : _id(0), _initialized(false) {}

    void begin(int id) {
        _id = id;
        _pcf.begin(PCF8574A_BASE_ADDR + 2 * _id);
        _adc[0].begin(AD7417_BASE_ADDR + 2 * _id + 0);
        _adc[1].begin(AD7417_BASE_ADDR + 2 * _id + 1);
        for (int c = 0; c < 2; c++) {
            _dacEnabled[c] = true;
            for (int ch = 0; ch < 4; ch++)
                _dacTarget[c][ch] = 0;
        }
        _initialized = true;
    }

    int id() const { return _id; }
    bool isInitialized() const { return _initialized; }

    void update() {
        _cached.pcfValue = _pcf.read();
        for (int chip = 0; chip < 2; chip++) {
            for (int ch = 0; ch < 4; ch++)
                _cached.adc[chip][ch] = _adc[chip].readADC(ch + 1);
            _cached.temp[chip] = _adc[chip].readTemp();
        }
        for (int c = 0; c < 2; c++) {
            _cached.dacEnabled[c] = _dacEnabled[c];
            for (int ch = 0; ch < 4; ch++)
                _cached.dacTarget[c][ch] = _dacTarget[c][ch];
        }
    }

    const FEBStatus& getStatus() const { return _cached; }

    float readADC(int chip, int channel) {
        if (chip < 0 || chip > 1 || channel < 1 || channel > 4) return -1.0;
        return _adc[chip].readADC(channel);
    }

    float readTemp(int chip) {
        if (chip < 0 || chip > 1) return -1.0;
        return _adc[chip].readTemp();
    }

    bool setDAC(int chip, int channel, float targetMV) {
        if (chip < 0 || chip > 1 || channel < 1 || channel > 4) return false;
        if (targetMV < 0) targetMV = 0;
        if (targetMV > 5000) targetMV = 5000;

        PCF8574A::selDac(_id, chip);
        PCF8574A::enDac(_id, chip);
        delay(1);

        uint16_t code = AD5316::voltageToCode(targetMV);
        AD5316::writeRaw(channel - 1, code);
        delay(2);

        for (int iter = 0; iter < 5; iter++) {
            esp_task_wdt_reset();
            float measured = _adc[chip].readADC(channel);
            if (measured < 0) break;
            float error = targetMV - measured;
            if (fabs(error) < 2.0) break;
            float correction = targetMV / measured;
            code = (uint16_t)(code * correction);
            if (code > AD5316_MAX_CODE) code = AD5316_MAX_CODE;
            AD5316::writeRaw(channel - 1, code);
            delay(2);
        }

        _dacTarget[chip][channel - 1] = targetMV;
        return true;
    }

    void enableDAC(int chip) {
        if (chip < 0 || chip > 1) return;
        PCF8574A::enDac(_id, chip);
        _dacEnabled[chip] = true;
    }

    void disableDAC(int chip) {
        if (chip < 0 || chip > 1) return;
        PCF8574A::disDac(_id, chip);
        _dacEnabled[chip] = false;
    }

private:
    int _id;
    bool _initialized;
    PCF8574A _pcf;
    AD7417 _adc[2];
    bool _dacEnabled[2];
    float _dacTarget[2][4];
    FEBStatus _cached;
};
