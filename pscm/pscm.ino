#include "dispsniff.h"
#include "pscm.h"
#include <Arduino.h>

#include <stdint.h>

static const uint16_t p10_lookup[4] = {1000, 100, 10, 1};
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
    if ((hex & ~(1 << 7)) == hex2int_lookup[i]) {
      return i;
    }
  }
  return 0xFF; // Return an error value if not found
}

uint8_t dispsniff_read_next_quartet(uint16_t *value) {
  uint8_t v;
  uint16_t _value = 0;
  for (uint8_t i = 0; i < 4; i++) {
    dispsniff_read(&v);
    // Serial.print(v);
    uint8_t val = hex2int(v);
    // Serial.print("=");
    // Serial.print(val);
    // Serial.print(" ");
    if (val == 0xFF) {
      Serial.print("BAD");
      return 0;
    }
    //(v & (1 << 7));
    _value = _value + (val * p10_lookup[i]);
  }
  // Serial.print("  V=");
  // Serial.println(value);
  *value = _value;
  return 1;
}

uint8_t dispsniff_poll(uint16_t *voltage, uint16_t *current, uint16_t *power) {
  uint8_t v;
  if (dispsniff_available() >= 14) {
    while (dispsniff_available()) {
      dispsniff_read(&v);
      //printHex(v);
      //Serial.print(" ");
      if (v == DATA_MARK) {
       Serial.println("START ");
        if (!dispsniff_read_next_quartet(voltage))
          return 0;
        Serial.print("  V=");
        Serial.print(*voltage);
        if (!dispsniff_read_next_quartet(current))
          return 0;
        //Serial.print("  A=");
        //Serial.print(*current);
        if (!dispsniff_read_next_quartet(power))
          return 0;
       // Serial.print("  W=");
        //Serial.println(*power);
      }
    }
  }
  return 1;
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
  uint16_t voltage;
  uint16_t current;
  uint16_t power;
  dispsniff_poll(&voltage, &current, &power);
  if (dispsniff_available()) {
    Serial.print("  V=");
    Serial.print(voltage);
    Serial.print("  A=");
    Serial.print(current);
    Serial.print("  W=");
    Serial.println(power);
  }
}