#include "dispsniff.h"
#include "pscm.h"
#include <Arduino.h>
#include <stdint.h>

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
  if (dispsniff_poll(&voltage, &current, &power) == 2) {
    Serial.print("  V=");
    Serial.print(voltage);
    Serial.print("  A=");
    Serial.print(current);
    Serial.print("  W=");
    Serial.println(power);
  }
}