#pragma once

#include <Arduino.h>
#include "ad7417.h"

void read_all_adc() {
  Serial.println("\n ------------- Reading the ADC -------------\n");
  for (int nFEB = 0; nFEB < 4; nFEB++) {
    char FEB = 65 + nFEB;
    for (int chip = 0; chip < 2; chip++) {
      for (int ch = 1; ch < 5; ch++) {
        float x = read_ADC(FEB, chip, ch);
        Serial.print("FEB ");
        Serial.print(FEB);
        Serial.printf(" chip %d, channel %d: %.0f mV\n", chip, ch, x);
        
      }
      Serial.println("");
    }
    Serial.println("");
  }
}

void read_feb_adc(char FEB) {
  Serial.println("\n ------------- Reading the ADC FEB -------------\n");
  for (int chip = 0; chip < 2; chip++) {
    for (int ch = 1; ch < 5; ch++) {
      float x = read_ADC(FEB, chip, ch);
      Serial.print("FEB ");
      Serial.print(FEB);
      Serial.printf(" chip %d, channel %d: %.0f mV\n", chip, ch, x);
      
    }
    Serial.println("");
  }
  Serial.println("");
  }

bool check_feb(char FEB, int chip) {
  if (FEB < 'A' || FEB > 'D') {
    Serial.println("FEB should be A, B, C, D");
    return false;
    }
  if (chip < 0 || chip > 1) {
    Serial.println("DAC can only be 0 or 1!!!");
    return false;
  }
  return true;
  
}

void scanner() {
  Serial.println("\nI2C Scanner");

  byte error, address;
  int nDevices;

  Serial.println("Scanning...");

  nDevices = 0;
  for(address = 1; address < 127; address++ )
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address<16)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.print(", 0b");
      if (address<16)
        Serial.print("00");
      Serial.print(address, BIN);
      Serial.println("  !");

      nDevices++;
    }
    else if (error==4)
    {
      Serial.print("Unknown error at address 0x");
      if (address<16)
        Serial.print("0");
      Serial.println(address,HEX);
    }
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");
}

char* toBinary8(uint8_t value) {
  static char buffer[9]; // 8 bits + null terminator
  for (int i = 7; i >= 0; i--) {
    buffer[7 - i] = (value & (1 << i)) ? '1' : '0';
  }
  buffer[8] = '\0';
  return buffer;
}

char* toBinary16(uint16_t valor) {
  static char buffer[17]; // 16 bits + 1 para o terminador '\0'
  
  for (int i = 15; i >= 0; i--) {
    // Verifica bit a bit do mais significativo (15) ao menos significativo (0)
    buffer[15 - i] = (valor & (1 << i)) ? '1' : '0';
  }
  buffer[16] = '\0'; // Finaliza a string do padrão C
  return buffer;
}
