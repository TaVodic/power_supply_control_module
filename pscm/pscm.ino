#include <Arduino.h>
#include <stdint.h>
#include "dispsniff.h"
#include "pscm.h"
#include "MCP4251.h"

MCP4251 MCP_1(pin_MCP_CS1);
MCP4251 MCP_2(pin_MCP_CS2);

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(8, OUTPUT);

  Serial.begin(115200);
  delay(50);
  Serial.println();
  Serial.println("TM1640 sniff start ");

  dispsniff_begin();
  MCP_1.begin();
  MCP_2.begin();
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