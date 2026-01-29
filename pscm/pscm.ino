#include <Arduino.h>
#include "pscm.h"
#include "dispsniff.h"


static void prHex8(uint8_t b) {
  const char hx[] = "0123456789ABCDEF";
  Serial.write(hx[(b >> 4) & 0x0F]);
  Serial.write(hx[b & 0x0F]);
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(115200);
  delay(50);
  Serial.println();
  Serial.println("TM1640 sniff start ");

  dispsniff_begin();
}

void loop() {
  uint8_t v;
  if (dispsniff_read(&v)) {
    if (v == START_MARK) {
      Serial.print("\r\n[S] ");
    } else if (v == END_MARK) {
      Serial.print(" [E]\r\n");
    } else {
      prHex8(v);
      Serial.write(' ');
    }
  }
}