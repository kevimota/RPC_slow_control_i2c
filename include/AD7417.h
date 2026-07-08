#pragma once

#include <Arduino.h>
#include <Wire.h>

#define AD7417_BASE_ADDR 0x28
#define AD7417_REG_CONFIG 0x01
#define AD7417_REG_TEMP   0x00
#define AD7417_REG_ADC    0x04

class AD7417 {
public:
    AD7417() : _address(0) {}

    void begin(uint8_t address) { _address = address; }

    float readADC(int channel) {
        if (channel < 1 || channel > 4) return -1.0;
        writeRegister(AD7417_REG_CONFIG, channel << 5);
        Wire.beginTransmission(_address);
        Wire.write(AD7417_REG_ADC);
        Wire.endTransmission();
        float vref = (channel < 3) ? 2500.0 : 5000.0;
        return readResult(vref);
    }

    float readTemp() {
        writeRegister(AD7417_REG_CONFIG, 0x00);
        Wire.beginTransmission(_address);
        Wire.write(AD7417_REG_TEMP);
        Wire.endTransmission();
        Wire.requestFrom(_address, (uint8_t)2);
        if (Wire.available() == 2) {
            int16_t raw = (Wire.read() << 8) | Wire.read();
            raw >>= 6;
            if (raw >= 512) raw -= 1024;
            return raw * 0.25;
        }
        return -999.0;
    }

private:
    uint8_t _address;

    void writeRegister(uint8_t reg, uint8_t value) {
        Wire.beginTransmission(_address);
        Wire.write(reg);
        Wire.write(value);
        Wire.endTransmission();
    }

    float readResult(float vref) {
        Wire.requestFrom(_address, (uint8_t)2);
        if (Wire.available() == 2) {
            uint16_t raw = (Wire.read() << 8) | Wire.read();
            raw >>= 6;
            return (raw * vref) / 1024.0;
        }
        return -999.0;
    }
};
