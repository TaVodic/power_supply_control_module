#ifndef PSCM_H
#define PSCM_H

#ifdef __cplusplus
extern "C" {
#endif

#define pin_MCP_CS1 9
#define pin_MCP_CS2 10
#define pin_MOSI    11
#define pin_MISO    12
#define pin_SCK     13
#define pin_LED_V   16
#define pin_LED_C   17
#define pin_SW_V    18
#define pin_SW_C    19
#define pin_ENC_V_A 4
#define pin_ENC_V_B 5
#define pin_ENC_C_A 6
#define pin_ENC_C_B 7

// Fixed pins for classic ATmega328P/328PB Arduino boards:
// CLK -> PD2 (INT0 / Arduino D2) DISPSNIFF_CLK_PIN_PORTD
// DIN -> PD3 (INT1 / Arduino D3) DISPSNIFF_DIN_PIN_PORTD

#ifdef __cplusplus
}
#endif

#endif