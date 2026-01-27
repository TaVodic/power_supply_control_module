#ifndef TM1640SNF_H
#define TM1640SNF_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Markers returned by tm1640snf_read()
#define TM1640SNF_MARK_S  ((uint8_t)'S')   // START
#define TM1640SNF_MARK_E  ((uint8_t)'E')   // END

// Init sniffer (uses PD2=INT0 as CLK, PD3=INT1 as DIN, no pullups).
void tm1640snf_begin(void);

// Stop sniffer (disables INT0/INT1).
void tm1640snf_end(void);

// How many bytes are currently buffered (0..255).
uint8_t tm1640snf_available(void);

// Read one byte/marker from buffer. Returns 1 if a byte was read, else 0.
uint8_t tm1640snf_read(uint8_t *out);

// Optional: clear buffered data.
void tm1640snf_flush(void);

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
