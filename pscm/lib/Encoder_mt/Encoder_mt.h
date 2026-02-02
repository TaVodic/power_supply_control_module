#ifndef Encoder_mt_h_
#define Encoder_mt_h_

/*  Uses PCINT features hardcoded to PD4, PD5, PD6, PD7 
    Uses Timer1 for debouncing 
*/

#include "Arduino.h"

#define ENC_DEBUG

typedef struct { // Do not reorder fields; asm depends on this layout!
  volatile uint8_t *pin1_register;
  volatile uint8_t *pin2_register;
  uint8_t pin1_bitmask;
  uint8_t pin2_bitmask;
  uint8_t state;
  int32_t position; // crucial to stay after uint8_t state fields for asm code
} Encoder_internal_state_t;

class Encoder {
public:

  Encoder();

  uint8_t begin(uint8_t pin1, uint8_t pin2);
  int32_t read();

  // TODO: reduce array size to only two Encoders
  static Encoder_internal_state_t *interruptArgs[28]; // max 28 PCINT pins on ATmega328PB
  static void update(Encoder_internal_state_t *arg);

private:
  Encoder_internal_state_t encoder;
};

#endif