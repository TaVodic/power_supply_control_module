#include "Encoder_mt.h"
#include "MCP4251.h"
#include "dispsniff.h"
#include "pscm.h"

#include <Arduino.h>
#include <stdint.h>

// #define MAIN_DEBUG

MCP4251 MCP_1(pin_MCP_CS1);
MCP4251 MCP_2(pin_MCP_CS2);
Encoder ENC_V;
Encoder ENC_C;

int32_t oldPosition_V = 0;
int32_t oldPosition_C = 0;

uint16_t voltage = 0;
uint16_t current = 0;
uint16_t power = 0;

uint8_t sw_V_oldState = 1;
uint16_t sw_V_lastMillis = 0;
uint8_t sw_C_oldState = 1;
uint16_t sw_C_lastMillis = 0;

enum state_t {
  FINE,
  COARSE
};

enum state_t state_V = COARSE;
enum state_t state_C = COARSE;

void io_init() {
  pinMode(pin_LED_V, OUTPUT);
  pinMode(pin_LED_C, OUTPUT);
  pinMode(pin_SW_V, INPUT_PULLUP);
  pinMode(pin_SW_C, INPUT_PULLUP);

  // debug pins
  pinMode(8, OUTPUT);  // debug pin PB0
  pinMode(A0, OUTPUT); // debug pin PC0

  *digitalPinToPCMSK(pin_SW_V) |= (1 << digitalPinToPCMSKbit(pin_SW_V)); // PCINT12 Pin Change Mask Register //PC4
  *digitalPinToPCMSK(pin_SW_C) |= (1 << digitalPinToPCMSKbit(pin_SW_C)); // PCINT13 Pin Change Mask Register //PC5
  PCICR |= (1 << digitalPinToPCICRbit(pin_SW_V));                        // Pin Change Interrupt Control Register (PCIE1)
  PCICR |= (1 << digitalPinToPCICRbit(pin_SW_C));                        // Pin Change Interrupt Control Register (PCIE1)
}

void setup() {
  MCP_1.begin();
  MCP_2.begin();
  MCP_1.DigitalPotSetWiperMin(0);
  MCP_1.DigitalPotSetWiperMin(1);
  MCP_2.DigitalPotSetWiperMin(0);
  MCP_2.DigitalPotSetWiperMin(1);

  io_init();

  Serial.begin(115200);
  Serial.println();
  Serial.println("TM1640 sniff start ");

  if (!ENC_V.begin(pin_ENC_V_A, pin_ENC_V_B)) {
    Serial.println("ENC_V init failed: only PD4,PD5,PD6,PD7 are supported!");
  }
  if (!ENC_C.begin(pin_ENC_C_A, pin_ENC_C_B)) {
    Serial.println("ENC_C init failed: only PD4,PD5,PD6,PD7 are supported!");
  }
  dispsniff_begin();
}

void loop() {

  /*uint16_t tcon = MCP_1.DigitalPotReadTconRegister();
  Serial.print("MCP_1 TCON= "); //511
  Serial.println(tcon);
  delay(1000);*/

  int32_t newPosition_V = ENC_V.read();
  if (newPosition_V != oldPosition_V) {
    /*int32_t diff = newPosition_V - oldPosition_V;
    if (state_V == COARSE) {      
      uint16_t wiper = MCP_1.DigitalPotReadWiperPosition(0);
      MCP_1.DigitalPotSetWiperPosition(0, diff * COARSE_RES + wiper);
      oldPosition_V = newPosition_V;
    }*/
    
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

  if (state_V == FINE) {
    digitalWrite(pin_LED_V, HIGH);
  } else {
    digitalWrite(pin_LED_V, LOW);
  }
  if (state_C == FINE) {
    digitalWrite(pin_LED_C, HIGH);
  } else {
    digitalWrite(pin_LED_C, LOW);
  }
}

ISR(PCINT1_vect) { // max 10us
#ifdef MAIN_DEBUG
  PORTC |= (1u << PC0);
#endif

  if (!READ_DPIN(pin_SW_V) && sw_V_oldState) {
    if (millis() > SW_DEBOUNCE_MS + sw_V_lastMillis) {
      state_V = (state_V == COARSE) ? FINE : COARSE;
      sw_V_oldState = 0;
      sw_V_lastMillis = millis();
    }
  } else if (READ_DPIN(pin_SW_V) && !sw_V_oldState) {
    sw_V_oldState = 1;
    sw_V_lastMillis = millis();
  }

  if (!READ_DPIN(pin_SW_C) && sw_C_oldState) {
    if (millis() > SW_DEBOUNCE_MS + sw_C_lastMillis) {
      state_C = (state_C == COARSE) ? FINE : COARSE;
#ifdef MAIN_DEBUG
      PORTB ^= (1u << PB0);
#endif
      sw_C_oldState = 0;
      sw_C_lastMillis = millis();
    }
  } else if (READ_DPIN(pin_SW_C) && !sw_C_oldState) {
    sw_C_oldState = 1;
    sw_C_lastMillis = millis();
  }
#ifdef MAIN_DEBUG
  PORTC &= ~(1u << PC0);
#endif
}