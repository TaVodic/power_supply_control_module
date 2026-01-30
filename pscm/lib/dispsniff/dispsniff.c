#include "dispsniff.h"

#if defined(__AVR__)

#include <avr/interrupt.h>
#include <avr/io.h>
#include <math.h>
#include <util/atomic.h>

static volatile uint8_t rb[DISPSNIFF_RB_SZ]; // ring buffer, indexes -> 255+1=0 2-254=4
static volatile uint8_t rb_w = 0;            // ring buffer write index
static volatile uint8_t rb_r = 0;            // ring buffer read index

// Sniffer state
static volatile uint8_t in_frame = 0;     // are we currently in frame?
static volatile uint8_t bitcnt = 0;       // index in temp
static volatile uint8_t temp = 0;         // temp in ISR, when full (8b) then wrote to ring buffer
static volatile uint8_t pending_mark = 0; // 0=no event, 'S' or 'E'
static volatile uint8_t last_data_N = 0;

static inline uint8_t clk_level(void) { return (PIND >> DISPSNIFF_CLK_PIN_PORTD) & 1u; }
static inline uint8_t din_level(void) { return (PIND >> DISPSNIFF_DIN_PIN_PORTD) & 1u; }

static inline void rb_put(uint8_t v) {
  uint8_t n = (uint8_t)(rb_w + 1u);
  if (n == rb_r)
    return; // overflow: drop
  rb[rb_w] = v;
  rb_w = n;
}

uint8_t dispsniff_available(void) {
  if (in_frame)
    return 0;
  uint8_t w, r;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    w = rb_w;
    r = rb_r;
  }
  return (uint8_t)(w - r); // wraps naturally for uint8_t ring, otherwise it would be int so negative number and wrong result
}

uint8_t dispsniff_read(uint8_t *out) {
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

void dispsniff_flush(void) {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    rb_r = rb_w;
  }
}

// Change of DIN happens only on START or END
// INT1: DIN changed -> detect START/END when CLK high
ISR(INT1_vect) {
  if (!clk_level()) {
    return;
  }
  // If CLK high, START is DIN high->low; END is DIN low->high.
  if (!din_level()) { // START
    in_frame = 1;
    bitcnt = 0;
    temp = 0;
  } else { // END
    in_frame = 0;
    bitcnt = 0;
    temp = 0;
    if ((uint8_t)(rb_w - rb_r) - last_data_N >= 13) { // ensure only valid frames will cause old data overide
      rb_r = rb_r + last_data_N; // ensure only last byte is in mem
      last_data_N = (uint8_t)(rb_w - rb_r);
    }
  }
  PORTB &= ~(1u << PB0);
}

// INT0: CLK rising -> sample DIN, assemble byte LSB first
ISR(INT0_vect) {
  if (!in_frame)
    return;
  PORTB |= (1u << PB0);
  // Data captured on CLK rising edge; LSB first.
  uint8_t bit = din_level() ? 1u : 0u;
  temp |= (uint8_t)(bit << bitcnt);
  bitcnt++;

  if (bitcnt >= 8u) {
    rb_put(temp);
    temp = 0;
    bitcnt = 0;
  }
  PORTB &= ~(1u << PB0);
}

static void io_init(void) {
  // PD2/PD3 inputs, no pullups (avoid loading bus)
  DDRD &= (uint8_t)~((1u << DISPSNIFF_CLK_PIN_PORTD) | (1u << DISPSNIFF_DIN_PIN_PORTD));
  PORTD &= (uint8_t)~((1u << DISPSNIFF_CLK_PIN_PORTD) | (1u << DISPSNIFF_DIN_PIN_PORTD));

  // INT0 on rising edge (CLK)
  EICRA |= (1u << ISC01) | (1u << ISC00);

  // INT1 on any logical change (DIN)
  EICRA |= (1u << ISC10);
  EICRA &= (uint8_t)~(1u << ISC11);

  EIFR = (1u << INTF0) | (1u << INTF1); // clear pending
  EIMSK |= (1u << INT0) | (1u << INT1); // enable
}

static void io_deinit(void) { // to disable INT on pin INT1 and INT0
  EIMSK &= (uint8_t)~((1u << INT0) | (1u << INT1));
}

void dispsniff_begin(void) {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    rb_w = 0;
    rb_r = 0;
    in_frame = 0;
    bitcnt = 0;
    temp = 0;
  }
  io_init();
}

void dispsniff_end(void) {
  io_deinit();
}

#else
// Non-AVR fallback stubs (compile, but does nothing)
void dispsniff_begin(void) {}
void dispsniff_end(void) {}
uint8_t dispsniff_available(void) { return 0; }
uint8_t dispsniff_read(uint8_t *out) {
  (void)out;
  return 0;
}
void dispsniff_flush(void) {}
#endif
