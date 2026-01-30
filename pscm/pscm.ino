#include "dispsniff.h"
#include "pscm.h"
#include <Arduino.h>

static const uint8_t hex2int_lookup[10] = {
    0b00111111, // 0 63  0x3F
    0b00000110, // 1 6   0x06
    0b01011011, // 2 91  0x5B
    0b01001111, // 3 79  0x4F
    0b01100110, // 4 102 0x66
    0b01101101, // 5 109 0x6D
    0b01111101, // 6 125 0x7D
    0b00000111, // 7 7   0x07
    0b01111111, // 8 127 0x7F
    0b01101111, // 9 111 0x6F
};

uint8_t hex2int(uint8_t hex) {
  for (uint8_t i = 0; i < 10; i++) {
    if (hex == hex2int_lookup[i]) {
      return i;
    }
  }
  return 0xFF; // Return an error value if not found
}

uint16_t dispsniff_poll(void) {
  uint16_t voltage_int = 0xFFFF;
  uint8_t v;
  if (dispsniff_read(&v)) {
    Serial.print(v);
    if (v == DATA_MARK) {
      Serial.print("START");
      voltage_int = 0;
      uint8_t count = 4;
    }
    for (uint8_t i = 0; i < 10; i++) {
      if ((v & ~(1 << 7)) == hex2int_lookup[i]) {
        Serial.print("=");
        Serial.print(i);
      }
    }
    Serial.print(" ");

    if (v == 0x8A) {
      Serial.println();
    }
  }

  return 0;
  uint16_t voltage_int = 0xFFFF;

  if (dispsniff_read(&v)) {
    printHex(v);
    Serial.print(".");
  }
  if (v == DATA_MARK) {
    voltage_int = 0;
    uint8_t count = 4;
    while (count > 0 && count <= 4) {
      dispsniff_read(&v);
      printHex(v);
      Serial.print(" ");
      uint8_t val = hex2int(v);
      if (val == 0xFF) {
        Serial.println("-");
        return; // TODO
      }
      count--;
      voltage_int = voltage_int + (val * pow(10, count));
    }
    Serial.println();
  }
  return voltage_int;
}

static void printHex(uint8_t b) {
  const char hx[] = "0123456789ABCDEF";
  Serial.write(hx[(b >> 4) & 0x0F]);
  Serial.write(hx[b & 0x0F]);
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(8, OUTPUT);

  Serial.begin(115200);
  delay(50);
  Serial.println();
  Serial.println("TM1640 sniff start ");

  dispsniff_begin();
}

void loop() {
  uint16_t voltage = dispsniff_poll();
  /*if (voltage != 0xFFFF){
    Serial.println(voltage);
  }  */
}