#include "tm1640snf.h"

#if defined(__AVR__)

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

// Fixed pins for classic ATmega328P/328PB Arduino boards:
// CLK -> PD2 (INT0 / Arduino D2)
// DIN -> PD3 (INT1 / Arduino D3)
#ifndef TM1640SNF_CLK_PIN
#define TM1640SNF_CLK_PIN   PD2
#endif

#ifndef TM1640SNF_DIN_PIN
#define TM1640SNF_DIN_PIN   PD3
#endif

#ifndef TM1640SNF_RB_SZ
#define TM1640SNF_RB_SZ 256
#endif

static volatile uint8_t rb[TM1640SNF_RB_SZ];
static volatile uint8_t rb_w = 0;
static volatile uint8_t rb_r = 0;

// Sniffer state
static volatile uint8_t in_frame = 0;
static volatile uint8_t bitcnt   = 0;
static volatile uint8_t cur      = 0;

static inline uint8_t clk_level(void) { return (PIND & (uint8_t)(1u << TM1640SNF_CLK_PIN)) ? 1u : 0u; }
static inline uint8_t din_level(void) { return (PIND & (uint8_t)(1u << TM1640SNF_DIN_PIN)) ? 1u : 0u; }

static inline void rb_put(uint8_t v) {
  uint8_t n = (uint8_t)(rb_w + 1u);
  if (n == rb_r) return;           // overflow: drop
  rb[rb_w] = v;
  rb_w = n;
}

uint8_t tm1640snf_available(void) {
  uint8_t w, r;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    w = rb_w;
    r = rb_r;
  }
  return (uint8_t)(w - r);         // wraps naturally for uint8_t ring
}

uint8_t tm1640snf_read(uint8_t *out) {
  uint8_t ok = 0;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    if (rb_r != rb_w) {
      *out = rb[rb_r];
      rb_r = (uint8_t)(rb_r + 1u);
      ok = 1;
    }
  }
  return ok;
}

void tm1640snf_flush(void) {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    rb_r = rb_w;
  }
}

// INT1: DIN changed -> detect START/END when CLK high
ISR(INT1_vect) {
  if (!clk_level()) return;

  // If CLK high, START is DIN high->low; END is DIN low->high.
  if (!din_level()) {
    // START
    in_frame = 1;
    bitcnt = 0;
    cur = 0;
    rb_put((uint8_t)'S');
  } else {
    // END
    if (in_frame) rb_put((uint8_t)'E');
    in_frame = 0;
    bitcnt = 0;
    cur = 0;
  }
}

// INT0: CLK rising -> sample DIN, assemble byte LSB first
ISR(INT0_vect) {
  if (!in_frame) return;

  // Data captured on CLK rising edge; LSB first.
  uint8_t bit = din_level() ? 1u : 0u;
  cur |= (uint8_t)(bit << bitcnt);
  bitcnt++;

  if (bitcnt >= 8u) {
    rb_put(cur);
    cur = 0;
    bitcnt = 0;
  }
}

static void ints_init(void) {
  // PD2/PD3 inputs, no pullups (avoid loading bus)
  DDRD  &= (uint8_t)~((1u << TM1640SNF_CLK_PIN) | (1u << TM1640SNF_DIN_PIN));
  PORTD &= (uint8_t)~((1u << TM1640SNF_CLK_PIN) | (1u << TM1640SNF_DIN_PIN));

  // INT0 on rising edge (CLK)
  EICRA |= (1u << ISC01) | (1u << ISC00);

  // INT1 on any logical change (DIN)
  EICRA |= (1u << ISC10);
  EICRA &= (uint8_t)~(1u << ISC11);

  EIFR = (1u << INTF0) | (1u << INTF1);   // clear pending
  EIMSK |= (1u << INT0) | (1u << INT1);   // enable
}

static void ints_deinit(void) {
  EIMSK &= (uint8_t)~((1u << INT0) | (1u << INT1));
}

void tm1640snf_begin(void) {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    rb_w = 0;
    rb_r = 0;
    in_frame = 0;
    bitcnt = 0;
    cur = 0;
  }
  ints_init();
}

void tm1640snf_end(void) {
  ints_deinit();
}

#else
// Non-AVR fallback stubs (compile, but does nothing)
void tm1640snf_begin(void) {}
void tm1640snf_end(void) {}
uint8_t tm1640snf_available(void) { return 0; }
uint8_t tm1640snf_read(uint8_t *out) { (void)out; return 0; }
void tm1640snf_flush(void) {}
#endif
