#pragma once

#include <Arduino.h>
#include <Wire.h>

#define AD5316_ADDR 0x0C
#define AD5316_MAX_CODE 1023
#define AD5316_VREF_MV 5000.0

class AD5316 {
public:
    static void writeRaw(int channel, uint16_t dacCode) {
        if (channel < 0 || channel > 3) return;
        if (dacCode > AD5316_MAX_CODE) dacCode = AD5316_MAX_CODE;

        uint8_t gain = 0, buf = 0, clr = 1, pd = 1;
        uint8_t msb = (gain << 7) | (buf << 6) | (clr << 5) | (pd << 4);
        msb |= (dacCode >> 6) & 0x0F;

        uint8_t lsb = (dacCode << 2) & 0xFC;

        Wire.beginTransmission(AD5316_ADDR);
        Wire.write((1 << channel) & 0x0F);
        Wire.write(msb);
        Wire.write(lsb);
        Wire.endTransmission();
    }

    static uint16_t voltageToCode(float voltageMV) {
        if (voltageMV < 0) voltageMV = 0;
        uint16_t code = (uint16_t)((voltageMV * AD5316_MAX_CODE / AD5316_VREF_MV) + 0.5);
        if (code > AD5316_MAX_CODE) code = AD5316_MAX_CODE;
        return code;
    }

    static float codeToVoltage(uint16_t code) {
        return (code * AD5316_VREF_MV) / AD5316_MAX_CODE;
    }
};
