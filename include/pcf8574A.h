#pragma once

#include <Arduino.h>
#include "utils.h"

#define PCF8574A_ADDR  0x38

char read_pcf(int nFEB) {
  int address = PCF8574A_ADDR + 2*nFEB;
  Wire.requestFrom(address, 1);
  char read;
  while (Wire.available()) {
    read = Wire.read();
  }
  return read;
}

void write_pcf(int nFEB, char value) {
  int address = PCF8574A_ADDR + 2*nFEB;
  Wire.beginTransmission(address);
  Wire.write(value);
  Wire.endTransmission();
}

void sel_dac(char FEB, int chip) {
  if (!check_feb(FEB, chip)) {
    return;
  }
  int nFEB = (int) FEB - 65;
  char read;
  for (int i = 0; i < 4; i++) {
    //Serial.println(i);
    read = read_pcf(i);
    //Serial.println(read, BIN);
    for (int j = 0; j < 2; j++) {
      if ((i == nFEB ) && (j == chip)) {
        read &= ~(1 << j);
      }
      else {
        read |= (1 << j);
      }
    }
    //Serial.println(read, BIN);
    write_pcf(i, read);
    read = read_pcf(i);
    //Serial.println(read, BIN);

  }
    
}

void dis_dac(char FEB, int chip) {
  if (!check_feb(FEB, chip)) {
    return;
  }
  int nFEB = (int) FEB - 65;
  char read = read_pcf(nFEB);
  read |= (1 << (2+chip));
  write_pcf(nFEB, read);
}

void en_dac(char FEB, int chip) {
  if (!check_feb(FEB, chip)) {
    return;
  }
  int nFEB = (int) FEB - 65;
  char read = read_pcf(nFEB);
  read &= ~(1 << (2+chip));
  write_pcf(nFEB, read);
    
}

void enable_all_dac() {
  for (char FEB = 'A'; FEB < 'E'; ++FEB) {
    for (int chip = 0; chip < 2; ++chip) {
      en_dac(FEB, chip);
    }
  }
}