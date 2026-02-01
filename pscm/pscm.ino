#include "Encoder.h"
//#include "MCP4251.h"
//#include "dispsniff.h"
#include "pscm.h"

#include <Arduino.h>
#include <stdint.h>

//MCP4251 MCP_1(pin_MCP_CS1);
//MCP4251 MCP_2(pin_MCP_CS2);
Encoder ENC_V(3, 2);
//Encoder ENC_C(pin_ENC_C_A, pin_ENC_C_B);

int16_t oldPosition = 0;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(8, OUTPUT);

  Serial.begin(115200);
  delay(50);
  Serial.println();
  Serial.println("TM1640 sniff start ");

  //dispsniff_begin();
  //MCP_1.begin();
  //MCP_2.begin();
}

void loop() {
  /*uint16_t voltage;
  uint16_t current;
  uint16_t power;
  if (dispsniff_poll(&voltage, &current, &power) == 2) {
    Serial.print("  V=");
    Serial.print(voltage);
    Serial.print("  A=");
    Serial.print(current);
    Serial.print("  W=");
    Serial.println(power);
  }*/

  long newPosition = ENC_V.read();
  if (newPosition != oldPosition) {
    oldPosition = newPosition;
    Serial.println(newPosition);
  }
}