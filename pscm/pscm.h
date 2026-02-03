#ifndef PSCM_H
#define PSCM_H

#ifdef __cplusplus
extern "C" {
#endif

#define READ_DPIN(dp) ( *portInputRegister(digitalPinToPort(dp)) & digitalPinToBitMask(dp) )

#define SW_DEBOUNCE_MS 80u

#define pin_MCP_CS1 9  // PB1
#define pin_MCP_CS2 10 // PB2
#define pin_MOSI    11 // PB3
#define pin_MISO    12 // PB4
#define pin_SCK     13 // PB5
#define pin_LED_V   16 // PC2
#define pin_LED_C   17 // PC3
#define pin_SW_V    18 // PC4 PCINT12
#define pin_SW_C    19 // PC5 PCINT13
#define pin_ENC_V_A 4  // PD4 PCINT20
#define pin_ENC_V_B 5  // PD5 PCINT21
#define pin_ENC_C_A 6  // PD6 PCINT22
#define pin_ENC_C_B 7  // PD7 PCINT23

// Fixed pins for classic ATmega328P/328PB Arduino boards:
// CLK -> PD2 (INT0 / Arduino D2) DISPSNIFF_CLK_PIN_PORTD
// DIN -> PD3 (INT1 / Arduino D3) DISPSNIFF_DIN_PIN_PORTD

#ifdef __cplusplus
}
#endif

#endif