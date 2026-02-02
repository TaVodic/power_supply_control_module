#ifndef Encoder_mt_h_
#define Encoder_mt_h_

#include "Arduino.h"

typedef struct {
  volatile uint8_t *pin1_register;
  volatile uint8_t *pin2_register;
  uint8_t pin1_bitmask;
  uint8_t pin2_bitmask;
  uint8_t state;
  int32_t position;
} Encoder_internal_state_t;

class Encoder {
public:
  // one step setup like before
  Encoder(uint8_t pin1, uint8_t pin2);

  void begin();
  int32_t read();

  // TODO: reduce array size to only two Encoders
  static Encoder_internal_state_t *interruptArgs[28]; // max 28 PCINT pins on ATmega328PB
  static void update(Encoder_internal_state_t *arg);

private:   
  Encoder_internal_state_t encoder;
  uint8_t pin1;
  uint8_t pin2;
};

#endif