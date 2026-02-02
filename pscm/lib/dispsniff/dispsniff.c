#include "dispsniff.h"

#if defined(__AVR__)

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/atomic.h>

static const uint16_t p10_lookup[4] = {1000, 100, 10, 1};
static const uint8_t hex2int_lookup[10] = {
    0b00111111, // 0 63  0x3F
    0b00000110, // 1 6   0x06
    0b01011011, // 2 91  0x5B
    0b01001111, // 3 79  0x4F
    0b01100110, // 4 102 0x66
    0b01101101, // 5 109 0x6D
    0b01111101, // 6 125 0x7D
    0b00000111, // 7 7   0x07
    0b01111111, // 8 127 0x7F
    0b01101111, // 9 111 0x6F
};

static volatile uint8_t rb[DISPSNIFF_RB_SZ]; // ring buffer, indexes -> 255+1=0 2-254=4
static volatile uint8_t rb_w = 0;            // ring buffer write index
static volatile uint8_t rb_r = 0;            // ring buffer read index

// Sniffer state
static volatile uint8_t in_frame = 0; // are we currently in frame?
static volatile uint8_t bitcnt = 0;   // index in temp
static volatile uint8_t temp = 0;     // temp in ISR, when full (8b) then wrote to ring buffer

static inline uint8_t clk_level(void) { return (PIND >> DISPSNIFF_CLK_PIN_PORTD) & 1u; }
static inline uint8_t din_level(void) { return (PIND >> DISPSNIFF_DIN_PIN_PORTD) & 1u; }

uint8_t hex2int(uint8_t hex) {
  for (uint8_t i = 0; i < 10; i++) {
    if ((hex & ~(1 << 7)) == hex2int_lookup[i]) {
      return i;
    }
  }
  return 0xFF; // Return an error value if not found
}

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

uint8_t dispsniff_read_next_quartet(uint16_t *value) {
  uint8_t v;
  uint16_t _value = 0;
  for (uint8_t i = 0; i < 4; i++) {
    dispsniff_read(&v);
    // Serial.print(v);
    uint8_t val = hex2int(v);
    // Serial.print("=");
    // Serial.print(val);
    // Serial.print(" ");
    if (val == 0xFF) {
      // Serial.print("BAD");
      return 0;
    }
    //(v & (1 << 7));
    _value = _value + (val * p10_lookup[i]);
  }
  // Serial.print("  V=");
  // Serial.println(value);
  *value = _value;
  return 1;
}

uint8_t dispsniff_poll(uint16_t *voltage, uint16_t *current, uint16_t *power) {
  uint8_t v;
  uint8_t avail = dispsniff_available();
  if (avail >= 14) {
    while (avail) {
      dispsniff_read(&v);
      // printHex(v);
      // Serial.print(" ");
      if (v == DATA_MARK) {
        // Serial.println("START ");
        if (!dispsniff_read_next_quartet(voltage)) return 0;
        // Serial.print("  V=");
        // Serial.print(*voltage);
        if (!dispsniff_read_next_quartet(current)) return 0;
        // Serial.print("  A=");
        // Serial.print(*current);
        if (!dispsniff_read_next_quartet(power)) return 0;
        // Serial.print("  W=");
        // Serial.println(*power);
        return 2;
      }
    }
  }
  return 1;
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
  }
}

// INT0: CLK rising -> sample DIN, assemble byte LSB first
ISR(INT0_vect) {
  if (!in_frame)
    return;
#ifdef DISP_DEBUG
  PORTB |= (1u << PB0);
#endif
  // Data captured on CLK rising edge; LSB first.
  uint8_t bit = din_level() ? 1u : 0u;
  temp |= (uint8_t)(bit << bitcnt);
  bitcnt++;

  if (bitcnt >= 8u) {
    rb_put(temp);
    temp = 0;
    bitcnt = 0;
  }
#ifdef DISP_DEBUG
  PORTB &= ~(1u << PB0);
#endif
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

/*
static void printHex(uint8_t b) {
  const char hx[] = "0123456789ABCDEF";
  Serial.write(hx[(b >> 4) & 0x0F]);
  Serial.write(hx[b & 0x0F]);
}
*/

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
