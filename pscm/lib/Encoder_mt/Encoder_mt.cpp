#include "Encoder_mt.h"
#include <util/atomic.h>

#define PIN_TO_BASEREG(pin)         (portInputRegister(digitalPinToPort(pin)))
#define PIN_TO_BITMASK(pin)         (digitalPinToBitMask(pin))
#define DIRECT_PIN_READ(base, mask) (((*(base)) & (mask)) ? 1 : 0)

#define digitalPinToPCINT(p) (((p) >= 8 && (p) <= 13) ? (0 + ((p) - 8)) : (((p) >= 20 && (p) <= 21) ? (0 + ((p) - 14)) : (((p) >= 14 && (p) <= 19) ? (8 + ((p) - 14)) : (((p) >= 0 && (p) <= 7) ? (16 + (p)) : (((p) >= 22 && (p) <= 25) ? (24 + ((p) - 22)) : 0)))))

#define ENC_DEB_US      600u
#define T1_TICKS_PER_US 2u // with prescaler = 8 at 16MHz
#define ENC_DEB_TICKS   (ENC_DEB_US * T1_TICKS_PER_US)

Encoder_internal_state_t *Encoder::interruptArgs[28] = {nullptr};
static inline void t1_init_freerun(void);

static volatile uint16_t enc_last_t1 = 0;

Encoder::Encoder(uint8_t pin1, uint8_t pin2) {
  // TODO: allow only specific pins PD4, PD5, PD6, PD7
  this->pin1 = pin1;
  this->pin2 = pin2;
}

void Encoder::begin() {
  pinMode(pin1, INPUT_PULLUP);
  pinMode(pin2, INPUT_PULLUP);

  encoder.pin1_register = PIN_TO_BASEREG(pin1);
  encoder.pin1_bitmask = PIN_TO_BITMASK(pin1);
  encoder.pin2_register = PIN_TO_BASEREG(pin2);
  encoder.pin2_bitmask = PIN_TO_BITMASK(pin2);
  encoder.position = 0;

  uint8_t state = 0;
  if (DIRECT_PIN_READ(encoder.pin1_register, encoder.pin1_bitmask)) {
    state |= 1;
  }
  if (DIRECT_PIN_READ(encoder.pin2_register, encoder.pin2_bitmask)) {
    state |= 2;
  }
  encoder.state = state;

  interruptArgs[digitalPinToPCINT(pin1)] = &encoder; // assuming using first interrupt slot
  interruptArgs[digitalPinToPCINT(pin2)] = &encoder; // assuming using second interrupt slot

  PCMSK2 |= (1 << PCINT4); // PCINT20 Pin Change Mask Register //PD4
  PCMSK2 |= (1 << PCINT5); // PCINT21 Pin Change Mask Register //PD5
  PCMSK2 |= (1 << PCINT6); // PCINT22 Pin Change Mask Register //PD6
  PCMSK2 |= (1 << PCINT7); // PCINT23 Pin Change Mask Register //PD7
  PCICR |= (1 << PCIE2);   // Pin Change Interrupt Control Register

  t1_init_freerun();

#ifdef ENC_DEBUG
  Serial.println("Encoder initialized");
  Serial.print("PCINT20/22:");
  Serial.println(digitalPinToPCINT(pin1));
#endif
}

static inline void t1_init_freerun(void) {
  TCCR1A = 0;           // TC1 Control Register A (disable OC pins)
  TCCR1B = (1 << CS11); // prescaler = 8 - 0.5us per tick at 16MHz
  TCNT1 = 0;            // TC1 Counter Value - reset the counter
}

static inline uint8_t enc_debounce_pass(void) {
  uint16_t now = TCNT1;
  uint16_t dt = (uint16_t)(now - enc_last_t1); // wrap-safe
  if (dt < (uint16_t)ENC_DEB_TICKS)
    return 0;
  enc_last_t1 = now;
  return 1;
}

int32_t Encoder::read() {
  int32_t ret;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    update(&encoder);
    ret = encoder.position;
  }
  return ret;
}

ISR(PCINT2_vect) {
#ifdef ENC_DEBUG
  PORTB |= (1u << PB0);
#endif
  if (!enc_debounce_pass()) {
#ifdef ENC_DEBUG
    PORTB &= ~(1u << PB0);
#endif
    return;
  }

  Encoder_internal_state_t *e20 = Encoder::interruptArgs[20];
  if (e20) {
    Encoder::update(e20);
  }
  Encoder_internal_state_t *e22 = Encoder::interruptArgs[22];
  if (e22) {
    Encoder::update(e22);
  }
#ifdef ENC_DEBUG
  PORTB &= ~(1u << PB0);
#endif
}

