#pragma once

#include <Arduino.h>
#include "utils.h"
#include "pcf8574A.h"

#define AD7417_ADR 0x28

#define REG_CONFIG         0x01
#define REG_TEMP           0x00
#define REG_ADC            0x04

// Função genérica para ler temperatura do AD7417
float read_temp(char FEB, int chip) {
  int nFEB = (int) FEB - 65;
  int addr = AD7417_ADR + 2*nFEB + chip;

  Wire.beginTransmission(addr);
  Wire.write(REG_CONFIG); // config register
  Wire.write(0x00);
  Wire.endTransmission();

  Wire.beginTransmission(addr);
  Wire.write(REG_TEMP); 
  Wire.endTransmission();

  Wire.requestFrom(addr, 2);

  if (Wire.available() == 2) {
    byte msb = Wire.read();
    byte lsb = Wire.read();

    // Combina os bytes e ajusta a justificativa (10 bits alinhados à esquerda)
    int raw = (msb << 8) | lsb;
    raw = raw >> 6; // Desloca 6 bits para alinhar os 10 bits à direita

    // Converte o complemento de dois de 10 bits para float
    if (raw >= 512) {
    raw = raw - 1024;
    }
    return raw * 0.25; // Cada LSB equivale a 0,25°C
  }
  return -999.0; // Erro de leitura
    

}

// Função genérica para ler um canal ADC específico (1 a 4 = AIN1 a AIN4)
float read_ADC(char FEB, int chip, int ch) {
  if (ch < 1 || ch > 4) return -1.0;

  uint8_t config_value = ch << 5;

  int nFEB = (int) FEB - 65;
  int addr = AD7417_ADR + 2*nFEB + chip;

  Wire.beginTransmission(addr);
  Wire.write(REG_CONFIG); // config register
  Wire.write(config_value);
  Wire.endTransmission();

  Wire.beginTransmission(addr);
  Wire.write(REG_ADC); 
  Wire.endTransmission();

  Wire.requestFrom(addr, 2);

  if (Wire.available() == 2) {
    byte msb = Wire.read();
    byte lsb = Wire.read();

    // Combina os bytes e ajusta a justificativa (10 bits alinhados à esquerda)
    int raw = (msb << 8) | lsb;
    raw = raw >> 6; // Desloca 6 bits para alinhar os 10 bits à direita

    float vref = ch < 3 ? 2.5 : 5.0; 
    return (raw * vref) * 1000 / 1024.0;
  }
  return -999.0; // Erro de leitura
    

}