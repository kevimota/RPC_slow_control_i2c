#pragma once

#include <Arduino.h>
#include "pcf8574A.h"
#include "utils.h"

#define AD5316_ADDR 0x0C

void write_dac(char FEB, int chip, int ch, float value) {
  check_feb(FEB, chip);
  if (ch < 1 || ch > 4) return;
  //en_dac(FEB, chip);
  //sel_dac(FEB, chip);
  ch -= 1;

  uint16_t dac_value = (uint16_t)((value * 1023.0 / 5000.0) + 0.5);

  //Serial.printf("DAC value:    0b%s\n", toBinary16(dac_value));

  if (dac_value > 1023) dac_value = 1023;

  // 1. Definição dos bits de configuração padrão:
  uint8_t gain = 0; // 0 = Ganho de 1x, 1 = Ganho de 2x
  uint8_t buf  = 0; // 0 = Sem buffer na referência, 1 = Com buffer
  uint8_t clr  = 1; // 1 = Operação normal (CLR ativo em nível baixo)
  uint8_t pd   = 1; // 1 = Operação normal (Power Down ativo em nível baixo)

  // 2. Montagem do Byte MSB (Byte 1)
  // Pegamos os 4 bits de configuração e deslocamos para suas posições mais altas (7, 6, 5 e 4)
  uint8_t msb = (gain << 7) | (buf << 6) | (clr << 5) | (pd << 4);
  
  // Pegamos os bits D9, D8, D7 e D6 do valor (que estão nas posições 9, 8, 7 e 6)
  // Deslocamos 6 posições para a direita para que fiquem nas posições 3, 2, 1 e 0
  msb |= (dac_value >> 6) & 0x0F;

  // 3. Montagem do Byte LSB (Byte 2)
  // Pegamos os bits restantes D5 a D0 (posições 5 a 0 do valor)
  // Deslocamos 2 posições para a esquerda para abrir espaço para os dois zeros finais
  uint8_t lsb = (dac_value << 2) & 0xFC; 

  //Serial.printf("MSB:          0b%s\n", toBinary8(msb));
  //Serial.printf("LSB:          0b%s\n", toBinary8(lsb));

  uint8_t channel_mask = 1 << ch;
  uint8_t control_byte = channel_mask & 0x0F;

  //Serial.printf("Control byte: 0b%s\n", toBinary8(control_byte));

  Wire.beginTransmission(AD5316_ADDR);
  Wire.write(control_byte); // Envia o 1º Byte (Controle)
  Wire.write(msb);     // Envia o 2º Byte (Dados MSB)
  Wire.write(lsb);     // Envia o 3º Byte (Dados LSB)
  Wire.endTransmission();

}
