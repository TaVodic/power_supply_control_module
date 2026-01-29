#ifndef DISPSNIFF_H
#define DISPSNIFF_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Fixed pins for classic ATmega328P/328PB Arduino boards:
// CLK -> PD2 (INT0 / Arduino D2)
// DIN -> PD3 (INT1 / Arduino D3)
#ifndef DISPSNIFF_CLK_PIN_PORTD
#define DISPSNIFF_CLK_PIN_PORTD PD2
#endif

#ifndef DISPSNIFF_DIN_PIN_PORTD
#define DISPSNIFF_DIN_PIN_PORTD PD3
#endif

// Ring buffer size
#ifndef DISPSNIFF_RB_SZ
#define DISPSNIFF_RB_SZ 256
#endif

// Markers returned by dispsniff_read()
#define START_MARK ((uint8_t)0x40) // Data command setting -> Address auto + 1
#define END_MARK   ((uint8_t)0x8A) // Display control -> Set pulse width to 4/16
#define DATA_MARK  ((uint8_t)0xC0) // Address command setting -> Display address 00H

// Init sniffer (uses PD2=INT0 as CLK, PD3=INT1 as DIN, no pullups).
void dispsniff_begin(void);

// Stop sniffer (disables INT0/INT1).
void dispsniff_end(void);

// How many bytes are currently buffered (0..255).
uint8_t dispsniff_available(void);

// Read one byte/marker from buffer. Returns 1 if a byte was read, else 0.
uint8_t dispsniff_read(uint8_t *out);

// Optional: clear buffered data.
void dispsniff_flush(void);

uint16_t dispsniff_poll(void);

#ifdef __cplusplus
}
#endif

#endif

/*

Example output 00.00 0.000 0.000
[S] 40
[S] C0 3F BF 3F 3F BF 3F 3F 3F BF 3F 3F 3F 00
[S] 8A  [E]

 */
