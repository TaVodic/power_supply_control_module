#include "Encoder_mt.h"
#include <util/atomic.h>

#define PIN_TO_BASEREG(pin)         (portInputRegister(digitalPinToPort(pin)))
#define PIN_TO_BITMASK(pin)         (digitalPinToBitMask(pin))
#define DIRECT_PIN_READ(base, mask) (((*(base)) & (mask)) ? 1 : 0)

#define digitalPinToPCINT(p) (((p) >= 8 && (p) <= 13) ? (0 + ((p) - 8)) : (((p) >= 20 && (p) <= 21) ? (0 + ((p) - 14)) : (((p) >= 14 && (p) <= 19) ? (8 + ((p) - 14)) : (((p) >= 0 && (p) <= 7) ? (16 + (p)) : (((p) >= 22 && (p) <= 25) ? (24 + ((p) - 22)) : 0)))))

#define ENC_DEB_US      300u
#define T1_TICKS_PER_US 2u // with prescaler = 8 at 16MHz
#define ENC_DEB_TICKS   (ENC_DEB_US * T1_TICKS_PER_US - 1)

Encoder_internal_state_t *Encoder::encVoltage = nullptr;
Encoder_internal_state_t *Encoder::encCurrent = nullptr;
static inline void timer1_init(void);
static volatile uint16_t enc_last_t1 = 0;

Encoder::Encoder() {}

uint8_t Encoder::begin(uint8_t pin1, uint8_t pin2) {

  if (pin1 < 4 || pin1 > 7) return false;
  if (pin2 < 4 || pin2 > 7) return false;
  if (pin1 == pin2) return false;

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

  if (pin1 == 4 || pin2 == 4) {
    encVoltage = &encoder;
  } else if (pin1 == 6 || pin2 == 6) {
    encCurrent = &encoder;
  } else {
    return false;
  }

  PCMSK2 |= (1 << PCINT4); // PCINT20 Pin Change Mask Register //PD4
  PCMSK2 |= (1 << PCINT5); // PCINT21 Pin Change Mask Register //PD5
  PCMSK2 |= (1 << PCINT6); // PCINT22 Pin Change Mask Register //PD6
  PCMSK2 |= (1 << PCINT7); // PCINT23 Pin Change Mask Register //PD7
  PCICR |= (1 << PCIE2);   // Pin Change Interrupt Control Register

  timer1_init();
  return true;
}

static inline void timer1_init(void) {
  TCCR1A = 0;                          // Reset TC1 Control Register A
  TCCR1B = 0;                          // Reset TC1 Control Register B
  TCCR1B = (1 << WGM12) | (1 << CS11); // CTC, prescaler = 8
  TCNT1 = 0;                           // TC1 Counter Value - reset the counter
  OCR1A = ENC_DEB_TICKS;               // 600us at 0.5us per tick
}

int32_t Encoder::read() {
  int32_t ret;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    ret = encoder.position;
  }
  return ret;
}

ISR(TIMER1_COMPA_vect) {
#ifdef ENC_DEBUG
  PORTC &= ~(1u << PC0);
#endif
  TIMSK1 &= ~(1 << OCIE1A); // turn off timer interrupt
  PCICR |= (1 << PCIE2);    // Pin Change Interrupt Control Register
}

ISR(PCINT2_vect) { // max 9.5us
#ifdef ENC_DEBUG
  PORTB |= (1u << PB0);
#endif

  Encoder::update(Encoder::encCurrent);
  Encoder::update(Encoder::encVoltage);

  PCICR &= ~(1 << PCIE2);  // Pin Change Interrupt Control Register - diable PCINT
  TIFR1 |= (1 << OCF1A);   // clear Interrupt Flag if pending
  TCNT1 = 0;               // TC1 Counter Value - reset the counter
  TIMSK1 |= (1 << OCIE1A); // TC1 Interrupt Mask Register - enable timer interrupt
#ifdef ENC_DEBUG
  PORTB &= ~(1u << PB0);
  PORTC |= (1u << PC0);
#endif
}

