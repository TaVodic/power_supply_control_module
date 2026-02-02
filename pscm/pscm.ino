#include "Encoder_mt.h"
#include "MCP4251.h"
#include "dispsniff.h"
#include "pscm.h"

#include <Arduino.h>
#include <stdint.h>

MCP4251 MCP_1(pin_MCP_CS1);
MCP4251 MCP_2(pin_MCP_CS2);
Encoder ENC_V;
Encoder ENC_C;

int32_t oldPosition_V = 0;
int32_t oldPosition_C = 0;

uint16_t voltage = 0;
uint16_t current = 0;
uint16_t power = 0;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(8, OUTPUT);  // debug pin PB0
  pinMode(A0, OUTPUT); // debug pin PC0

  Serial.begin(115200);
  delay(50);
  Serial.println();
  Serial.println("TM1640 sniff start ");

  dispsniff_begin();
  MCP_1.begin();
  MCP_2.begin();
  if (!ENC_V.begin(pin_ENC_V_A, pin_ENC_V_B)) {
    Serial.println("ENC_V init failed: only PD4,PD5,PD6,PD7 are supported!");
  }
  if (!ENC_C.begin(pin_ENC_C_A, pin_ENC_C_B)) {
    Serial.println("ENC_C init failed: only PD4,PD5,PD6,PD7 are supported!");
  }
}

void loop() {

  int32_t newPosition_V = ENC_V.read();
  if (newPosition_V != oldPosition_V) {
    oldPosition_V = newPosition_V;
    Serial.print("V: ");
    Serial.println(newPosition_V);
    /*dispsniff_poll(&voltage, &current, &power);
    Serial.print("V=");
    Serial.print(voltage);
    Serial.print("  A=");
    Serial.print(current);
    Serial.print("  W=");
    Serial.println(power);*/
  }
  int32_t newPosition_C = ENC_C.read();
  if (newPosition_C != oldPosition_C) {
    oldPosition_C = newPosition_C;
    Serial.print("C: ");
    Serial.println(newPosition_C);
    /*dispsniff_poll(&voltage, &current, &power);
    Serial.print("V=");
    Serial.print(voltage);
    Serial.print("  A=");
    Serial.print(current);
    Serial.print("  W=");
    Serial.println(power);*/
  }
}