void Encoder::update(Encoder_internal_state_t *arg) {
  asm volatile(
      "ld	r30, X+"
      "\n\t"
      "ld	r31, X+"
      "\n\t"
      "ld	r24, Z"
      "\n\t" // r24 = pin1 input
      "ld	r30, X+"
      "\n\t"
      "ld	r31, X+"
      "\n\t"
      "ld	r25, Z"
      "\n\t" // r25 = pin2 input
      "ld	r30, X+"
      "\n\t" // r30 = pin1 mask
      "ld	r31, X+"
      "\n\t" // r31 = pin2 mask
      "ld	r22, X"
      "\n\t" // r22 = state
      "andi	r22, 3"
      "\n\t"
      "and	r24, r30"
      "\n\t"
      "breq	L%=1"
      "\n\t" // if (pin1)
      "ori	r22, 4"
      "\n\t" //	state |= 4
      "L%=1:"
      "and	r25, r31"
      "\n\t"
      "breq	L%=2"
      "\n\t" // if (pin2)
      "ori	r22, 8"
      "\n\t" //	state |= 8
      "L%=2:"
      "ldi	r30, lo8(pm(L%=table))"
      "\n\t"
      "ldi	r31, hi8(pm(L%=table))"
      "\n\t"
      "add	r30, r22"
      "\n\t"
      "adc	r31, __zero_reg__"
      "\n\t"
      "asr	r22"
      "\n\t"
      "asr	r22"
      "\n\t"
      "st	X+, r22"
      "\n\t" // store new state
      "ld	r22, X+"
      "\n\t"
      "ld	r23, X+"
      "\n\t"
      "ld	r24, X+"
      "\n\t"
      "ld	r25, X+"
      "\n\t"
      "ijmp"
      "\n\t" // jumps to update_finishup()
             // TODO move this table to another static function,
             // so it doesn't get needlessly duplicated.  Easier
             // said than done, due to linker issues and inlining
      "L%=table:"
      "\n\t"
      "rjmp	L%=end"
      "\n\t" // 0
      "rjmp	L%=plus1"
      "\n\t" // 1
      "rjmp	L%=minus1"
      "\n\t" // 2
      "rjmp	L%=plus2"
      "\n\t" // 3
      "rjmp	L%=minus1"
      "\n\t" // 4
      "rjmp	L%=end"
      "\n\t" // 5
      "rjmp	L%=minus2"
      "\n\t" // 6
      "rjmp	L%=plus1"
      "\n\t" // 7
      "rjmp	L%=plus1"
      "\n\t" // 8
      "rjmp	L%=minus2"
      "\n\t" // 9
      "rjmp	L%=end"
      "\n\t" // 10
      "rjmp	L%=minus1"
      "\n\t" // 11
      "rjmp	L%=plus2"
      "\n\t" // 12
      "rjmp	L%=minus1"
      "\n\t" // 13
      "rjmp	L%=plus1"
      "\n\t" // 14
      "rjmp	L%=end"
      "\n\t" // 15
      "L%=minus2:"
      "\n\t"
      "subi	r22, 2"
      "\n\t"
      "sbci	r23, 0"
      "\n\t"
      "sbci	r24, 0"
      "\n\t"
      "sbci	r25, 0"
      "\n\t"
      "rjmp	L%=store"
      "\n\t"
      "L%=minus1:"
      "\n\t"
      "subi	r22, 1"
      "\n\t"
      "sbci	r23, 0"
      "\n\t"
      "sbci	r24, 0"
      "\n\t"
      "sbci	r25, 0"
      "\n\t"
      "rjmp	L%=store"
      "\n\t"
      "L%=plus2:"
      "\n\t"
      "subi	r22, 254"
      "\n\t"
      "rjmp	L%=z"
      "\n\t"
      "L%=plus1:"
      "\n\t"
      "subi	r22, 255"
      "\n\t"
      "L%=z:"
      "sbci	r23, 255"
      "\n\t"
      "sbci	r24, 255"
      "\n\t"
      "sbci	r25, 255"
      "\n\t"
      "L%=store:"
      "\n\t"
      "st	-X, r25"
      "\n\t"
      "st	-X, r24"
      "\n\t"
      "st	-X, r23"
      "\n\t"
      "st	-X, r22"
      "\n\t"
      "L%=end:"
      "\n" : : "x"(arg) : "r22",
                          "r23", "r24", "r25", "r30", "r31");
}