void Encoder::update(Encoder_internal_state_t *arg) {
  asm volatile(
      // X points to Encoder_internal_state_t
      // Layout (as used here) is effectively:
      //   [pin1_port_ptr][pin2_port_ptr][pin1_mask][pin2_mask][state][position (4 bytes little-endian)]
      //
      // We read the two input pins, build a 4-bit index:
      //   index = (prevAB in bits0..1) | (currA<<2) | (currB<<3)
      // then use a jump table to decide whether to change position.
      //
      // This modified version counts ONLY when currAB becomes 00,
      // giving 1 count per full 4-step quadrature cycle.

      // ---- Load pin1 port address from struct (2 bytes) into Z ----
      "ld  r30, X+               \n\t" // r30 = low byte of pin1 port address
      "ld  r31, X+               \n\t" // r31 = high byte of pin1 port address
      "ld  r24, Z                \n\t" // r24 = *pin1_port (raw port input)

      // ---- Load pin2 port address from struct (2 bytes) into Z ----
      "ld  r30, X+               \n\t" // r30 = low byte of pin2 port address
      "ld  r31, X+               \n\t" // r31 = high byte of pin2 port address
      "ld  r25, Z                \n\t" // r25 = *pin2_port (raw port input)

      // ---- Load masks and previous state ----
      "ld  r30, X+               \n\t" // r30 = pin1 mask
      "ld  r31, X+               \n\t" // r31 = pin2 mask
      "ld  r22, X                \n\t" // r22 = state (we only use bits0..1)

      // Keep only previous AB in bits0..1
      "andi  r22, 3              \n\t" // r22 = prevAB (0..3)

      // ---- Apply masks and OR current A/B into r22 (bits2..3) ----
      "and   r24, r30            \n\t" // r24 = pin1 & mask
      "breq  L%=A0               \n\t" // if zero -> A=0
      "ori   r22, 4              \n\t" // else set bit2 => currA=1
      "L%=A0:                    \n\t"

      "and   r25, r31            \n\t" // r25 = pin2 & mask
      "breq  L%=B0               \n\t" // if zero -> B=0
      "ori   r22, 8              \n\t" // else set bit3 => currB=1
      "L%=B0:                    \n\t"

      // ---- Jump-table dispatch ----
      // Z = &table
      "ldi   r30, lo8(pm(L%=table)) \n\t"
      "ldi   r31, hi8(pm(L%=table)) \n\t"

      // Add index (0..15) to table base so IJMP lands on the right RJMP
      "add   r30, r22            \n\t"
      "adc   r31, __zero_reg__   \n\t"

      // ---- Update stored state to current AB ----
      // currAB is in bits2..3 of r22; shift down into bits0..1
      "asr   r22                 \n\t"
      "asr   r22                 \n\t"
      "st    X+, r22             \n\t" // store new state (currAB)

      // ---- Load 4-byte position into r22..r25 ----
      "ld    r22, X+             \n\t"
      "ld    r23, X+             \n\t"
      "ld    r24, X+             \n\t"
      "ld    r25, X+             \n\t"

      // ---- Dispatch to one of the RJMPs below, then fall into store ----
      "ijmp                      \n\t"

      // =============================================================
      // Jump table (16 entries):
      // index = prevAB | (currAB<<2)
      //
      // We only count when currAB == 00 and the transition is valid:
      //   prev=01 curr=00  (index=1) => +1
      //   prev=10 curr=00  (index=2) => -1
      // Everything else => 0 (no count)
      // =============================================================
      "L%=table:                 \n\t"
      "rjmp  L%=end              \n\t" // 0:  00->00  no move
      "rjmp  L%=plus1            \n\t" // 1:  01->00  +1 (count here)
      "rjmp  L%=minus1           \n\t" // 2:  10->00  -1 (count here)
      "rjmp  L%=end              \n\t" // 3:  11->00  (skip/bounce) ignore

      "rjmp  L%=end              \n\t" // 4:  00->01  ignore
      "rjmp  L%=end              \n\t" // 5:  01->01  ignore
      "rjmp  L%=end              \n\t" // 6:  10->01  ignore
      "rjmp  L%=end              \n\t" // 7:  11->01  ignore

      "rjmp  L%=end              \n\t" // 8:  00->10  ignore
      "rjmp  L%=end              \n\t" // 9:  01->10  ignore
      "rjmp  L%=end              \n\t" // 10: 10->10  ignore
      "rjmp  L%=end              \n\t" // 11: 11->10  ignore

      "rjmp  L%=end              \n\t" // 12: 00->11  ignore
      "rjmp  L%=end              \n\t" // 13: 01->11  ignore
      "rjmp  L%=end              \n\t" // 14: 10->11  ignore
      "rjmp  L%=end              \n\t" // 15: 11->11  ignore

      // ---- position -= 1 ----
      "L%=minus1:                \n\t"
      // Check if r22..r25 == 0. If yes, ignore decrement.
      "mov   r30, r22            \n\t"
      "or    r30, r23            \n\t"
      "or    r30, r24            \n\t"
      "or    r30, r25            \n\t"
      "breq  L%=end              \n\t" // position already 0 -> do nothing

      // Not zero, so decrement 32-bit value in r22..r25
      "subi  r22, 1              \n\t"
      "sbci  r23, 0              \n\t"
      "sbci  r24, 0              \n\t"
      "sbci  r25, 0              \n\t"
      "rjmp  L%=store            \n\t"

      // ---- position += 1 ----
      "L%=plus1:                 \n\t"
      "subi  r22, 255            \n\t" // add 1 (two's complement trick)
      "sbci  r23, 255            \n\t"
      "sbci  r24, 255            \n\t"
      "sbci  r25, 255            \n\t"

      // ---- Store updated position back (pre-decrement X to original position field) ----
      "L%=store:                 \n\t"
      "st   -X, r25              \n\t"
      "st   -X, r24              \n\t"
      "st   -X, r23              \n\t"
      "st   -X, r22              \n\t"

      "L%=end:                   \n\t"
      "\n"
      :
      : "x"(arg)
      : "r22", "r23", "r24", "r25", "r30", "r31");
}
