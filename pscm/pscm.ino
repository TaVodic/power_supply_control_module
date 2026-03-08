#include "Encoder_mt.h"
#include "MCP4251.h"
#include "dispsniff.h"
#include "pscm.h"

#include <Arduino.h>
#include <stdint.h>
#include <util/atomic.h>

#define MAIN_DEBUG

MCP4251 MCP_1(pin_MCP_CS1);
MCP4251 MCP_2(pin_MCP_CS2);
Encoder ENC_V;
Encoder ENC_C;

uint16_t meas_voltage = 0;
uint16_t meas_current = 0;
uint16_t meas_power = 0;

enum mode_t : uint8_t {
  FINE,
  COARSE
};

enum MCP_adddr : uint8_t {
  CURRENT = 0,
  VOLTAGE = 1,
};

struct el_quantity_t {
  Encoder *ENC;
  MCP_adddr mcp_addr;
  int16_t old_enc_position = 0;
  volatile uint8_t btn_oldState = 1;
  volatile uint32_t btn_lastMillis = 0;
  volatile mode_t mode = COARSE;
};

el_quantity_t Voltage = {&ENC_V, VOLTAGE};
el_quantity_t Current = {&ENC_C, CURRENT};

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
  MCP_2.DigitalPotSetWiperMin(0); // short-circuit DPOT2
  MCP_2.DigitalPotSetWiperMin(1); // short-circuit DPOT2

  io_init();

  Serial.begin(115200);
  Serial.println();
  Serial.println("TM1640 sniff start ");

  if (!Voltage.ENC->begin(pin_ENC_V_B, pin_ENC_V_A)) {
    Serial.println("ENC_V init failed: only PD4,PD5,PD6,PD7 are supported!");
  }
  if (!Current.ENC->begin(pin_ENC_C_B, pin_ENC_C_A)) {
    Serial.println("ENC_C init failed: only PD4,PD5,PD6,PD7 are supported!");
  }
  dispsniff_begin();

  for (uint8_t i = 0; i < 3; i++) {
    digitalWrite(pin_LED_V, HIGH);
    digitalWrite(pin_LED_C, HIGH);
    delay(100);
    digitalWrite(pin_LED_V, LOW);
    digitalWrite(pin_LED_C, LOW);
    delay(100);
  }

  MCP_1.DigitalPotSetWiperMin(0);
  MCP_1.DigitalPotSetWiperMin(1);
  MCP_2.DigitalPotSetWiperMin(0);
  MCP_2.DigitalPotSetWiperMin(1);

  int16_t wiper = MCP_1.DigitalPotReadWiperPosition(Voltage.mcp_addr);
#ifdef MAIN_DEBUG
  Serial.print("init wiper: ");
  Serial.println(wiper);
#endif
}

void loop() {
  dispsniff_poll(&meas_voltage, &meas_current, &meas_power);
  static uint32_t cmillis = 0;
  if (millis() > 1000 + cmillis) {    
    Serial.print("V=");
    Serial.print(meas_voltage);
    Serial.print("  A=");
    Serial.print(meas_current);
    Serial.print("  W=");
    Serial.println(meas_power);
    cmillis = millis();
  }

  set_el_quantity(&Voltage);
  set_el_quantity(&Current);

  if (Voltage.mode == FINE) {
    digitalWrite(pin_LED_V, HIGH);
  } else {
    digitalWrite(pin_LED_V, LOW);
  }
  if (Current.mode == FINE) {
    digitalWrite(pin_LED_C, HIGH);
  } else {
    digitalWrite(pin_LED_C, LOW);
  }
}

void set_el_quantity(el_quantity_t *quantity) {
  int16_t newPosition = (int16_t)quantity->ENC->read();
  int16_t diff = newPosition - quantity->old_enc_position;
  int16_t tempWiper;
  int16_t wiper_1 = MCP_1.DigitalPotReadWiperPosition(quantity->mcp_addr);
  int16_t wiper_2 = MCP_2.DigitalPotReadWiperPosition(quantity->mcp_addr);
  int16_t wiper = wiper_1 + wiper_2;

  if (diff != 0) {
#ifdef MAIN_DEBUG
    Serial.print("read wiper_1: ");
    Serial.println(wiper_1);
    Serial.print("read wiper_2: ");
    Serial.println(wiper_2);
    Serial.print("read wiper_sum: ");
    Serial.println(wiper);
#endif
    if (quantity->mode == COARSE) {
      diff *= COARSE_RES;
    }
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      tempWiper = wiper + diff; // TODO: wiper get stuc when we chnage mode
    }
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      if (tempWiper < 0) {
        wiper = 0;
      } else if (tempWiper > WIPERS_MAX_VAL) {
        wiper = WIPERS_MAX_VAL;
      } else {
        wiper = tempWiper;
      }
    }
#ifdef MAIN_DEBUG
    Serial.print("wiper: ");
    Serial.println(wiper);
#endif
    quantity->old_enc_position = newPosition;
    if (wiper > WIPER_MAX_VAL) {
      MCP_1.DigitalPotSetWiperMax(quantity->mcp_addr);
      MCP_2.DigitalPotSetWiperPosition(quantity->mcp_addr, wiper - 256);
    } else {
      MCP_1.DigitalPotSetWiperPosition(quantity->mcp_addr, wiper);
      MCP_2.DigitalPotSetWiperMin(quantity->mcp_addr);
    }
  }
}

ISR(PCINT1_vect) { // max 10us
#ifdef MAIN_DEBUG
  PORTC |= (1u << PC0);
#endif

  if (!READ_DPIN(pin_SW_V) && Voltage.btn_oldState) {
    if (millis() > BTN_DEBOUNCE_MS + Voltage.btn_lastMillis) {
      Voltage.mode = (Voltage.mode == COARSE) ? FINE : COARSE;
      Voltage.btn_oldState = 0;
      Voltage.btn_lastMillis = millis();
    }
  } else if (READ_DPIN(pin_SW_V) && !Voltage.btn_oldState) {
    Voltage.btn_oldState = 1;
    Voltage.btn_lastMillis = millis();
  }

  if (!READ_DPIN(pin_SW_C) && Current.btn_oldState) {
    if (millis() > BTN_DEBOUNCE_MS + Current.btn_lastMillis) {
      Current.mode = (Current.mode == COARSE) ? FINE : COARSE;
#ifdef MAIN_DEBUG
      PORTB ^= (1u << PB0);
#endif
      Current.btn_oldState = 0;
      Current.btn_lastMillis = millis();
    }
  } else if (READ_DPIN(pin_SW_C) && !Current.btn_oldState) {
    Current.btn_oldState = 1;
    Current.btn_lastMillis = millis();
  }
#ifdef MAIN_DEBUG
  PORTC &= ~(1u << PC0);
#endif
}