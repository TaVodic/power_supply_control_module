#include <Arduino.h>
#include "pscm.h"
#include "dispsniff.h"

static const uint8_t hex2int_lookup[10] = {
    0b00111111, // 0
    0b00000110, // 1
    0b01011011, // 2
    0b01001111, // 3
    0b01100110, // 4
    0b01101101, // 5
    0b01111101, // 6
    0b00000111, // 7
    0b01111111, // 8
    0b01101111, // 9
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
  if (voltage != 0xFFFF){
    Serial.println(voltage);  
  }  
}