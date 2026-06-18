#include <Arduino.h>
#include <Wire.h>
#include "pcf8574A.h"
#include "ad7417.h"
#include "ad5316.h"
#include "utils.h"

unsigned long previousMillis = 0;
unsigned long delay_time = 5000;

void setup() {
  Wire.begin(10, 11, 100000);
  
  Serial.begin(115200);
  while (!Serial)
    delay(10);

  delay(1000);
  Serial.println("Initialized");

}

void send_dac() {
  long x = random(-1000, 1000);
  float voltage = 2500 + ((float)x);
  x = random(-20, 20);
  float th_voltage = 220 + ((float)x);
  Serial.printf("Voltage: %.0f\n", voltage);
  Serial.printf("th Voltage: %.0f\n", th_voltage);

  en_dac('C', 1);
  sel_dac('C', 1);
  write_dac('C', 1, 1, th_voltage);
  write_dac('C', 1, 2, th_voltage);
  write_dac('C', 1, 3, voltage);
  write_dac('C', 1, 4, voltage);

}

void loop() {
  if ((millis() - previousMillis) > delay_time)
  {
    previousMillis = millis();

    for (int i = 0; i < 4; ++i) {
      char ports = read_pcf(i);
      Serial.printf("FEB %c, PCF port = 0b%s\n", (char)i+65, toBinary8(ports));
    }
    //send_dac();
    read_all_adc();
  }

  
}
